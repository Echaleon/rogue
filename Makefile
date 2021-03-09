# Output executable
RELEASE_EXEC ?= rogue
DEBUG_EXEC ?= rogue-debug

# Flags to apply to every build
FLAGS ?=-std=c11 -pipe -march=native -Wall -Wextra -pedantic

# Flags to apply to specific targets
DEBUG_FLAGS ?=-g
RELEASE_FLAGS ?=-O2 -DNDEBUG -flto -Werror

# Source directory
SRC_DIRS ?= ./Source

# Output directories
DEBUG_DIR ?= ./make-build-debug
RELEASE_DIR ?= ./make-build-release

# Default goal
.DEFAULT_GOAL := $(DEBUG_EXEC)

###############################################################

SRCS := $(shell find $(SRC_DIRS) -name *.cpp -or -name *.c)

DEBUG_OBJS := $(SRCS:%=$(DEBUG_DIR)/%.o)
RELEASE_OBJS := $(SRCS:%=$(RELEASE_DIR)/%.o)

DEBUG_DEPS := $(DEBUG_OBJS:.o=.d)
RELEASE_DEPS := $(RELEASE_OBJS:.o=.d)

INC_DIRS := $(shell find $(SRC_DIRS) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

BUILD_FLAGS ?= $(INC_FLAGS) $(FLAGS) -MMD -MP

###############################################################

debug: $(DEBUG_EXEC)

$(DEBUG_EXEC): $(DEBUG_OBJS)
	$(info Linking $(notdir $@))
	@$(CC) $(DEBUG_FLAGS) $(BUILD_FLAGS) $(DEBUG_OBJS) -o $@ $(LDFLAGS)

$(DEBUG_DIR)/%.c.o: %.c
	$(info Compiling $(notdir $<))
	@$(MKDIR_P) $(dir $@)
	@$(CC) $(DEBUG_FLAGS) $(BUILD_FLAGS) $(CFLAGS) -c $< -o $@

# c++ source
$(DEBUG_DIR)/%.cpp.o: %.cpp
	$(info Compiling $(notdir $<))
	@$(MKDIR_P) $(dir $@)
	@$(CXX) $(DEBUG_FLAGS) $(BUILD_FLAGS) $(CXXFLAGS) -c $< -o $@

run_debug: debug
	./$(DEBUG_EXEC)

###############################################################

release: $(RELEASE_EXEC)

$(RELEASE_EXEC): $(RELEASE_OBJS)
	$(info Linking $(notdir $@))
	@$(CC) $(RELEASE_FLAGS) $(BUILD_FLAGS) $(RELEASE_OBJS) -o $@ $(LDFLAGS)

$(RELEASE_DIR)/%.c.o: %.c
	$(info Compiling $(notdir $<))
	@$(MKDIR_P) $(dir $@)
	@$(CC) $(RELEASE_FLAGS) $(BUILD_FLAGS) $(CFLAGS) -c $< -o $@

# c++ source
$(RELEASE_DIR)/%.cpp.o: %.cpp
	$(info Compiling $(notdir $<))
	@$(MKDIR_P) $(dir $@)
	@$(CXX) $(RELEASE_FLAGS) $(BUILD_FLAGS) $(CXXFLAGS) -c $< -o $@

run_release: release
	./$(RELEASE_EXEC)

###############################################################

.PHONY: clean

clean:
	$(info Cleaning up)
	@$(RM) -r $(DEBUG_DIR)
	@$(RM) -r $(RELEASE_DIR)
	@$(RM) $(DEBUG_EXEC)
	@$(RM) $(RELEASE_EXEC)

run: $(.DEFAULT_GOAL)
	./$(.DEFAULT_GOAL)

valgrind: debug
	valgrind --leak-check=full --show-leak-kinds=all --leak-resolution=high --track-origins=yes --vgdb=yes ./$(DEBUG_EXEC) --seed 10

###############################################################

-include $(DEBUG_DEPS)
-include $(RELEASE_DEPS)

MKDIR_P ?= mkdir -p



