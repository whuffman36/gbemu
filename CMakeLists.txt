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

add_subdirectory(src/lib)
add_executable(gbemu src/gbemu.c)
add_executable(gbemu-test tests/test.c)

# Link libraries.
target_link_libraries(gbemu PUBLIC gblib)
target_link_libraries(gbemu PUBLIC SDL2::SDL2)
target_link_libraries(gbemu-test PUBLIC gblib)

# Link header files.
target_include_directories(gbemu PUBLIC
                          "${PROJECT_BINARY_DIR}"
                          "${PROJECT_SOURCE_DIR}/lib"
                          )
target_include_directories(gbemu-test PUBLIC
                          "${PROJECT_BINARY_DIR}"
                          "${PROJECT_SOURCE_DIR}/lib"
                          )

if (APPLE)
  set(CMAKE_EXE_LINKER_FLAGS "-Wl,-stack_size,0x80000")
endif (APPLE)

# Compiler flags.
target_compile_options(gbemu PUBLIC
  "$<${gcc_like_cxx}:-g;-Wall;-Wextra;-pedantic;-Wunused>"
  "$<${msvc_cxx}:-W3>>")
target_compile_options(gbemu-test PUBLIC
  "$<${gcc_like_cxx}:-Wall;-Wextra;-pedantic;-Wunused>"
  "$<${msvc_cxx}:-W3>>")
