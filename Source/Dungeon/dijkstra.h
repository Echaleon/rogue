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
int *generate_dijkstra_map(const Dungeon_T *d, int num_sources, const int *sources, bool diagonal, Dijkstra_T type);

// Generate a cost map by updating a new cost mapping. It is used for monsters that are fleeing another. By first
// generating a normal cost map, then multiplying by some negative multiplier, and rerunning Dijkstra's on it, we end
// up with a reverse map that can be used for fleeing.
void generate_reverse_map(const Dungeon_T *d, int *cost, bool diagonal, Dijkstra_T type);

// Prints out a given cost map to console
void print_dijkstra_map(const Dungeon_T *d, const int *cost, Dijkstra_T type);

#endif //ROGUE_DIJKSTRA_H
