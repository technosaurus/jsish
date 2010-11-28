#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "error.h"
#include "mempool.h"

#ifndef DONT_USE_POOL
static void pool_extern(mpool_t mp)
{
	int size = sizeof(Memblock) + ((mp->elemsize + sizeof(Memnode)) * MP_BLOCK_SIZE);
	struct Memblock *mb = malloc(size);
	if (!mb) die("Out of memory\n");
	
	/* push to block head */
	mb->next = mp->blockhead;
	mp->blockhead = mb;
	
	char *p = (char *)mb;				/* raw byte proccess */
	p += (int)sizeof(Memblock);			/* p pointed to (Memnode+elesize) * BLOCKSIZE */

	int i;
	for (i = 0; i < MP_BLOCK_SIZE; ++i) {
		Memnode *n = (Memnode *)p;
		n->esize = mp->elemsize;
		
		n->next = mp->nodehead;
		mp->nodehead = n;

		p += (mp->elemsize + sizeof(Memnode));
	}
}
#endif

mpool_t mpool_create(unsigned int elemsize)
{
	mpool_t mp = malloc(sizeof(struct Mempool));
	if (!mp) die("Out of memory\n");
	
	memset(mp, 0, sizeof(struct Mempool));

	if (elemsize % 4 != 0)
		bug("Problem, create a non-align mempool, failed");

	mp->elemsize = elemsize;

#ifndef DONT_USE_POOL
	pool_extern(mp);
#endif

	return mp;
}

void *mpool_alloc(mpool_t mp)
{
#ifndef DONT_USE_POOL
	if (!mp->nodehead) 		/* running out of cache */
		pool_extern(mp);

	char *ret = (char *)mp->nodehead;
	mp->nodehead = mp->nodehead->next;

	return (void *)(ret + (int)sizeof(Memnode));
#else
	return malloc(mp->elemsize);
#endif
}

void mpool_free(void *p, mpool_t mp)
{
#ifndef DONT_USE_POOL
	Memnode *n = (Memnode *)(((char *)p) - sizeof(Memnode));
	if (n->esize != mp->elemsize) {
		bug("Release an (%d)sized memory into (%d)sized pool",
			n->esize, mp->elemsize);
	}
	n->next = mp->nodehead;
	mp->nodehead = n;
#else
	free(p);
#endif
}

#ifndef DONT_USE_POOL
/* Create pools to replace malloc */
#define POOL_COUNT	8

static mpool_t general_pools[POOL_COUNT];
static unsigned int gpools_sizes[POOL_COUNT] = {
	8, 12, 16, 24, 32, 64, 128, 256
};

static int sizeindexes[129] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 0 - 8 */
	1, 1, 1, 1,					/* 9 - 12 */
	2, 2, 2, 2,					/* 13 - 16 */
	3, 3, 3, 3, 3, 3, 3, 3,		/* 17 - 24 */
	4, 4, 4, 4, 4, 4, 4, 4,		/* 25 - 32 */
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,	/* 33 - 64 */
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6  /* 65 - 128 */
};

#define roundsize(n) (((n)<=128)?sizeindexes[n]:(((n)>256)?-1:7))
#endif

void mpool_init()
{
#ifndef DONT_USE_POOL
	int i;
	for (i = 0; i < POOL_COUNT; ++i) {
		if (gpools_sizes[i]) {
			general_pools[i] = mpool_create(gpools_sizes[i]);
		}
	}
#endif
}

void *mm_alloc(unsigned int size)	/* size_t ... */
{
#ifndef DONT_USE_POOL
	int poolindex = roundsize(size);
	if (poolindex >= 0) {
		return mpool_alloc(general_pools[poolindex]);
	}
	
	/* block that bigger then 256 bytes, use malloc */
	/* still add a Memnode struct before the allocated memory */
	Memnode *n = malloc(size + sizeof(Memnode));
	n->esize = size;

	n++;
	return n;
#else
	return malloc(size);
#endif
}

void mm_free(void *p)
{
#ifndef DONT_USE_POOL
	Memnode *n = (Memnode *)(((char *)p) - sizeof(Memnode));
	int poolindex = roundsize(n->esize);
	if (poolindex >= 0) {
		mpool_free(p, general_pools[poolindex]);
		return;
	}

	/* more then 256 bytes, use free */
	free(n);
#else
	free(p);
#endif
}

