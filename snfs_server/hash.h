#ifndef _HASH_MAP_
#define _HASH_MAP_

#include "structCache.h"
#include <stdlib.h>

//######ASSINATURAS DA HASH###############
/**
 * Cria uma tabela de hash.
 * Se não for possível devolve NULL.
 * 
 * @param size   dimensão inicial da tabela.
 * @return tabela inicializada ou NULL.
 */
HashMap newHash(int size);


/**
 * Insere um par chave/valor numa tabela de hash.
 *
 * @param hmap    tabela de hash.
 * @param key     chave.
 * @param value   valor associado à chave.
 *
 * @return 0 se o elemento for inserido;\n
 *         1 se a tabela já possuia a chave indicada;\n
 *         2 se não for possível alocar memória para o novo elemento.
 */
int hashInsert(HashMap hmap,int key,icache_t value);


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
int hashRemove(HashMap hmap,int key,icache_t* v);


/**
 *Devolve o tamanho da tabela de hash.
 * @param hmap tabela de hash.
 *
 * @return número de elementos da tabela.
 */
int hashSize(HashMap hmap);

/**
 * Devolve o indice correspondente para a key dada
 * */
int funcaoHash(int key);


/**
 * Ve se as duas keys sao iguais
 * @return 1 se sao, 0 caso contrario
 * */
int keyEquals(int n1,int n2);


/**
 * Procura um elemento associado a uma chave, se ela existir, numa tabela de hash.
 * Se a chave nao existir coloca NULL em @value.
 *
 * @attention esta função coloca em @a value o endereço do valor pretendido;
 *            depois de executar esta função é aconselhável fazer uma cópia do
 *            mesmo e passar a trabalhar com a cópia para que não haja problemas
 *            de partilha de referências.
 *
 * @param hmap  tabela de hash.
 * @param key   chave que procuramos.
 * @param value endereço onde é colocado o resultado.
 *
 * @return 0 se o elemento existir;
 *         1 se o elemento não existir.
 */
int hashGet(HashMap hmap,int key,icache_t* value);


/**
 * Elimina uma tabela de hash.
 *
 * @attention apenas liberta a memória referente à estrutura da tabela; não
 *            liberta o espaço ocupado pelos elementos nela contidos.
 *
 * @param hmap tabela de hash.
 */
void hashDelete(HashMap hmap);

/**
 * Percorre uma hashmap
 * @param hmap hashmap a ser percorrida
 * */
void percorrerHash(HashMap hmap);


#endif
