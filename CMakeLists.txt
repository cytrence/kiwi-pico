cmake_minimum_required(VERSION 3.12)

include(pico_sdk_import.cmake)

project(picodvi C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()
set(LIBDVI_PATH "${CMAKE_CURRENT_LIST_DIR}/libs/PicoDVI/software")
add_subdirectory(${LIBDVI_PATH}/libdvi ${CMAKE_BINARY_DIR}/libdvi_build)
add_compile_options(-Wall)

add_subdirectory(snake)
add_subdirectory(frameDisplay)