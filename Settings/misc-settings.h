#ifndef ROGUE_MISC_SETTINGS_H
#define ROGUE_MISC_SETTINGS_H

// This file sets all the misc Settings that don't belong under others

// Default size for the stack data structure
#define DEFAULT_STACK_SIZE 50

// Current assignment due
#define VERSION "assignment-1.03"

// Similar to CORR_NUM_HARDNESS_LEVELS, this will control how granular the cost function treats hardness in the dungeon
// for the purposes of tunneling monsters.
#define TUNNEL_NUM_HARDNESS_LEVELS 3

// Controls if characters can travel diagonally and how they can do it. With DIAGONAL_NEEDS_OPEN_WALL set to true,
// regular monsters won't be able to go diagonally if one of the immediate directions (north, south, east, west) are not
// open as well. This is to stop monsters from jumping between corridors or rooms where they touch at the corner.
#define CHARACTER_DIAGONAL_TRAVEL true
#define DIAGONAL_NEEDS_OPEN_SPACE true

#endif //ROGUE_MISC_SETTINGS_H
