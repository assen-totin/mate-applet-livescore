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

int feed_iddaa_main(struct livescore_applet *applet) {
	struct iddaa_match_data iddaa_match;

        memset(&iddaa_match.the_time[0], '\0', sizeof(iddaa_match.the_time));
        memset(&iddaa_match.team_home[0], '\0', sizeof(iddaa_match.team_home));
        memset(&iddaa_match.team_away[0], '\0', sizeof(iddaa_match.team_away));
        memset(&iddaa_match.score[0], '\0', sizeof(iddaa_match.score));

	int res = get_url(IDDAA_URL, IDDAA_USER_AGENT, IDDAA_FILENAME);

	if (!res) {
	        htmlDocPtr parser = htmlReadFile(IDDAA_FILENAME, IDDAA_CHARSET, HTML_PARSE_NOBLANKS | HTML_PARSE_NOIMPLIED | HTML_PARSE_COMPACT);
	        walkTree(applet, xmlDocGetRootElement(parser), &iddaa_match);
        	return 0;
	}

	return 1;
}


void split_score(char *s, int *home, int *away) {
	if (!strstr(s, "?")) {
		*home = atoi(trim(strtok(s, "-")));
		*away = atoi(trim(strtok(NULL, "-")));
	}
}

void walkTree(struct livescore_applet *applet, xmlNode * a_node, struct iddaa_match *iddaa_match) {
	xmlNode *cur_node = NULL;
	xmlAttr *cur_attr = NULL;
	for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
		if (cur_node->content && (strlen(cur_node->content) > 1)) {
			if (iddaa_match->stage == IDDAA_PARSING_LEAGUE)
				strcat(&iddaa_match->league_name[0], cur_node->content);
			else if (iddaa_match->stage == IDDAA_PARSING_TIME)
				strcat(&iddaa_match->match_time[0], cur_node->content);
			else if (iddaa_match->stage == IDDAA_PARSING_HOME) 
				strcat(&iddaa_match->team_home[0], cur_node->content);
			else if (iddaa_match->stage == IDDAA_PARSING_SCORE)
				strcat(&iddaa_match->score[0], cur_node->content);
			else if (iddaa_match->stage == IDDAA_PARSING_AWAY)
				strcat(&iddaa_match->team_away[0], cur_node->content);

		}
		for (cur_attr = cur_node->properties; cur_attr; cur_attr = cur_attr->next) {
			if (!strcmp(cur_attr->name, "class")) {
				if (!strcmp(cur_attr->children->content, "livescore-table")) {
					memset(&iddaa_match->league_name[0], '\0', 256);
					iddaa_match->stage = IDDAA_PARSING_LEAGUE;
				}
				else if (!strcmp(cur_attr->children->content, "live_time")) {
					iddaa_match->stage = IDDAA_PARSING_TIME;
				
					if(strlen(&iddaa_match->score[0]) > 2) {
						split_score(&iddaa_match->score[0], &iddaa_match->score_home, &iddaa_match->score_away);

						struct match_data new_match;
						sprintf(&new_match.league[0], "%s", trim(&iddaa_match->league_name[0])); 
						sprintf(&new_match.team_home[0], "%s", trim(&iddaa_match->team_home[0]));
						sprintf(&new_match.team_away[0], "%s", trim(&iddaa_match->team_away[0])); 
						new_match.score_home = iddaa_match->score_home;
						new_match.score_away = iddaa_match->score_away;

						// TODO: add start_time, match_time, status
						// Match time - 23' or 16:00 or FT or HT
						
						// Start time

						// Status
						
						// Feed to manager
						manager_main(applet, &new_match);

						memset(&iddaa_match->match_time[0], '\0', 256);
						memset(&iddaa_match->team_home[0], '\0', 256);
						memset(&iddaa_match->team_away[0], '\0', 256);
						memset(&iddaa_match->score[0], '\0', 256);
					}
				}
				else if (!strcmp(cur_attr->children->content, "live_home")) {
					iddaa_match.stage = IDDAA_PARSING_HOME;
				}
				else if (!strcmp(cur_attr->children->content, "live_score")) {
					iddaa_match.stage = IDDAA_PARSING_SCORE;
				}
				else if (!strcmp(cur_attr->children->content, "live_away")) {
					iddaa_match.stage = IDDAA_PARSING_AWAY;
				}
			}	
		}

		walkTree(applet, cur_node->children, iddaa_match);
	}
}

