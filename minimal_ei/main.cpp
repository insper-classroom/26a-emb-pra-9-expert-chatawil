// Camada 4: minimal_freertos + Edge Impulse SDK linkado.
// NAO chamamos run_classifier — so o link e suficiente pra disparar
// construtores estaticos C++ do EI. Se o LED nao piscar mais o ciclo
// inicial, problema confirmado nos static initializers do EI.

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <stdio.h>

// Inclui headers do EI pra forcar link (mas nao usa run_classifier).
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
#include "model-parameters/model_metadata.h"

#define LED_R 7
#define LED_G 8
#define LED_B 9
#define LED_ONBOARD 25

// Construtor estatico com prioridade 101 (a mais alta possivel pra usuario).
// Roda ANTES de qualquer outro static init. Se LED nao piscar aqui, problema
// e em BSS/zero-init/boot, NAO em ctor C++. Se piscar, ctor posterior trava.
static void busy_wait(uint32_t loops) { for (volatile uint32_t j = 0; j < loops; j++) {} }

static void setup_all_leds(void) {
    gpio_init(LED_R); gpio_set_dir(LED_R, GPIO_OUT); gpio_put(LED_R, 0);
    gpio_init(LED_G); gpio_set_dir(LED_G, GPIO_OUT); gpio_put(LED_G, 0);
    gpio_init(LED_B); gpio_set_dir(LED_B, GPIO_OUT); gpio_put(LED_B, 0);
    gpio_init(LED_ONBOARD); gpio_set_dir(LED_ONBOARD, GPIO_OUT); gpio_put(LED_ONBOARD, 0);
}

// Cada landmark:
//   1. apaga tudo
//   2. pisca seu sinal lento
//   3. SEGURA sua cor por ~2 segundos (pra voce ver qual ficou)
//
// Se algo travar entre landmarks, voce verra a cor do ULTIMO landmark
// segurada eternamente (ou apagada se travou DENTRO do landmark).

#define HOLD busy_wait(8000000)    // ~2s segurando cor
#define BLINK_ON busy_wait(800000) // ~200ms

static void all_off(void) {
    gpio_put(LED_R, 0); gpio_put(LED_G, 0); gpio_put(LED_B, 0); gpio_put(LED_ONBOARD, 0);
}

__attribute__((constructor(101)))
static void lm_101(void) {
    setup_all_leds();
    // GP25 5x rapido + segura GP25 aceso
    for (int i = 0; i < 5; i++) { gpio_put(LED_ONBOARD, 1); BLINK_ON; gpio_put(LED_ONBOARD, 0); BLINK_ON; }
    gpio_put(LED_ONBOARD, 1); HOLD;
    all_off();
}

__attribute__((constructor(500)))
static void lm_500(void) {
    // VERMELHO segurado
    gpio_put(LED_R, 1); HOLD;
    all_off();
}

__attribute__((constructor(5000)))
static void lm_5000(void) {
    // VERDE segurado
    gpio_put(LED_G, 1); HOLD;
    all_off();
}

__attribute__((constructor(30000)))
static void lm_30000(void) {
    // AZUL segurado
    gpio_put(LED_B, 1); HOLD;
    all_off();
}

__attribute__((constructor(45000)))
static void lm_45000(void) {
    // AMARELO (R+G, sem B) — fica entre azul e ciano
    gpio_put(LED_R, 1); gpio_put(LED_G, 1); HOLD;
    all_off();
}

__attribute__((constructor(60000)))
static void lm_60000(void) {
    // CIANO (G+B, SEM vermelho) — inequivoco
    gpio_put(LED_G, 1); gpio_put(LED_B, 1); HOLD;
    all_off();
}

// Landmark BEM tarde, prioridade quase no maximo do user range
__attribute__((constructor(65500)))
static void lm_65500(void) {
    // MAGENTA (R+B) 2s
    gpio_put(LED_R, 1); gpio_put(LED_B, 1); HOLD;
    all_off();
    // Sinal de "sobrevivi": GP25 5x rapido. Se voce ver isso, todos os
    // landmarks rodaram e main() vai comecar logo depois.
    for (int i = 0; i < 5; i++) { gpio_put(LED_ONBOARD, 1); BLINK_ON; gpio_put(LED_ONBOARD, 0); BLINK_ON; }
}

// Marcadores INSIDE main() pra ver ate onde main() chega.
static void main_marker(int color_r, int color_g, int color_b) {
    all_off(); busy_wait(2000000); // 0.5s preto
    gpio_put(LED_R, color_r); gpio_put(LED_G, color_g); gpio_put(LED_B, color_b);
    HOLD;
    all_off();
}

static QueueHandle_t xQ;

static void pin_setup(uint pin) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_put(pin, 0);
}

extern "C" void producer_task(void *p) {
    (void)p;
    uint32_t i = 0;
    while (1) {
        xQueueSend(xQ, &i, 0);
        i++;
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

extern "C" void consumer_task(void *p) {
    (void)p;
    uint32_t value;
    while (1) {
        if (xQueueReceive(xQ, &value, portMAX_DELAY) == pdPASS) {
            printf("recv=%lu  (EI linked, classifier NOT called)\n", (unsigned long)value);
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
    pin_setup(LED_R);
    pin_setup(LED_G);
    pin_setup(LED_B);
    pin_setup(LED_ONBOARD);

    // M1: VERDE — main() entrou
    main_marker(0, 1, 0);

    stdio_init_all();

    // M2: AZUL — stdio_init_all completou
    main_marker(0, 0, 1);

    printf("=== camada 4c: EI + model + ALLOC_STATIC ===\n");

    // M3: CIANO (G+B, sem R) — printf nao travou
    main_marker(0, 1, 1);

    xQ = xQueueCreate(1, sizeof(uint32_t));

    // M4: AMARELO (R+G) — xQueueCreate funcionou (pode parecer vermelho)
    main_marker(1, 1, 0);

    xTaskCreate(producer_task, "producer", 4096, NULL, 2, NULL);
    xTaskCreate(consumer_task, "consumer", 8192, NULL, 1, NULL);

    // M5: MAGENTA (R+B) — xTaskCreate funcionou (pode parecer vermelho)
    main_marker(1, 0, 1);

    vTaskStartScheduler();

    while (1) {
        gpio_put(LED_R, 1); gpio_put(LED_G, 1); gpio_put(LED_B, 1);
        sleep_ms(100);
        gpio_put(LED_R, 0); gpio_put(LED_G, 0); gpio_put(LED_B, 0);
        sleep_ms(100);
    }
}
