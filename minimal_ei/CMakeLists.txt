set(EI_SDK_FOLDER ${CMAKE_SOURCE_DIR}/edge-impulse-sdk)
set(MODEL_FOLDER ${CMAKE_SOURCE_DIR}/tflite-model)

add_executable(minimal_ei
    main.cpp
    ${MODEL_FOLDER}/tflite_learn_1002973_3_compiled.cpp
)

# Mesmos fontes do EI SDK que main/CMakeLists.txt usa.
file(GLOB_RECURSE EI_SDK_SOURCES
    "${EI_SDK_FOLDER}/*.cpp"
    "${EI_SDK_FOLDER}/*.cc"
    "${EI_SDK_FOLDER}/*.c"
    "${EI_SDK_FOLDER}/*.s"
)
target_sources(minimal_ei PRIVATE ${EI_SDK_SOURCES})

target_include_directories(minimal_ei PRIVATE
    ${CMAKE_SOURCE_DIR}/main
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

target_compile_definitions(minimal_ei PRIVATE
    # configNUMBER_OF_CORES=1   # REMOVIDO: inconsistencia com FreeRTOS-Kernel-Heap4 (SMP, 2 cores)
    EI_PORTING_RASPBERRY_PI_PICO=1
    EIDSP_USE_CMSIS_DSP=0
    TF_LITE_DISABLE_X86_NEON=1
    EI_CLASSIFIER_TFLITE_ENABLE_CMSIS_NN=0
    EI_CLASSIFIER_ALLOCATION_STATIC=1
)

target_link_libraries(minimal_ei PRIVATE
    pico_stdlib
    hardware_gpio
    hardware_i2c
    pico_async_context_freertos
    FreeRTOS-Kernel-Heap4
    m
)

pico_enable_stdio_usb(minimal_ei 1)
pico_enable_stdio_uart(minimal_ei 0)

pico_add_extra_outputs(minimal_ei)
