OBJS	= rogue.o Character/character.o Dungeon/dijkstra.o Dungeon/dungeon.o Dungeon/dungeon-disk.o  Helpers/helpers.o Helpers/pairing-heap.o Helpers/program-init.o Helpers/stack.o
SOURCE	= rogue.c Character/character.c Dungeon/dijkstra.c Dungeon/dungeon.c Dungeon/dungeon-disk.c  Helpers/helpers.c Helpers/pairing-heap.c Helpers/program-init.c Helpers/stack.c
HEADER	= Character/character.h Dungeon/dijkstra.h Dungeon/dungeon.h Dungeon/dungeon-disk.h Helpers/helpers.h Helpers/pairing-heap.h Helpers/program-init.h Helpers/stack.h Settings/arguments.h Settings/dungeon-settings.h Settings/exit-codes.h Settings/file-settings.h Settings/misc-settings.h Settings/print-settings.h
OUT	= rogue
LFLAGS = -std=c11 -g3 -Wall -pedantic

all: rogue

rogue: $(OBJS)
	$(CC) -o $@ $^ $(LFLAGS)

%.o: %.c $(HEADER)
	$(CC) -c -o $@ $< $(LFLAGS)

# clean house
clean:
	rm -f $(OBJS) $(OUT)

# run the program
run: $(OUT)
	./$(OUT)

# compile program with debugging information
debug: $(OUT)
	valgrind ./$(OUT)

# run program with valgrind for errors
valgrind: $(OUT)
	valgrind ./$(OUT)

# run program with valgrind for leak checks
valgrind_leakcheck: $(OUT)
	valgrind --leak-check=full ./$(OUT)

# run program with valgrind for leak checks (extreme)
valgrind_extreme: $(OUT)
	valgrind --leak-check=full --show-leak-kinds=all --leak-resolution=high --track-origins=yes --vgdb=yes ./$(OUT)

