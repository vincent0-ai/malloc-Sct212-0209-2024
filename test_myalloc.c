/* Provided test harness. Do NOT edit; extend in your own file if you wish. */
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "myalloc.h"

static int passed = 0, failed = 0;
#define CHECK(name, cond) do { \
    if (cond) { passed++; printf("  PASS  %s\n", name); } \
    else      { failed++; printf("  FAIL  %s  (line %d)\n", name, __LINE__); } } while (0)

static void test_basic(void)
{
    puts("T1 basic allocation");
    char *a = my_malloc(100);
    CHECK("returns non-NULL", a != NULL);
    CHECK("16-byte aligned", ((uintptr_t)a % 16) == 0);
    memset(a, 0xAB, 100);
    CHECK("heap consistent", heap_check());
    my_free(a);
    CHECK("malloc(0) is NULL", my_malloc(0) == NULL);
    my_free(NULL);
    CHECK("free(NULL) is safe", heap_check());
}

static void test_no_overlap(void)
{
    puts("T2 blocks do not overlap");
    unsigned char *p[20];
    for (int i = 0; i < 20; i++) { p[i] = my_malloc(50 + i * 7); memset(p[i], i + 1, 50 + i * 7); }
    int ok = 1;
    for (int i = 0; i < 20; i++)
        for (int j = 0; j < 50 + i * 7; j++) if (p[i][j] != i + 1) ok = 0;
    CHECK("data intact in all blocks", ok);
    for (int i = 0; i < 20; i++) my_free(p[i]);
    CHECK("heap consistent after frees", heap_check());
}

static void test_reuse_split(void)
{
    puts("T3 reuse and splitting");
    heap_stats_t s0, s1;
    void *big = my_malloc(4000);
    my_free(big);
    heap_stats(&s0);
    void *a = my_malloc(64);
    CHECK("freed space is reused (no growth)", a == big);
    heap_stats(&s1);
    CHECK("heap did not grow", s1.heap_bytes == s0.heap_bytes);
    CHECK("block was split (remainder is free)", s1.free_blocks >= 1 && s1.largest_free > 3000);
    my_free(a);
}

static void test_coalesce(void)
{
    puts("T4 coalescing");
    void *a = my_malloc(200), *b = my_malloc(200), *c = my_malloc(200);
    my_free(a); my_free(c); my_free(b);        /* free out of order */
    heap_stats_t s; heap_stats(&s);
    CHECK("all memory is free", s.used_bytes == 0);
    CHECK("adjacent free blocks merged into one", s.free_blocks == 1);
    void *big = my_malloc(550);
    CHECK("merged block satisfies a 550-byte request", big != NULL && big == a);
    my_free(big);
}

static void test_calloc_realloc(void)
{
    puts("T5 calloc and realloc");
    unsigned char *z = my_calloc(10, 20);
    int zero = z != NULL;
    for (int i = 0; zero && i < 200; i++) if (z[i]) zero = 0;
    CHECK("calloc zeroes memory", zero);
    CHECK("calloc overflow returns NULL", my_calloc(SIZE_MAX / 2, 4) == NULL);
    my_free(z);

    char *s = my_malloc(16);
    strcpy(s, "hello, heap");
    s = my_realloc(s, 5000);
    CHECK("realloc preserves contents", s && strcmp(s, "hello, heap") == 0);
    s = my_realloc(s, 8);
    CHECK("shrinking keeps prefix", s && strncmp(s, "hello, h", 8) == 0);
    my_free(s);
    s = my_realloc(NULL, 32);
    CHECK("realloc(NULL, n) acts like malloc", s != NULL);
    CHECK("realloc(p, 0) frees and returns NULL", my_realloc(s, 0) == NULL);
    CHECK("heap consistent", heap_check());
}

static void test_grow(void)
{
    puts("T6 heap growth via sbrk");
    heap_stats_t a, b;
    heap_stats(&a);
    void *p = my_malloc(300 * 1024);
    heap_stats(&b);
    CHECK("large request succeeds", p != NULL);
    CHECK("heap grew", b.heap_bytes > a.heap_bytes);
    memset(p, 0x5A, 300 * 1024);
    my_free(p);
    CHECK("heap consistent", heap_check());
}

static void test_policy(void)
{
    puts("T7 best-fit vs first-fit");
    void *a = my_malloc(512), *g1 = my_malloc(16), *b = my_malloc(128), *g2 = my_malloc(16);
    my_free(a); my_free(b);                      /* holes: 512 then 128 */
    set_policy(FIT_FIRST);
    void *f = my_malloc(100);
    CHECK("first-fit takes the first hole", f == a);
    my_free(f);
    set_policy(FIT_BEST);
    void *bf = my_malloc(100);
    CHECK("best-fit takes the tightest hole", bf == b);
    my_free(bf); my_free(g1); my_free(g2);
    set_policy(FIT_FIRST);
}

static void test_stress(void)
{
    puts("T8 randomized stress test");
    enum { N = 400 };
    unsigned char *p[N] = {0}; size_t len[N] = {0}; unsigned char tag[N] = {0};
    srand(12345);
    int ok = 1;
    for (int it = 0; it < 20000; it++) {
        int i = rand() % N;
        if (p[i]) {
            for (size_t k = 0; k < len[i]; k++) if (p[i][k] != tag[i]) ok = 0;
            my_free(p[i]); p[i] = NULL;
        } else {
            len[i] = 1 + (size_t)(rand() % 2000);
            p[i] = my_malloc(len[i]);
            if (!p[i]) { ok = 0; break; }
            tag[i] = (unsigned char)(rand() & 0xFF);
            memset(p[i], tag[i], len[i]);
        }
        if (it % 1000 == 0 && !heap_check()) ok = 0;
    }
    CHECK("no data corruption during 20000 operations", ok);
    for (int i = 0; i < N; i++) my_free(p[i]);
    heap_stats_t s; heap_stats(&s);
    CHECK("everything freed", s.used_bytes == 0);
    CHECK("fully coalesced (few free blocks remain)", s.free_blocks <= 3);
    CHECK("heap consistent", heap_check());
}

int main(void)
{
    test_basic(); test_no_overlap(); test_reuse_split(); test_coalesce();
    test_calloc_realloc(); test_grow(); test_policy(); test_stress();
    printf("\nResult: %d passed, %d failed\n", passed, failed);
    return failed ? 1 : 0;
}
