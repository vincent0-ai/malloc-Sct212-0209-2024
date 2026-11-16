# myalloc

**Name:** Vincent Odhiambo
**Student ID:** SCT212-0209/2024

## Build and run

```sh
make
make bench
gcc -std=c11 -g -fsanitize=address,undefined -o t_asan myalloc.c test_myalloc.c
./t_asan
make clean
```

## Task status

| Task | Status |
|---|---|
| 1. Header and alignment helpers | Complete |
| 2. Heap growth with `sbrk` | Complete |
| 3. First-fit `my_malloc` | Implemented |
| 4. Block splitting | Implemented |
| 5. `my_free` and coalescing | Implemented |
| 6. `calloc` and `realloc` | Implemented |
| 7. Best-fit, statistics, dump, check, sanitizers | Implemented; 29/29 harness checks pass |
| 8. Benchmark and report | Complete |

## Final test output

```text
Result: 29 passed, 0 failed
```

Run the sanitizer command with a GCC toolchain that provides
`libasan` and `libubsan`.

## Design explanation

Each allocation is represented by a `block_t` header followed by its payload.
The header stores the aligned payload size, the next block in the address-ordered
list, a free flag, and a magic value used to reject corrupted or unknown
blocks. `HDR` is rounded to 16 bytes, so adding it to a block address preserves
the required payload alignment; requested sizes are rounded in the same way.

The allocator obtains memory only with `sbrk()`. Before each growth request it
adds enough padding to align the current program break, then requests at least
64 KiB or enough space for the requested block. A first-fit search is the
default; best-fit selects the smallest suitable free block when requested.
Large free blocks are split only when the remainder can hold another header and
16-byte payload. The list remains address ordered, but list neighbors are not
assumed to be physically contiguous because libc may move the program break.

When freeing, the allocator marks a block free and scans the complete list.
Coalescing occurs only when the end of one payload equals the next header
address. Reallocations shrink in place, absorb a physically adjacent free
successor when possible, or allocate-copy-free as a fallback. Statistics walk
the list, while `heap_check()` verifies magic values, alignment, address order,
and that adjacent free blocks have been merged.

## Questions

1. The header must be aligned so the payload starts on a 16-byte boundary after
   the header. Payload sizes must also be multiples of 16 so every subsequent
   split header remains aligned.
2. `printf()` may allocate internal buffers. If it is called inside an allocator
   function, that internal allocation can call the allocator again and recurse
   indefinitely.
3. A `prev` pointer in each header would locate the previous block in O(1).
   This is a doubly linked list.
4. Internal fragmentation is unused space inside an allocated block caused by
   alignment or a larger reusable block. External fragmentation is free space
   divided among separate blocks, which this allocator reduces with coalescing.
5. This allocator does not lower the program break when blocks are freed, so
   released space is retained for reuse. A stack-aware allocator could lower
   the break with `brk()` when the final block at the top of the heap is free.
6. An intervening libc allocation can advance the program break between two
   allocator growth calls, leaving a gap. Coalescing therefore checks the exact
   payload-end-to-next-header address equality before merging.

## Known limitations

The allocator is single-threaded, never returns memory to the operating system,
and has no segregated size classes or locking.

## Sources and AI declaration

References:
https://0xkiire.com/memory-allocators/,
https://github.com/shrutej27/Writing-your-own-memory-allocator-in-C,
https://www.jterrazz.com/articles/1-writing-your-own-malloc-in-c-a-memory-allocator-from-scratch.

AI assistance was used to review the requirements, structure the implementation,
and identify edge cases. The submitted code was adapted to the provided public
API and test harness.
