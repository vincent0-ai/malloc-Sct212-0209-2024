#define _DEFAULT_SOURCE

#include "myalloc.h"

#include <stdint.h>
#include <stddef.h>
#include <unistd.h>

extern void *sbrk(intptr_t increment);

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

static block_t *grow(size_t need)
{
    char *program_break;
    size_t padding;
    size_t request;
    block_t *block;

    program_break = (char *)sbrk(0);
    if (program_break == (void *)-1)
        return NULL;
    padding = ((size_t)(-(uintptr_t)program_break)) % ALIGNMENT;
    if (padding != 0 && sbrk((intptr_t)padding) == (void *)-1)
        return NULL;

    request = align_up(need + HDR);
    if (request < 64 * 1024)
        request = 64 * 1024;
    block = (block_t *)sbrk((intptr_t)request);
    if (block == (void *)-1)
        return NULL;

    block->size = request - HDR;
    block->next = NULL;
    block->free = 0;
    block->magic = MAGIC;
    heap_bytes += padding + request;
    if (tail != NULL)
        tail->next = block;
    else
        head = block;
    tail = block;
    return block;
}

static block_t *find_fit(size_t need)
{
    block_t *current;

    for (current = head; current != NULL; current = current->next)
        if (current->free && current->size >= need)
            return current;
    return NULL;
}

static block_t *find_block(void *ptr)
{
    block_t *candidate = header(ptr);
    block_t *current;

    for (current = head; current != NULL; current = current->next)
        if (current == candidate && payload(current) == ptr)
            return current;
    return NULL;
}

static void report_error(const char *message, size_t length)
{
    (void)write(STDERR_FILENO, message, length);
}

void *my_malloc(size_t size)
{
    size_t need;
    block_t *block;

    if (size == 0 || size > SIZE_MAX - (ALIGNMENT - 1))
        return NULL;
    need = align_up(size);
    block = find_fit(need);
    if (block == NULL)
        block = grow(need);
    if (block == NULL)
        return NULL;
    if (block->free && block->size >= need + HDR + ALIGNMENT) {
        block_t *remainder = (block_t *)((char *)payload(block) + need);

        remainder->size = block->size - need - HDR;
        remainder->next = block->next;
        remainder->free = 1;
        remainder->magic = MAGIC;
        block->next = remainder;
        block->size = need;
        if (tail == block)
            tail = remainder;
    }
    block->free = 0;
    return payload(block);
}

void my_free(void *ptr)
{
    block_t *block;
    block_t *current;

    if (ptr == NULL)
        return;
    block = find_block(ptr);
    if (block == NULL || block->magic != MAGIC) {
        report_error("my_free: invalid pointer\n", sizeof("my_free: invalid pointer\n") - 1);
        return;
    }
    if (block->free) {
        report_error("my_free: double free detected\n",
                     sizeof("my_free: double free detected\n") - 1);
        return;
    }
    block->free = 1;
    current = head;
    while (current != NULL && current->next != NULL) {
        block_t *next = current->next;

        if (current->free && next->free &&
            (char *)payload(current) + current->size == (char *)next) {
            current->size += HDR + next->size;
            current->next = next->next;
            if (tail == next)
                tail = current;
        } else {
            current = next;
        }
    }
}
