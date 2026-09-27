#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>

int main() {
    char buffer[1024];
    ssize_t bytes = 0;
    ssize_t written = 0;

    while (bytes = read(STDIN_FILENO, buffer, sizeof(buffer))) {
        if (bytes < 0) {
            const char msg[] = "error: failed to read from stdin\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            exit(EXIT_FAILURE);
        }
        
        for (ssize_t i = 0; i < bytes; i++) {
            buffer[i] = (char)tolower((unsigned char)buffer[i]);
        }
        
        written = write(STDOUT_FILENO, buffer, bytes);
        if (written != bytes) {
            const char msg[] = "error: failed to write to stdout\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            exit(EXIT_FAILURE);
        }
    }

    exit(EXIT_SUCCESS);
}