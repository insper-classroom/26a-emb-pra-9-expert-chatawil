add_executable(minimal_usb
    main.c
)

target_link_libraries(minimal_usb PRIVATE
    pico_stdlib
    hardware_gpio
)

# USB stdio LIGADO (cenario igual ao main.cpp original).
pico_enable_stdio_usb(minimal_usb 1)
pico_enable_stdio_uart(minimal_usb 0)

pico_add_extra_outputs(minimal_usb)
