#include "channel.h"

void error(const char* msg) {
    write(STDERR_FILENO, msg, strlen(msg));
    _exit(EXIT_FAILURE);
}

channel_t create_channel(const char* shm_name, const char* sem_data_name, const char* sem_space_name) {
    shm_unlink(shm_name);
    int shm = shm_open(shm_name, O_RDWR | O_CREAT | O_EXCL, 0600);
    if (shm == -1) {
        error("failed to create SHM\n");
    }

    if (ftruncate(shm, SHM_SIZE) == -1) {
        shm_unlink(shm_name);
        error("failed to resize SHM\n");
    }

    char* buffer = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
    close(shm);
    if (buffer == MAP_FAILED) {
        shm_unlink(shm_name);
        error("failed to map SHM\n");
    }

    sem_unlink(sem_data_name);
    sem_t* sem_data = sem_open(sem_data_name, O_CREAT | O_EXCL, 0600, 0);
    if (sem_data == SEM_FAILED) {
        shm_unlink(shm_name);
        error("failed to open semaphore\n");
    }

    sem_unlink(sem_space_name);
    sem_t* sem_space = sem_open(sem_space_name, O_CREAT | O_EXCL, 0600, SLOTS);
    if (sem_space == SEM_FAILED) {
        shm_unlink(shm_name);
        sem_unlink(sem_data_name);
        error("failed to open semaphore\n");
    }
    
    channel_t channel = {buffer, sem_data, sem_space};

    return channel;
}

channel_t open_channel(const char* shm_name, const char* sem_data_name, const char* sem_space_name) {
    int shm = shm_open(shm_name, O_RDWR, 0600);
    if (shm == -1) {
        error("failed to open SHM\n");
    }

    char* buffer = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
    close(shm);
    if (buffer == MAP_FAILED) {
        error("failed to map SHM\n");
    }

    sem_t* sem_data = sem_open(sem_data_name, 0);
    if (sem_data == SEM_FAILED) {
        error("failed to open semaphore\n");
    }

    sem_t* sem_space = sem_open(sem_space_name, 0);
    if (sem_space == SEM_FAILED) {
        error("failed to open semaphore\n");
    }
    
    channel_t channel = {buffer, sem_data, sem_space};

    return channel;
}

void destroy_channel(channel_t* channel, size_t len, const char* shm_name, const char* sem_data_name, const char* sem_space_name) {
    munmap(channel->buffer, len);
    sem_close(channel->sem_data);
    sem_close(channel->sem_space);
    shm_unlink(shm_name);
    sem_unlink(sem_data_name);
    sem_unlink(sem_space_name);
}