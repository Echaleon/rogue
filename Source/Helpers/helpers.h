#ifndef ROGUE_HELPERS_H
#define ROGUE_HELPERS_H

// Defines colors for printing to console
#define BACKGROUND_WHITE "\x1b[48;2;255;255;255m"
#define BACKGROUND_GREY "\x1b[48;2;127;127;127m"
#define BACKGROUND_BLACK "\x1b[48;2;0;0;0m"

#define FOREGROUND_WHITE "\x1b[38;2;255;255;255m"
#define FOREGROUND_GREY "\x1b[38;2;170;170;170m"
#define FOREGROUND_KHAKI "\x1b[38;2;240;230;140m"
#define FOREGROUND_BROWN "\x1b[38;2;139;69;19m"

#define FOREGROUND_LIME_GREEN "\x1b[38;2;50;205;50m"
#define FOREGROUND_COBALT "\x1b[38;2;70;130;180m"
#define FOREGROUND_TEAL "\x1b[38;2;0;128;128m"
#define FOREGROUND_SKY_BLUE "\x1b[38;2;135;206;235m"
#define FOREGROUND_BRICK "\x1b[38;2;178;34;34m"
#define FOREGROUND_SLATE_BLUE "\x1b[38;2;106;90;205m"

#define FOREGROUND_RED "\x1b[38;2;255;0;0m"
#define FOREGROUND_ORANGE "\x1b[38;2;255;165;0m"
#define FOREGROUND_YELLOW "\x1b[38;2;255;255;0m"
#define FOREGROUND_GREEN "\x1b[38;2;0;128;0m"
#define FOREGROUND_BLUE "\x1b[38;2;65;105;225m"
#define FOREGROUND_PURPLE "\x1b[38;2;138;43;226m"
#define FOREGROUND_PINK "\x1b[38;2;238;130;238m"

// Defines the proper key combo to reset the console
#define CONSOLE_RESET "\x1b[0m"

// Wraps malloc() with an additional check to make sure the pointer is real. If it's not the program kills.
void *safe_malloc(size_t size);

// Same as safe_malloc()
void *safe_calloc(size_t num, size_t size);

// Same as safe_malloc()
void *safe_realloc(void *ptr, size_t size);

// Returns a random integer in the range [lower, upper] (inclusive). LOWER MUST BE < UPPER OR AN ARITHMETIC FAULT
// WILL BE GENERATED
int rand_int_in_range(int lower, int upper);

// Shuffles the given int array with a Fisher-Yates algorithm. Modifies the array in memory.
void shuffle_int_array(int *arr, int n);

// Returns the number of digits in a given integer (used for printing)
int count_digits(int n);

// Calculates the Manhattan distance between two points
int manhattan_distance(int y0, int x0, int y1, int x1);

// Wrapper to kill the program while informing the user as to why. MSG MUST BE PROPERLY FORMATTED FOR FPRINT OR WE WILL
// CAUSE A SEGFAULT AND CRASH
_Noreturn void bail(int code, char *msg, ...);

#endif //ROGUE_HELPERS_H
