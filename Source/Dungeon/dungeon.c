// We have to include this macro so gcc shuts up and will actually compile
// I think this is what I get for wanting to compile against C11
#define _POSIX_C_SOURCE 200809L // NOLINT(bugprone-reserved-identifier)

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "dungeon.h"
#include "dijkstra.h"

#include "Character/character.h"
#include "Dungeon/Loaders/dungeon-disk.h"
#include "Dungeon/Loaders/dungeon-random.h"
#include "Helpers/helpers.h"
#include "Helpers/pairing-heap.h"
#include "Settings/character-settings.h"
#include "Settings/dungeon-settings.h"
#include "Settings/exit-codes.h"
#include "Settings/misc-settings.h"
#include "Settings/print-settings.h"

// Places a new PC in a room in the dungeon
static void place_new_pc(Dungeon_T *d) {
    int room, y, x;

    // Pick room randomly, then pick random coordinate in that room and create the PC.
    room = rand_int_in_range(0, d->num_rooms - 1);
    y = rand_int_in_range(d->rooms[room].y, d->rooms[room].y + d->rooms[room].height - 1);
    x = rand_int_in_range(d->rooms[room].x, d->rooms[room].x + d->rooms[room].width - 1);
    d->player = new_character(d, y, x, PC_SPEED, 0, PC_SYMBOL, PC_COLOR, true);
    d->MAP(y, x).character = d->player;
}

// Helper to place a monster in the room. Always places monsters in rooms other than the one the PC is in.
static bool place_individual_monster(Dungeon_T *d, int player_room) {
    int tries, room, y, x, speed, behavior;
    char symbol;
    char *color;

    // Try placing our monster... if we can't, then print to stderr and return to the place_monsters() loop
    tries = 0;
    do {

        // Check to see if we failed after too many tries to place a monster
        if (tries > FAILED_MONSTER_PLACEMENT) {
            fprintf(stderr,
                    "Failed to place monster after %i tries! There will only be %i monsters in the dungeon!\n Try using less monsters!",
                    FAILED_MONSTER_PLACEMENT, d->num_monsters);
            return false;
        }

        // Generate our values. This will guarantee monsters are placed in rooms.
        room = rand_int_in_range(0, d->num_rooms - 1);
        y = rand_int_in_range(d->rooms[room].y, d->rooms[room].y + d->rooms[room].height - 1);
        x = rand_int_in_range(d->rooms[room].x, d->rooms[room].x + d->rooms[room].width - 1);

        tries++;
    } while (d->MAP(y, x).character != NULL || room == player_room);

    // Set up our speed
    speed = rand_int_in_range(MIN_MONSTER_SPEED, MAX_MONSTER_SPEED);

    // Set up our monster behavior. It randomly allocates one at a time, using bit shifting to set the proper flag
    behavior = 0;
    behavior |= rand() % 2 ? INTELLIGENT : 0; // NOLINT(cert-msc50-cpp)
    behavior |= rand() % 2 ? TELEPATHIC : 0; // NOLINT(cert-msc50-cpp)
    behavior |= rand() % 2 ? TUNNELER : 0; // NOLINT(cert-msc50-cpp)
    behavior |= rand() % 2 ? ERRATIC : 0; // NOLINT(cert-msc50-cpp)

    // Set up our display variables
    symbol = monster_behavior_char(behavior);
    color = monster_behavior_color(behavior);

    // Place our monster
    d->num_monsters++;
    d->monsters[d->num_monsters - 1] = new_character(d, y, x, speed, behavior, symbol, color, false);
    d->MAP(y, x).character = d->monsters[d->num_monsters - 1];

    return true;
}

// Loops through placing monsters in the room. If there were no monsters placed, we have a massive error, and the
// function bails out.
static void place_monsters(Dungeon_T *d, int num_monster) {
    int i, player_room;

    player_room = find_player_room(d, d->player);

    // Allocate space for our monster array
    d->monsters = malloc(num_monster * sizeof(Character_T *));

    // Place monsters one by one randomly
    for (i = 0; i < num_monster; i++) {
        if (!place_individual_monster(d, player_room)) {
            break;
        }
    }

    // Bail if the dungeon isn't workable and has no monsters
    if (d->num_monsters < 1) {
        bail(DUNGEON_GENERATION_FAILURE,
             "FATAL ERROR! DUNGEONS MUST HAVE MONSTERS! TRY LOADING A DIFFERENT DUNGEON OR USING DIFFERENT PARAMETERS!\n");
    }

    // If we failed to place a monster, shrink our array to not waste memory
    if (d->num_monsters != num_monster) {
        d->monsters = safe_realloc(d->monsters, d->num_monsters * sizeof(Character_T *));
    }
}

// See dungeon.h
Dungeon_T *new_random_dungeon(int num_monsters) {
    Dungeon_T *d;

    // Generate the dungeon
    d = generate_dungeon(DUNGEON_HEIGHT, DUNGEON_WIDTH, MIN_NUM_ROOMS, MAX_NUM_ROOMS, PERCENTAGE_ROOM_COVERED);

    // Place our PC
    place_new_pc(d);

    // Place our monsters
    place_monsters(d, num_monsters);

    return d;
}

// See dungeon.h
Dungeon_T *new_dungeon_from_disk(const char *path, bool stairs, int num_monsters) {
    Dungeon_T *d;

    // Load the dungeon
    d = load_dungeon(path, stairs);

    // Check if we need to place a player... it means loading failed if we do
    if (d->player == NULL) {
        place_new_pc(d);
    }

    // Place our monsters
    place_monsters(d, num_monsters);

    return d;
}

// See dungeon.h
Dungeon_T *new_dungeon_from_pgm(const char *path, bool stairs, int num_monsters) {
    Dungeon_T *d;

    // Load the PGM
    d = load_pgm(path, stairs);

    // Place our PC
    place_new_pc(d);

    // Place our monsters
    place_monsters(d, num_monsters);

    return d;
}

// See dungeon.h
void save_dungeon_to_disk(Dungeon_T *d, const char *path) {
    save_dungeon(d, path);
}

// See dungeon.h
void save_dungeon_to_pgm(Dungeon_T *d, const char *path) {
    save_pgm(d, path);
}

// The main bulk of the gameplay lies in this function. It starts by initializing the heap, and setting up the character
// array, so we can have an intrusive pairing heap. It then builds the cost maps for the dungeon around the player.
// Once set up is done, it beings looping through the heap, until the player either dies, or the player is the only
// character that remains. It does this by pulling the next character off the heap, processing their movement, dealing
// with a character if they die, and then inserting them back into the heap. If the player is killed, the loop cleans
// up and ends, but if a monster dies, it pulls them off the heap, so they won't be queued anymore, and removes them
// from the game completely.
void play_dungeon(Dungeon_T *d) {

    // Local struct for storing heap nodes and characters. This way our heap nodes are associated with a character and
    // our character with a heap node.
    typedef struct Character_Node_S {
        Character_T *c;
        Heap_Node_T n;
    } Character_Node_T;

    // Print the d
    print_dungeon(d);
    nanosleep((const struct timespec[]) {{0, 1000000000 / FPS}}, NULL);

    Heap_T *h;
    Character_Node_T *characters;
    int i, j, character_len;

    // Initialize our heap as an intrusive one (see pairing-heap.h for details)
    h = new_heap(true);

    // Allocate space for our character array
    character_len = d->num_monsters + 1;
    characters = safe_malloc((character_len) * sizeof(Character_Node_T));

    // Add characters to the array and heap
    for (i = 0; i < d->num_monsters; i++) {
        characters[i].c = d->monsters[i];
        heap_intrusive_insert(h, &characters[i].n, GAME_SPEED / characters[i].c->speed, &characters[i]);
    }

    // Add our player to the array and heap
    characters[character_len - 1].c = d->player;
    heap_intrusive_insert(h, &characters[character_len - 1].n, GAME_SPEED / characters[character_len - 1].c->speed,
                          &characters[character_len - 1]);

    // Build the cost maps to be safe
    build_dungeon_cost_maps(d, true, true);

    // Begin processing our heap. As long as the size is above 2, it means there is a monster and a player on the heap
    // If there isn't, it means the player won.
    while (h->size > 1) {
        Character_Node_T *cn;
        Character_T *killed;

        // Pull off the next character to move
        cn = heap_remove_min(h)->data;

        // Process the player and the monsters differently
        killed = cn->c->player ? move_player(cn->c) : move_monster(cn->c);

        // Process a character if they were killed
        if (killed != NULL) {
            Character_Node_T *cn2 = NULL;

            // Find the killed monster in the characters_array
            for (i = 0; i < character_len; i++) {
                if (characters[i].c == killed) {
                    cn2 = &characters[i];
                }
            }

            // Check if the killed character was the player. Otherwise remove the character from the array
            if (killed->player) {
                printf("You died! Better luck next time!\n");
                cn2->c = NULL;
                cleanup_character(d->player);
                d->player = NULL;
                break;

            } else {

                // Remove the killed monster from the game
                d->MAP(cn2->c->y, cn2->c->x).character = NULL;
                d->num_monsters--;
                heap_delete(h, &cn2->n);

                // Destroy the monster completely
                cleanup_character(cn2->c);
                cn2->c = NULL;
            }
        }

        // If the player was the one that moved, print the map, and wait.
        if (cn->c->player) {
            print_dungeon(d);
            nanosleep((const struct timespec[]) {{0, 1000000000 / FPS}}, NULL);
        }

        // Reinsert the monster back into the queue
        heap_intrusive_insert(h, &cn->n, cn->n.key + (GAME_SPEED / cn->c->speed), cn);
    }

    // If we get to this point, the player won or left the dungeon. Rebuild the monster array to be smaller.
    d->monsters = safe_realloc(d->monsters, d->num_monsters * sizeof(Character_T *));
    for (i = 0, j = 0; i < character_len; i++) {

        // The number of characters actually in characters (minus the player) are guaranteed to be in lockstep
        if (characters[i].c != NULL && !characters[i].c->player) {
            d->monsters[j] = characters[i].c;
            j++;
        }
    }

    // Cleanup
    cleanup_heap(h);
    free(characters);
}

// See dungeon.h
void init_dungeon(Dungeon_T *d, int height, int width) {
    int i, j;

    // Static values
    d->height = height;
    d->width = width;

    // These will be set later as part of generation
    d->num_rooms = 0;
    d->num_monsters = 0;
    d->rooms = NULL;
    d->player = NULL;
    d->monsters = NULL;
    d->regular_cost = NULL;
    d->tunnel_cost = NULL;

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
void build_dungeon_cost_maps(Dungeon_T *d, bool regular_map, bool tunnel_map) {
    int sources[1][2];

    // Set up our sources
    sources[0][0] = d->player->y;
    sources[0][1] = d->player->x;

    // Build our cost maps
    if (regular_map) {
        free(d->regular_cost);
        d->regular_cost = generate_dijkstra_map(d, 1, (int *) sources, CHARACTER_DIAGONAL_TRAVEL, REGULAR_MAP);
    }
    if (tunnel_map) {
        free(d->tunnel_cost);
        d->tunnel_cost = generate_dijkstra_map(d, 1, (int *) sources, CHARACTER_DIAGONAL_TRAVEL, TUNNEL_MAP);
    }
}

// See dungeon.h
void cleanup_dungeon(Dungeon_T *d) {
    int i;

    // Free any monsters in the dungeon
    for (i = 0; i < d->num_monsters; i++) {
        cleanup_character(d->monsters[i]);
    }

    // Free the rest of our pointers
    cleanup_character(d->player);
    free(d->map);
    free(d->rooms);
    free(d->monsters);
    free(d->regular_cost);
    free(d->tunnel_cost);
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

// See dungeon.h
void print_dungeon_cost_maps(Dungeon_T *d) {

    // Build the cost maps if they don't exist
    if (d->regular_cost == NULL || d->tunnel_cost == NULL) {
        build_dungeon_cost_maps(d, true, true);
    }

    print_dijkstra_map(d, d->regular_cost, REGULAR_MAP);
    print_dijkstra_map(d, d->tunnel_cost, TUNNEL_MAP);
}







