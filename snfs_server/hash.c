//Funcoes da hash

#include "hash.h"
#include <stdio.h>

#include <sthread.h>
#ifdef USE_PTHREADS
#include <pthread.h>
#endif

#include <stdlib.h>

sthread_mutex_t mutex;  //MUTEX PARA CONTROLAR EXCLUSIVIDADE DE ACESSO

//#################################################################################
int funcaoHash(int key){ 
	return key%10;
}
//----------------------------------------------------------------------
int keyEquals(int n1,int n2){
	if(n1 == n2)
		return 1;
	else
		return 0;
}

//##############################################################################

HashMap newHash(int size)
{
	mutex = sthread_mutex_init();
	
	HashMap hmap=(HashMap)malloc(sizeof(SHashMap));

    if(!hmap) return NULL;
    else{
	hmap->size=0;
	hmap->length=size;
	hmap->elems=(HashNode*)calloc(size,sizeof(HashNode));

	if(hmap->elems) return hmap;
	else return NULL;
	}
}

void hashDelete(HashMap hmap)
{
  sthread_mutex_lock(mutex);
  int i;
  HashNode p,aux;

  for(i=0;i<hmap->length;i++)
	for(p=hmap->elems[i];p;){
		aux=p;
		p=p->next;
		free(aux);
	}
	free(hmap->elems);
	free(hmap);
  sthread_mutex_unlock(mutex);
}

//##############################################################################

int hashInsert(HashMap hmap,int key,icache_t value)
{
    sthread_mutex_lock(mutex);
	int h,stop;
	HashNode aux;
  
	h= funcaoHash(key)%hmap->length;
	for(aux=hmap->elems[h],stop=1;aux&&stop;aux=aux->next)
	{
		if(keyEquals(aux->key,key)){	//se as chaves forem iguais			
			value->modificado = SIM;	//vai ficar modificado
			value->referenciado = SIM;
			aux->value = value;//se o bloco ja existe vai ficar modificado
			stop=0;
		}
	}

	if(stop){
		aux=(HashNode)malloc(sizeof(SHashNode));
	if(aux){
		aux->key=key;
		aux->value=value;
		aux->next=hmap->elems[h];
		hmap->elems[h]=aux;

		hmap->size++;
		sthread_mutex_unlock(mutex);
		return 0;
    }
    
    else{
      sthread_mutex_unlock(mutex);
      return 2;
    }
  }
  else{
    sthread_mutex_unlock(mutex);
    return 1;
  }
}

//##############################################################################
/**
 * Remove um elemento de uma tabela de hash.
 * Permite devolver o valor removido-
 * Se a chave não existir ou o elemento não for removido é colocado o valor NULL
 *   em @a value.
 *
 * @param hmap  tabela de hash.
 * @param key   chave que queremos remover.
 * @param value endereço onde é colocado o elemento removido (ou NULL).
 *
 * @return 0 se o elemento for removido;\n
 *         1 se a chave não existir.
 */
int hashRemove(HashMap hmap,int key,icache_t* value)
{
  sthread_mutex_lock(mutex);
  int index;
  HashNode aux,*last;

  index=funcaoHash(key)%hmap->length;

  for(aux=hmap->elems[index],last=&(hmap->elems[index]);
      aux&&!keyEquals(key,aux->key);
      last=&(aux->next),aux=aux->next);

  if(aux)
  {
    *last=aux->next;
    if(*value) *value=aux->value;
    free(aux);
    hmap->size--;
    sthread_mutex_unlock(mutex);
    return 0;
  }
  else
  {
    if(value) *value=NULL;
    sthread_mutex_unlock(mutex);
    return 1;
  }
}

//##############################################################################
/** Return 0 se o elemento existe, 1 se nao existe*/
int hashGet(HashMap hmap,int key,icache_t* value)
{
  sthread_mutex_lock(mutex);
  HashNode aux;

  for(aux=hmap->elems[funcaoHash(key)%hmap->length];
      aux&&(!(keyEquals(key,aux->key)));
      aux=aux->next);

  if(aux)
  {
    *value=aux->value;
    sthread_mutex_unlock(mutex);
    return 0;
  }
  else
  {
    *value=NULL;
    sthread_mutex_unlock(mutex);
    return 1;
  }
}

//##############################################################################

int hashSize(HashMap hmap)
{
  return(hmap->size);
}

//##############################################################################

void percorrerHash(HashMap hmap){
  sthread_mutex_lock(mutex);
	int i;
	HashNode aux;
	for(i = 0; i<hmap->length;i++)
		for(aux=hmap->elems[i];aux;aux=aux->next)
			;
  sthread_mutex_unlock(mutex);
			//aux esta a apontar para a estrutura da hash
}


