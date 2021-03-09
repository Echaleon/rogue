#include "Character/character.h"
#include "Dungeon/dungeon.h"
#include "Helpers/program-init.h"

// All this is is a driver for the underlying headers... this entire codebase is designed to carry forward through
// the entire semester, so main() will always contain minimal code. Not going to bother commenting heavily on what its
// doing since it will change often and week to week
int main(int argc, const char *argv[]) {
    Program_T p;
    Dungeon_T *d;

    init_program(argc, argv, &p);

    d = p.pgm_load ? new_dungeon_from_pgm(p.load_path, p.stairs, p.num_monsters) :
        p.load ? new_dungeon_from_disk(p.load_path, p.stairs, p.num_monsters) :
        new_random_dungeon(p.num_monsters);

    if (p.save) {
        save_dungeon_to_disk(d, p.save_dungeon_path);
    }

    if (p.pgm_save) {
        save_dungeon_to_pgm(d, p.save_pgm_path);
    }

    if (p.print) {
        print_dungeon(d);
        print_dungeon_cost_maps(d);
    } else {
        play_dungeon(d);
    }
    
    cleanup_dungeon(d);
    cleanup_program(&p);
}
