add_executable(minimal_blink
    main.c
)

target_link_libraries(minimal_blink PRIVATE
    pico_stdlib
    hardware_gpio
)

# SEM stdio USB/UART — queremos o setup mais simples possivel.
pico_enable_stdio_usb(minimal_blink 0)
pico_enable_stdio_uart(minimal_blink 0)

pico_add_extra_outputs(minimal_blink)

set_target_properties(minimal_blink PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
