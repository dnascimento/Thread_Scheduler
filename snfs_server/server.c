/* 
 * SNFS Server
 * 
 * snfs_server.c
 *
 * Server implementing SNFS services. Current implementation is
 * multi-threaded.
 * 
 * Read the project specification for futher information.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/select.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sthread.h>
#ifdef USE_PTHREADS
#include <pthread.h>
#endif

// SNFS includes
#include <snfs_proto.h>
#include "snfs.h"


#ifndef SERVER_SOCK
#define SERVER_SOCK "/tmp/server.socket"
#endif

// request descriptor structure
struct _req {
	snfs_msg_req_t req;
	struct sockaddr_un cliaddr;
	int reqsz;
	socklen_t clilen;
};
typedef struct _req* req_t;


/*
 * SNFS Services
 *
 * all services are registered in a global table which is
 * used during request resolution to find the handler that
 * serves the request
 *
 * service handlers are implemented in snfs.c
 */

#define NUM_REQ_HANDLERS 13
#define NUM_TC 5		// max number of active threads
#define RING_SIZE 10


static sthread_mon_t mon = NULL;
static int available_reqs; // buffer requests not yet consumed 
req_t ring[RING_SIZE];
int sockfd;

struct {
  snfs_msg_type_t type;
  snfs_handler_t handler;
} Service[NUM_REQ_HANDLERS] = {
  {REQ_PING, snfs_ping},
  {REQ_LOOKUP, snfs_lookup},
  {REQ_READ, snfs_read},
  {REQ_WRITE, snfs_write},
  {REQ_CREATE, snfs_create},
  {REQ_MKDIR, snfs_mkdir},
  {REQ_READDIR, snfs_readdir},
  {REQ_REMOVE, snfs_remove},
  {REQ_COPY, snfs_copy},
  {REQ_APPEND, snfs_append},
  {REQ_DEFRAG, snfs_defrag},
  {REQ_DISKUSAGE, snfs_diskusage},
  {REQ_DUMPCACHE, snfs_dumpcache}
};

/*
 * Buffer management functions
 */

req_t get_req() {
	static short int pos = 0;
	
	req_t temp = ring[pos];
	pos = (pos + 1) % RING_SIZE;
	
	return temp;
}

void put_req(req_t req) {
	static short int pos = 0;
	
	ring[pos] = req;
	pos = (pos + 1) % RING_SIZE;
	
	return;
}

int my_recvfrom(snfs_msg_req_t* req, struct sockaddr_un* cliaddr, socklen_t* clilen) {
	int reqsz;
	
	*clilen = sizeof(*cliaddr);
	
	do {
		sthread_yield();
		errno = 0;
		reqsz = recvfrom(sockfd, (void*)req, sizeof(*req), MSG_DONTWAIT,
							(struct sockaddr *)cliaddr, clilen);
	} while(errno == EAGAIN);
	
	return reqsz;
}


/* receives a Server Id, from the main function argv */

void srv_init_socket(struct sockaddr_un* servaddr)
{	
   	// creates socket datagram domain unix
	if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0){
		printf("[snfs_srv] socket error: %s.\n", strerror(errno));
		exit(-1);
	}

   	// structure address cleaning
	bzero(servaddr, sizeof(*servaddr));
	servaddr->sun_family = AF_UNIX;
	strcpy(servaddr->sun_path, SERVER_SOCK);

	// if exists, deletes socket file name
	if (unlink(servaddr->sun_path) < 0 && errno != ENOENT) {
		printf("[snfs_srv] unlink error: %s.\n", strerror(errno));
		exit(-1);
	}

   	// binds socket to address
	if (bind(sockfd, (struct sockaddr *) servaddr, sizeof(*servaddr)) < 0){
		printf("[snfs_srv] unbind error: %s.\n", strerror(errno));
		exit(-1);
	}
	
	printf("Server is running...\n");
}


int srv_recv_request(snfs_msg_req_t* req, struct sockaddr_un* cliaddr, socklen_t* clilen)
{
	int status = my_recvfrom(req, cliaddr, clilen);
	
	if (status == 0) {
		printf("[snfs_srv] request error.\n");
		return 0;
	}
	if (status < 0) {
		printf("[snfs_srv] recvfrom error: %s.\n", strerror(errno));
		exit(-1);
	}
	
	return status;
}


void srv_send_response(snfs_msg_res_t* res, int ressz, struct sockaddr_un* cliaddr, socklen_t clilen)
{
	int status = sendto(sockfd, res, ressz, 0, (struct sockaddr *)cliaddr, clilen);
	if (status < 0) {
		printf("[snfs_srv] sendto error: %s.\n", strerror(errno));
	}
	if (status != ressz) {
		printf("[snfs_srv] message size mismatch.\n");
	}
}

/*
* SNFS request handler thread
*/

void* thread_consumer() {
	req_t req_d;
	int ressz, req_i;
	snfs_msg_res_t res;
	
	while(1) {
		sthread_monitor_enter(mon);
		// get request from queue
		while (!available_reqs) sthread_monitor_wait(mon);
		
		req_d = get_req();
		available_reqs--;
		sthread_monitor_signal(mon); 
		sthread_monitor_exit(mon); 

		
		// clean response
		memset(&res,0,sizeof(res));
		
		// find request handler
		req_i = -1;
		for (int i = 0; i < NUM_REQ_HANDLERS; i++) {
			if (req_d->req.type == Service[i].type) {
				req_i = i;
				break;
			}
		}

      		// serve the request
		if (req_i == -1) {
			res.status = RES_UNKNOWN;
			ressz = sizeof(res) - sizeof(res.body);
			printf("[snfs_srv] unknown request.\n");
		} else {
			Service[req_i].handler(&(req_d->req),req_d->reqsz,&res,&ressz);
		}

      		// send response to client
		srv_send_response(&res,ressz,&(req_d->cliaddr),req_d->clilen);
		
		// free stuff
		free(req_d); req_d = NULL;
		
		// force request processing
		sthread_yield();
	}
}


/*
* SNFS request receiver thread
*/

void* thread_producer() 
{
	req_t req_d;
	
	while(1) 
	{
		// wait for a free buffer slot
		sthread_monitor_enter(mon);
		while (available_reqs == RING_SIZE) sthread_monitor_wait(mon);
		sthread_monitor_exit(mon); 

		// create and clean request
		req_d = (req_t) malloc(sizeof(struct _req));
		memset(req_d,0,sizeof(struct _req));

		if ((req_d->reqsz = srv_recv_request(&(req_d->req),&(req_d->cliaddr),&(req_d->clilen))) == 0) 
			continue;
		
		sthread_monitor_enter(mon); 
		// send to buffer
		put_req(req_d);
		available_reqs++;
		sthread_monitor_signalall(mon);
		sthread_monitor_exit(mon);
		sthread_yield();
	}
}

/*
* SNFS server main
*/
/*
* SNFS server main
*/

int main(int argc, char **argv)
{
	sthread_t threads[NUM_TC];
	sthread_t prodthr;
	int i;
	available_reqs = 0;
	
	// initialize sthread lib	
	sthread_init();
	
	//initialize filesystem
	snfs_init(argc, argv);
		
   	// initialize SNFS layer
       struct sockaddr_un servaddr;
                	
	// initialize communications
	srv_init_socket(&servaddr);
			
	// initialize  monitor
        mon = sthread_monitor_init();
        
	// create thread_consumer threads
	for(i = 0; i < NUM_TC; i++) {
		threads[i] = sthread_create(thread_consumer, (void*) NULL,1);
		if (threads[i] == NULL) {
			printf("Error while creating threads. Terminating...\n");
			exit(-1);
		}
	}
	
	// create producer thread
	prodthr = sthread_create(thread_producer, (void*) NULL,1);
	
	
	sthread_join(prodthr, (void**)NULL);
	for(i = 0; i < NUM_TC; i++)
		sthread_join(threads[i], (void **)NULL);

	return 0;
}

