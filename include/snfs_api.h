/* 
 * SNFS API Layer
 * 
 * snfs_api.h
 *
 * Interface of the SNFS API layer of the client side SNFS library.
 * 
 * Read the project specification for futher information about
 * the description of the services.
 */

#ifndef _SNFS_API_H_
#define _SNFS_API_H_

#include <snfs_proto.h>


// the file handle of the root directory is '1'
#define ROOT_FHANDLE 1

// status of service invocation
typedef enum {STAT_OK = 0, STAT_BUSY = 1,STAT_ERROR = -1} snfs_call_status_t;


/*
 * snfs_init: internal initialization of the API (e.g. socket).
 * - local_addr - client socket address
 * - remote_addr - server socket address
 *   returns: -1/0 if (un)suceeds
 */
int snfs_init(char* local_addr, char* remote_addr);


/*
 * snfs_ping: dummy service just to ping the server.
 * - inmsg - message to send
 * - insize - size of inmsg
 * - outmsg - received message [out]
 * - outsize - maximum size of buffer outmsg
 *   returns: status
 */
snfs_call_status_t snfs_ping(char* inmsg, int insize, char* outmsg, 
   int outsize);


/*
 * lookup: obtains file handle of file 'name'
 * - name - pathname of the file
 * - file - the file handle [out]
 * - fsize - the file size [out]
 *   returns: status
 */
snfs_call_status_t snfs_lookup(char* pathname, snfs_fhandle_t* file, unsigned* fsize);


/*
 * read: read 'count' bytes from file 'fhandle' starting at 'offset'
 * - fhandle: handle of the file to read
 * - offset: start reading position
 * - count: maximum number of bytes to read
 * - buffer: where to put data [out]
 * - nread: number of bytes read [out]
 *   returns: status
 */
snfs_call_status_t snfs_read(snfs_fhandle_t fhandle, unsigned offset,
   unsigned count, char* buffer, int* nread);


/*
 * write: write 'count' bytes to file 'fhandle' starting at 'offset'
 * - fhandle: handle of the file to write
 * - offset: starting position
 * - count: number of bytes to write
 * - buffer: data to write
 * - fsize: size of file after the write [out]
 *   returns: status
 */
snfs_call_status_t snfs_write(snfs_fhandle_t fhandle, unsigned offset, 
   unsigned count, char* buffer, unsigned int* fsize);


/*
 * create: create file 'name' in directory 'dir'
 * - dir - file handle of the directory
 * - name - name of the file to create
 * - file - the file handle of the created file [out]
 *   returns: status
 */
snfs_call_status_t snfs_create(snfs_fhandle_t dir, char* name, 
   snfs_fhandle_t* file);


/*
 * mkdir: create subdirectory 'name' in directory 'dir'
 * - dir - file handle of the directory
 * - name - name of the subdirectory to create
 * - file - the file handle of the created subdirectory [out]
 *   returns: status
 */
snfs_call_status_t snfs_mkdir(snfs_fhandle_t dir, char* name, 
   snfs_fhandle_t* file);


/*
 * readdir: read the contents of directory 'dir'
 * - dir - file handle of the directory to read
 * - cmax - maximum number of entries that can be read
 * - list - the list of directory entries [out]
 * - count - the number of entries read [out]
 *   returns: status
 */
snfs_call_status_t snfs_readdir(snfs_fhandle_t dir, unsigned cmax, 
   snfs_dir_entry_t* list, unsigned* count);

/*
 * delete: deletes file 'name' on directory 'dir' 
 * - dir - file handle of the directory
 * - name - name of file bo deleted  
 * - file - the file handle of the deleted file [out]
 *   returns: status
 */
snfs_call_status_t snfs_remove(snfs_fhandle_t dir, char* name, snfs_fhandle_t* file);

   
/*
 * copy: copies file 'src_name' on directory 'src_dir' to 
 * file 'dst_name' on directory 'dst_dir'
 * - src_dir - file handle of the source directory
 * - src_name - name of the source file to be copied 
 * - dst_dir - file handle of the destination directory
 * - dst_name - name of the destination file
 * - file - the file handle of the destination file [out]
 *   returns: status
 */
snfs_call_status_t snfs_copy(snfs_fhandle_t src_dir, char* src_name, 
   snfs_fhandle_t dst_dir, char* dst_name, snfs_fhandle_t* file);

/*
 * append: appends file 'name2' on directory 'dir2' to 
 * file 'name1' on directory 'dir1'
 * - dir1 - file handle of the destination directory
 * - name1 - name of the destination file  
 * - dir2 - file handle of the directory that contains the file to be appended
 * - name2 - name of the file to be appended
 * - fsize - size of file after the write [out]
 *   returns: status
 */
snfs_call_status_t snfs_append(snfs_fhandle_t dir1, char* name1, snfs_fhandle_t dir2, char* name2, unsigned int* fsize);

/*
 * defragmentation: operates the file system block defragmentation 
 *   returns: status
 */
snfs_call_status_t snfs_defrag();

/*
 * diskusage: dumps the file system busy blocks along with the
 * name of the files that are using them. This dumping operation takes place on the server side.
 *   returns: status
 */
snfs_call_status_t snfs_diskusage();

/*
 * dumpcache: dumps the cache of blocks content. This dumping operation takes place on the server side.
 *   returns: status
 */
snfs_call_status_t snfs_dumpcache();



/*
 * snfs_finish: internal finalization of the SNFS API
 */

void snfs_finish();


#endif
