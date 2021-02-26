#include <limits.h>
#include <stdlib.h>

#include "dijkstra.h"
#include "../Dungeon/dungeon.h"
#include "../Helpers/helpers.h"
#include "../Helpers/pairing-heap.h"
#include "../Settings/exit-codes.h"
#include "../Settings/dungeon-settings.h"
#include "../Settings/misc-settings.h"

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

    // Only needs to be initialized once... its a compile time constant.
    static int difference = (MAX_ROCK_HARDNESS - MIN_ROCK_HARDNESS) / CORR_NUM_HARDNESS_LEVELS;

    // Infinite cost if we can't go here.
    if (d->MAP(y + 1, x + 1).hardness == IMMUTABLE_ROCK_HARDNESS) {
        return INT_MAX;
    }

    // Different cell types have different weights for our corridor map... produces more interesting corridors this way
    switch (d->MAP(y + 1, x + 1).type) {
        case ROCK:
            // Checks if we are using hardness... if we are it divides the hardness by a difference to split it into
            // multiple ranges. If there is remainder, we just add it onto the top.
            return USE_HARDNESS_FOR_CORRIDORS ? d->MAP(y + 1, x + 1).hardness / difference > CORR_NUM_HARDNESS_LEVELS ?
                                                CORR_NUM_HARDNESS_LEVELS + 1 :
                                                d->MAP(y + 1, x + 1).hardness / difference + 1 : 1 + CORR_ROCK_WEIGHT;
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

// Generate dijkstra maps across the dungeon for any type necessary. y and x are the source, diagonal is whether or not
// the algorithm can move diagonally, and type specifies which cost function the algorithm will use. It starts by
// setting the cost function up to use the correct one specified. Next it initializes the heap, and the output cost map,
// then moves onto setting up the Dijkstra map and adding nodes to the heap. It only does the bare minimum here...
// the Dijkstra map does not include the dungeon borders, and only adds nodes to the heap that actually should be
// evaluated. Because the Dijkstra map is slightly smaller, there is a helper macro called COST_TRANSLATE() to convert
// Dijkstra map coordinates to output cost map coordinates. Additionally by having the Dijkstra map store the heap node
// itself, we only need one call to malloc(). After setting up the heap and maps, it start iterating through the heap,
// always going to the minimum node and evaluating it's neighbors, updating their cost in the cost map if a cheaper
// cost was found. When the heap is empty, it will clean up, and return the bare cost map to minimize overhead.
int *generate_dijkstra_map(const Dungeon_T *d, int y, int x, bool diagonal, Dijkstra_T type) {
    Heap_T *h;
    Vertex_T *dijkstra;
    int *cost;
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
            kill(INVALID_STATE, "FATAL ERROR! DIJKSTRA FUNCTION CALLED WITH IMPOSSIBLE ENUM TYPE!");
    }

    // Initialize our heap as an intrusive one (see pairing-heap.h for details)
    h = new_heap(true);

    // Allocate space for our output cost map
    cost = safe_malloc(d->height * d->width * sizeof(int));

    // Fill the left and right of the cost map -- this corresponds to the immutable dungeon border
    for (i = 1; i < d->height - 1; i++) {
        COST(i, 0) = INT_MAX;
        COST(i, d->width - 1) = INT_MAX;
    }

    // Fill the top and bottom of the cost map -- this corresponds to the immutable dungeon border
    for (i = 0; i < d->width; i++) {
        COST(0, i) = INT_MAX;
        COST(d->height - 1, i) = INT_MAX;
    }

    // Start building our Dijkstra map, which holds our vertices.
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

            // Check if the current Dijkstra coordinates correspond to our source point. We have to translate them to
            // be the full dungeon map coordinates.  If these are the source point, insert it to the heap with a key of
            // 0 and set the cost to 0 so it will be the first one out when we get to our heap. Otherwise we just set
            // the cost of the cell to infinite.
            if (i + 1 == y && j + 1 == x) {
                COST_TRANSLATE(i, j) = 0;
                heap_intrusive_insert(h, &DIJKSTRA(i, j).node, 0, &DIJKSTRA(i, j));
                continue;

            } else {
                COST_TRANSLATE(i, j) = INT_MAX;
            }

            // Check if we are building a regular map or not. If we are building a regular map, then we only need to add
            // cells that are not in rock. If it's not a regular map, then we just add cells as long as the cell isn't
            // immutable.
            if (type == REGULAR_MAP) {
                
                if (d->MAP(i + 1, j + 1).type != ROCK) {
                    heap_intrusive_insert(h, &DIJKSTRA(i, j).node, INT_MAX, &DIJKSTRA(i, j));
                }

            } else if (d->MAP(i + 1, j + 1).hardness != IMMUTABLE_ROCK_HARDNESS) {
                heap_intrusive_insert(h, &DIJKSTRA(i, j).node, INT_MAX, &DIJKSTRA(i, j));
            }
        }
    }

    // Loop through our heap
    while (h->size > 0) {
        Vertex_T *v;
        int self;

        // Pull the minimum off the heap for processing and set the node data to NULL to indicate it's not on the heap
        // anymore.
        v = heap_remove_min(h)->data;
        v->node.data = NULL;

        // Only translate cost once to simplify code
        self = COST_TRANSLATE(v->y, v->x);

        // If we pulled off a cell that still has infinite cost, then it means it is an unreachable cell
        if (self == INT_MAX) {
            continue;
        }

        // Only going to comment one of these heavily since they are all just the same checking different directions...
        // By having them separate if statements the code will be faster but longer. The order of statements is:
        // 1. Bounds check
        // 2. Check that the vertex is still on the heap by checking if node.data is NULL, since we always set it to
        //    NULL when pulling it off the stack.
        // 3. Check that the movement cost of the vertex is not infinite
        // 4. Finally check if the total cost stored for the vertex is less than the current cost plus the vertex's
        //    movement cost.
        // If all of these are true, the cost map is updated, and the corresponding heap node is updated as well
        //
        // Go north
        if (v->y - 1 >= 0 && DIJKSTRA(v->y - 1, v->x).node.data != NULL && DIJKSTRA(v->y - 1, v->x).cost != INT_MAX &&
            COST_TRANSLATE(v->y - 1, v->x) > self + DIJKSTRA(v->y - 1, v->x).cost) {
            COST_TRANSLATE(v->y - 1, v->x) = self + DIJKSTRA(v->y - 1, v->x).cost;
            heap_decrease_key(h, &DIJKSTRA(v->y - 1, v->x).node, COST_TRANSLATE(v->y - 1, v->x));

        }

        // Go south
        if (v->y + 1 < d->height - 2 && DIJKSTRA(v->y + 1, v->x).node.data != NULL &&
            DIJKSTRA(v->y + 1, v->x).cost != INT_MAX &&
            COST_TRANSLATE(v->y + 1, v->x) > self + DIJKSTRA(v->y + 1, v->x).cost) {
            COST_TRANSLATE(v->y + 1, v->x) = self + DIJKSTRA(v->y + 1, v->x).cost;
            heap_decrease_key(h, &DIJKSTRA(v->y + 1, v->x).node, COST_TRANSLATE(v->y + 1, v->x));
        }

        // Go west
        if (v->x - 1 >= 0 && DIJKSTRA(v->y, v->x - 1).node.data != NULL && DIJKSTRA(v->y, v->x - 1).cost != INT_MAX &&
            COST_TRANSLATE(v->y, v->x - 1) > self + DIJKSTRA(v->y, v->x - 1).cost) {
            COST_TRANSLATE(v->y, v->x - 1) = self + DIJKSTRA(v->y, v->x - 1).cost;
            heap_decrease_key(h, &DIJKSTRA(v->y, v->x - 1).node, COST_TRANSLATE(v->y, v->x - 1));
        }

        // Go east
        if (v->x + 1 < d->width - 2 && DIJKSTRA(v->y, v->x + 1).node.data != NULL &&
            DIJKSTRA(v->y, v->x + 1).cost != INT_MAX &&
            COST_TRANSLATE(v->y, v->x + 1) > self + DIJKSTRA(v->y, v->x + 1).cost) {
            COST_TRANSLATE(v->y, v->x + 1) = self + DIJKSTRA(v->y, v->x + 1).cost;
            heap_decrease_key(h, &DIJKSTRA(v->y, v->x + 1).node, COST_TRANSLATE(v->y, v->x + 1));
        }

        // Only enter into these branches if the caller wants a diagonal map
        if (diagonal) {

            // This branch handles regular monsters, if DIAGONAL_NEEDS_OPEN_SPACE is set to true, because then monsters
            // can't move diagonally without the cardinal direction being open as well.
            if (DIAGONAL_NEEDS_OPEN_SPACE && type == REGULAR_MAP) {

                // Go northwest
                if (v->y - 1 >= 0 && v->x - 1 >= 0 &&
                    (DIJKSTRA(v->y - 1, v->x).cost != INT_MAX || DIJKSTRA(v->y, v->x - 1).cost != INT_MAX) &&
                    DIJKSTRA(v->y - 1, v->x - 1).node.data != NULL && DIJKSTRA(v->y - 1, v->x - 1).cost != INT_MAX &&
                    COST_TRANSLATE(v->y - 1, v->x - 1) > self + DIJKSTRA(v->y - 1, v->x - 1).cost) {
                    COST_TRANSLATE(v->y - 1, v->x - 1) = self + DIJKSTRA(v->y, v->x).cost;
                    heap_decrease_key(h, &DIJKSTRA(v->y - 1, v->x - 1).node, COST_TRANSLATE(v->y - 1, v->x - 1));
                }

                // Go northeast
                if (v->y - 1 >= 0 && v->x + 1 < d->width - 2 &&
                    (DIJKSTRA(v->y - 1, v->x).cost != INT_MAX || DIJKSTRA(v->y, v->x + 1).cost != INT_MAX) &&
                    DIJKSTRA(v->y - 1, v->x + 1).node.data != NULL && DIJKSTRA(v->y - 1, v->x + 1).cost != INT_MAX &&
                    COST_TRANSLATE(v->y - 1, v->x + 1) > self + DIJKSTRA(v->y - 1, v->x + 1).cost) {
                    COST_TRANSLATE(v->y - 1, v->x + 1) = self + DIJKSTRA(v->y - 1, v->x + 1).cost;
                    heap_decrease_key(h, &DIJKSTRA(v->y - 1, v->x + 1).node, COST_TRANSLATE(v->y - 1, v->x + 1));
                }

                // Go southwest
                if (v->y + 1 < d->height - 2 && v->x - 1 >= 0 &&
                    (DIJKSTRA(v->y + 1, v->x).cost != INT_MAX || DIJKSTRA(v->y, v->x - 1).cost != INT_MAX) &&
                    DIJKSTRA(v->y + 1, v->x - 1).node.data != NULL && DIJKSTRA(v->y + 1, v->x - 1).cost != INT_MAX &&
                    COST_TRANSLATE(v->y + 1, v->x - 1) > self + DIJKSTRA(v->y + 1, v->x - 1).cost) {
                    COST_TRANSLATE(v->y + 1, v->x - 1) = self + DIJKSTRA(v->y + 1, v->x - 1).cost;
                    heap_decrease_key(h, &DIJKSTRA(v->y + 1, v->x - 1).node, COST_TRANSLATE(v->y + 1, v->x - 1));
                }

                // Go southeast
                if (v->y + 1 < d->height - 2 && v->x + 1 < d->width - 2 &&
                    (DIJKSTRA(v->y + 1, v->x).cost != INT_MAX || DIJKSTRA(v->y, v->x + 1).cost != INT_MAX) &&
                    DIJKSTRA(v->y + 1, v->x + 1).node.data != NULL && DIJKSTRA(v->y + 1, v->x + 1).cost != INT_MAX &&
                    COST_TRANSLATE(v->y + 1, v->x + 1) > self + DIJKSTRA(v->y + 1, v->x + 1).cost) {
                    COST_TRANSLATE(v->y + 1, v->x + 1) = self + DIJKSTRA(v->y + 1, v->x + 1).cost;
                    heap_decrease_key(h, &DIJKSTRA(v->y + 1, v->x + 1).node, COST_TRANSLATE(v->y + 1, v->x + 1));
                }

            } else {

                // Go northwest
                if (v->y - 1 >= 0 && v->x - 1 >= 0 && DIJKSTRA(v->y - 1, v->x - 1).node.data != NULL &&
                    DIJKSTRA(v->y - 1, v->x - 1).cost != INT_MAX &&
                    COST_TRANSLATE(v->y - 1, v->x - 1) > self + DIJKSTRA(v->y - 1, v->x - 1).cost) {
                    COST_TRANSLATE(v->y - 1, v->x - 1) = self + DIJKSTRA(v->y, v->x).cost;
                    heap_decrease_key(h, &DIJKSTRA(v->y - 1, v->x - 1).node, COST_TRANSLATE(v->y - 1, v->x - 1));
                }

                // Go northeast
                if (v->y - 1 >= 0 && v->x + 1 < d->width - 2 && DIJKSTRA(v->y - 1, v->x + 1).node.data != NULL &&
                    DIJKSTRA(v->y - 1, v->x + 1).cost != INT_MAX &&
                    COST_TRANSLATE(v->y - 1, v->x + 1) > self + DIJKSTRA(v->y - 1, v->x + 1).cost) {
                    COST_TRANSLATE(v->y - 1, v->x + 1) = self + DIJKSTRA(v->y - 1, v->x + 1).cost;
                    heap_decrease_key(h, &DIJKSTRA(v->y - 1, v->x + 1).node, COST_TRANSLATE(v->y - 1, v->x + 1));
                }

                // Go southwest
                if (v->y + 1 < d->height - 2 && v->x - 1 >= 0 && DIJKSTRA(v->y + 1, v->x - 1).node.data != NULL &&
                    DIJKSTRA(v->y + 1, v->x - 1).cost != INT_MAX &&
                    COST_TRANSLATE(v->y + 1, v->x - 1) > self + DIJKSTRA(v->y + 1, v->x - 1).cost) {
                    COST_TRANSLATE(v->y + 1, v->x - 1) = self + DIJKSTRA(v->y + 1, v->x - 1).cost;
                    heap_decrease_key(h, &DIJKSTRA(v->y + 1, v->x - 1).node, COST_TRANSLATE(v->y + 1, v->x - 1));
                }

                // Go southeast
                if (v->y + 1 < d->height - 2 && v->x + 1 < d->width - 2 &&
                    DIJKSTRA(v->y + 1, v->x + 1).node.data != NULL &&
                    DIJKSTRA(v->y + 1, v->x + 1).cost != INT_MAX &&
                    COST_TRANSLATE(v->y + 1, v->x + 1) > self + DIJKSTRA(v->y + 1, v->x + 1).cost) {
                    COST_TRANSLATE(v->y + 1, v->x + 1) = self + DIJKSTRA(v->y + 1, v->x + 1).cost;
                    heap_decrease_key(h, &DIJKSTRA(v->y + 1, v->x + 1).node, COST_TRANSLATE(v->y + 1, v->x + 1));
                }
            }
        }
    }

    // Cleanup
    free(dijkstra);
    cleanup_heap(h);
    return cost;
}
