#include "channel.h"

const char SHM_PARENT_CHILD[] = "/parent-child-shm";
const char SHM_CHILD_CHILD[] = "/child-child-shm";
const char SHM_CHILD_PARENT[] = "/child-parent-shm";
const char SEM_PC_DATA[] = "/pc-data-sem";
const char SEM_PC_SPACE[] = "/pc-space-sem";
const char SEM_CC_DATA[] = "/cc-data-sem";
const char SEM_CC_SPACE[] = "/cc-space-sem";
const char SEM_CP_DATA[] = "/cp-data-sem";
const char SEM_CP_SPACE[] = "/cp-space-sem";

int main() {
    char path[PATH_MAX];
	ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
	if (len == -1) {
		error("error: failed to read full program path\n");
	}
    else {
		while(path[len] != '/') {
			len--;
		}
		path[len] = '\0';
	}

    channel_t parent_child = create_channel(SHM_PARENT_CHILD, SEM_PC_DATA, SEM_PC_SPACE);
    channel_t child_child = create_channel(SHM_CHILD_CHILD, SEM_CC_DATA, SEM_CC_SPACE);
    size_t* pc_write_index = (size_t*)(parent_child.buffer);
    size_t* pc_read_index = (size_t*)(parent_child.buffer + sizeof(size_t));
    *pc_write_index = 0;
    *pc_read_index = 0;
    size_t* cc_write_index = (size_t*)(child_child.buffer);
    size_t* cc_read_index = (size_t*)(child_child.buffer + sizeof(size_t));
    *cc_write_index = 0;
    *cc_read_index = 0;

    pid_t child_1 = fork();
    if (child_1 == -1) {
        error("failed to spawn a new process\n");
    }
    else if (child_1 == 0) {
        char process_path[PATH_MAX + NAME_MAX + 2];
		snprintf(process_path, sizeof(process_path), "%s/child1_shm", path);

        execl(process_path, "child1_shm", SHM_PARENT_CHILD, SEM_PC_DATA, SEM_PC_SPACE,
            SHM_CHILD_CHILD, SEM_CC_DATA, SEM_CC_SPACE, NULL);

        error("failed to exec into new executable image\n");
    }

    channel_t child_parent = create_channel(SHM_CHILD_PARENT, SEM_CP_DATA, SEM_CP_SPACE);
    size_t* cp_write_index = (size_t*)(child_parent.buffer);
    size_t* cp_read_index = (size_t*)(child_parent.buffer + sizeof(size_t));
    *cp_write_index = 0;
    *cp_read_index = 0;

    pid_t child_2 = fork();
    if (child_2 == -1) {
        error("failed to spawn a new process\n");
    }
    else if (child_2 == 0) {
        char process_path[PATH_MAX + NAME_MAX + 2];
		snprintf(process_path, sizeof(process_path), "%s/child2_shm", path);

        execl(process_path, "child2_shm", SHM_CHILD_CHILD, SEM_CC_DATA, SEM_CC_SPACE,
            SHM_CHILD_PARENT, SEM_CP_DATA, SEM_CP_SPACE, NULL);
        
       error("failed to exec into new executable image\n");
    }


	ssize_t bytes = 0;
	ssize_t written = 0;
	size_t slot_size = (SHM_SIZE - HEADER) / SLOTS;
    char buffer[slot_size];
    char* pc_data_segment = parent_child.buffer + HEADER;
    char* cp_data_segment = child_parent.buffer + HEADER;
    char* slot = NULL;
    bool data = 1;
	while (1) {
        if (data != 0) {
            bytes = read(STDIN_FILENO, buffer, sizeof(buffer));
		    if (bytes < 0) {
			    const char msg[] = "error: failed to read from stdin\n";
			    write(STDERR_FILENO, msg, sizeof(msg) - 1);
			    exit(EXIT_FAILURE);
		    }

            size_t len = (size_t)bytes;
            if (buffer[0] == '\n' || bytes == 0) {
                data = 0;
                len = 0;
            }

            sem_wait(parent_child.sem_space);
            
            slot = pc_data_segment + (*pc_write_index * slot_size);
            memcpy(slot, &len, sizeof(size_t));
            memcpy(slot + sizeof(size_t), buffer, len);

            *pc_write_index = (*pc_write_index + 1) % SLOTS;
            sem_post(parent_child.sem_data);
        }

        sem_wait(child_parent.sem_data);

        slot = cp_data_segment + (*cp_read_index * slot_size);
        memcpy(&len, slot, sizeof(size_t));
		memcpy(buffer, slot + sizeof(size_t), len);

        *cp_read_index = (*cp_read_index + 1) % SLOTS;
        sem_post(child_parent.sem_space);

        if (len == 0) {
            break;
        }
		written = write(STDOUT_FILENO, buffer, len);
		if (written != len) {
			const char msg[] = "error: failed to write to stdout\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            exit(EXIT_FAILURE);
		}
    }
    
    wait(NULL);
    wait(NULL);
    
    destroy_channel(&parent_child, SHM_SIZE, SHM_PARENT_CHILD, SEM_PC_DATA, SEM_PC_SPACE);
    destroy_channel(&child_child, SHM_SIZE, SHM_CHILD_CHILD, SEM_CC_DATA, SEM_CC_SPACE);
    destroy_channel(&child_parent, SHM_SIZE, SHM_CHILD_PARENT, SEM_CP_DATA, SEM_CP_SPACE);

    return 0;
}
