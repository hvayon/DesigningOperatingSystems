cmake_minimum_required(VERSION 3.30)
project(slab_allocator C)

set(CMAKE_C_STANDARD 11)

add_executable(slab_allocator
        main.c slab.c)
