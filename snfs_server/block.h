/* 
 * Storage Layer
 * 
 * block.h
 *
 * Interface to the storage layer which offers the abstraction
 * of a sequence of blocks of fixed size.
 * 
 */

#ifndef _BLOCK_H_
#define _BLOCK_H_


/*
 * blocks_t: the storage abstraction of a virtual disk
 * the implementation is hidden
 */
typedef struct blocks_ blocks_t;


/*
 * block_new: create a blocks instance
 * - block_sz: the size of blocks
 * - num_blocks: number of blocks
 *   returns: the blocks instance
 */
blocks_t* block_new(unsigned block_sz, unsigned num_blocks);


/*
 * block_free: free the blocks
 * - bks - the blocks to free
 */
void block_free(blocks_t* bks);


/*
 * block_size: get the size of each block
 */
unsigned block_size(blocks_t* bks);


/*
 * block_num_blocks: get the total number of blocks
 */
unsigned block_num_blocks(blocks_t* bks);


/*
 * block_read: read a whole block
 * - bks: the blocks instance
 * - block_no: the number of the block to read
 * - block: the buffer were to copy the block [out]
 *   returns: 0 if sucessful, -1 if not
 */
int block_read(blocks_t* bks, unsigned block_no, char* block);


/*
 * block_write: write a whole block
 * - bks: the blocks instance
 * - block_no: the number of the block to write
 * - block: the data with the data to write
 *   returns: 0 if sucessful, -1 if not
 */
int block_write(blocks_t* bks, unsigned block_no, char* block);


/*
 * block_load: load an image of blocks from a file
 * - file: the name of the file
 *   returns: the blocks instance
 */
blocks_t* block_load(char* file);


/*
 * block_store: store an image of blocks to a file
 * - bks - the blocks instance
 * - file: the name of the file
 *   returns: 0 if sucessful, -1 if not
 */
int block_store(blocks_t* bks, char* file);


/*
 * block_dump: dumps the content of blocks
 * - bks - the blocks instance
 */
void block_dump(blocks_t* bks);


#endif
