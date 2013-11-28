
#ifndef _STR_CACHE_
#define _STR_CACHE_

#define SIM 1
#define NAO 0

#include "block.h"
#include "list.h"


#include <sthread.h>		//para criar a thread que varre a cache
#ifdef USE_PTHREADS
#include <pthread.h>
#endif

//estrutura de uma entrada de cache
typedef struct icache_{
	int idBloco;
	int valido;		//o bloco permanece valido?
	int referenciado;	//referenciado
	int modificado;		//o bloco necessita de ser escrito antes de ser removido?
	int tempoEmCache;			//quando aguardar 10segundos, escrita em disco.
	char* conteudobloco;
}*icache_t;


//Definicao das estruturas da Hash
/**
 * Estrutura do nodo de uma tabela de hash.
 */
typedef struct sHashNode
{
  ///Apontador para a chave do nodo.
  int key;
  ///Apontador para a entrada associada.
  icache_t value;
  ///Apontador para o próximo nodo.
  struct sHashNode*  next;
}SHashNode;


/**
 * Definição do apontador para os nodes da tabela de hash.
 */
typedef SHashNode* HashNode;

/**
 * Estrutura de uma tabela de hash.
 */
typedef struct sHashMap
{
	int size;
  ///Tamanho do array que constitui a tabela.
  int length;
  ///Tabela de apontadores.
 HashNode* elems;
}SHashMap;
typedef SHashMap* HashMap;


typedef struct cache_{
	HashMap tabela;
	int numeroNosUtilizados;
	int numeroMaxNos;
	blocks_t* disco;
	List naoRef_naoMod;	//nao referenciadas, nao modificadas
	List naoRef_Mod;	//nao referenciadas, modificadas
	List Ref_naoMo;		//referenciadas, nao modificadas
	List Ref_Mod;		//referenciada,modificada
	
	sthread_mutex_t mutex;
}*cache_t;

#endif
