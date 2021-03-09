#ifndef ROGUE_PROGRAM_INIT_H
#define ROGUE_PROGRAM_INIT_H

#include <stdbool.h>

// Hold all program setting in a struct... makes clean up easier. Paths are are only allocated if we have to and are
// saving or loading from disk
typedef struct Program_S {
    bool load;
    bool save;
    bool pgm_load;
    bool pgm_save;
    bool stairs;
    bool print;
    char *load_path;
    char *save_dungeon_path;
    char *save_pgm_path;
    int num_monsters;
} Program_T;

// Function that initializes settings, and sets up the correct environment for later functions.
void init_program(int argc, const char *argv[], Program_T *p);

// Function that cleans up, so we have no memory leaks.
void cleanup_program(Program_T *p);

#endif //ROGUE_PROGRAM_INIT_H

