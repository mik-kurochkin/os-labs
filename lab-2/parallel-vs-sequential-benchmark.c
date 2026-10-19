#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>


static size_t max_memory = 33 * 10000;
static size_t current_memory = 0;
static size_t n_threads = 0;
static const char* filename = NULL;
static const char* mode = "seq";
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    __uint128_t high;
    __uint128_t low;
} Accumulator;

typedef struct {
    const char* filename;
    size_t start;
    size_t end;
    size_t memory_limit;
    Accumulator result;
    size_t count;
} Thread_args;

static void error(const char* msg) {
    write(STDERR_FILENO, msg, strlen(msg));
    exit(EXIT_FAILURE);
}

static void* safe_malloc(size_t size) {
    pthread_mutex_lock(&mutex);

    if (size > max_memory - current_memory) {
        pthread_mutex_unlock(&mutex);
        error("memory usage limit exceeded\n");
    }

    void* ptr = malloc(size);
    if (ptr == NULL) {
        pthread_mutex_unlock(&mutex);
        error("failed to allocate memory\n");
    }

    current_memory += size;
    pthread_mutex_unlock(&mutex);
    return ptr;
}

static void safe_free(void* ptr, size_t size) {
    pthread_mutex_lock(&mutex);
    if (ptr != NULL) {
        free(ptr);
        current_memory -= size;
    }
    pthread_mutex_unlock(&mutex);
}

static __uint128_t hex_to_uint128(const char* string) {
    __uint128_t number = 0;
    __uint128_t digit = 0;
    while (*string) {
        char current = *string++;
        if (current >= '0' && current <= '9') {
            digit = (__uint128_t)(current - '0');
        }
        else if (current >= 'A' && current <= 'F') {
            digit = (__uint128_t)(current - 'A' + 10);
        }
        else if (current >= 'a' && current <= 'f') {
            digit = (__uint128_t)(current - 'a' + 10);
        }
        else {
            error("data error\n");
        }

    number = (number << 4) | digit;
    }

    return number;
}

static void* accumulate(void* arg) {
    Thread_args* args = (Thread_args*)arg;
    
    if (args->start >= args->end) {
        return NULL;
    }

    int fd = open(args->filename, O_RDONLY);
    if (fd == -1) {
        error("failed to open file\n");
    }

    if(lseek(fd, args->start, SEEK_SET) == -1) {
        error("lseek error\n");
    }

    size_t chunk_size = 33 * 1000;
    size_t remaining_file = args->end - args->start;
    if (chunk_size > remaining_file) {
        chunk_size = remaining_file;
    }

    if (chunk_size > args->memory_limit) {
        chunk_size = args->memory_limit;
    }

    if (chunk_size < remaining_file && chunk_size >= 33) {
        chunk_size = (chunk_size / 33) * 33;
    }

    if (chunk_size < 32) {
        error("memory limit is too small\n");
    }

    char* buffer = safe_malloc(chunk_size);

    size_t total_read = 0;
    while(remaining_file > total_read) {
        size_t to_read = remaining_file - total_read;
        if (to_read > chunk_size) {
            to_read = chunk_size;
        }

        ssize_t bytes = read(fd, buffer, to_read);
        if (bytes < 0) {
            error("failed to read file\n");
        }

        if (bytes == 0) {
            error("unexpected EOF\n");
        }
    

        for (size_t i = 0; i + 32 <= (size_t)bytes; i += 33) {
            char string[33];
            string[32] = '\0';
            for (size_t j = 0; j < 32; j++) {
                string[j] = buffer[i + j];
            }

            __uint128_t number = hex_to_uint128(string);

            __uint128_t low_before_sum = args->result.low;
            args->result.low += number;
            if (low_before_sum > args->result.low) {
                args->result.high++;
            }
            args->count++;
        }

        total_read += (size_t)bytes;
    }

    close(fd);
    safe_free(buffer, chunk_size);

    return NULL;
}

static Accumulator sequential_accumulate(const char* filename, size_t* count) {
    Accumulator sum = {0};

    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        error("failed to open file\n");
    }

    ssize_t bytes = 0;
    char buffer[33 * 100];
    size_t buffer_size = sizeof(buffer);
    while ((bytes = read(fd, buffer, buffer_size)) > 0) {
        if (bytes % 33 != 0 && (bytes - 32) % 33 != 0) {
            error("file format error\n");
        }

        for (size_t i = 0; i + 32 <= (size_t)bytes; i += 33) {
            char string[33];
            string[32] = '\0';
            for (size_t j = 0; j < 32; j++) {
                string[j] = buffer[i + j];
            }

            __uint128_t number = hex_to_uint128(string);

            __uint128_t old_low = sum.low;
            sum.low += number;
            if (sum.low < old_low) {
                sum.high++;
            }
            (*count)++;
        }
    }

    close(fd);

    return sum;
}

static Accumulator div_256(Accumulator sum, size_t count, __uint128_t* remainder) {
    if (count == 0) {
        error("division by zero\n");
    }

    Accumulator result = {0};
    result.high = sum.high / count;
    __uint128_t rem = sum.high % count;
    for (int i = 127; i >= 0; i--) {
        rem = (rem << 1) | ((sum.low >> i) & 1);
        if (rem >= count) {
            rem -= count;
            result.low |= ((__uint128_t)1 << i);
        }
    }

    *remainder = rem;

    return result;
}

static Accumulator round_256(Accumulator average, size_t count, __uint128_t remainder) {
    if (remainder >= (__uint128_t)(count / 2 + count % 2)) {
        __uint128_t old_low = average.low;
        average.low++;
        if (average.low < old_low) {
            average.high++;
        }
    }
    return average;
}
static void print_256(Accumulator number) {
    if (number.high == 0 && number.low == 0) {
        ssize_t written = write(STDOUT_FILENO, "0\n", 2);
        if (written != 2) {
            error("failed to write to stdout\n");
        }
        return;
    }

    char buffer[100];
    int index = 0;
    while (number.high > 0 || number.low > 0) {
        __uint128_t remainder = 0;
        number = div_256(number, 10, &remainder);
        buffer[index++] = '0' + remainder;
    }

    while (index > 0) {
        ssize_t written = write(STDOUT_FILENO, &buffer[--index], 1);
        if (written != 1) {
            error("failed to write to stdout\n");
        }
    }
    ssize_t written = write(STDOUT_FILENO, "\n", 1);
    if (written != 1) {
        error("failed to write to stdout\n");
    }
}

int main(int argc, char* argv[]) {
    static struct option long_options[] = {
        {"filename", required_argument, 0, 'f'},
        {"mode", required_argument, 0, 'r'},
        {"n_threads", required_argument, 0, 't'},
        {"max_memory", required_argument, 0, 'm'},
        {0, 0, 0, 0}
    };
    int option_index = 0;
    int c;
    while ((c = getopt_long(argc, argv, "f:r:t:m:", long_options, &option_index)) != -1) {
        switch(c) {
            case 'f':
                filename = optarg;
                break;
            case 'r':
                mode = optarg;
                break;
            case 't':
                n_threads = atoi(optarg);
                break;
            case 'm':
                max_memory = atoi(optarg);
                break;
            case '?':
                error("unknown key\n");
        }
    }

    struct stat st;
    if (stat(filename, &st) == -1) {
        error("cannot stat file\n");
    }
    size_t file_size = st.st_size;
    struct timespec start, end;
    double elapsed_ms = 0.0;
    long long elapsed_ns = 0;

    if (strcmp(mode, "seq") == 0 || n_threads <= 1) {
        size_t count = 0;
        __uint128_t remainder = 0;

        clock_gettime(CLOCK_MONOTONIC, &start);

        Accumulator sum = sequential_accumulate(filename, &count);

        clock_gettime(CLOCK_MONOTONIC, &end);

        elapsed_ns = (end.tv_sec - start.tv_sec) * 1000000000LL + (end.tv_nsec - start.tv_nsec);
        elapsed_ms = (double)elapsed_ns / 1000000.0;

        Accumulator average = div_256(sum, count, &remainder);
        average = round_256(average, count, remainder);
        print_256(average);

        const char* text = "time of sequential execution: ";
        ssize_t written = write(STDOUT_FILENO, text, strlen(text));
        if (written != (ssize_t)strlen(text)) {
            error("failed to write to stdout\n");
        }
        char time[64];
        int len = snprintf(time, sizeof(time), "%.3f ms\n", elapsed_ms);
        written = write(STDOUT_FILENO, time, len);
        if (written != len) {
            error("failed to write to stdout\n");
        }
    }
    else if (strcmp(mode, "par") == 0) {
        pthread_t* threads = malloc(n_threads * sizeof(pthread_t));
        Thread_args* thread_args = malloc(n_threads * sizeof(Thread_args));

        size_t total_items = file_size / 33;
        if (total_items % 33 != 0) {
            total_items++;
        }
        if (total_items < n_threads) {
            n_threads = total_items;
        }
        size_t base = total_items / n_threads;
        size_t rest = total_items % n_threads;
        size_t current_item = 0;
        size_t thread_limit = max_memory / n_threads;
        for (size_t i = 0; i < n_threads; i++) {
            size_t items = base + ((size_t)i < rest ? 1 : 0);
            size_t start = current_item * 33;
            current_item += items;
            size_t end = current_item * 33;
            if (end > file_size) {
                end = file_size;
            }
            thread_args[i] = (Thread_args){
                .filename = filename,
                .start = start,
                .end = end,
                .memory_limit = thread_limit,
                .count = 0,
                .result = {0}
            };
        }
        
        clock_gettime(CLOCK_MONOTONIC, &start);

        for (size_t i = 0; i < n_threads; i++) {
            pthread_create(&threads[i], NULL, accumulate, &thread_args[i]);
        }

        for (size_t i = 0; i < n_threads; i++) {
            pthread_join(threads[i], NULL);
        }

        Accumulator total_sum = {0};
        size_t total_count = 0;
        __uint128_t remainder = 0;
        for(size_t i = 0; i < n_threads; i++) {
            total_count += thread_args[i].count;
            total_sum.high += thread_args[i].result.high;
            __uint128_t old_low = total_sum.low;
            total_sum.low += thread_args[i].result.low;
            if (total_sum.low < old_low) {
                total_sum.high++;
            }
        }

        clock_gettime(CLOCK_MONOTONIC, &end);

        elapsed_ns = (end.tv_sec - start.tv_sec) * 1000000000LL + (end.tv_nsec - start.tv_nsec);
        elapsed_ms = (double)elapsed_ns / 1000000.0;
        Accumulator average = div_256(total_sum, total_count, &remainder);
        average = round_256(average, total_count, remainder);
        print_256(average);

        const char* text = "time of parallel execution: ";
        ssize_t written = write(STDOUT_FILENO, text, strlen(text));
        if (written != (ssize_t)strlen(text)) {
            error("failed to write to stdout\n");
        }
        char time[64];
        int len = snprintf(time, sizeof(time), "%.3f ms\n", elapsed_ms);
        written = write(STDOUT_FILENO, time, len);
        if (written != len) {
            error("failed to write to stdout\n");
        }

        free(threads);
        free(thread_args);
    }
    
    return 0;
}
