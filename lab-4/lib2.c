#include "contract.h"

float cos_derivative(float a, float dx) {
    double da = (double)a;
    double ddx = (double)dx;
    float derivative = (float)((cos(da + ddx) - cos(da - ddx)) / (2 * ddx));
    if (derivative > -0.001f && derivative < 0.001f) {
        derivative = 0.0f;
    }
    return derivative;
}

char* convert(int x) {
    char* base = malloc(BUFFER_SIZE);
    if (base == NULL) {
        const char msg[] = "failed to malloc\n";
        write(STDERR_FILENO, msg, strlen(msg));
        exit(EXIT_FAILURE);
    }

    char* buffer = base + BUFFER_SIZE - 1;
    *buffer = '\0';

    bool sign = x >= 0 ? false : true;
    unsigned int ux = sign ? (~(unsigned int)x) + 1 : (unsigned int)x;
    do {
        *--buffer = (char)(ux % 3) + '0';
        ux /= 3;
    }
    while (ux > 0);

    if (sign) {
        *--buffer = '-';
    }

    memmove(base, buffer, strlen(buffer) + 1);

    return base;
}