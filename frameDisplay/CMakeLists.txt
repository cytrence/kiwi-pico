cmake_minimum_required(VERSION 3.13)
project(PicoDVI_Display C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(DVI_DEFAULT_SERIAL_CONFIG "pico_sock_cfg" CACHE STRING "")

add_executable(frameDisplay main.c)

target_compile_options(frameDisplay PRIVATE -Wall)

target_compile_definitions(frameDisplay PRIVATE DVI_DEFAULT_SERIAL_CONFIG=${DVI_DEFAULT_SERIAL_CONFIG})

target_include_directories(frameDisplay PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    ${LIBDVI_PATH}/include
)

target_link_libraries(frameDisplay PUBLIC
    pico_stdlib 
    pico_multicore
    libdvi
)

pico_add_extra_outputs(frameDisplay)