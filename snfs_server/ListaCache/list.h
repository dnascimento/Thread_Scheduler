/**
 * Implementação de uma lista (duplamente) ligada.
 * Esta biblioteca disponibiliza um conjunto de funções que permitem manipular
 *   uma lista (duplamente) ligada.
 *
 */
#ifndef _LIST_H_
#define _LIST_H_

#include "structCache.h"
#include <stdlib.h>

//#######Assinaturas da Lista Duplamente Ligada###################

/**
 * Cria uma lista,caso não seja possivel devolve NULL.
 *
 * @return lista inicializada ou NULL.
 */
List newList();

/**
 * Elimina uma lista.
 *
 * @attention apenas liberta a memória referente à estrutura da lista; não
 *            liberta o espaço ocupado pelos elementos nela contidos.
 *
 * @param list lista.
 */
void listDelete(List list);

/**
 * Insere um elemento na cabeça da lista.
 * Verifica se é possível inserir o elemento, devolvendo 1 caso não seja
 *   possível.
 *
 * @param list lista.
 * @param inf  endereço do elementos que queremos inserir.
 *
 * @return 0 se o elemento for inserido.
 *         1 se não for possível alocar memória para o novo elemento.
 */
int listInsertFst(List list,icache_t inf);

/**
 * Remove o primeiro elemento de uma lista.
 * Permite devolver a informação do elemento removido, caso o valor de @a inf
 *   seja diferente de NULL.
 * Se a lista estiver vazia é colocado o valor NULL em @a inf.
 *
 * @attention esta função não liberta o espaço ocupado pelo elemento removido.
 *
 * @param list lista.
 * @param inf  endereço onde é colocado o elemento removido (ou NULL).
 *
 * @return 0 se o elemento for removido;\n
 *         1 se a lista estiver vazia.
 */
int listRemoveFst(List list,icache_t* inf);

/**
 * Remove a ultima entrada da lista.
 * Permite devolver a informação do elemento removido, caso o valor de @a inf
 *   seja diferente de NULL.
 * Se a lista estiver vazia é colocado o valor NULL em @a inf.
 *
 * @attention esta função NÃO liberta o espaço ocupado pelo elemento removido.
 *
 * @param list lista.
 * @param inf  endereço onde é colocado o elemento removido (ou NULL).
 *
 * @return 0 se o elemento for removido;
 *         1 se a lista estiver vazia.
 */
int listRemoveLst(List list,icache_t* inf);

/**
 * Verifica qual o primeiro elemento de uma lista.
 * Se a lista estiver vazia é colocado o valor NULL em @a inf.
 *
 * @attention esta função coloca em @a inf o endereço do elemento pretendido;
 *            depois de executar esta função é aconselhável fazer uma cópia da
 *            informação e passar a trabalhar com a cópia para que não haja
 *            problemas de partilha de referências.
 *
 * @param list lista.
 * @param inf  endereço onde é colocado o resultado.
 *
 * @return 0 se a lista não estiver vazia;\n
 *         1 se a lista estiver vazia.
 */
int listFst(List list,icache_t* inf);

/**
 * Verifica qual o último elemento de uma lista.
 * Se a lista estiver vazia é colocado o valor NULL em @a inf.
 *
 * @attention esta função coloca em @a inf o endereço do elemento pretendido;
 *            depois de executar esta função é aconselhável fazer uma cópia da
 *            informação e passar a trabalhar com a cópia para que não haja
 *            problemas de partilha de referências.
 *
 * @param list lista.
 * @param inf  endereço onde é colocado o resultado.
 *
 * @return 0 se a lista não estiver vazia;
 *         1 se a lista estiver vazia.
 */
int listLst(List list,icache_t* inf);

/**
 * Devolve o valor do campo @a size da lista.
 *
 * @param list lista.
 *
 * @return número de elementos da lista.
 */
int listSize(List list);

/**Procura o bloco dentro da lista e remove-lo.
 * 	@param list - lista onde vamos procurar
 * 	@param id - identificador do bloco
 *  @param bloco - bloco que foi removido da lista 
 *
 * @return < 0 se nao conseguir remover da lista	*/

int listSearch_Remove(List list, int id,icache_t* bloco);

#endif
