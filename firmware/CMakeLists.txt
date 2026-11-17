cmake_minimum_required(VERSION 3.13)
set(PICO_BOARD pico2_w)
set(PICO_PLATFORM rp2350-arm-s)
include(pico_sdk_import.cmake)
project(procontroller_dongle C CXX ASM)
pico_sdk_init()

add_executable(procontroller_dongle src/main.c src/procon.c src/usb_procon.c)
target_include_directories(procontroller_dongle PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)
target_link_libraries(procontroller_dongle
    pico_stdlib
    pico_cyw43_arch_none
    pico_btstack_classic
    pico_btstack_cyw43
    tinyusb_device)
pico_enable_stdio_usb(procontroller_dongle 0)
pico_enable_stdio_uart(procontroller_dongle 0)

# Build release (-DPROCON_RELEASE=ON): sem porta serial de log; descritor USB identico ao Pro Controller original.
option(PROCON_RELEASE "Build final: sem CDC de diagnostico e sem log" OFF)
option(PROCON_HOME_PASSTHROUGH "Repassar a luz do Home pedida pelo PC (senao o dongle controla a luz)" OFF)
option(PROCON_WAKE "Interface de teclado HID para acordar o PC da suspensao" ON)
if(PROCON_RELEASE)
    target_compile_definitions(procontroller_dongle PRIVATE PROCON_DEBUG_CDC=0 PROCON_VERBOSE_DEFAULT=0)
else()
    target_compile_definitions(procontroller_dongle PRIVATE PROCON_DEBUG_CDC=1 PROCON_VERBOSE_DEFAULT=1)
endif()
if(PROCON_HOME_PASSTHROUGH)
    target_compile_definitions(procontroller_dongle PRIVATE PROCON_HOME_PASSTHROUGH=1)
endif()
if(PROCON_WAKE)
    target_compile_definitions(procontroller_dongle PRIVATE PROCON_WAKE=1)
else()
    target_compile_definitions(procontroller_dongle PRIVATE PROCON_WAKE=0)
endif()
pico_add_extra_outputs(procontroller_dongle)
