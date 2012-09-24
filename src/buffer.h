#ifndef _BUFFER_H_
#define _BUFFER_H_

#include "arch/sysdep.h"
#include "configuration.h"

/* 64bytes is the common size of a cache line */
#define longxCacheLine  (64/sizeof(long))

/* Padding is required to avoid false-sharing
   between core's private cache */

#ifdef _USE_BOUNDED_BUFFER_
struct buffer_t {
	unsigned long pread;
	//  volatile unsigned long pread;
	long padding1[longxCacheLine-1];
	unsigned long pwrite;
	//  volatile unsigned long pwrite;
	long padding2[longxCacheLine-1];
	unsigned long size;
	void **data;
};

#endif		/* _USE_BOUNDED_BUFFER_ */



#ifdef _USE_UNBOUNDED_BUFFER_
typedef struct entry entry;
struct entry {
	void *data;
	entry *next;
};

struct buffer_t{
	entry *head;
	entry *tail;
	int count;
};

#endif	/* _USE_UNBOUNDED_BUFFER_ */


typedef struct buffer_t buffer_t;

void  LpelBufferInit(buffer_t *buf, unsigned int size);
void  LpelBufferCleanup(buffer_t *buf);

void *LpelBufferTop(buffer_t *buf);
void  LpelBufferPop(buffer_t *buf);
int   LpelBufferIsSpace(buffer_t *buf);
void  LpelBufferPut(buffer_t *buf, void *item);
int	  LpelBufferCount(buffer_t *buf);

#endif /* _BUFFER_H_ */
