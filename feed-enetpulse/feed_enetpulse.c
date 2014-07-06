/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 *  USA.
 *
 *  MATE Livescore applet written by Assen Totin <assen.totin@gmail.com>
 *  
 */

#include "../config.h"
#include "../src/applet.h"
#include "feed_enetpulse.h"

gboolean enetpulse_is_half_time(char *s) {
	if (strstr(s, "HT"))
		return TRUE;
	return FALSE;
}

gboolean enetpulse_is_full_time(char *s) {
	if (strstr(s, "FT"))
		return TRUE;
	return FALSE;
}

gboolean enetpulse_is_playing(char *s, int *match_time, int *match_time_added) {
	char *c;

	if (strstr(s, "+")) {
		*match_time = atoi(trim(strtok(s, "+")));
		if (c = trim(strtok(NULL, "+")))
			*match_time_added = atoi(c);
	}
	else {
		*match_time = atoi(strtok(s, "'"));
		*match_time_added = 0;
	}
	return TRUE;

	//return FALSE;
}

gboolean enetpulse_is_future(char *s) {
	if (strstr(s, ":")) {
		return TRUE;
	}
	return FALSE;
}

time_t enetpulse_convert_time(char *s) {
	struct tm now, *now_p;

	time_t ts = time(NULL);
	now_p = gmtime(&ts);

	now = *now_p;
	now.tm_hour = atoi(strtok(s, ":"));
	now.tm_min = atoi(strtok(NULL, ":" ));
	now.tm_sec = 0;

	// mktime treats 'now' as localtime, but we need it UTC.
	// NOTE: timegm() is not POSIX-compliant, hence not portable!
	// See 'man timegm' for POSIX-compliant replacecement function.
	//return mktime(&now);
	return timegm(&now);
}

void enetpulse_build_match(enetpulse_match_data *enetpulse_match, match_data **feed_matches, int *feed_matches_counter) {
	int match_status, match_time, match_time_added;
	time_t start_time = time(NULL);

	//if (strlen(&enetpulse_match->team_home[0]) < 2)
	//	return;

	if (enetpulse_is_half_time(trim(&enetpulse_match->match_time[0]))) {
		match_status = MATCH_HALF_TIME;
		match_time = 45;
		match_time_added = 0;
	}
	else if (enetpulse_is_full_time(trim(&enetpulse_match->match_time[0]))) {
		match_status = MATCH_FULL_TIME;
		match_time = 90;
		match_time_added = 0;
	}
	else if (enetpulse_is_future(trim(&enetpulse_match->match_time[0]))) {
		match_status = MATCH_NOT_COMMENCED;
		start_time = enetpulse_convert_time(&enetpulse_match->match_time[0]);
	}
	else if (enetpulse_is_playing(trim(&enetpulse_match->match_time[0]), &match_time, &match_time_added)) {
		if (match_time < 46)
			match_status = MATCH_FIRST_TIME;
		else if (match_time < 91)
			match_status = MATCH_SECOND_TIME;
		else
			match_status = MATCH_EXTRA_TIME;
	}
	else {
		// Something went wrong... skip this match
		enetpulse_match->skip = TRUE;
	}

	// Add match to list
	if (!enetpulse_match->skip) {
		int index = *feed_matches_counter;
		void *_tmp = realloc(*feed_matches, (index + 1) * sizeof(match_data));
		*feed_matches = (match_data *) _tmp;
		match_data *tmp_matches = *feed_matches;
		snprintf(&tmp_matches[index].league_name[0], sizeof(tmp_matches[index].league_name), "%s", trim(&enetpulse_match->league_name[0]));
		snprintf(&tmp_matches[index].team_home[0], sizeof(tmp_matches[index].team_home), "%s", trim(&enetpulse_match->team_home[0]));
		snprintf(&tmp_matches[index].team_away[0], sizeof(tmp_matches[index].team_away), "%s", trim(&enetpulse_match->team_away[0]));
		tmp_matches[index].score_home = enetpulse_match->score_home;
		tmp_matches[index].score_away = enetpulse_match->score_away;
		tmp_matches[index].status = match_status;
		tmp_matches[index].match_time = match_time;
		tmp_matches[index].match_time_added = match_time_added;
		tmp_matches[index].start_time = start_time;
		(*feed_matches_counter)++;
	}
 
	enetpulse_match->skip = FALSE;
}

void enetpulse_walk_tree(xmlNode * a_node, enetpulse_match_data *enetpulse_match, match_data **feed_matches, int *feed_matches_counter) {
	xmlNode *cur_node = NULL;
	xmlAttr *cur_attr = NULL;

	for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
		if (cur_node->content && (strlen(cur_node->content) > 0)) {
			if (enetpulse_match->stage == ENETPULSE_PARSING_LEAGUE2) {
				strcpy(&enetpulse_match->league_name[0], cur_node->content);
				enetpulse_match->stage = ENETPULSE_PARSING_SKIP;
			}
			else if (enetpulse_match->stage == ENETPULSE_PARSING_HOME) {
				strcpy(&enetpulse_match->team_home[0], cur_node->content);
			}
			else if (enetpulse_match->stage == ENETPULSE_PARSING_AWAY) {
				strcpy(&enetpulse_match->team_away[0], cur_node->content);
				// Flush last match
				enetpulse_build_match(enetpulse_match, feed_matches, feed_matches_counter);
				enetpulse_match->stage = ENETPULSE_PARSING_SKIP;
			}
			else if (enetpulse_match->stage == ENETPULSE_PARSING_TIME) {
				strcpy(&enetpulse_match->match_time[0], cur_node->content);
			}
			else if (enetpulse_match->stage == ENETPULSE_PARSING_SCORE) {
				// Skip if the value is "-"
				if (strcmp(trim(cur_node->content), "-")) {
					enetpulse_match->score_home = atoi(cur_node->content);
				}
			}
			else if (enetpulse_match->stage == ENETPULSE_PARSING_SCORE2) {
				enetpulse_match->score_away = atoi(cur_node->content);
			}
		}

		for (cur_attr = cur_node->properties; cur_attr; cur_attr = cur_attr->next) {
			if (!strcmp(cur_attr->name, "class")) {
				if (!strcmp(cur_attr->children->content, "Heading")) {
					enetpulse_match->stage = ENETPULSE_PARSING_LEAGUE;
					memset(&enetpulse_match->league_name[0], '\0', 256);
				}
			}

			if (!strcmp(cur_attr->name, "style")) {
				if (strstr(cur_attr->children->content, "padding-left: 10px;text-align: left;"))  {
					enetpulse_match->stage = ENETPULSE_PARSING_TIME;
				}
				else if ((!strcmp(cur_attr->children->content, "")) && (enetpulse_match->stage != ENETPULSE_PARSING_SCORE))
					enetpulse_match->stage = ENETPULSE_PARSING_SCORE;
				else if (!strcmp(cur_attr->children->content, ""))
					enetpulse_match->stage = ENETPULSE_PARSING_SCORE2;
			}

			else if (!strcmp(cur_attr->name, "id")) {
				if (strstr(cur_attr->children->content, "home_info_")) 
					enetpulse_match->stage = ENETPULSE_PARSING_HOME;
				else if (strstr(cur_attr->children->content, "away_info_")) 
					enetpulse_match->stage = ENETPULSE_PARSING_AWAY;
				else if (strstr(cur_attr->children->content, "s_info_")) 
					enetpulse_match->stage = ENETPULSE_PARSING_SKIP;
			}

			else if (!strcmp(cur_attr->name, "href")) {
				if ((enetpulse_match->stage == ENETPULSE_PARSING_LEAGUE) && (strstr(cur_attr->children->content, "standings.php")))
					enetpulse_match->stage = ENETPULSE_PARSING_LEAGUE2;
			}
		}

		enetpulse_walk_tree(cur_node->children, enetpulse_match, feed_matches, feed_matches_counter);
	}
}


int feed_main(match_data **feed_matches, int *feed_matches_counter) {
	enetpulse_match_data enetpulse_match;
	char tmp_file[1024];

	memset(&enetpulse_match.match_time[0], '\0', sizeof(enetpulse_match.match_time));
	memset(&enetpulse_match.team_home[0], '\0', sizeof(enetpulse_match.team_home));
	memset(&enetpulse_match.team_away[0], '\0', sizeof(enetpulse_match.team_away));
	enetpulse_match.score_home = 0;
	enetpulse_match.score_away = 0;
	enetpulse_match.stage = -1;
	enetpulse_match.skip = FALSE;

	struct passwd *pw = getpwuid(getuid());
	snprintf(&tmp_file[0], sizeof(tmp_file), "%s-%u", ENETPULSE_FILENAME, pw->pw_uid);
//debug("Getting page...");
	int res = get_url(ENETPULSE_URL, ENETPULSE_USER_AGENT, &tmp_file[0]);
	if (!res) {
//debug("Got page!");
		htmlDocPtr parser = htmlReadFile(&tmp_file[0], ENETPULSE_CHARSET, 
			HTML_PARSE_RECOVER |
			//HTML_PARSE_NOBLANKS | 
			HTML_PARSE_NOERROR | 
			HTML_PARSE_NOWARNING |
#ifdef HAVE_MATE
			HTML_PARSE_NOIMPLIED | 
#endif
			HTML_PARSE_COMPACT);
		enetpulse_walk_tree(xmlDocGetRootElement(parser), &enetpulse_match, feed_matches, feed_matches_counter);
	}

	return 1;
}

