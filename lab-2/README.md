# Lab 2: Multithreading and Parallel Programming

Implementation of sequential and parallel calculation of the arithmetic mean of 128-bit hexadecimal numbers using POSIX Threads.

## Architecture

- `parallel-vs-sequential-benchmark.c` — reads hexadecimal numbers from a file, calculates their sum using a 256-bit accumulator, and computes the arithmetic mean.
- **Sequential mode** — processes all numbers in a single thread.
- **Parallel mode** — distributes numbers among multiple threads using POSIX Threads (`pthread`).
- **Memory limit** — limits memory allocated for thread input buffers.
- **Benchmarking** — measures execution time in milliseconds.

## Build

Using Makefile:

```bash
make
```

Or manually:

```bash
gcc -Wall -Wextra -std=gnu11 parallel-vs-sequential-benchmark.c -o parallel-vs-sequential-benchmark -pthread
```

## Run

Sequential mode:

```bash
./parallel-vs-sequential-benchmark --filename=tests --mode=seq --n_threads=0 --max_memory=1024
```

Parallel mode with 4 threads:

```bash
./parallel-vs-sequential-benchmark --filename=tests --mode=par --n_threads=4 --max_memory=1024
```

Arguments:

- `--filename` — path to the input file.
- `--mode` — execution mode: `seq` or `par`.
- `--n_threads` — number of threads used in parallel mode.
- `--max_memory` — maximum memory allowed for input buffers, in bytes.

## Input Format

Each input line must contain one 128-bit hexadecimal number represented by exactly 32 hexadecimal characters.

Example:

```text
FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
80000000000000000000000000000000
123456789ABCDEF0123456789ABCDEF0
```

The final number may omit the trailing newline.

## Output

The program prints the rounded arithmetic mean of the input numbers in decimal notation, followed by the execution time in milliseconds.

## Notes

- OS: Linux (Ubuntu).
- Language: C.
- Uses POSIX system calls and POSIX Threads.
- Uses `__uint128_t` and a custom 256-bit accumulator.
