CC = gcc
CFLAGS = -Wall -Wextra -std=gnu11
LDFLAGS = -pthread

SOURCE = parallel-vs-sequential-benchmark.c
TARGET = $(basename $(SOURCE))

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(SOURCE) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
