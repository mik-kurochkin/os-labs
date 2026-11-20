#pragma once
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define BUFFER_SIZE 34
typedef float cos_derivative_func(float a, float dx);
typedef char* convert_func(int x);

float cos_derivative(float a, float dx);
char* convert(int x);