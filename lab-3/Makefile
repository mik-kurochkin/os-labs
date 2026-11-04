CC = gcc
CFLAGS = -std=gnu99 -pthread -lrt

all: parent_shm child1_shm child2_shm

parent_shm: parent_shm.c channel.c
	$(CC) $(CFLAGS) parent_shm.c channel.c -o parent_shm

child1_shm: child1_shm.c channel.c
	$(CC) $(CFLAGS) child1_shm.c channel.c -o child1_shm

child2_shm: child2_shm.c channel.c
	$(CC) $(CFLAGS) child2_shm.c channel.c -o child2_shm

clean:
	rm -f parent_shm child1_shm child2_shm

.PHONY: all clean
