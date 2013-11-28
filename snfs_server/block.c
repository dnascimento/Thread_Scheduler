/* 
 * Storage Layer
 * 
 * block.c
 *
 * Storage layer which offers the abstraction of a sequence of 
 * blocks of fixed size. Blocks are kept in memory.
 * 
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "block.h"


void io_delay_read_block();
void io_delay_write_block();

// internal implementation of 'blocks_t' 
struct blocks_ {
   unsigned block_size;
   unsigned num_blocks;
   char blocks[0];
};


blocks_t* block_new(unsigned num_blocks, unsigned block_sz)
{
   if (num_blocks * block_sz == 0) {
      return NULL;
   }
   blocks_t* bks = (blocks_t*) 
      malloc(sizeof(blocks_t) + num_blocks * block_sz);
   bks->block_size = block_sz;
   bks->num_blocks = num_blocks;
   memset(&bks->blocks[0], 0, num_blocks * block_sz);
   return bks;
}


void block_free(blocks_t* bks)
{
   free(bks);
}


unsigned block_size(blocks_t* bks)
{
   return bks->block_size;
}


unsigned block_num_blocks(blocks_t* bks)
{
   return bks->num_blocks;
}


int block_read(blocks_t* bks, unsigned block_no, char* block)
{
   if (block_no >= bks->num_blocks) {
	  return -1;
   }
 
   io_delay_read_block();
   char* ptr = &bks->blocks[block_no * bks->block_size]; 
   memcpy(block,ptr,bks->block_size);
   return 0;
}


int block_write(blocks_t* bks, unsigned block_no, char* block)
{
   if (block_no >= bks->num_blocks) {
	  return -1;
   }


   io_delay_write_block();
   char* ptr = &bks->blocks[block_no * bks->block_size]; 
   memcpy(ptr,block,bks->block_size);
   return 0;
}


blocks_t* block_load(char* file)
{
   if (file == NULL) {
      return NULL;
   }

   int fd = open(file, O_RDONLY);
   if (fd < 0) {
      return NULL;
   }

   int status = 0;

   unsigned block_size;
   status = read(fd,&block_size,sizeof(block_size));
   if (status != sizeof(block_size)) {
      close(fd);
      return NULL;
   }

   unsigned num_blocks;
   status = read(fd,&num_blocks,sizeof(num_blocks));
   if (status != sizeof(num_blocks)) {
      close(fd);
      return NULL;
   }

   blocks_t* bks = (blocks_t*)
      malloc(sizeof(blocks_t) + num_blocks * block_size);
   status = read(fd, bks->blocks, num_blocks * block_size);
   if (status != num_blocks * block_size) {
      close(fd);
      free(bks);
      return NULL;
   }
   bks->block_size = block_size;
   bks->num_blocks = num_blocks;
   return bks;
}


int block_store(blocks_t* bks, char* file)
{
   if (bks == NULL && file == NULL) {
      return -1;
   }

   int fd = open(file, O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR);
   if (fd < 0) {
      return -1;
   }

   unsigned size = sizeof(blocks_t) + bks->block_size * bks->num_blocks;

   int status = write(fd, (char*)bks, size);
   if (status != size) {
      close(fd);
      return -1;
   }

   close(fd);
   return 0;
}


void block_dump(blocks_t* bks)
{
   printf("Blocks:\n");
   printf("- Block size: %u\n", bks->block_size);
   printf("- Num blocks: %u\n", bks->num_blocks);
}
