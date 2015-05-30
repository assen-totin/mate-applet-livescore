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


/* New FIFO queue*/ 
fifo *fifo_new(void) {
	fifo *queue = malloc(sizeof(fifo));
	queue->fifo_tail = NULL;
	queue->fifo_head = NULL;
	return (queue);
}


/* Add to the end of the fifo */
void fifo_add(fifo *queue, void *data) {
	fifo_data *node = malloc(sizeof(fifo_data));

	node->node_data = data;
	node->node_next = NULL;

	if (queue->fifo_tail == NULL)
		queue->fifo_head = queue->fifo_tail = node;
	else {
		queue->fifo_tail->node_next = node;
		queue->fifo_tail = node;
	}
}


/* Remove from the front of the fifo */
void *fifo_remove(fifo *queue) {
	fifo_data *node;
	void *data;

	if ((node = queue->fifo_head) == NULL)
		return (NULL);

	data = node->node_data;
	if ((queue->fifo_head = node->node_next) == NULL)
		queue->fifo_tail = NULL;

	//free(node);

	return (data);
}


/* Get the length of the fifo */
int fifo_len(fifo *queue) {
	fifo_data *node;
	int i;

	for (i = 0, node = queue->fifo_head; node; node = node->node_next, i++);

	return (i);
}


/* Free an entire fifo */
void fifo_free(fifo *queue) {
	fifo_data *node = queue->fifo_head;
	fifo_data *tmp;

	if (fifo_len(queue) > 0) {
		while (node->node_next) {
			tmp = node;
			node = node->node_next;
			free(tmp);
		}
		free(node);
	}

	free(queue);
}


gboolean fifo_is_empty(fifo *queue) {
	return (queue->fifo_head == NULL);
}

