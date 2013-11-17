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

void queue_notification (livescore_applet *applet, gchar *title, gchar *body, int image, gboolean block) {
	notif_data *notification = malloc(sizeof(notif_data));
	sprintf(notification->title, "%s", title);
	sprintf(notification->body, "%s", body);
	notification->image = image;
	notification->block = block;
	fifo_add(applet->notif_queue, notification);
}


gboolean manager_populate_feed(livescore_applet *applet, gchar *selected_feed, gboolean flag_replace) {
	char selected_feed_so[1024];
	void *new_handle;
	void (*new_feed_main)(match_data **, int *);

	// Test dlopen() of feed provider
	sprintf(&selected_feed_so[0], "%s/%s/%s", LIBDIR, PACKAGE, selected_feed);
	new_handle = dlopen(&selected_feed_so[0], RTLD_NOW|RTLD_GLOBAL);
	if (!new_handle)
		return FALSE;

	*(void**)(&new_feed_main) = dlsym(applet->feed_handle,"feed_main");
	if (!new_feed_main)
		return FALSE;

	// Do the dlopen()
	if (new_handle)
		dlclose(new_handle);
	if (applet->feed_handle)
		dlclose(applet->feed_handle);
	applet->feed_handle = dlopen(&selected_feed_so[0], RTLD_NOW|RTLD_GLOBAL);
	*(void**)(&applet->feed_main) = dlsym(applet->feed_handle,"feed_main");

	// If replacing old provider, clear saved info on expanded and favourite leagues
	if (flag_replace) {
		char value1[256], value2[256];
		sprintf(&value1[0], "\"0\"");
		sprintf(&value2[0], "\"%s\"", selected_feed);
#ifdef HAVE_MATE
		g_settings_set_string(applet->gsettings, APPLET_GSETTINGS_KEY_EXP, &value1[0]);
		g_settings_set_string(applet->gsettings, APPLET_GSETTINGS_KEY_FAV, &value1[0]);
		g_settings_set_string(applet->gsettings, APPLET_GSETTINGS_KEY_FEED, &value2[0]);
#elif HAVE_GNOME_2
		panel_applet_gconf_set_string(PANEL_APPLET(applet->applet), APPLET_GSETTINGS_KEY_EXP, &value1[0], NULL);
		panel_applet_gconf_set_string(PANEL_APPLET(applet->applet), APPLET_GSETTINGS_KEY_FAV, &value1[0], NULL);
		panel_applet_gconf_set_string(PANEL_APPLET(applet->applet), APPLET_GSETTINGS_KEY_FEED, &value2[0], NULL);
#endif
	}

	return TRUE;
}


int manager_timer(livescore_applet *applet) {
	int i, j;
	gboolean league_has_matches = FALSE;
	time_t now = time(NULL);
	div_t q;

	// Show pending notifications
	for (i=0; i<3; i++) {
		if (!fifo_is_empty(applet->notif_queue)) {
			notif_data *notification = fifo_remove(applet->notif_queue);
			if (notification) {
				if (notification->image == NOTIF_SHOW_IMAGE_GOAL)
					show_notification(notification->title, notification->body, applet->notif_image_goal);
				else if (notification->image == NOTIF_SHOW_IMAGE_WHISTLE)
					show_notification(notification->title, notification->body, applet->notif_image_whistle);
				else
					show_notification(notification->title, notification->body, NULL);

				if (notification->block)
					break;

				free(notification);
			}
		}
		else
			break;
	}

	// Is it time for clean-up?
	q = div(now, 1000);
	if (q.rem < 10) {
		// Remove all matches older than 36 hours
		for (i=0; i < applet->all_matches_counter; i++) {
			if ((now - applet->all_matches[i].start_time) > APPLET_KEEP_TIME_MATCH) {
				applet->all_matches[i].used = FALSE;
				for (j=0; j < applet->all_goals_counter; j++) {
					if (applet->all_goals[j].match_id == i)
						applet->all_goals[j].used = FALSE;
				}
			}
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

	// Is it time to call parser?
	q = div(now, 60);
	if (q.rem < 10) {
		// Call feed
		if (applet->feed_main) {
			match_data *feed_matches = malloc(sizeof(match_data));
			int feed_matches_counter = 0;
			(void) applet->feed_main(&feed_matches, &feed_matches_counter);
			for (i=0; i < feed_matches_counter; i++)
				manager_main(applet, &feed_matches[i]);
			free(feed_matches);

			// Rebuild model for GUI
			gui_update_model(applet);
			gui_update_model_goals(applet);
		}
	}

	return 1;
}


void manager_add_goal(livescore_applet *applet, int match_id, int match_time, int match_time_added, char *ntf_text_score, gboolean flag_goal_home) {
	int i, goal_id, goals_found = 0;
	int goals_before = applet->all_matches[match_id].score_home + applet->all_matches[match_id].score_away;
	gboolean flag_need_new_goal = TRUE;

        for (i=0; i < applet->all_goals_counter; i++) {
        	if (applet->all_goals[i].used && (applet->all_goals[i].match_id == match_id)) {
                	goals_found++;
                        continue;
                }
                if (!applet->all_goals[i].used && (goals_found == goals_before)) {
                        flag_need_new_goal = FALSE;
	                        goal_id = i;
                                break;
                }
        }

        if (flag_need_new_goal) {
                void *_tmp = realloc(applet->all_goals, (applet->all_goals_counter + 1) * sizeof(goal_data));
                applet->all_goals = (goal_data *) _tmp;
                applet->all_goals_counter++;
                goal_id = applet->all_goals_counter - 1;
                applet->all_goals[goal_id].goal_id = goal_id;
        }

	if (flag_goal_home) {
		applet->all_matches[match_id].score_home++;
		sprintf(ntf_text_score, "%s %s%s %u:%u", _("GOAL for"), &applet->all_matches[match_id].team_home[0], _("! Score now is"), applet->all_matches[match_id].score_home, applet->all_matches[match_id].score_away);
	}
	else {
		applet->all_matches[match_id].score_away++;
		sprintf(ntf_text_score, "%s %s%s %u:%u", _("GOAL for"), &applet->all_matches[match_id].team_away[0], _("! Score now is"), applet->all_matches[match_id].score_home, applet->all_matches[match_id].score_away);
	}

        applet->all_goals[goal_id].used = TRUE;
        applet->all_goals[goal_id].match_id = match_id;
        applet->all_goals[goal_id].score_home = applet->all_matches[match_id].score_home;
        applet->all_goals[goal_id].score_away = applet->all_matches[match_id].score_away;
        applet->all_goals[goal_id].match_time = match_time;
        applet->all_goals[goal_id].match_time_added = match_time_added;
        applet->all_goals[goal_id].time_added = time(NULL);
}

gboolean manager_main (livescore_applet *applet, match_data *new_match) {
	int i, j, league_id, match_id;
	gboolean flag_have_match = FALSE;
	gboolean flag_unused = FALSE;
	gboolean flag_have_league = FALSE;
	gboolean flag_ntf_score = FALSE;
	gboolean flag_ntf_status = FALSE;
	gboolean flag_ntf_status_first = FALSE;
	gboolean flag_need_new_goal = TRUE;
	time_t now = time(NULL);
	char ntf_text_status[256], ntf_title_status[256], ntf_text_score[10][256], ntf_title_score[256];
	int scored_home = 0, scored_away = 0;

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
		scored_home = new_match->score_home - applet->all_matches[match_id].score_home;
		scored_away = new_match->score_away - applet->all_matches[match_id].score_away;

		if ((scored_home > 0) || (scored_away > 0)) {
			// Add the goal(s)
			for (i=0; i < scored_home; i++)
				manager_add_goal(applet, match_id, new_match->match_time, new_match->match_time_added, &ntf_text_score[i][0], TRUE);
			for (i=scored_home; i < (scored_home + scored_away); i++)
				manager_add_goal(applet, match_id, new_match->match_time, new_match->match_time_added, &ntf_text_score[i][0], FALSE);

			// Prepare notification(s)
			if (applet->all_leagues[applet->all_matches[match_id].league_id].favourite) {
				sprintf(&ntf_title_score[0], "%s vs. %s", &applet->all_matches[match_id].team_home[0], &applet->all_matches[match_id].team_away[0]);
				flag_ntf_score = TRUE;
			}
		}

		// Has status changed?
		if (applet->all_matches[match_id].status < new_match->status) {
			applet->all_matches[match_id].status = new_match->status;

			if (applet->all_leagues[applet->all_matches[match_id].league_id].favourite) {
				sprintf(&ntf_title_status[0], "%s vs. %s", &applet->all_matches[match_id].team_home[0], &applet->all_matches[match_id].team_away[0]);

				if (new_match->status == MATCH_FIRST_TIME) {
					sprintf(&ntf_text_status[0], "%s", _("The game commences."));
					flag_ntf_status_first = TRUE;
				}
				else if (new_match->status == MATCH_HALF_TIME) 
					sprintf(&ntf_text_status[0], "%s %u:%u", _("Half time at"), applet->all_matches[match_id].score_home, applet->all_matches[match_id].score_away);
				
				else if (new_match->status == MATCH_SECOND_TIME)
					sprintf(&ntf_text_status[0], "%s %u:%u", _("Second time commences at"), applet->all_matches[match_id].score_home, applet->all_matches[match_id].score_away);
				else if (new_match->status == MATCH_EXTRA_TIME)
					sprintf(&ntf_text_status[0], "%s %u:%u", _("Extra time commences at"), applet->all_matches[match_id].score_home, applet->all_matches[match_id].score_away);

				else if (new_match->status == MATCH_FULL_TIME)
					sprintf(&ntf_text_status[0], "%s %u:%u", _("Full time at"), applet->all_matches[match_id].score_home, applet->all_matches[match_id].score_away);
				flag_ntf_status = TRUE;
			}
		}

		// Has the time changed?
		if (applet->all_matches[match_id].match_time != new_match->match_time) 
			applet->all_matches[match_id].match_time = new_match->match_time;
		if (applet->all_matches[match_id].match_time_added != new_match->match_time_added)
			 applet->all_matches[match_id].match_time_added = new_match->match_time_added;

		// Queue notifications: always queue first goal changes unless the status change is beginning of game
		if (flag_ntf_status_first) {
			if (flag_ntf_status)
				queue_notification(applet, &ntf_title_status[0], &ntf_text_status[0], NOTIF_SHOW_IMAGE_WHISTLE, FALSE);
			if (flag_ntf_score) {
				for (i=0; i < (scored_home + scored_away); i++) {
					if (i == 0)
						queue_notification(applet, &ntf_title_score[0], &ntf_text_score[i][0], NOTIF_SHOW_IMAGE_GOAL, FALSE);
					else
						queue_notification(applet, &ntf_title_score[0], &ntf_text_score[i][0], NOTIF_SHOW_IMAGE_GOAL, TRUE);
				}
			}
		}
		else {
			if (flag_ntf_score) {
				for (i=0; i < (scored_home + scored_away); i++) {
					if (i == 0)
						queue_notification(applet, &ntf_title_score[0], &ntf_text_score[i][0], NOTIF_SHOW_IMAGE_GOAL, FALSE);
					else
						queue_notification(applet, &ntf_title_score[0], &ntf_text_score[i][0], NOTIF_SHOW_IMAGE_GOAL, TRUE);
				}
			}
			if (flag_ntf_status)
				queue_notification(applet, &ntf_title_status[0], &ntf_text_status[0], NOTIF_SHOW_IMAGE_WHISTLE, FALSE);
		}

		return TRUE;
	}
	// If we don't have it, add it
	else {
		// If it is too far in the future, skip it
		if (((new_match->start_time - now) > APPLET_KEEP_TIME_MATCH) && (new_match->status == MATCH_NOT_COMMENCED))
			return TRUE;

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
		applet->all_matches[match_id].match_time_added = new_match->match_time_added;

		// If the match is ongoing, notify about this:
		if (applet->all_leagues[applet->all_matches[match_id].league_id].favourite) {
			if ((new_match->status > MATCH_NOT_COMMENCED) && (new_match->status < MATCH_FULL_TIME)) {
				sprintf(&ntf_title_score[0], "%s vs. %s", &applet->all_matches[match_id].team_home[0], &applet->all_matches[match_id].team_away[0]);
				sprintf(&ntf_text_score[0][0], "%u' %u:%u", new_match->match_time, new_match->score_home, new_match->score_away);		
				queue_notification(applet, &ntf_title_score[0], &ntf_text_score[0][0], NOTIF_SHOW_IMAGE_GOAL, FALSE);
			}
		}

//sprintf(&dbg[0], "Registered match: %s - %s", &new_match->team_home[0], &new_match->team_away[0]);
//debug(&dbg[0]);
	}

	return TRUE;
}


