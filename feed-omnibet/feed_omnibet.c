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
#include "feed_omnibet.h"

char *omnibet_load_file(char *filename) {
	FILE *fp_in = fopen (filename, "r");
	if (!fp_in)
		return NULL;

	int counter = 1;

	char *output = malloc(1024);
	char *tmp = malloc(1024);

	if (!output || !tmp) {
		fclose(fp_in);
		return NULL;
	}

	memset(output, '\0', 1024);

	while (fgets(tmp, 1023, fp_in)) {
		// Append the chunk
		void *_tmp = realloc(output, (counter * 1024));
		if (!_tmp) {
			fclose(fp_in);
			printf("realloc failed!");
			return NULL;
		}
		output = (char *)_tmp;

		strcat(output, tmp);
		counter++;
	}

	free(tmp);

	fclose(fp_in);

	return output;
}

char *omnibet_replace(char *input, char *delim, char *replace) {
debug("CALLED REPLACE...");
        // Calc how many bytes to add 
        int counter = 0;
	char *match;

        char *in = (char *)malloc(strlen(input) + 1);
	if (! in)
		return NULL;
	strcpy(in, input);
        char *in_orig = in;

        while (1) {
                match = strstr(in, delim);
                if (match) {
                        in = match + strlen(delim);
                        counter++;
                }
                else
                        break;
        }

        int bytes = counter * (strlen(replace) - strlen(delim));
	// Ensure we can fit the original input even if no matches were found
	if (bytes < 0)
		bytes = 1;
	else
		bytes++;
debug_i(bytes);

        char *output = (char *)malloc(strlen(input) + bytes);
	if (!output || !in)
		return FALSE;
	memset(output, '\0', strlen(input) + bytes);

        in = in_orig;

        while (1) {
                match = strstr(in, delim);
                if (match) {
                        strncat(output, in, match - in);
			in = match + strlen(delim);
                        if (strlen(replace) > 0)
                                strcat(output, replace);
                }
                else {
                        strcat(output, in);
                        break;
                }
        }

        free(in_orig);

        return output;
}

gboolean omnibet_is_cancelled(char *s) {
	if (strstr(s, "Postp."))
		return TRUE;
	return FALSE;
}

gboolean omnibet_no_info(char *s) {
	if (strstr(s, "NIY"))
		return TRUE;
	return FALSE;
}

gboolean omnibet_is_half_time(char *s) {
	if (strstr(s, "HT"))
		return TRUE;
	return FALSE;
}

gboolean omnibet_is_full_time(char *s) {
	if (strstr(s, "FT") || strstr(s, "FAP") || strstr(s, "FAE"))
		return TRUE;
	return FALSE;
}

gboolean omnibet_is_playing(char *s, int *match_time, int *match_time_added) {
	char *c;

	if (strstr(s, "+")) {
		*match_time = atoi(trim(strtok(s, "+")));
		if (c = trim(strtok(NULL, "+")))
			*match_time_added = atoi(c);
	}
	else {
		*match_time = atoi(s);
		*match_time_added = 0;
	}
	return TRUE;
}

gboolean omnibet_is_future(char *s) {
	if (strstr(s, ":")) {
		return TRUE;
	}
	return FALSE;
}

void omnibet_split_score(char *s, int *home, int *away) {
        if (strstr(s, "-")) {
                *home = atoi(trim(strtok(s, "-")));
                *away = atoi(trim(strtok(NULL, "-")));
        }
}

time_t omnibet_convert_time(char *s) {
	struct tm now, *now_p;

	time_t ts = time(NULL);
	now_p = gmtime(&ts);

	now = *now_p;
	now.tm_hour = atoi(strtok(s, ":"));
	now.tm_min = atoi(strtok(NULL, ":" ));
	now.tm_sec = 0;

	// mktime treats 'now' as localtime, but we have it UTC.
	// NOTE: timegm() is not POSIX-compliant, hence not portable!
	// See 'man timegm' for POSIX-compliant replacecement function.
	//return mktime(&now);
	return timegm(&now);
}

void omnibet_build_match(omnibet_match_data *omnibet_match, match_data **feed_matches, int *feed_matches_counter) {
	int match_status, match_time, match_time_added;
	time_t start_time = time(NULL);

	if (omnibet_is_cancelled(trim(&omnibet_match->match_time[0]))) {
		return;
	}

	if (omnibet_no_info(trim(&omnibet_match->match_time[0]))) {
		return;
	}

	if (omnibet_is_half_time(trim(&omnibet_match->match_time[0]))) {
		match_status = MATCH_HALF_TIME;
		match_time = 45;
		match_time_added = 0;
	}
	else if (omnibet_is_full_time(trim(&omnibet_match->match_time[0]))) {
		match_status = MATCH_FULL_TIME;
		match_time = 90;
		match_time_added = 0;
	}
	else if (omnibet_is_future(trim(&omnibet_match->match_time[0]))) {
		match_status = MATCH_NOT_COMMENCED;
		omnibet_match->score_home = 0;
		omnibet_match->score_away = 0;
		start_time = omnibet_convert_time(&omnibet_match->match_time[0]);
	}
	else if (omnibet_is_playing(trim(&omnibet_match->match_time[0]), &match_time, &match_time_added)) {
		if (match_time < 46)
			match_status = MATCH_FIRST_TIME;
		else if (match_time < 91)
			match_status = MATCH_SECOND_TIME;
		else
			match_status = MATCH_EXTRA_TIME;
	}
	else {
		// Something went wrong... skip this match
		omnibet_match->skip = TRUE;
	}

	// Add match to list
	if (!omnibet_match->skip) {
		int index = *feed_matches_counter;
		void *_tmp = realloc(*feed_matches, (index + 1) * sizeof(match_data));
		*feed_matches = (match_data *) _tmp;
		match_data *tmp_matches = *feed_matches;
		snprintf(&tmp_matches[index].league_name[0], sizeof(tmp_matches[index].league_name), "%s", trim(&omnibet_match->league_name[0]));
		snprintf(&tmp_matches[index].team_home[0], sizeof(tmp_matches[index].team_home), "%s", trim(&omnibet_match->team_home[0]));
		snprintf(&tmp_matches[index].team_away[0], sizeof(tmp_matches[index].team_away), "%s", trim(&omnibet_match->team_away[0]));
		tmp_matches[index].score_home = omnibet_match->score_home;
		tmp_matches[index].score_away = omnibet_match->score_away;
		tmp_matches[index].status = match_status;
		tmp_matches[index].match_time = match_time;
		tmp_matches[index].match_time_added = match_time_added;
		tmp_matches[index].start_time = start_time;
		(*feed_matches_counter)++;
	}
 
	omnibet_match->skip = FALSE;
}

void omnibet_walk_tree(xmlNode * a_node, omnibet_match_data *omnibet_match, match_data **feed_matches, int *feed_matches_counter) {
	xmlNode *cur_node = NULL;
	xmlAttr *cur_attr = NULL;

	for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
		if (cur_node->content && (strlen(cur_node->content) > 0)) {
			if (omnibet_match->stage == OMNIBET_PARSING_LEAGUE) {
				strcpy(&omnibet_match->league_name[0], cur_node->content);
				omnibet_match->stage = OMNIBET_PARSING_SKIP;
			}
			else if (omnibet_match->stage == OMNIBET_PARSING_HOME) {
				strcpy(&omnibet_match->team_home[0], cur_node->content);
				omnibet_match->stage = OMNIBET_PARSING_SKIP;
			}
			else if (omnibet_match->stage == OMNIBET_PARSING_AWAY) {
				strcpy(&omnibet_match->team_away[0], cur_node->content);
				// Flush last match
				omnibet_build_match(omnibet_match, feed_matches, feed_matches_counter);
				omnibet_match->stage = OMNIBET_PARSING_SKIP;
			}
			else if (omnibet_match->stage == OMNIBET_PARSING_TIME) {
				strcpy(&omnibet_match->match_time[0], cur_node->content);
				omnibet_match->stage = OMNIBET_PARSING_SKIP;
			}
			else if (omnibet_match->stage == OMNIBET_PARSING_SCORE) {
                                omnibet_match->score_home = 0;
                                omnibet_match->score_away = 0;
                                memset(&omnibet_match->score[0], '\0', 32);
                                strcat(&omnibet_match->score[0], cur_node->content);
                                if(strlen(&omnibet_match->score[0]) > 2)
                                        omnibet_split_score(&omnibet_match->score[0], &omnibet_match->score_home, &omnibet_match->score_away);
				omnibet_match->stage = OMNIBET_PARSING_SKIP;
			}
		}

		for (cur_attr = cur_node->properties; cur_attr; cur_attr = cur_attr->next) {
			if (!strcmp(cur_attr->name, "style")) {
				if (!strcmp(cur_attr->children->content, "display:inline-block; vertical-align:middle")) {
					omnibet_match->stage = OMNIBET_PARSING_LEAGUE;
					memset(&omnibet_match->league_name[0], '\0', 256);
				}
			}

			else if (!strcmp(cur_attr->name, "width")) {
				if (strstr(cur_attr->children->content, "60")) 
					omnibet_match->stage = OMNIBET_PARSING_TIME;
			}

			else if (!strcmp(cur_attr->name, "align")) {
				if (strstr(cur_attr->children->content, "right"))
					omnibet_match->stage = OMNIBET_PARSING_HOME;
			}

			else if (!strcmp(cur_attr->name, "custom")) {
				if (strstr(cur_attr->children->content, "score")) 
					omnibet_match->stage = OMNIBET_PARSING_SCORE;
				else if (strstr(cur_attr->children->content, "away")) 
					omnibet_match->stage = OMNIBET_PARSING_AWAY;
			}
		}

		omnibet_walk_tree(cur_node->children, omnibet_match, feed_matches, feed_matches_counter);
	}
}


int feed_main(match_data **feed_matches, int *feed_matches_counter) {
debug("CALLED LIB...");
	omnibet_match_data omnibet_match;
	char tmp_file[1024];
	char tmp_file2[1024];
	char tmp_file3[1024];

	memset(&omnibet_match.match_time[0], '\0', sizeof(omnibet_match.match_time));
	memset(&omnibet_match.team_home[0], '\0', sizeof(omnibet_match.team_home));
	memset(&omnibet_match.team_away[0], '\0', sizeof(omnibet_match.team_away));
	omnibet_match.score_home = 0;
	omnibet_match.score_away = 0;
	omnibet_match.stage = -1;
	omnibet_match.skip = FALSE;

	struct passwd *pw = getpwuid(getuid());
	snprintf(&tmp_file[0], sizeof(tmp_file), "%s-%u", OMNIBET_FILENAME, pw->pw_uid);
	snprintf(&tmp_file2[0], sizeof(tmp_file), "%s-%u-a", OMNIBET_FILENAME, pw->pw_uid);
	//snprintf(&tmp_file3[0], sizeof(tmp_file), "%s-%u-prev", OMNIBET_FILENAME, pw->pw_uid);

	//rename(&tmp_file[0], &tmp_file3[0]);

	int res = get_url(OMNIBET_URL, OMNIBET_USER_AGENT, &tmp_file[0]);
	if (!res) {
		char *orig_xml = omnibet_load_file(&tmp_file[0]);
		if (!orig_xml)
			return 0;
	
		char *fixed_xml1 = omnibet_replace(orig_xml, "&nbsp;", "");
		if (!fixed_xml1)
			return 0;

		char *fixed_xml2 = omnibet_replace(fixed_xml1, "&", " and ");
		if (!fixed_xml2) 
			return 0;

		char *fixed_xml3 = omnibet_replace(fixed_xml2, "<strong>", "<strong custom=score>");
		if (!fixed_xml3)
			return 0;

		char *fixed_xml4 = omnibet_replace(fixed_xml3, "<td>", "<td custom=away>");
		if (!fixed_xml4)
			return 0;

		FILE *fp = fopen (&tmp_file2[0], "w");
		if (!fp) {
			printf("Cannot open output file!\n");
			return 0;
		}
		fprintf(fp, "%s\n", fixed_xml4);
		fclose(fp);

		htmlDocPtr parser = htmlReadFile(&tmp_file2[0], OMNIBET_CHARSET, 
			HTML_PARSE_RECOVER |
			//HTML_PARSE_NOBLANKS | 
			HTML_PARSE_NOERROR | 
			HTML_PARSE_NOWARNING |
#ifdef HAVE_MATE
			HTML_PARSE_NOIMPLIED | 
#endif
			HTML_PARSE_COMPACT);
		omnibet_walk_tree(xmlDocGetRootElement(parser), &omnibet_match, feed_matches, feed_matches_counter);

		free(orig_xml);
		free(fixed_xml1);
		free(fixed_xml2);
		free(fixed_xml3);
		free(fixed_xml4);
	}

	return 1;
}

