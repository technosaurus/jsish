#ifndef __MEM_POOL_H__
#define __MEM_POOL_H__

/* 
 * struct relationship:
 * |------------------------MemBlock------------------------|
 * |                        ________        __________      |
 * |                 next* /        \      /          \     |
 * |struct Memblock|Memnode|UserData|Memnode|UserData|...   |
 *         next* |         /\
 *               |          Return to user
 * |struct Memblock|
 *              ...
 *
 * define DONT_USE_POOL to disable mempool and use native c mem funcs
 * Note:
 *    all allocated mem from pool is not zero filled
 */

/* how many node in one block */
#define MP_BLOCK_SIZE	1024

/* per node size is (sizeof(Memnode) + mp->elemsize); */
typedef struct Memnode {
	struct Memnode *next;
	unsigned int esize;
} Memnode;

typedef struct Memblock {
	struct Memblock *next;
} Memblock;

typedef struct Mempool {
	unsigned int elemsize;

	Memblock *blockhead;
	Memnode *nodehead;
} *mpool_t;

void mpool_init();

/* create a pool, with size and initial element count */
/* return pool handle */
mpool_t mpool_create(unsigned int elemsize);

/* allocate an element from pool */
/* same as malloc */
void *mpool_alloc(mpool_t mpool);

/* free an element to pool */
/* same as free */
void mpool_free(void *p, mpool_t mpool);

/* general malloc/free replacement
 * cutting down calling times of malloc and free,
 * but some times slower then native malloc/free, 
 * don't know why, if so
 * simply USE DONT_USE_POOL to disable mempool
 */
void *mm_alloc(unsigned int size);
void mm_free(void *p);

#endif
