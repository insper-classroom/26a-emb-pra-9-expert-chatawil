// Camada 2: minimal_blink + USB stdio.
// Estrategia: pisca ANTES de stdio_init_all (pra provar que main() rodou),
// depois init USB, depois printf no loop.
//
// Diagnostico:
//   - Se LED piscar 3x antes e depois travar: stdio_init_all bloqueia.
//   - Se LED piscar 3x antes E continuar piscando com printf: USB stdio OK,
//     entao o culpado no main.cpp e o Edge Impulse (construtores estaticos).

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <stdio.h>

#define LED_R 7
#define LED_G 8
#define LED_B 9
#define LED_ONBOARD 25

static void pin_setup(uint pin) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_put(pin, 0);
}

static void blink_once(uint pin, uint32_t ms) {
    gpio_put(pin, 1);
    sleep_ms(ms);
    gpio_put(pin, 0);
    sleep_ms(ms);
}

int main(void) {
    // FASE 1: LED antes de qualquer init USB.
    // Se NAO piscar 3x aqui, problema e construtor estatico ou hard fault na init.
    pin_setup(LED_R);
    pin_setup(LED_G);
    pin_setup(LED_B);
    pin_setup(LED_ONBOARD);

    for (int i = 0; i < 3; i++) {
        blink_once(LED_ONBOARD, 200);
    }

    // FASE 2: tenta inicializar USB stdio.
    // Se travar aqui, LED de bordo fica APAGADO (depois dos 3 blinks).
    stdio_init_all();

    // FASE 3: se chegou aqui, USB stdio inicializou.
    // Indica fase 3 acendendo vermelho fixo por 1s.
    gpio_put(LED_R, 1);
    sleep_ms(1000);
    gpio_put(LED_R, 0);

    // FASE 4: loop com printf + cores rotativas.
    int counter = 0;
    while (1) {
        printf("tick=%d\n", counter++);

        // Cor cicla a cada tick.
        switch (counter % 3) {
            case 0: gpio_put(LED_R, 1); gpio_put(LED_G, 0); gpio_put(LED_B, 0); break;
            case 1: gpio_put(LED_R, 0); gpio_put(LED_G, 1); gpio_put(LED_B, 0); break;
            case 2: gpio_put(LED_R, 0); gpio_put(LED_G, 0); gpio_put(LED_B, 1); break;
        }
        gpio_put(LED_ONBOARD, counter & 1);

        sleep_ms(500);
    }
}
