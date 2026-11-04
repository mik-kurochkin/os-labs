# Lab 3: Process management and IPC via memory - mapped files

Implementation of a pipeline of three processes connected by memory - mapped files.

## Architecture

User → parent → buffer1 → child1 → buffer2 → child2 → buffer3 → parent → User

- `parent_shm.c` — reads lines from user, sends to `child1`, prints result from `child2`.
- `child1_shm.c` — converts input to lowercase.
- `child2_shm.c` — removes duplicate whitespace.

## Build

make

Or manually:
```bash
gcc parent_shm.c -o parent
gcc child1_shm.c -o child1
gcc child2_shm.c -o child2
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
- Uses POSIX system calls: fork, shm_open, mmap, sem_open, exec, read, write, wait.
