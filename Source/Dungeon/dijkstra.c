#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <Settings/print-settings.h>

#include "dijkstra.h"

#include "Dungeon/dungeon.h"
#include "Helpers/helpers.h"
#include "Helpers/pairing-heap.h"
#include "Settings/exit-codes.h"
#include "Settings/dungeon-settings.h"
#include "Settings/misc-settings.h"

// Define a difference macro just to make the code more readable
#define DIFFERENCE ((MAX_ROCK_HARDNESS - MIN_ROCK_HARDNESS) / CORR_NUM_HARDNESS_LEVELS)

// Define macros to help obfuscate bare pointer arithmetic. COST_TRANSLATE() is a simple one to translate the
// coordinate of the Dijkstra map to the cost map we return, since the dijkstra map does not include outer immutable
// cells.
#define COST_TRANSLATE(a, b) COST((a) + 1, (b) + 1)
#define DIJKSTRA(a, b) dijkstra[(a) * (d->width - 2) + (b)]

// Represents a vertex to be used in our map generation... by storing heap nodes intrusively, we can hugely decrease
// the number of calls to malloc() and increase cache coherency. By storing our cost, we can avoid calling the cost
// function more than we need, since it's static and does not change as we map through the dungeon.
typedef struct Vertex_S {
    int y, x, cost;
    Heap_Node_T node;
} Vertex_T;

// Cost function for calculating corridor costs during dungeon generation
static int corridor_cost(const Dungeon_T *d, int y, int x) {

    // Infinite cost if we can't go here.
    if (d->MAP(y + 1, x + 1).hardness == IMMUTABLE_ROCK_HARDNESS) {
        return INT_MAX;
    }

    // Different cell types have different weights for our corridor map... produces more interesting corridors this way
    switch (d->MAP(y + 1, x + 1).type) {
        case ROCK:
            // Checks if we are using hardness... if we are it divides the hardness by a difference to split it into
            // multiple ranges. If there is remainder, we just add it onto the top. Use the compiler to do the magic
            // for us
            #if USE_HARDNESS_FOR_CORRIDORS == true
            return d->MAP(y + 1, x + 1).hardness / DIFFERENCE > CORR_NUM_HARDNESS_LEVELS ?
                   CORR_NUM_HARDNESS_LEVELS + 1 : d->MAP(y + 1, x + 1).hardness / DIFFERENCE + 1;
            #else
            return 1 + CORR_ROCK_WEIGHT;
            #endif
        case STAIR_UP:
        case STAIR_DOWN:
        case ROOM:
            return 1 + CORR_ROOM_WEIGHT;
        case CORRIDOR:
            return 1 + CORR_CORRIDOR_WEIGHT;
        default: // Unreachable but it keeps the compiler happy
            return INT_MAX;
    }
}

// Cost function for tunneling monsters
static int tunnel_cost(const Dungeon_T *d, int y, int x) {

    // Only needs to be initialized once... it's a compile time constant.
    static int difference = (MAX_ROCK_HARDNESS - MIN_ROCK_HARDNESS) / TUNNEL_NUM_HARDNESS_LEVELS;

    // Infinite cost if we can't go here
    if (d->MAP(y + 1, x + 1).hardness == IMMUTABLE_ROCK_HARDNESS) {
        return INT_MAX;
    }

    // Otherwise we can just return hardness... since rooms and corridors have OPEN_SPACE_HARDNESS (0 by default), we
    // can cover that here too, without needing to
    return 1 + (d->MAP(y + 1, x + 1).hardness / difference);
}

// Cost function for non-tunneling monsters. Regular monsters cannot go through rock, so a cell that is has infinite
// cost to the algorithm
static int regular_cost(const Dungeon_T *d, int y, int x) {
    return d->MAP(y + 1, x + 1).type == ROCK ? INT_MAX : 1;
}

// Helper function to check the neighbors of a cell in the main Dijkstra loop and process them. Cleans up the code and,
// frankly, probably not any slower, since the compiler is magic.
static void check_neighbor(Heap_T *h, const Dungeon_T *d, Vertex_T *dijkstra, int *cost, Vertex_T *v,
                           int y_dir, int x_dir, Dijkstra_T type) {

    // Bounds check y
    if (v->y + y_dir < 0 || d->height - 3 < v->y + y_dir) {
        return;
    }

    // Bound check x
    if (v->x + x_dir < 0 || d->width - 3 < v->x + x_dir) {
        return;
    }

    // Make sure the node is still on the heap
    if (DIJKSTRA(v->y + y_dir, v->x + x_dir).node.data == NULL) {
        return;
    }

    // Make sure movement cost isn't infinite
    if (DIJKSTRA(v->y + y_dir, v->x + x_dir).cost == INT_MAX) {
        return;
    }

    // Check if one of the two cells on approach are open... by using a compiler directive, we can avoid including this
    // if the setting is off
    #if DIAGONAL_NEEDS_OPEN_SPACE == true
    if (type == REGULAR_MAP && DIJKSTRA(v->y + y_dir, v->x).cost == INT_MAX &&
        DIJKSTRA(v->y, v->x + x_dir).cost == INT_MAX) {
        return;
    }
    #endif

    // Check if we actually found a new path
    if (COST_TRANSLATE(v->y + y_dir, v->x + x_dir) <=
        COST_TRANSLATE(v->y, v->x) + DIJKSTRA(v->y + y_dir, v->x + x_dir).cost) {
        return;
    }

    // Update our cost map if we get to this point; we found a new path
    COST_TRANSLATE(v->y + y_dir, v->x + x_dir) = COST_TRANSLATE(v->y, v->x) + DIJKSTRA(v->y + y_dir, v->x + x_dir).cost;
    heap_decrease_key(h, &DIJKSTRA(v->y + y_dir, v->x + x_dir).node, COST_TRANSLATE(v->y + y_dir, v->x + x_dir));
}

// Helper that does the heavy lifting of generating Dijkstra mappings. Given an already properly set up COST map, it
// starts by setting the cost function up to use the correct one specified. Then it moves onto initializing the heap and
// setting up the Dijkstra map and adding nodes to the heap. It only does the bare minimum here... the Dijkstra map does
// not include the dungeon borders, and only adds nodes to the heap that actually should be evaluated. Because the
// Dijkstra map is slightly smaller, there is a helper macro called COST_TRANSLATE() to convert Dijkstra map coordinates
// to output cost map coordinates. Additionally by having the Dijkstra map store the heap node itself, we only need one
// call to malloc(). After setting up the heap and maps, it start iterating through the heap, always going to the minimum
// node and evaluating it's neighbors, updating their cost in the cost map if a cheaper cost was found. When the heap is
// empty, it will clean up, and return to caller.
void dijkstra_helper(const Dungeon_T *d, int *cost, bool diagonal, Dijkstra_T type) {
    Heap_T *h;
    Vertex_T *dijkstra;
    int i, j;

    // By using a function pointer, we can adapt the Dijkstra's algorithm easily to do many types of maps without having
    // to adapt code or copy and paste. It's far more elegant.
    int (*cost_function)(const Dungeon_T *, int, int);

    // Determine what cost function we are using and set a pointer to use it. By using pointers to functions, we don't
    // have to deal with using the same function multiple times.
    switch (type) {
        case CORRIDOR_MAP:
            cost_function = &corridor_cost;
            break;
        case TUNNEL_MAP:
            cost_function = &tunnel_cost;
            break;
        case REGULAR_MAP:
            cost_function = &regular_cost;
            break;
        default:
            bail(INVALID_STATE, "FATAL ERROR! DIJKSTRA FUNCTION CALLED WITH IMPOSSIBLE ENUM TYPE!");
    }

    // Initialize our heap as an intrusive one (see pairing-heap.h for details)
    h = new_heap(true);

    // Build our Dijkstra map, which holds our vertices.
    dijkstra = safe_malloc((d->height - 2) * (d->width - 2) * sizeof(Vertex_T));
    for (i = 0; i < d->height - 2; i++) {
        for (j = 0; j < d->width - 2; j++) {

            // Set up our Dijkstra variables. By setting node.data to NULL, we can check if a vertex is on the heap
            // later, since the heap will update the data to point to the parent struct, and we always set data back
            // to null once we pull it off the heap.
            DIJKSTRA(i, j).y = i;
            DIJKSTRA(i, j).x = j;
            DIJKSTRA(i, j).cost = cost_function(d, i, j);
            DIJKSTRA(i, j).node.data = NULL;

            // Check what type of map we are building... if building a regular map, we only add nodes that are part of
            // the floor. Otherwise add all map nodes unconditionally. We already set the cost map above, so we don't
            // have to check for sources.
            if (type == REGULAR_MAP) {
                if (d->MAP(i + 1, j + 1).type != ROCK) {
                    heap_intrusive_insert(h, &DIJKSTRA(i, j).node, COST_TRANSLATE(i, j), &DIJKSTRA(i, j));
                }
            } else {
                heap_intrusive_insert(h, &DIJKSTRA(i, j).node, COST_TRANSLATE(i, j), &DIJKSTRA(i, j));
            }
        }
    }

    // Loop through our heap
    while (h->size > 0) {
        Vertex_T *v;

        // Pull the minimum off the heap for processing and set the node data to NULL to indicate it's not on the heap
        // anymore.
        v = heap_remove_min(h)->data;
        v->node.data = NULL;

        // If we pulled off a cell that still has infinite cost, then it means it is an unreachable cell
        if (COST_TRANSLATE(v->y, v->x) == INT_MAX) {
            continue;
        }

        // Go north
        check_neighbor(h, d, dijkstra, cost, v, -1, 0, type);

        // Go south
        check_neighbor(h, d, dijkstra, cost, v, 1, 0, type);

        // Go west
        check_neighbor(h, d, dijkstra, cost, v, 0, -1, type);

        // Go east
        check_neighbor(h, d, dijkstra, cost, v, 0, 1, type);

        // Only enter into these branches if the caller wants a diagonal map
        if (diagonal) {

            // Go northwest
            check_neighbor(h, d, dijkstra, cost, v, -1, -1, type);

            // Go northeast
            check_neighbor(h, d, dijkstra, cost, v, -1, 1, type);

            // Go southwest
            check_neighbor(h, d, dijkstra, cost, v, 1, -1, type);

            // Go southeast
            check_neighbor(h, d, dijkstra, cost, v, 1, 1, type);
        }
    }

    // Cleanup
    free(dijkstra);
    cleanup_heap(h);
}

// Helper to print to console the cost of each cell... should only be handed 0-9 ever. All it does is print out the
// number with its corresponding color.
static void print_cost_helper(int i) {
    printf("%s", COST_MAP_BACKGROUND);
    switch (i) {
        case 0:
            printf("%s%i", COST_0, 0);
            break;
        case 1:
            printf("%s%i", COST_1, 1);
            break;
        case 2:
            printf("%s%i", COST_2, 2);
            break;
        case 3:
            printf("%s%i", COST_3, 3);
            break;
        case 4:
            printf("%s%i", COST_4, 4);
            break;
        case 5:
            printf("%s%i", COST_5, 5);
            break;
        case 6:
            printf("%s%i", COST_6, 6);
            break;
        case 7:
            printf("%s%i", COST_7, 7);
            break;
        case 8:
            printf("%s%i", COST_8, 8);
            break;
        case 9:
            printf("%s%i", COST_9, 9);
            break;
        default:
            printf("%s ", CONSOLE_RESET);
    }

    printf("%s", CONSOLE_RESET);
}

// Generate dijkstra maps across the dungeon for any type necessary. sources refers an array of tuples representing the
// y and x of the various sources; diagonal is whether or not the algorithm can move diagonally, and type specifies
// which cost function the algorithm will use. It sets up the cost map, then passes it to the helper for the heavy
// lifting
int *generate_dijkstra_map(const Dungeon_T *d, int num_sources, const int *sources, bool diagonal, Dijkstra_T type) {
    int *cost;
    int i, j;

    // Allocate space for our output cost map
    cost = safe_malloc(d->height * d->width * sizeof(int));

    // Fill in our cost map
    for (i = 0; i < d->height; i++) {
        for (j = 0; j < d->width; j++) {
            COST(i, j) = INT_MAX;
        }
    }

    // Fill in our sources
    for (i = 0; i < num_sources; i++) {
        COST(sources[i * num_sources + 0], sources[i * num_sources + 1]) = 0;
    }

    // Build our map
    dijkstra_helper(d, cost, diagonal, type);

    return cost;
}

// See dijkstra.h
void generate_reverse_map(const Dungeon_T *d, int *cost, bool diagonal, Dijkstra_T type) {
    dijkstra_helper(d, cost, diagonal, type);
}

// See dijkstra.h
void print_dijkstra_map(const Dungeon_T *d, const int *cost, Dijkstra_T type) {
    int i, j;

    // First check if we have regular map... otherwise tunnel and corridor maps can be printed the same
    // By checking, it lets us add new types of cost maps in the future if we need safely
    if (type == REGULAR_MAP) {

        // Iterate through the regular cost map
        for (i = 0; i < d->height; i++) {
            for (j = 0; j < d->width; j++) {

                // Check if it's max cost, which means it's a rock, or a cell we couldn't get to. If both are not true
                // it's just a normal cell that can be printed normally.
                if (COST(i, j) == INT_MAX) {

                    // Check if it's not a rock... if it has infinite cost and isn't rock, it means a part of the
                    // dungeon that is disconnected from where the character is, and it's impossible to path-find to it.
                    // We want to print these differently than normal rock cells, which also have infinite cost.
                    if (d->MAP(i, j).type != ROCK) {
                        printf("%s%s%c%s", COST_MAP_BACKGROUND, COST_IMPOSSIBLE_COLOR, COST_IMPOSSIBLE, CONSOLE_RESET);
                    } else {
                        printf("%s%c%s", COST_INFINITE_COLOR, COST_INFINITE, CONSOLE_RESET);
                    }

                } else {

                    // We just have a normal cell here.
                    print_cost_helper(COST(i, j) % 10);
                }
            }

            // Print a new line after every row
            printf("\n");
        }

        // Print a new line after the last row to space multiple maps apart
        printf("\n");

    } else if (type == TUNNEL_MAP || type == CORRIDOR_MAP) {

        // Iterate through the cost map
        for (i = 0; i < d->height; i++) {
            for (j = 0; j < d->width; j++) {

                // Check if it's max cost, which means it's an immutable cell. If
                // both are not true it's just a normal cell that can be printed normally
                if (COST(i, j) == INT_MAX) {
                    printf("%s%c%s", COST_INFINITE_COLOR, COST_INFINITE, CONSOLE_RESET);

                } else {
                    print_cost_helper(COST(i, j) % 10);
                }
            }

            // Print a new line after every row
            printf("\n");
        }

        // Print a new line after the last row to space multiple maps apart
        printf("\n");

    }
}

