/*
 * queue.h
 *
 * Definition and declarations of sthread queue
 * 
 */

#ifndef _QUEUE_H_
#define _QUEUE_H_

#include "myfs.h"

/* queue_element_t */
typedef struct queue_element {
    fd_t fd;
    struct queue_element *next;
} queue_element_t;

/* queue_t */
typedef struct {
  queue_element_t *first;
  queue_element_t *last;
} queue_t;


/* queue_create - allocates memory for queue_t and initiates the structure */
queue_t* queue_create();


/* queue_destroy - frees all memory used by queue if it is empty */
int queue_destroy(queue_t *queue);


/* queue_is_empty - returns 1 if queue empty, else returns 0 */
int queue_is_empty(queue_t *queue);


/* queue_first - returns a pointer to the queue first element */
fd_t queue_first(queue_t *queue);


/* queue_enqueue - inserts a new element on the end of the queue */
void queue_enqueue(queue_t *queue, fd_t fd);


/* queue_dequeue - removes the first element of the queue */
fd_t queue_dequeue(queue_t *queue);

/* queue_thread_search - searches for a designated nodeinside the queue */
int queue_node_search(queue_t *queue, int fileId);

/* queue_thread_remove - searches for a designated node inside the queue and removes the queue node */
fd_t queue_node_remove(queue_t *queue, int fileId);

/* queue_node_get - searches for a designated node and returns its value */
fd_t queue_node_get(queue_t *queue, int fileId);

#endif
