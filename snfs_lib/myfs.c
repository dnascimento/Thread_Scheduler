/* 
 * File System Interface
 * 
 * myfs.c
 *
 * Implementation of the SNFS programming interface simulating the 
 * standard Unix I/O interface. This interface uses the SNFS API to
 * invoke the SNFS services in a remote server.
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <myfs.h>
#include <snfs_api.h>
#include <snfs_proto.h>
#include <unistd.h>
#include "queue.h"

#define dprintf if(0) printf

#define SUBSTITUI 1

#ifndef SERVER_SOCK
#define SERVER_SOCK "/tmp/server.socket"
#endif

#define MAX_OPEN_FILES 10		// how many files can be open at the same time


static queue_t *Open_files_list;		// Open files list
static int Lib_initted = 0;           // Flag to test if library was initted
static int Open_files = 0;		// How many files are currently open


int mkstemp(char *template);

int myparse(char *pathname);

int my_init_lib()
{
	char CLIENT_SOCK[] = "/tmp/clientXXXXXX";
	if(mkstemp(CLIENT_SOCK) < 0) {
		printf("[my_init_lib] Unable to create client socket.\n");
		return -1;
	}
	
	if(snfs_init(CLIENT_SOCK, SERVER_SOCK) < 0) {
		printf("[my_init_lib] Unable to initialize SNFS API.\n");
		return -1;
	}
	Open_files_list = queue_create();
	Lib_initted = 1;
	
	return 0;
}


int my_open(char* name, int flags)
{
	
	dprintf("[my_open]Inicio myopen\n");
	if (!Lib_initted) {
		printf("[my_open] Library is not initialized.\n");
		return -1;
	}
	
	if(Open_files >= MAX_OPEN_FILES) {
		printf("[my_open] All slots filled.\n");
		return -1;
	}
	
        if ( myparse(name) != 0 ) {
		printf("[my_open] Malformed pathname.\n");
		return -1;
            
        }
           	

	
	snfs_fhandle_t dir, file_fh;
	unsigned fsize = 0;
	char newfilename[MAX_FILE_NAME_SIZE];
	char newdirname[MAX_PATH_NAME_SIZE];
	char fulldirname[MAX_PATH_NAME_SIZE];
	char *token;
	char *search="/";
	int i=0;
	
	memset(&newfilename,0,MAX_FILE_NAME_SIZE);
	memset(&newdirname,0,MAX_PATH_NAME_SIZE);
	memset(&fulldirname,0,MAX_PATH_NAME_SIZE);

	strcpy(fulldirname,name);
	token = strtok(fulldirname, search);


        while(token != NULL) {
             i++;
             strcpy(newfilename,token);

             token = strtok(NULL, search);
         } 
         if ( i > 1){
              strncpy(newdirname, name, strlen(name)-(strlen(newfilename)+1));
              newdirname[strlen(newdirname)] = '\0';
            }
         else {  
            strncpy(newfilename, &name[1], (strlen(name)-1)); 
            newfilename[strlen(newfilename)] = '\0';
            strcpy(newdirname, name);   
          }    	

	if(newfilename == NULL) {
		printf("[my_open] Error looking for directory in server.\n");
		return -1;
	}
	dprintf("[my_open]Vou procurar o ficheiro de nome %s\n",name);
    snfs_call_status_t status = snfs_lookup(name,&file_fh,&fsize);
	 
	dprintf("[my_open]Encontrei o ficheiro na entrada: %d\n",file_fh);
	
	if (status != STAT_OK) {
			dprintf("[my_open]Vou criar um directorio com nome %s\n",newdirname);
	         snfs_lookup(newdirname,&dir,&fsize);        
         
                 if (i==1)  //Create a file in Root directory
	             dir = ( snfs_fhandle_t)  1;
	} 
	
	if(flags == O_CREATE && status != STAT_OK) {
		dprintf("[my_open]Vou tentar criar o ficheiro\n");
		
		if (snfs_create(dir,newfilename,&file_fh) != STAT_OK) {
			printf("[my_open] Error creating a file in server.\n");
			return -1;
		}
	} else	if (status != STAT_OK) {
		printf("[my_open] Error opening up file.\n");
		return -1;
	}
	
	fd_t fdesc = (fd_t) malloc(sizeof(struct _file_desc));
	fdesc->fileId = file_fh;
	fdesc->size = fsize;
	fdesc->write_offset = 0;
	fdesc->read_offset = 0;
	
	queue_enqueue(Open_files_list, fdesc);
	Open_files++;
	
	return file_fh;
}

int my_read(int fileId, char* buffer, unsigned numBytes)
{

	if (!Lib_initted) {
		printf("[my_read] Library is not initialized.\n");
		return -1;
	}
	
	fd_t fdesc = queue_node_get(Open_files_list, fileId);
	if(fdesc == NULL) {
		printf("[my_read] File isn't in use. Open it first.\n");
		return -1;
	}
	
	// EoF ?
	if(fdesc->read_offset == fdesc->size)
		return 0;
	
	int nread;
	unsigned rBytes;
	int counter = 0;
	
	// If bytes to be read are greater than file size
	if(fdesc->size < ((unsigned)fdesc->read_offset) + numBytes)
		numBytes = fdesc->size - (unsigned)(fdesc->read_offset);
	
	//if data must be read using blocks
	if(numBytes > MAX_READ_DATA)
		rBytes = MAX_READ_DATA;
	else
		rBytes = numBytes;
	
	while(numBytes) {
		if(rBytes > numBytes)
			rBytes = numBytes;
		
		if (snfs_read(fileId,(unsigned)fdesc->read_offset,rBytes,&buffer[counter],&nread) != STAT_OK) {
			printf("[my_read] Error reading from file.\n");
			return -1;
		}
		fdesc->read_offset += nread;
		numBytes -= (unsigned)nread;
		counter += nread;
	}
	
	return counter;
}

int my_write(int fileId, char* buffer, unsigned numBytes)
{
	if (!Lib_initted) {
		printf("[my_write] Library is not initialized.\n");
		return -1;
	}
	
	fd_t fdesc = queue_node_get(Open_files_list, fileId);
	if(fdesc == NULL) {
		printf("[my_write] File isn't in use. Open it first.\n");
		return -1;
	}
	
	unsigned fsize;
	unsigned wBytes;
	int counter = 0;
	
	if(numBytes > MAX_WRITE_DATA)
		wBytes = MAX_WRITE_DATA;
	else
		wBytes = numBytes;
	
	while(numBytes) {
		if(wBytes > numBytes)
			wBytes = numBytes;
		
		if (snfs_write(fileId,(unsigned)fdesc->write_offset,wBytes,&buffer[counter],&fsize) != STAT_OK) {
			printf("[my_write] Error writing to file.\n");
			return -1;
		}
		fdesc->size = fsize;
		fdesc->write_offset += (int)wBytes;
		numBytes -= (unsigned)wBytes;
		counter += (int)wBytes;
	}
	
	return counter;
}

int my_close(int fileId)
{
	if (!Lib_initted) {
		printf("[my_close] Library is not initialized.\n");
		return -1;
	}
	
	fd_t temp = queue_node_remove(Open_files_list, fileId);
	if(temp == NULL) {
		printf("[my_close] File isn't in use. Open it first.\n");
		return -1;
	}
	
	free(temp);
	Open_files--;
	
	return 0;
}


int my_listdir(char* path, char **filenames, int* numFiles)
{
	if (!Lib_initted) {
		printf("[my_listdir] Library is not initialized.\n");
		return -1;
	}
	
	snfs_fhandle_t dir;
	unsigned fsize;	
	
        if ( myparse(path) != 0 ) {
       	     printf("[my_listdir] Error looking for folder in server.\n");
            return -1;
         }   
     		
	if ((strlen(path)==1) && (path[0]== '/') ) dir = ( snfs_fhandle_t)  1;
	else
          if(snfs_lookup(path, &dir, &fsize) != STAT_OK) {
	     printf("[my_listdir] Error looking for folder in server.\n");
	     return -1;
	   }
	
	
	snfs_dir_entry_t list[MAX_READDIR_ENTRIES];
	unsigned nFiles;
	char* fnames;
	
	if (snfs_readdir(dir, MAX_READDIR_ENTRIES, list, &nFiles) != STAT_OK) {
		printf("[my_listdir] Error reading directory in server.\n");
		return -1;
	}
	
	*numFiles = (int)nFiles;
	
	*filenames = fnames = (char*) malloc(sizeof(char)*((MAX_FILE_NAME_SIZE+1)*(*numFiles)));
	for (int i = 0; i < *numFiles; i++) {
		strcpy(fnames, list[i].name);
		fnames += strlen(fnames)+1;
	}
	
	return 0;
}

int my_mkdir(char* dirname) {
	if (!Lib_initted) {
		printf("[my_mkdir] Library is not initialized.\n");
		return -1;
	}

        if ( myparse(dirname) != 0 ) {
		printf("[my_mkdir] Malformed pathname.\n");
		return -1;
            
        }
	
	
	
	snfs_fhandle_t dir, newdir;
	unsigned fsize;
	char newfilename[MAX_FILE_NAME_SIZE];
	char newdirname[MAX_PATH_NAME_SIZE];
	char fulldirname[MAX_PATH_NAME_SIZE];
	char *token;
	char *search="/";
	int i=0;
	
	memset(&newfilename,0,MAX_FILE_NAME_SIZE);
	memset(&newdirname,0,MAX_PATH_NAME_SIZE);
	memset(&fulldirname,0,MAX_PATH_NAME_SIZE);
	
        if(snfs_lookup(dirname, &dir, &fsize) == STAT_OK) {
		printf("[my_mkdir] Error creating a  subdirectory that already exists.\n");
		return -1;
	}
	

	strcpy(fulldirname,dirname);
	token = strtok(fulldirname, search);


        while(token != NULL) {
             i++;
             strcpy(newfilename,token);
             token = strtok(NULL, search);
         } 
         if ( i > 1){
              strncpy(newdirname, dirname, strlen(dirname)-(strlen(newfilename)+1));
              newdirname[strlen(newdirname)] = '\0';
             }
         else {  
            strncpy(newfilename, &dirname[1], (strlen(dirname)-1));
             newfilename[strlen(newfilename)] = '\0'; 
            strcpy(newdirname, dirname);   
          }    
	

	if(newdirname == NULL) {
		printf("[my_mkdir] Error looking for directory in server.\n");
		return -1;
	}
	
	
	if (i==1)  //Create a directory in Root
	     dir = ( snfs_fhandle_t)  1;
	else   //Create a directory elsewhere
	   if(snfs_lookup(newdirname, &dir, &fsize) != STAT_OK) {
			printf("[my_mkdir] Error creating a  subdirectory which has a wrong pathname.\n");
			return -1;
	   }


	if(snfs_mkdir(dir, newfilename, &newdir) != STAT_OK) {
		printf("[my_mkdir] Error creating new directory in server.\n");
		return -1;
	}
	
	return 0;
}

int myparse(char* pathname) {

	char line[MAX_PATH_NAME_SIZE]; 
	char *token;
	char *search = "/";
	int i=0;

	strcpy(line,pathname); 

	if(strlen(line) >= MAX_PATH_NAME_SIZE || (strlen(line) < 1) ) {
	   printf("[myparse] Wrong pathname size.\n"); 
	   return -1; 
	   }

	if (strchr(line, ' ') != NULL || strstr( (const char *) line, "//") != NULL || line[0] != '/' ) {
	   printf("[myparse] Malformed pathname.\n");
	   return -1; 
	   }


	if ((i=strlen(pathname)) && line[i]=='/') {
	   printf("[myparse] Malformed pathname.\n");
	   return -1; 
	   }
	   
	i=0;
	token = strtok(line, search);

	while(token != NULL) {
	  if ( strlen(token) > MAX_FILE_NAME_SIZE -1) { 
	      printf("[myparse] File/Directory name size exceeded limits.:%s \n", token);
	      return -1; 
	      }
	  i++;

	  token = strtok(NULL, search);
	}

	return 0;
}




/*Apaga o ficheiro ou directório identificado por name. Devolve 1 se teve sucesso,
0 caso contrário.*/
int my_remove(char *name){
	
	dprintf("\n\n\n[my_remove] inicio\n");
	if(!Lib_initted){	
		printf("[my_remove] Library is not initialized.\n");
		return 0;
	}
	
	if(myparse(name) != 0){
		printf("[my_remove] Malformed pathname.\n");
		return 0;
	}
	
	snfs_fhandle_t novoFileHandler, file_fh,dir_fh; //recebe o fhandler do ficheiro
	unsigned fsize,dirsize;
	char newfilename[MAX_FILE_NAME_SIZE];	//buffer para o nome do ficheiro
	char newdirname[MAX_PATH_NAME_SIZE];	//buffer para o dome do directorio
	char fulldirname[MAX_PATH_NAME_SIZE];	//buffer para todo o caminho
	
	char* token;	//ponteiro para o token que vamos fazer
	char* search ="/"; //separador do caminho
	int i = 0;
	
	memset(&newfilename,0,MAX_FILE_NAME_SIZE);	//zerar os buffers
	memset(&newdirname,0,MAX_PATH_NAME_SIZE);
	memset(&fulldirname,0,MAX_PATH_NAME_SIZE);


	/*Verificar se o ficheiro a remover existe*/
	if(snfs_lookup(name,&file_fh,&fsize) != STAT_OK){	//nao encontrou?
		printf("[my_remove] o ficheiro que pretende remover nao existe\n");	
		return 0;
	}

	strcpy(fulldirname,name);		//copiar o nome
	token = strtok(fulldirname, search);	//quebrar o nome por cada /

	while(token != NULL){	//enquanto houver nomes de directorio
		i++;	//contar o numero de nomes
		strcpy(newfilename,token);	//copiar o 1º nome para o file name
		
		token = strtok(NULL,search);	//qubrar mais um /
	}
	
	if(i>1){	//se nao e directorio de raiz
		strncpy(newdirname,name,strlen(name)-(strlen(newfilename)+1));	//nome do directorio, sem o ficheiro
		newdirname[strlen(newdirname)] = '\0';
	}
	else{
		strncpy(newfilename,&name[1],(strlen(name)-1));	//o nome do ficheiro
		newfilename[strlen(newfilename)] = '\0'; 
		strcpy(newdirname,name);	//nome do directorio
	}

	if(newfilename == NULL){
		printf("[my_remove] estou a tentar remover um ficheiro sem nome\n");
		return 0;
	}
	
	dprintf("fulldirname : %s\n newdirname: %s \n newfilename : %s\n",fulldirname,newdirname,newfilename);
	
	if(i==1)	//vamos remover um ficheiro do root
		dir_fh = (snfs_fhandle_t) 1;	
	
	else{			//procurar o directorio de onde vamos remover
		if(snfs_lookup(newdirname,&dir_fh,&dirsize)){	//obter o hanlder do directorio
			printf("[my_remove] o directorio nao existe\n");	
			return -1;
		}
	}
	
	if((snfs_remove(dir_fh,newfilename,&novoFileHandler)) !=STAT_OK){		//enviar a mensagem para o servidor  
		printf("[my_remove] erro no servidor ao remover o ficheiro");
		return 0;
	}
	
	return 1;	//teve sucesso
}
	

	


/*Copia o ficheiro ou directório identificado por name1 para outro ficheiro ou
directório identificado por name2. Devolve 1 se teve sucesso, 0 caso contrário.*/
int my_copy(char *nameOrigem,char *nameDestino){
	
	dprintf("[my_copy] Copiar: %s para %s\n",nameOrigem,nameDestino);
	
	if(!Lib_initted){	//verificar se a livraria esta inicializada
		printf("[my_copy] Library is not initialized.\n");
		return 0;
	}
	
	if(myparse(nameOrigem) != 0 && myparse(nameDestino) != 0 ){
		printf("[my_copy] Malformed pathname.\n"); 
		return 0;
	}	 
	
	char *token;
	char *search="/";
	
	
	/*Tratar nome da origem*/
	snfs_fhandle_t dirOrigem_fh,fileOrigem_fh;
	unsigned origemfsize,dirOrigemsize;	
	
	
	/*Procurar o ficheiro que queremos copiar e obter o file handler*/
	if(snfs_lookup(nameOrigem,&fileOrigem_fh,&origemfsize) != STAT_OK){	//nao encontrou?
		printf("[my_copy] o ficheiro que pretende copiar nao existe\n");		
		return 0;
	}	
	
	//criar strings
	char filenameOrigem[MAX_FILE_NAME_SIZE];		//para o nome do ficheiro
	char dirnameOrigem[MAX_PATH_NAME_SIZE];		//para o nome directorio
	char fulldirnameOrigem[MAX_PATH_NAME_SIZE];		//para o caminho completo
	
	memset(&filenameOrigem,0,MAX_FILE_NAME_SIZE);	//zerar os buffers
	memset(&dirnameOrigem,0,MAX_PATH_NAME_SIZE);
	memset(&fulldirnameOrigem,0,MAX_PATH_NAME_SIZE);
	
	strcpy(fulldirnameOrigem,nameOrigem);		//copiar o nome
	
	dprintf("fulldirOrigem %s",fulldirnameOrigem);
	
	token = strtok(fulldirnameOrigem, search);	//quebrar o nome por cada /
	int i=0;
	while(token != NULL){	//enquanto houver nomes de directorio
		i++;	//contar o numero de nomes
		strcpy(filenameOrigem,token);	//copiar o 1º nome para o file name
		token = strtok(NULL,search);	//qubrar mais um /
	}
	
	if(i>1){	//se nao e directorio de raiz
		strncpy(dirnameOrigem,nameOrigem,strlen(nameOrigem)-(strlen(filenameOrigem)+1));	//nome do directorio, sem o ficheiro
		dirnameOrigem[strlen(dirnameOrigem)] = '\0';
	}
	else{
		strncpy(filenameOrigem,&nameOrigem[1],(strlen(nameOrigem)-1));	//o nome do ficheiro
		filenameOrigem[strlen(filenameOrigem)] = '\0'; 
		strcpy(dirnameOrigem,nameOrigem);	//nome do directorio
	}
	
	if(filenameOrigem == NULL){
		printf("[my_copy] estou a tentar copiar UM DIRECTORIO\n");		//se estiver a ler um directorio sai
		return 0;
	}
	
	
	if(i==1){	//vamos copiar um ficheiro do root
		dprintf("vou copiar apartir do root\n");
		dirOrigem_fh = (snfs_fhandle_t) 1;	
	}
	else{			//procurar o directorio de onde vamos copiar
		if(snfs_lookup(dirnameOrigem,&dirOrigem_fh,&dirOrigemsize) != STAT_OK){	//obter o hanlder do directorio
			printf("[my_copy] o directorio de origem nao existe\n");	
			return 0;
		}
	}
		
	dprintf("Obti o nome %s para a origem\n",filenameOrigem);
	
	//Tratar ficheiro de destino
	snfs_fhandle_t dirDestino_fh,fileDestino_fh;
	unsigned dirDestinosize,destinofsize;
	
	//criar strings
	char filenameDestino[MAX_FILE_NAME_SIZE];		//para o nome do ficheiro
	char dirnameDestino[MAX_PATH_NAME_SIZE];		//para o nome directorio
	char fulldirnameDestino[MAX_PATH_NAME_SIZE];		//para o caminho completo
	
	memset(&filenameDestino,0,MAX_FILE_NAME_SIZE);	//zerar os buffers
	memset(&dirnameDestino,0,MAX_PATH_NAME_SIZE);
	memset(&fulldirnameDestino,0,MAX_PATH_NAME_SIZE);
	
	strcpy(fulldirnameDestino,nameDestino);		//copiar o nome
	token = strtok(fulldirnameDestino, search);	//quebrar o nome por cada /
	i=0;
	while(token != NULL){	//enquanto houver nomes de directorio
		i++;	//contar o numero de nomes
		strcpy(filenameDestino,token);	//copiar o 1º nome para o file name
		token = strtok(NULL,search);	//qubrar mais um /
	}
	
	if(i>1){	//se nao e directorio de raiz
		strncpy(dirnameDestino,nameDestino,strlen(nameDestino)-(strlen(filenameDestino)+1));	//nome do directorio, sem o ficheiro
		dirnameDestino[strlen(dirnameDestino)] = '\0';
	}
	else{
		strncpy(filenameDestino,&nameDestino[1],(strlen(nameDestino)-1));	//o nome do ficheiro
		filenameDestino[strlen(filenameDestino)] = '\0';
		strcpy(dirnameDestino,nameDestino);	//nome do directorio
	}
	
	if(filenameDestino == NULL){
		printf("[my_copy] path errado\n");	
		return 0;
	}
	
	if(i==1)	//vamos copiar um ficheiro para oroot
		dirDestino_fh = (snfs_fhandle_t) 1;	
	
	else{			//procurar o directorio para onde vamos copiar
		if(snfs_lookup(dirnameDestino,&dirDestino_fh,&dirDestinosize) != STAT_OK){	//obter o hanlder do directorio
			printf("[my_copy] o directorio de Destino nao existe\n");	
			return 0;
		}
	}
	
	
	/*Procurar o ficheiro para onde iremos copiar e obter o file handler*/
	if(snfs_lookup(nameDestino,&fileDestino_fh,&destinofsize) != STAT_OK){	//o ficheiro nao existe?
			if((snfs_create(dirDestino_fh,filenameDestino,&fileDestino_fh)) != STAT_OK){
				printf("[my_copy] erro ao criar novo ficheiro\n");
				return 0;
			}
	}	
	
	else if(!SUBSTITUI){
		printf("[my_copy] Substituicao de ficheiro nao autorizada");
		return 0;
	}
					
	dprintf("Vou copiar do directorio %d, o ficheiro %s para o directorio %d, ficheiro %s\n\n",dirOrigem_fh,filenameOrigem,dirDestino_fh,filenameDestino);
	int tstatus = snfs_copy(dirOrigem_fh,filenameOrigem,dirDestino_fh,filenameDestino,&fileDestino_fh);

	if(tstatus == RES_OK)
		return 1;
	else
		return 0;
}


/*Concatena ao ficheiro name1 o conteudo do ficheiro name2. Devolve 1 se teve sucesso, 0 caso contrario.
 * Em caso de sucesso, apenas o conteudo name1 se altera*/
int my_append(char *nameOrigem,char *nameDestino){
	dprintf("[my_append] Fazer append de: %s para %s\n",nameOrigem,nameDestino);

		if(!Lib_initted){	//verificar se a livraria esta inicializada
			printf("[my_append] Library is not initialized.\n");
			return -1;
		}

		if(myparse(nameOrigem) != 0 && myparse(nameDestino) != 0 ){
			printf("[my_append] Malformed pathname.\n");
			return -1;
		}

		char *token;
		char *search="/";


		/*Tratar nome da origem*/
		snfs_fhandle_t dirOrigem_fh,fileOrigem_fh;
		unsigned origemfsize,dirOrigemsize;


		/*Procurar o ficheiro que queremos copiar e obter o file handler*/
		if(snfs_lookup(nameOrigem,&fileOrigem_fh,&origemfsize) != STAT_OK){	//nao encontrou?
			printf("[my_append] o ficheiro que pretende fazer append nao existe\n");
			return -1;
		}

		//criar strings
		char filenameOrigem[MAX_FILE_NAME_SIZE];		//para o nome do ficheiro
		char dirnameOrigem[MAX_PATH_NAME_SIZE];		//para o nome directorio
		char fulldirnameOrigem[MAX_PATH_NAME_SIZE];		//para o caminho completo

		memset(&filenameOrigem,0,MAX_FILE_NAME_SIZE);	//zerar os buffers
		memset(&dirnameOrigem,0,MAX_PATH_NAME_SIZE);
		memset(&fulldirnameOrigem,0,MAX_PATH_NAME_SIZE);

		strcpy(fulldirnameOrigem,nameOrigem);		//copiar o nome

		dprintf("[my_append] fulldirOrigem %s",fulldirnameOrigem);

		token = strtok(fulldirnameOrigem, search);	//quebrar o nome por cada /
		int i=0;
		while(token != NULL){	//enquanto houver nomes de directorio
			i++;	//contar o numero de nomes
			strncpy(filenameOrigem,token,strlen(token));	//copiar o 1º nome para o file name
			token = strtok(NULL,search);	//qubrar mais um /
		}

		if(i>1)	//se nao e directorio de raiz
			strncpy(dirnameOrigem,nameOrigem,strlen(nameOrigem)-(strlen(filenameOrigem)+1));	//nome do directorio, sem o ficheiro
		else{
			strncpy(filenameOrigem,&nameOrigem[1],(strlen(nameOrigem)-1));	//o nome do ficheiro
			strcpy(dirnameOrigem,nameOrigem);	//nome do directorio
		}

		if(filenameOrigem == NULL){
			printf("[my_append] estou a tentar fazer append de  UM DIRECTORIO\n");		//se estiver a ler um directorio sai
			return -1;
		}


		if(i==1){	//vamos fazer append de um ficheiro do root directory
			dirOrigem_fh = (snfs_fhandle_t) 1;
		}
		else{			//procurar o directorio de onde vamos remover
			if(snfs_lookup(dirnameOrigem,&dirOrigem_fh,&dirOrigemsize) != STAT_OK){	//obter o hanlder do directorio
				printf("[my_append] o directorio de origem nao existe\n");
				return -1;
			}
		}

		dprintf("Obti o nome %s para a origem\n",filenameOrigem);

		//Tratar ficheiro de destino
		snfs_fhandle_t dirDestino_fh,fileDestino_fh;
		unsigned destinofsize, dirDestinosize, res;

		/*Procurar o ficheiro que queremos copiar e obter o file handler*/
		if(snfs_lookup(nameDestino,&fileDestino_fh,&destinofsize) != STAT_OK){	//encontrou?
			printf("[my_append] o ficheiro d\n");
			return -1;
		}

		//criar strings
		char filenameDestino[MAX_FILE_NAME_SIZE];		//para o nome do ficheiro
		char dirnameDestino[MAX_PATH_NAME_SIZE];		//para o nome directorio
		char fulldirnameDestino[MAX_PATH_NAME_SIZE];		//para o caminho completo

		memset(&filenameDestino,0,MAX_FILE_NAME_SIZE);	//zerar os buffers
		memset(&dirnameDestino,0,MAX_PATH_NAME_SIZE);
		memset(&fulldirnameDestino,0,MAX_PATH_NAME_SIZE);

		strcpy(fulldirnameDestino,nameDestino);		//copiar o nome
		token = strtok(fulldirnameDestino, search);	//quebrar o nome por cada /
		i=0;
		while(token != NULL){	//enquanto houver nomes de directorio
			i++;	//contar o numero de nomes
			strncpy(filenameDestino,token,strlen(token));	//copiar o 1º nome para o file name
			token = strtok(NULL,search);	//qubrar mais um /
		}

		if(i>1)	//se nao e directorio de raiz
			strncpy(dirnameDestino,nameDestino,strlen(nameDestino)-(strlen(filenameDestino)+1));	//nome do directorio, sem o ficheiro
		else{
			strncpy(filenameDestino,&nameDestino[1],(strlen(nameDestino)-1));	//o nome do ficheiro
			strcpy(dirnameDestino,nameDestino);	//nome do directorio
		}

		if(filenameDestino == NULL){
			printf("[my_append] estou a tentar fazer append a UM DIRECTORIO\n");		//se estiver a ler um directorio sai
			return -1;
		}

		if(i==1)	//vamos copiar um ficheiro do root
			dirDestino_fh = (snfs_fhandle_t) 1;

		else{			//procurar o directorio de onde vamos fazer append
			if(snfs_lookup(dirnameDestino,&dirDestino_fh,&dirDestinosize) != STAT_OK){	//obter o hanlder do directorio
				printf("[my_append] o directorio  do segundo ficheiro nao existe\n");
				return -1;
			}
		}


		dprintf("Vou fazer append do directorio %d, o ficheiro %s para o directorio %d, ficheiro %s\n\n",dirOrigem_fh,filenameOrigem,dirDestino_fh,filenameDestino);
		int tstatus = snfs_append(dirOrigem_fh,filenameOrigem,dirDestino_fh,filenameDestino,&res);

		if(tstatus == RES_OK)
			return 1;
		else
			return 0;
}

/*Efecutar do lado do servidor a desfragmentacao do disco associado ao sistema de ficheiros.
 * Devolve 1 se teve sucesso, 0 caso esteja em curso uma operacao de desfragmentacao que ainda nao
 * terminou e -1 caso nao haja disponibilidade de espaco em disco para realizar a desfragmentacao*/
int my_defrag(){
		
	int stat = snfs_defrag();

	if(stat == STAT_OK)
		return 1;
	if(stat == STAT_BUSY)
		return 0;
	return -1;
}

/*Imprime, do lado do servidor, os blocos ocupados do disco com a indicacao do nome dos
 * ficheiros/directorias que estao a utilizar cada um desses blocos.
 * O output desta funcao tera que ser obrigatoriamente o seguinte:
 * 
 * ===== Dump: FileSystem Blocks =======================
 * blk_id: <nº do bloco>
 * file_name1: <pathname do 1º ficheiro ou directoria que usa o bloco>
 * file_name2: ......................
 * *******************************************************
 */
int my_diskusage(){
	snfs_diskusage();
	return 0;
}


/*Imprime do lado do servidor o conteudo da cache de blocos no seguinte formato:
 * 
 * ===== Dump: Cache of Blocks Entries =======================
 * Entry_N: <Nº da Entrada na cache>
 * V: <Bit V de validade>
 * R: <Bit R do metodo NRU> M: <Bit M do metodo NRU>
 * Blk_Cnt: <Conteudo do bloco (em Hexa)>
 * *******************************************************
 */
int my_dumpcache(){
	snfs_dumpcache();
	return 0;
}
