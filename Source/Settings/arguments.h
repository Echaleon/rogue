#ifndef ROGUE_ARGUMENTS_H
#define ROGUE_ARGUMENTS_H

// Defines our program arguments.
// If these are changed make sure print_help() is updated in program_init()

// Load options. Use l<file> or --load=<file> to use an argument
#define LOAD_LONG "--load"
#define LOAD_SHORT "-l"

// Save options. Use -s <file
#define SAVE_LONG "--save"
#define SAVE_SHORT "-s"

// Load PGM options. Use --pgm-load <file>
#define PGM_LOAD_LONG "--pgm-load"
#define PGM_LOAD_SHORT ""

// Save PGM options. Use --pgm-save <file>
#define PGM_SAVE_LONG "--pgm-save"
#define PGM_SAVE_SHORT ""

// Stair options
#define STAIRS_LONG "--stairs"
#define STAIRS_SHORT ""

// Seed options. Use --seed <seed>
#define SEED_LONG "--seed"
#define SEED_SHORT ""

// Monster number options. User --nummon <int>
#define NUMMON_LONG "--nummon"
#define NUMMON_SHORT "-n"

// Print options
#define PRINT_LONG "--print"
#define PRINT_SHORT "-p"

// Help options
#define HELP_LONG "--help"
#define HELP_SHORT ""

// Version options
#define VERSION_LONG "--version"
#define VERSION_SHORT ""

#endif //ROGUE_ARGUMENTS_H
