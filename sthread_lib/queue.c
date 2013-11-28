/*
 * queue.c - implementation of queue manipulation functions
 */
#include "queue.h"

queue_element_t *empty_element_ptr;

void sthread_user_free(void *conteudo);

queue_t* create_queue() {
  queue_t *queue;
  queue = (queue_t*) malloc(sizeof(queue_t));
  queue->first = NULL;
  queue->last = NULL;
  empty_element_ptr = NULL;
  return queue;
}

void delete_queue(queue_t *queue) {
  queue_element_t *pointer, *next;
  pointer = queue->first;
  while(pointer){
    next = pointer->next;
    sthread_user_free(pointer->conteudo);
    free(pointer);
    pointer = next;
  }
  free(queue);
}

void queue_insert(queue_t *queue,void* conteudo) {

  queue_element_t *new_element;

  if(empty_element_ptr == NULL)
    new_element = (queue_element_t*) malloc(sizeof(queue_element_t));
  else {
    new_element = empty_element_ptr;
    empty_element_ptr = NULL;
  }

  new_element->conteudo = conteudo;
  new_element->next = NULL;

  if(queue->first == NULL) {
    queue->first = new_element;
    queue->last = new_element;
    return;
  }
  queue->last->next = new_element;
  queue->last = new_element;
}


int queue_is_empty(queue_t *queue){
  if (queue->first == NULL)return 1;
  else return 0;
}


struct _sthread * queue_firstThread(queue_t *queue){
	return (struct _sthread *) queue_first(queue);
}



void* queue_first(queue_t *queue) {
  queue_element_t *pointer; 
  pointer = queue->first;
  return pointer->conteudo;
}




struct _sthread* queue_removeThread(queue_t *queue){			/*Remover devolvendo o tipo thread*/
	return (struct _sthread*) queue_remove(queue);
}



struct _sthread_mutex * queue_removeMutex(queue_t *queue){		/*Realizar o cast do return*/
	return (struct _sthread_mutex *) queue_remove(queue);
}

struct _sthread_mon * queue_removeMonitor(queue_t *queue){
	return (struct _sthread_mon *) queue_remove(queue);
}



void* queue_remove(queue_t *queue) {
  queue_element_t *temp;
  void* conteudo;

  temp = queue->first;
  queue->first = temp->next;
  conteudo = temp->conteudo;
  temp->conteudo = NULL;
  temp->next = NULL;

  if(empty_element_ptr)free(temp);
  else empty_element_ptr = temp;
 
  return conteudo;
}


queue_element_t* queue_next(queue_element_t* no){
	return no->next;
}

queue_element_t * queue_firstElem(queue_t *queue){
	return queue->first;
}

