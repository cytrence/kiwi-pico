project(picodvi C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(DVI_DEFAULT_SERIAL_CONFIG "pico_sock_cfg" CACHE STRING "")

add_executable(snake__c_tusb main.c)

target_compile_options(snake__c_tusb PRIVATE -Wall)

target_compile_definitions(snake__c_tusb PRIVATE DVI_DEFAULT_SERIAL_CONFIG=${DVI_DEFAULT_SERIAL_CONFIG})

target_include_directories(snake__c_tusb PUBLIC
    ${CMAKE_SOURCE_DIR}/lib/tinyusb/src
    ${CMAKE_CURRENT_LIST_DIR}
    ${LIBDVI_PATH}/include
)

target_sources(snake__c_tusb PUBLIC ${CMAKE_CURRENT_LIST_DIR}/hid_app.c)

target_link_libraries(snake__c_tusb PUBLIC
    pico_stdlib 
    tinyusb_host 
    tinyusb_device 
    tinyusb_board 
    libdvi
    pico_multicore
)

pico_add_extra_outputs(snake__c_tusb)