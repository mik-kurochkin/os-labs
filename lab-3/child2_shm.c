#include "channel.h"

int main(int argc, char* argv[]) {
    if (argc < 7) {
        error("not enough arguments\n");
    }
    channel_t child_child = open_channel(argv[1], argv[2], argv[3]);
    size_t* cc_read_index = (size_t*)(child_child.buffer + sizeof(size_t));
    *cc_read_index = 0;

    channel_t child_parent = open_channel(argv[4], argv[5], argv[6]);
    size_t* cp_write_index = (size_t*)(child_parent.buffer);
    *cp_write_index = 0;

    char* cc_data_segment = child_child.buffer + HEADER;
    char* cp_data_segment = child_parent.buffer + HEADER;

    size_t slot_size = (SHM_SIZE - HEADER) / SLOTS;
    char buffer[slot_size];
    char* write_slot = NULL;
    char* read_slot = NULL;
    size_t len = 0;
    bool running = true;
    while(running) {
        sem_wait(child_child.sem_data);
        read_slot = cc_data_segment + (*cc_read_index * slot_size);
        memcpy(&len, read_slot, sizeof(size_t));
        memcpy(buffer, read_slot + sizeof(size_t), len);
        *cc_read_index = (*cc_read_index + 1) % SLOTS;
        sem_post(child_child.sem_space);

        size_t pos = 0;
        for (size_t i = 0; i < len; i++) {
            if (buffer[i] == '\n' || !isspace((unsigned char)buffer[i]) || (i > 0 && !isspace((unsigned char)buffer[i - 1]))) {
                buffer[pos++] = buffer[i];
            }
        }
        len = pos;

        sem_wait(child_parent.sem_space);
        write_slot = cp_data_segment + (*cp_write_index * slot_size);
        memcpy(write_slot, &len, sizeof(size_t));
        memcpy(write_slot + sizeof(size_t), buffer, len);
        *cp_write_index = (*cp_write_index + 1) % SLOTS;
        sem_post(child_parent.sem_data);

        if (len == 0) {
            running = 0;
        }
    }
    _exit(EXIT_SUCCESS);
}