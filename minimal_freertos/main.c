// Camada 3: minimal_usb + FreeRTOS.
// Mesma estrutura do main.cpp original (2 tasks com fila),
// mas SEM Edge Impulse. Se travar aqui, problema e FreeRTOS/stack.
// Se funcionar, culpado e o EI.

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <stdio.h>

#define LED_R 7
#define LED_G 8
#define LED_B 9
#define LED_ONBOARD 25

static QueueHandle_t xQ;

static void pin_setup(uint pin) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_put(pin, 0);
}

// Task "produtora" (paralela com mpu_task do main.cpp): manda contador na fila.
static void producer_task(void *p) {
    (void)p;
    uint32_t i = 0;
    while (1) {
        xQueueSend(xQ, &i, 0);
        i++;
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// Task "consumidora" (paralela com inference_task): le fila e pisca + imprime.
static void consumer_task(void *p) {
    (void)p;
    uint32_t value;
    while (1) {
        if (xQueueReceive(xQ, &value, portMAX_DELAY) == pdPASS) {
            printf("recv=%lu\n", (unsigned long)value);
            switch (value % 3) {
                case 0: gpio_put(LED_R, 1); gpio_put(LED_G, 0); gpio_put(LED_B, 0); break;
                case 1: gpio_put(LED_R, 0); gpio_put(LED_G, 1); gpio_put(LED_B, 0); break;
                case 2: gpio_put(LED_R, 0); gpio_put(LED_G, 0); gpio_put(LED_B, 1); break;
            }
            gpio_put(LED_ONBOARD, value & 1);
        }
    }
}

int main(void) {
    // FASE 1: sinaliza com LED de bordo que main() rodou (3 blinks).
    pin_setup(LED_R);
    pin_setup(LED_G);
    pin_setup(LED_B);
    pin_setup(LED_ONBOARD);
    for (int i = 0; i < 3; i++) {
        gpio_put(LED_ONBOARD, 1); sleep_ms(200);
        gpio_put(LED_ONBOARD, 0); sleep_ms(200);
    }

    // FASE 2: USB stdio.
    stdio_init_all();

    // FASE 3: marcador visual (vermelho fixo 1s) antes de criar tasks.
    gpio_put(LED_R, 1);
    sleep_ms(1000);
    gpio_put(LED_R, 0);

    // FASE 4: cria fila e tasks com tamanhos parecidos com o main.cpp original.
    xQ = xQueueCreate(1, sizeof(uint32_t));

    xTaskCreate(producer_task, "producer", 4096, NULL, 2, NULL);
    xTaskCreate(consumer_task, "consumer", 8192, NULL, 1, NULL);

    // FASE 5: scheduler. Se travar aqui, problema esta no FreeRTOS port.
    vTaskStartScheduler();

    // Nao deveria chegar aqui.
    while (1) {
        gpio_put(LED_R, 1); gpio_put(LED_G, 1); gpio_put(LED_B, 1);
        sleep_ms(100);
        gpio_put(LED_R, 0); gpio_put(LED_G, 0); gpio_put(LED_B, 0);
        sleep_ms(100);
    }
}
