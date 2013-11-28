/* 
 * File System Interface
 * 
 * myfs.h
 *
 * SNFS programming interface simulating the standard Unix I/O interface.
 * 
 */

#ifndef _MYFS_H_
#define _MYFS_H_

struct _file_desc {
	int fileId;
	unsigned size;
	int read_offset;
	int write_offset;
};
typedef struct _file_desc* fd_t;


/*
 * open flags:
 * - O_CERATE: create the file if it does not exist
 */
enum my_open_flags { O_CREATE = 1 };


/*
 * my_init_lib: internal initialization
 */
int my_init_lib();


/*
 * my_open: open the file 'nome'
 * - nome: absolute path of the file to open (e.g. "/f1" refers to
 *   file 'f1' in the root directory)
 * - flags: open flags
 *   returns: the internal file id of the opened file of -1 if error
 */
int my_open(char* nome, int flags);


/*
 * my_read: read data from file
 * - fileId: the file id of the opened file
 * - buffer: where to put the read data [out]
 * - numBytes: maximum number of bytes to read
 *   returns: the number of bytes read, 0 if end of file reached or -1 if error
 */
int my_read(int fileId, char* buffer, unsigned numBytes);


/*
 * my_write: write data to file
 * - fileId: the file id of the opened file
 * - buffer: buffer with the data to write
 * - numBytes: number of bytes to write
 *   returns: the number of bytes written or -1 if error
 */
int my_write(int fileId, char* buffer, unsigned numBytes);


/*
 * my_close: close a previously opened file
 * - fileId: the file id of the file to close
 *   returns: 0 if successful or -1 if error
 */
int my_close(int fileId);


/*
 * my_listdir: list the content of directory 'path'
 * - path: the absolute directory path (e.g. "/", "/d1", "/d1/sd1")
 * - filenames: memory were to write the filenames, each separated 
 *   by '\0', the memory must be freed by the caller [out]
 * - numFiles: number of files written into *filenames [out]
 *   returns: 0 if successful, -1 if error
 */
int my_listdir(char* path, char **filenames, int* numFiles);

int my_mkdir(char* dirname);

/*Apaga o ficheiro ou directorio identificado por name.
 *  Devolve 0 se teve sucesso, negativo caso contrario.*/
int my_remove(char *name);

/*COpiar um ficheiro ou directorio identificado por name1 para name2.
 * Devolve 1 se teve sucesso, 0 caso contrario*/
int my_copy(char *name1, char *name2);



/*Concatena ao ficheiro name1 o conteudo do ficheiro name2. Devolve 1 se teve sucesso, 0 caso contrario.
 * Em caso de sucesso, apenas o conteudo name1 se altera*/
int my_append(char *name1, char *name2);



/*Efecutar do lado do servidor a desfragmentacao do disco associado ao sistema de ficheiros.
 * Devolve 1 se teve sucesso, 0 caso esteja em curso uma operacao de desfragmentacao que ainda nao
 * terminou e -1 caso nao haja disponibilidade de espaco em disco para realizar a desfragmentacao*/
int my_defrag();


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
int my_diskusage();




/*Imprime do lado do servidor o conteudo da cache de blocos no seguinte formato:
 * 
 * ===== Dump: Cache of Blocks Entries =======================
 * Entry_N: <Nº da Entrada na cache>
 * V: <Bit V de validade>
 * R: <Bit R do metodo NRU> M: <Bit M do metodo NRU>
 * Blk_Cnt: <Conteudo do bloco (em Hexa)>
 * *******************************************************
 */
int my_dumpcache();


#endif 
