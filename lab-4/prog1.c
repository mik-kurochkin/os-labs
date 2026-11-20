#include "contract.h"
#include <stdio.h>

static void error(const char* msg) {
    write(STDERR_FILENO, msg, strlen(msg));
    exit(EXIT_FAILURE);
}

int main(void) {
    int func, x;
    float a, dx, result;
    char buffer[256];
    char output[40];
    char* converted_number = NULL;
    char* ptr = NULL;
    while(true) {
        if (write(STDOUT_FILENO, ">>> ", 4) == -1) {
            error("failed to write to stdout\n");
        }

        ssize_t bytes = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
        if (bytes == -1) {
            error("failed to read from stdin\n");
        }
        else if (bytes == 0) {
            break;
        }

        buffer[bytes] = '\0';
        ptr = buffer;
        func = strtol(ptr, &ptr, 10);

        switch (func) {
            case 1:
                a = strtof(ptr, &ptr);
                dx = strtof(ptr, &ptr);
                result = cos_derivative(a, dx);
                size_t len = snprintf(output, sizeof(output), "result: %.3f\n", result);
                bytes = write(STDOUT_FILENO, output, len);
                if (bytes != len) {
                    error("failed to write to stdout\n");
                }
                break;
            case 2:
                x = strtol(ptr, &ptr, 10);
                converted_number = convert(x);
                len = snprintf(output, sizeof(output), "result: %s\n", converted_number);
                bytes = write(STDOUT_FILENO, output, len);
                if (bytes != len) {
                    error("failed to write to stdout\n");
                }
                free(converted_number);
                break;
            default:
                error("unknown command\n");
        }
    }
    
    return 0;
}