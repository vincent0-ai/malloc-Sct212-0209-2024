# Benchmark report

The benchmark replays the same 50,000 deterministic operations (`srand(42)`)
for each allocator. Each operation selects one of 1,024 slots and either frees
the existing allocation or allocates a size from 1 through 4096 bytes.
Fragmentation is measured immediately after the operation sequence, before
cleaning up the remaining slots. Run `make bench` on Linux and replace the
table below with that run's output.

| Allocator | Runtime (s) | Heap bytes | Free bytes | Free blocks | Largest free | External fragmentation |
|---|---:|---:|---:|---:|---:|---:|
| FIT_FIRST | pending Linux run | pending | pending | pending | pending | pending |
| FIT_BEST | pending Linux run | pending | pending | pending | pending | pending |
| glibc malloc/free | pending Linux run | N/A | N/A | N/A | N/A | N/A |

FIT_FIRST is faster because it stops at the first adequate hole, while
FIT_BEST scans the entire list to find the smallest suitable block. Depending
on the sequence, best-fit can leave fewer free blocks but a smaller largest
free block, increasing its external-fragmentation ratio. Both custom policies
may request similar memory when they reach the same live allocation levels.
The custom allocator is expected to be substantially slower than glibc's ptmalloc2:
it performs a linear linked-list search for every allocation, stores a header
for every block, and performs whole-list coalescing on every free. ptmalloc2
uses bins, size classes, boundary metadata, caching, and highly optimized
system calls, which reduce both search time and metadata overhead.
