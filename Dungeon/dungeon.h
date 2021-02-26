#ifndef ROGUE_DUNGEON_H
#define ROGUE_DUNGEON_H

// See dungeon.c for helper functions

// Define a macro to help obfuscate bare pointer arithmetic
#define MAP(a, b) map[(a) * d->width + (b)]

// Forward declare so we don't have to include the character header
typedef struct Character_S Character_T;

// Enum for possible neighbor directions. Not necessary, but helps avoid programming errors.
typedef enum Direction_E {
    UP, DOWN, LEFT, RIGHT
} Direction_T;

// Enum to store the cell type. Allows us to easily add new cell types later if we so desire, and makes code more
// readable and reliable. Make sure to add the new types to cell_type_char() and cell_type_color()
typedef enum Cell_Type_E {
    ROCK, ROOM, CORRIDOR, STAIR_UP, STAIR_DOWN
} Cell_Type_T;

// Stores attributes of a cell inside of the dungeon map. Extensible as long as the corresponding initializer in
// dungeon.c is updated (init_dungeon()). By having cells keep track of any character on them, it makes printing a lot
// easier and faster.
typedef struct Cell_S {
    Cell_Type_T type;
    int hardness;
    Character_T *character;
} Cell_T;

// Stores attributes about a room. y and x refer to the top left point.
typedef struct Room_S {
    int y, x, height, width;
} Room_T;

// Stores all attributes about a dungeon. Will be expanded upon later as new features are added. Extensible as long as
// init_dungeon() and cleanup_dungeon() is updated.
typedef struct Dungeon_S {
    Cell_T *map;
    Room_T *rooms;
    Character_T *player;
    int height, width, num_rooms;
} Dungeon_T;

// Returns a pointer to a new dungeon. A few parameters are able to changed at runtime if the user so desires. For now
// the driver code uses the defaults in setting.h
Dungeon_T *
new_dungeon(int height, int width, int min_rooms, int max_rooms, float percentage_covered);

// Initializes a dungeon's variables. Sets num_rooms to be zero, and rooms to NULL. This is the function to be sure to
// update if Cell_T is extended.
void init_dungeon(Dungeon_T *d, int height, int width);

// Runs along all sises of the dungeon, setting the immutable flag on them so they cannot be changed. Used by
// loag_pgm() in dungeon-disk.c
void generate_dungeon_border(Dungeon_T *d);

// Places the PC randomly in the dungeon. WILL NEED TO BE CHANGED LATER. Currently used by load_pgm() in dungeon-disk.c
void place_pc(Dungeon_T *d);

// Cleans up a dungeon, freeing all child structs and arrays.
void cleanup_dungeon(Dungeon_T *d);

// Returns the char type of a cell type.
char cell_type_char(Cell_Type_T c);

// Returns the ASCI control string to color a cell type.
char *cell_type_color(Cell_Type_T c);

// Returns the ASCI control string to color the background of a cell type
char *cell_type_background(Cell_Type_T c);

// Iterates through the dungeon and prints it out. Calls cell_type_char() and cell_type_color() to print it.
void print_dungeon(const Dungeon_T *d);

#endif //ROGUE_DUNGEON_H
