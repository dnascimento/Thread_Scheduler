/* 
 * File System Layer
 * 
 * fs.h
 *
 * Interface to the file system layer implemented at the server side.
 * The following interface functions are provided.
 * 
 * Manages the internal organization of files and directories in a 'virtual
 * memory disk' and provides the following interface functions to programmers.
 * 
 */

#ifndef _FS_H_
#define _FS_H_

#include "cache.h"
#include "block.h"
#include "list.h"

// maximum space for the file name (13 chars + '\0')
#define FS_MAX_FNAME_SZ 14

// maximum size of a file name used in messages
#define MAX_PATH_NAME_SIZE 200

// type of the inode: directory or file
typedef enum {FS_DIR = 1, FS_FILE = 2} fs_itype_t;


// type of inode identifier
typedef unsigned short inodeid_t;


// attributes of a file
typedef struct {
   inodeid_t inodeid;  // inode number
   fs_itype_t type;    // directory or file
   unsigned size;      // total size in bytes
   int num_entries;    // number of entries if it is a directory
} fs_file_attrs_t;


// identify the name and the type of a file
typedef struct {
   char name[FS_MAX_FNAME_SZ];
   fs_itype_t type;
} fs_file_name_t;


// file system structure (the implementation is hidden)
typedef struct fs_ fs_t;


//Estrutura para guardar as referencias de cada bloco
typedef struct no{
	unsigned blockId;
	inodeid_t inodeId;
	char* pathname;
}noRef_;
typedef noRef_ *noRef_t;



/*
 * fs_new: allocates storage - blocks - and memory for the fs structure
 * - num_blocks - number of blocks
 *   returns: the fs structure
 */
fs_t* fs_new(unsigned num_blocks, int disk_delay);


/*
 * fs_format: formats the file system
 * - fs: reference to file system
 *   returns: 0 if successful, -1 otherwise
 */
int fs_format(fs_t* fs);


/*
 * fs_lookup: gets the inode id of an object (file/directory)
 * - fs: reference to file system
 * - file: the name of the object
 * - fileid: the inode id of the object [out]
 *   returns: 0 if successful,  -1 otherwise
 */
int fs_lookup(fs_t* fs,  char* file, inodeid_t* fileid);


/*
 * fs_get_attrs: gets the attributes of an object (file/directory)
 * - fs: reference to file system
 * - file: node id of the object
 * - attrs: the attributes of the object [out]
 *   returns: 0 if successful, -1 otherwise
 */
int fs_get_attrs(fs_t* fs, inodeid_t file, fs_file_attrs_t* attrs);


/*
 * fs_read: read the contents of a file
 * - fs: reference to file system
 * - file: node id of the file
 * - offset: starting position for reading
 * - count: number of bytes to read
 * - buffer: where to put the data [out]
 * - nread: number of bytes effectively read [out]
 *   returns: 0 if successful, -1 otherwise
 */
int fs_read(fs_t* fs, inodeid_t file, unsigned offset, unsigned count, 
   char* buffer, int* nread);


/*
 * fs_write: write data to file
 * - fs: reference to file system
 * - file: node id of the file
 * - offset: starting position for writing
 * - count: number of bytes to write
 * - buffer: the data to write
 *   returns: 0 if successful, -1 otherwise (the write operation is atomic)
 */
int fs_write(fs_t* fs, inodeid_t file, unsigned offset, unsigned count,
   char* buffer);


/*
 * fs_create: create a file in a specified directory
 * - fs: reference to file system
 * - dir: the directory where to create the file
 * - file: the name of the file
 * - fileid: the inode id of the file [out]
 *   returns: 0 if successful, -1 otherwise
 */
int fs_create(fs_t* fs, inodeid_t dir, char* file, inodeid_t* fileid);


/*
 * fs_mkdir: create a subdirectory in a specified directory
 * - fs: reference to file system
 * - dir: the directory where to create the file
 * - newdir: the name of the new subdirectory
 * - newdirid: the inode id of the subdirectory [out]
 *   returns: 0 if successful, -1 otherwise
 */
int fs_mkdir(fs_t* fs, inodeid_t dir, char* newdir, inodeid_t* newdirid);


/*
 * fs_readdir: read the contents of a directory
 * - fs: reference to file system
 * - dir: the directory
 * - entries: where to write the entries of the directory [out]
 * - maxentries: maximum number of entries to write in 'entries'
 * - numentries: number of entries written [out]
 *   returns: 0 if successful, -1 otherwise
 */
int fs_readdir(fs_t* fs, inodeid_t dir, fs_file_name_t* entries, int maxentries,
   int* numentries);


int fs_remove(fs_t* fs, inodeid_t directorio, char* nomeFicheiro, inodeid_t* fileHandler);

int fs_append(fs_t* fs, inodeid_t dirOrigem, char* nomeOrigem,inodeid_t dirDestino,char* nomeDestino, inodeid_t* fileHandler,unsigned* fsize);

int fs_copy(fs_t* fs, inodeid_t dirOrigem, char* nomeOrigem,inodeid_t dirDestino,char* nomeDestino, inodeid_t* fileHandler);


/*
 * fd_dump: dump the contents of a file system
 */
void fs_dump(fs_t* fs);

void imprimirInodeTab(fs_t* fs);	//para debug



int fs_defrag(fs_t* X);



/**Realizar o dump aos blocos do disco*/
int fs_diskUsage(fs_t* fs);

int fs_dumpcache(fs_t* fs);


void* thread_actualiza_Cache(void* sistemaFicheiros);
#endif
