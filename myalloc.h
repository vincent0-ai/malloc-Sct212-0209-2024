#ifndef MYALLOC_H
#define MYALLOC_H

#include <stddef.h>

/* Placement policy used when searching the free list. */
typedef enum { FIT_FIRST, FIT_BEST } fit_policy_t;

/* Snapshot of the allocator's state (used by the tests and by heap_dump). */
typedef struct {
    size_t heap_bytes;    /* total bytes obtained from the OS via sbrk()      */
    size_t used_bytes;    /* payload bytes in allocated blocks                */
    size_t free_bytes;    /* payload bytes in free blocks                     */
    size_t blocks;        /* total number of blocks (allocated + free)        */
    size_t free_blocks;   /* number of blocks currently free                  */
    size_t largest_free;  /* payload size of the largest free block           */
} heap_stats_t;

/* Required API */
void *my_malloc(size_t size);
void  my_free(void *ptr);
void *my_calloc(size_t nmemb, size_t size);
void *my_realloc(void *ptr, size_t size);

/* Diagnostics and configuration */
void  heap_stats(heap_stats_t *out);
void  heap_dump(void);                 /* print every block using write(2) */
void  set_policy(fit_policy_t p);
int   heap_check(void);                /* returns 1 if the heap is consistent */

#endif
