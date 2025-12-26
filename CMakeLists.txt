cmake_minimum_required(VERSION 3.15)
project(SpotsProcessor C)

# clangd stuff
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(stb REQUIRED)

add_executable(spots src/main.c)

target_compile_options(spots PRIVATE
    -O3
    -march=native
    # -Rpass=loop-vectorize # too verbose when failing on non optimizable code
    # -Rpass-analysis=loop-vectorize
)

target_link_libraries(spots PRIVATE stb::stb)

# vim: set ts=4 sw=4 et sts=4:
