#ifndef ROGUE_CHARACTER_H
#define ROGUE_CHARACTER_H

#include <stdbool.h>

// See character.c for helper functions

// Forward declare so we don't have to include the dungeon header
typedef struct Dungeon_S Dungeon_T;

// Enums flags to hold monster behaviors. By using bit fiddling, we can store all the flags in one value
typedef enum Behavior_E {
    INTELLIGENT = 1 << 0,
    TELEPATHIC = 1 << 1,
    TUNNELER = 1 << 2,
    ERRATIC = 1 << 3
} Behavior_T;

// Struct for a character. Will be used for player and monster.
typedef struct Character_S {
    Dungeon_T *d;
    int y, x, last_y, last_x, speed, behavior;
    char symbol;
    char *color;
    bool player;
    int *cost;
} Character_T;

// Returns a pointer to a new character. May be made static later. Simply initializes the above. Make sure to update
// this function when extending the above struct
Character_T *new_character(Dungeon_T *d, int y, int x, int speed, int behavior, char symbol, char *color, bool player);

// Builds a dijkstra cost map for a character to use
void build_character_cost_map(Character_T *c, int num_sources, int sources[][2]);

// Process a move for a monster, returning the character it killed, if any
Character_T *move_monster(Character_T *c);

// Process a move for the player, returning the character killed, if any
Character_T *move_player(Character_T *c);

// Find which room of the dungeon a character is in. Returns -1 if it can't find it.
int find_player_room(Dungeon_T *d, Character_T *c);

// Returns the char associated with a type of monster
char monster_behavior_char(int behavior);

// Returns the ASCI control string to color a cell type
char *monster_behavior_color(int behavior);

// Cleans up a character, freeing the cost maps they may have.
void cleanup_character(Character_T *c);

#endif //ROGUE_CHARACTER_H
