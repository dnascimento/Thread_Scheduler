/**
 * Implementação de uma lista (duplamente) ligada.
 *
 */
#include <stdlib.h>
#include "list.h"

List newList()
{
  List list=(List)malloc(sizeof(SList));

  if(list)
  {
    list->size=0;
    list->first=NULL;
    list->last=NULL;
  }
  return list;
}

//##############################################################################

void listDelete(List list)
{
  ListNode this,next;

  if(!list->size) free(list);
  else
  {
    for(this=list->first,next=this->next;next;this=next,next=next->next)
      free(this);

    free(this);
    free(list);
  }
}

//##############################################################################

int listInsertFst(List list,void* inf)
{
  ListNode new;

  if(!list->size)
  {
    new=(ListNode)malloc(sizeof(SListNode));
	
    if(new)
    {
      new->inf=inf;
      new->next=NULL;
      new->prev=NULL;

      list->size++;
      list->first=new;
      list->last=new;

      return 0;
    }
    else return 1;
  }
  else
  {
    new=(ListNode)malloc(sizeof(SListNode));

    if(new)
    {
      new->inf=inf;
      new->next=list->first;
      new->prev=NULL;

      list->size++;
      list->first->prev=new;
      list->first=new;

      return 0;
    }
    else return 1;
  }
}

//##############################################################################

int listInsertLst(List list,void* inf)
{
  ListNode new;

  if(!list->size)
  {
    new=(ListNode)malloc(sizeof(SListNode));

    if(new)
    {
      new->inf=inf;
      new->next=NULL;
      new->prev=NULL;

      list->size++;
      list->first=new;
      list->last=new;

      return 0;
    }
    else return 1;
  }
  else
  {
    new=(ListNode)malloc(sizeof(SListNode));

    if(new)
    {
      new->inf=inf;
      new->next=NULL;
      new->prev=list->last;

      list->size++;
      list->last->next=new;
      list->last=new;

      return 0;
    }
    else return 1;
  }
}

//##############################################################################

int listInsertAt(List list,int index,void* inf)
{
  int size=listSize(list);
  ListNode aux,new;
  
  if(index<0||index>size) return 1;
  else if(!index) return(listInsertFst(list,inf));
  else if(index==size) return(listInsertLst(list,inf));
  else if(index>size/2)
  {
    for(index=size-index,aux=list->last;index>0;index--,aux=aux->prev);

    new=(ListNode)malloc(sizeof(SListNode));

    if(new)
    {
      new->inf=inf;
      new->next=aux->next;
      new->prev=aux;

      list->size++;
      aux->next->prev=new;
      aux->next=new;

      return 0;
    }
    else return 1;
  }
  else
  {
    for(aux=list->first;index>0;index--,aux=aux->next);

    new=(ListNode)malloc(sizeof(SListNode));

    if(new)
    {
      new->inf=inf;
      new->next=aux;
      new->prev=aux->prev;

      list->size++;
      aux->prev->next=new;
      aux->prev=new;

      return 0;
    }
    else return 0;
  }
}

//##############################################################################

int listRemoveFst(List list,void** inf)
{
  ListNode aux;
  
  if(!list->size)
  {
    if(inf) *inf=NULL;
    return 1;
  }
  else if(list->size==1)
  {
    if(inf) *inf=list->first->inf;
    free(list->first);
    list->first=NULL;
    list->last=NULL;
    list->size=0;

    return 0;
  }
  else
  {
    if(inf) *inf=list->first->inf;
    aux=list->first;
    list->first->next->prev=NULL;
    list->first=list->first->next;
    free(aux);
    list->size--;

    return 0;
  }
}

//##############################################################################

int listRemoveLst(List list,void** inf)
{
  ListNode aux;

  if(!list->size)
  {
    if(inf) *inf=NULL;
    return 1;
  }
  else if(list->size==1)
  {
    if(inf) *inf=list->last->inf;
    free(list->last);
    list->first=NULL;
    list->last=NULL;
    list->size=0;

    return 0;
  }
  else
  {
    if(inf) *inf=list->last->inf;
    aux=list->last;
    list->last->prev->next=NULL;
    list->last=list->last->prev;
    free(aux);
    list->size--;

    return 0;
  }
}

//##############################################################################

int listRemoveAt(List list,int index,void** inf)
{
  int size=listSize(list);
  ListNode aux;
  
  if(index<0||index>size-1) return 1;
  else if(index==0) return listRemoveFst(list,inf);
  else if(index==size-1) return listRemoveLst(list,inf);
  else if(index>size/2)
  {
    for(aux=list->last,index=size-index-1;index>0;index--,aux=aux->prev);

    if(inf) *inf=aux->inf;
    aux->prev->next=aux->next;
    aux->next->prev=aux->prev;
    free(aux);
    list->size--;

    return 0;
  }
  else
  {
    for(aux=list->first;index>0;index--,aux=aux->next);

    if(inf) *inf=aux->inf;
    aux->prev->next=aux->next;
    aux->next->prev=aux->prev;
    free(aux);
    list->size--;

    return 0;
  }
}

//##############################################################################

int listFst(List list,void** inf)
{
  if(!list->size)
  {
    *inf=NULL;
    return 1;
  }
  else
  {
    *inf=list->first->inf;
    return 0;
  }
}

//##############################################################################

int listLst(List list,void** inf)
{
  if(!list->size)
  {
    *inf=NULL;
    return 1;
  }
  else
  {
    *inf=list->last->inf;
    return 0;
  }
}

//##############################################################################

int listAt(List list,int index,void** inf)
{
  int size=listSize(list);
  ListNode aux;

  if(index<0||index>size-1)
  {
    *inf=NULL;
    return 1;
  }
  else
  {
    if(index>size/2) for(index=size-index-1,aux=list->last;index>0;index--,aux=aux->prev);
    else for(aux=list->first;index>0;index--,aux=aux->next);

    *inf=aux->inf;
    return 0;
  }
}

//##############################################################################

int listSize(List list)
{
  return(list->size);
}


/**Inserir ordenadamente na lista segundo um inteiro*/
int inserirOrdenado(List list,void* inf,unsigned short ordem){
	ListNode new;
	ListNode ptr,ptr_anterior;
	if(!list->size){	//lista vazia
		new=(ListNode)malloc(sizeof(SListNode));
		if(new){
			new->inf=inf;
			new->next=NULL;
			new->prev=NULL;
			new->ordem = ordem;
	
			list->size++;
			list->first=new;
			list->last=new;
			
			return 0;
		}
		else return 1;
	}
	else{
		new=(ListNode)malloc(sizeof(SListNode));
		
		if(new)
		{
		  new->inf=inf;
		  new->ordem = ordem;
		  list->size++;
		  if(list->first->ordem > ordem){	//inserir na cabeca da lista
				list->first->prev = new;
				new->prev = NULL;
				new->next = list->first;
				list->first = new;
				return 0;
			}
			
			if(list->last->ordem < ordem){	//inserir no final da lista
				list->last->next = new;
				new->next = NULL;
				new->prev = list->last;
				list->last = new;
				return 0;
			}
			
			 ptr = list->first;
			while(ptr != NULL && ptr->ordem < ordem)	//inserir por ordem crescente
					ptr = ptr->next;
			
			if(ptr == NULL)
				return -1;
			
			ptr_anterior = ptr->prev;
			if(ptr_anterior == NULL)
				return -1;
			
			ptr_anterior->next = new;
			new->prev = ptr_anterior;
			new->next = ptr;
			ptr->prev = new;
			return 0;
		}
		else return 1;
	  }
}
