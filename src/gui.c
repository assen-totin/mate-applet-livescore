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
#include <assert.h>

void gui_quit(GtkWidget *widget, gpointer data) {
	livescore_applet *applet = data;

	if (!applet->dialog_matches_is_visible)
		return;

	applet->dialog_matches_is_visible = FALSE;

	// Get the VBox out of the dialog
	gpointer *gp_vbox = g_object_ref(gtk_dialog_get_content_area (GTK_DIALOG (applet->dialog_matches)));
	gtk_container_remove (GTK_CONTAINER(applet->dialog_matches), gtk_dialog_get_content_area (GTK_DIALOG (applet->dialog_matches)));

	// Get the scrolled window out of the VBox
	GList *glist = gtk_container_get_children(GTK_CONTAINER(gp_vbox));
	gpointer *gp_scrolled_window = g_object_ref(glist->data);
	gtk_container_remove (GTK_CONTAINER(gp_vbox), GTK_WIDGET(gp_scrolled_window));

	// Get applet->tree_view out of the scrolled window
	gtk_container_remove (GTK_CONTAINER(gp_scrolled_window), applet->tree_view);

	g_object_unref(gp_vbox);
	g_object_unref(gp_scrolled_window);

	gtk_widget_destroy(applet->dialog_matches);
}


gboolean gui_row_expand_collapse(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data) {
	livescore_applet *applet = data;
	gchar *line;
	int i;

	gtk_tree_model_get(model, iter, COL_MATCH, &line, -1);

	for (i=0; i < applet->all_leagues_counter; i++) {
		if (!strcmp(&applet->all_leagues[i].league_name[0], line)) {
			if (gtk_tree_view_row_expanded (GTK_TREE_VIEW(applet->tree_view), gtk_tree_model_get_path(model, iter))) {
				applet->all_leagues[i].expanded = TRUE;
				break;
			}
			else {
				applet->all_leagues[i].expanded = FALSE;
				break;
			}
		}
	}

	return FALSE;
}


void gui_rows_expand_collapse(GtkTreeView *tree_view, GtkTreeIter *iter, GtkTreePath *path, gpointer data) {
	livescore_applet *applet = data;
	int i;
	char value[10240];
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(applet->tree_view));

	if (!applet->dialog_matches_is_visible)
		return;

	gtk_tree_model_foreach(model, gui_row_expand_collapse, applet);

	// Save expanded leagues in GSettings
	sprintf(&value[0], "\"0");
	for (i=0; i < applet->all_leagues_counter; i++) {
		if (applet->all_leagues[i].expanded && applet->all_leagues[i].used) {
			strcat(&value[0], APPLET_GSETTINGS_SEPARATOR);
			strcat(&value[0], &applet->all_leagues[i].league_name[0]);
		}
	}
	strcat(&value[0], "\"");

#ifdef HAVE_MATE
	g_settings_set_string(applet->gsettings, APPLET_GSETTINGS_KEY_EXP, &value[0]);
#elif HAVE_GNOME_2
        panel_applet_gconf_set_string(PANEL_APPLET(applet->applet), APPLET_GSETTINGS_KEY_EXP, &value[0], NULL);
#endif
}



gboolean gui_expand_row(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data) {
	livescore_applet *applet = data;
	gchar *line;
	int i;

	gtk_tree_model_get(model, iter, COL_MATCH, &line, -1);
	for (i=0; i < applet->all_leagues_counter; i++) {
		if (!strcmp(&applet->all_leagues[i].league_name[0], line) && applet->all_leagues[i].expanded) {
			gtk_tree_view_expand_row (GTK_TREE_VIEW(applet->tree_view), gtk_tree_model_get_path(model, iter), FALSE);
			break;
		}
	}
	return FALSE;
}


void gui_create_view_and_model(livescore_applet *applet) {
	GtkCellRenderer *renderer, *renderer_image;
	GtkTreeViewColumn *column;
	GtkTreeModel *model;
	GtkTreeIter iter;

	applet->tree_view = gtk_tree_view_new();
	applet->tree_store = gtk_tree_store_new(NUM_COLS, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_BOOLEAN);

	// Column 1 - image
	renderer_image = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (applet->tree_view), -1, " ", renderer_image, "pixbuf", COL_PIC, NULL);

	// Column 2
	renderer = gtk_cell_renderer_text_new();
	g_object_set (renderer, "xalign", 1.0, NULL);
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (applet->tree_view), -1, _("Time"), renderer, "text", COL_TIME, NULL);

	// Column 3
	renderer = gtk_cell_renderer_text_new();
	g_object_set (renderer, "xalign", 0.5, NULL);
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (applet->tree_view), -1, _("Score"), renderer, "text", COL_SCORE, NULL);

	// Column 4
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (applet->tree_view), -1, _("League / Match"), renderer, "text", COL_MATCH, "weight", COL_HIDDEN_BOLD, "weight-set", COL_HIDDEN_BOOLEAN, NULL);

	// Hidden column for bold font on some rows - for the font weight...
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (applet->tree_view), -1, _("Hidden"), renderer, "text", COL_HIDDEN_BOLD, NULL);
	column = gtk_tree_view_get_column (GTK_TREE_VIEW (applet->tree_view), COL_HIDDEN_BOLD);
	gtk_tree_view_column_set_visible(column, FALSE);

	// ... and to enable or disable them. 
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (applet->tree_view), -1, _("Hidden"), renderer, "text", COL_HIDDEN_BOOLEAN, NULL);
	column = gtk_tree_view_get_column (GTK_TREE_VIEW (applet->tree_view), COL_HIDDEN_BOOLEAN);
	gtk_tree_view_column_set_visible(column, FALSE);

	gtk_tree_store_append (applet->tree_store, &iter, NULL);
	gtk_tree_store_set (applet->tree_store, &iter, COL_MATCH, _("No data yet. Wait until we gather some."), COL_HIDDEN_BOLD, PANGO_WEIGHT_NORMAL, COL_HIDDEN_BOOLEAN, TRUE, -1);

	model = GTK_TREE_MODEL(applet->tree_store);
	gtk_tree_view_set_model (GTK_TREE_VIEW (applet->tree_view), model);
	// The tree view has acquired its own reference to the model, so we can drop ours. 
	// That way the model will be freed automatically when the tree view is destroyed 
	g_object_unref (model);

	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(applet->tree_view));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
}


void gui_matches_dialog (livescore_applet *applet) {
	GtkWidget *scrolled_window, *button_close;
	GtkTreeModel *model;

	scrolled_window = gtk_scrolled_window_new(NULL,NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	gtk_container_add (GTK_CONTAINER (scrolled_window), applet->tree_view);

	// Assemble window
	applet->dialog_matches = gtk_dialog_new_with_buttons (_("MATE Livescore Applet"), GTK_WINDOW(applet), GTK_DIALOG_MODAL, NULL);
	gtk_window_resize(GTK_WINDOW(applet->dialog_matches), APPLET_WINDOW_MATCHES_WIDTH, APPLET_WINDOW_MATCHES_HEIGHT);
	button_close = gtk_dialog_add_button (GTK_DIALOG(applet->dialog_matches), GTK_STOCK_CLOSE, GTK_RESPONSE_CANCEL);
	gtk_dialog_set_default_response (GTK_DIALOG (applet->dialog_matches), GTK_RESPONSE_CANCEL);
	gtk_container_add (GTK_CONTAINER(gtk_dialog_get_content_area (GTK_DIALOG (applet->dialog_matches))), scrolled_window);

	// Signals
	g_signal_connect (G_OBJECT(button_close), "clicked", G_CALLBACK (gui_quit), (gpointer) applet);
	g_signal_connect(G_OBJECT(applet->tree_view), "row-expanded", G_CALLBACK (gui_rows_expand_collapse), (gpointer) applet);
	g_signal_connect(G_OBJECT(applet->tree_view), "row-collapsed", G_CALLBACK (gui_rows_expand_collapse), (gpointer) applet);
	g_signal_connect(G_OBJECT(applet->dialog_matches), "destroy", G_CALLBACK(gui_quit), (gpointer)applet);

	// Expand rows
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(applet->tree_view));
	gtk_tree_model_foreach(model, gui_expand_row, applet);

	gtk_widget_show_all(GTK_WIDGET(applet->dialog_matches));
	applet->dialog_matches_is_visible = TRUE;
}


void gui_update_model(livescore_applet * applet) {
	GtkTreeIter parent, child, grandson;
	GtkTreeModel *model;
	GdkPixbuf *running_image = NULL;
	int i, j, k;
	char all_match[256], score[16], image_file[1024], time_elapsed[32], goal_info[16];
	struct tm *ltp, lt;
	gboolean league_has_matches = FALSE;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(applet->tree_view));

	if (applet->all_matches_counter < 2) 
		return;

	gtk_tree_store_clear(applet->tree_store);

	for (i=0; i < applet->all_leagues_counter; i++) {
		// Skip leagues which have no matches (e.g. loaded from GSettings)
		league_has_matches = FALSE;
		for (j=0; j < applet->all_matches_counter; j++) {
			if (applet->all_matches[j].used && (i == applet->all_matches[j].league_id)) {
				league_has_matches = TRUE;
				break;
			}
		}
		if (!league_has_matches)
			continue;

		// Show league
		gtk_tree_store_append (applet->tree_store, &parent, NULL);
		gtk_tree_store_set (applet->tree_store, &parent, COL_MATCH, &applet->all_leagues[i].league_name[0], COL_HIDDEN_BOLD, PANGO_WEIGHT_BOLD, COL_HIDDEN_BOOLEAN, TRUE, -1);

		for (j=0; j < applet->all_matches_counter; j++) {
			if (applet->all_matches[j].used && (i == applet->all_matches[j].league_id)) {
				// Show match
				sprintf(&score[0], "%u : %u", applet->all_matches[j].score_home, applet->all_matches[j].score_away);

				if (applet->all_matches[j].status == MATCH_FIRST_TIME) {
					if (applet->all_matches[j].match_time_added == 0)
						sprintf(&time_elapsed[0], "%u'", applet->all_matches[j].match_time);
					else
						sprintf(&time_elapsed[0], "%u+%u'", applet->all_matches[j].match_time, applet->all_matches[j].match_time_added);
					running_image = applet->running_image_green;
				}
				else if (applet->all_matches[j].status == MATCH_HALF_TIME) {
					sprintf(&time_elapsed[0], _("HT"));
					running_image = applet->running_image_gray;
				}
				else if (applet->all_matches[j].status == MATCH_SECOND_TIME) {
					if (applet->all_matches[j].match_time_added == 0)
						sprintf(&time_elapsed[0], "%u'", applet->all_matches[j].match_time);
					else
						sprintf(&time_elapsed[0], "%u+%u'", applet->all_matches[j].match_time, applet->all_matches[j].match_time_added);
					if (applet->all_matches[j].match_time < 80)
						running_image = applet->running_image_yellow;
					else
						running_image = applet->running_image_red;
				}
				else if (applet->all_matches[j].status == MATCH_EXTRA_TIME) {
					if (applet->all_matches[j].match_time_added == 0)
						sprintf(&time_elapsed[0], "%u'", applet->all_matches[j].match_time);
					else
						sprintf(&time_elapsed[0], "%u+%u'", applet->all_matches[j].match_time, applet->all_matches[j].match_time_added);
					running_image = applet->running_image_red;
				}
				else if (applet->all_matches[j].status == MATCH_FULL_TIME) { 
					sprintf(&time_elapsed[0], _("FT"));
					running_image = NULL;
				}
				else {
					sprintf(&score[0], " ");
					ltp = localtime(&applet->all_matches[j].start_time);
					sprintf(&time_elapsed[0], "%u:%u", ltp->tm_hour, ltp->tm_min);
					if (ltp->tm_min < 10)
						strcat(&time_elapsed[0], "0");
					running_image = NULL;
				}

				sprintf(&all_match[0], "%s - %s", &applet->all_matches[j].team_home[0], &applet->all_matches[j].team_away[0]);

				gtk_tree_store_append (applet->tree_store, &child, &parent);
				gtk_tree_store_set (applet->tree_store, &child, COL_PIC, running_image, COL_TIME, &time_elapsed[0], COL_SCORE, &score[0], COL_MATCH, &all_match[0], COL_HIDDEN_BOLD, PANGO_WEIGHT_NORMAL, COL_HIDDEN_BOOLEAN, TRUE, -1);

				// Goals for this match
				for (k=0; k < applet->all_goals_counter; k++) {
					if (applet->all_goals[k].used && (j == applet->all_goals[k].match_id)) {
						sprintf(&goal_info[0], "%u' %u:%u", applet->all_goals[k].match_time, applet->all_goals[k].score_home, applet->all_goals[k].score_away); 
						gtk_tree_store_append (applet->tree_store, &grandson, &child);
						gtk_tree_store_set (applet->tree_store, &grandson, COL_MATCH, &goal_info[0], COL_HIDDEN_BOLD, PANGO_WEIGHT_NORMAL, COL_HIDDEN_BOOLEAN, TRUE, -1);
					}
				}
			}
		}
	}		

	// Expand rows
	// Termporarily block visibility in order not to trigget the 'row-expanded' callback function
	gboolean _tmp = applet->dialog_matches_is_visible;
	applet->dialog_matches_is_visible = FALSE;
	gtk_tree_model_foreach(model, gui_expand_row, applet);
	//gtk_tree_view_columns_autosize(GTK_TREE_VIEW(applet->tree_view));
	if (_tmp)
		applet->dialog_matches_is_visible = TRUE;
}


gboolean on_left_click (GtkWidget *event_box, GdkEventButton *event, livescore_applet *applet) {
	static GtkWidget *label;
	char msg[1024], image_file[1024];

	// We only process left clicks here
	if (event->button != 1)
		return FALSE;

	// Open the matches window
	if (!applet->dialog_matches_is_visible)
		gui_matches_dialog(applet);

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

