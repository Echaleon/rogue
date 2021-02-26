#include <stdio.h>
#include <stdlib.h>

#include "dungeon.h"
#include "dijkstra.h"
#include "../Character/character.h"
#include "../Helpers/helpers.h"
#include "../Helpers/stack.h"
#include "../Settings/dungeon-settings.h"
#include "../Settings/print-settings.h"
#include "../Settings/exit-codes.h"

// Private struct to store partitions. Same as rooms (for now), but separating them makes it easier to extend Room_T
// in the future, and keeps code more readable.
typedef struct Partition_ {
    int y, x, height, width;
} Partition_T;

// Returns a new partition. Disparate "initializer" functions are preferred but they can cause overhead
// in cases where heap space is already allocated to them. In this case, partitions are being put on a stack so a pointer
// works great
static Partition_T *new_partition(int y, int x, int height, int width) {
    Partition_T *p;
    p = safe_malloc(sizeof(Partition_T));
    p->y = y;
    p->x = x;
    p->height = height;
    p->width = width;
    return p;
}

// Determines if room parameters are valid in the given dungeon. Since rooms are contained in a partition, only the
// exterior walls need to be checked if they are within one tile of any other room.
static bool is_valid_room(const Dungeon_T *d, int y, int x, int height, int width) {
    int i;

    // Check left and right
    for (i = y - 1; i < y + height + 1; i++) {
        if (d->MAP(i, x - 1).type != ROCK || d->MAP(i, x + width).type != ROCK) {
            return false;
        }
    }

    // Check top and bottom
    for (i = x; i < x + width; i++) {
        if (d->MAP(y - 1, i).type != ROCK || d->MAP(y + height, i).type != ROCK) {
            return false;
        }
    }

    return true;
}

// Helper function that tries to fill a partition from the generate_rooms() function... if it can't fill a partition
// after FAILED_ROOM_PLACEMENT tries, it will return false, which will trigger generate_rooms() to return false, and
// trigger a dungeon generation failure and start the generation from scratch.
static bool fill_partition(Dungeon_T *d, const Partition_T *p) {
    int tries, y, x, height, width, i, j;

    tries = 0;

    // Try placing room randomly within valid range... we're just making sure rooms aren't touching here.
    do {

        // If we can't generate a room, cleanup and bail out.
        if (tries > FAILED_ROOM_PLACEMENT) {
            return false;
        }

        // Try to generate room parameters in range.
        y = rand_int_in_range(p->y, p->y + p->height - MIN_ROOM_HEIGHT);
        x = rand_int_in_range(p->x, p->x + p->height - MIN_ROOM_WIDTH);
        height = rand_int_in_range(MIN_ROOM_HEIGHT, p->y + p->height - y);
        width = rand_int_in_range(MIN_ROOM_WIDTH, p->x + p->height - x);

        tries++;

    } while (!is_valid_room(d, y, x, height, width));

    // Add the room to the dungeon.
    d->num_rooms++;
    d->rooms[d->num_rooms - 1].y = y;
    d->rooms[d->num_rooms - 1].x = x;
    d->rooms[d->num_rooms - 1].height = height;
    d->rooms[d->num_rooms - 1].width = width;

    // Paint the room into the dungeon
    for (i = y; i < y + height; i++) {
        for (j = x; j < x + width; j++) {
            d->MAP(i, j).type = ROOM;
            d->MAP(i, j).hardness = OPEN_SPACE_HARDNESS;
        }
    }

    return true;
}

// Heavy lifting function that fills that generate rooms in the dungeon. It does so using a binary space partition
// It starts by partitioning the dungeon. If the dimensions of a given partition are outside of
// PERCENTAGE_SPLIT_FORCE of each other, or one of the sides is in range for the allowed sizes for partitions
// it will split along the long side... otherwise it will randomly choose a direction to split along. It splits along
// a valid range, making sure both resultant partitions will be valid according to the minimums in setting.h. It then
// adds the child partitions to the stack if they need to be split more; otherwise it fills the partition with a room,
// bailing out if it can't fill one, and cleans up the partition. Returns true only if it was able to fill the dungeon
// successfully.
static bool generate_rooms(Dungeon_T *d, int max_rooms) {
    Stack_T *s;
    Partition_T *p;

    // Initialize our stack, and seed it with a partition
    s = new_stack();
    p = new_partition(1, 1, d->height - 2, d->width - 2);
    stack_push(s, p);

    // Loop through the entire stack
    while (s->top != -1) {
        int split;
        bool split_horizontal;
        Partition_T *left, *right;

        // Pop off the stack
        p = stack_pop(s);

        // Ugly ternary... checks if the dimension is within valid range, since we don't want to split that direction
        // then. If it's not, we split on if height or width is within PERCENTAGE_SPLIT_FORCE of each other. If all of
        // these are false, we pick a random direction. Cast to bool for safety... even though it will always be 0 or 1.
        split_horizontal = (bool)
                (MIN_PARTITION_HEIGHT <= p->height && p->height <= MAX_PARTITION_HEIGHT ? false : // height range
                 MIN_PARTITION_WIDTH <= p->width && p->width <= MAX_PARTITION_WIDTH ? true : // width range
                 p->height * (1 + PERCENTAGE_SPLIT_FORCE) < (float) p->width ? false : // height is much smaller
                 p->width * (1 + PERCENTAGE_SPLIT_FORCE) < (float) p->height ? true : // width is much smaller
                 rand() % 2); // NOLINT(cert-msc50-cpp)

        // Do the split
        if (split_horizontal) {
            split = rand_int_in_range(MIN_PARTITION_HEIGHT, p->height - MIN_PARTITION_HEIGHT);
            left = new_partition(p->y, p->x, split, p->width);
            right = new_partition(p->y + split, p->x, p->height - split, p->width);
        } else {
            split = rand_int_in_range(MIN_PARTITION_WIDTH, p->width - MIN_PARTITION_WIDTH);
            left = new_partition(p->y, p->x, p->height, split);
            right = new_partition(p->y, p->x + split, p->height, p->width - split);
        }

        // Check if the children partitions are valid to be split... if they are... add them to the stack to be split.
        // Otherwise it's a partition within range, and we can generate a room for it
        if (!(MIN_PARTITION_HEIGHT <= left->height && left->height <= MAX_PARTITION_HEIGHT) ||
            !(MIN_PARTITION_WIDTH <= left->width && left->width <= MAX_PARTITION_WIDTH)) {
            stack_push(s, left);
        } else {

            // If we exceed the max number of rooms or if a partition couldn't be filled, we can bail now
            if (d->num_rooms > max_rooms || !fill_partition(d, left)) {
                cleanup_stack(s);
                return false;
            }

            // Cleanup the left partition... it has a room now. We don't need it.
            free(left);
        }

        if (!(MIN_PARTITION_HEIGHT <= right->height && right->height <= MAX_PARTITION_HEIGHT) ||
            !(MIN_PARTITION_WIDTH <= right->width && right->width <= MAX_PARTITION_WIDTH)) {
            stack_push(s, right);
        } else {

            if (d->num_rooms > max_rooms || !fill_partition(d, right)) {
                cleanup_stack(s);
                return false;
            }

            free(right);
        }

        // Clean up our partition... after processing we don't need it any more.
        free(p);
    }

    // Cleanup
    cleanup_stack(s);
    return true;

}

// Helper function that iterates through the array, counts all cells that are rooms, and returns true if it is more than
// the provided percentage.
static bool is_room_percentage_covered(const Dungeon_T *d, float percentage_covered) {
    int i, j, count;

    count = 0;
    for (i = 0; i < d->height; i++) {
        for (j = 0; j < d->width; j++) {
            if (d->MAP(i, j).type == ROOM) {
                count++;
            }
        }
    }

    return ((count / (double) (d->height * d->width)) > percentage_covered);
}

// Randomizes the hardness across the dungeon, within the range provided in setting.h. It does not touch the exterior
// walls at all.
static void randomize_hardness(Dungeon_T *d) {
    int i, j;

    for (i = 1; i < d->height - 1; i++) {
        for (j = 1; j < d->width - 1; j++) {
            if (d->MAP(i, j).type == ROCK) {
                d->MAP(i, j).hardness += rand_int_in_range(MIN_ROCK_HARDNESS, MAX_ROCK_HARDNESS);
            }
        }
    }
}

// Heavy lifting function that paints corridors. Given the source pair and destination pair, it will call the
// dijkstra functions to create a new cost map from the source. It then starts at the destination and "rolls downhill".
// By starting at the destination and always visiting the cheapest neighbor node, we can generate a shortest path.
// There may be multiple, but it will always go in order of UP, DOWN, LEFT, and then RIGHT, if costs are equal.
// As it builds the path, it will paint any rock tiles to be corridors and set the hardness of those to be equal to
// the default hardness in setting.h
static void paint_corridor(Dungeon_T *d, int src_y, int src_x, int dst_y, int dst_x) {
    int *cost;
    int i, j;

    // Generate our cost map. See dijkstra.c
    cost = generate_dijkstra_map(d, src_y, src_x, false, CORRIDOR_MAP);

    // Starting at the destination, roll downhill to the cheapest node
    i = dst_y;
    j = dst_x;

    while (i != src_y || j != src_x) {
        int neighbor_cost;
        Direction_T direction;

        // Start with up
        direction = UP;
        neighbor_cost = COST(i - 1, j);

        if (neighbor_cost > COST(i + 1, j)) {
            neighbor_cost = COST(i + 1, j);
            direction = DOWN;
        }

        if (neighbor_cost > COST(i, j - 1)) {
            neighbor_cost = COST(i, j - 1);
            direction = LEFT;
        }

        // We don't care about setting cost, since this is the last neighbor to check
        if (neighbor_cost > COST(i, j + 1)) {
            direction = RIGHT;
        }

        // Travel to the chosen cell
        switch (direction) {  // NOLINT(hicpp-multiway-paths-covered)
            case UP:
                i -= 1;
                break;
            case DOWN:
                i += 1;
                break;
            case LEFT:
                j -= 1;
                break;
            case RIGHT:
                j += 1;
                break;
        }

        // Actually paint the corridor into the dungeon map
        if (d->MAP(i, j).type == ROCK) {
            d->MAP(i, j).type = CORRIDOR;
            d->MAP(i, j).hardness = OPEN_SPACE_HARDNESS;
        }
    }
    free(cost);
}

// Function that iterates through a random permutation of the rooms. Starting at the first room in the permutation
// it will connect it to the next, and so on, until it's connected all of them. The source and destination coordinates
// for paint_corridor() are set to be random points within the room itself for added variability.
static void generate_corridors(Dungeon_T *d) {
    int *shuffle;
    int i;

    // Generate a random permutation of the rooms
    shuffle = safe_malloc(d->num_rooms * sizeof(int));
    for (i = 0; i < d->num_rooms; i++) {
        shuffle[i] = i;
    }
    shuffle_int_array(shuffle, d->num_rooms);

    // Connect the rooms one to the next. Always generate in range so we don't have to bounds check.
    for (i = 0; i < d->num_rooms - 1; i++) {
        int src_y, src_x, dst_y, dst_x;

        // Generate pairs in each room
        src_y = rand_int_in_range(d->rooms[shuffle[i]].y, d->rooms[shuffle[i]].y + d->rooms[shuffle[i]].height - 1);
        src_x = rand_int_in_range(d->rooms[shuffle[i]].x, d->rooms[shuffle[i]].x + d->rooms[shuffle[i]].width - 1);

        dst_y = rand_int_in_range(d->rooms[shuffle[i + 1]].y,
                                  d->rooms[shuffle[i + 1]].y + d->rooms[shuffle[i + 1]].height - 1);
        dst_x = rand_int_in_range(d->rooms[shuffle[i + 1]].x,
                                  d->rooms[shuffle[i + 1]].x + d->rooms[shuffle[i + 1]].width - 1);

        paint_corridor(d, src_y, src_x, dst_y, dst_x);
    }

    // Cleanup our permutation array
    free(shuffle);
}

// Simply places an upward stair and a downwards stair within rooms in the dungeon, and not in the same room.
static void place_stairs(Dungeon_T *d) {
    int r1, r2, y, x;

    // Pick a room, then pick coordinates in that room, and paint
    r1 = rand_int_in_range(0, d->num_rooms - 1);
    y = rand_int_in_range(d->rooms[r1].y + 1, d->rooms[r1].y + d->rooms[r1].height - 2);
    x = rand_int_in_range(d->rooms[r1].x + 1, d->rooms[r1].x + d->rooms[r1].width - 2);
    d->MAP(y, x).type = STAIR_UP;

    // Pick a different room, then pick coordinates in the new room and paint
    do {
        r2 = rand_int_in_range(0, d->num_rooms - 1);
    } while (r1 == r2);
    y = rand_int_in_range(d->rooms[r2].y + 1, d->rooms[r2].y + d->rooms[r2].height - 2);
    x = rand_int_in_range(d->rooms[r2].x + 1, d->rooms[r2].x + d->rooms[r2].width - 2);
    d->MAP(y, x).type = STAIR_DOWN;
}

// Generates a new dungeon. It sets up the dungeon struct itself so it can call init_dungeon() and sets up the binary
// tree for the partition() function, it then calls the generate_rooms() function, which uses a BSP algorithm to ensure
// rooms are relatively spread out. If it fails to fill every partition, or makes too many rooms, it will return false
// as soon as it does. If it fails, makes too few rooms or doesn't cover enough of the map, it will retry, until it has
// failed too many times. When it generates a workable dungeon, it randomizes the hardness across the array, connects
// rooms, places stairs, places the PC, cleans up after itself, and returns a pointer to the full dungeon to the caller.
Dungeon_T *new_dungeon(int height, int width, int min_rooms, int max_rooms, float percentage_covered) {
    int tries;
    bool success;
    Dungeon_T *d;

    // Allocate our dungeon
    d = safe_malloc(sizeof(Dungeon_T));

    // Initialize all dungeon variable and draw the border
    init_dungeon(d, height, width);
    generate_dungeon_border(d);

    // Try generating our dungeon... if a partition can't be filled, or the dungeon isn't covered enough, or the
    // number of rooms isn't in range... try again.
    tries = 0;
    do {

        // Check to see if we failed after too many tries to generate a dungeon
        if (tries > FAILED_DUNGEON_GENERATION) {
            kill(DUNGEON_GENERATION_FAILURE,
                 "FATAL ERROR! FAILED TO GENERATE WORKABLE DUNGEON AFTER %i TRIES! TRY NEW PARAMETERS!\n",
                 FAILED_DUNGEON_GENERATION);
        }

        // Allocate space for room array. generate_rooms will bail if it makes too many and return false. realloc()
        // will act like malloc() if d-rooms =  NULL.
        d->rooms = safe_realloc(d->rooms, max_rooms * sizeof(Room_T));

        // Try generating rooms... if it makes more than max_rooms, it will bail as soon as it does, returning false
        // meaning we need to try again to generate new rooms.
        success = generate_rooms(d, max_rooms);
        tries++;

        // Try until we have filled every partition, the dungeon is sufficiently covered, and we have enough rooms
    } while (!success || !is_room_percentage_covered(d, percentage_covered) || min_rooms > d->num_rooms);

    // If we generate less rooms than max_rooms, shrink the array so as to not waste memory
    d->rooms = safe_realloc(d->rooms, d->num_rooms * sizeof(Room_T));

    // Finish up our generation
    randomize_hardness(d);
    generate_corridors(d);
    place_stairs(d);
    place_pc(d);

    return d;
}

// Initializes a dungeon's variables. Sets num_rooms to be zero, and rooms to NULL. This is the function to be sure to
// update if Cell_T is extended.
void init_dungeon(Dungeon_T *d, int height, int width) {
    int i, j;

    // Static values
    d->height = height;
    d->width = width;

    // These will be set later as part of generation
    d->num_rooms = 0;
    d->rooms = NULL;
    d->player = NULL;

    // Allocate our map array
    d->map = safe_malloc(height * width * sizeof(Cell_T));
    for (i = 0; i < d->height; i++) {
        for (j = 0; j < d->width; j++) {
            d->MAP(i, j).type = DEFAULT_CELL_TYPE;
            d->MAP(i, j).hardness = DEFAULT_HARDNESS;
            d->MAP(i, j).character = NULL;
        }
    }
}

// See dungeon.h
void generate_dungeon_border(Dungeon_T *d) {
    int i;

    // Left and right
    for (i = 1; i < d->height - 1; i++) {
        d->MAP(i, 0).hardness = IMMUTABLE_ROCK_HARDNESS;
        d->MAP(i, d->width - 1).hardness = IMMUTABLE_ROCK_HARDNESS;
    }

    // Top and bottom
    for (i = 0; i < d->width; i++) {
        d->MAP(0, i).hardness = IMMUTABLE_ROCK_HARDNESS;
        d->MAP(d->height - 1, i).hardness = IMMUTABLE_ROCK_HARDNESS;
    }
}

// See dungeon.h
void place_pc(Dungeon_T *d) {
    int room, y, x;

    // Pick room randomly, then pick random coordinate in that room and create the PC.
    room = rand_int_in_range(0, d->num_rooms - 1);
    y = rand_int_in_range(d->rooms[room].y, d->rooms[room].y + d->rooms[room].height - 1);
    x = rand_int_in_range(d->rooms[room].x, d->rooms[room].x + d->rooms[room].width - 1);
    d->player = new_character(d, y, x, PC_SYMBOL, PC_COLOR, true);
    d->MAP(y, x).character = d->player;
}

// See dungeon.h
void cleanup_dungeon(Dungeon_T *d) {
    int i, j;

    // Free any characters that aren't the player
    for (i = 0; i < d->height; i++) {
        for (j = 0; j < d->width; j++) {
            if (d->MAP(i, j).character != NULL && !d->MAP(i, j).character->player) {
                cleanup_character(d->MAP(i, j).character);
            }
        }
    }

    // Free the rest of our pointers
    cleanup_character(d->player);
    free(d->map);
    free(d->rooms);
    free(d);
}

// See dungeon.h
char cell_type_char(Cell_Type_T c) {
    switch (c) {
        case ROCK: // NOLINT(bugprone-branch-clone)
            return ROCK_CHAR;
        case ROOM:
            return ROOM_CHAR;
        case CORRIDOR:
            return CORRIDOR_CHAR;
        case STAIR_UP:
            return STAIR_UP_CHAR;
        case STAIR_DOWN:
            return STAIR_DOWN_CHAR;
        default:
            return '?';
    }
}

// See dungeon.h
char *cell_type_color(Cell_Type_T c) {
    switch (c) {
        case ROCK: // NOLINT(bugprone-branch-clone)
            return ROCK_COLOR;
        case ROOM:
            return ROOM_COLOR;
        case CORRIDOR:
            return CORRIDOR_COLOR;
        case STAIR_UP:
            return STAIR_UP_COLOR;
        case STAIR_DOWN:
            return STAIR_DOWN_COLOR;
        default:
            return CONSOLE_RESET;
    }
}

// See dungeon.h
char *cell_type_background(Cell_Type_T c) {
    switch (c) {
        case ROCK:
            return ROCK_BACKGROUND;
        case ROOM:
            return ROOM_BACKGROUND;
        case CORRIDOR:
            return CORRIDOR_BACKGROUND;
        case STAIR_UP: // NOLINT(bugprone-branch-clone)
            return STAIR_UP_BACKGROUND;
        case STAIR_DOWN:
            return STAIR_DOWN_BACKGROUND;
        default:
            return CONSOLE_RESET;
    }
}

// See dungeon.h
void print_dungeon(const Dungeon_T *d) {
    int i, j;

    for (i = 0; i < d->height; i++) {
        for (j = 0; j < d->width; j++) {

            // Check if there's a character. Otherwise print the cell.
            if (d->MAP(i, j).character != NULL) {
                printf("%s%s%c%s", cell_type_background(d->MAP(i, j).type), d->MAP(i, j).character->color,
                       d->MAP(i, j).character->symbol, CONSOLE_RESET);
            } else {
                printf("%s%s%c%s", cell_type_background(d->MAP(i, j).type), cell_type_color(d->MAP(i, j).type),
                       cell_type_char(d->MAP(i, j).type), CONSOLE_RESET);
            }
        }

        // Print a new line after every row
        printf("\n");
    }

    // Print a new line after the last row to space multiple maps apart
    printf("\n");
}



