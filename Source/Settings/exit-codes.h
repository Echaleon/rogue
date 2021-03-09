#ifndef ROGUE_EXIT_CODES_H
#define ROGUE_EXIT_CODES_H

// Defines various exit codes used across the program

// Used for a normal program exit
#define NORMAL_EXIT 0

// Used for when the program is passed invalid arguments on the command line
#define INVALID_ARGUMENT 1

// Used for when the program has a call for memory allocation fail
#define MEM_FAILURE 2

// Used for when the program can't properly generate a dungeon after too many tries
#define DUNGEON_GENERATION_FAILURE 3

// Used for catastrophic errors the program cannot recover from, like null values being passed around...
// Really only used when clang-tidy complains if something isn't done about it... the code isn't constantly checking
// for null types and the like.
#define INVALID_STATE 4

#endif //ROGUE_EXIT_CODES_H
