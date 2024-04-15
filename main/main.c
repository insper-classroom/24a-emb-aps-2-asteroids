/*
 * LED blink with FreeRTOS
 */
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include <string.h>

#include "pico/stdlib.h"
#include <stdio.h>

#include "hc06.h"

#define SHOOTING_BUTTON 5  // Shooting button
#define ENCA_PIN 7
#define ENCB_PIN 6

#define UART_ID uart0
#define BAUD_RATE 115200

// GPIO pins used for UART
#define UART_TX_PIN 0
#define UART_RX_PIN 1

SemaphoreHandle_t xButtonSemaphore;

// void hc06_task(void *p) {
//     uart_init(HC06_UART_ID, HC06_BAUD_RATE);
//     gpio_set_function(HC06_TX_PIN, GPIO_FUNC_UART);
//     gpio_set_function(HC06_RX_PIN, GPIO_FUNC_UART);
//     hc06_init("aps2_legal", "1234");

//     while (true) {
//         uart_puts(HC06_UART_ID, "OLAAA ");
//         vTaskDelay(pdMS_TO_TICKS(100));
//     }
// }

// PUT THE FOLLOWING IN MAIN FUNCTION:
    //printf("Start bluetooth task\n");
    //xTaskCreate(hc06_task, "UART_Task 1", 4096, NULL, 1, NULL);


void btn_callback(uint gpio, uint32_t events) {
    if (gpio == SHOOTING_BUTTON) {
        xSemaphoreGiveFromISR(xButtonSemaphore, NULL);
    }
}

void rotate_task(void *p) {
    static const int8_t state_table[] = {
        0, -1,  1,  0,
        1,  0,  0, -1,
        -1,  0,  0,  1,
        0,  1, -1,  0
    };
    uint8_t enc_state = 0; // Current state of the encoder
    int8_t last_encoded = 0; // Last encoded state
    int8_t encoded;
    int sum;
    int last_sum = 0; // Last non-zero sum to filter out noise
    int debounce_counter = 0; // Debounce counter

    // Initialize GPIO pins for the encoder
    gpio_init(ENCA_PIN);
    gpio_init(ENCB_PIN);

    gpio_set_dir(ENCA_PIN, GPIO_IN);
    gpio_set_dir(ENCB_PIN, GPIO_IN);

    gpio_pull_up(ENCA_PIN);  // Enable internal pull-up
    gpio_pull_up(ENCB_PIN);  // Enable internal pull-up

    last_encoded = (gpio_get(ENCA_PIN) << 1) | gpio_get(ENCB_PIN);

    while (1) {
        encoded = (gpio_get(ENCA_PIN) << 1) | gpio_get(ENCB_PIN);
        enc_state = (enc_state << 2) | encoded;
        sum = state_table[enc_state & 0x0f];

        if (sum != 0) {
            if (sum == last_sum) {
                if (++debounce_counter > 1) {  // Check if the same movement is read consecutively
                    if (sum == 1) {
                        printf("RIGHT\n");
                    } else if (sum == -1) {
                        printf("LEFT\n");
                    }
                    debounce_counter = 0;  // Reset the counter after confirming the direction
                }
            } else {
                debounce_counter = 0;  // Reset the counter if the direction changes
            }
            last_sum = sum;  // Update last_sum to the current sum
        }

        vTaskDelay(pdMS_TO_TICKS(1)); // Poll every 1 ms to improve responsiveness
    }
}


void shooting_task(void *p) {
    // Initialize GPIO pin for button input
    gpio_init(SHOOTING_BUTTON);
    gpio_set_dir(SHOOTING_BUTTON, GPIO_IN);
    gpio_pull_up(SHOOTING_BUTTON);  // Enable internal pull-up

    // Register the unified callback for all buttons
    gpio_set_irq_enabled_with_callback(SHOOTING_BUTTON, GPIO_IRQ_EDGE_FALL, true, &btn_callback);

    //initialize_uart();

    uint8_t message[4];

    // Wait for button press event
    while (1) {
        if (xSemaphoreTake(xButtonSemaphore, portMAX_DELAY) == pdTRUE) {
            //printf("BANG\n");
            uart_putc_raw(uart0, 3);
            uart_putc_raw(uart0, 1);
            uart_putc_raw(uart0, 0);
            uart_putc_raw(uart0, -1);
            // Debouncing by delaying the re-enable of the interrupt
            vTaskDelay(pdMS_TO_TICKS(200));
            gpio_set_irq_enabled(SHOOTING_BUTTON, GPIO_IRQ_EDGE_FALL, true);
        }
    }
}

int main() {
    stdio_init_all();
    xButtonSemaphore = xSemaphoreCreateBinary();

    xTaskCreate(shooting_task, "shooting_task", 4096, NULL, 1, NULL);
    xTaskCreate(rotate_task, "rotate_task", 4096, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true)
        ;
}
       