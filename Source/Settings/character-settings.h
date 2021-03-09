#ifndef ROGUE_CHARACTER_SETTINGS_H
#define ROGUE_CHARACTER_SETTINGS_H

// This file sets all the defaults and compile time settings related to character generation

// Defines the default amount of monsters to spawn
#define DEFAULT_NUM_OF_MONSTERS 10

// Defines speeds for characters
#define PC_SPEED 10
#define MIN_MONSTER_SPEED 5
#define MAX_MONSTER_SPEED 20

// Defines if characters can move diagonally
#define CHARACTER_DIAGONAL_TRAVEL true

// How many times the algorithm will try to place a monster... really only comes into play if the number of monsters is
// insanely high.
#define FAILED_MONSTER_PLACEMENT 2000

#endif //ROGUE_CHARACTER_SETTINGS_H
