cmake_minimum_required(VERSION 3.13)
project(PicoDVI_Display C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(DVI_DEFAULT_SERIAL_CONFIG "pico_sock_cfg" CACHE STRING "")

add_executable(pico_dvi_display main.c)

target_compile_options(pico_dvi_display PRIVATE -Wall)

target_compile_definitions(pico_dvi_display PRIVATE DVI_DEFAULT_SERIAL_CONFIG=${DVI_DEFAULT_SERIAL_CONFIG})

target_include_directories(pico_dvi_display PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    ${LIBDVI_PATH}/include
)

target_link_libraries(pico_dvi_display PUBLIC
    pico_stdlib 
    pico_multicore
    libdvi
)

pico_add_extra_outputs(pico_dvi_display)