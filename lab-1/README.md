# Lab 1: Process management and IPC via pipes

Implementation of a pipeline of three processes connected by pipes.

## Architecture

User → parent → pipe1 → child1 → pipe2 → child2 → pipe3 → parent → User

- `parent.c` — reads lines from user, sends to `child1`, prints result from `child2`.
- `child1.c` — converts input to lowercase.
- `child2.c` — removes duplicate whitespace.

## Build

make

Or manually:
```bash
gcc parent.c -o parent
gcc child1.c -o child1
gcc child2.c -o child2
```
## Run

Interactive mode:
```bash
./parent
```
With test file:
```bash
./parent < tests.txt
```
## Notes

- OS: Linux (Ubuntu).
- Uses POSIX system calls: fork, pipe, dup2, exec, read, write, wait.
