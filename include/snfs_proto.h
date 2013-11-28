/* 
 * SNFS Protocol
 * 
 * snfs_proto.h
 *
 * SNFS protocol specification used both by the SNFS library and 
 * the SNFS server. Namely it describes the format of the messages
 * exchanged between client-server: the request and the response
 * messages.
 * 
 * Read the project specification for futher information about
 * the description of the services.
 */

#ifndef _SNFS_PROTO_H_
#define _SNFS_PROTO_H_


/*
 * SNFS Protocol Types and Macros
 */


// maximum size of a file name used in messages
#define MAX_FILE_NAME_SIZE 14

// maximum size of a file name used in messages
#define MAX_PATH_NAME_SIZE 200


// maximum size of data read in one single message
#define MAX_READ_DATA (1024)


// maximum amount of data written sent in one single message
#define MAX_WRITE_DATA (1024)


// maximum amount of directory entries sent in one single message
#define MAX_READDIR_ENTRIES 64


// file handle describing a remote directory/file
typedef int snfs_fhandle_t;


// file type in a directory entry
typedef enum {SNFS_DIR = 1, SNFS_FILE = 2} snfs_dir_entry_type_t;


// directory entry of a directory listing
typedef struct {
   char name[MAX_FILE_NAME_SIZE];
   int len;
   snfs_dir_entry_type_t type;
} snfs_dir_entry_t;


/*
 * SNFS Message Codes
 *  - message identifier - snfs_msg_type_t
 *  - status returned by server - snfs_msg_res_status_t
 */

typedef enum {
   REQ_NULL = 0,
   REQ_PING = 1,
   REQ_LOOKUP = 2,
   REQ_READ = 3,
   REQ_WRITE = 4,
   REQ_CREATE = 5,
   REQ_MKDIR = 6,
   REQ_READDIR = 7,
   REQ_COPY = 8,
   REQ_REMOVE = 9,
   REQ_APPEND = 10,
   REQ_DEFRAG = 11,
   REQ_DISKUSAGE = 12,
   REQ_DUMPCACHE = 13   
} snfs_msg_type_t;

typedef int snfs_req_serial_num_t;

typedef enum {
   RES_OK = 0,
   RES_ERROR = -1,
   RES_UNKNOWN = -2
} snfs_msg_res_status_t;


/*
 * SNFS Ping
 *   - request message: snfs_msg_req_ping_t
 *   - response message: snfs_msg_res_ping_t
 */


typedef struct {
   char msg[256];
} snfs_msg_req_ping_t;


typedef struct {
   char msg[256];
} snfs_msg_res_ping_t;


/*
 * SNFS Lookup
 *   - request message: snfs_msg_req_lookup_t
 *   - response message: snfs_msg_res_lookup_t
 */


typedef struct {
   char pname[MAX_PATH_NAME_SIZE];
} snfs_msg_req_lookup_t;


typedef struct {
   snfs_fhandle_t file;
   unsigned fsize;
} snfs_msg_res_lookup_t;


/*
 * SNFS Read
 *   - request message: snfs_msg_req_read_t
 *   - response message: snfs_msg_res_read_t
 */


typedef struct {
   snfs_fhandle_t fhandle;
   unsigned offset;
   unsigned count;
} snfs_msg_req_read_t;


typedef struct {
   char data[MAX_READ_DATA];
   unsigned nread;
} snfs_msg_res_read_t;


/*
 * SNFS Write
 *   - request message: snfs_msg_req_write_t
 *   - response message: snfs_msg_res_write_t
 */


typedef struct {
   snfs_fhandle_t fhandle;
   unsigned offset;
   unsigned count;
   char data[MAX_WRITE_DATA];
} snfs_msg_req_write_t;


typedef struct {
   unsigned fsize;
} snfs_msg_res_write_t;


/*
 * SNFS Create
 *   - request message: snfs_msg_req_create_t
 *   - response message: snfs_msg_res_create_t
 */


typedef struct {
   snfs_fhandle_t dir;
   char name[MAX_FILE_NAME_SIZE];
} snfs_msg_req_create_t;


typedef struct {
   snfs_fhandle_t file;
} snfs_msg_res_create_t;


/*
 * SNFS Mkdir
 *   - request message: snfs_msg_req_mkdir_t
 *   - response message: snfs_msg_res_mkdir_t
 */


typedef struct {
   snfs_fhandle_t dir;
   char file[MAX_FILE_NAME_SIZE];
} snfs_msg_req_mkdir_t;


typedef struct {
   snfs_fhandle_t newdirid;
} snfs_msg_res_mkdir_t;


/*
 * SNFS Readdir
 *   - request message: snfs_msg_req_readdir_t
 *   - response message: snfs_msg_res_readdir_t
 */


typedef struct {
   snfs_fhandle_t dir;
   unsigned cmax;
} snfs_msg_req_readdir_t;


typedef struct {
   unsigned count;
   snfs_dir_entry_t list[MAX_READDIR_ENTRIES];
} snfs_msg_res_readdir_t;

/*
 * SNFS Remove
 *   - request message: snfs_msg_req_remove_t
 *   - response message: snfs_msg_res_remove_t
 */


typedef struct {
  snfs_fhandle_t dir;
  char name[MAX_FILE_NAME_SIZE];
} snfs_msg_req_remove_t;

typedef struct {
     snfs_fhandle_t file;
} snfs_msg_res_remove_t;


/*
 * SNFS Copy
 *   - request message: snfs_msg_req_copy_t
 *   - response message: snfs_msg_res_copy_t
 */


typedef struct {
  snfs_fhandle_t src_dir;
  snfs_fhandle_t dst_dir;
  char src_name[MAX_FILE_NAME_SIZE];
  char dst_name[MAX_FILE_NAME_SIZE];
} snfs_msg_req_copy_t;

typedef struct {
     snfs_fhandle_t file;
} snfs_msg_res_copy_t;


/*
 * SNFS Append
 *   - request message: snfs_msg_req_append_t
 *   - response message: snfs_msg_res_append_t
 */


typedef struct {
  snfs_fhandle_t dir1;
  snfs_fhandle_t dir2;
  char name1[MAX_FILE_NAME_SIZE];
  char name2[MAX_FILE_NAME_SIZE];
} snfs_msg_req_append_t;


typedef struct {
   unsigned fsize;
} snfs_msg_res_append_t;



/*
 * SNFS Messages
 *
 * Messages have a common field ('type') indicating the type of
 * service that the request/response refers to; the union contains
 * all the possible message formats both referring to requests and
 * responses. Response messages have also the 'status' field
 * indicating if the service succeeded.
 * 
 *   - request message: snfs_msg_req_t
 *   - response message: snfs_msg_res_t
 */

typedef struct {
  snfs_msg_type_t type;
  union {
    snfs_msg_req_ping_t ping;
    snfs_msg_req_lookup_t lookup;
    snfs_msg_req_read_t read;
    snfs_msg_req_write_t write;
    snfs_msg_req_create_t create;
    snfs_msg_req_mkdir_t mkdir;
    snfs_msg_req_readdir_t readdir;
    snfs_msg_req_remove_t remove;
	snfs_msg_req_copy_t copy;
	snfs_msg_req_append_t append;	
  } body;
} snfs_msg_req_t;

typedef struct {
   snfs_msg_type_t type;
   snfs_msg_res_status_t status;
   union {
      snfs_msg_res_ping_t ping;
      snfs_msg_res_lookup_t lookup;
      snfs_msg_res_read_t read;
      snfs_msg_res_write_t write;
      snfs_msg_res_create_t create;
      snfs_msg_res_mkdir_t mkdir;
      snfs_msg_res_readdir_t readdir;
	  snfs_msg_res_remove_t remove;
	  snfs_msg_res_copy_t copy;
	  snfs_msg_res_append_t append;	
   } body;
} snfs_msg_res_t;


#endif
