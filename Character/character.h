#ifndef ROGUE_CHARACTER_H
#define ROGUE_CHARACTER_H

#include <stdbool.h>

// See character.c for helper functions

// Forward declare so we don't have to include the dungeon header
typedef struct Dungeon_S Dungeon_T;

// Struct for a character. Will be used for player and monster.
typedef struct Character_S {
    Dungeon_T *d;
    int y, x;
    char symbol;
    char *color;
    bool player;
    int *regular_cost, *tunnel_cost;
} Character_T;

// Returns a pointer to a new character. May be made static later. Simply initializes the above. Make sure to update
// this function when extending the above struct
Character_T *new_character(Dungeon_T *d, int y, int x, char symbol, char *color, bool player);

// Builds a dijkstra cost map with the given character as the source for both regular and tunneling characters
void build_character_cost_maps(Character_T *c);

// Prints a character's cost maps
void print_character_cost_maps(Character_T *c);

// Cleans up a character, freeing the cost maps they may have.
void cleanup_character(Character_T *c);

#endif //ROGUE_CHARACTER_H
