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
#include <libxml/HTMLparser.h>
#include <libxml/HTMLtree.h>
#include <libsoup/soup.h>
#include <dlfcn.h>

#ifdef HAVE_MATE
	#include <mate-panel-applet.h>
#elif HAVE_GNOME_2
	#include <panel-applet.h>
	#include <panel-applet-gconf.h>
#endif

#ifdef HAVE_LIBMATENOTIFY
	#include <libmatenotify/notify.h>
#elif HAVE_LIBNOTIFY
	#include <libnotify/notify.h>
#endif

#ifdef HAVE_MATE
	#define APPLET_FACTORY "LivescoreAppletFactory"
	#define APPLET_ID "LivescoreApplet"
#elif HAVE_GNOME_2
	#define APPLET_FACTORY "OAFIID:LivescoreApplet_Factory"
	#define APPLET_ID "OAFIID:LivescoreApplet"
#endif


#define _(String) gettext (String)
#define APPLET_NAME "livescore"
#define APPLET_ICON_STATIC "applet_livescore_icon.png"
#define APPLET_IMAGE_RED "applet_livescore_red.png"
#define APPLET_IMAGE_YELLOW "applet_livescore_yellow.png"
#define APPLET_IMAGE_GREEN "applet_livescore_green.png"
#define APPLET_IMAGE_GRAY "applet_livescore_gray.png"
#define APPLET_IMAGE_NOTIF_GOAL "applet_livescore_goal.png"
#define APPLET_IMAGE_NOTIF_WHISTLE "applet_livescore_whistle.png"
#define APPLET_WINDOW_MATCHES_WIDTH 640
#define APPLET_WINDOW_MATCHES_HEIGHT 480
#define APPLET_WINDOW_SETTINGS_WIDTH 480
#define APPLET_WINDOW_SETTINGS_HEIGHT 320
#define APPLET_KEEP_TIME 57600	// 16 hours
#define APPLET_FEED_DEFAULT "lib_feed_iddaa.so"
// GSettings
#define APPLET_GSETTINGS_SCHEMA "org.mate.panel.applet.LivescoreApplet"
#define APPLET_GSETTINGS_PATH "/org/mate/panel/objects/livescore/"
#define APPLET_GSETTINGS_SEPARATOR "|"
#define APPLET_GSETTINGS_KEY_FAV "leagues-notify"
#define APPLET_GSETTINGS_KEY_EXP "leagues-expanded"
#define APPLET_GSETTINGS_KEY_FEED "feed"

// Menu strings
#ifdef HAVE_MATE
static const gchar *ui = 
"<menuitem name='MenuItem1' action='Settings' />"
"<menuitem name='MenuItem2' action='About' />"
;
#elif HAVE_GNOME_2
static const gchar *ui =
"<popup name='button3'>"
"<menuitem name='MenuItem1' verb='Settings' label='Settings'/>"
"<menuitem name='MenuItem2' verb='About' label='About' pixtype='stock' pixname='gnome-stock-about'/>"
"</popup>"
;
#endif

#ifdef HAVE_MATE
        typedef MatePanelApplet MyPanelApplet;
	typedef MatePanelAppletBackgroundType MyPanelAppletBackgroundType;
#elif HAVE_GNOME_2
        typedef PanelApplet MyPanelApplet;
	typedef PanelAppletBackgroundType MyPanelAppletBackgroundType;
#endif

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

enum {
	NOTIF_SHOW_IMAGE_WHISTLE = 0,
	NOTIF_SHOW_IMAGE_GOAL
};

typedef struct f_data {
	void *node_data;
	struct f_data *node_next;
} fifo_data;

typedef struct {
	fifo_data *fifo_head;
	fifo_data *fifo_tail;
} fifo;

typedef struct {
	char title[256];
	char body[256];
	int image;
} notif_data;

typedef struct {
	int feed_id;
	char feed_name[256];
	gboolean enabled;
	gboolean selected;
} feed_data;

typedef struct {
	int league_id;
	char league_name[256];
	gboolean used;
	gboolean favourite;
	gboolean expanded;
} league_data;

typedef struct {
	int league_id;
	char league_name[256];
	char team_home[64];
	char team_away[64];
	int score_home;
	int score_away;
	int status;
	time_t start_time;
	int match_time;
	int match_time_added;
	gboolean used;
} match_data;

typedef struct {
	GMainLoop *loop;
	MyPanelApplet *applet;
	GtkActionGroup *action_group;
	GtkWidget *image;
	GtkWidget *event_box;
	match_data *all_matches;
	league_data *all_leagues;
	feed_data *all_feeds;
	int all_matches_counter;
	int all_leagues_counter;
	int all_feeds_counter;
	void *feed_handle;
	void (*feed_main)(match_data **, int *);
	gboolean dialog_matches_is_visible;
	fifo *notif_queue;
#ifdef HAVE_MATE
	GSettings *gsettings;
#endif
	GtkTreeStore *tree_store;
	GtkWidget *tree_view;
	GtkWidget *dialog_matches;
	GtkWidget *dialog_settings;
	GdkPixbuf *running_image_red;
	GdkPixbuf *running_image_green;
	GdkPixbuf *running_image_yellow;
	GdkPixbuf *running_image_gray;
	GdkPixbuf *notif_image_goal;
	GdkPixbuf *notif_image_whistle;
} livescore_applet;

// util.c
void show_notification (gchar *, gchar *, GdkPixbuf *);
gboolean cp(const char *, const char *);
char *trim(char *);
char *trim_quotes(char *);
char *string_ends(char *, int);
void debug(char *);

//menu.c
void quitDialogClose(GtkWidget *, gpointer);
void menu_cb_settings(GtkAction *, livescore_applet *);
void menu_cb_about(GtkAction *, livescore_applet *);

// gui.c
void gui_update_model(livescore_applet * applet);
gboolean on_left_click (GtkWidget *, GdkEventButton *, livescore_applet *);

// manager.c
//gboolean manager_main(livescore_applet *, match_data *);
int manager_timer(livescore_applet *);
gboolean manager_populate_feed(livescore_applet *, gchar *, gboolean);

// http.c
int get_url (char *, char *, char *);

// fifo.c
fifo *fifo_new(void);
void fifo_add(fifo *, void *);
void *fifo_remove(fifo *);
void fifo_free(fifo *);
gboolean fifo_is_empty(fifo *);

// feed_iddaa.c
int feed_iddaa_main(livescore_applet *);

// main.c
void applet_back_change (MyPanelApplet *, MyPanelAppletBackgroundType, GdkColor *, GdkPixmap *, livescore_applet *);
void applet_destroy(MyPanelApplet *, livescore_applet *);

// Menu skeleton
#ifdef HAVE_MATE
static const GtkActionEntry applet_menu_actions_mate[] = {
	{ "Settings", GTK_STOCK_EXECUTE, "_Settings", NULL, NULL, G_CALLBACK (menu_cb_settings) },
	{ "About", GTK_STOCK_ABOUT, "_About", NULL, NULL, G_CALLBACK (menu_cb_about) }
};
#elif HAVE_GNOME_2
static const BonoboUIVerb applet_menu_actions_gnome [] = {
        BONOBO_UI_UNSAFE_VERB ("Settings", menu_cb_settings),
        BONOBO_UI_UNSAFE_VERB ("About", menu_cb_about),
        BONOBO_UI_VERB_END
};
#endif
