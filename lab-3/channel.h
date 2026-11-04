#pragma once
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <errno.h>
#include <limits.h>

#define HEADER (sizeof(size_t) * 2)
#define SHM_SIZE 4096
#define SLOTS 4

void error(const char* msg);

typedef struct {
    char* buffer;
    sem_t* sem_data;
    sem_t* sem_space;
} channel_t;

channel_t create_channel(const char* shm_name, const char* sem_data_name, const char* sem_space_name);
channel_t open_channel(const char* shm_name, const char* sem_data_name, const char* sem_space_name);
void destroy_channel(channel_t* channel, size_t len, const char* shm_name, const char* sem_data_name, const char* sem_space_name);