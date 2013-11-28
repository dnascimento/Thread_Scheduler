/* 
 * File System Layer
 * 
 * fs.c
 *
 * Implementation of the file system layer. Manages the internal 
 * organization of files and directories in a 'virtual memory disk'.
 * Implements the interface functions specified in fs.h.
 *
 * 
 * 
 * 	Funcoes do sistema de ficheiros. É aqui que temos de trabalhar no servidor
 * 
 * 
 *  
 * Caracteristicas do sistema de ficheiros:
 * Tamanho maximo de ficheiro: 10 blocos
 * Suporta a criacao de subdirectorios no directorio de raiz
 * Tamanho dos blocos tem dimensao fixa de 512bytes. O numero de blocos e definido em tempo de compilacao
 * 
 * O sistema de ficheiros é baseado em i-nodes com 10 entradas directas para blocos de dados.
 * Cada i-node ocupada 64bytes. 
 * Temos 64 i-nodes por omissao (a tabela assim ocupa 8 blocos) o valor pode ser alterado em tempo de compilacao
 * 
 * Directorios:
 * 		Fichiero com tabela de entradas, 16 bytes: nomeficheiro ou directorio | numero de idone a 2bytes
 *		Podem ocupar no maximo 4 blocos, ou seja, ate 128 entradas
 * 
 * 
 * BLOCO:
 * |BLOCO0|Reservado para o bitmap de blocos Livres|
 * |Bloco1|Reservado para bitmap de i-nodes livres|
 * |Bloco2-9|Reservado para tabela de i-nodes|
 * |Bloco>10| dados de ficheiros e directorios|
 */


#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "fs.h"
#include "cache.h"

#include <sthread.h>		//para criar a thread que varre a cache
#ifdef USE_PTHREADS
#include <pthread.h>
#endif



#define dprintf if(0) printf

#define INTERVALO_TEMPO_CACHE 2
#define CACHE_ACTIVA 1   //1 cache activa, 0 cache inactiva

#define INODES_USED_BY_FS  1  //blocos ocupados pelo fs
#define NUM_BLOCOS_MAX INODE_NUM_BLKS*ITAB_SIZE //maximo de blocos que podem ser apontados

#define LEITURA 0
#define ESCRITA 1

#define BLOCK_SIZE 512

#define DIM_CACHE 8	//dimensao da cache

/*
 * Inode
 * - inode size = 64 bytes
 * - num of direct block refs = 10 blocks
 */

#define INODE_NUM_BLKS 10

#define EXT_INODE_NUM_BLKS (BLOCK_SIZE / sizeof(unsigned int))

//inode do FileSystem: tipo, dimensao, vector de ponteiros para blocos, 4 para realizar a extensao da tabela
typedef struct fs_inode {
   fs_itype_t type;			//tipo do i-node: FS_DIR
   unsigned int size;		//dimensao 
   unsigned int blocks[INODE_NUM_BLKS];//blocos associados ao inode
   unsigned int reserved[4]; // reserved[0] -> extending table block number

} fs_inode_t;

typedef unsigned int fs_inode_ext_t;


/*
 * Directory entry
 * - directory entry size = 16 bytes
 * - filename max size - 14 bytes (13 chars + '\0') defined in fs.h
 */

#define DIR_PAGE_ENTRIES (BLOCK_SIZE / sizeof(fs_dentry_t))	//n dentrys por bloco
/*Directory entry - Estrutura para o directorio
 * 	Associa um nome a um inode*/
typedef struct dentry {
   char name[FS_MAX_FNAME_SZ]; //directorio
   inodeid_t inodeid;	//id do inode na tabela de inodes
} fs_dentry_t;


/*
 * File syste structure
 * - inode table size = 64 entries (8 blocks)
 * 
 * Internal organization 
 *   - block 0        - free block bitmap
 *   - block 1        - free inode bitmap
 *   - block 2-9      - inode table (8 blocks)
 *   - block 10-(N-1) - data blocks, where N is the number of blocks
 */

#define ITAB_NUM_BLKS 8 //numero de blocos da tabela de cada inode

#define ITAB_SIZE (ITAB_NUM_BLKS*BLOCK_SIZE / sizeof(fs_inode_t)) //tamanho de cada tabela Inodes

struct fs_ {		//um file system e constituido por um 
   blocks_t* blocks;		//estrutura de dados percistente
   cache_t cache;			//uma cache
   char* referencias;		//contador de referencias (char porque basta 1 byte para contar o numero de referencias*/
   char inode_bmap [BLOCK_SIZE];	//bitmap de inodes
   char blk_bmap [BLOCK_SIZE];		//bitmap de blocos
   fs_inode_t inode_tab [ITAB_SIZE]; //uma tabela de inodes
   
   //cada FS controla os seus acessos
   sthread_mon_t monLeitores;	//monitor para os leitores
   sthread_mon_t monEscritores;//monitor para os escritores
   int leitores;	//numero de leitores activos
   int escritores;	//numero de escritores activos
   int wanna_leitor;	//numero de threads que querem ler
   int wanna_escritor;	//numero de threads que querem escrever   
   int turno;		//leitura ou escrita
};

#define NOT_FS_INITIALIZER  1
                               
/*
 * Internal functions for loading/storing file system metadata do the blocks
 */
                                
                                
static void fsi_load_fsdata(fs_t* fs)
{
   blocks_t* bks = fs->blocks;

   // load free block bitmap from block 0
   /*if(CACHE_ACTIVA)
		lerCache(fs->cache,0,fs->blk_bmap);
   else*/
		block_read(bks,0,fs->blk_bmap);

   // load free inode bitmap from block 1
  /* if(CACHE_ACTIVA)
		lerCache(fs->cache,1,fs->inode_bmap);
   else*/
		block_read(bks,1,fs->inode_bmap);
   
   // load inode table from blocks 2-9
   for (int i = 0; i < ITAB_NUM_BLKS; i++) {
     /* if(CACHE_ACTIVA)
		lerCache(fs->cache,i+2,&((char*)fs->inode_tab)[i*BLOCK_SIZE]);
	else*/
		block_read(bks,i+2,&((char*)fs->inode_tab)[i*BLOCK_SIZE]);
   }
#define NOT_FS_INITIALIZER  1  //file system is already initialized, subsequent block acess will be delayed using a sleep function.
}


static void fsi_store_fsdata(fs_t* fs)
{
   blocks_t* bks = fs->blocks;
 
   // store free block bitmap to block 0
   //if(CACHE_ACTIVA)
	//	escreverCache(fs->cache,0,fs->blk_bmap);
   //else
		block_write(bks,0,fs->blk_bmap);	//os metadados sao gravados de imediato em disco

   // store free inode bitmap to block 1
  // if(CACHE_ACTIVA)
	//	escreverCache(fs->cache,1,fs->inode_bmap);
	//else
		block_write(bks,1,fs->inode_bmap);
   
   // store inode table to blocks 2-9
   for (int i = 0; i < ITAB_NUM_BLKS; i++) {
	  // if(CACHE_ACTIVA)
		//	escreverCache(fs->cache,i+2,&((char*)fs->inode_tab)[i*BLOCK_SIZE]);
		//else
			block_write(bks,i+2,&((char*)fs->inode_tab)[i*BLOCK_SIZE]);
   }
}


/*
 * Bitmap management macros and functions
 */

#define BMAP_SET(bmap,num) ((bmap)[(num)/8]|=(0x1<<((num)%8)))	//bpmap, num esta OCUPADO

#define BMAP_CLR(bmap,num) ((bmap)[(num)/8]&=~((0x1<<((num)%8))))	//num esta LIVRE

#define BMAP_ISSET(bmap,num) ((bmap)[(num)/8]&(0x1<<((num)%8)))		//qual o estado de num?

/*Procura blocos livres no bmap. Retorna 0 se nao ha livres,
 * 1 se ha livres e em free o indice do bit que esta livre para um novo inode*/
static int fsi_bmap_find_free(char* bmap, int size, unsigned* free)
{
   for (int i = 0; i < size; i++) {
      if (!BMAP_ISSET(bmap,i)) {
         *free = i;
         return 1;
      }
   }
   return 0;
}


static void fsi_dump_bmap(char* bmap, int size)
{
   int i = 0;
   for (; i < size; i++) {
      printf("%x.", (unsigned char)bmap[i]);
      if (i > 0 && i % 32 == 0) {
         printf("\n");
      }
   }
}


/*
 * Other internal file system macros and functions
 */

#define MIN(a,b) ((a)<=(b)?(a):(b))
                                
#define MAX(a,b) ((a)>=(b)?(a):(b))
                                
#define OFFSET_TO_BLOCKS(pos) ((pos)/BLOCK_SIZE+(((pos)%BLOCK_SIZE>0)?1:0))	//calcular o numero de entradas ocupadas do vector de blocos do inode

/*Inicializar um inode*/                         
static void fsi_inode_init(fs_inode_t* inode, fs_itype_t type)
{
   int i;
   
   inode->type = type;
   inode->size = 0;
   for (i = 0; i < INODE_NUM_BLKS; i++) { //colocar os ponteiros para blocos a 0
      inode->blocks[i] = 0;
   }
   
   for (i = 0; i < 4; i++) {//colocar os ponteiros para blocos de ponteiros a 0
	   inode->reserved[i] = 0;
   }
}

/*  Procurar o directorio 
 * Procurar em todas as entradas do directorio o dentry com o nome do ficheiro e devolve o id do inode do ficheiro
 * 	Recebe:
 * 	- File system
 *  - inode do file que buscamos
 *  - nome do file
 *  - devolve o id do inode do ficheiro no fileid
 *   */
static int fsi_dir_search(fs_t* fs, inodeid_t dir, char* file,inodeid_t* fileid)
{	
   fs_dentry_t page[DIR_PAGE_ENTRIES];	//Criar um bloco constituido por entradas de directorio
   fs_inode_t* idir = &fs->inode_tab[dir];	//Carregar o inode do directorio
   int num = idir->size / sizeof(fs_dentry_t);	//numero de dentrys do inode
   int iblock = 0;
	
   while (num > 0) {
	   if(idir->type == FS_FILE)
			lerCache(fs->cache,idir->blocks[iblock++],(char*)page);
		else
			block_read(fs->blocks,idir->blocks[iblock++],(char*)page);
      for (int i = 0; i < DIR_PAGE_ENTRIES && num > 0; i++, num--) {
         if (strcmp(page[i].name,file) == 0) {	//verificar se a entrada de directorio tem o nome do ficheiro
            *fileid = page[i].inodeid;//colocar o id do node do ficheiro na resposta
            return 0;
         }
      }
   }
   return -1;
}


/**Algoritmos para garantir sincronizacao*/
/*Inicia a leitura se nao houver escritores activos ou que pretendam escrever
 * e se estivermos no turno de leitura*/
void iniciaLeitura(fs_t* fs){
	fs->wanna_leitor++;	//desejo ser leitor
	sthread_monitor_enter(fs->monLeitores);
	while(fs->escritores != 0 || (fs->turno != LEITURA && fs->wanna_escritor > 0))
			sthread_monitor_wait(fs->monLeitores);
	fs->wanna_leitor--; 
	fs->leitores++;
	if(fs->wanna_leitor != 0)
			sthread_monitor_signalall(fs->monLeitores);
	else
		fs->turno = ESCRITA;
}

/*Terminou leitura. Se eramos o ultimo leitor, acordar um escritor.
 * (apenas pode estar um escritor activo pelo que não e necessário desbloquear todos*/
void terminaLeitura(fs_t* fs){
	fs->leitores--;
	if(fs->leitores == 0)	//ja todos leram
		sthread_monitor_signal(fs->monEscritores);
	sthread_monitor_exit(fs->monLeitores);
}

/*Inicia escrita quando nao houver escritores nem leitores*/
void iniciaEscrita(fs_t* fs){
	fs->wanna_escritor++;
	sthread_monitor_enter(fs->monEscritores);
	while(fs->escritores != 0 || fs->leitores != 0)
		sthread_monitor_wait(fs->monEscritores);
	fs->wanna_escritor--;
	fs->escritores++;
}

void terminaEscrita(fs_t* fs){
	fs->escritores--;
	fs->turno = LEITURA;
	if(fs->wanna_leitor != 0)
		sthread_monitor_signalall(fs->monLeitores);
	else
		sthread_monitor_signal(fs->monEscritores);
	sthread_monitor_exit(fs->monEscritores);
}

/*Algoritmo em arvore. Percorrer o FS em todos os ramos. Quando e aberto um directorio, 
 * concatenamos o nome a uma string que lhe e passada, contendo o path anterior. Ao chegar ao ficheiro,
 * colocamos o nome no path. Depois criamos uma estrutura com:
 * 		-blockId
 * 		-inodeId
 * 		-pathname obtido
 * Esta estrutura e inserida ordenadamente (atraves do blockId) numa lista (insertion sort) criando deste modo
 * uma lista que indica cada uma das referencias de cada bloco de modo ordenado.*/

/*Recebe o sistema de ficheiros em questao, o numero de inode, o path ja acumulado e a lista de referencias
 * 
 * 1º Abrir o inode
2º se o inode for um ficheiro, retorna 1
3º se for um directorio:
	- criar uma page temporaria para as dentrys
	- para cada dentry:
		criar uma copia do path que temos 
		concatenar o nome da dentry
		invocar a funcao com a copia do path
	


 * 		retorna 1 quando chega a um ficheiro*/
int gerar_lista_referencias_aux(fs_t*fs,inodeid_t inode,char** path,List* lista){
	fs_inode_t* idir = &fs->inode_tab[inode];	//abrir o inode
	
	if(idir->type == FS_FILE){	//se ficheiro
		int num = OFFSET_TO_BLOCKS(idir->size);
		for(int i = 0;i<num;i++){
			noRef_t novoNo = (noRef_t) malloc(sizeof(noRef_));
			novoNo->blockId = idir->blocks[i];		//adicionar uma entrada por cada bloco do inode
			novoNo->pathname = *path;
			inserirOrdenado(*lista,(void*)novoNo,idir->blocks[i]);
		}
		return 1;
	}
	
	
	//se for directorio
	fs_dentry_t page[DIR_PAGE_ENTRIES];	//Criar buffer para as entradas de directorio
	int num = idir->size / sizeof(fs_dentry_t);	//numero de dentrys do directorio
	int iblock = 0;
	int i;
	 while (num > 0) {	//percorrer todas as dentrys
		if(idir->type == FS_FILE)
			lerCache(fs->cache,idir->blocks[iblock],(char*)page);
		else
			block_read(fs->blocks,idir->blocks[iblock],(char*)page);
		
		noRef_t novoNoDir = (noRef_t) malloc(sizeof(noRef_));		//considerar a referencia do directorio aos seus blocos
		char *newPathDIR = (char*) malloc(sizeof(char)*MAX_PATH_NAME_SIZE+1);
		
		if(inode == 1)
			strcpy(newPathDIR,"/");
		else
			strcpy(newPathDIR,*path);	//criar uma copia do nosso path
		
		novoNoDir->blockId = idir->blocks[iblock];
		novoNoDir->inodeId = inode;
		novoNoDir->pathname = newPathDIR;
		inserirOrdenado(*lista,(void*)novoNoDir,idir->blocks[iblock]);
		
		for (i = 0; i < DIR_PAGE_ENTRIES && num > 0; i++, num--) {
			char *newPath = (char*) malloc(sizeof(char)*MAX_PATH_NAME_SIZE+1);
			strcpy(newPath,*path);	//criar uma copia do nosso path
			strcat(newPath,"/");		//adicionar o nome do ficheiro/directorio que vamos seguir
			strcat(newPath,page[i].name);
			gerar_lista_referencias_aux(fs,page[i].inodeid,&newPath,lista);		
			}	
		iblock++;
		}
	return 0;	//sou um directorio	
}


List gerar_lista_referencias(fs_t* fs){
	
	List refList = newList();
	
	char *path = (char*) malloc(sizeof(char)*MAX_PATH_NAME_SIZE+1);
	strcpy(path,"");
	
	
	gerar_lista_referencias_aux(fs,1,&path,&refList);
	
	return refList;
}




/*Remover um bloco de sistema.
 * Verificar se o bloco tem mais do que 1 referencia, 
 *SIM, vamos simplesmente retirar a nossa referencia a esse bloco 
 * e decrementar o numero de referencias. O bloco continua valido para as restantes
 * 
 * Nao, vamos elimina-lo utilizando o mecanismo disponibilizado pela cache
 * 
 * return 0 - bloco nao referenciado
 * return 1 - eramos a unica referencia
 * return 2 - havia mais referencias*/
int apagarBloco(fs_t* fs,unsigned idBloco,inodeid_t inode){
	dprintf("[apagarBloco] START bloco %d\n",idBloco);
	
	int nRef = fs->referencias[idBloco];
	
	if(nRef > 1){
		dprintf("[apagarBloco] retirar referencia ao bloco %d\n",idBloco);
		fs->referencias[idBloco]--;
		return 2;
	}
	
	dprintf("[apagarBloco] apagar bloco so nosso");
	fs_inode_t* iNode = &fs->inode_tab[inode];	
	fs->referencias[idBloco] = 0;
	if(iNode != NULL && iNode->type == FS_DIR){	//se for directorio, operar sobre o disco
		char null_block[BLOCK_SIZE];
		memset(null_block,0,sizeof(null_block)); //criar um bloco vazio/zerado
		block_write(fs->blocks,idBloco,null_block);
	}
	else
		eliminarBlocoCache(fs->cache,idBloco);
		
	BMAP_CLR(fs->blk_bmap,idBloco);	//considerar bloco livre
	return 0;
}


/*Mecanismo de Copy-on-Write
 * 1º Verificar quantas referencias tem o bloco
* 2º se tiver 1, sai porque somos a unica
* 3º se tiver mais do que 1, vamos ler o bloco para um buffer (de onde será copiado)
* 4º obter a lista de referencias do bloco
* 5º para cada referencia vamos:
	-criar um bloco novo
	-alterar a referencia do inode para o novo bloco
	-diminuir uma referencia do bloco antigo

 * return -2 se nao possui referencias, -1 se somos a unica referencia
 * se fez cria um novo devolve o id do novobloco*/
int copy_on_write(fs_t* fs,unsigned blockId,inodeid_t idFile){
	int i;
	int nRef = fs->referencias[blockId];	//determinar o numero de referencias que o bloco tem
	
	if(nRef == 0){
		dprintf("[copy_on_write] o bloco %d nao possui referencias\n",blockId);
		return -2;
	}
	
	dprintf("[copy_on_write] o bloco %d possui %d referencia\n",blockId,nRef);
	
	if(nRef == 1){
		return -1;
	}
	
	//alterar no inode a referencia para o novo bloco
	fs_inode_t* iNode = &fs->inode_tab[idFile];	//carregar o inode que referencia

	//Vamos criar uma copia do bloco e alterar a referencia para este conteudo
	unsigned bks;	//indice do novo bloco
	int k;	//indice dentro do vector do inode onde se encontrava o bloco referenciado
	
	char block[BLOCK_SIZE];
	
	if(iNode->type == FS_FILE)
		lerCache(fs->cache,blockId,block);
	else
		block_read(fs->blocks,blockId,block);	
			
	//reservar novo bloco
	if (!fsi_bmap_find_free(fs->blk_bmap,block_num_blocks(fs->blocks),&bks)) { // ITAB_SIZE	Procurar blocos livres
			dprintf("[copy_on_write] there are no free blocks.\n");
			return -1;
	}
		
	BMAP_SET(fs->blk_bmap, bks);//Colocar o bloco a set
		
	for(i = 0,k = -1; i< INODE_NUM_BLKS; i++){	//determinar qual a entrada do inode que tem a ref deste bloco
		if((iNode->blocks[i]) == blockId){
				k = i;
				break;
			}
		}
		
	if(k == -1){
		dprintf("[copy_on_write] nao encontro a referencia neste inode\n");
		return -1;
	}
	//alterar a referencia do bloco antigo para o novo
	iNode->blocks[k] = bks;
	//incrementar as referencias do novo
	fs->referencias[bks]++;
		
	fs->referencias[blockId]--; //retiramos uma referencia ao bloco
	
	if(iNode->type == FS_FILE)		//realizar a copia
		escreverCache(fs->cache,bks,block);
	else
		block_write(fs->blocks,bks,block);	
	
	return bks;
}
		

int escreverBloco(fs_t* fs,unsigned idBloco,inodeid_t idFile,char* dados){
	int novoBloco = copy_on_write(fs,idBloco,idFile);		//garantir que ao escrevermos no bloco, somos os unicos a referencia-lo
	
	fs_inode_t* iNode = &fs->inode_tab[idFile];	//carregar o inode que referencia

	if(novoBloco >= 0){
		if(iNode->type == FS_DIR)
			block_write(fs->blocks,novoBloco,dados);
		else
			escreverCache(fs->cache,novoBloco,dados);	//se criamos um novo bloco, vamos escrever nele
	}
	else{
		if(iNode->type == FS_DIR)
			block_write(fs->blocks,idBloco,dados);
		else
			escreverCache(fs->cache,idBloco,dados);	//se criamos um novo bloco, vamos escrever nele
	}
	return 0;
}


/*
 * File system interface functions
 */
void io_delay_on(int disk_delay);

/*Criar um novo file system*/
fs_t* fs_new(unsigned num_blocks, int disk_delay)
{
	int i;
	
   fs_t* fs = (fs_t*) malloc(sizeof(fs_t));
   fs->blocks = block_new(num_blocks,BLOCK_SIZE);	//criar uma estrutura de dados permanente
   fs->cache = criarCache(DIM_CACHE,fs->blocks);	//criar uma cache de blocos
   fs->referencias = (char*) malloc((sizeof(char)*num_blocks));	//estrutura para registar quantas referencias tem cada bloco
   fs->monLeitores = sthread_monitor_init();
   fs->monEscritores = sthread_monitor_init();
   fs->leitores = 0;
   fs->escritores = 0;
   fs->wanna_escritor = 0;
   fs->wanna_leitor = 0;
   
   for(i = 0; i<num_blocks; i++){
	   fs->referencias[i] = 0;	//inicialmente todos tem referencias a 0
   }
   
   fsi_load_fsdata(fs);		//actulizar o file system
   io_delay_on(disk_delay);
	   if((sthread_create(thread_actualiza_Cache,(void*)fs,1)) == NULL){
	   printf("[new File System] Erro ao criar a thread de actualizacao da cache");
		exit(-1);
	}  
   return fs;
}

/*Formatar o sistema de ficheiros*/
int fs_format(fs_t* fs)
{
	iniciaEscrita(fs);	//sincronizar	
	
	int i;
   if (fs == NULL) {
      printf("[fs_format] argument is null.\n");
      terminaEscrita(fs);
      return -1;
   }

   // erase all blocks
   char null_block[BLOCK_SIZE];
   memset(null_block,0,sizeof(null_block)); //criar um bloco vazio/zerado
   for (int i = 0; i < block_num_blocks(fs->blocks); i++) {
		block_write(fs->blocks,i,null_block);
   }
	
	//destruir todas as referencias que existiam
	for(i = 0; i<block_num_blocks(fs->blocks); i++){
	   fs->referencias[i] = 0;	//inicialmente todos tem referencias a 0
	}
	
	//Apagar todo o bitmap de blocos
	for(i = 0; i<BLOCK_SIZE*sizeof(char)*8;i++){
		BMAP_CLR(fs->blk_bmap,i);
	}
	
		//Apagar todo o bitmap de inodes
	for(i = 0; i<BLOCK_SIZE*sizeof(char)*8;i++){	//num bits =nºcaracteres*dimensao em byte dos caracteres*nºbits de um byte
		BMAP_CLR(fs->inode_bmap,i);
	}
	
	
   // reserve file system meta data blocks
   BMAP_SET(fs->blk_bmap,0);
   BMAP_SET(fs->blk_bmap,1);
   for (int i = 0; i < ITAB_NUM_BLKS; i++) {
      BMAP_SET(fs->blk_bmap,i+2);
   }

   // reserve inodes 0 (will never be used) and 1 (the root)
   BMAP_SET(fs->inode_bmap,0);
   BMAP_SET(fs->inode_bmap,1);
   fsi_inode_init(&fs->inode_tab[1],FS_DIR);

   // save the file system metadata
   fsi_store_fsdata(fs);
   
   terminaEscrita(fs);
   return 0;
}


int fs_get_attrs(fs_t* fs, inodeid_t file, fs_file_attrs_t* attrs)
{
   if (fs == NULL || file >= ITAB_SIZE || attrs == NULL) {
      dprintf("[fs_get_attrs] malformed arguments.\n");
      return -1;
   }

   if (!BMAP_ISSET(fs->inode_bmap,file)) {
      dprintf("[fs_get_attrs] inode is not being used.\n");
      return -1;
   }

   fs_inode_t* inode = &fs->inode_tab[file]; //Obter o inode correspondente ao ficheiro
   attrs->inodeid = file;
   attrs->type = inode->type;
   attrs->size = inode->size;
   switch (inode->type) {
      case FS_DIR:
         attrs->num_entries = inode->size / sizeof(fs_dentry_t);
         break;
      case FS_FILE:
         attrs->num_entries = -1;
         break;
      default:
         dprintf("[fs_get_attrs] fatal error - invalid inode.\n");
         exit(-1);
   }
   return 0;
}

/*Obtem o identificador "fhandle" e respectivo tamanho "fsize" do ficheiro
 * ou directorio "name" fhandle onde esta o ficheiro
 * Se a resposta obtida for STATOK, o file esta no directorio dir.
 * Se "fhandle" for 0, referece ao directorio de raiz
 * 
 * -fs ->sistema de ficheiros a referir
 * -file ->nome do objecto(ficheiro ou directorio)
 * - fileid -> o inode que é devolvido
 * - returns: 0 if successful,  -1 otherwise
 * */
int fs_lookup(fs_t* fs, char* file, inodeid_t* fileid)
{
	dprintf("[fs_lookup]Lookup com file: %s\n",file);
	
char *token;
char line[MAX_PATH_NAME_SIZE]; 
char *search = "/";
int i=0;
int dir=0;
   if (fs==NULL || file==NULL ) { //Se o nome do ficheiro ou do file system for nulo, erro
      dprintf("[fs_lookup] malformed arguments.\n");
      return -1;
   }


    if(file[0] != '/') {	//O nome do directorio tem de comecar com /
        dprintf("[fs_lookup] malformed pathname.\n");
        return -1;
    }

    strcpy(line,file);	//Guardar o nome do ficheiro
    token = strtok(line, search);//separar os nomes de cada parte do directorio /parte1/parte2...
    
    iniciaLeitura(fs);
   while(token != NULL) {
     i++;
     if(i==1) dir=1;   //Root directory, se so tem o /
     
     if (!BMAP_ISSET(fs->inode_bmap,dir)) {	//se o inode do directorio nao esta usado, e porque o inode nao existe
	      dprintf("[fs_lookup] inode is not being used.\n");
	      terminaLeitura(fs);
	      return -1;
     }
     fs_inode_t* idir = &fs->inode_tab[dir];//obter o i-node do diretorio
     if (idir->type != FS_DIR) { //se esse inode nao for do tipo FS_DIR, diz que nao e um directorio e da erro
        dprintf("[fs_lookup] inode is not a directory.\n");
        terminaLeitura(fs);
        return -1;
     }
     inodeid_t fid;
     if (fsi_dir_search(fs,dir,token,&fid) < 0) { //vamos procurar o ficheiro dentro da pasta
        dprintf("[fs_lookup] file does not exist.\n");
        terminaLeitura(fs);
        return 0;
     }
     *fileid = fid; //obteve o ficheiro
     dir=fid;
     token = strtok(NULL, search); //vai avancar na procura
   }
	terminaLeitura(fs);
   return 1;
}


/*Devolver até "count" bytes de "data" do ficheiro "file" apartir do byte "offset" a contar do inicio do ficheiro.
 * O primeiro byte do ficheiro corresponde ao offset 0
 * 
 *  * - fs: reference to file system
 * - file: node id of the file
 * - offset: starting position for reading
 * - count: number of bytes to read
 * - buffer: where to put the data [out]
 * - nread: number of bytes effectively read [out]
 *   returns: 0 if successful, -1 otherwise
 */
int fs_read(fs_t* fs, inodeid_t file, unsigned offset, unsigned count, 
   char* buffer, int* nread)
{
	if (fs==NULL || file >= ITAB_SIZE || buffer==NULL || nread==NULL) {
		dprintf("[fs_read] malformed arguments.\n");		
		return -1;
	}
	
	iniciaLeitura(fs);
	
	if (!BMAP_ISSET(fs->inode_bmap,file)) {
		dprintf("[fs_read] inode is not being used.\n");
		terminaLeitura(fs);
		return -1;
	}

	fs_inode_t* ifile = &fs->inode_tab[file]; //abrir o inode	
	if (ifile->type != FS_FILE) {
		dprintf("[fs_read] inode is not a file.\n"); //verificar se nao e um directorio
		terminaLeitura(fs);
		return -1;
	}

	if (offset >= ifile->size) { //se excedemos o tamanho, acabamos a leitura, retorna 0
		*nread = 0;
		terminaLeitura(fs);
		return 0;
	}
	
   	// read the specified range
	int pos = 0;
	int iblock = offset/BLOCK_SIZE; //determinar qual o numero do bloco em que vamos comecar
	int blks_used = OFFSET_TO_BLOCKS(ifile->size); //tamanho ocupado no bloco
	int max = MIN(count,ifile->size-offset);
	int tbl_pos;
	unsigned int *blk;
	char block[BLOCK_SIZE];
   
	while (pos < max && iblock < blks_used) { //enquanto nao chegar ao maximo do bloco e o bloco nao for maior que os blocos usados
		if(iblock < INODE_NUM_BLKS) { //verificar se o bloco ainda esta dentro da tabela de inodes
			blk = ifile->blocks;
			tbl_pos = iblock;
		}
		
		
		if(ifile->type == FS_FILE)
			lerCache(fs->cache, blk[tbl_pos], block); //ler o bloco
		else
			block_read(fs->blocks, blk[tbl_pos], block); //ler o bloco
			
		int start = ((pos == 0)?(offset % BLOCK_SIZE):0);
		int num = MIN(BLOCK_SIZE - start, max - pos);
		memcpy(&buffer[pos],&block[start],num);

		pos += num;
		iblock++;
	}
	*nread = pos;
	terminaLeitura(fs);
	return 0;
}

/*Escrever "data" apartir do byte "offset" a partir do inicio do ficheiro "file".
 * O primeiro byte do ficheiro está no offset 0. A operacao de esctrita é atomica. Os dados de escrita
 * nao serao misturados com os dados da escrita de outro cliente. Se bem sucedida devolve a dimensao
 * actual do ficheiro em "fsize".
 * Count e o numero de bytes de data a escrever*/
int fs_write(fs_t* fs, inodeid_t file, unsigned offset, unsigned count,
   char* buffer)
{
	dprintf("[my_write] START\n");
	if (fs == NULL || file >= ITAB_SIZE || buffer == NULL) {
		dprintf("[fs_write] malformed arguments.\n");
		return -1;
	}
	
	iniciaEscrita(fs);
	
	if (!BMAP_ISSET(fs->inode_bmap,file)) {
		dprintf("[fs_write] inode is not being used.\n");
		terminaEscrita(fs);
		return -1;
	}

	fs_inode_t* ifile = &fs->inode_tab[file]; //abrir o inode	
	if (ifile->type != FS_FILE) {
		dprintf("[fs_write] inode is not a file.\n"); //verificar se nao e um directorio
		terminaEscrita(fs);
		return -1;
	}

	if (offset > ifile->size) { //Se o offset excede o size do ficheiro, coloca-o no final para que nao leia lixo
		offset = ifile->size;
	}

	unsigned *blk;

	int blks_used = OFFSET_TO_BLOCKS(ifile->size); //Calcula o numero de blocos utilizados
	int blks_req = MAX(OFFSET_TO_BLOCKS(offset+count),blks_used)-blks_used; //tamanho ocupado no bloco

	dprintf("[fs_write] count=%d, offset=%d, fsize=%d, bused=%d, breq=%d\n",
		count,offset,ifile->size,blks_used,blks_req);
	
	if (blks_req > 0) {
		if(blks_req > INODE_NUM_BLKS-blks_used) { //Se sao necessarios blocos mas nao os ha neste inode
			dprintf("[fs_write] no free block entries in inode.\n");
			terminaEscrita(fs);
			return -1;
		}

		dprintf("[fs_write] required %d blocks, used %d\n", blks_req, blks_used); //requerir blocos

      		// check and reserve if there are free blocks
		for (int i = blks_used; i < blks_used + blks_req; i++) {

			if(i < INODE_NUM_BLKS) //se ainda der para reservar um bloco dentro da estruta do inode
				blk = &ifile->blocks[i];
	 
			if (!fsi_bmap_find_free(fs->blk_bmap,block_num_blocks(fs->blocks),blk)) { // ITAB_SIZE	Procurar blocos livres
				dprintf("[fs_write] there are no free blocks.\n");
				terminaEscrita(fs);
				return -1;
			}
			BMAP_SET(fs->blk_bmap, *blk);//Colocar o bloco a set
			fs->referencias[*blk]++;	//adicionar uma referencia ao bloco			
			dprintf("[fs_write] block %d allocated.\n", *blk);
		}
	}
   
	char block[BLOCK_SIZE]; //criar um buffer do tamanho de todo o bloco
	int num = 0, pos;
	int iblock = offset/BLOCK_SIZE;	//bloco em que comecamos

   	// write within the existent blocks
	while (num < count && iblock < blks_used) { //enquanto nao escrevermos tudo e nao excedermos os blocos usados
		if(iblock < INODE_NUM_BLKS) { //se o bloco ainda esta dentro do numero max de blocos de cada inode
			blk = ifile->blocks; //abrir a tabela de blocos do inode do ficheiro
			pos = iblock; 	//abrir a posicao correcta no bloco
		}
		
		if(ifile->type == FS_FILE)
			lerCache(fs->cache,blk[pos], block);
		else
			block_read(fs->blocks, blk[pos], block);

		int start = ((num == 0)?(offset % BLOCK_SIZE):0);
		
		for (int i = start; i < BLOCK_SIZE && num < count; i++, num++) {
			block[i] = buffer[num];
		}
		
		if(ifile->type == FS_FILE)
			escreverBloco(fs, blk[pos],file,block);
		else 
			block_write(fs->blocks, blk[pos], block);
		iblock++;
	}

	dprintf("[fs_write] written %d bytes within.\n", num);

  	// write within the allocated blocks
	while (num < count && iblock < blks_used + blks_req) {
		if(iblock < INODE_NUM_BLKS) {
			blk = ifile->blocks;
			pos = iblock;
		}
      
		for (int i = 0; i < BLOCK_SIZE && num < count; i++, num++) {
			block[i] = buffer[num];
		}
		if(ifile->type == FS_FILE)
			escreverBloco(fs, blk[pos],file,block);
		else
			block_write(fs->blocks, blk[pos], block);
		iblock++;
	}

	if (num != count) {
		printf("[fs_write] severe error: num=%d != count=%d!\n", num, count);
		terminaEscrita(fs);
		exit(-1);
	}

	ifile->size = MAX(offset + count, ifile->size);

   	// update the inode in disk
	fsi_store_fsdata(fs);

	dprintf("[fs_write] written %d bytes, file size %d.\n", count, ifile->size);	
	terminaEscrita(fs);
	return 0;
}

/*Criar um ficheiro num directorio especifico
 * 
 * Os atributos iniciais do ficheiro sao dados por "atributes". Se o resultado
 * é STAT_OK, o ficheiro foi criado com uscesso e "file" e "attributes" contém o "fhandle" e
 * os atributos do ficheiro. Caso contrario a operacao falhou e nenhum ficheiro é criado * 
 * - fs: reference to file system
 * - dir: the directory where to create the file, inode do directorio
 * - file: the name of the file
 * - fileid: the inode id of the file [out]
 *   returns: 0 if successful, -1 otherwise
 */
int fs_create(fs_t* fs, inodeid_t dir, char* file, inodeid_t* fileid)
{
	dprintf("[fs_create]Criar o ficheiro %s, no directorio %d\n",file,dir);
	
   if (fs == NULL || dir >= ITAB_SIZE || file == NULL || fileid == NULL) {
      printf("[fs_create] malformed arguments.\n");
      return -1;
   }

   if (strlen(file) == 0 || strlen(file)+1 > FS_MAX_FNAME_SZ){
      dprintf("[fs_create] file name size error.\n");
      return -1;
   }
   
   iniciaEscrita(fs);

   if (!BMAP_ISSET(fs->inode_bmap,dir)) { //verificar se o inode esta ocupado
      dprintf("[fs_create] inode is not being used.\n");
      terminaEscrita(fs);
      return -1;
   }

   fs_inode_t* idir = &fs->inode_tab[dir];	//abrir o inode do directorio
   if (idir->type != FS_DIR) {
      dprintf("[fs_create] inode is not a directory.\n");
      terminaEscrita(fs);
      return -1;
   }

   if (fsi_dir_search(fs,dir,file,fileid) == 0) { //ver se o ficheiro ja existe dentro do directorio
      dprintf("[fs_create] file already exists.\n");
      terminaEscrita(fs);
      return -1;
   }
   
   // check if there are free inodes
   unsigned finode;
   if (!fsi_bmap_find_free(fs->inode_bmap,ITAB_SIZE,&finode)) { //procurar um inodes livre
      dprintf("[fs_create] there are no free inodes.\n");
      terminaEscrita(fs);
      return -1;
   }

	dprintf("[fs_create]inode do ficheiro: %d\n",finode);

   // add a new block to the directory if necessary
   if (idir->size % BLOCK_SIZE == 0) { //se o bloco do directorio esta cheio ou vazio:
      unsigned fblock;
      if (!fsi_bmap_find_free(fs->blk_bmap,block_num_blocks(fs->blocks),&fblock)) { 	//procurar um bloco livre
         dprintf("[fs_create] no free blocks to augment directory.\n");
         terminaEscrita(fs);
         return -1;
      }
     ///fs->referencias[fblock]++; apenas necessario no caso da copia de directorios
      dprintf("[fs_create]adicionei o bloco %d ao directorio %d\n",fblock,dir);
      BMAP_SET(fs->blk_bmap,fblock); //indifcar que o nosso bloco vai ser utilizado
      idir->blocks[idir->size / BLOCK_SIZE] = fblock;  //Add nº do novo bloco ao vector de blocos do directorio
   }

   // add the entry to the directory
   fs_dentry_t page[DIR_PAGE_ENTRIES]; //Criar um array de entradas de diretorio
   if(idir->type == FS_FILE)
		lerCache(fs->cache,idir->blocks[idir->size/BLOCK_SIZE],(char*)page); //ler o bloco do directorio actual
   else
		block_read(fs->blocks,idir->blocks[idir->size/BLOCK_SIZE],(char*)page);
	
	dprintf("[fs_create]Vou ler o bloco do %d, o conteudo: %s\n\n",idir->blocks[idir->size/BLOCK_SIZE],(char*)page);
	
	
   fs_dentry_t* entry = &page[idir->size % BLOCK_SIZE / sizeof(fs_dentry_t)]; //seleccionar a entrada livre
   strcpy(entry->name,file);	//dar o nome do ficheiro a essa entrada de directorio
   entry->inodeid = finode;		//colocar a entrada de directorio a apontar para o inode novo
   
   if(idir->type == FS_FILE)
		escreverBloco(fs,idir->blocks[idir->size/BLOCK_SIZE],dir,(char*)page); //escrever o bloco ja com a nova entrada
   else
		block_write(fs->blocks,idir->blocks[idir->size/BLOCK_SIZE],(char*)page);
		
	dprintf("[fs_create]Escrevi o bloco %d, com o conteudo: %s\n\n",idir->blocks[idir->size/BLOCK_SIZE],(char*)page);
   idir->size += sizeof(fs_dentry_t);

   // reserve and init the new file inode
   BMAP_SET(fs->inode_bmap,finode);	//reservar o inode que associamos ao ficheiro
   fsi_inode_init(&fs->inode_tab[finode],FS_FILE); //inicializar esse inode como um inode do FILE SYSTEM

   // save the file system metadata
   fsi_store_fsdata(fs); //actualizar a meta data

   *fileid = finode;
   terminaEscrita(fs);
   return 0;
}

/*Cria o novo directorio "name" no directório "dir". A resposta STAT_OK, indica que o 
 * directorio foi criado com sucesso e "file" contém o fhandle dp directório. 
 * Caso contrário, a operação falhou e o directorio não foi criado.  
 * - fs: reference to file system
 * - dir: the directory where to create the file
 * - newdir: the name of the new subdirectory
 * - newdirid: the inode id of the subdirectory [out]
 *   returns: 0 if successful, -1 otherwise
 */
int fs_mkdir(fs_t* fs, inodeid_t dir, char* newdir, inodeid_t* newdirid)
{
	dprintf("[fs_mkdir] criar um directorio com nome: %s, no directorio: %d\n",newdir,dir);
	
	
	if (fs==NULL || dir>=ITAB_SIZE || newdir==NULL || newdirid==NULL) {
		printf("[fs_mkdir] malformed arguments.\n");
		return -1;
	}
	
	if (strlen(newdir) == 0 || strlen(newdir)+1 > FS_MAX_FNAME_SZ){
		printf("[fs_mkdir] directory size error.\n");
		return -1;
	}
	
	iniciaEscrita(fs);
	if (!BMAP_ISSET(fs->inode_bmap,dir)) {
		printf("[fs_mkdir] inode is not being used.\n");
		return -1;
	}

	fs_inode_t* idir = &fs->inode_tab[dir];
	if (idir->type != FS_DIR) {
		printf("[fs_mkdir] inode is not a directory.\n");
		terminaEscrita(fs);
		return -1;
	}

	if (fsi_dir_search(fs,dir,newdir,newdirid) == 0) {
		printf("[fs_mkdir] directory already exists.\n");
		terminaEscrita(fs);
		return -1;
	}
   
   	// check if there are free inodes
	unsigned finode;
	if (!fsi_bmap_find_free(fs->inode_bmap,ITAB_SIZE,&finode)) {
		printf("[fs_mkdir] there are no free inodes.\n");
		terminaEscrita(fs);
		return -1;
	}

   	// add a new block to the directory if necessary
	if (idir->size % BLOCK_SIZE == 0) {
		unsigned fblock;
		if (!fsi_bmap_find_free(fs->blk_bmap,block_num_blocks(fs->blocks),&fblock)) {
			printf("[fs_mkdir] no free blocks to augment directory.\n");
			terminaEscrita(fs);
			return -1;
		}
		///fs->referencias[fblock]++; apenas necessario no caso da copia de directorios
		BMAP_SET(fs->blk_bmap,fblock);
		idir->blocks[idir->size / BLOCK_SIZE] = fblock;
	}

   	// add the entry to the directory
	fs_dentry_t page[DIR_PAGE_ENTRIES];
	if(idir->type == FS_FILE)
		lerCache(fs->cache,idir->blocks[idir->size/BLOCK_SIZE],(char*)page);
	else
		block_read(fs->blocks,idir->blocks[idir->size/BLOCK_SIZE],(char*)page);
		
	fs_dentry_t* entry = &page[idir->size % BLOCK_SIZE / sizeof(fs_dentry_t)];
	strcpy(entry->name,newdir);
	entry->inodeid = finode;
	
	if(idir->type == FS_FILE)
		escreverBloco(fs,idir->blocks[idir->size/BLOCK_SIZE],dir,(char*)page);
	else	
		block_write(fs->blocks,idir->blocks[idir->size/BLOCK_SIZE],(char*)page);
		
	idir->size += sizeof(fs_dentry_t);

   	// reserve and init the new file inode
	BMAP_SET(fs->inode_bmap,finode);
	fsi_inode_init(&fs->inode_tab[finode],FS_DIR);

   	// save the file system metadata
	fsi_store_fsdata(fs);

	*newdirid = finode;
	terminaEscrita(fs);
	return 0;
}

/*				LER DIRECTORIO
 * Retorna um numero variavel de entradas do directorio ate "count" bytes do directorio
 * dado por "dir". Se o valor retornado e STAT_OK, entao segue-se de um numero variavel de
 * "entrys". Cada entry é uma estrutura com nome da entrada (ficheiro ou directorio),
 * e o inode relativo a essa entrada*/
int fs_readdir(fs_t* fs, inodeid_t dir, fs_file_name_t* entries, int maxentries,
   int* numentries)
{
   if (fs == NULL || dir >= ITAB_SIZE || entries == NULL ||
      numentries == NULL || maxentries < 0) {
      printf("[fs_readdir] malformed arguments.\n");
      return -1;
   }

	iniciaLeitura(fs);
	
   if (!BMAP_ISSET(fs->inode_bmap,dir)) {
      printf("[fs_readdir] inode is not being used.\n");
      terminaLeitura(fs);
      return -1;
   }

   fs_inode_t* idir = &fs->inode_tab[dir];
   if (idir->type != FS_DIR) {
      printf("[fs_readdir] inode is not a directory.\n");
       terminaLeitura(fs);
      return -1;
   }

   // fill in the entries with the directory content
   fs_dentry_t page[DIR_PAGE_ENTRIES];
   int num = MIN(idir->size / sizeof(fs_dentry_t), maxentries);
   int iblock = 0, ientry = 0;

   while (num > 0) {
	   if(idir->type == FS_FILE)
			lerCache(fs->cache,idir->blocks[iblock++],(char*)page);
	   else
			block_read(fs->blocks,idir->blocks[iblock++],(char*)page);
		
      for (int i = 0; i < DIR_PAGE_ENTRIES && num > 0; i++, num--) {
         strcpy(entries[ientry].name, page[i].name);
         entries[ientry].type = fs->inode_tab[page[i].inodeid].type;
         ientry++;
      }
   }
   *numentries = ientry;
    terminaLeitura(fs);
   return 0;
}
/*NOSSA IMPLEMENTACAO

//SNFS_REMOVE
* -fhandle dir
* -fname name
* 
* Remove o ficheiro "name" do directorio "dir". Se o resultado for STAT_OK,
* o ficheiro foi removido com sucesso e "file" contem o respectivo "fhandle". Caso contrario nao teve sucesso
int fs_remove


DESCRICAO DO ALGORITMO.
Verificara se mais nenhum ficheiro esta a aceder a este inode.
Se estiver sai com erro...

Se nao, vai formatar os blocos de todos os inodes
Depois libertar o inode 

Por fim apagar o inode de dentro do directorio
*/
/*fileSystem,id do inode do directorio,nome do ficheiro,fileHandle a devolver*/
int fs_remove(fs_t* fs, inodeid_t directorio, char* nomeFicheiro, inodeid_t* fileHandler){
	
	dprintf("[fs_remove] START: remover ficheiro %s, do directorio %d\n",nomeFicheiro,directorio);
	int i;
	
	if (fs==NULL || directorio>=ITAB_SIZE || nomeFicheiro==NULL || fileHandler==NULL) {
		printf("[fs_remove] malformed arguments.\n");
		return -1;
	}

	//verificar o nome do ficheiro
	if (strlen(nomeFicheiro) == 0 || strlen(nomeFicheiro)+1 > FS_MAX_FNAME_SZ){
		printf("[fs_remove] directory size error.\n");
		return -1;
	}
	
	iniciaEscrita(fs);
	//verificar se o directorio existe
	if (!BMAP_ISSET(fs->inode_bmap,directorio)) {
		printf("[fs_remove] inode is not being used.\n");
		terminaEscrita(fs);
		return -1;
	}


	fs_inode_t* idirectorio = &fs->inode_tab[directorio];		//Aceder ao inode do directorio
	
	if (idirectorio->type != FS_DIR) {
		printf("[fs_remove] inode is not a directory.\n");
		terminaEscrita(fs);
		return -1;
	}
	
	inodeid_t idFile;
	
	//procurar o id do inode do ficheiro		(newFileHandle e o id do ficheiro*/
	if (fsi_dir_search(fs,directorio,nomeFicheiro,&idFile) != 0) {		
		printf("[fs_remove] file doesn't exist.\n");
		terminaEscrita(fs);
		return -1;
	}	
	
	dprintf("[fs_remove]directorio na inode_tab: %d\n",directorio);
	dprintf("[fs_remove]ficheiro na inode_tab: %d\n",idFile);
   
   
   //Abrir o inode do ficheiro
   fs_inode_t* ifile = &fs->inode_tab[idFile];
   if(ifile->type == FS_DIR){
	   dprintf("[fs_remove] Vou remover um DIRECTORIO");
	   if(ifile->size != 0){
			printf("[fs_remove] Remoção de um directorio nao vazio, por favor remova o conteudo primeiro\n");
			terminaEscrita(fs);
			return -1;
		}
   }
   
   
    //Vamos formatar os blocos que o inode estava a ocupar e declara-los como livres
	int blocosOcupados = OFFSET_TO_BLOCKS(ifile->size);	//numero de blocos ocupados
    
    int idBlock;
    
	for(i = 0; i < blocosOcupados; i++){
		idBlock = ifile->blocks[i];	//obter o numero de bloco
		if(idBlock < 10){
			printf("[fs_remove] Nao pode remover um bloco do sistema de ficheiros");
			terminaEscrita(fs);
			return -1;
		}
			apagarBloco(fs,idBlock,idFile);	//solicitar remocao do bloco
			ifile->blocks[i] = 0;	//destruir o ponteiro do inode
   }
   
   
	//Apagar da entrada de directorio: (substituindo-a pela ultima entrada do directorio)
	//calcular a localizacao da entrada
	int idEntradaDirectorio = -1;
	fs_dentry_t ficheirosContidos[DIR_PAGE_ENTRIES];	//Criar um bloco constituido por entradas de directorio
	int num = idirectorio->size / sizeof(fs_dentry_t);	//numero de dentrys que o inode tem
	int stop = 1, iblock = 0;	//comecar a ler do bloco 0
	
	if(num == 1){	//se somos a unica entrada do directorio, podemos apagar o bloco
		apagarBloco(fs,idirectorio->blocks[iblock],directorio);
		idirectorio->blocks[iblock] = 0;	//destruir o ponteiro do idirectory			
	}
	else{	//procurar a entrada
		while (num > 0 && stop) {
			if(idirectorio->type == FS_FILE)
				lerCache(fs->cache,idirectorio->blocks[iblock],(char*)ficheirosContidos);//ler id's dos do inode de directoria para um vector de entradas de directorio
			else
				block_read(fs->blocks,idirectorio->blocks[iblock],(char*)ficheirosContidos);
				
			for (int i = 0; i < DIR_PAGE_ENTRIES && num > 0; i++, num--) { 
				if (strcmp(ficheirosContidos[i].name,nomeFicheiro) == 0) {	//verificar se a entrada de directorio tem o nome do ficheiro
					dprintf("[fs_remove]O FICHEIRO TEM ID: %d\n",ficheirosContidos[i].inodeid);
					idEntradaDirectorio = i;	//i e a entrada do dentry no bloco
					stop = 0;
					//break;
				}
			}
			iblock++;
		}
		
		if(idEntradaDirectorio == -1){
			printf("[fs_remove] Nao encontrou entrada do ficheiro no directorio");
			terminaEscrita(fs);
			return -1;
		}
   
   //a entrada a eliminar localiza-se no bloco iblock e no indice i
    fs_dentry_t* entrada = &ficheirosContidos[idEntradaDirectorio];

	
	//procurar a ultima entrada, troca-la com esta e eliminar a ultima entrada
	//obter a ultima entrada de ficheiros do directorio
	num = idirectorio->size / sizeof(fs_dentry_t);	//numero de dentrys que o inode tem
	int idUltimaEntrada;
	int ultimoBloco = num/DIR_PAGE_ENTRIES;		//indice do ultimo bloco
	if(ultimoBloco == 0)
		idUltimaEntrada = num-1;
	else
		idUltimaEntrada = (num-1) %(ultimoBloco*DIR_PAGE_ENTRIES);	//indice da ultima entrada
		
	if(!(ultimoBloco == 0 && idUltimaEntrada == 0)){		//se nao estamos a remover a unica entrada do directorio, substituimos pela ultima
		fs_dentry_t entradasUltimoBloco[DIR_PAGE_ENTRIES];
		
		if(idirectorio->type == FS_FILE)
			lerCache(fs->cache,idirectorio->blocks[ultimoBloco],(char*)entradasUltimoBloco);	//ler o ultimo bloco
		else
			block_read(fs->blocks,idirectorio->blocks[ultimoBloco],(char*)entradasUltimoBloco);	//ler o ultimo bloco

		fs_dentry_t* ultimo = &entradasUltimoBloco[idUltimaEntrada];
		strcpy(entrada->name,ultimo->name);
		entrada->inodeid = ultimo->inodeid;
	
		if(idUltimaEntrada == 0){ //eliminamos a ultima entrada de um bloco que nao o primeiro, temos de libertar este bloco
			apagarBloco(fs,idirectorio->blocks[ultimoBloco],directorio);
				idirectorio->blocks[ultimoBloco] = 0;	//destruir o ponteiro do idirectory
				
		}
		else{
			strcpy(ultimo->name,""); //escrever lixo para garantir que os dados sao apagados
			ultimo->inodeid = -1;
				escreverBloco(fs,idirectorio->blocks[ultimoBloco],directorio,(char*)entradasUltimoBloco);	//escrever o bloco com a ultima entrada eliminada
		}
	}
	else{	//unica entrada, podemos apagar o bloco do directorio
		apagarBloco(fs,idirectorio->blocks[iblock],directorio);
	}
	}
	idirectorio->size -= sizeof(fs_dentry_t);	//subtrair o tamanho da entrada que retiramos do bloco
	
	
	fsi_inode_init(ifile,0);
	
	//Inode do directorio: directorio
	//inode do ficheiro: idFile
	
	//Apagar o inode do ficheiro	
	BMAP_CLR(fs->inode_bmap,idFile);	//colocar o inode do ficheiro como livre

	dprintf("[fs_remove]directorio na inode_tab: %d\n",directorio);
	dprintf("[fs_remove]ficheiro na inode_tab: %d\n",idFile);
	
	// save the file system metadata
   fsi_store_fsdata(fs);		//actualizar a meta data
	
	*fileHandler = idFile;
	terminaEscrita(fs);
   return 0;
  }
 
//SNFS_COPY
/** Copia o ficheiro "file1" da directoria "dir1" para um novo ficheiro "file2" na directoria "dir2".
* Se o restltado e STAT_OK, o ficheiro foi cirado com sucesso e "file" contem o "fhandle" do ficheiro.
* Caso contrario, a operacao falhou
* -fhandle dir1
* -fhandle name1
* -fhandle dir2
* -fhandle name2
* - fileHandler do ficheiro criado*/
int fs_copy(fs_t* fs, inodeid_t dirOrigem, char* nomeOrigem,inodeid_t dirDestino,char* nomeDestino, inodeid_t* fileHandler){
	dprintf("[fs_copy] INICIAR COPIA: dirOrigem: %d , nomeOrigem: %s , dirDestino: %d ,nomeDestino: %s\n",dirOrigem,nomeOrigem,dirDestino,nomeDestino);
	
	
	/*Algoritmo: 1º Verificar se os nomes estão certos
	2º Verificar se a origem é um ficheiro ou um directorio
	3º Obter o handler da origem
	4º Verificar se o destino e ficheiro ou directorio.
	5º Obter o handler do destino
	*/
	
	if (fs==NULL || dirOrigem>=ITAB_SIZE || nomeOrigem==NULL || fileHandler==NULL) {
		printf("[fs_copy] malformed source arguments.\n");
		return -1;
	}

	//VERIFICAR A ORIGEM
	//verificar o nome do ficheiro de origem
	if (strlen(nomeOrigem) == 0 || strlen(nomeOrigem)+1 > FS_MAX_FNAME_SZ){
		printf("[fs_copy] source directory size error.\n");
		return -1;
	}

	iniciaEscrita(fs);
	//verificar se o directorio de origem existe
	if (!BMAP_ISSET(fs->inode_bmap,dirOrigem)) {
		printf("[fs_copy] inode is not being used.\n");
		terminaEscrita(fs);
		return -1;
	}
	
	fs_inode_t* iDirOrigem = &fs->inode_tab[dirOrigem];		//Aceder ao inode do directorio
	
	if (iDirOrigem->type != FS_DIR) {
		printf("[fs_copy] inode directory source is not a directory.\n");
		terminaEscrita(fs);
		return -1;
	}
	
	//verificar o nome do ficheiro de origem
	if (strlen(nomeOrigem) == 0 || strlen(nomeOrigem)+1 > FS_MAX_FNAME_SZ){
		printf("[fs_copy] source directory size error.\n");
		terminaEscrita(fs);
		return -1;
	}
	
	inodeid_t idFileOrigem;
	//procurar o id do inode do ficheiro de origem	*/
	if (fsi_dir_search(fs,dirOrigem,nomeOrigem,&idFileOrigem) != 0) {		
		printf("[fs_copy] file source doesn't exist.\n");
		terminaEscrita(fs);
		return -1;
	}	
	
		
   //Abrir o inode do ficheiro de origem
   fs_inode_t* iFileOrigem = &fs->inode_tab[idFileOrigem];
   if(iFileOrigem->type != FS_FILE){
	   printf("[fs_copy] Copia de directorios nao suportada\n");
	   terminaEscrita(fs);
	   return -1;
   }
   
	//VERIFICAR O DESTINO
	//verificar o nome do ficheiro do destino
	if (strlen(nomeDestino) == 0 || strlen(nomeDestino)+1 > FS_MAX_FNAME_SZ){
		printf("[fs_copy] destination directory size error.\n");
		terminaEscrita(fs);
		return -1;
	}
	
	//verificar se o directorio de destino existe
	if (!BMAP_ISSET(fs->inode_bmap,dirDestino)) {
		printf("[fs_copy] inode destination is not being used.\n");
		terminaEscrita(fs);
		return -1;
	}
	
	fs_inode_t* iDirDestino = &fs->inode_tab[dirDestino];		//Aceder ao inode do directorio
	
	if (iDirDestino->type != FS_DIR) {
		printf("[fs_copy] inode directory destination is not a directory.\n");
		terminaEscrita(fs);
		return -1;
	}

	inodeid_t idFileDestino;
	//procurar o id do inode do ficheiro de destino		*/
	if (fsi_dir_search(fs,dirDestino,nomeDestino,&idFileDestino) != 0) {
		terminaEscrita(fs);	//vamos criar um ficheiro novo
		if((fs_create(fs,dirDestino,nomeDestino,&idFileDestino)) == -1){	//se nao existe, criar ficheiro novo		
			printf("[fs_copy] Erro ao criar o ficheiro de destino\n");
			terminaEscrita(fs);
			return -1;
		}
		iniciaEscrita(fs);	//voltar a ter exclusividade
		if(fsi_dir_search(fs,dirDestino,nomeDestino,&idFileDestino) != 0){ //confirmar que o ficheiro foi criado
			printf("[fs_copy] Erro ao criar o ficheiro de destino\n");
			terminaEscrita(fs);
			return -1;
		}
	}	
	
	
	
    fs_inode_t* iFileDestino = &fs->inode_tab[idFileDestino];
	if(iFileDestino->type != FS_FILE){
	   printf("[fs_copy] O destino e um directorio");
	   terminaEscrita(fs);
	   return -1;
   }
     
     //garantir que o destino se existir, e substituido
 	int numBlocoSubs = OFFSET_TO_BLOCKS(iFileDestino->size);
			
	for(int k = 0; k<numBlocoSubs;k++){
		apagarBloco(fs,iFileDestino->blocks[k],idFileDestino);
	}
	
   	dprintf("[fs_copy]Origem: directorio %d, inode ficheiro %d\n",dirOrigem,idFileOrigem);
	dprintf("[fs_copy]Destino: directorio %d, inode ficheiro %d\n",dirDestino,idFileDestino);


	//LISTA DE DADOS DISPONIVEIS:
	/*ORIGEM:
	 * iDirOrigem - inode directorio origem
	 * iFileOrigem - inode ficheiro origem
	 * iDirDestino - inode directorio origem
	 * iFileDestino - inode ficheiro origem
	*/

	//Neste momento temos os inodes do destino e da origem.
	
	//calcular numero de entradas do vector de blocos que estao ocupadas
	int num = OFFSET_TO_BLOCKS(iFileOrigem->size);
	int i;
	
	for(i = 0; i < num;i++){
		iFileDestino->blocks[i] = iFileOrigem->blocks[i];	//copiar a referencia para o bloco
		fs->referencias[iFileDestino->blocks[i]]++;//adicionar referencia do destino ao bloco
	}
	iFileDestino->size = iFileOrigem->size;
	
	terminaEscrita(fs);
	return 0;
	
}	
	

/* NFS_APPEND
* Concatenar o conteudo do ficheiro "nameOrigem" da directoria "dest" com o conteudo do
* ficheiro "name2" da directoria "dir2". Se o resultado é STAT_OK, a contatenacao tve sucesso
* e "fsize" contem o tamanho do ficheiro "name1".
*/ 
int fs_append(fs_t* fs, inodeid_t dirOrigem, char* nomeOrigem,inodeid_t dirDestino,char* nomeDestino, inodeid_t* fileHandler, unsigned* fsize){

	//imprimirInodeTab(fs);
	
	dprintf("[fs_append] INICIAR Apend: dirOrigem: %d , nomeOrigem: %s , dirDestino: %d ,nomeDestino: %s\n",dirOrigem,nomeOrigem,dirDestino,nomeDestino);

	if (fs==NULL || dirOrigem>=ITAB_SIZE || nomeOrigem==NULL || fileHandler==NULL) {
		printf("[fs_append] malformed source arguments.\n");
		return -1;
	}

	//VERIFICAR A ORIGEM
	//verificar o nome do ficheiro de origem
	if (strlen(nomeOrigem) == 0 || strlen(nomeOrigem)+1 > FS_MAX_FNAME_SZ){
		printf("[fs_append] source directory size error.\n");
		return -1;
	}
	
	iniciaEscrita(fs);
	//verificar se o directorio de origem existe
	if (!BMAP_ISSET(fs->inode_bmap,dirOrigem)) {
		printf("[fs_append] directory not exist.\n");
		terminaEscrita(fs);
		return -1;
	}

	fs_inode_t* iDirOrigem = &fs->inode_tab[dirOrigem];		//Aceder ao inode do directorio

	if (iDirOrigem->type != FS_DIR) {
		printf("[fs_append] inode directory source is not a directory.\n");
		terminaEscrita(fs);
		return -1;
	}
	inodeid_t idFileOrigem;
	//procurar o id do inode do ficheiro de origem	*/
	if (fsi_dir_search(fs,dirOrigem,nomeOrigem,&idFileOrigem) != 0) {
		printf("[fs_append] file source doesn't exist.\n");
		terminaEscrita(fs);
		return -1;
	}

	//Abrir o inode do ficheiro de origem
	 fs_inode_t* iFileOrigem = &fs->inode_tab[idFileOrigem];
	 if(iFileOrigem->type != FS_FILE){
		 printf("[fs_append] inode que pretende fazer append nao e um ficheiro");
		 terminaEscrita(fs);
		 return -1;
	 }

	 //VERIFICAR O DESTINO
	 //verificar o nome do segundo ficheiro
	 if (strlen(nomeDestino) == 0 || strlen(nomeDestino)+1 > FS_MAX_FNAME_SZ){
	 	printf("[fs_append] destination directory size error.\n");
	 	terminaEscrita(fs);
	 	return -1;
	 }

	 //verificar se o directorio de segundo ficheiro existe
	 if (!BMAP_ISSET(fs->inode_bmap,dirDestino)) {
	 	printf("[fs_append] inode destination is not being used.\n");
	 	terminaEscrita(fs);
	 	return -1;
	 }

	 fs_inode_t* iDirDestino = &fs->inode_tab[dirDestino];		//Aceder ao inode do directorio

	 if (iDirDestino->type != FS_DIR) {
	 	printf("[fs_append] inode directory destination is not a directory.\n");
	 	terminaEscrita(fs);
	 	return -1;
	}

	inodeid_t idFileDestino;
	 //procurar o id do inode do segundo ficheiro		*/
	 if (fsi_dir_search(fs,dirDestino,nomeDestino,&idFileDestino) != 0) {
	 	printf("[fs_append] erro, o ficheiro de destino nao foi criado\n");
	 	terminaEscrita(fs);
		return -1;
	}

	fs_inode_t* iFileDestino = &fs->inode_tab[idFileDestino];
	 if(iFileOrigem->type != FS_FILE){
		 printf("[fs_append] inode para onde pretende copiar nao e um ficheiro");
		 terminaEscrita(fs);
	 	 return -1;
	   }

	 dprintf("[fs_append]1º: directorio %d, inode ficheiro %d\n",dirOrigem,idFileOrigem);
	 dprintf("[fs_append]2º: directorio %d, inode ficheiro %d\n",dirDestino,idFileDestino);


	 //calcular o numero de blocos ocupados por cada ficheiro
	 int num = OFFSET_TO_BLOCKS(iFileOrigem->size);//
	 int num2 = OFFSET_TO_BLOCKS(iFileDestino->size);

	 dprintf("[fs_append]freepointers file1: %d   file2 :%d\n", num, num2);
	 if(INODE_NUM_BLKS-num < num2){
		 printf("[fs_append] File Will not Fit\n");
		 terminaEscrita(fs);
		return -1;
	 }

	 //Fazer a copia das referencias
	 for(int a=0;a<num2;++a){
		 iFileOrigem->blocks[num+a] = iFileDestino->blocks[a];
		 fs->referencias[iFileDestino->blocks[a]]++;//adicionar referencia do destino ao bloco
	 }
	 //mandar o novo tamanho do ficheiro
	 iFileOrigem->size += iFileDestino->size;
	 *fsize = iFileOrigem->size;
	 dprintf("[fs_append] new file size %u\n",*fsize);
	 //imprimirInodeTab(fs);
	 terminaEscrita(fs);
	 return 0;
}


/* //SNFS_defrag
* Efectua a operacao de desfragmentacao do disco.
* STAT_OK - teve sucesso 1
* STAT_BUSY - esta em curso outra operacao de desfragmentacao 0
* STAT_ERROR se nao foi possivel a desgramentacao por falta de espaco -1
*/
static int defrag_state = 0;

void mexeBlocoPara(fs_t* fs, int bl, int dest);

int fs_defrag(fs_t* fs){
	
	if(defrag_state == 1){
		printf("[snfs_defrag] Esta em curso outra operacao de desfragmentacao\n");
		return 0;
	}
	iniciaEscrita(fs);
	dprintf("[fs_defrag]DESFRAGMENTANDO\n");
	defrag_state = 1;

	//verifica quantidade de blocos vazios
	int blocosvz=0;
	int i=INODES_USED_BY_FS*INODE_NUM_BLKS;
	for(;i<NUM_BLOCOS_MAX;++i)
		if(BMAP_ISSET(fs->blk_bmap,i)){
			++blocosvz;
			break;
		}


	if(blocosvz==0){
		printf("\n[snfs_defrag] Nao existe espaço suficiente\n");
		defrag_state = 0;
		terminaEscrita(fs);
		return -1;
	}

	int z,q,pos=10;//pos a posiçao de ordenamento
	for(z=INODES_USED_BY_FS;z<ITAB_SIZE;++z){
		for(q=0;q<INODE_NUM_BLKS;++q){
			if(fs->inode_tab[z].blocks[q]==pos){	//se for igual passa a frente
				++pos;
			}else if(fs->inode_tab[z].blocks[q]>pos){//verifica se é para organizar
					mexeBlocoPara(fs, fs->inode_tab[z].blocks[q], pos);
					++pos;
			}
		}
	}
	puts("");
	defrag_state = 0;
	terminaEscrita(fs);
	return 1;

}
 
//mexe bloco bl para pos dest
void mexeBlocoPara(fs_t* fs, int bl, int dest){

	dprintf(".");

	int o,p,r=0;
	int l,w;
	int refinodes[ITAB_SIZE+1];	// inodes que apontam para o bloco e
	int tempRef[ITAB_SIZE+1];	// inodes que apontam pa o bloco temporario
	char* tp = malloc(sizeof(char)*BLOCK_SIZE);

	for(o=0;o<ITAB_SIZE+1;++o)//zera o vector
				refinodes[o] = 0;
	for(o=0;o<ITAB_SIZE+1;++o)//zera o vector
			tempRef[o] = 0;

	//encontra as referencia para esse bloco
	for(o=INODES_USED_BY_FS;o<ITAB_SIZE;o++){// mete no vector os inodes que referenciam este bloco
		for(p=0;p<INODE_NUM_BLKS;++p)
			if(fs->inode_tab[o].blocks[p] == bl){
				refinodes[r++] = o;
				break;
			}
	}



	if(!BMAP_ISSET(fs->blk_bmap,dest)){//se o bloco destino estiver vazio copia para para la

		block_read(fs->blocks,bl,tp);	//copia o ficheiro encontrado
		block_write(fs->blocks,dest,tp);// grava-o na nova posiçao
		BMAP_SET(fs->blk_bmap,dest); // actualiza bitmap
		for(l=0;refinodes[l] != 0;++l){
			for(w=0;w<INODE_NUM_BLKS;++w){//actualiza referencias
				if(fs->inode_tab[refinodes[l]].blocks[w] == bl){
					dprintf("inode %d bloco:%d     de %d para%d\n",refinodes[l],w,bl,dest);
					fs->inode_tab[refinodes[l]].blocks[w] = dest;
					break;
				}
			}
		}
	}else{//senao copia o que estava la para o fim do disco

		//encontra um bloco vazio a contar do fim
		int k=NUM_BLOCOS_MAX; // vai ter a pos do bloco vazio

		for(;k>INODES_USED_BY_FS*INODE_NUM_BLKS-1;--k){
			if(!BMAP_ISSET(fs->blk_bmap,k)){
				break;
			}
		}
		BMAP_SET(fs->blk_bmap,k);//mete o bloco k como utilizado
		r = 0;
		for(o=INODES_USED_BY_FS;o<ITAB_SIZE;o++){// mete no vector os inodes que referenciam o bloco temporario
				for(p=0;p<INODE_NUM_BLKS;++p)
					if(fs->inode_tab[o].blocks[p] == dest){
						tempRef[r++] = o;
						break;
					}
		}
		block_read(fs->blocks,dest,tp);	//copia o ficheiro temporario
		block_write(fs->blocks,k,tp);// grava-o na nova posiçao temporaria
		BMAP_SET(fs->blk_bmap,k); // actualiza bitmap
		for(l=0;tempRef[l] != 0;++l){
			for(w=0;w<INODE_NUM_BLKS;++w){//actualiza referencias
				if(fs->inode_tab[tempRef[l]].blocks[w] == dest){
					dprintf("inode %d   bloco:%d     de   %d   para  %d\n",tempRef[l],w,dest,k);
					fs->inode_tab[tempRef[l]].blocks[w] = k;
					break;
				}
			}
		}
		invalidarBloco(fs->cache,dest);//retira o bloco da cache se existir

		block_read(fs->blocks,bl,tp);	//copia o ficheiro encontrado
		block_write(fs->blocks,dest,tp);// grava-o na nova posiçao
		BMAP_SET(fs->blk_bmap,dest); // actualiza bitmap
		for(l=0;refinodes[l] != 0;++l){
			for(w=0;w<INODE_NUM_BLKS;++w){//actualiza referencias
				if(fs->inode_tab[refinodes[l]].blocks[w] == bl){
					dprintf("inode %d     bloco:%d     de %d para%d\n",refinodes[l],w,bl,dest);
					fs->inode_tab[refinodes[l]].blocks[w] = dest;
					break;
				}
			}
		}

	}
	invalidarBloco(fs->cache,bl);//retira o bloco da cache se existir
	BMAP_CLR(fs->blk_bmap,bl); // actualiza bitmap
	fsi_store_fsdata(fs);//guarda os bitmaps
}
 
/* //SNFS_DISKUSAGE
* Efectua o dump, do lado do servidor, dos blocos do sisstema de ficheiros utilizados e correspondentes
* ficheiros/directorias que os ocupam. O resultado e STAT_OK se tiver sucesso.
*/ 
/* //SNFS_DUMPCACHE
* Efectua o dump, do lado do servidor, das entradas da cache de blocos do servidor.
* Stat_OK se sucesso
*/ 

/* FUNCOES INTERNAS PARA DEBUG DO SISTEMA*/
void imprimirInodeTab(fs_t* fs){
	
	  dprintf("\n\n////////////////DUMP///////////// \n\n ");
	  int i,w;
	dprintf("TABELA DE INODES\n");
	dprintf("type1: directorio, type2:ficheiro\n");
	for(i = 0; i<ITAB_SIZE;i++){
	  if(fs->inode_tab[i].size>0){
			dprintf("\n");
			dprintf("inode:%d  %d:%d    \n",i,(fs->inode_tab[i]).type,(fs->inode_tab[i]).size);
			for(w=0;w<INODE_NUM_BLKS;++w){
				if(fs->inode_tab[i].blocks[w] >0)
					dprintf("-->bloco[%d] = %d\n",w,fs->inode_tab[i].blocks[w]);
			}
			}

	}
	dprintf("/////////////////////////////////////////\n");
}


void fs_dump(fs_t* fs)
{
	iniciaLeitura(fs);
   printf("[fs_dump]Free block bitmap:\n");
   fsi_dump_bmap(fs->blk_bmap,BLOCK_SIZE);
   printf("\n");
   
   printf("[fs_dump]Free inode table bitmap:\n");
   fsi_dump_bmap(fs->inode_bmap,BLOCK_SIZE);
   printf("\n");
   terminaLeitura(fs);
}

/**===== Dump: FileSystem Blocks =======================
blk_id: < no do bloco>
file_name1: <pathname do 1o ficheiro/directoria que usa o
bloco>
file_name2: <pathname do 2o ficheiro/directoria que usa o
bloco>
. . . . . .
file_nameN: <pathname do No ficheiro/directoria que usa o
bloco>
*******************************************************
**/
int fs_diskUsage(fs_t* fs){
	unsigned idBloco,novoIdBloco;
	int i;
	
	iniciaLeitura(fs);
	
	List listaReferencias = gerar_lista_referencias(fs);
	noRef_t referencia;
	void* leitor;
	
	printf("===== Dump: FileSystem Blocks =======================\n");
		
	i = 1;
	if(listSize(listaReferencias) !=0){
		listRemoveFst(listaReferencias,&leitor);	//remover uma referencia
		referencia = (noRef_t) leitor;
		printf("blk_id: %u\n",referencia->blockId);
		idBloco = novoIdBloco = referencia->blockId;
		printf("file_name%d: %s\n",i++,referencia->pathname);
	}
		
	while(listSize(listaReferencias) !=0){
		listRemoveFst(listaReferencias,&leitor);//remover uma referencia
		referencia = (noRef_t) leitor;
		novoIdBloco = referencia->blockId;
		
		if(novoIdBloco != idBloco){	//se ja nao e o mesmo bloco
			printf("blk_id: %u\n",novoIdBloco);
			idBloco = novoIdBloco;	//passamos a listar um novo bloco;
			i = 1;
		}
		printf("file_name%d: %s\n",i++,referencia->pathname);
	}
	printf("*******************************************************\n");
	terminaLeitura(fs);
	return 0;
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
int fs_dumpcache(fs_t* fs){
	iniciaEscrita(fs);	//apesar de ser uma leitura, garantimos que mais nenhuma thread está a modificar a cache (o que acontece se permitirmos leituras)
	int status = visualizarCache(fs->cache);
	terminaEscrita(fs);
	return status;
}

void* thread_actualiza_Cache(void* sistemaFicheiros){
	
	fs_t* fs = (fs_t*) sistemaFicheiros;
	int tempoInvervalo = INTERVALO_TEMPO_CACHE;	//guardar os dados de 2 em 2 segundos
	while(1){
		if((actualizarCache(fs->cache,tempoInvervalo)) != 0){
			printf("[thread_actualizaCache] Erro ao actualizar a cache\n");
			sthread_exit(NULL);
		}
			    
		sthread_sleep(tempoInvervalo*100000); 
	}
	sthread_exit(NULL);
}
