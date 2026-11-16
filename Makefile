CC     = gcc
CFLAGS = -std=c11 -Wall -Wextra -g -O0

test: myalloc.c test_myalloc.c myalloc.h
	$(CC) $(CFLAGS) -o test_myalloc myalloc.c test_myalloc.c
	./test_myalloc

bench: myalloc.c bench.c myalloc.h
	$(CC) $(CFLAGS) -o bench myalloc.c bench.c
	./bench

clean:
	rm -f test_myalloc bench t_asan *.o
