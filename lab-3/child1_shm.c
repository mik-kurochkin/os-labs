#include "channel.h"

int main(int argc, char* argv[]) {
    if (argc < 7) {
        error("not enough arguments\n");
    }

    channel_t parent_child = open_channel(argv[1], argv[2], argv[3]);
    size_t* pc_read_index = (size_t*)(parent_child.buffer + sizeof(size_t));
    *pc_read_index = 0;

    channel_t child_child = open_channel(argv[4], argv[5], argv[6]);
    size_t* cc_write_index = (size_t*)child_child.buffer;
    *cc_write_index = 0;

    size_t slot_size = (SHM_SIZE - HEADER) / SLOTS;
    char buffer[slot_size];
    char* read_slot = NULL;
    char* write_slot = NULL;
    char* pc_data_segment = parent_child.buffer + HEADER;
    char* cc_data_segment = child_child.buffer + HEADER;
    size_t len = 0;
    bool running = true;
    while (running) {
        sem_wait(parent_child.sem_data);
        read_slot = pc_data_segment + (*pc_read_index * slot_size);
        memcpy(&len, read_slot, sizeof(size_t));
        memcpy(buffer, read_slot + sizeof(size_t), len);
        *pc_read_index = (*pc_read_index + 1) % SLOTS;
        sem_post(parent_child.sem_space);

        for (size_t i = 0; i < len; i++) {
            buffer[i] = (char)tolower((unsigned char)buffer[i]);
        }

        sem_wait(child_child.sem_space);
        write_slot = cc_data_segment + (*cc_write_index * slot_size);
        memcpy(write_slot, &len, sizeof(size_t));
        memcpy(write_slot + sizeof(size_t), buffer, len);
        *cc_write_index = (*cc_write_index + 1) % SLOTS;
        sem_post(child_child.sem_data);
        if (len == 0) {
            running = 0;
        }
    }
    _exit(EXIT_SUCCESS);
}