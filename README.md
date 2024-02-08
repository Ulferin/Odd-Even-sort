# Odd Even Sort implementations
The repository contains various implenetations of the [Odd Even sort](https://it.wikipedia.org/wiki/Odd-even_sort) algorithm.

The algorithm is divided in two phases:
- **Phase 1**: pair-wise comparison on even-indexed elements;
- **Phase 2**: pair-wise comparison on odd-indexed elements;

At each phase the algorithm proceeds by swapping the pair of elements that are not in order (i.e. $a[i] > a[i+1]$) and it stops when no element swaps are performed at the end of the second
phase. Parallel implementations are based on the parallelization of the two phases with synchronization between each of them due to the sequential structure of the original algorithm.

# Project Structure
This project represents the final project for the course of Parallel And Distributed Systems. Its implementation had to take care of main optimization mechanisms, such as _vectorization_, _thread pinning_, _false sharing_.

The three main implementations are:
- `oe-sortseq.cpp`: sequential implementation.
- `oe-sortparnofs.cpp`: parallel implementation using C++ standard mechanisms.
- `oe-sortmw.cpp`: parallel implementation using the [FastFlow](https://github.com/fastflow/fastflow) library.

All the implementations can be compiled using the provided `Makefile`. Tests can be executed with the `test.sh` and `stats.sh` scripts.

The mandatory parameters for all implementations are:
- `seed`: used for random number generation during initialization phase.
- `length`: number of elements contained in the vector to be sorted.
- `max`: maximum value in the vector.

Parallel implementations require additional arguments:
- `nw`: number of workers (threads).
- `cache-size`: cache line size in bytes, used for padding.

> [!NOTE]
> Parameters must be provided in the following order: `seed, len, nw, cache-size, max`.

For a complete description of algorithms implementation and results, refer to [the final report](final.pdf).
