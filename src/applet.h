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

#include <string.h>
#include <unistd.h>
#include <mate-panel-applet.h>
#include <libintl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <glib.h>
#include <stdlib.h>
#include <wait.h>
#include <unistd.h>
#include <pwd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
//#include <libxml/parser.h>
//#include <libxml/tree.h>
#include <libxml/HTMLparser.h>
#include <libxml/HTMLtree.h>
#include <libsoup/soup.h>

#ifdef HAVE_LIBMATENOTIFY
	#include <libmatenotify/notify.h>
#elif HAVE_LIBNOTIFY
	#include <libnotify/notify.h>
#endif

#define _(String) gettext (String)
#define APPLET_FACTORY "LivescoreAppletFactory"
#define APPLET_ID "LivescoreApplet"
#define APPLET_NAME "livescore"
#define APPLET_ICON_STATIC "applet_livescore_icon.png"
#define APPLET_IMAGE_RED "applet_livescore_red.png"
#define APPLET_IMAGE_YELLOW "applet_livescore_yellow.png"
#define APPLET_IMAGE_GREEN "applet_livescore_green.png"
#define APPLET_IMAGE_GRAY "applet_livescore_gray.png"
#define APPLET_WINDOW_MATCHES_WIDTH 640
#define APPLET_WINDOW_MATCHES_HEIGHT 480
// GSettings
#define APPLET_GSETTINGS_SCHEMA "org.mate.panel.applet.LivescoreApplet"
#define APPLET_GSETTINGS_PATH "/org/mate/panel/objects/livescore/"
#define APPLET_GSETTINGS_KEY_FAV "leagues-notify"

// Menu strings
static const gchar *ui1 = 
"<menuitem name='MenuItem1' action='Settings' />"
"<menuitem name='MenuItem2' action='About' />"
;

// Two hidden colums will help manage the font weight for some cells
enum {
        COL_PIC = 0,
        COL_TIME,
	COL_SCORE,
	COL_MATCH,
	COL_HIDDEN_BOLD,
	COL_HIDDEN_BOOLEAN,
        NUM_COLS
};

enum {
	MATCH_NOT_COMMENCED = 0,
	MATCH_FIRST_TIME,
	MATCH_HALF_TIME,
	MATCH_SECOND_TIME,
	MATCH_EXTRA_TIME,
	MATCH_FULL_TIME
};

typedef struct {
	int league_id;
	char league_name[128];
	gboolean used;
	gboolean favourite;
} league_data;

typedef struct {
	int league_id;
	char league_name[128];
	char team_home[64];
	char team_away[64];
	int score_home;
	int score_away;
	int status;
	time_t start_time;
	int match_time;
	gboolean used;
} match_data;

typedef struct {
	GMainLoop *loop;
	MatePanelApplet *applet;
	GtkActionGroup *action_group;
        GtkWidget *image;
        GtkWidget *event_box;
	GtkWidget *text;
	match_data *all_matches;
	league_data *all_leagues;
	int all_matches_counter;
	int all_leagues_counter;
	char url[1024];
	char name[1024];
	char xmlfile[1024];
	int status;
	time_t timestamp;
	GSettings *gsettings;
	GtkTreeStore *tree_store;
	GtkWidget *tree_view;
	GtkWidget *dialog_matches;
	GtkWidget *dialog_settings;
} livescore_applet;

// util.c
void push_notification (gchar *, gchar *, gchar *);
gboolean cp(const char *, const char *);
char *trim(char *);
char *trim_quotes(char *);
void debug(char *);

//menu.c
void quitDialogClose(GtkWidget *, gpointer);
void menu_cb_settings(GtkAction *, livescore_applet *);
void menu_cb_about(GtkAction *, livescore_applet *);
void create_view_and_model (livescore_applet *);
void cell_edit_name(GtkCellRendererText *, gchar *, gchar *, gpointer);
void cell_edit_url(GtkCellRendererText *, gchar *, gchar *, gpointer);
void row_down(GtkWidget *, gpointer);
void row_up(GtkWidget *, gpointer);
void save_favourites(livescore_applet *);
gboolean write_favourites(GtkTreeModel *, GtkTreePath *, GtkTreeIter *, gpointer);
void do_play(livescore_applet *);
gboolean on_left_click (GtkWidget *, GdkEventButton *, livescore_applet *);

// gui.c
void clear_store(livescore_applet *);

// manager.c
gboolean manager_main(livescore_applet *, match_data *);
void manager_cleanup(livescore_applet *applet);

// http.c
int get_url (char *, char *, char *);

// feed_iddaa.c
int feed_iddaa_main(livescore_applet *);

// main.c
void applet_back_change (MatePanelApplet *, MatePanelAppletBackgroundType, GdkColor *, GdkPixmap *, livescore_applet *);
void applet_destroy(MatePanelApplet *, livescore_applet *);

// Menu skeleton
static const GtkActionEntry applet_menu_actions[] = {
        { "All", GTK_STOCK_EXECUTE, "_Settings", NULL, NULL, G_CALLBACK (menu_cb_settings) },
        { "About", GTK_STOCK_ABOUT, "_About", NULL, NULL, G_CALLBACK (menu_cb_about) }
};
