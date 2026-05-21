add_executable(minimal_freertos
    main.c
)

# Reusa FreeRTOSConfig.h que ja existe em main/
target_include_directories(minimal_freertos PRIVATE
    ${CMAKE_SOURCE_DIR}/main
)

target_link_libraries(minimal_freertos PRIVATE
    pico_stdlib
    hardware_gpio
    pico_async_context_freertos
    FreeRTOS-Kernel-Heap4
)

pico_enable_stdio_usb(minimal_freertos 1)
pico_enable_stdio_uart(minimal_freertos 0)

pico_add_extra_outputs(minimal_freertos)
