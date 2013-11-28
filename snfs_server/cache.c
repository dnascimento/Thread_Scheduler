/*
 * 
 * Implementacao de uma cache
 * 
 * Permite que o programa que a utiliza seja independente da estrutura
 * interna da cache.
 * 
 * Cache implementada com uma tabela hash por Encadeamento Externo
 * com uma funcao de hash em que o indice corresponde ao resto da
 * divisao inteira por 10.
 * 
 * A cache possui um contador de nos, limitando deste modo a quantidade
 * de nos disponiveis.
 * 
 * As entradas/chaves da cache sao determinadas atraves do numero de bloco
 * 
 * A cache tem uma politica de substituicao NRU(not recently used)
 * Devolve listas de entras:
 * 1- Nao referenciadas, nao modificadas
 * 2- nao referenciadas, modificadas
 * 3- referenciadas, nao modificadas
 * 4- referenciadas, modificadas
 * 5 - nao validas*/

/*Funcionamento da cache ao longo do tempo:
 * 1 - O sys pede para criar uma cache de uma dimensao fixa.
 * 2 - O sys solicita que seja devolvido um bloco
 * 			-Existe na cache, e devolvido e e considerado como R (referenciado desde a ultima passagem do paginador)
 * 			-Nao existe na cache, devolve null e o sistema tem de procurar no disco
 * 3 - Inserir bloco na cache. E inserido como referenciado,pergunta se modificado,valido
 * 
 * 4 - Procurar blocos livres
 * 
 * 
 * */
#include "cache.h"
#include "string.h"

#define BLOCK_SIZE 512

#define dprintf if(0) printf



/**Criar uma cache
 * - dimensaoCache - nº de blocos que a cache deve suportar
 * - disco - conjunto de blocos a que a cache ira aceder quando nao tiver o conteudo*/
cache_t criarCache(int dimensaoCache,blocks_t* disco){
	cache_t novaCache = (cache_t) malloc(sizeof(struct cache_));
	
	novaCache->tabela = newHash(dimensaoCache);
	novaCache->numeroNosUtilizados = 0;
	novaCache->numeroMaxNos = dimensaoCache;
	novaCache->disco = disco;
	novaCache->naoRef_naoMod = newList();
	novaCache->naoRef_Mod = newList();
	novaCache->Ref_naoMo = newList();
	novaCache->Ref_Mod = newList();
	
	novaCache->mutex = sthread_mutex_init();
	return novaCache;
}


/**Criar uma nova entrada na cache*/
icache_t novaEntradaCache(cache_t cache, int idbloco,int modificado,char* dados){
	int status;
	
	if(cache->numeroNosUtilizados == cache->numeroMaxNos){
		dprintf("\n[cache] cache cheia\n");
		libertarEspaco(cache);
	}
	
	
	icache_t blocoCache = (icache_t) malloc(sizeof(struct icache_));
		
	blocoCache->idBloco = idbloco;
	blocoCache->valido = SIM;
	blocoCache->referenciado = SIM;
	blocoCache->modificado = modificado;
	blocoCache->tempoEmCache = 0;
	
	char *bloco = (char*) malloc(sizeof(char)*BLOCK_SIZE);	//alocar um bloco para guardar estes dados
	memcpy(bloco,dados,BLOCK_SIZE);	//copiar o bloco para a cache
	
	blocoCache->conteudobloco = bloco;
	
	//status: 0 : inserido,1:substituiu,2:erro
	status = hashInsert(cache->tabela,blocoCache->idBloco,blocoCache);
	
	if(status == 0)	
		cache->numeroNosUtilizados++;
	
	return blocoCache;
}

/* Carregar um bloco do disco
 * - cache - cache onde sera inserido
 * - idBloco - numero do bloco
 * Devolve a estrutura de cache com o conteudo do bloco carregado do disco*/
icache_t carregarBlocoParaCache(cache_t cache,int idbloco){
	int status;
	
	//Carregar bloco em disco
	dprintf("Vou carregar do disco o bloco: %d\n",idbloco);
	char block [BLOCK_SIZE];
		
	status = block_read(cache->disco,idbloco,block);	//ler bloco a bloco do inode do ficheiro para um vector de entradas de directorio
	
	if(status != 0){
		dprintf("[cache] erro de leitura do disco\n");
		return NULL;
	}
	
	return novaEntradaCache(cache,idbloco,NAO,block);
}


/**Escrever um bloco completo em cache. Cuidado, quando for escrito em
 * disco ira substituir completamente o bloco antigo*/
int escreverCache(cache_t cache,int idbloco,char* bloco){
	icache_t entradaCache;
	
	sthread_mutex_lock(cache->mutex);
	
	int status = hashGet(cache->tabela,idbloco,&entradaCache);//procurar o icache do bloco na hash
	
	if(status == 0 && entradaCache->valido == NAO){	//existe e esta invalida
		retirarDaCache(cache,idbloco);			//Retirar o lixo da cache
		entradaCache = carregarBlocoParaCache(cache,idbloco); //carregar o bloco valido
	}	
	
	if(status == 1){ //a entrada nao existe, vamos criar uma entrada nova
		entradaCache = novaEntradaCache(cache,idbloco,SIM,bloco);
	}
	else{	//valida e existe
		if(entradaCache == NULL){
			dprintf("[cache] erro ao abrir o icache");
			sthread_mutex_unlock(cache->mutex);
			return -1;
		}
		dprintf("Substituir conteudo\n");
		memcpy(entradaCache->conteudobloco,bloco,BLOCK_SIZE);	//substituir o conteudo
		entradaCache->referenciado = SIM;
		entradaCache->modificado = SIM;
		entradaCache->valido = SIM;
	}
	
	
	
	dprintf("icache: bloco id: %d,valido: %d,referenciado %d,modificado: %d,tempoEmCache: %d\n conteudo: %s",entradaCache->idBloco,entradaCache->valido,entradaCache->referenciado,entradaCache->modificado,entradaCache->tempoEmCache,entradaCache->conteudobloco);
	sthread_mutex_unlock(cache->mutex);
	return 0;
}
	
/**Ler um bloco da cache: (caso nao exista na cache, carrega do disco)
 * - cache - cache que pode conter o bloco
 * - blocks_t - estrutura de blocos PERSISTENTE
 * - idBloco - numero de bloco
 * - bloco - ponteiro para o inicio do bloco pretendido [out]
 * 	return - <0 em caso de erro*/
int lerCache(cache_t cache,int idBloco,char* bloco){
	icache_t entradaCache;
	
	sthread_mutex_lock(cache->mutex);
	int status = hashGet(cache->tabela,idBloco,&entradaCache);//procurar o icache do bloco na hash

	if(status == 1){ //a entrada nao existe
		dprintf("bloco %d nao existe na cache\n",idBloco);
		entradaCache = carregarBlocoParaCache(cache,idBloco);
		entradaCache->valido = SIM;
	}
	
	if(!(entradaCache->valido)){
		retirarDaCache(cache,idBloco);			//Retirar o lixo da cache
		entradaCache = carregarBlocoParaCache(cache,idBloco); //carregar o bloco valido
		entradaCache->valido = SIM;
	}
	entradaCache->referenciado = SIM;
	
	char* ptr = entradaCache->conteudobloco;
	memcpy(bloco,ptr,BLOCK_SIZE);	//fazer a copia do bloco
	sthread_mutex_unlock(cache->mutex);
	return 0;
}

/*Colocar uma entrada da cache como invalida
 * return 0 se colocou como invalido
 * return -1 se nao estava em cache*/
int invalidarBloco(cache_t cache,int idBloco){
	icache_t entradaCache;
	sthread_mutex_lock(cache->mutex);
	int status = hashGet(cache->tabela,idBloco,&entradaCache);//procurar o icache do bloco na hash

	if(status == 1){ //a entrada nao existe
		sthread_mutex_unlock(cache->mutex);
		return -1;
	}
	
	
	entradaCache->valido = NAO;
	sthread_mutex_unlock(cache->mutex);
	return 0;
}
	
	

//retirar:
//Recebe a cache e o numero de bloco que pretendemos retirar da cache
int retirarDaCache(cache_t cache,int idBloco){
	icache_t blocoRemovido;
	int status;
	
	status = hashRemove(cache->tabela,idBloco,&blocoRemovido);

	if(status){
		dprintf("[retirar Bloco Da Cache] O bloco nao existe na cache\n");
		return -1;
	}
	if(blocoRemovido->modificado && blocoRemovido->valido == SIM){	//se e invalido, nao e escrito em disco
		dprintf("Vou escrever em disco o bloco %d removido\n",idBloco);
		status = block_write(cache->disco,idBloco,blocoRemovido->conteudobloco); //guardar o bloco em disco
		if(status<0){
			dprintf("[retirar Bloco Da Cache] Erro de escrita em disco\n");
			return -1;
		}
	}
	cache->numeroNosUtilizados--;	//eliminamos 1 bloco
	return 0;
}


//varrer todos os blocos da cache fazendo reset à sua referencia e incrementando o tempo em cache(2s)*/
int actualizarCache(cache_t cache,int intervaloTempo){
	HashMap hmap = cache->tabela;
	icache_t entrada;
	int i;
	HashNode aux;
	sthread_mutex_lock(cache->mutex);
	for(i = 0; i<hmap->length;i++)
		for(aux=hmap->elems[i];aux;aux=aux->next){
			entrada = aux->value;
			
			entrada->tempoEmCache += intervaloTempo;		//SUPONDO QUE O ALGORITMO DE ACTUALIZACAO E INVOCADO DE 2s em 2s
			entrada->referenciado = NAO;
			if(entrada->tempoEmCache >= 10){	//ja aguardou 10 segundos
				retirarDaCache(cache,entrada->idBloco);
			}
		}
	sthread_mutex_unlock(cache->mutex);
	return 0;
}


//Libertar espaco na cache//
/*Quando a cache atinge uma determinada ocupacao, vamos tentar arranjar espaco na cache segundo a ordem:
 *  - blocos invalidos
 * 1- blocos nao referenciadas e nao modificadas,
 * 2- blocos nao referenciadas e modificadas
 * 3- referenciada nao modificada
 * 4- referenciada, modificada*/

void libertarEspaco(cache_t cache){
	HashMap hmap = cache->tabela;
	icache_t entrada;
	int i,entradas_removidas;
	HashNode aux;
	
	for(i = 0; i<hmap->length;i++){
		for(aux=hmap->elems[i];aux;aux=aux->next){
			entrada = aux->value;
	
			if(entrada->valido == NAO){
				retirarDaCache(cache,entrada->idBloco);
				dprintf("[libertarEspaco - Algoritmo NRU] libertou bloco: %d : invalido\n",entrada->idBloco);
				entradas_removidas++;
			}
		}
	}
	
	if(entradas_removidas != 0){ //libertamos espaco?
		return;
	}
	
	//Pesquisar pelo grupo Nao Referenciada e nao modificada
	entradas_removidas = 0;
	for(i = 0; i<hmap->length;i++){
		for(aux=hmap->elems[i];aux;aux=aux->next){
			entrada = aux->value;
	
			if(!(entrada->referenciado) && !(entrada->modificado)){
				retirarDaCache(cache,entrada->idBloco);
				dprintf("[libertarEspaco - Algoritmo NRU] libertou bloco: %d : Nao Ref e Nao Mod\n",entrada->idBloco);
				entradas_removidas++;
			}
		}
	}
	
	if(entradas_removidas != 0){ //libertamos espaco?
		return;
	}
	
	//procurar no grupo Nao Referenciadas, Modificadas
	entradas_removidas = 0;
	for(i = 0; i<hmap->length;i++){
		for(aux=hmap->elems[i];aux;aux=aux->next){
			entrada = aux->value;
	
			if(!(entrada->referenciado) && (entrada->modificado)){
				retirarDaCache(cache,entrada->idBloco);
				dprintf("[libertarEspaco - Algoritmo NRU] libertou bloco: %d : Nao Ref e Mod\n",entrada->idBloco);
				entradas_removidas++;
			}
		}
	}
	
	if(entradas_removidas != 0){ //libertamos espaco?
		return;
	}
	
	//procurar no grupo Referenciadas, nao Modificadas
	entradas_removidas = 0;
	for(i = 0; i<hmap->length;i++){
		for(aux=hmap->elems[i];aux;aux=aux->next){
			entrada = aux->value;
	
			if((entrada->referenciado) && !(entrada->modificado)){
				retirarDaCache(cache,entrada->idBloco);
				dprintf("[libertarEspaco - Algoritmo NRU] libertou bloco: %d :Ref e Nao Mod\n",entrada->idBloco);
				entradas_removidas++;
			}
		}
	}
	
	if(entradas_removidas != 0){ //libertamos espaco?
		return;
	}
	
	//procurar no grupo Referenciadas, Modificadas
	entradas_removidas = 0;
	for(i = 0; i<hmap->length;i++){
		for(aux=hmap->elems[i];aux;aux=aux->next){
			entrada = aux->value;
	
			if((entrada->referenciado) && (entrada->modificado)){
				retirarDaCache(cache,entrada->idBloco);
				dprintf("[libertarEspaco - Algoritmo NRU] libertou bloco: %d : Ref e Mod\n",entrada->idBloco);
				entradas_removidas++;
			}
		}
	}
	
	if(entradas_removidas == 0) //libertamos espaco?
		dprintf("[libertarEspaco - Algoritmo NRU] nao libertou espaco nenhum");
	return;
}

/*Apagar um bloco do sistema:
 * - Se o bloco tem mais do que 1 ficheiro a referenciar, retira a referencia e esta pronto.
 * - Se existia em cache, fica invalido
 * - Apagar do disco
 * - cache
 * - indice do bloco a eliminar
 * - indice do inode do ficheiro que pretende apagar este bloco
 * 
 * return 1: apagou do disco
 * */
int eliminarBlocoCache(cache_t cache,unsigned idBloco){
	int status;
	icache_t entradaCache;
	
	sthread_mutex_lock(cache->mutex);
	status = hashGet(cache->tabela,idBloco,&entradaCache);//procurar o icache do bloco na hash
	
	if(status == 0){ //o bloco esta em cache
		entradaCache->valido = NAO;	//coloca como entrada invalida
	}
	
	char blocoVazio[BLOCK_SIZE];
    memset(&blocoVazio,0,sizeof(blocoVazio));	//criar um bloco completamente limpo
	
	block_write(cache->disco,idBloco,blocoVazio);	//formata colocando o bloco como vazio
	
	sthread_mutex_unlock(cache->mutex);
	return 1;	//removemos o bloco do disco
}
		
	


//Visualizar todo o conteudo da cache
/*
 * ===== Dump: Cache of Blocks Entries =======================
Entry_N: <No da Entrada na cache>
V: < Bit V de validade> D: <Dirty-Bit de escrita atrasada>
R: <Bit R do método NRU> M: <Bit M do método NRU>
Blk_Cnt: <Conteúdo do Bloco (em Hex?)>
************************************************************
*/
int visualizarCache(cache_t cache){
	HashMap hmap = cache->tabela;
	
	printf("===== Dump: Cache of Blocks Entries =======================\n");
	
	icache_t entrada;
	int i,k;
	HashNode aux;
	
	sthread_mutex_lock(cache->mutex);
	
	if(hmap->size == 0){
		dprintf("[visualizarCache] cache vazia\n");
	}
	else{
		for(i = 0; i<hmap->length;i++)
			for(aux=hmap->elems[i];aux;aux=aux->next){
				entrada = aux->value;
				printf("Entry_N: %u\n",entrada->idBloco);
				printf("V: %d\n",entrada->valido);
				printf("R: %d ",entrada->referenciado);
				printf("M: %d\n",entrada->modificado);
				printf("Blk_Cnt: ");
				for(k = 0; k<BLOCK_SIZE;k++){
					printf("%x:",entrada->conteudobloco[k]);
				}
				printf("\n");
		}		
	}
	
	sthread_mutex_unlock(cache->mutex);
	printf("************************************************************\n");
	return 0;
}

	
