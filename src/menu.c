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


void quitDialogClose(GtkWidget *widget, gpointer data) {
	livescore_applet *applet = data;

        gtk_widget_destroy(applet->dialog_settings);
}


void menu_cb_about (GtkAction *action, livescore_applet *applet) {
        char msg1[1024];

        sprintf(&msg1[0], "%s\n\n%s\n\n%s", _("MATE Livescore Applet"), _("An applet which lets you get football live scores."), _("Assen Totin <assen.totin@gmail.com>"));
        GtkWidget *label = gtk_label_new (&msg1[0]);

        applet->dialog_settings = gtk_dialog_new_with_buttons (_("MATE Livescore Applet"), GTK_WINDOW(applet), GTK_DIALOG_MODAL, NULL);
        GtkWidget *buttonOK = gtk_dialog_add_button (GTK_DIALOG(applet->dialog_settings), GTK_STOCK_OK, GTK_RESPONSE_OK);

        gtk_dialog_set_default_response (GTK_DIALOG (applet->dialog_settings), GTK_RESPONSE_CANCEL);
        gtk_container_add (GTK_CONTAINER (GTK_DIALOG(applet->dialog_settings)->vbox), label);
        g_signal_connect (G_OBJECT(buttonOK), "clicked", G_CALLBACK (quitDialogClose), (gpointer) applet);

        gtk_widget_show_all (GTK_WIDGET(applet->dialog_settings));
}


void menu_cb_feed_combo(GtkWidget *widget, gpointer data) {
        livescore_applet *applet = data;
	int i, feed_id = -1;
	gchar *feed_name = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget));

	for (i=0; i < applet->all_feeds_counter; i++) {
		if (!strcmp(&applet->all_feeds[i].feed_name[0], feed_name)) {
			feed_id = 1;
			break;
		}
	}

	if (feed_id > -1)
		g_settings_set_int(applet->gsettings, APPLET_GSETTINGS_KEY_FEED, feed_id);

	free(feed_name);
}


void menu_cb_notif_checkbox(GtkWidget *widget, gpointer data) {
        livescore_applet *applet = data;
	char value[10240];
	int i;

	int league_id = atoi(gtk_widget_get_name(widget));

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
		applet->all_leagues[league_id].favourite = TRUE;
	else
		applet->all_leagues[league_id].favourite = FALSE; 

        // Save favourite leagues in GSettings
        sprintf(&value[0], "\"0");
        for (i=0; i < applet->all_leagues_counter; i++) {
                if (applet->all_leagues[i].favourite) {
                        strcat(&value[0], ",");
                        strcat(&value[0], &applet->all_leagues[i].league_name[0]);
                }
        }
        strcat(&value[0], "\"");

	g_settings_set_string(applet->gsettings, APPLET_GSETTINGS_KEY_FAV, &value[0]);
}


void menu_cb_settings (GtkAction *action, livescore_applet *applet) {
	int i;
	char n[8];

	// Prepare Notifications tab 
	GtkWidget *notif_label = gtk_label_new(_("Choose which leagues you want to be notified about:"));

	GtkWidget *notif_table = gtk_scrolled_window_new(NULL,NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(notif_table), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	GtkWidget *viewport = gtk_viewport_new (NULL, NULL);

	GtkWidget *notif_vbox_2 = gtk_vbox_new (FALSE, 0);
	for (i=0; i < applet->all_leagues_counter; i++) {
		// Skip league with name '0' - a servie one
		if (!strcmp(&applet->all_leagues[i].league_name[0], "0"))
			continue;
		GtkWidget *_tmp_check_button = gtk_check_button_new_with_label(&applet->all_leagues[i].league_name[0]);
		sprintf(&n[0], "%u", i);
		gtk_widget_set_name(_tmp_check_button, &n[0]);

		if (applet->all_leagues[i].favourite)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_tmp_check_button), TRUE);

		g_signal_connect (G_OBJECT(_tmp_check_button), "toggled", G_CALLBACK (menu_cb_notif_checkbox), (gpointer) applet);

		gtk_box_pack_start(GTK_BOX(notif_vbox_2), _tmp_check_button, FALSE, FALSE, 0);
	}
	gtk_container_add (GTK_CONTAINER (viewport), notif_vbox_2);
	gtk_container_add (GTK_CONTAINER (notif_table), viewport);

	GtkWidget *notif_vbox_1 = gtk_vbox_new (FALSE, 0);

        gtk_box_pack_start(GTK_BOX(notif_vbox_1), notif_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(notif_vbox_1), notif_table, TRUE, TRUE, 0);

	// Prepare Feed tab
	GtkWidget *feed_label = gtk_label_new(_("Choose which feed provider to use:"));

        GtkWidget *feed_combo = gtk_combo_box_text_new();
        for (i=0; i < applet->all_feeds_counter; i++) {
                gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(feed_combo), &applet->all_feeds[i].feed_name[0]);
                if (applet->all_feeds[i].selected)
                        gtk_combo_box_set_active(GTK_COMBO_BOX(feed_combo), i);
        }
	g_signal_connect (G_OBJECT(feed_combo), "changed", G_CALLBACK (menu_cb_feed_combo), (gpointer) applet);

	GtkWidget *feed_vbox_1 = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start(GTK_BOX(feed_vbox_1), feed_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(feed_vbox_1), feed_combo, FALSE, FALSE, 0);


	// Create notebook widget
	GtkWidget *notebook = gtk_notebook_new();
	gtk_widget_set_size_request (notebook, APPLET_WINDOW_SETTINGS_WIDTH, APPLET_WINDOW_SETTINGS_HEIGHT);

	// First page - Favourites
	GtkWidget *tab_label_1 = gtk_label_new(_("Notifications"));
	gtk_notebook_append_page (GTK_NOTEBOOK(notebook), notif_vbox_1, tab_label_1);

	// Second page - Icecast
	GtkWidget *tab_label_2 = gtk_label_new(_("Feeds"));
	gtk_notebook_append_page (GTK_NOTEBOOK(notebook), feed_vbox_1, tab_label_2);

	// Assemble window
        applet->dialog_settings = gtk_dialog_new_with_buttons (_("MATE Livescore Applet"), GTK_WINDOW(applet), GTK_DIALOG_MODAL, NULL);
	GtkWidget *buttonClose = gtk_dialog_add_button (GTK_DIALOG(applet->dialog_settings), GTK_STOCK_CLOSE, GTK_RESPONSE_CANCEL);

        gtk_dialog_set_default_response (GTK_DIALOG (applet->dialog_settings), GTK_RESPONSE_CANCEL);
        gtk_container_add (GTK_CONTAINER (GTK_DIALOG(applet->dialog_settings)->vbox), notebook);
	g_signal_connect (G_OBJECT(buttonClose), "clicked", G_CALLBACK (quitDialogClose), (gpointer) applet);

        gtk_widget_show_all(GTK_WIDGET(applet->dialog_settings));
}



