#ifndef ROGUE_FILE_SETTINGS_H
#define ROGUE_FILE_SETTINGS_H

// Setting for the default save info. If use home directory is true, SAVE_PATH will be appended to the user's
// home directory. If not SAVE_PATH will serve at the absolute load_path. It will create the last directory, but cannot
// create more than that
#define USE_HOME_DIRECTORY true
#define SAVE_PATH "/.rlg327/"
#define DEFAULT_DUNGEON_NAME "dungeon"
#define DEFAULT_PGM_NAME "dungeon.pgm"

// Setting to define the maximum file size in bytes... currently set to be the theoretical maximum: a grid of 1x1 roms
// each filled with a stair. A dungeon *literally* cannot be bigger than this unless it has rooms that completely
// overlap, or stairs that overlap... in which case it's such a bad dungeon, it's not viable, and we can probably
// safely reject it.
#define MAX_DUNGEON_FILE_SIZE 11788
#define MAX_PGM_FILE_SIZE 4096

// Setting for the version of saved and loaded files. The header must match this, or we will bail and not read them.
#define FILE_MARKER "RLG327-S2021"
#define FILE_VERSION 0

// PGM file settings
#define PGM_MAGIC_NUMBER "P5"
#define PGM_COMMENT "# CREATOR: CS327 RLG"
#define PGM_CORRIDOR_VAL PGM_MAX_VAL
#define PGM_ROOM_VAL 0
#define PGM_MAX_VAL 255

#endif //ROGUE_FILE_SETTINGS_H
