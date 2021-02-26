cmake_minimum_required(VERSION 3.17)
project(Rogue C)

set(CMAKE_C_STANDARD 11)

add_compile_options(-Wall -Wextra -pedantic)

add_executable(Rogue
        rogue.c

        Character/character.c
        Character/character.h

        Dungeon/dijkstra.c
        Dungeon/dijkstra.h
        Dungeon/dungeon.c
        Dungeon/dungeon.h
        Dungeon/dungeon-disk.c
        Dungeon/dungeon-disk.h

        Helpers/helpers.c
        Helpers/helpers.h
        Helpers/pairing-heap.c
        Helpers/pairing-heap.h
        Helpers/program-init.c
        Helpers/program-init.h
        Helpers/stack.c
        Helpers/stack.h

        Settings/arguments.h
        Settings/exit-codes.h
        Settings/file-settings.h
        Settings/dungeon-settings.h
        Settings/misc-settings.h
        Settings/print-settings.h)