#ifndef ROGUE_DUNGEON_RANDOM_H
#define ROGUE_DUNGEON_RANDOM_H

// Forward declare so we don't have to include the dungeon header
typedef struct Dungeon_S Dungeon_T;

// Returns a pointer to a new dungeon. A few parameters are able to changed at runtime if the user so desires. For now
// the driver code uses the defaults in setting.h
Dungeon_T *generate_dungeon(int height, int width, int min_rooms, int max_rooms, float percentage_covered);


#endif //ROGUE_DUNGEON_RANDOM_H
