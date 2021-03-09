#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

// Check if we are on Windows or *nix to include the correct director to make directories
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

#include "program-init.h"
#include "helpers.h"

#include "Settings/arguments.h"
#include "Settings/character-settings.h"
#include "Settings/exit-codes.h"
#include "Settings/file-settings.h"
#include "Settings/misc-settings.h"

// Struct to store all our arguments to pass between read_arguments() and init_program()
typedef struct Arguments_S {
    bool load;
    bool save;
    bool pgm_load;
    bool pgm_save;
    bool stairs;
    bool seed;
    bool nummon;
    bool print;
    bool help;
    bool version;
    char *load_path;
    char *save_dungeon_path;
    char *save_pgm_path;
    unsigned int rand_seed;
    int num_monsters;
} Arguments_T;

// Helper for read_arguments that checks if the string is an actual argument for the program.
// Adding new arguments means this needs to be updated
static bool is_argument_string(const char *s) {

    if (strcmp(s, LOAD_LONG) == 0 || strcmp(s, LOAD_SHORT) == 0) {
        return true;
    }
    if (strcmp(s, SAVE_LONG) == 0 || strcmp(s, SAVE_SHORT) == 0) {
        return true;
    }
    if (strcmp(s, PGM_LOAD_LONG) == 0 || strcmp(s, PGM_LOAD_SHORT) == 0) {
        return true;
    }
    if (strcmp(s, PGM_SAVE_LONG) == 0 || strcmp(s, PGM_SAVE_SHORT) == 0) {
        return true;
    }
    if (strcmp(s, STAIRS_LONG) == 0 || strcmp(s, STAIRS_SHORT) == 0) {
        return true;
    }
    if (strcmp(s, SEED_LONG) == 0 || strcmp(s, SEED_SHORT) == 0) {
        return true;
    }
    if (strcmp(s, NUMMON_LONG) == 0 || strcmp(s, NUMMON_SHORT) == 0) {
        return true;
    }
    if (strcmp(s, PRINT_LONG) == 0 || strcmp(s, PRINT_SHORT) == 0) {
        return true;
    }
    if (strcmp(s, HELP_LONG) == 0 || strcmp(s, HELP_SHORT) == 0) {
        return true;
    }
    if (strcmp(s, VERSION_LONG) == 0 || strcmp(s, VERSION_LONG) == 0) {
        return true;
    }

    return false;
}

// Helper to read arguments from the command line. Update this if adding new arguments. Arguments can only be specified
// once, and the program will error out if it's specified more than once.
static void read_arguments(int argc, const char *argv[], Arguments_T *a) {
    int i;

    i = 1;
    while (i < argc) {

        // Check for the load flag
        if (strcmp(argv[i], LOAD_LONG) == 0 || strcmp(argv[i], LOAD_SHORT) == 0) {

            // Check if it's been used
            if (a->load) {
                bail(INVALID_ARGUMENT, "Load option already specified!\n");
            }

            // Check if pgm-load has been used
            if (a->pgm_load) {
                bail(INVALID_ARGUMENT, "PGM Load option already specified!\n");
            }

            a->load = true;
            i++;

            // Check if there is a path specified
            if (i < argc && !is_argument_string(argv[i])) {
                a->load_path = safe_malloc(strlen(argv[i]) + sizeof("\0")); // allocate space for null terminator
                strcpy(a->load_path, argv[i]);
                i++;
            }
            continue;
        }

        // Check for the save flag
        if (strcmp(argv[i], SAVE_LONG) == 0 || strcmp(argv[i], SAVE_SHORT) == 0) {

            // Check if it's been used
            if (a->save) {
                bail(INVALID_ARGUMENT, "Save option already specified!\n");
            }

            a->save = true;
            i++;

            // Check if there is a path specified
            if (i < argc && !is_argument_string(argv[i])) {
                a->save_dungeon_path = safe_malloc(
                        strlen(argv[i]) + sizeof("\0")); // allocate space for null terminator
                strcpy(a->save_dungeon_path, argv[i]);
                i++;
            }
            continue;
        }

        // Check for the PGM load flag
        if (strcmp(argv[i], PGM_LOAD_LONG) == 0 || strcmp(argv[i], PGM_LOAD_SHORT) == 0) {

            // Check if it's been used
            if (a->pgm_load) {
                bail(INVALID_ARGUMENT, "PGM Load option already specified!\n");
            }

            // Check if save has been used
            if (a->load) {
                bail(INVALID_ARGUMENT, "Load option already specified!\n");
            }

            a->pgm_load = true;
            i++;

            // Check if there is a path specified
            if (i < argc && !is_argument_string(argv[i])) {
                a->load_path = safe_malloc(strlen(argv[i]) + sizeof("\0")); // allocate space for null terminator
                strcpy(a->load_path, argv[i]);
                i++;
            }
            continue;
        }

        // Check for the PGM save flag
        if (strcmp(argv[i], PGM_SAVE_LONG) == 0 || strcmp(argv[i], PGM_SAVE_SHORT) == 0) {

            // Check if it's been used
            if (a->pgm_save) {
                bail(INVALID_ARGUMENT, "PGM Save option already specified!\n");
            }

            a->pgm_save = true;
            i++;

            // Check if there is a path specified
            if (i < argc && !is_argument_string(argv[i])) {
                a->save_pgm_path = safe_malloc(strlen(argv[i]) + sizeof("\0")); // allocate space for null terminator
                strcpy(a->save_pgm_path, argv[i]);
                i++;
            }
            continue;
        }

        // Check for the stair flag
        if (strcmp(argv[i], STAIRS_LONG) == 0 || strcmp(argv[i], STAIRS_SHORT) == 0) {

            // Check if it's been used
            if (a->stairs) {
                bail(INVALID_ARGUMENT, "Stair option already specified!\n");
            }
            a->stairs = true;
            i++;

            continue;
        }

        // Check for the seed flag
        if (strcmp(argv[i], SEED_LONG) == 0 || strcmp(argv[i], SEED_SHORT) == 0) {

            // Check if it's been used
            if (a->seed) {
                bail(INVALID_ARGUMENT, "Seed option already specified!\n");
            }
            a->seed = true;
            i++;

            // Find if there is an argument to seed and bail if there isn't
            if (i < argc && !is_argument_string(argv[i])) {
                char *end;

                // strtol is safer than atil()... we can check if it's an int and if it actually worked
                a->rand_seed = (unsigned int) strtol(argv[i], &end, 0);
                if (end == NULL || *end != (char) 0) {
                    bail(INVALID_ARGUMENT, "Invalid seed %s! Seed must be in integer!\n", argv[i]);
                }
                i++;

            } else {
                bail(INVALID_ARGUMENT, "Seed option must have a seed argument!\n");
            }

            continue;
        }

        // Check for the nummon flag
        if (strcmp(argv[i], NUMMON_LONG) == 0 || strcmp(argv[i], NUMMON_SHORT) == 0) {

            // Check if it's been used
            if (a->nummon) {
                bail(INVALID_ARGUMENT, "Monster number option already specified!\n");
            }
            a->nummon = true;
            i++;

            // Find if there is an argument to seed and bail if there isn't
            if (i < argc && !is_argument_string(argv[i])) {
                char *end;

                // strtol is safer than atil()... we can check if it's an int and if it actually worked
                a->num_monsters = (int) strtol(argv[i], &end, 0);
                if (end == NULL || *end != (char) 0 || a->num_monsters < 1) {
                    bail(INVALID_ARGUMENT,
                         "Invalid integer %s! Number of monsters must be an integer and greater than 0!\n", argv[i]);
                }
                i++;

            } else {
                bail(INVALID_ARGUMENT, "Monster number option must have an integer argument!\n");
            }

            continue;
        }

        // Check for the help flag
        if (strcmp(argv[i], PRINT_LONG) == 0 || strcmp(argv[i], PRINT_SHORT) == 0) {

            // Check if it's been used
            if (a->print) {
                bail(INVALID_ARGUMENT, "Print option already specified!\n");
            }
            a->print = true;
            i++;

            continue;
        }

        // Check for the help flag
        if (strcmp(argv[i], HELP_LONG) == 0 || strcmp(argv[i], HELP_SHORT) == 0) {

            // Check if it's been used
            if (a->help) {
                bail(INVALID_ARGUMENT, "Help option already specified!\n");
            }
            a->help = true;
            i++;

            continue;
        }

        // Check for the version flag
        if (strcmp(argv[i], VERSION_LONG) == 0 || strcmp(argv[i], VERSION_SHORT) == 0) {

            // Check if it's been used
            if (a->help) {
                bail(INVALID_ARGUMENT, "Version option already specified!\n");
            }
            a->version = true;
            i++;

            continue;
        }

        // Invalid option
        bail(INVALID_ARGUMENT, "Unknown option %s!\n", argv[i]);
    }
}

// Print a help menu
static void print_help() {
    printf("--load or -l causes a dungeon to load from disk.\n ");
    printf("     <file> after will mean it loads from that path instead of the default.\n");
    printf("--save or -s causes a dungeon to save to disk.\n");
    printf("     <file> after will mean it saves to that path instead of the default.\n");
    printf("--pgm-load causes a dungeon to be read in from a binary PGM file.\n");
    printf("     <file> after will mean it loads from that path instead of the default.\n");
    printf("--pgm-save causes a dungeon to save as a binary PGM file.\n");
    printf("     <file> after will mean it saves to the p[ath instead of the default.\n");
    printf("--stairs causes stairs to be guaranteed to placed. Mostly useful for --pgm-load\n");
    printf("--seed <seed> will specify a seed for the RNG. MUST BE AN INTEGER!\n");
    printf("--nummons <num> will specific the number of monsters to spawn. MUST BE AN INTEGER!\n");
    printf("--print or -p causes the dungeon and cost maps to be printed out, instead of the game playing.\n");
    printf("--version will print the version of the program.\n");
    printf("--help will print this.\n");
    printf("\n");
    printf("Default dungeon path is: $HOME%s%s\n", SAVE_PATH, DEFAULT_DUNGEON_NAME);
    printf("Default PGM path is: $HOME%s%s\n", SAVE_PATH, DEFAULT_PGM_NAME);
    printf("\n");
    printf("PGM files must be binary, with max value of %i. Values of %i are corridors\n", PGM_MAX_VAL,
           PGM_CORRIDOR_VAL);
    printf("Values of %i are rooms. There is no support for specifying the PC or stairs\n", PGM_ROOM_VAL);
}

// Print the program version (defined in misc settings)
static void print_version() {
    printf("Current version: %s\n", VERSION);
}

// Right now the bulk of the logic here is allocating the load_path variable, and setting up the directory. We have to do
// some annoying calloc() and length calculations because C is irritating with its strings.
void init_program(int argc, const char *argv[], Program_T *p) {
    char *dungeon_path, *pgm_path;
    Arguments_T a;

    dungeon_path = NULL;
    pgm_path = NULL;

    // Initialize the program struct
    p->load_path = NULL;
    p->save_dungeon_path = NULL;
    p->save_pgm_path = NULL;
    p->num_monsters = 0;

    // Initialize the argument struct
    a.load = false;
    a.save = false;
    a.pgm_load = false;
    a.pgm_save = false;
    a.stairs = false;
    a.seed = false;
    a.nummon = false;
    a.print = false;
    a.help = false;
    a.version = false;
    a.load_path = NULL;
    a.save_dungeon_path = NULL;
    a.save_pgm_path = NULL;
    a.rand_seed = 0;
    a.num_monsters = DEFAULT_NUM_OF_MONSTERS;

    // Read in our arguments
    read_arguments(argc, argv, &a);

    // Print help
    if (a.help) {
        print_help();
        cleanup_program(p);
        exit(NORMAL_EXIT);
    }

    // Print version
    if (a.version) {
        print_version();
        cleanup_program(p);
        exit(NORMAL_EXIT);
    }

    // Set random seed
    if (a.seed) {
        srand(a.rand_seed);
    } else {
        srand((unsigned int) time(NULL)); // NOLINT(cert-msc51-cpp)
    }

    // Set up the dungeon_path only if it's not specified on the command line, and we're actually interacting with the disk
    if ((a.load && a.load_path == NULL) || (a.save && a.save_dungeon_path == NULL)) {
        int length;

        length = 0;

        // Set up the length of our dungeon_path variable. Need to dynamically allocate it since it needs to be passed upwards
        if (USE_HOME_DIRECTORY) {
            length += (int) strlen(getenv("HOME"));
        }
        length += strlen(SAVE_PATH);
        length += strlen(DEFAULT_DUNGEON_NAME);
        length += sizeof(char); // Need to include null terminator

        // Allocate size for our full load_path and begin actually building it
        dungeon_path = safe_calloc((size_t) length, sizeof(char));
        if (USE_HOME_DIRECTORY) {
            strcat(dungeon_path, getenv("HOME"));
        }
        strcat(dungeon_path, SAVE_PATH);

        // Make the directory if it doesn't exist... need to specify different methods if we are on Windows or *nix.
        // The compiler will help us with that.
        #if defined(_WIN32)
        _mkdir(load_path);
        #else
        mkdir(dungeon_path, 0700);
        #endif

        // Finish setting up the full load_path to the dungeon.
        strcat(dungeon_path, DEFAULT_DUNGEON_NAME);
    }

    // Set up the pgm_path only if it's not specified on the command line, and we're actually interacting with the disk
    if ((a.pgm_load && a.load_path == NULL) || (a.pgm_save && a.save_pgm_path == NULL)) {
        int length;

        length = 0;

        // Set up the length of our pgm_path variable. Need to dynamically allocate it since it needs to be passed upwards
        if (USE_HOME_DIRECTORY) {
            length += (int) strlen(getenv("HOME"));
        }
        length += strlen(SAVE_PATH);
        length += strlen(DEFAULT_PGM_NAME);
        length += sizeof(char); // Need to include null terminator

        // Allocate size for our full path and begin actually building it
        pgm_path = safe_calloc((size_t) length, sizeof(char));
        if (USE_HOME_DIRECTORY) {
            strcat(pgm_path, getenv("HOME"));
        }
        strcat(pgm_path, SAVE_PATH);

        // Make the directory if it doesn't exist... need to specify different methods if we are on Windows or *nix.
        // The compiler will help us with that.
        #if defined(_WIN32)
        _mkdir(load_path);
        #else
        mkdir(pgm_path, 0700);
        #endif

        // Finish setting up the full path to the pgm.
        strcat(pgm_path, DEFAULT_PGM_NAME);
    }

    // Set our paths to return to main
    p->load_path = a.load_path != NULL ? a.load_path : a.pgm_load ? pgm_path : dungeon_path;
    p->save_dungeon_path = a.save_dungeon_path == NULL ? dungeon_path : a.save_dungeon_path;
    p->save_pgm_path = a.save_pgm_path == NULL ? pgm_path : a.save_pgm_path;

    // Set our bools to return to main
    p->load = a.load;
    p->save = a.save;
    p->pgm_load = a.pgm_load;
    p->pgm_save = a.pgm_save;
    p->stairs = a.stairs;
    p->print = a.print;

    // Misc values to return to main
    p->num_monsters = a.num_monsters;
}

// See program-init.h
void cleanup_program(Program_T *p) {
    if (p->load_path == p->save_dungeon_path) {
        free(p->load_path);
        free(p->save_pgm_path);
    } else if (p->load_path == p->save_pgm_path) {
        free(p->load_path);
        free(p->save_dungeon_path);
    } else {
        free(p->load_path);
        free(p->save_dungeon_path);
        free(p->save_pgm_path);
    }
}
