#include "Character/character.h"
#include "Dungeon/dungeon.h"
#include "Dungeon/dungeon-disk.h"
#include "Helpers/program-init.h"
#include "Settings/dungeon-settings.h"

// All this is is a driver for the underlying headers... this entire codebase is designed to carry forward through
// the entire semester, so main() will always contain minimal code. Not going to bother commenting heavily on what its
// doing since it will change often and week to week
int main(int argc, const char *argv[]) {
    Program_T p;
    Dungeon_T *d;

    init_program(argc, argv, &p);

    d = p.pgm_load ? load_pgm(p.load_path, p.stairs) :
        p.load ? load_dungeon(p.load_path, p.stairs) :
        new_dungeon(DUNGEON_HEIGHT, DUNGEON_WIDTH, MIN_NUM_ROOMS, MAX_NUM_ROOMS, PERCENTAGE_ROOM_COVERED);

    print_dungeon(d);

    build_character_cost_maps(d->player);

    print_character_cost_maps(d->player);

    if (p.save) {
        save_dungeon(d, p.save_dungeon_path);
    }

    if (p.pgm_save) {
        save_pgm(d, p.save_pgm_path);
    }

    cleanup_dungeon(d);
    cleanup_program(&p);
}
