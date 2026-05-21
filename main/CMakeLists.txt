if (NOT FREERTOS_KERNEL_PATH AND NOT DEFINED ENV{FREERTOS_KERNEL_PATH})
    message("Skipping FreeRTOS examples as FREERTOS_KERNEL_PATH not defined")
    return()
endif()

include(FreeRTOS_Kernel_import.cmake)

# Caminhos do Edge Impulse SDK
set(EI_SDK_FOLDER ${CMAKE_SOURCE_DIR}/edge-impulse-sdk)
set(MODEL_FOLDER ${CMAKE_SOURCE_DIR}/tflite-model)

# Adiciona o executável (main.cpp + modelo)
add_executable(pico_emb
    main.cpp
    ${MODEL_FOLDER}/tflite_learn_1002973_3_compiled.cpp
)

# Busca recursivamente todos os fontes do Edge Impulse SDK
file(GLOB_RECURSE EI_SDK_SOURCES
    "${EI_SDK_FOLDER}/*.cpp"
    "${EI_SDK_FOLDER}/*.cc"
    "${EI_SDK_FOLDER}/*.c"
    "${EI_SDK_FOLDER}/*.s"
)

target_sources(pico_emb PRIVATE ${EI_SDK_SOURCES})

# Diretórios de include
target_include_directories(pico_emb PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_SOURCE_DIR}
    ${EI_SDK_FOLDER}
    ${EI_SDK_FOLDER}/third_party/flatbuffers/include
    ${EI_SDK_FOLDER}/third_party/gemmlowp
    ${EI_SDK_FOLDER}/third_party/ruy
    ${EI_SDK_FOLDER}/third_party
    ${EI_SDK_FOLDER}/tensorflow
    ${EI_SDK_FOLDER}/CMSIS/Core/Include
    ${EI_SDK_FOLDER}/CMSIS/DSP/Include
    ${EI_SDK_FOLDER}/CMSIS/DSP/PrivateInclude
    ${EI_SDK_FOLDER}/CMSIS/NN/Include
    ${MODEL_FOLDER}
    ${CMAKE_SOURCE_DIR}/model-parameters
)

# Definições de compilação necessárias pro Edge Impulse SDK
target_compile_definitions(pico_emb PRIVATE
    EI_PORTING_RASPBERRY_PI_PICO=1
    EIDSP_USE_CMSIS_DSP=0
    TF_LITE_DISABLE_X86_NEON=1
    EI_CLASSIFIER_TFLITE_ENABLE_CMSIS_NN=0
    EI_CLASSIFIER_ALLOCATION_STATIC=1     # FIX: tensor_arena estatico (evita dynamic init que travava o boot)
)

# Linka bibliotecas
target_link_libraries(pico_emb PRIVATE
    pico_async_context_freertos
    FreeRTOS-Kernel-Heap4
    hardware_i2c
    pico_stdlib
    pico_multicore
    m
)

if(PICO_CYW43_SUPPORTED)
    target_link_libraries(pico_emb PRIVATE
        pico_cyw43_arch_none
    )
endif()

# Habilita USB pra stdio
pico_enable_stdio_usb(pico_emb 1)
pico_enable_stdio_uart(pico_emb 0)

pico_add_extra_outputs(pico_emb)

set_target_properties(pico_emb PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})