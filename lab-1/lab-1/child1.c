#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>

int main() {
    // Buffer for reading data from stdin
    char buffer[1024];
    ssize_t bytes = 0;
    ssize_t written = 0;

    // Read data from stdin (redirected to pipe from parent)
    while (bytes = read(STDIN_FILENO, buffer, sizeof(buffer))) {
        if (bytes < 0) {
            const char msg[] = "error: failed to read from stdin\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            exit(EXIT_FAILURE);
        }
        
        // Convert each character to lowercase
        for (ssize_t i = 0; i < bytes; i++) {
            buffer[i] = (char)tolower((unsigned char)buffer[i]);
        }
        
        // Write result to stdout (redirected to pipe to child2)
        written = write(STDOUT_FILENO, buffer, bytes);
        if (written != bytes) {
            const char msg[] = "error: failed to write to stdout\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            exit(EXIT_FAILURE);
        }
    }

    exit(EXIT_SUCCESS);
}