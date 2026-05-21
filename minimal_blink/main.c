// Teste minimo absoluto: piscar LED RGB sem USB, sem FreeRTOS, sem Edge Impulse.
// Se este .uf2 nao piscar o LED, problema esta em hardware/build/flash, nao em FreeRTOS/EI.

#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define LED_R 7
#define LED_G 8
#define LED_B 9

// LED on-board do Pico (pra detectar se o LED externo tem polaridade invertida).
// Em Pico 2 standard o LED de bordo esta no GPIO 25.
#define LED_ONBOARD 25

static void pin_setup(uint pin) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_put(pin, 0);
}

int main(void) {
    pin_setup(LED_R);
    pin_setup(LED_G);
    pin_setup(LED_B);
    pin_setup(LED_ONBOARD);

    // Pulso inicial agressivo: acende TUDO por 500ms.
    // Se o LED externo for anodo-comum, ele apaga aqui em vez de acender.
    gpio_put(LED_R, 1);
    gpio_put(LED_G, 1);
    gpio_put(LED_B, 1);
    gpio_put(LED_ONBOARD, 1);
    sleep_ms(500);

    gpio_put(LED_R, 0);
    gpio_put(LED_G, 0);
    gpio_put(LED_B, 0);
    gpio_put(LED_ONBOARD, 0);
    sleep_ms(500);

    while (1) {
        // R
        gpio_put(LED_R, 1); gpio_put(LED_ONBOARD, 1);
        sleep_ms(300);
        gpio_put(LED_R, 0); gpio_put(LED_ONBOARD, 0);

        // G
        gpio_put(LED_G, 1); gpio_put(LED_ONBOARD, 1);
        sleep_ms(300);
        gpio_put(LED_G, 0); gpio_put(LED_ONBOARD, 0);

        // B
        gpio_put(LED_B, 1); gpio_put(LED_ONBOARD, 1);
        sleep_ms(300);
        gpio_put(LED_B, 0); gpio_put(LED_ONBOARD, 0);

        sleep_ms(300);
    }
}
