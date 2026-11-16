#define _DEFAULT_SOURCE

#include "myalloc.h"

#include <stdint.h>
#include <stddef.h>

#define ALIGNMENT ((size_t)16)
#define MAGIC ((int)0x4D414C4C)

typedef struct block {
    size_t size;
    struct block *next;
    int free;
    int magic;
} block_t;

#define HDR (((sizeof(block_t) + ALIGNMENT - 1) / ALIGNMENT) * ALIGNMENT)

static block_t *head;
static block_t *tail;
static fit_policy_t policy = FIT_FIRST;
static size_t heap_bytes;

static size_t align_up(size_t n)
{
    size_t remainder = n % ALIGNMENT;

    if (remainder == 0)
        return n;
    return n + (ALIGNMENT - remainder);
}

static void *payload(block_t *block)
{
    return (void *)((char *)block + HDR);
}

static block_t *header(void *ptr)
{
    return (block_t *)((char *)ptr - HDR);
}
