/*
 * queue.h -- definition and declarations of void queue
 */
#include <stdlib.h>
#include <stdio.h>
#include <sthread.h>
#include <sthread_user.h>


/* queue_element_t */
typedef struct queue_element {
	void* conteudo;
    /*struct _sthread *thread;*/
    struct queue_element *next;
} queue_element_t;

/* queue_t */
typedef struct {
  queue_element_t *first;
  queue_element_t *last;
} queue_t;

/* create_queue - allocates memory for queue_t and initiates the structure */
queue_t* create_queue();

/* delete_queue - frees all memory used by queue and its elements */
void delete_queue(queue_t *queue);

/* queue_is_empty - returns 1 if queue empty, else returns 0 */
int queue_is_empty(queue_t *queue);

/* queue_first - returns a pointer to the queue first element */
void* queue_first(queue_t *queue);

/* queue_insert - inserts a new element on the end of the queue */
void queue_insert(queue_t *queue, void *conteudo);

/* queue_remove - removes the first element of the queue */
void* queue_remove(queue_t *queue);

/* queue_rotate - replaces active conteudo for the first in the queue, the old
 *                conteudo is inserted in the queue end */  
void* queue_rotate(queue_t *queue, void* old_thr);

/*Retornar o primeiro queue_element_t da lista*/

queue_element_t* queue_firstElem(queue_t *queue);

/* queue_next - da o ponteiro para o elemento seguinte do no actual(atencao pode ser null)*/

queue_element_t* queue_next(queue_element_t* no);


/*Interface para thread*/
struct _sthread* queue_removeThread(queue_t *queue);

struct _sthread * queue_firstThread(queue_t *queue);

/*Interface para mutex*/
struct _sthread_mutex * queue_removeMutex(queue_t *queue);


/*Interface para monitor*/
struct _sthread_mon * queue_removeMonitor(queue_t *queue);

