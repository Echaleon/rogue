#ifndef ROGUE_DUNGEON_DISK_H
#define ROGUE_DUNGEON_DISK_H

#include <stdbool.h>

// See dungeon-disk.c for helper functions

// Forward declare so we don't have to include the dungeon header
typedef struct Dungeon_S Dungeon_T;

// Loads a dungeon from disk, returning a pointer to the dungeon. If it fails, it will fall back to a random one.
Dungeon_T *load_dungeon(const char *path, bool stairs);

// Load a PGM file from disk, creating a new dungeon. If it fails, it will fall back to a random one.
Dungeon_T *load_pgm(const char *path, bool stairs);

// Store a dungeon on disk. Will print an error to stderr if it fails
void save_dungeon(Dungeon_T *d, const char *path);

// Store a dungeon as a PGM file on disk. Will print an error to stderr if it fails.
void save_pgm(Dungeon_T *d, const char *path);

#endif //ROGUE_DUNGEON_DISK_H
