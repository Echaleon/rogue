#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>

#include "helpers.h"

#include "Settings/exit-codes.h"

// See helpers.h
void *safe_malloc(size_t size) {
    void *p = malloc(size);
    if (p == NULL) { // if malloc() returns null, it failed
        bail(MEM_FAILURE, "FATAL ERROR! FAILED TO ALLOCATE %i BYTES IN MEMORY!\n", size);
    }

    return p;
}

// See helpers.h
void *safe_calloc(size_t num, size_t size) {
    void *p = calloc(num, size);
    if (p == NULL) { // if calloc() returns null, it failed
        bail(MEM_FAILURE, "FATAL ERROR! FAILED TO ALLOCATE %i BYTES IN MEMORY!\n", num * size);
    }

    return p;
}

// See helpers.h
void *safe_realloc(void *old_ptr, size_t size) {
    void *p = realloc(old_ptr, size);
    if (p == NULL) { // if realloc() returns null, it failed
        bail(MEM_FAILURE, "FATAL ERROR! FAILED TO ALLOCATE %i BYTES IN MEMORY!\n", size);
    }

    return p;
}

// See helpers.h
int rand_int_in_range(int lower, int upper) {
    return rand() % (upper - lower + 1) + lower; // NOLINT(cert-msc50-cpp)
}

// See helpers.h
void shuffle_int_array(int *arr, int n) {
    int i, swap, temp;

    // Start at the last element, pick a random element, and swap them
    for (i = n - 1; i > 0; i--) {
        swap = rand_int_in_range(0, i);
        temp = arr[i];
        arr[i] = arr[swap];
        arr[swap] = temp;
    }
}

// See helpers.h
int count_digits(int n) {
    int c;

    c = 1;

    // Handle negatives
    if (n < 0) {
        n = n == INT_MIN ? INT_MAX : -n;
        c++;
    }

    // Count down
    while (n > 9) {
        n /= 10;
        c++;
    }

    return c;
}

// See helpers.h
int manhattan_distance(int y0, int x0, int y1, int x1) {
    return abs(y0 - y1) + abs(x0 - x1);
}


// See helpers.h
void bail(int code, char *msg, ...) {
    va_list args;
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);
    exit(code);
}











