#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Make use of endian functions in the host system. Better than anything we can probably code. Need a compile time
// conditional so we load the windows variant or the *nix variant.
#ifdef _WIN32
#include <winsock.h>
#else
#include <arpa/inet.h>
#endif

#include "dungeon-disk.h"

#include "dungeon-random.h"
#include "Dungeon/dungeon.h"
#include "Character/character.h"
#include "Helpers/helpers.h"
#include "Settings/character-settings.h"
#include "Settings/dungeon-settings.h"
#include "Settings/print-settings.h"
#include "Settings/file-settings.h"


// Helper function that places stairs in the dungeon randomly. d->num_rooms must be more than 0, and the player has to
// be initialized.
static void place_stairs(Dungeon_T *d) {
    int r;

    // If there is three rooms, guarantee the PC won't be on top of them and they won't be the same
    if (d->num_rooms > 2) {
        int r2;

        // Up stairs
        do {
            r = rand_int_in_range(0, d->num_rooms - 1);
        } while (d->rooms[r].y == d->player->y && d->rooms[r].x == d->player->x);
        d->MAP(d->rooms[r].y, d->rooms[r].x).type = STAIR_UP;

        // Down stairs
        do {
            r2 = rand_int_in_range(0, d->num_rooms - 1);
        } while ((d->rooms[r2].y == d->player->y && d->rooms[r2].x == d->player->x) || r == r2);
        d->MAP(d->rooms[r2].y, d->rooms[r2].x).type = STAIR_DOWN;

    } else if (d->num_rooms > 1) {
        r = rand() % 2;  // NOLINT(cert-msc50-cpp)

        // Randomly place up or down in the two available rooms
        if (r) {
            d->MAP(d->rooms[0].y, d->rooms[0].x).type = STAIR_UP;
            d->MAP(d->rooms[1].y, d->rooms[1].x).type = STAIR_DOWN;
        } else {
            d->MAP(d->rooms[0].y, d->rooms[0].x).type = STAIR_DOWN;
            d->MAP(d->rooms[1].y, d->rooms[1].x).type = STAIR_UP;
        }

    } else {
        r = rand() % 2;  // NOLINT(cert-msc50-cpp)

        // Randomly place up or down stairs
        if (r) {
            d->MAP(d->rooms[0].y, d->rooms[0].x).type = STAIR_UP;
        } else {
            d->MAP(d->rooms[0].y, d->rooms[0].x).type = STAIR_DOWN;
        }
    }
}

// Massive function the loads a dungeon. It could be broken up into smaller functions, but that's a lot of
// unnecessary overhead in my opinion, and adds some additional complexity. Makes use of goto statements to clean up
// the function if the file input is bad, instead returning a new dungeon instead. This is one of the very few times
// we can actually make use of gotos without it being very bad form.
//
// It starts off by reading the entire file into memory, bailing out if the file doesn't exist, we can't open it, or
// the file size is greater then MAX_DUNGEON_FILE_SIZE. This is more efficient than reading a few bytes at a time. It
// initializes an iterator (p) so we can step through the buffer safely, always making sure we are in bounds on the
// array. First it checks the semantic of the file: the marker, version, and making sure the size matches. Then it
// starts actually building the dungeon, stepping through the file entry by entry. If the stairs bool is true AND there
// is no stairs in the dungeon yet, the loader will place some with the helper function.
Dungeon_T *load_dungeon(const char *path, bool stairs) {
    FILE *f;
    int y, x, i, j, r;
    unsigned char *buffer; // Stores array
    unsigned long long size, p; // p stores our buffer iterator
    Dungeon_T *d;
    bool placed_stairs;

    // Check if the file exists... bail if we can't open it.
    f = fopen(path, "rb");
    if (f == NULL) {
        fprintf(stderr, "Couldn't open file %s! Using random dungeon!\n", path);
        goto cleanup;
    }

    // Get the size of the file.
    fseek(f, 0, SEEK_END);
    size = (unsigned long long int) ftell(f); // Cast for safety
    rewind(f);

    // Check if the file is bigger than the max. Bail out if it is. It's not going to be a valid file then.
    if (size > MAX_DUNGEON_FILE_SIZE) {
        fprintf(stderr, "File is %llu bytes, bigger than maximum size of %i bytes! Using random dungeon!\n",
                size, MAX_DUNGEON_FILE_SIZE);
        goto cleanup;
    }

    // Allocate a buffer for the entire file and read it into memory. If we can't for any reason, or the file open with
    // an error, we bail.
    buffer = safe_malloc(size * sizeof(char));
    if (fread(buffer, 1, size, f) != size) {
        fprintf(stderr, "Error opening file %s! Using random dungeon!\n", path);
        fclose(f);
        goto cleanup_buffer;
    }
    fclose(f); // We read it. Don't need to keep it open.
    p = 0;

    // Check if we can read the file marker without going out of bounds.
    if (p + sizeof(FILE_MARKER) - 1 > size) {
        fprintf(stderr, "EOF! File is missing file marker info! Using random dungeon!\n");
        goto cleanup_buffer;
    }

    // Check if the file marker is correct.
    if (strncmp((char *) &buffer[p], FILE_MARKER, sizeof(FILE_MARKER) - 1) != 0) {
        char error[sizeof(FILE_MARKER)];
        strncpy(error, (char *) buffer, sizeof(FILE_MARKER) - 1);
        error[sizeof(FILE_MARKER) - 1] = '\0';
        fprintf(stderr, "Invalid file marker: %s! Using random dungeon!\n", error);
        goto cleanup_buffer;
    }
    p += sizeof(FILE_MARKER) - 1;

    // Check if we can read the file version without going out of bounds.
    if (p + sizeof(uint32_t) > size) {
        fprintf(stderr, "EOF! File is missing file version info! Using random dungeon!\n");
        goto cleanup_buffer;
    }

    // Check if the file version is correct.
    // Ugly cast to grab than more one byte is needed. Need to do endian conversion.
    if (ntohl(*(uint32_t *) &buffer[p]) != FILE_VERSION) {
        fprintf(stderr, "Invalid file version %i! Using random dungeon!\n", ntohl(*(uint32_t *) &buffer[p]));
        goto cleanup_buffer;
    }
    p += sizeof(uint32_t);

    // Check if we can read the file size without going out of bounds.
    if (p + sizeof(uint32_t) > size) {
        fprintf(stderr, "EOF! File is missing file size info! Using random dungeon!\n");
        goto cleanup_buffer;
    }

    // Check if the file size matches the one reported by the OS.
    // Ugly cast to grab than more one byte is needed. Need to do endian conversion.
    if (ntohl(*(uint32_t *) &buffer[p]) != size) {
        fprintf(stderr, "File size %i in file does not match OS file size of %llu! Using random dungeon!\n",
                ntohl(*(uint32_t *) &buffer[p]), size);
        goto cleanup_buffer;
    }
    p += sizeof(uint32_t);

    // Allocate space for our dungeon and initialize it.
    d = safe_malloc(sizeof(Dungeon_T));
    init_dungeon(d, DUNGEON_HEIGHT, DUNGEON_WIDTH);

    // Check if we can read the player coordinates without going out bounds.
    if (p + sizeof(uint8_t) * 2 > size) {
        fprintf(stderr, "EOF! File is missing player location info! Using random dungeon!\n");
        goto cleanup_dungeon;
    }

    // Start building our dungeon... get player coordinates and save them for now.
    x = buffer[p];
    y = buffer[p + 1];

    // Check if the player coordinate are in bounds on the dungeon map.
    if (x >= DUNGEON_WIDTH && y >= DUNGEON_HEIGHT) {
        fprintf(stderr, "Out of range player coordinates: (%i, %i)! Using random dungeon!\n", x, y);
        goto cleanup_dungeon;
    }
    p += sizeof(uint8_t) * 2;

    // Check if we can read the dungeon map without going out of bounds
    if (p + sizeof(uint8_t) * DUNGEON_HEIGHT * DUNGEON_WIDTH > size) {
        fprintf(stderr, "EOF! File is missing dungeon hardness info! Using random dungeon!\n");
        goto cleanup_dungeon;
    }

    // Get our dungeon map. Checks if any of the border cells are not max hardness and bails out if they aren't.
    for (i = 0; i < DUNGEON_HEIGHT; i++) {
        for (j = 0; j < DUNGEON_WIDTH; j++) {
            if ((i == 0 || i == DUNGEON_HEIGHT - 1 || j == 0 || j == DUNGEON_WIDTH - 1) &&
                buffer[p] != IMMUTABLE_ROCK_HARDNESS) {
                fprintf(stderr, "Border of the dungeon must be immutable (%i hardness)! Using random dungeon!\n",
                        IMMUTABLE_ROCK_HARDNESS);
                goto cleanup_dungeon;
            }
            if (buffer[p] == 0) {
                d->MAP(i, j).type = CORRIDOR;
                d->MAP(i, j).hardness = 0;
            } else {
                d->MAP(i, j).type = ROCK;
                d->MAP(i, j).hardness = buffer[p];
            }
            p += sizeof(uint8_t);
        }
    }

    // Attempt to place our PC. Fail out if coords are not an open room.
    if (d->MAP(y, x).type == ROCK) {
        fprintf(stderr,
                "Player cannot be in rock: (%i, %i)! Dungeon will be unplayable! Using random dungeon!\n", x, y);
        goto cleanup_dungeon;
    }
    d->player = new_character(d, y, x, PC_SPEED, 0, PC_SYMBOL, PC_COLOR, true);
    d->MAP(y, x).character = d->player;

    // Check if we can read the number of rooms without going out of bounds.
    if (p + sizeof(uint16_t) > size) {
        fprintf(stderr, "EOF! File is missing number of rooms! Using random dungeon!\n");
        goto cleanup_dungeon;
    }

    // Get how many rooms we have and store it into our dungeon. Make sure the number of rooms is greater than 0
    // and warn if it's not that it won't be a good dungeon.
    // Ugly cast to grab than more one byte is needed. Need to do endian conversion.
    if (ntohs(*(uint16_t *) &buffer[p]) == 0) {
        fprintf(stderr,
                "Dungeon does not have rooms and will be unplayable! Using random dungeon!\n");
        goto cleanup_dungeon;
    }
    d->num_rooms = ntohs(*(uint16_t *) &buffer[p]);
    d->rooms = safe_malloc(d->num_rooms * sizeof(Room_T));
    p += sizeof(uint16_t);

    // Check if we can read the rooms without going out of bounds
    if (p + d->num_rooms * 4 * sizeof(uint8_t) > size) {
        fprintf(stderr, "EOF! File is missing room info! Using random dungeon!\n");
        goto cleanup_dungeon;
    }

    // Start building our rooms. Bail out if coordinate are not in range, or the hardness underneath has not been set
    // to zero. Iterate through each room.
    for (r = 0; r < d->num_rooms; r++) {
        int width, height;

        // Read in the 4 parameters for a room
        x = buffer[p];
        y = buffer[p + 1];
        width = buffer[p + 2];
        height = buffer[p + 3];

        // Check if the bounds of the room are valid and in range.
        if (y + height > DUNGEON_HEIGHT - 1 || x + width > DUNGEON_WIDTH - 1) {
            fprintf(stderr,
                    "Invalid room specified! (x: %i, y: %i, w: %i, h: %i)! Rooms must be in bounds! Using random dungeon!\n",
                    x, y, width, height);
            goto cleanup_dungeon;
        }

        // Check if all cells covered by the room have zero hardness. Paint the cell as a room if it's a valid cell.
        for (i = y; i < y + height; i++) {
            for (j = x; j < x + width; j++) {
                if (d->MAP(i, j).hardness != 0) {
                    fprintf(stderr,
                            "Invalid room specified! (x: %i, y: %i, w: %i, h: %i)! Rooms must have 0 hardness! Using random dungeon!\n",
                            x, y, width, height);
                    goto cleanup_dungeon;
                }
                d->MAP(i, j).type = ROOM;
            }
        }

        // Add the rooms to the array and increment.
        d->rooms[r].y = y;
        d->rooms[r].x = x;
        d->rooms[r].height = height;
        d->rooms[r].width = width;
        p += 4 * sizeof(uint8_t);
    }

    // Keep track if we placed stairs
    placed_stairs = false;

    // Check if we can read the number of upward stairs without going out of bounds.
    if (p + sizeof(uint16_t) > size) {
        fprintf(stderr, "EOF! File is missing number of upwards staircases! Using random dungeon!\n");
        goto cleanup_dungeon;
    }

    // Get our number of upwards stairs. Ugly cast to grab than more one byte is needed. Need to do endian conversion.
    r = ntohs(*(uint16_t *) &buffer[p]);
    p += sizeof(uint16_t);

    // Check if we can read the upwards stairs without going out of bounds.
    if (p + r * sizeof(uint8_t) > size) {
        fprintf(stderr, "EOF! File is missing upwards staircase info! Using random dungeon!\n");
        goto cleanup_dungeon;
    }

    // Place our upwards staircases, failing if the coordinate aren't in range or if they aren't in open space.
    // Iterate through each staircase.
    for (i = 0; i < r; i++) {

        // Check if the room stair coordinate are in range
        if (buffer[p + 1] > DUNGEON_HEIGHT - 1 || buffer[p] > DUNGEON_WIDTH - 1) {
            fprintf(stderr,
                    "Out of bounds upwards stairs (%i, %i)! Using random dungeon!\n",
                    buffer[p], buffer[p + 1]);
            goto cleanup_dungeon;
        }

        // Check if the stairs are in open space.
        if (d->MAP(buffer[p + 1], buffer[p]).hardness != 0) {
            fprintf(stderr, "Upwards staircases cannot be in rock (%i, %i)! Using random dungeon!\n",
                    buffer[p], buffer[p + 1]);
            goto cleanup_dungeon;
        }

        // Add the stair to the dungeon
        d->MAP(buffer[p + 1], buffer[p]).type = STAIR_UP;
        placed_stairs = true;
        p += 2 * sizeof(uint8_t);
    }

    // Check if we can read the number of downward stairs without going out of bounds.
    if (p + sizeof(uint16_t) > size) {
        fprintf(stderr, "EOF! File is missing number of downwards staircases! Using random dungeon!\n");
        goto cleanup_dungeon;
    }

    // Get our number of downward stairs. Ugly cast to grab than more one byte is needed. Need to do endian conversion.
    r = ntohs(*(uint16_t *) &buffer[p]);
    p += sizeof(uint16_t);

    // Check if we can read the downwards stairs without going out of bounds.
    if (p + r * sizeof(uint8_t) > size) {
        fprintf(stderr, "EOF! File is missing downwards staircase info! Using random dungeon!\n");
        goto cleanup_dungeon;
    }

    // Place our downwards staircases, failing if the coordinate aren't in range or if they aren't in open space.
    // Iterate through each staircase.
    for (i = 0; i < r; i++) {

        // Check if the room stair coordinate are in range
        if (buffer[p + 1] > DUNGEON_HEIGHT - 1 || buffer[p] > DUNGEON_WIDTH - 1) {
            fprintf(stderr,
                    "Out of bounds downwards stairs (%i, %i)! Using random dungeon!\n",
                    buffer[p], buffer[p + 1]);
            goto cleanup_dungeon;
        }

        // Check if the stairs are in open space.
        if (d->MAP(buffer[p + 1], buffer[p]).hardness != 0) {
            fprintf(stderr, "Downwards staircases cannot be in rock (%i, %i)! Using random dungeon!\n",
                    buffer[p], buffer[p + 1]);
            goto cleanup_dungeon;
        }

        // Add the stair to the dungeon
        d->MAP(buffer[p + 1], buffer[p]).type = STAIR_DOWN;
        placed_stairs = true;
        p += 2 * sizeof(uint8_t);
    }

    // If the dungeon from the file has no stairs, and the user passed the stairs option, make sure we add some.
    // Otherwise warn that the dungeon won't be playable without stairs.
    if (!placed_stairs) {
        if (stairs) {
            place_stairs(d);
        } else {
            fprintf(stderr,
                    "Dungeons without stairs will be mostly unplayable! Consider using a random dungeon or the stairs switch!\n");
        }
    }

    // Cleanup
    free(buffer);
    return d;

    // Bail out labels in case we run into an error... massively cleans up the code.
    cleanup_dungeon:
    cleanup_dungeon(d);

    cleanup_buffer:
    free(buffer);

    cleanup:
    return generate_dungeon(DUNGEON_HEIGHT, DUNGEON_WIDTH, MIN_NUM_ROOMS, MAX_NUM_ROOMS, PERCENTAGE_ROOM_COVERED);
}

// Load a dungeon from a pgm file, allowing the user to create their own dungeons in a photo editor. It's picky, but
// should be relatively strong against malformed files. Settings for this are stored in misc-settings.h. It's basically
// a clone of load_dungeon() just for a different function. If the stairs bool is true, it will also
// generate stairs in rooms on the dungeon map.
//
// A limitation of these pgm maps is that all rooms will be 1x1. Shouldn't be the end of the world, but might be
// significant later as the project progresses. Just adds memory overhead.
Dungeon_T *load_pgm(const char *path, bool stairs) {
    FILE *f;
    int height, width, max_val, i, j, r;
    char *read, *end;
    unsigned char *buffer;
    unsigned long long size, p, p2;
    Dungeon_T *d;

    // Check if the file exists... bail if we can't open it.
    f = fopen(path, "rb");
    if (f == NULL) {
        fprintf(stderr, "Couldn't open file %s! Using random dungeon!\n", path);
        goto cleanup;
    }

    // Get the size of the file.
    fseek(f, 0, SEEK_END);
    size = (unsigned long long int) ftell(f); // Cast for safety
    rewind(f);

    // Check if the file is bigger than the max. Bail out if it is. It's not going to be a valid file then.
    if (size > MAX_PGM_FILE_SIZE) {
        fprintf(stderr, "File is %llu bytes, bigger than maximum size of %i bytes! Using random dungeon!\n",
                size, MAX_PGM_FILE_SIZE);
        goto cleanup;
    }

    // Allocate a buffer for the entire file and read it into memory. If we can't for any reason, or the file opens with
    // an error, we bail.
    buffer = safe_malloc(size * sizeof(char));
    if (fread(buffer, 1, size, f) != size) {
        fprintf(stderr, "Error opening file %s! Using random dungeon!\n", path);
        fclose(f);
        goto cleanup_buffer;
    }
    fclose(f); // We read it. Don't need to keep it open.
    p = 0;

    // Check if we can read the first line without going out of bounds
    if (p + sizeof(PGM_MAGIC_NUMBER) > size) {
        fprintf(stderr, "EOF! File is missing PGM magic val! Using random dungeon!\n");
        goto cleanup_buffer;
    }

    // Read the magic value
    if (strncmp((char *) &buffer[p], PGM_MAGIC_NUMBER, sizeof(PGM_MAGIC_NUMBER) - 1) != 0) {
        char error[sizeof(PGM_MAGIC_NUMBER)];
        strncpy(error, (char *) buffer, sizeof(PGM_MAGIC_NUMBER) - 1);
        error[sizeof(PGM_MAGIC_NUMBER) - 1] = '\0';
        fprintf(stderr, "Invalid PGM value: %s! Using random dungeon!\n", error);
        goto cleanup_buffer;
    }
    p += sizeof(PGM_MAGIC_NUMBER);

    // Skip past the comment
    while (p < size) {
        if (buffer[p] == '\n') {
            p += sizeof(char);
            break;
        }
        p += sizeof(char);
    }

    // Bail because the file is malformed
    if (p == size) {
        fprintf(stderr, "EOF! File is missing dimensions! Using random dungeon!\n");
        goto cleanup_buffer;
    }

    // Get the width
    p2 = p;
    while (p2 < size) {
        if (buffer[p2] == ' ') {
            p2 += sizeof(char);
            break;
        }
        p2 += sizeof(char);
    }

    // Bail because the file is malformed
    if (p2 == size) {
        fprintf(stderr, "EOF! File is missing width! Using random dungeon!\n");
        goto cleanup_buffer;
    }

    // Read in width
    read = safe_calloc(p2 - p, sizeof(char));
    strncpy(read, (char *) &buffer[p], (p2 - 1) - p);
    width = (int) strtol(read, &end, 0);
    if (width == 0 || end == NULL || *end != (char) 0) {
        fprintf(stderr, "Malformed width! %s is invalid! Using random dungeon!\n", read);
        goto cleanup_buffer;
    }
    free(read);
    p = p2;

    // Get the height
    p2 = p;
    while (p2 < size) {
        if (buffer[p2] == '\n') {
            p2 += sizeof(char);
            break;
        }
        p2 += sizeof(char);
    }

    // Bail because the file is malformed
    if (p2 == size) {
        fprintf(stderr, "EOF! File is missing height! Using random dungeon!\n");
        goto cleanup_buffer;
    }

    // Read in height
    read = safe_calloc(p2 - p, sizeof(char));
    strncpy(read, (char *) &buffer[p], (p2 - 1) - p);
    height = (int) strtol(read, &end, 0);
    if (height == 0 || end == NULL || *end != (char) 0) {
        fprintf(stderr, "Malformed height! %s is invalid! Using random dungeon!\n", read);
        goto cleanup_buffer;
    }
    free(read);
    p = p2;

    // Get max val
    while (p2 < size) {
        if (buffer[p2] == '\n') {
            p2 += sizeof(char);
            break;
        }
        p2 += sizeof(char);
    }

    // Bail because the file is malformed
    if (p2 == size) {
        fprintf(stderr, "EOF! File is missing height! Using random dungeon!\n");
        goto cleanup_buffer;
    }

    // Read in the max val
    read = safe_calloc(p2 - p, sizeof(char));
    strncpy(read, (char *) &buffer[p], (p2 - 1) - p);
    max_val = (int) strtol(read, &end, 0);
    if (max_val != PGM_MAX_VAL || end == NULL || *end != (char) 0) {
        fprintf(stderr, "Malformed max val! %s is invalid! Using random dungeon!\n", read);
        goto cleanup_buffer;
    }
    free(read);
    p = p2;

    // Check if we have space to read in the full array
    if (p + width * height * sizeof(uint8_t) > size) {
        fprintf(stderr, "EOF! File is missing array data! Using random dungeon!\n");
        goto cleanup_buffer;
    }

    // Allocate space for our dungeon
    d = safe_malloc(sizeof(Dungeon_T));
    init_dungeon(d, height + 2, width + 2);

    // Read in the pgm array
    for (i = 1; i < d->height - 1; i++) {
        for (j = 1; j < d->width - 1; j++) {
            if (buffer[p] == PGM_CORRIDOR_VAL) {
                d->MAP(i, j).type = CORRIDOR;
                d->MAP(i, j).hardness = 0;
            } else if (buffer[p] == PGM_ROOM_VAL) {
                d->MAP(i, j).type = ROOM;
                d->MAP(i, j).hardness = 0;
                d->num_rooms++;
            } else {
                d->MAP(i, j).type = ROCK;
                d->MAP(i, j).hardness = buffer[p];
            }
            p += sizeof(uint8_t);
        }
    }

    // Make sure we have rooms for methods to work
    if (d->num_rooms == 0) {
        fprintf(stderr, "Dungeons does not have any rooms and will be unplayable! Using random dungeon!\n");
        goto cleanup_buffer;
    }

    // Cleanup our buffer - we don't need it anymore.
    free(buffer);

    // Create our room array... 1x1 rooms because it will be incredible complicated if not
    d->rooms = safe_malloc(d->num_rooms * sizeof(Room_T));
    r = 0;

    for (i = 1; i < d->height - 1; i++) {
        for (j = 1; j < d->width - 1; j++) {
            if (d->MAP(i, j).type == ROOM) {
                d->rooms[r].y = i;
                d->rooms[r].x = j;
                d->rooms[r].height = 1;
                d->rooms[r].width = 1;
                r++;
            }
        }
    }

    // Generator our dungeon borders
    generate_dungeon_border(d);

    // Place up and down stairs if asked to
    if (stairs) {
        place_stairs(d);
    } else {
        fprintf(stderr, "Dungeons without stairs will be mostly unplayable! Consider using the stairs switch!\n");
    }

    return d;

    // Bail out labels in case we run into an error... massively cleans up the code.
    cleanup_buffer:
    free(buffer);

    cleanup:
    return generate_dungeon(DUNGEON_HEIGHT, DUNGEON_WIDTH, MIN_NUM_ROOMS, MAX_NUM_ROOMS, PERCENTAGE_ROOM_COVERED);
}

// Massive function that saves the dungeon to disk. Works basically the opposite of the load_dungeon() function. It
// allocates a byte array the size of the file, fills the byte array, and writes it in one fell swoop for efficiency
// purposes. It keeps an iterator int, always updating so we write to the correct part of the byte array. Makes sure
// to always write in big-endian, as per spec.
//
// Makes a bunch of explicit casts for the purposes of safety... and to keep my IDE happy.
void save_dungeon(Dungeon_T *d, const char *path) {
    FILE *f;
    int i, j, p, p2;
    unsigned char *buffer;
    char *marker;
    uint32_t size, version;
    uint16_t num_rooms, up, down;

    // Try to open the file... don't need to do the other work if it doesn't open
    f = fopen(path, "wb");
    if (f == NULL) {
        fprintf(stderr, "Couldn't open file %s! Unable to save!\n", path);
        return;
    }

    // Count the number of up and down stair cases since we don't store that
    up = down = 0;
    for (i = 0; i < d->height; i++) {
        for (j = 0; j < d->width; j++) {
            if (d->MAP(i, j).type == STAIR_UP) {
                up++;
                continue;
            }
            if (d->MAP(i, j).type == STAIR_DOWN) {
                down++;
                continue;
            }
        }
    }

    // Set up values for memcpy() later. Necessary because we can't just set the memory to directly be the macro
    // values.
    marker = FILE_MARKER;
    version = htonl(FILE_VERSION);
    size = sizeof(FILE_MARKER) - 1 + sizeof(uint32_t) * 2 + sizeof(uint16_t) * 3 +
           (2 + d->height * d->width + d->num_rooms * 4 + up * 2 + down * 2) * sizeof(uint8_t);

    // Allocate our entire array so we can write at once
    buffer = safe_malloc(size);

    // Start assigning values to our output array
    p = 0;

    // Insert file marker
    memcpy(&buffer[p], marker, strlen(marker)); // NOLINT(bugprone-not-null-terminated-result)
    p += (int) strlen(marker);

    // Insert version number
    memcpy(&buffer[p], &version, sizeof(uint32_t));
    p += sizeof(uint32_t);

    // Insert file size
    size = htonl(size);
    memcpy(&buffer[p], &size, sizeof(uint32_t));
    p += sizeof(uint32_t);
    size = ntohl(size);

    // Insert the player coordinates
    buffer[p] = (unsigned char) d->player->x;
    p += sizeof(uint8_t);
    buffer[p] = (unsigned char) d->player->y;
    p += sizeof(uint8_t);

    // Insert the hardness map
    for (i = 0; i < d->height; i++) {
        for (j = 0; j < d->width; j++) {
            buffer[p] = (unsigned char) d->MAP(i, j).hardness;
            p += sizeof(uint8_t);
        }
    }

    // Insert the number of rooms
    num_rooms = htons((uint16_t) d->num_rooms);
    memcpy(&buffer[p], &num_rooms, sizeof(uint16_t));
    p += sizeof(uint16_t);

    // Insert the room values
    for (i = 0; i < d->num_rooms; i++) {
        buffer[p] = (unsigned char) d->rooms[i].x;
        buffer[p + 1] = (unsigned char) d->rooms[i].y;
        buffer[p + 2] = (unsigned char) d->rooms[i].width;
        buffer[p + 3] = (unsigned char) d->rooms[i].height;
        p += 4 * sizeof(uint8_t);
    }

    // Insert number of upward stair cases
    up = htons(up);
    memcpy(&buffer[p], &up, sizeof(uint16_t));
    p += sizeof(uint16_t);
    up = ntohs(up);

    // Insert number of downward stair cases
    down = htons(down);
    p2 = p + (int) (up * 2 * sizeof(uint8_t));
    memcpy(&buffer[p2], &down, sizeof(uint16_t));
    p2 += sizeof(uint16_t);

    // Insert the both stairs at once... inefficient since we don't stair stairs globally.
    for (i = 0; i < d->height; i++) {
        for (j = 0; j < d->width; j++) {
            if (d->MAP(i, j).type == STAIR_UP) {
                buffer[p] = (unsigned char) j;
                buffer[p + 1] = (unsigned char) i;
                p += 2 * sizeof(uint8_t);
                continue;
            }
            if (d->MAP(i, j).type == STAIR_DOWN) {
                buffer[p2] = (unsigned char) j;
                buffer[p2 + 1] = (unsigned char) i;
                p2 += 2 * sizeof(uint8_t);
                continue;
            }
        }
    }

    // Write the file out, and clean up.
    if (fwrite(buffer, 1, size, f) != size) {
        fprintf(stderr, "Error writing to %s! File is most likely corrupted!\n", path);
    }
    fclose(f);
    free(buffer);
}

// A relatively simple function in comparison to the others in this file... this will write the file to disk, once again
// in a single call to fwrite(), since manipulating the byte array is easier. It sets up the proper header for the PGM
// file then iterates through the array of the room. It's not a bulletproof function... and will need updating if
// the dungeon hardness values are updated.
void save_pgm(Dungeon_T *d, const char *path) {
    FILE *f;
    int i, j;
    unsigned char *buffer;
    char *header;
    unsigned long long size, p;

    // Try to open the file... don't need to do the other work if it doesn't open
    f = fopen(path, "wb");
    if (f == NULL) {
        fprintf(stderr, "Couldn't open file %s! Unable to save!\n", path);
        return;
    }

    // Set up our header for the PGM
    size = sizeof(PGM_MAGIC_NUMBER) + sizeof(PGM_COMMENT) + count_digits(d->width - 2) + 1 +
           count_digits(d->height - 2) + 1 + count_digits(PGM_MAX_VAL) + 2;
    header = safe_calloc(size, sizeof(char));
    sprintf(header, "%s\n%s\n%i %i\n%i\n",
            PGM_MAGIC_NUMBER, PGM_COMMENT, DUNGEON_WIDTH - 2, DUNGEON_HEIGHT - 2, PGM_MAX_VAL);

    // Set up our entire array so we can write at once
    size += (d->width - 2) * (d->height - 2) * sizeof(uint8_t) - 1;
    buffer = safe_malloc(size);

    p = 0;

    // Write our header
    memcpy(&buffer[p], header, strlen(header));
    p += strlen(header);

    // Done with the header
    free(header);

    // Write the hardness values to the array
    for (i = 1; i < d->height - 1; i++) {
        for (j = 1; j < d->width - 1; j++) {
            if (d->MAP(i, j).type == ROOM) {
                buffer[p] = PGM_ROOM_VAL;
            } else if (d->MAP(i, j).type == CORRIDOR) {
                buffer[p] = PGM_CORRIDOR_VAL;
            } else {
                buffer[p] = (unsigned char) d->MAP(i, j).hardness;
            }
            p += sizeof(uint8_t);
        }
    }

    // Write the file out, and clean up.
    if (fwrite(buffer, 1, size, f) != size) {
        fprintf(stderr, "Error writing to %s! File is most likely corrupted!\n", path);
    }
    fclose(f);
    free(buffer);
}
