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

void applet_back_change (MatePanelApplet *a, MatePanelAppletBackgroundType type, GdkColor *color, GdkPixmap *pixmap, livescore_applet *applet) {
        /* taken from the TrashApplet */
        GtkRcStyle *rc_style;
        GtkStyle *style;

        /* reset style */
        gtk_widget_set_style (GTK_WIDGET(applet->applet), NULL);
	gtk_widget_set_style (GTK_WIDGET(applet->event_box), NULL);
        rc_style = gtk_rc_style_new ();
        gtk_widget_modify_style (GTK_WIDGET(applet->applet), rc_style);
	gtk_widget_modify_style (GTK_WIDGET(applet->event_box), rc_style);
        g_object_unref (rc_style);

        switch (type) {
                case PANEL_COLOR_BACKGROUND:
                        gtk_widget_modify_bg (GTK_WIDGET(applet->applet), GTK_STATE_NORMAL, color);
			gtk_widget_modify_bg (GTK_WIDGET(applet->event_box), GTK_STATE_NORMAL, color);
                        break;

                case PANEL_PIXMAP_BACKGROUND:
                        style = gtk_style_copy (gtk_widget_get_style (GTK_WIDGET(applet->applet)));
                        if (style->bg_pixmap[GTK_STATE_NORMAL])
                                g_object_unref (style->bg_pixmap[GTK_STATE_NORMAL]);
                        style->bg_pixmap[GTK_STATE_NORMAL] = g_object_ref(pixmap);
                        gtk_widget_set_style (GTK_WIDGET(applet->applet), style);
			gtk_widget_set_style (GTK_WIDGET(applet->event_box), style);
                        g_object_unref (style);
                        break;

                case PANEL_NO_BACKGROUND:
                default:
                        break;
        }

}

void applet_destroy(MatePanelApplet *applet_widget, livescore_applet *applet) {
	g_main_loop_quit(applet->loop);
        g_assert(applet);
	g_free(applet->all_matches);
        g_free(applet);
        return;
}


gboolean applet_main (MatePanelApplet *applet_widget, const gchar *iid, gpointer data) {
	livescore_applet *applet;
	int i;
	char dbg[1024];
	//char ui[24576];

	if (strcmp (iid, APPLET_ID) != 0)
		return FALSE;

	// i18n
	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE_NAME, LOCALEDIR);
	bind_textdomain_codeset(PACKAGE_NAME, "utf-8");
	textdomain (PACKAGE_NAME);

	// Init 
	applet = g_malloc0(sizeof(livescore_applet));
	applet->applet = applet_widget;
	//applet->timestamp = time(NULL);
	applet->all_matches = g_malloc0(sizeof(match_data));
	applet->all_matches->league_id = -1;
	applet->all_matches->score_home = 0;
	applet->all_matches->score_away = 0;
	applet->all_matches->used = FALSE;
	applet->all_matches->status = MATCH_NOT_COMMENCED;
	applet->all_matches_counter = 1;
	applet->all_leagues = g_malloc0(sizeof(league_data));
	applet->all_leagues->league_id = 0;
	applet->all_leagues->used = FALSE;
	applet->all_leagues_counter = 1;

	// Fvourite leagues - via GSettings
	applet->gsettings = g_settings_new_with_path(APPLET_GSETTINGS_SCHEMA, APPLET_GSETTINGS_PATH);
	gchar *fav_leagues = trim_quotes(g_settings_get_string(applet->gsettings, APPLET_GSETTINGS_KEY_FAV));
	char *fav_leagues_1 = strtok(fav_leagues, ",");
	i = 0;
	if (fav_leagues_1) {
		applet->all_leagues[i].league_id = i;
		sprintf(&applet->all_leagues[i].league_name[0], "%s", fav_leagues_1);
		applet->all_leagues[i].used = TRUE;
		applet->all_leagues[i].favourite = TRUE;
		while (fav_leagues_1 = strtok(NULL, ",")) {
			i++;
			void *_tmp = realloc(applet->all_leagues, i * sizeof(league_data));
			applet->all_leagues = (league_data *) _tmp;

			applet->all_leagues[i].league_id = i;
			sprintf(&applet->all_leagues[i].league_name[0], "%s", fav_leagues_1);
			applet->all_leagues[i].used = TRUE;
			applet->all_leagues[i].favourite = TRUE;
		}
		applet->all_leagues_counter = i;
	}

	// Get an image
	char image_file[1024];
	sprintf(&image_file[0], "%s/%s", APPLET_ICON_PATH, APPLET_ICON_STATIC);
	applet->image = gtk_image_new_from_file (&image_file[0]);

	// Put the image into a container (it needs to receive actions)
	applet->event_box = gtk_event_box_new();
	gtk_container_add (GTK_CONTAINER (applet->event_box), applet->image);

	// Put the container into the applet
        gtk_container_add (GTK_CONTAINER (applet->applet), applet->event_box);

	// Define menu action group
	applet->action_group = gtk_action_group_new ("Livescore_Applet_Actions");
	gtk_action_group_add_actions (applet->action_group, applet_menu_actions, G_N_ELEMENTS (applet_menu_actions), applet);

	// Build menu
	//sprintf(&ui[0], "%s %s %s %s %s", ui1, &applet->ui_recent[0], ui2, &applet->ui_fav[0], ui3);
	//mate_panel_applet_setup_menu(applet->applet, &ui[0], applet->action_group);

	// Signals
        g_signal_connect(G_OBJECT(applet->event_box), "button_press_event", G_CALLBACK (on_left_click), (gpointer)applet);
        g_signal_connect(G_OBJECT(applet->applet), "change_background", G_CALLBACK (applet_back_change), (gpointer)applet);
	g_signal_connect(G_OBJECT(applet->applet), "destroy", G_CALLBACK(applet_destroy), (gpointer)applet);

	// Tooltip
	gtk_widget_set_tooltip_text (GTK_WIDGET (applet->applet), _("Livescore: click to view, right-click to change settings."));

        // Show applet
        gtk_widget_show_all (GTK_WIDGET (applet->applet));

	// Run updates each minute
	// TODO: write wrapper to read actually selected feed and activate it's main function
	g_timeout_add(60000, (GSourceFunc)feed_iddaa_main, (gpointer)applet);

	// Run
        applet->loop = g_main_loop_new (NULL, FALSE);
        g_main_loop_run (applet->loop);

	return TRUE;
}

MATE_PANEL_APPLET_OUT_PROCESS_FACTORY (APPLET_FACTORY, PANEL_TYPE_APPLET, APPLET_NAME, applet_main, NULL)

