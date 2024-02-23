cmake_minimum_required(VERSION 3.28)

project(gbemu VERSION 1.0 LANGUAGES C)

# Set for Apple Silicon.
set(CMAKE_HOST_SYSTEM_PROCESSOR arm64)
# C11.
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)
# Compiler variables to set flags.
set(gcc_like_cxx "$<COMPILE_LANG_AND_ID:C,ARMClang,AppleClang,Clang,GNU,LCC>")
set(msvc_cxx "$<COMPILE_LANG_AND_ID:C,MSVC>")
# clangd interopability.
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

find_package(SDL2 REQUIRED COMPONENTS SDL2)

add_library(gblib bus.c cartridge.c mbc.c gb.c cpu.c instruction.c)

target_link_libraries(gblib PUBLIC SDL2::SDL2)

# Link header files.
target_include_directories(gblib PUBLIC
                           "${PROJECT_SOURCE_DIR}")

# Compiler flags.
target_compile_options(gblib PUBLIC
  "$<${gcc_like_cxx}:-g;-Wall;-Wextra;-pedantic;-Wunused>"
  "$<${msvc_cxx}:-W3>>")
