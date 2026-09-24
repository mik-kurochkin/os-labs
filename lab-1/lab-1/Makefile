CC = gcc
CFLAGS = -std=gnu99

all: parent child1 child2

parent: parent.c
	$(CC) $(CFLAGS) parent.c -o parent

child1: child1.c
	$(CC) $(CFLAGS) child1.c -o child1

child2: child2.c
	$(CC) $(CFLAGS) child2.c -o child2

clean:
	rm -f parent child1 child2

.PHONY: all clean
