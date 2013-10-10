#include <string.h>
#include <stdlib.h>
#include <libxml/HTMLparser.h>
#include <libxml/HTMLtree.h>

/*
$var1="http://www.iddaamerkezi.com/feed_plugin.php";
$var5="Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; .NET CLR 1.0.3705; .NET CLR 1.1.4322)";
 */

int now_parsing = -1;

enum {
	PARSING_LEAGUE = 0,
	PARSING_TIME,
	PARSING_HOME,
	PARSING_SCORE,
	PARSING_AWAY
};

char league_name[256];
char the_time[256];
char home_team[256];
char away_team[256];
char score[32];
int home_score = 0;
int away_score = 0;

char *trim(char *s) {
	char *ptr;
	if (!s)
        	return NULL;   // handle NULL string
	if (!*s)
        	return s;      // handle empty string
	while (isspace (*s))	// remove left spaces
		s++; 
	for (ptr = s + strlen(s) - 1; (ptr >= s) && isspace(*ptr); --ptr);
	ptr[1] = '\0';
	return s;
}

void split_score(char *s, int *home, int *away) {
	if (!strstr(s, "?")) {
		*home = atoi(trim(strtok(s, "-")));
		*away = atoi(trim(strtok(NULL, "-")));
	}
}

void walkTree(xmlNode * a_node) {
	xmlNode *cur_node = NULL;
	xmlAttr *cur_attr = NULL;
	for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
		// do something with that node information, like… printing the tag’s name and attributes
		//printf("Got tag : %s\n", cur_node->name);

		if (cur_node->content && (strlen(cur_node->content) > 1)) {
			if (now_parsing == PARSING_LEAGUE)
				strcat(&league_name[0], cur_node->content);
			else if (now_parsing == PARSING_TIME)
				strcat(&the_time[0], cur_node->content);
			else if (now_parsing == PARSING_HOME) 
				strcat(&home_team[0], cur_node->content);
			else if (now_parsing == PARSING_SCORE)
				strcat(&score[0], cur_node->content);
			else if (now_parsing == PARSING_AWAY)
				strcat(&away_team[0], cur_node->content);

		}
		for (cur_attr = cur_node->properties; cur_attr; cur_attr = cur_attr->next) {
			if (!strcmp(cur_attr->name, "class")) {
				if (!strcmp(cur_attr->children->content, "livescore-table")) {
					memset(&league_name[0], '\0', sizeof(league_name));
					now_parsing = PARSING_LEAGUE;
				}
				else if (!strcmp(cur_attr->children->content, "live_time")) {
					now_parsing = PARSING_TIME;
				
					if(strlen(&score[0]) > 2) {
						split_score(&score[0], &home_score, &away_score);
						printf("Got league: %s\n", trim(&league_name[0]));
						printf("Got row: %s %s %u - %u %s\n", trim(&home_team[0]), trim(&away_team[0]), home_score, away_score, &the_time[0]);
						memset(&the_time[0], '\0', sizeof(the_time));
						memset(&home_team[0], '\0', sizeof(home_team));
						memset(&away_team[0], '\0', sizeof(away_team));
						memset(&score[0], '\0', sizeof(score));
					}
				}
				else if (!strcmp(cur_attr->children->content, "live_home")) {
					now_parsing = PARSING_HOME;
				}
				else if (!strcmp(cur_attr->children->content, "live_score")) {
					now_parsing = PARSING_SCORE;
				}
				else if (!strcmp(cur_attr->children->content, "live_away")) {
					now_parsing = PARSING_AWAY;
				}
			}	
			//printf("  -> with attribute : %s\n", cur_attr->name);
			//printf("  -> with attr value : %s\n", cur_attr->children->content);
		}

		walkTree(cur_node->children);
	}
}

int main(int argc, char ** argv) {
	memset(&the_time[0], '\0', sizeof(the_time));
        memset(&home_team[0], '\0', sizeof(home_team));
        memset(&away_team[0], '\0', sizeof(away_team));
        memset(&score[0], '\0', sizeof(score));

	htmlDocPtr parser = htmlReadFile("http://localhost/feed_plugin.php.html", "UTF-8", HTML_PARSE_NOBLANKS | HTML_PARSE_NOIMPLIED | HTML_PARSE_COMPACT);
	
	walkTree(xmlDocGetRootElement(parser));

	return 0;
}
