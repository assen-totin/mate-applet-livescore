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
#include "http.h"

int get_url (char *url, char *user_agent, char *filename) {
	SoupMessage *msg;
	const char *header;
	FILE *output_file = NULL;

	SoupSession *session = g_object_new (SOUP_TYPE_SESSION_SYNC,
				SOUP_SESSION_ADD_FEATURE_BY_TYPE, SOUP_TYPE_CONTENT_DECODER,
				SOUP_SESSION_ADD_FEATURE_BY_TYPE, SOUP_TYPE_COOKIE_JAR,
#ifdef HAVE_MATE
				SOUP_SESSION_ACCEPT_LANGUAGE_AUTO, TRUE,
#endif
				NULL);
	if (user_agent)
		g_object_set(session, "user-agent", user_agent);
	else {
		char ua[1024];
		sprintf(&ua[0], "%s", HTTP_USER_AGENT);
		g_object_set(session, "user-agent", &ua[0]);
	}
	msg = soup_message_new ("GET", url);
	soup_message_set_flags (msg, SOUP_MESSAGE_NO_REDIRECT);
	g_object_ref (msg);
	soup_session_send_message (session, msg);
	if (SOUP_STATUS_IS_TRANSPORT_ERROR (msg->status_code)) {
		char _err[1024];
		sprintf (&_err[0], "%s: %d %s\n", soup_message_get_uri(msg)->path, msg->status_code, msg->reason_phrase);
		//debug(&_err[0]);
		return 1;
	}

	else if (SOUP_STATUS_IS_REDIRECTION (msg->status_code)) {
		header = soup_message_headers_get_one (msg->response_headers, "Location");
		if (header) {
			SoupURI *uri;
			char *uri_string;
			uri = soup_uri_new_with_base (soup_message_get_uri (msg), header);
			uri_string = soup_uri_to_string (uri, FALSE);
			get_url (uri_string, user_agent, filename);
			g_free (uri_string);
			soup_uri_free (uri);
			return 0;
		}
	} 

	else if (SOUP_STATUS_IS_SUCCESSFUL (msg->status_code)) {
		output_file = fopen (filename, "w");
		if (!output_file) {
			g_printerr ("Error trying to create file %s.\n", filename);
			return 1;
		}

		fwrite (msg->response_body->data,
			1,
			msg->response_body->length,
			output_file);

		fclose (output_file);

		return 0;
	}
}

