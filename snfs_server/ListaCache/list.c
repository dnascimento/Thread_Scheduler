//Funcoes da lista

#include "list.h"

//######################ESTRUTURA DA LISTA DUPLAMENTE LIGADA#####################
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
/**INserir na cabeca da lista*/
int listInsertFst(List list,icache_t inf)
{
  ListNode new;

  if(!list->size)
  {
    new=(ListNode)malloc(sizeof(SListNode));

    if(new)
    {
      new->conteudo=inf;
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
      new->conteudo=inf;
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
/**Remover a ultima entrada da lista*/
int listRemoveLst(List list,icache_t* inf)
{
  ListNode aux;

  if(!list->size)
  {
    if(inf) *inf=NULL;
    return 1;
  }
  else if(list->size==1)
  {
    if(inf) *inf=list->last->conteudo;
    free(list->last);
    list->first=NULL;
    list->last=NULL;
    list->size=0;

    return 0;
  }
  else
  {
    if(inf) *inf=list->last->conteudo;
    aux=list->last;
    list->last->prev->next=NULL;
    list->last=list->last->prev;
    free(aux);
    list->size--;

    return 0;
  }
}

//##############################################################################
/**Obter o primeiro elemento da lista*/
int listFst(List list,icache_t*inf)
{
  if(!list->size)
  {
    *inf=NULL;
    return 1;
  }
  else
  {
    *inf=list->first->conteudo;
    return 0;
  }
}

//##############################################################################
/**Obter o ultimo elemento da lista*/
int listLst(List list,icache_t* inf)
{
  if(!list->size)
  {
    *inf=NULL;
    return 1;
  }
  else
  {
    *inf=list->last->conteudo;
    return 0;
  }
}

//##############################################################################
/**Obter a dimensao da lista*/
int listSize(List list)
{
  return(list->size);
}

//##############################################################################
/**Procura o bloco dentro da lista e remove-lo.
 * 	list - lista onde vamos procurar
 * 	id - identificador do bloco
 *  bloco - bloco que foi removido da lista 
 * 	Devolve < 0 se nao conseguir remover da lista	*/
int listSearch_Remove(List list, int id,icache_t* bloco){
	
	ListNode p;
	ListNode anterior;
	ListNode seguinte;
	
	if(list == NULL){
		return -1;	//lista nao existente
	}
	
	p = list->first;
	if(p == NULL){
		return -2;	//a lista vazia
	}
	
	while(p != NULL && p->conteudo != NULL && p->conteudo->idBloco != id);
	
	if(p == NULL)
		return -3; //o bloco nao existe na lista
	
	anterior = p->prev;
	seguinte = p->next;
	
	if(p->prev != NULL)	//nao estamos na cabeca da lista
		anterior->next = seguinte;
	
	if(p->next != NULL) //nao estamos no final da lista
		seguinte->prev = anterior;
	
	//ja garanti que p esta isolado da lista
	*bloco = p->conteudo;
	
	p->conteudo = NULL;
	free(p);
	
	return 0;
}
	

	
