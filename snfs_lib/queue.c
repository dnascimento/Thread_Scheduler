/*
 * queue.c
 *
 * Implementation of queue manipulation functions
 * 
 */


#include <stdio.h>
#include <stdlib.h>
#include "myfs.h"
#include "queue.h"


queue_t* queue_create()
{
	queue_t *queue;
	queue = (queue_t*) malloc(sizeof(queue_t));
	queue->first = NULL;
	queue->last = NULL;
	return queue;
}


int queue_destroy(queue_t *queue)
{
	if (queue == NULL) {
		return 0;
	}
	if (queue->first != NULL) {
		return -1;
	} else {
		free(queue);
		return 0;
	}
}


void queue_enqueue(queue_t *queue, fd_t fd)
{
	if (queue == NULL || fd == NULL) {
		return;
	}

	queue_element_t *new_element;
	new_element = (queue_element_t*) malloc(sizeof(queue_element_t));
	new_element->fd = fd;
	new_element->next = NULL;

	if(queue->first == NULL) {
		queue->first = new_element;
		queue->last = new_element;
	} else {
		queue->last->next = new_element;
		queue->last = new_element;
	}
}


int queue_is_empty(queue_t *queue)
{
	if (queue == NULL || queue->first == NULL) {
		return 1;
	} else {
		return 0;
	}
}


fd_t queue_first(queue_t *queue)
{
	if (queue != NULL && queue->first != NULL) {
		return queue->first->fd;
	} else {
		return NULL;
	}
}


fd_t queue_dequeue(queue_t *queue)
{
	if (queue == NULL || queue->first == NULL) {
		return NULL;
	}
   
	queue_element_t *out_element = queue->first;
	if (queue->first == queue->last) {
		queue->first = NULL;
		queue->last = NULL;
	} else {
		queue->first = out_element->next;
	}
   
	fd_t ret = out_element->fd;
	free(out_element);
	return ret;
}

int queue_node_search(queue_t *queue, int fileId) {
	queue_element_t *aux = queue->first;
	
	if (queue == NULL) {
		return 0;
	}
	
	while(aux != NULL) {
		if(aux->fd->fileId == fileId)
			return 1;
			
		aux = aux->next;
	}
	
	return 0;
}

fd_t queue_node_remove(queue_t *queue, int fileId) {
	queue_element_t *aux1 = queue->first, *aux2;
	
	if (queue == NULL) {
		return NULL;
	}
	
	if(aux1 != NULL) {
		if(aux1->fd->fileId == fileId) {
			return queue_dequeue(queue);
		}
		
		aux2 = aux1->next;
		while(aux2 != NULL) {
			if(aux2->fd->fileId == fileId) {
				if(queue->last == aux2)
					queue->last = aux1;
				aux1->next = aux2->next;
				fd_t fd = aux2->fd;
				free(aux2);
				return fd;
			}
			aux1 = aux2;
			aux2 = aux2->next;
		}
	}
	
	return NULL;
}

fd_t queue_node_get(queue_t *queue, int fileId) {
	queue_element_t *aux = queue->first;
	
	if (queue == NULL) {
		return NULL;
	}
	
	while(aux != NULL) {
		if(aux->fd->fileId == fileId)
			return aux->fd;
			
		aux = aux->next;
	}
	
	return NULL;
}
