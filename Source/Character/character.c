#include <stdlib.h>
#include <limits.h>


#include "character.h"

#include "Dungeon/dijkstra.h"
#include "Dungeon/dungeon.h"
#include "Helpers/helpers.h"
#include "Settings/character-settings.h"
#include "Settings/exit-codes.h"
#include "Settings/print-settings.h"

// Enum for possible neighbor directions. Not necessary, but helps avoid programming errors.
typedef enum Direction_E {
    NORTH, SOUTH, WEST, EAST, NORTHWEST, NORTHEAST, SOUTHWEST, SOUTHEAST, STUCK
} Direction_T;

// Helper to determine if a monster can see the player
static bool can_see_player(Character_T *c) {
    Dungeon_T *d;
    int delta_y, delta_x, dir_x, dir_y, error, i, j, r;

    // Set up our variables
    d = c->d;

    delta_y = abs(c->y - d->player->y);
    delta_x = abs(c->x - d->player->x);

    dir_y = d->player->y > c->y ? 1 : -1;
    dir_x = d->player->x > c->x ? 1 : -1;
    error = delta_y - delta_x;

    i = c->y;
    j = c->x;

    r = 1 + delta_y + delta_x;
    delta_y *= 2;
    delta_x *= 2;

    // Travel towards the player
    while (r > 0) {
        if (d->MAP(i, j).type == ROCK) {
            return false;
        }

        // Error keeps track of which cells we are moving through.
        if (error > 0) {
            i += dir_y;
            error -= delta_y;
        } else {
            j += dir_x;
            error += delta_x;
        }

        r++;
    }

    return true;
}

// Helper that determines a random move for a monster
static Direction_T calculate_random_move(Character_T *c) {
    Dungeon_T *d;
    Direction_T directions[8];
    int possible_count;

    // Define our dungeon as a variable so we can use macros
    d = c->d;

    possible_count = 0;

    // Start figuring out what directions we can go
    if (c->y - 1 > 0 && (c->behavior & TUNNELER || d->MAP(c->y - 1, c->x).type != ROCK)) {
        possible_count++;
        directions[possible_count - 1] = NORTH;
    }

    // Check south
    if (c->y + 1 < d->height && (c->behavior & TUNNELER || d->MAP(c->y + 1, c->x).type != ROCK)) {
        possible_count++;
        directions[possible_count - 1] = SOUTH;
    }

    // Check west
    if (c->x - 1 > 0 && (c->behavior & TUNNELER || d->MAP(c->y, c->x - 1).type != ROCK)) {
        possible_count++;
        directions[possible_count - 1] = WEST;
    }

    // Check east
    if (c->x + 1 < d->width && (c->behavior & TUNNELER || d->MAP(c->y, c->x + 1).type != ROCK)) {
        possible_count++;
        directions[possible_count - 1] = EAST;
    }

    // Only emit diagonal code if characters can travel diagonally
    #if CHARACTER_DIAGONAL_TRAVEL == true

    // Check northwest
    if (c->y - 1 > 0 && c->x - 1 > 0 && (c->behavior & TUNNELER || d->MAP(c->y - 1, c->x - 1).type != ROCK)) {
        possible_count++;
        directions[possible_count - 1] = NORTHWEST;
    }

    // Check northeast
    if (c->y - 1 > 0 && c->x + 1 < d->width && (c->behavior & TUNNELER || d->MAP(c->y - 1, c->x + 1).type != ROCK)) {
        possible_count++;
        directions[possible_count - 1] = NORTHEAST;
    }

    // Check southwest
    if (c->y + 1 < d->height && c->x - 1 > 0 && (c->behavior & TUNNELER || d->MAP(c->y + 1, c->x - 1).type != ROCK)) {
        possible_count++;
        directions[possible_count - 1] = SOUTHWEST;
    }

    // Check southwest. Don't need to save cost because it's just used as a transient.
    if (c->y + 1 < d->height && c->x + 1 < d->width &&
        (c->behavior & TUNNELER || d->MAP(c->y + 1, c->x + 1).type != ROCK)) {
        possible_count++;
        directions[possible_count - 1] = SOUTHEAST;
    }
    #endif

    // If no direction was found, the monster is stuck
    if (possible_count == 0) {
        return STUCK;
    }

    // Pick a random direction of the ones we have figured out we can move to
    return directions[rand_int_in_range(0, possible_count - 1)];
}

static Direction_T calculate_intelligent_monster_move(Character_T *c) {
    Dungeon_T *d;
    int *cost;
    Direction_T direction;
    int best_cost;

    // Define our dungeon and cost map as a variable so we can use macros.
    d = c->d;
    cost = c->cost;

    // Move randomly if they are erratic
    if (c->behavior & ERRATIC && rand() % 2) { // NOLINT(cert-msc50-cpp)
        return calculate_random_move(c);
    }

    // Initialize our variables for finding the best node
    direction = STUCK;
    best_cost = INT_MAX;

    // Start with north
    if (c->y - 1 > 0 && COST(c->y - 1, c->x) < best_cost) {
        direction = NORTH;
        best_cost = COST(c->y - 1, c->x);
    }

    // Check south
    if (c->y + 1 < d->height && COST(c->y + 1, c->x) < best_cost) {
        direction = SOUTH;
        best_cost = COST(c->y + 1, c->x);
    }

    // Check west
    if (c->x - 1 > 0 && COST(c->y, c->x - 1) < best_cost) {
        direction = WEST;
        best_cost = COST(c->y, c->x - 1);
    }

    // Check east
    if (c->x + 1 < d->width && COST(c->y, c->x + 1) < best_cost) {
        direction = EAST;
        best_cost = COST(c->y, c->x + 1);
    }

    // Only emit diagonal code if characters can travel diagonally
    #if CHARACTER_DIAGONAL_TRAVEL == true

    // Check northwest
    if (c->y - 1 > 0 && c->x - 1 > 0 && COST(c->y - 1, c->x - 1) < best_cost) {
        direction = NORTHWEST;
        best_cost = COST(c->y - 1, c->x - 1);
    }

    // Check northeast
    if (c->y - 1 > 0 && c->x + 1 < d->width && COST(c->y - 1, c->x + 1) < best_cost) {
        direction = NORTHEAST;
        best_cost = COST(c->y - 1, c->x + 1);
    }

    // Check southwest
    if (c->y + 1 < d->height && c->x - 1 > 0 && COST(c->y + 1, c->x - 1) < best_cost) {
        direction = SOUTHWEST;
        best_cost = COST(c->y + 1, c->x - 1);
    }

    // Check southwest. Don't need to save cost because it's just used as a transient.
    if (c->y + 1 < d->height && c->x + 1 < d->width && COST(c->y + 1, c->x + 1) < best_cost) {
        direction = SOUTHEAST;
    }
    #endif

    // Start checking the other directions
    return direction;
}

static Direction_T calculate_unintelligent_monster_move(Character_T *c) {
    Dungeon_T *d;
    Direction_T direction;
    int best_cost;

    // Define our dungeon as a variable so we can use macros
    d = c->d;

    // Move randomly if they are erratic
    if (c->behavior & ERRATIC && rand() % 2) { // NOLINT(cert-msc50-cpp)
        return calculate_random_move(c);
    }

    // Initialize our variables for finding the best node
    direction = STUCK;
    best_cost = INT_MAX;

    // Start with north
    if (c->y - 1 > 0 && (c->behavior & TUNNELER || d->MAP(c->y - 1, c->x).type != ROCK)) {
        direction = NORTH;
        best_cost = manhattan_distance(c->y - 1, c->x, d->player->y, d->player->x);
    }

    // Check south
    if (c->y + 1 < d->height && (c->behavior & TUNNELER || d->MAP(c->y + 1, c->x).type != ROCK)) {
        int cost = manhattan_distance(c->y + 1, c->x, d->player->y, d->player->x);
        if (cost < best_cost) {
            direction = SOUTH;
            best_cost = cost;
        }
    }

    // Check west
    if (c->x - 1 > 0 && (c->behavior & TUNNELER || d->MAP(c->y, c->x - 1).type != ROCK)) {
        int cost = manhattan_distance(c->y, c->x - 1, d->player->y, d->player->x);
        if (cost < best_cost) {
            direction = WEST;
            best_cost = cost;
        }
    }

    // Check east
    if (c->x + 1 < d->width && (c->behavior & TUNNELER || d->MAP(c->y, c->x + 1).type != ROCK)) {
        int cost = manhattan_distance(c->y, c->x + 1, d->player->y, d->player->x);
        if (cost < best_cost) {
            direction = EAST;
            best_cost = cost;
        }
    }

    // Only emit diagonal code if characters can travel diagonally
    #if CHARACTER_DIAGONAL_TRAVEL == true

    // Check northwest
    if (c->y - 1 > 0 && c->x - 1 > 0 && (c->behavior & TUNNELER || d->MAP(c->y - 1, c->x - 1).type != ROCK)) {
        int cost = manhattan_distance(c->y - 1, c->x - 1, d->player->y, d->player->x);
        if (cost < best_cost) {
            direction = NORTHWEST;
            best_cost = cost;
        }
    }

    // Check northeast
    if (c->y - 1 > 0 && c->x + 1 < d->width && (c->behavior & TUNNELER || d->MAP(c->y - 1, c->x + 1).type != ROCK)) {
        int cost = manhattan_distance(c->y - 1, c->x + 1, d->player->y, d->player->x);
        if (cost < best_cost) {
            direction = NORTHEAST;
            best_cost = cost;
        }
    }

    // Check southwest
    if (c->y + 1 < d->height && c->x - 1 > 0 && (c->behavior & TUNNELER || d->MAP(c->y + 1, c->x - 1).type != ROCK)) {
        int cost = manhattan_distance(c->y + 1, c->x - 1, d->player->y, d->player->x);
        if (cost < best_cost) {
            direction = SOUTHWEST;
            best_cost = cost;
        }
    }

    // Check southwest. Don't need to save cost because it's just used as a transient.
    if (c->y + 1 < d->height && c->x + 1 < d->width &&
        (c->behavior & TUNNELER || d->MAP(c->y + 1, c->x + 1).type != ROCK)) {
        if (manhattan_distance(c->y + 1, c->x + 1, d->player->y, d->player->x) < best_cost) {
            direction = SOUTHEAST;
        }
    }
    #endif

    return direction;
}

// Helper that actually moves the monster in the direction given, and can do so without bounds checks or checking if the
// move is valid, as the calculate_monster_moves() functions take care of that. Just need to make sure to rebuild cost
// maps for the dungeons as necessary, and figure out if we killed another character
static Character_T *do_character_move(Character_T *c, Direction_T direction) {
    Dungeon_T *d;
    Character_T *killed;
    int y, x;

    // Define our dungeon as a variable so we can use macros
    d = c->d;

    // Start by building our new coordinates, or returning for the special case the monster is stuck
    switch (direction) { // NOLINT(hicpp-multiway-paths-covered)
        case NORTH:
            y = c->y - 1;
            x = c->x;
            break;
        case SOUTH:
            y = c->y + 1;
            x = c->x;
            break;
        case WEST:
            y = c->y;
            x = c->x - 1;
            break;
        case EAST:
            y = c->y;
            x = c->x + 1;
            break;
        case NORTHWEST:
            y = c->y - 1;
            x = c->x - 1;
            break;
        case NORTHEAST:
            y = c->y - 1;
            x = c->x + 1;
            break;
        case SOUTHWEST:
            y = c->y + 1;
            x = c->x - 1;
            break;
        case SOUTHEAST:
            y = c->y + 1;
            x = c->x + 1;
            break;
        case STUCK:
            return NULL;
        default:
            bail(INVALID_STATE, "FATAL ERROR! DIJKSTRA FUNCTION CALLED WITH IMPOSSIBLE ENUM TYPE!");
    }

    // Assign the character in the target cell for returning later
    killed = d->MAP(y, x).character;

    // Check if the monster is trying to move into a cell where the hardness is not zero. If it is, subtract 85, and
    // move only if the monster can, rebuilding cost maps as necessary. Otherwise... don't move at all.
    if (d->MAP(y, x).hardness != 0) {
        d->MAP(y, x).hardness = d->MAP(y, x).hardness - 85 > 0 ? d->MAP(y, x).hardness - 85 : 0;

        if (d->MAP(y, x).hardness == 0) {
            d->MAP(y, x).type = CORRIDOR;

            // Rebuild both cost maps since the dungeon was changed
            build_dungeon_cost_maps(d, true, true);

            // Update the dungeon
            d->MAP(y, x).character = c;
            d->MAP(c->y, c->x).character = NULL;
            c->y = y;
            c->x = x;
        } else {
            build_dungeon_cost_maps(d, false, true);
        }
    } else {

        // Update the dungeon
        d->MAP(y, x).character = c;
        d->MAP(c->y, c->x).character = NULL;
        c->y = y;
        c->x = x;
    }

    return killed;
}


// See character.h
Character_T *new_character(Dungeon_T *d, int y, int x, int speed, int behavior, char symbol, char *color, bool player) {
    Character_T *c = safe_malloc(sizeof(Character_T));
    c->d = d;
    c->y = y;
    c->x = x;
    c->last_y = -1;
    c->last_x = -1;
    c->speed = speed;
    c->behavior = behavior;
    c->symbol = symbol;
    c->color = color;
    c->player = player;
    c->cost = NULL;
    return c;
}

// See character.h
void build_character_cost_map(Character_T *c, int num_sources, int sources[][2]) {
    free(c->cost);
    c->cost = generate_dijkstra_map(c->d, num_sources, (int *) sources, CHARACTER_DIAGONAL_TRAVEL,
                                    c->behavior & TUNNELER ? TUNNEL_MAP : REGULAR_MAP);
}

Character_T *move_monster(Character_T *c) {
    Dungeon_T *d = c->d;
    Direction_T direction;
    Character_T *killed;

    // If the monster is intelligent and telepathic, we can just use the current dungeon cost map
    if (c->behavior & INTELLIGENT && c->behavior & TELEPATHIC) {
        c->cost = c->behavior & TUNNELER ? d->tunnel_cost : d->regular_cost;
        direction = calculate_intelligent_monster_move(c);
        killed = do_character_move(c, direction);
        c->cost = NULL;
        return killed;
    }

    // If the monster is intelligent we need to do some checking. First see if the monster can see the player; if it can
    // we just use the dungeon cost map and update the last seen position. If not, we check the edge cases where it's
    // the monster's first turn, or it's already at the spot where it last saw the PC. Otherwise, we build a cost map
    // around the last seen spot, and move towards there.
    if (c->behavior & INTELLIGENT) {
        if (can_see_player(c)) {
            c->last_y = d->player->y;
            c->last_x = d->player->x;
            c->cost = c->behavior & TUNNELER ? d->tunnel_cost : d->regular_cost;
            direction = calculate_intelligent_monster_move(c);
            killed = do_character_move(c, direction);
            c->cost = NULL;
            return killed;

        } else if (c->last_y == -1 && c->last_x == -1) {
            c->last_y = c->y;
            c->last_x = c->x;
            return NULL;

        } else if (c->y == c->last_y && c->x == c->last_x) {
            return NULL;

        } else {

            // We have to build a cost map always, dungeons may change since the last time the character was up.
            int sources[1][2];

            sources[0][0] = c->last_y;
            sources[0][1] = c->last_x;

            build_character_cost_map(c, 1, sources);

            direction = calculate_intelligent_monster_move(c);
            return do_character_move(c, direction);
        }
    }

    // If the monster is unintelligent but telepathic, or can see the player, move in a straight line towards them
    if (c->behavior & TELEPATHIC || can_see_player(c)) {
        direction = calculate_unintelligent_monster_move(c);
        return do_character_move(c, direction);
    }

    // Monster needs to wander now
    direction = calculate_random_move(c);
    return do_character_move(c, direction);
}

// See character.h
Character_T *move_player(Character_T *c) {
    return NULL;
}

// See character.h
int find_player_room(Dungeon_T *d, Character_T *c) {
    int r;

    // Iterate through our rooms, searching them one by one
    for (r = 0; r < d->num_rooms; r++) {
        int i, j;

        for (i = d->rooms[r].y; i < d->rooms[r].y + d->rooms[r].height; i++) {
            for (j = d->rooms[r].x; j < d->rooms[r].x + d->rooms[r].width; j++) {
                if (d->MAP(i, j).character == c) {
                    return r;
                }
            }
        }
    }

    return -1;
}

// See character.h
char monster_behavior_char(int behavior) {
    switch (behavior) {
        case 0:
            return MONSTER_0_CHAR;
        case 1:
            return MONSTER_1_CHAR;
        case 2:
            return MONSTER_2_CHAR;
        case 3:
            return MONSTER_3_CHAR;
        case 4:
            return MONSTER_4_CHAR;
        case 5:
            return MONSTER_5_CHAR;
        case 6:
            return MONSTER_6_CHAR;
        case 7:
            return MONSTER_7_CHAR;
        case 8:
            return MONSTER_8_CHAR;
        case 9:
            return MONSTER_9_CHAR;
        case 10:
            return MONSTER_10_CHAR;
        case 11:
            return MONSTER_11_CHAR;
        case 12:
            return MONSTER_12_CHAR;
        case 13:
            return MONSTER_13_CHAR;
        case 14:
            return MONSTER_14_CHAR;
        case 15:
            return MONSTER_15_CHAR;
        default:
            return '?';
    }
}

char *monster_behavior_color(int behavior) {
    switch (behavior) {
        case 0:
            return MONSTER_0_COLOR;
        case 1:
            return MONSTER_1_COLOR;
        case 2:
            return MONSTER_2_COLOR;
        case 3:
            return MONSTER_3_COLOR;
        case 4:
            return MONSTER_4_COLOR;
        case 5:
            return MONSTER_5_COLOR;
        case 6:
            return MONSTER_6_COLOR;
        case 7:
            return MONSTER_7_COLOR;
        case 8:
            return MONSTER_8_COLOR;
        case 9:
            return MONSTER_9_COLOR;
        case 10:
            return MONSTER_10_COLOR;
        case 11:
            return MONSTER_11_COLOR;
        case 12:
            return MONSTER_12_COLOR;
        case 13:
            return MONSTER_13_COLOR;
        case 14:
            return MONSTER_14_COLOR;
        case 15:
            return MONSTER_15_COLOR;
        default:
            return CONSOLE_RESET;
    }
}

// See character.h
void cleanup_character(Character_T *c) {
    if (c != NULL && c->cost != c->d->regular_cost && c->cost != c->d->tunnel_cost) {
        free(c->cost);
    }
    free(c);
}
