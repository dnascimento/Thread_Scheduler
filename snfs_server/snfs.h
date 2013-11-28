/* 
 * SNFS Logic Layer
 * 
 * snfs.h
 *
 * Interface of the SNFS logic layer containing the functions that manage
 * the internal logic of SNFS (e.g. file handle translation) and 
 * handles the SNFS requests issued by clients.
 * 
 */

#ifndef _SNFS_HANDLERS_H_
#define _SNFS_HANDLERS_H_

#include <snfs_proto.h>


/*
 * the snfs handler type
 * - req: the incoming message (request)
 * - reqsz: the size of the incoming message
 * - res: the outgoing message (response) [out]
 * - ressz: the size of the outgoing message [out]
 */
typedef void (*snfs_handler_t)(snfs_msg_req_t *req, int reqsz,
   snfs_msg_res_t *res, int* ressz);


/*
 * snfs_init: performs internal SNFS initialization; currently
 * argc and argv are not being used.
 */
void snfs_init(int argc, char **argv);


/*
 * SNFS Handlers
 *
 * All handlers follow the above mentioned signature.
 * 
 */


void snfs_ping(snfs_msg_req_t *req, int reqsz, snfs_msg_res_t *res, 
   int* ressz);


void snfs_lookup(snfs_msg_req_t *req, int reqsz, snfs_msg_res_t *res, 
   int* ressz);


void snfs_read(snfs_msg_req_t *req, int reqsz, snfs_msg_res_t *res, 
   int* ressz);


void snfs_write(snfs_msg_req_t *req, int reqsz, snfs_msg_res_t *res, 
   int* ressz);


void snfs_create(snfs_msg_req_t *req, int reqsz, snfs_msg_res_t *res, 
   int* ressz);


void snfs_mkdir(snfs_msg_req_t *req, int reqsz, snfs_msg_res_t *res, 
   int* ressz);


void snfs_readdir(snfs_msg_req_t *req, int reqsz, snfs_msg_res_t *res, 
   int* ressz);


void snfs_remove(snfs_msg_req_t *req, int reqsz, snfs_msg_res_t *res, 
	       int* ressz);
		   
void snfs_copy(snfs_msg_req_t *req, int reqsz, snfs_msg_res_t *res, 
	       int* ressz);		   

void snfs_append(snfs_msg_req_t *req, int reqsz, snfs_msg_res_t *res, 
	       int* ressz);		   
		   
void snfs_defrag(snfs_msg_req_t *req, int reqsz, snfs_msg_res_t *res, 
	       int* ressz);		   
		   
void snfs_diskusage(snfs_msg_req_t *req, int reqsz, snfs_msg_res_t *res, 
	       int* ressz);		   
		   
void snfs_dumpcache(snfs_msg_req_t *req, int reqsz, snfs_msg_res_t *res, 
	       int* ressz);		   
		   
#endif
