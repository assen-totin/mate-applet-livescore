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

gboolean manager_main (livescore_applet *applet, struct match_data *new_match) {
	int i, match_id, league_id;
	gboolean flag_have_match = FALSE;
	gboolean flag_unused = FALSE;
	gboolean flag_have_league = FALSE;
	char ntf_text[128], ntf_title[128];

	// Do we have this match?
	for (i=0; i < applet->all_matches_counter; i++) {
		if (!strcmp(&applet->all_matches[i].team_home[0], new_match->team_home) && !strcmp(&applet->all_matches[i].team_away[0], new_match->team_away)) {
			flag_have_match = TRUE;
			match_id = i;
			break;
		}
	}

	// If we have it, check events
	if (flag_have_match) {
		// Has score changed?
		if ((applet->all_matches[match_id].score_home + applet->all_matches[match_id].score_away) != (new_match->score_home + new_match->score_away)) {
			applet->all_matches[match_id].score_home = new_match->score_home;
			applet->all_matches[match_id].score_away = new_match->score_away;

			sprintf(&ntf_title[0], "%s vs. %s", &applet->all_matches[match_id].team_home[0], &applet->all_matches[match_id].team_away[0]);

			sprintf(&ntf_text[0], "%s %u:%u", _("GOAL! Score now is "), applet->all_matches[match_id].score_home, applet->all_matches[match_id].score_away);

			push_notification(&ntf_title[0], &ntf_text[0], NULL);

			return TRUE;
		}

		// Has status changed?
		if (applet->all_matches[match_id].status < new_match.status) {
			applet->all_matches[match_id].status = new_match.status;

			sprintf(&ntf_title[0], "%s vs. %s", &applet->all_matches[match_id].team_home[0], &applet->all_matches[match_id].team_away[0]);

			if (new_match.status == MATCH_FIRST_TIME) 
				sprintf(&ntf_text[0], "%s", _("The game commences."));
			else if (new_match.status == MATCH_HALF_TIME)
				sprintf(&ntf_text[0], "%s %u:%u", _("Half time at"), applet->all_matches[match_id].score_home, applet->all_matches[match_id].score_away);
			else if (new_match.status == MATCH_SECOND_TIME)
				sprintf(&ntf_text[0], "%s %u:%u", _("Second time commences at"), applet->all_matches[match_id].score_home, applet->all_matches[match_id].score_away);
			else if (new_match.status == MATCH_EXTRA_TIME)
				sprintf(&ntf_text[0], "%s %u:%u", _("Extra time commences at"), applet->all_matches[match_id].score_home, applet->all_matches[match_id].score_away);

			else if (new_match.status == MATCH_FULL_TIME)
				sprintf(&ntf_text[0], "%s %u:%u", _("Full time at"), applet->all_matches[match_id].score_home, applet->all_matches[match_id].score_away);

			push_notification(&ntf_title[0], &ntf_text[0], NULL);
			return TRUE;
		}

		// Has the time changed?
		if (applet->all_matches[match_id].match_time < new_match.match_time) {
			applet->all_matches[match_id].match_time = new_match.match_time;
		}
	}
	// If we don't have it, add it
	else {
		// TODO: Do we have this league? Check and add if not.

		// Find unused slot
		for (i=0; i < applet->all_matches_counter; i++) {
			if (!applet->all_matches[i].used) {
				flag_unused = TRUE;
				match_id = i;
				break;
			}
		}

		// If no slots, add one
		if (!flag_unused) {
			void *_tmp = realloc(applet->all_matches, (applet->all_matches_counter + 1) * sizeof(struct match_data));
			applet->all_matches = (struct match_data *) _tmp;
			applet->all_matches_counter++;
			match_id = applet->all_matches_counter - 1;
		}
	
		// Copy values from new match
		applet->all_matches[match_id].used = TRUE;
		applet->all_matches[match_id].league_id = league_id;
		applet->all_matches[match_id].score_home = new_match->score_home;
		applet->all_matches[match_id].score_away = new_match->score_away;
		sprintf(&applet->all_matches[match_id].team_home[0], "%s", &new_match->team_home[0]);
		sprintf(&applet->all_matches[match_id].team_away[0], "%s", &new_match->team_away[0]);
		sprintf(&applet->all_matches[match_id].league_name[0], "%s", &new_match->league_name[0]);
		applet->all_matches[match_id].status = new_match->status;
		applet->all_matches[match_id].start_time = new_match->start_time;
		applet->all_matches[match_id].match_time = new_match->match_time;
	}

	return TRUE;
}


