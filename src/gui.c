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

void gui_quit(GtkWidget *widget, gpointer data) {
	livescore_applet *applet = data;

        gtk_widget_destroy(applet->dialog_matches);
}


void gui_matches (livescore_applet *applet) {
        GtkCellRenderer *renderer, *renderer_image;
        GtkTreeModel *model;
        GtkTreeIter child, parent;
        GtkTreeViewColumn *column;
	char score[16], image_file[1024], time_elapsed[32];
	int i, j;
	struct tm *ltp, lt;

        applet->tree_view = gtk_tree_view_new();
        applet->tree_store = gtk_tree_store_new(NUM_COLS, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT, G_TYPE_BOOLEAN);

	// VIEW and MODEL
        // Column 1 - image
        renderer_image = gtk_cell_renderer_pixbuf_new();
        gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (applet->tree_view), -1, " ", renderer_image, "pixbuf", COL_PIC, NULL);

        // Column 2
        renderer = gtk_cell_renderer_text_new();
	g_object_set (renderer, "xalign", 1.0, NULL);
        gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (applet->tree_view), -1, _("Time"), renderer, "text", COL_TIME, NULL);

        // Column 3
        renderer = gtk_cell_renderer_text_new();
	//g_object_set (renderer, "xalign", 1.0, NULL);
        gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (applet->tree_view), -1, _("Home"), renderer, "text", COL_HOME, "weight", COL_HIDDEN_BOLD, "weight-set", COL_HIDDEN_BOOLEAN, "stretch", COL_HIDDEN_STRETCH, "stretch-set", COL_HIDDEN_BOOLEAN,  NULL);
	column = gtk_tree_view_get_column (GTK_TREE_VIEW (applet->tree_view), COL_HOME);
	gtk_tree_view_column_set_max_width(column, (int) (0.35 * APPLET_WINDOW_MATCHES_WIDTH));

        // Column 4
        renderer = gtk_cell_renderer_text_new();
	g_object_set (renderer, "xalign", 0.5, NULL);
        gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (applet->tree_view), -1, _("Score"), renderer, "text", COL_SCORE, NULL);

        // Column 5
        renderer = gtk_cell_renderer_text_new();
        gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (applet->tree_view), -1, _("Away"), renderer, "text", COL_AWAY, NULL);
        column = gtk_tree_view_get_column (GTK_TREE_VIEW (applet->tree_view), COL_AWAY);
        gtk_tree_view_column_set_max_width(column, (int) (0.35 * APPLET_WINDOW_MATCHES_WIDTH));

	// Hidden column for bold font on some rows - for the font weight...
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (applet->tree_view), -1, _("Hidden"), renderer, "text", COL_HIDDEN_BOLD, NULL);
	column = gtk_tree_view_get_column (GTK_TREE_VIEW (applet->tree_view), COL_HIDDEN_BOLD);
	gtk_tree_view_column_set_visible(column, FALSE);

	// Hidden column for condensed font on some rows - for the font stretch
        renderer = gtk_cell_renderer_text_new();
        gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (applet->tree_view), -1, _("Hidden"), renderer, "text", COL_HIDDEN_STRETCH, NULL);
        column = gtk_tree_view_get_column (GTK_TREE_VIEW (applet->tree_view), COL_HIDDEN_STRETCH);
        gtk_tree_view_column_set_visible(column, FALSE);

	// Hidden column - control 
        renderer = gtk_cell_renderer_text_new();
        gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (applet->tree_view), -1, _("Hidden"), renderer, "text", COL_HIDDEN_BOOLEAN, NULL);
        column = gtk_tree_view_get_column (GTK_TREE_VIEW (applet->tree_view), COL_HIDDEN_BOOLEAN);
        gtk_tree_view_column_set_visible(column, FALSE);

        gtk_tree_store_append (applet->tree_store, &child, NULL);
        gtk_tree_store_set (applet->tree_store, &child, COL_HOME, _("No data yet."), COL_AWAY, _("Open this window again in a minute."), COL_HIDDEN_BOLD, PANGO_WEIGHT_NORMAL, COL_HIDDEN_STRETCH, PANGO_STRETCH_NORMAL, COL_HIDDEN_BOOLEAN, TRUE, -1);

        model = GTK_TREE_MODEL(applet->tree_store);
        gtk_tree_view_set_model (GTK_TREE_VIEW (applet->tree_view), model);

        // The tree view has acquired its own reference to the model, so we can drop ours. 
        // That way the model will be freed automatically when the tree view is destroyed 
        g_object_unref (model);

        GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(applet->tree_view));
        gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);

	GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL,NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	gtk_container_add (GTK_CONTAINER (scrolled_window), applet->tree_view);

	// Assemble window
        applet->dialog_matches = gtk_dialog_new_with_buttons (_("MATE Livescore Applet"), GTK_WINDOW(applet), GTK_DIALOG_MODAL, NULL);
	gtk_window_resize(GTK_WINDOW(applet->dialog_matches), APPLET_WINDOW_MATCHES_WIDTH, APPLET_WINDOW_MATCHES_HEIGHT);
	GtkWidget *button_close = gtk_dialog_add_button (GTK_DIALOG(applet->dialog_matches), GTK_STOCK_CLOSE, GTK_RESPONSE_CANCEL);
        gtk_dialog_set_default_response (GTK_DIALOG (applet->dialog_matches), GTK_RESPONSE_CANCEL);
	gtk_container_add (GTK_CONTAINER(gtk_dialog_get_content_area (GTK_DIALOG (applet->dialog_matches))), scrolled_window);
	g_signal_connect (G_OBJECT(button_close), "clicked", G_CALLBACK (gui_quit), (gpointer) applet);

	if (applet->all_matches_counter > 1) {
		gtk_tree_store_clear(applet->tree_store);

		sprintf(&image_file[0], "%s/%s", APPLET_ICON_PATH, APPLET_IMAGE_RED);
		GdkPixbuf *running_image_red = gdk_pixbuf_new_from_file(&image_file[0], NULL);

		sprintf(&image_file[0], "%s/%s", APPLET_ICON_PATH, APPLET_IMAGE_GREEN);
                GdkPixbuf *running_image_green = gdk_pixbuf_new_from_file(&image_file[0], NULL);

		sprintf(&image_file[0], "%s/%s", APPLET_ICON_PATH, APPLET_IMAGE_YELLOW);
                GdkPixbuf *running_image_yellow = gdk_pixbuf_new_from_file(&image_file[0], NULL);

		sprintf(&image_file[0], "%s/%s", APPLET_ICON_PATH, APPLET_IMAGE_GRAY);
                GdkPixbuf *running_image_gray = gdk_pixbuf_new_from_file(&image_file[0], NULL);

		for (i=0; i < applet->all_leagues_counter; i++) {
			// Show league
			gtk_tree_store_append (applet->tree_store, &parent, NULL);
			gtk_tree_store_set (applet->tree_store, &parent, COL_HOME, &applet->all_leagues[i].league_name[0], COL_HIDDEN_BOLD, PANGO_WEIGHT_BOLD, COL_HIDDEN_BOOLEAN, TRUE, COL_HIDDEN_STRETCH, PANGO_STRETCH_CONDENSED, -1);

			for (j=0; j < applet->all_matches_counter; j++) {
				if (i == applet->all_matches[j].league_id) {
					// Show match
					sprintf(&score[0], "%u : %u", applet->all_matches[j].score_home, applet->all_matches[j].score_away);

					gtk_tree_store_append (applet->tree_store, &child, &parent);
					gtk_tree_store_set (applet->tree_store, &child, COL_HOME, &applet->all_matches[j].team_home[0], COL_AWAY, &applet->all_matches[j].team_away[0], COL_SCORE, &score[0], COL_HIDDEN_BOLD, PANGO_WEIGHT_NORMAL, COL_HIDDEN_STRETCH, PANGO_STRETCH_NORMAL, COL_HIDDEN_BOOLEAN, TRUE, -1);

					if (applet->all_matches[j].status == MATCH_FIRST_TIME) {
						sprintf(&time_elapsed[0], "%u'", applet->all_matches[j].match_time);
						gtk_tree_store_set (applet->tree_store, &child, COL_TIME, &time_elapsed[0], -1);
						gtk_tree_store_set (applet->tree_store, &child, COL_PIC, running_image_green, -1);
					}
					else if (applet->all_matches[j].status == MATCH_HALF_TIME) {
						gtk_tree_store_set (applet->tree_store, &child, COL_TIME, _("HT"), -1);
						gtk_tree_store_set (applet->tree_store, &child, COL_PIC, running_image_gray, -1);
					}
					else if (applet->all_matches[j].status == MATCH_SECOND_TIME) {
						sprintf(&time_elapsed[0], "%u'", applet->all_matches[j].match_time);
						gtk_tree_store_set (applet->tree_store, &child, COL_TIME, &time_elapsed[0], -1);
						if (applet->all_matches[j].match_time < 80)
							gtk_tree_store_set (applet->tree_store, &child, COL_PIC, running_image_yellow, -1);
						else
							gtk_tree_store_set (applet->tree_store, &child, COL_PIC, running_image_red, -1);
					}
					else if (applet->all_matches[j].status == MATCH_EXTRA_TIME) {
						sprintf(&time_elapsed[0], "%u'", applet->all_matches[j].match_time);
						gtk_tree_store_set (applet->tree_store, &child, COL_TIME, &time_elapsed[0], -1);
						gtk_tree_store_set (applet->tree_store, &child, COL_PIC, running_image_red, -1);
					}
					else if (applet->all_matches[j].status == MATCH_FULL_TIME) 
						gtk_tree_store_set (applet->tree_store, &child, COL_TIME, _("FT"), -1);
					else {
						ltp = localtime(&applet->all_matches[j].start_time);
						//sprintf(&time_elapsed[0], "%u.%u. %u:%u", ltp->tm_mday, ltp->tm_mon + 1, ltp->tm_hour, ltp->tm_min);
						sprintf(&time_elapsed[0], "%u:%u", ltp->tm_hour, ltp->tm_min);
						if (ltp->tm_min < 10)
							strcat(&time_elapsed[0], "0");
						gtk_tree_store_set (applet->tree_store, &child, COL_TIME, &time_elapsed[0], COL_SCORE, _("vs."), -1);
					}
				}
			}
		}		
	}

	gtk_widget_show_all(GTK_WIDGET(applet->dialog_matches));
	
/*
        GtkTreeModel *model = GTK_TREE_MODEL(applet->tree_store);
        selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(applet->tree_view));
        gtk_tree_model_get_iter_first(model, &iter);
        gtk_tree_selection_select_iter(selection, &iter);
*/

//	gtk_widget_grab_focus(fav_table);
}


gboolean on_left_click (GtkWidget *event_box, GdkEventButton *event, livescore_applet *applet) {
        static GtkWidget *label;
        char msg[1024], image_file[1024];

        // We only process left clicks here
        if (event->button != 1)
                return FALSE;

        // Open the matches window
	gui_matches(applet);

/*
        // TODO: Use this code for some animated GIF when goal is scored 
        if (applet->status == 0) {
                applet->status = 1;
                sprintf(&image_file[0], "%s/%s", APPLET_ICON_PATH, APPLET_ICON_PLAY);
                sprintf(&msg[0], "%s%s", _("PLAYING: "), &applet->name[0]);
                gtk_widget_set_tooltip_text (GTK_WIDGET (applet->applet), &msg[0]);
                gtk_image_set_from_file(GTK_IMAGE(applet->image), &image_file[0]);

                return TRUE;
        }
*/
        return FALSE;
}

