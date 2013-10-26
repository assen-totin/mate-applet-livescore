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

gboolean is_league_subscribed (livescore_applet *applet, int league_id) {
	return applet->all_leagues[league_id].favourite;
}


void manager_populate_feed(livescore_applet *applet, int selected_feed) {
	applet->all_feeds = g_malloc0(sizeof(feed_data));
	applet->all_feeds[0].feed_id = 0;
	applet->all_feeds[0].enabled = TRUE;
	sprintf(&applet->all_feeds[0].feed_name[0], "Default");

	applet->all_feeds_counter = 1;

	if (applet->all_feeds[selected_feed].enabled)
		applet->all_feeds[selected_feed].selected = TRUE;

	// TODO: If more than one feed is available and the selected one is no longer used,
	// chose another one to use
}


int manager_timer(livescore_applet *applet) {
	int i, j;
	gboolean league_has_matches = FALSE;
	time_t now;
	div_t q;

	// Is it time for clean-up?
        now = time(NULL);
        q = div(now, 1000);
        if (q.rem < 60) {
		// Remove all matches older than 36 hours
                for (i=0; i < applet->all_matches_counter; i++) {
                        if ((now - applet->all_matches[i].start_time) > 129600)
                                applet->all_matches[i].used = FALSE;
                }
		// Check all leagues which are not selected for notifications and have no matches
		for (i=0; i < applet->all_leagues_counter; i++) {
			if (!applet->all_leagues[i].used)
				continue;
			league_has_matches = FALSE;
			for (j=0; j < applet->all_matches_counter; j++) {
				if (applet->all_matches[j].league_id == j) {
					league_has_matches = TRUE;
					break;
				}
			}
			// No matches and not selected for notifications: remove from list
			if (!league_has_matches && !applet->all_leagues[i].favourite) 
				applet->all_leagues[i].used = FALSE;
			// No matches, but expanded: remove expansion
			if (!league_has_matches && applet->all_leagues[i].expanded)
				applet->all_leagues[i].expanded = FALSE;
		}
        }
	
	// Call parser
	if (applet->all_feeds[0].selected)
		feed_iddaa_main(applet);

	// Rebuild model for GUI
	gui_update_model(applet);

	return 1;
}


gboolean manager_main (livescore_applet *applet, match_data *new_match) {
	int i, match_id, league_id;
	gboolean flag_have_match = FALSE;
	gboolean flag_unused = FALSE;
	gboolean flag_have_league = FALSE;
	char ntf_text[128], ntf_title[128];

//char dbg[1024];
//sprintf(&dbg[0], "Called for match %s - %s", new_match->team_home, new_match->team_away);
//debug(&dbg[0]);

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

			sprintf(&ntf_text[0], "%s %u:%u", _("GOAL! Score now is"), applet->all_matches[match_id].score_home, applet->all_matches[match_id].score_away);

			if (is_league_subscribed(applet, applet->all_matches[match_id].league_id))
				push_notification(&ntf_title[0], &ntf_text[0], NULL);

			return TRUE;
		}

		// Has status changed?
		if (applet->all_matches[match_id].status < new_match->status) {
			applet->all_matches[match_id].status = new_match->status;

			sprintf(&ntf_title[0], "%s vs. %s", &applet->all_matches[match_id].team_home[0], &applet->all_matches[match_id].team_away[0]);

			if (new_match->status == MATCH_FIRST_TIME) 
				sprintf(&ntf_text[0], "%s", _("The game commences."));
			else if (new_match->status == MATCH_HALF_TIME)
				sprintf(&ntf_text[0], "%s %u:%u", _("Half time at"), applet->all_matches[match_id].score_home, applet->all_matches[match_id].score_away);
			else if (new_match->status == MATCH_SECOND_TIME)
				sprintf(&ntf_text[0], "%s %u:%u", _("Second time commences at"), applet->all_matches[match_id].score_home, applet->all_matches[match_id].score_away);
			else if (new_match->status == MATCH_EXTRA_TIME)
				sprintf(&ntf_text[0], "%s %u:%u", _("Extra time commences at"), applet->all_matches[match_id].score_home, applet->all_matches[match_id].score_away);

			else if (new_match->status == MATCH_FULL_TIME)
				sprintf(&ntf_text[0], "%s %u:%u", _("Full time at"), applet->all_matches[match_id].score_home, applet->all_matches[match_id].score_away);

			if (is_league_subscribed(applet, applet->all_matches[match_id].league_id))
				push_notification(&ntf_title[0], &ntf_text[0], NULL);

			return TRUE;
		}

		// Has the time changed?
		if (applet->all_matches[match_id].match_time < new_match->match_time) 
			applet->all_matches[match_id].match_time = new_match->match_time;
		if (applet->all_matches[match_id].match_time_added < new_match->match_time_added)
			 applet->all_matches[match_id].match_time_added = new_match->match_time_added;
	}
	// If we don't have it, add it
	else {
		// League: do we have it; add if not
		for (i=0; i < applet->all_leagues_counter; i++) {
			if (!strcmp(&applet->all_leagues[i].league_name[0], &new_match->league_name[0])) {
				flag_have_league = TRUE;
				league_id = i;
				break;
			}
		}
		if (!flag_have_league) {
			// Find empty slot for the league
			flag_unused = FALSE;
			for (i=0; i < applet->all_leagues_counter; i++) {
				if (!applet->all_leagues[i].used) {
					flag_unused = TRUE;
					league_id = i;
					break;
				}
			}

			if (!flag_unused) {
				// If no slots, add one
				void *_tmp = realloc(applet->all_leagues, (applet->all_leagues_counter + 1) * sizeof(league_data));
				applet->all_leagues = (league_data *) _tmp;
				applet->all_leagues_counter++;
				league_id = applet->all_leagues_counter - 1;
			}

			applet->all_leagues[league_id].league_id = league_id;
			sprintf(&applet->all_leagues[league_id].league_name[0], "%s", &new_match->league_name[0]);
			applet->all_leagues[league_id].used = TRUE;
			applet->all_leagues[league_id].favourite = FALSE;
			applet->all_leagues[league_id].expanded = FALSE;

//sprintf(&dbg[0], "Registered league: %s", &new_match->league_name[0]);
//debug(&dbg[0]);
		}

		// Find empty slot for the match
		flag_unused = FALSE;
		for (i=0; i < applet->all_matches_counter; i++) {
			if (!applet->all_matches[i].used) {
				flag_unused = TRUE;
				match_id = i;
				break;
			}
		}

		// If no slots, add one
		if (!flag_unused) {
			void *_tmp = realloc(applet->all_matches, (applet->all_matches_counter + 1) * sizeof(match_data));
			applet->all_matches = (match_data *) _tmp;
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

//sprintf(&dbg[0], "Registered match: %s - %s", &new_match->team_home[0], &new_match->team_away[0]);
//debug(&dbg[0]);
	}

	return TRUE;
}


