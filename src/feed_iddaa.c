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
#include "applet.h"
#include "feed_iddaa.h"

void iddaa_split_score(char *s, int *home, int *away) {
	if (!strstr(s, "?")) {
		*home = atoi(trim(strtok(s, "-")));
		*away = atoi(trim(strtok(NULL, "-")));
	}
}

gboolean iddaa_is_half_time(char *s) {
	if (strstr(s, "HT"))
		return TRUE;
	return FALSE;
}

gboolean iddaa_is_full_time(char *s) {
        if (strstr(s, "FT"))
                return TRUE;
        return FALSE;
}

gboolean iddaa_is_playing(char *s, int *res) {
	if (strstr(s, "'")) {
		*res = atoi(strtok(s, "'"));
		return TRUE;
	}
	return FALSE;
}

gboolean iddaa_is_future(char *s) {
        if (strstr(s, ":")) {
                return TRUE;
        }
        return FALSE;
}

time_t iddaa_convert_time(char *s) {
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

void iddaa_build_match(livescore_applet *applet, iddaa_match_data *iddaa_match) {
	match_data new_match;

//char dbg[1024];
//sprintf(&dbg[0], "Called build for match %s - %s, league %s", &iddaa_match->team_home[0], &iddaa_match->team_away[0], &iddaa_match->league_name[0]);
//debug(&dbg[0]);

	if (strlen(&iddaa_match->team_home[0]) < 2)
		return;

        sprintf(&new_match.league_name[0], "%s", trim(&iddaa_match->league_name[0]));
        sprintf(&new_match.team_home[0], "%s", trim(&iddaa_match->team_home[0]));
        sprintf(&new_match.team_away[0], "%s", trim(&iddaa_match->team_away[0]));
        new_match.score_home = iddaa_match->score_home;
        new_match.score_away = iddaa_match->score_away;

	if (iddaa_is_half_time(trim(&iddaa_match->match_time[0])))
		new_match.status = MATCH_HALF_TIME;
	else if (iddaa_is_full_time(trim(&iddaa_match->match_time[0])))
		new_match.status = MATCH_FULL_TIME;
	else if (iddaa_is_future(trim(&iddaa_match->match_time[0]))) {
		new_match.status = MATCH_NOT_COMMENCED;
		new_match.start_time = iddaa_convert_time(&iddaa_match->match_time[0]);
	}
        else if (iddaa_is_playing(trim(&iddaa_match->match_time[0]), &new_match.match_time)) {
                if (new_match.match_time < 45)
                	new_match.status = MATCH_FIRST_TIME;
                else if (new_match.match_time < 90)
                        new_match.status = MATCH_SECOND_TIME;
                else
                        new_match.status = MATCH_EXTRA_TIME;
        }
        else {
                 // Something went wrong... skip this match
                 iddaa_match->skip = TRUE;
        }

	// Feed to manager
        if (!iddaa_match->skip) {
//sprintf(&dbg[0], "Calling manager for match %s - %s, league %s", &iddaa_match->team_home[0], &iddaa_match->team_away[0], &iddaa_match->league_name[0]);
//debug(&dbg[0]);
                 manager_main(applet, &new_match);
        }
 
	iddaa_match->skip = FALSE;

}

void iddaa_walk_tree(livescore_applet *applet, xmlNode * a_node, iddaa_match_data *iddaa_match) {
	xmlNode *cur_node = NULL;
	xmlAttr *cur_attr = NULL;

	for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
		if (cur_node->content && (strlen(cur_node->content) > 1)) {
			if (iddaa_match->stage == IDDAA_PARSING_LEAGUE) {
				//memset(&iddaa_match->league_name[0], '\0', 256);
				strcat(&iddaa_match->league_name[0], cur_node->content);
			}
			else if (iddaa_match->stage == IDDAA_PARSING_TIME) {
				memset(&iddaa_match->match_time[0], '\0', 256);
				strcat(&iddaa_match->match_time[0], cur_node->content);
			}
			else if (iddaa_match->stage == IDDAA_PARSING_HOME) {
				memset(&iddaa_match->team_home[0], '\0', 256);
				strcat(&iddaa_match->team_home[0], cur_node->content);
			}
			else if (iddaa_match->stage == IDDAA_PARSING_SCORE) {
				iddaa_match->score_home = 0;
				iddaa_match->score_away = 0;
				memset(&iddaa_match->score[0], '\0', 32);
				strcat(&iddaa_match->score[0], cur_node->content);
				if(strlen(&iddaa_match->score[0]) > 2)
					iddaa_split_score(&iddaa_match->score[0], &iddaa_match->score_home, &iddaa_match->score_away);
			}
			else if (iddaa_match->stage == IDDAA_PARSING_AWAY) {
				memset(&iddaa_match->team_away[0], '\0', 256);
				strcat(&iddaa_match->team_away[0], cur_node->content);
				// Flush last match
				iddaa_build_match(applet, iddaa_match);
				iddaa_match->stage = IDDAA_PARSING_UNKNOWN;
			}
			else
				debug(cur_node->content);

		}
		for (cur_attr = cur_node->properties; cur_attr; cur_attr = cur_attr->next) {
			if (!strcmp(cur_attr->name, "class")) {
				if (!strcmp(cur_attr->children->content, "livescore-table")) {
					iddaa_match->stage = IDDAA_PARSING_LEAGUE;
					memset(&iddaa_match->league_name[0], '\0', 256);
				}
				else if (!strcmp(cur_attr->children->content, "live_time")) 
					iddaa_match->stage = IDDAA_PARSING_TIME;
				else if (!strcmp(cur_attr->children->content, "live_home")) 
					iddaa_match->stage = IDDAA_PARSING_HOME;
				else if (!strcmp(cur_attr->children->content, "live_score")) 
					iddaa_match->stage = IDDAA_PARSING_SCORE;
				else if (!strcmp(cur_attr->children->content, "live_away")) 
					iddaa_match->stage = IDDAA_PARSING_AWAY;
			}
		}

		iddaa_walk_tree(applet, cur_node->children, iddaa_match);
	}
}

int feed_iddaa_main(livescore_applet *applet) {
        iddaa_match_data iddaa_match;

        memset(&iddaa_match.match_time[0], '\0', sizeof(iddaa_match.match_time));
        memset(&iddaa_match.team_home[0], '\0', sizeof(iddaa_match.team_home));
        memset(&iddaa_match.team_away[0], '\0', sizeof(iddaa_match.team_away));
        memset(&iddaa_match.score[0], '\0', sizeof(iddaa_match.score));
        iddaa_match.score_home = 0;
        iddaa_match.score_away = 0;
        iddaa_match.stage = -1;
        iddaa_match.skip = FALSE;

        int res = get_url(IDDAA_URL, IDDAA_USER_AGENT, IDDAA_FILENAME);

        if (!res) {
                htmlDocPtr parser = htmlReadFile(IDDAA_FILENAME, IDDAA_CHARSET, HTML_PARSE_NOBLANKS | HTML_PARSE_NOIMPLIED | HTML_PARSE_COMPACT);
                iddaa_walk_tree(applet, xmlDocGetRootElement(parser), &iddaa_match);
        }

        return 1;
}
