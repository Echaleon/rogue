cmake_minimum_required(VERSION 3.17)
project(Rogue C)

set(CMAKE_C_STANDARD 11)

# Add all files recursively... by using configure depends it will causes rebuilds automatically
# and we can avoid annoying things like having to pollute our source tree or list every file individually
file(GLOB_RECURSE ROGUE_SOURCES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/Source/*.c)

# Has to come before the below options
add_executable(Rogue ${ROGUE_SOURCES})

# Set our source includes and our compile options
target_include_directories(Rogue PRIVATE ${PROJECT_SOURCE_DIR}/Source)
target_compile_options(Rogue PRIVATE -Wall -Wextra -pedantic)
