#include "contract.h"
#include <dlfcn.h>
#include <stdio.h>

static void error(const char* msg) {
    write(STDERR_FILENO, msg, strlen(msg));
    exit(EXIT_FAILURE);
}

static cos_derivative_func* cosd;
static convert_func* conv;

static float cos_deriv_impl_stub(float a, float dx) {
    (void)a;
    (void)dx;
    return 0.0f;
}

static char* convert_impl_stub(int x) {
    (void)x;
    char* string = malloc(1);
    if (string == NULL) {
        error("failed to malloc\n");
    }
    string[0] = '\0';
    return string;
}

int main(void) {
    int func, x, current_lib = 0;
    float a, dx, result;
    char* converted_number = NULL;

    void* library = dlopen("./lib1.so", RTLD_LOCAL | RTLD_NOW);
    if (library == NULL) {
        error("failed to load library\n");
    }

    cosd = (cos_derivative_func*)dlsym(library, "cos_derivative");
    if (cosd == NULL) {
        const char msg[] = "failed to find cos_derivative function implementation\n";
        write(STDERR_FILENO, msg, strlen(msg));
        cosd = cos_deriv_impl_stub;
    }

    conv = (convert_func*)dlsym(library, "convert");
    if (conv == NULL) {
        const char msg[] = "failed to find convert function implementation\n";
        write(STDERR_FILENO, msg, strlen(msg));
        conv = convert_impl_stub;
    }
    char buffer[256], output[40];
    while(true) {
        if (write(STDOUT_FILENO, ">>> ", 4) == -1) {
            error("failed to write to stdout\n");
        }
        ssize_t bytes = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
        if (bytes == -1) {
            error("failed to read from stdin\n");
        } else if (bytes == 0) {
            break;
        }
        buffer[bytes] = '\0';

        char* ptr = buffer;
        func = strtol(ptr, &ptr, 10);
        switch(func) {
            case 0:
                if (library != NULL) {
                    dlclose(library);
                }
                current_lib = (current_lib + 1) % 2;
                if (current_lib == 0) {
                    library = dlopen("./lib1.so", RTLD_LOCAL | RTLD_NOW);
                } else if (current_lib == 1) {
                    library = dlopen("./lib2.so", RTLD_LOCAL | RTLD_NOW);
                }

                if (library == NULL) {
                    error("failed to load library\n");
                }

                cosd = (cos_derivative_func*)dlsym(library, "cos_derivative");
                if (cosd == NULL) {
                    const char msg[] = "failed to find cos_derivative function implementation\n";
                    write(STDERR_FILENO, msg, strlen(msg));
                    cosd = cos_deriv_impl_stub;
                }

                conv = (convert_func*)dlsym(library, "convert");
                if (conv == NULL) {
                    const char msg[] = "failed to find convert function implementation\n";
                    write(STDERR_FILENO, msg, strlen(msg));
                    conv = convert_impl_stub;
                }
                break;
            case 1:
                a = strtof(ptr, &ptr);
                dx = strtof(ptr, &ptr);
                result = cosd(a, dx);
                size_t len = snprintf(output, sizeof(output), "result: %.3f\n", result);
                bytes = write(STDOUT_FILENO, output, len);
                if (bytes != len) {
                    error("failed to write to stdout\n");
                }
                break;
            case 2:
                x = strtol(ptr, &ptr, 10);
                converted_number = conv(x);
                if (converted_number == NULL) {
                    error("failed to convert\n");
                }
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
    if (library != NULL) {
        dlclose(library);
        }

    return 0;
}