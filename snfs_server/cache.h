/*
 * 
 *Cache
 * */
#ifndef _CACHE_
#define _CACHE_
#include <stdio.h>
#include <stdlib.h>
#include "fs.h"
#include "block.h"
#include "structCache.h"
#include "hash.h"
#include "list.h"

//##########Assinaturas da Cache#####################

/**Criar uma cache
 * @param dimensaoCache - nº de blocos que a cache deve suportar
 * @param disco - conjunto de blocos a que a cache ira aceder quando nao tiver o conteudo
 * - sistemaFicheiros - Sistema de ficherios a que pertencera a cache
 *@return a cache criada.
 */
cache_t criarCache(int dimensaoCache,blocks_t* disco);


/**Escrever um novo bloco na cache
 * @param cache - cache onde sera inserido
 * @param idBloco - numero do bloco
 * @return 0 se sucesso, <0 erro
 */
int escreverCache(cache_t cache,int idBloco,char* bloco);


/**Ler um bloco da cache: (caso nao exista na cache, carrega do disco)
 * @param cache - cache que pode conter o bloco
 * @param idBloco - numero de bloco
 * @return char* block - buffer de saida com o conteudo do bloco*/

int lerCache(cache_t cache,int idBloco,char* bloco);

/**
*Varre todos os blocos da cache, fazendo reset à sua referência e incrementando o tempo em cache em 2 segundos
*@param cache - cache que queremos varrer
*
**/
int actualizarCache(cache_t cache,int intervaloTempo);


/**Libertar espaco na cache
 *Quando a cache atinge uma determinada ocupacao, vamos tentar arranjar espaco na cache segundo a ordem:
 * 1- paginas nao referenciadas e nao modificadas,
 * 2- paginas nao referenciadas e modificadas
 * 3- referenciada nao modificada
 * 4- referenciada, modificada
 *@param cache em que queremos libertar espaco
 */
void libertarEspaco(cache_t cache);

/**
Visualizaa todo o conteudo da cache
*@param cache - cache cujo conteudo queremos visualizar
*/
int visualizarCache(cache_t cache);
	
/**retirarDaCache
*
*@param cache - cache da qual queremos retirar
*@param idBloco - nr de bloco que pretendemos retirar da cache
*@return 0 se for bem sucedido, -1 se ocorrer algum tipo de erro
*/
int retirarDaCache(cache_t cache,int idBloco);

/*Apagar o bloco da cache e do disco
 * return 1: apagou do disco*/
int eliminarBlocoCache(cache_t cache,unsigned idBloco);

/*Colocar uma entrada da cache como invalida
 * return 0 se colocou como invalido
 * return -1 se nao estava em cache*/
int invalidarBloco(cache_t cache,int idBloco);


#endif
