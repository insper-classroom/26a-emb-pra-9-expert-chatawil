#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"

#include <stdio.h>
#include <string.h>

#include "pins.h"
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
#include "edge-impulse-sdk/dsp/numpy.hpp"
#include "model-parameters/model_metadata.h"

// =========================================================
// CONFIGURAÇÕES
// =========================================================
#define SAMPLE_RATE_HZ        83
#define SAMPLE_INTERVAL_MS    (1000 / SAMPLE_RATE_HZ)   // ~12ms
#define WINDOW_SAMPLES        EI_CLASSIFIER_RAW_SAMPLE_COUNT
#define FEATURE_SIZE          EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE
#define CONFIDENCE_THRESHOLD  0.7f

// Fila com 1 buffer cheio por vez
static QueueHandle_t xWindowQueue;

// Tipo de cada mensagem na fila: um buffer pronto pra inferência
typedef struct {
    float features[FEATURE_SIZE];
} window_t;

// =========================================================
// MPU6050
// =========================================================
static void mpu6050_init(void) {
    i2c_init(i2c_default, 400 * 1000);
    gpio_set_function(I2C_SDA_GPIO, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_GPIO, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_GPIO);
    gpio_pull_up(I2C_SCL_GPIO);

    // Acorda o MPU6050 (registrador 0x6B = 0)
    uint8_t buf[] = {0x6B, 0x00};
    i2c_write_blocking(i2c_default, MPU_ADDRESS, buf, 2, false);
}

static void mpu6050_read_accel(int16_t accel[3]) {
    uint8_t buffer[6];
    uint8_t reg = 0x3B;  // Registrador inicial do acelerômetro
    i2c_write_blocking(i2c_default, MPU_ADDRESS, &reg, 1, true);
    i2c_read_blocking(i2c_default, MPU_ADDRESS, buffer, 6, false);

    for (int i = 0; i < 3; i++) {
        accel[i] = (buffer[i * 2] << 8) | buffer[(i * 2) + 1];
    }
}

// =========================================================
// LED RGB
// =========================================================
static void led_init(void) {
    gpio_init(LED_PIN_R); gpio_set_dir(LED_PIN_R, GPIO_OUT);
    gpio_init(LED_PIN_G); gpio_set_dir(LED_PIN_G, GPIO_OUT);
    gpio_init(LED_PIN_B); gpio_set_dir(LED_PIN_B, GPIO_OUT);
    gpio_put(LED_PIN_R, 0);
    gpio_put(LED_PIN_G, 0);
    gpio_put(LED_PIN_B, 0);
}

static void led_set(bool r, bool g, bool b) {
    gpio_put(LED_PIN_R, r);
    gpio_put(LED_PIN_G, g);
    gpio_put(LED_PIN_B, b);
}

// =========================================================
// TASK 1 — Lê IMU e empilha amostras
// =========================================================
extern "C" void mpu_task(void *p) {
    (void)p;
    mpu6050_init();
    vTaskDelay(pdMS_TO_TICKS(100));

    window_t window;
    int sample_idx = 0;

    while (1) {
        int16_t accel[3];
        mpu6050_read_accel(accel);

        // Empilha as 3 leituras (X, Y, Z) na janela
        window.features[sample_idx * 3 + 0] = (float)accel[0];
        window.features[sample_idx * 3 + 1] = (float)accel[1];
        window.features[sample_idx * 3 + 2] = (float)accel[2];

        sample_idx++;

        // Quando completou a janela, envia pra inference_task
        if (sample_idx >= (int)WINDOW_SAMPLES) {
            xQueueSend(xWindowQueue, &window, 0);
            sample_idx = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(SAMPLE_INTERVAL_MS));
    }
}

// =========================================================
// TASK 2 — Roda inferência e controla LED
// =========================================================
extern "C" void inference_task(void *p) {
    (void)p;
    led_init();

    window_t window;
    ei_impulse_result_t result = {0};

    while (1) {
        if (xQueueReceive(xWindowQueue, &window, portMAX_DELAY) == pdPASS) {
            signal_t features_signal;
            numpy::signal_from_buffer(window.features, FEATURE_SIZE, &features_signal);

            EI_IMPULSE_ERROR res = run_classifier(&features_signal, &result, false);

            if (res == EI_IMPULSE_OK) {
                // Encontra a classe com maior confiança
                float best_val = 0.0f;
                int best_idx = -1;
                for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
                    printf("%s: %.2f  ", result.classification[i].label,
                                          result.classification[i].value);
                    if (result.classification[i].value > best_val) {
                        best_val = result.classification[i].value;
                        best_idx = i;
                    }
                }
                printf("\n");

                // Acende o LED se tiver confiança suficiente
                if (best_val >= CONFIDENCE_THRESHOLD && best_idx >= 0) {
                    const char *label = result.classification[best_idx].label;
                    if (strcmp(label, "idle") == 0) {
                        led_set(0, 0, 1);  // azul
                    } else if (strcmp(label, "updown") == 0) {
                        led_set(1, 0, 0);  // vermelho
                    } else if (strcmp(label, "wave") == 0) {
                        led_set(0, 1, 0);  // verde
                    }
                } else {
                    led_set(0, 0, 0);  // apaga se incerto
                }
            } else {
                printf("Erro na inferencia: %d\n", res);
            }
        }
    }
}

// =========================================================
// MAIN
// =========================================================
int main(void) {
    stdio_init_all();

    // Inicializa o LED RGB pra teste
    gpio_init(LED_PIN_R); gpio_set_dir(LED_PIN_R, GPIO_OUT);
    gpio_init(LED_PIN_G); gpio_set_dir(LED_PIN_G, GPIO_OUT);
    gpio_init(LED_PIN_B); gpio_set_dir(LED_PIN_B, GPIO_OUT);

    // Pisca o LED em 3 cores pra mostrar que main() rodou
    for (int i = 0; i < 3; i++) {
        gpio_put(LED_PIN_R, 1); gpio_put(LED_PIN_G, 0); gpio_put(LED_PIN_B, 0);
        sleep_ms(300);
        gpio_put(LED_PIN_R, 0); gpio_put(LED_PIN_G, 1); gpio_put(LED_PIN_B, 0);
        sleep_ms(300);
        gpio_put(LED_PIN_R, 0); gpio_put(LED_PIN_G, 0); gpio_put(LED_PIN_B, 1);
        sleep_ms(300);
    }
    // Apaga tudo
    gpio_put(LED_PIN_R, 0); gpio_put(LED_PIN_G, 0); gpio_put(LED_PIN_B, 0);

    sleep_ms(2000);

    printf("=== Pico 2 + Edge Impulse ===\n");
    printf("Window samples: %d, Feature size: %d\n",
           (int)WINDOW_SAMPLES, (int)FEATURE_SIZE);

    xWindowQueue = xQueueCreate(1, sizeof(window_t));

    xTaskCreate(mpu_task,       "mpu_task",       4096, NULL, 2, NULL);
    xTaskCreate(inference_task, "inference_task", 8192, NULL, 1, NULL);

    vTaskStartScheduler();
    while (1);
}