cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

set(projname "pico_usbhost")

project(${projname} C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_subdirectory(libs/tinyusb)

add_executable(${projname}
    main.c
)

target_include_directories(${projname} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

pico_enable_stdio_uart(${projname} 1)
pico_add_extra_outputs(${projname})

target_link_libraries(${projname} pico_stdlib mytinyusb_host mytinyusb_board)