#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#include "character.h"
#include "../Dungeon/dijkstra.h"
#include "../Dungeon/dungeon.h"
#include "../Helpers/helpers.h"
#include "../Settings/misc-settings.h"
#include "../Settings/print-settings.h"

// Helper to print to console the cost of each cell... should only be handed 0-9 ever. All it does is print out the
// number with its corresponding color.
static void print_cost_helper(int i) {
    printf("%s", COST_MAP_BACKGROUND);
    switch (i) {
        case 0:
            printf("%s%i", COST_0, 0);
            break;
        case 1:
            printf("%s%i", COST_1, 1);
            break;
        case 2:
            printf("%s%i", COST_2, 2);
            break;
        case 3:
            printf("%s%i", COST_3, 3);
            break;
        case 4:
            printf("%s%i", COST_4, 4);
            break;
        case 5:
            printf("%s%i", COST_5, 5);
            break;
        case 6:
            printf("%s%i", COST_6, 6);
            break;
        case 7:
            printf("%s%i", COST_7, 7);
            break;
        case 8:
            printf("%s%i", COST_8, 8);
            break;
        case 9:
            printf("%s%i", COST_9, 9);
            break;
        default:
            printf("%s ", CONSOLE_RESET);
    }

    printf("%s", CONSOLE_RESET);
}

// See character.h
Character_T *new_character(Dungeon_T *d, int y, int x, char symbol, char *color, bool player) {
    Character_T *c = safe_malloc(sizeof(Character_T));
    c->d = d;
    c->y = y;
    c->x = x;
    c->symbol = symbol;
    c->color = color;
    c->player = player;
    c->regular_cost = NULL;
    c->tunnel_cost = NULL;
    return c;
}

// See character.h
void build_character_cost_maps(Character_T *c) {
    c->regular_cost = generate_dijkstra_map(c->d, c->y, c->x, CHARACTER_DIAGONAL_TRAVEL, REGULAR_MAP);
    c->tunnel_cost = generate_dijkstra_map(c->d, c->y, c->x, CHARACTER_DIAGONAL_TRAVEL, TUNNEL_MAP);
}

// See character.h
void print_character_cost_maps(Character_T *c) {
    int i, j;
    Dungeon_T *d;

    // Define our dungeon to be d so we can still use the MAP macro from dungeon.h
    d = c->d;

    // Only print a map if it exists, otherwise print that no map has been made to stderr.
    if (c->regular_cost != NULL) {
        int *cost;

        // Define our cost map to be cost so we can use the COST macro from dijkstra.h
        cost = c->regular_cost;

        // Iterate through the regular cost map
        for (i = 0; i < d->height; i++) {
            for (j = 0; j < d->width; j++) {

                // First check if the cell is actually the character the map is built around, since that takes
                // precedence for printing. Otherwise check if it's max cost, which means it's a rock, or a cell we
                // couldn't get to. If both are not true it's just a normal cell that can be printed normally.
                if (i == c->y && j == c->x) {
                    printf("%s%s%c%s", COST_MAP_BACKGROUND, c->color, c->symbol, CONSOLE_RESET);

                } else if (COST(i, j) == INT_MAX) {

                    // Check if it's not a rock... if it has infinite cost and isn't rock, it means a part of the
                    // dungeon that is disconnected from where the character is, and it's impossible to path-find to it.
                    // We want to print these differently than normal rock cells, which also have infinite cost.
                    if (d->MAP(i, j).type != ROCK) {
                        printf("%s%s%c%s", COST_MAP_BACKGROUND, COST_IMPOSSIBLE_COLOR, COST_IMPOSSIBLE, CONSOLE_RESET);
                    } else {
                        printf("%s%c%s", COST_INFINITE_COLOR, COST_INFINITE, CONSOLE_RESET);
                    }

                } else {

                    // We just have a normal cell here.
                    print_cost_helper(COST(i, j) % 10);
                }
            }

            // Print a new line after every row
            printf("\n");
        }

        // Print a new line after the last row to space multiple maps apart
        printf("\n");

    } else {
        fprintf(stderr, "Character does not have a regular cost map associated with it!\n");
    }

    // Only print a map if it exists, otherwise print that no map has been made to stderr.
    if (c->tunnel_cost != NULL) {
        int *cost;

        // Define our cost map to be cost so we can use the COST macro from dijkstra.h
        cost = c->tunnel_cost;

        // Iterate through the tunnel cost map
        for (i = 0; i < d->height; i++) {
            for (j = 0; j < d->width; j++) {

                // First check if the cell is actually the character the map is built around, since that takes
                // precedence for printing. Otherwise check if it's max cost, which means it's an immutable cell. If
                // both are not true it's just a normal cell that can be printed normally
                if (i == c->y && j == c->x) {
                    printf("%s%s%c%s", COST_MAP_BACKGROUND, c->color, c->symbol, CONSOLE_RESET);

                } else if (COST(i, j) == INT_MAX) {
                    printf("%s%c%s", COST_INFINITE_COLOR, COST_INFINITE, CONSOLE_RESET);

                } else {
                    print_cost_helper(COST(i, j) % 10);
                }
            }

            // Print a new line after every row
            printf("\n");
        }

        // Print a new line after the last row to space multiple maps apart
        printf("\n");

    } else {
        fprintf(stderr, "Character does not have a tunnel cost map associated with it!\n");
    }
}

// See character.h
void cleanup_character(Character_T *c) {
    free(c->regular_cost);
    free(c->tunnel_cost);
    free(c);
}
