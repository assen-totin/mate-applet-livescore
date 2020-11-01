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

enum {
	OMNIBET_PARSING_SKIP = 0,
	OMNIBET_PARSING_LEAGUE,
	OMNIBET_PARSING_TIME,
	OMNIBET_PARSING_TEAM_HOME,
	OMNIBET_PARSING_SCORE_HOME,
	OMNIBET_PARSING_SCORE_AWAY,
	OMNIBET_PARSING_TEAM_AWAY
};

enum {
	OMNIBET_EXPECT_NONE = 0,
	OMNIBET_EXPECT_TEAM_HOME, 
	OMNIBET_EXPECT_SCORE_HOME, 
	OMNIBET_EXPECT_SCORE_AWAY,
	OMNIBET_EXPECT_TEAM_AWAY,  
};

typedef struct {
	char league_name[256];
	char match_time[256];
	int unsigned start_time;
	char team_home[256];
	char team_away[256];
	char score[256];
	int score_home;
	int score_away;
	int stage;
	int expect;
	gboolean skip;
} omnibet_match_data;

#define OMNIBET_URL1 "https://omnibet.ro"
#define OMNIBET_URL2 "https://omnibet.ro/live-scores/"
#define OMNIBET_CHARSET "UTF-8"
//#define OMNIBET_USER_AGENT "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; .NET CLR 1.0.3705; .NET CLR 1.1.4322)"
#define OMNIBET_USER_AGENT "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/83.0.4103.97 Safari/537.36"
#define OMNIBET_FILENAME "/tmp/omnibet.html"

