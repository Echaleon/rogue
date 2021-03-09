#ifndef ROGUE_DUNGEON_SETTINGS_H
#define ROGUE_DUNGEON_SETTINGS_H

// This file sets all defaults and compile time settings related to dungeon generation... some of these parameters
// should not be changed and will cause errors if they are out of bounds. Use with caution.

// Defines the dungeon dimensions... in theory it could accept any size... most parameters are tuned to work well
// with these dimensions. I wouldn't change them.
#define DUNGEON_HEIGHT 21
#define DUNGEON_WIDTH 80

// If you want dungeons with more rooms covering them, increase the percentage. Must be between 0.0 and 1.0
// Too high and you won't generate workable dungeons and the program will error out.
#define PERCENTAGE_ROOM_COVERED .1

// How many times the code tries to place rooms in a partition, or generate an entire dungeon. Only really comes into
// play if you set too high or strict of standards for the dungeon generation parameters
#define FAILED_ROOM_PLACEMENT 2000
#define FAILED_DUNGEON_GENERATION 2000

// Defines the default for the minimum or maximum number of rooms... will be somewhere in this range
// Too strict and you won't generate workable dungeons.
#define MIN_NUM_ROOMS 3
#define MAX_NUM_ROOMS 10

// Tuned to the partition sizes. Don't change without knowing why. Will cause segfaults or arithmetic errors if min
// sizes are too close to partition minimums.
#define MIN_ROOM_HEIGHT 4
#define MIN_ROOM_WIDTH 7

// Tuned to the dungeon size. Smaller and you will make a ton of rooms. Bigger and there will be less. MAX MUST BE
// GREATER THAN DOUBLE OR THE CODE WITH HAVE ARITHMETIC ERRORS
#define MIN_PARTITION_HEIGHT 7
#define MAX_PARTITION_HEIGHT 13
#define MIN_PARTITION_WIDTH 13
#define MAX_PARTITION_WIDTH 27

// This is how the partitions will split. Too high and the partitions will get weird, leading to more room generation
// failures, and possible complete failure to generate workable dungeons
#define PERCENTAGE_SPLIT_FORCE .25

// Defines how the Dijkstra's mapping creates corridors. These are pretty safe to play with. Turn off USE_HARDNESS
// and the corridors will be far more linear, using the CORR_ROCK_WEIGHT instead. CORR_NUM_HARDNESS_LEVELS dictates
// how granular the hardness of the dungeon is treated... too high and corridors will be very very twisty.
#define USE_HARDNESS_FOR_CORRIDORS true
#define CORR_NUM_HARDNESS_LEVELS 6
#define CORR_ROOM_WEIGHT 1
#define CORR_ROCK_WEIGHT 5
#define CORR_CORRIDOR_WEIGHT 0

// Defines the range for hardness values to lay down in rocks (inclusive). These are added to the default below.
#define MIN_ROCK_HARDNESS 1
#define MAX_ROCK_HARDNESS 254
#define IMMUTABLE_ROCK_HARDNESS 255

// Don't know why you would change these. Default hardness would be the only one to maybe tweak if you wanted corridors
// to be straighter but... probably better not to mess with it.
#define DEFAULT_CELL_TYPE ROCK
#define DEFAULT_HARDNESS 0
#define OPEN_SPACE_HARDNESS 0

#endif //ROGUE_DUNGEON_SETTINGS_H
