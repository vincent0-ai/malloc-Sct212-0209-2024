#define _POSIX_C_SOURCE 200809L

#include "myalloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

enum { OPERATIONS = 50000, SLOTS = 1024 };

typedef struct {
    unsigned slot;
    size_t size;
} operation_t;

static operation_t operations[OPERATIONS];

static double elapsed_seconds(const struct timespec *start,
                              const struct timespec *end)
{
    return (double)(end->tv_sec - start->tv_sec) +
           (double)(end->tv_nsec - start->tv_nsec) / 1000000000.0;
}

static int monotonic_time(struct timespec *value)
{
    return clock_gettime(CLOCK_MONOTONIC, value);
}

static void make_operations(void)
{
    int i;

    srand(42);
    for (i = 0; i < OPERATIONS; i++) {
        operations[i].slot = (unsigned)(rand() % SLOTS);
        operations[i].size = 1 + (size_t)(rand() % 4096);
    }
}

static void print_allocator_result(const char *name, double seconds,
                                   const heap_stats_t *stats)
{
    double fragmentation = stats->free_bytes == 0
                               ? 0.0
                               : 1.0 - (double)stats->largest_free /
                                          (double)stats->free_bytes;

    printf("%s: time=%.6f heap_bytes=%zu free_bytes=%zu free_blocks=%zu "
           "largest_free=%zu external_fragmentation=%.6f\n",
           name, seconds, stats->heap_bytes, stats->free_bytes,
           stats->free_blocks, stats->largest_free, fragmentation);
}

static void run_myalloc(fit_policy_t policy, const char *name)
{
    void *slots[SLOTS] = {0};
    heap_stats_t stats;
    struct timespec start;
    struct timespec end;
    int i;

    set_policy(policy);
    if (monotonic_time(&start) != 0)
        exit(EXIT_FAILURE);
    for (i = 0; i < OPERATIONS; i++) {
        unsigned slot = operations[i].slot;

        if (slots[slot] != NULL) {
            my_free(slots[slot]);
            slots[slot] = NULL;
        } else {
            slots[slot] = my_malloc(operations[i].size);
        }
    }
    if (monotonic_time(&end) != 0)
        exit(EXIT_FAILURE);
    heap_stats(&stats);
    print_allocator_result(name, elapsed_seconds(&start, &end), &stats);
    for (i = 0; i < SLOTS; i++)
        my_free(slots[i]);
}

static void run_libc(void)
{
    void *slots[SLOTS] = {0};
    struct timespec start;
    struct timespec end;
    int i;

    if (monotonic_time(&start) != 0)
        exit(EXIT_FAILURE);
    for (i = 0; i < OPERATIONS; i++) {
        unsigned slot = operations[i].slot;

        if (slots[slot] != NULL) {
            free(slots[slot]);
            slots[slot] = NULL;
        } else {
            slots[slot] = malloc(operations[i].size);
        }
    }
    if (monotonic_time(&end) != 0)
        exit(EXIT_FAILURE);
    printf("libc malloc/free: time=%.6f\n",
           elapsed_seconds(&start, &end));
    for (i = 0; i < SLOTS; i++)
        free(slots[i]);
}

int main(void)
{
    make_operations();
    run_myalloc(FIT_FIRST, "FIT_FIRST");
    run_myalloc(FIT_BEST, "FIT_BEST");
    run_libc();
    return 0;
}
