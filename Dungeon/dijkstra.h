#ifndef ROGUE_DIJKSTRA_H
#define ROGUE_DIJKSTRA_H

#include <stdbool.h>

// See dijkstra.c for helper functions.

// Define a macro to help obfuscate bare pointer arithmetic
#define COST(a, b) cost[(a) * d->width + (b)]

// Forward declare so we don't have to include the dungeon header
typedef struct Dungeon_S Dungeon_T;

// Types of cost maps we can generate... makes the code safer and cleaner.
typedef enum Dijkstra_E {
    CORRIDOR_MAP, TUNNEL_MAP, REGULAR_MAP
} Dijkstra_T;

// Generates a Dijkstra cost map to map corridors. By then "rolling" downhill from the goal to the source, we can
// generate the shortest path and paint a new corridor. See paint_corridor in dungeon.c for details.
int *generate_dijkstra_map(const Dungeon_T *d, int y, int x, bool diagonal, Dijkstra_T type);

#endif //ROGUE_DIJKSTRA_H
