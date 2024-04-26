/*
 * LED blink with FreeRTOS
 */
#include "controller.h"

SemaphoreHandle_t xShootingSemaphore;
SemaphoreHandle_t xHyperspaceSemaphore;



void btn_callback(uint gpio, uint32_t events) {
    if (gpio == SHOOTING_BUTTON) {
        xSemaphoreGiveFromISR(xShootingSemaphore, NULL);
    } else if (gpio == HYPERSPACE_BUTTON) {
        xSemaphoreGiveFromISR(xHyperspaceSemaphore, NULL);
    }
}

void hc06_initialization(){
    uart_init(HC06_UART_ID, HC06_BAUD_RATE);
    gpio_set_function(HC06_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(HC06_RX_PIN, GPIO_FUNC_UART);
    hc06_init("asteroids", "8007");
}


void rotate_task(void *p) {
    static const int8_t state_table[] = {
        0, -1,  1,  0,
        1,  0,  0, -1,
        -1,  0,  0,  1,
        0,  1, -1,  0
    };
    uint8_t enc_state = 0; // Current state of the encoder
    
    int last_sum = 0; // Last non-zero sum to filter out noise
    int debounce_counter = 0; // Debounce counter

    // Initialize GPIO pins for the encoder
    gpio_init(ENCA_PIN);
    gpio_init(ENCB_PIN);

    gpio_set_dir(ENCA_PIN, GPIO_IN);
    gpio_set_dir(ENCB_PIN, GPIO_IN);

    gpio_pull_up(ENCA_PIN);  // Enable internal pull-up
    gpio_pull_up(ENCB_PIN);  // Enable internal pull-up

    
    while (1) {
        int8_t encoded = (gpio_get(ENCA_PIN) << 1) | gpio_get(ENCB_PIN);
        enc_state = (enc_state << 2) | encoded;
        int sum = state_table[enc_state & 0x0f];

        if (sum != 0) {
            if (sum == last_sum) {
                if (++debounce_counter > 1) {  // Check if the same movement is read consecutively
                    if (sum == 1) {
                        //printf("RIGHT\n");
                        uart_putc_raw(uart1, 3);
                        uart_putc_raw(uart1, 2);
                        uart_putc_raw(uart1, 0);
                        uart_putc_raw(uart1, -1);
                    } else if (sum == -1) {
                        //printf("LEFT\n");
                        uart_putc_raw(uart1, 3);
                        uart_putc_raw(uart1, 3);
                        uart_putc_raw(uart1, 0);
                        uart_putc_raw(uart1, -1);
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
    //hc06_initialization();

    //initialize_uart();

    

    // Wait for button press event
    while (1) {
        if (xSemaphoreTake(xShootingSemaphore, portMAX_DELAY) == pdTRUE) {
            //printf("BANG\n");
            uart_putc_raw(uart1, 3);
            uart_putc_raw(uart1, 1);
            uart_putc_raw(uart1, 0);
            uart_putc_raw(uart1, -1);
            // Debouncing by delaying the re-enable of the interrupt
            vTaskDelay(pdMS_TO_TICKS(200));
            gpio_set_irq_enabled(SHOOTING_BUTTON, GPIO_IRQ_EDGE_FALL, true);
        }
    }
}

void thrust_task(void *p) {
    adc_init();
    adc_gpio_init(THRUST_PIN);
    uint16_t threshold = 2000;  
    bool thrust_active = false;  // State variable to track thrust status
    //hc06_initialization();

    while (1) {
        adc_select_input(0);  // Select ADC0
        uint16_t result = adc_read();
        //printf("-----%d-------\n", result);

        if (result > threshold && !thrust_active) {
            // If the result is above the threshold and thrust is not already active
            uart_putc_raw(uart1, 3);
            uart_putc_raw(uart1, 4);
            uart_putc_raw(uart1, 0);
            uart_putc_raw(uart1, -1);

            thrust_active = true;  // Set thrust as active
            //printf("THRUST ACTIVATED\n");
        } else if (result <= threshold && thrust_active) {
            // If the result is below the threshold and thrust is currently active
            uart_putc_raw(uart1, 3);
            uart_putc_raw(uart1, 5);
            uart_putc_raw(uart1, 0);
            uart_putc_raw(uart1, -1);

            thrust_active = false;  // Set thrust as inactive
            //printf("THRUST DEACTIVATED\n");
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void hyperspace_task(void *p) {
    gpio_init(HYPERSPACE_BUTTON);
    gpio_set_dir(HYPERSPACE_BUTTON, GPIO_IN);
    gpio_pull_up(HYPERSPACE_BUTTON);  
    gpio_set_irq_enabled_with_callback(HYPERSPACE_BUTTON, GPIO_IRQ_EDGE_FALL, true, &btn_callback);
    hc06_initialization();

    while (1) {
        if (xSemaphoreTake(xHyperspaceSemaphore, portMAX_DELAY) == pdTRUE) {
            //printf("HYPERSPACE\n");
            uart_putc_raw(uart1, 3);
            uart_putc_raw(uart1, 6);
            uart_putc_raw(uart1, 0);
            uart_putc_raw(uart1, -1);
            vTaskDelay(pdMS_TO_TICKS(200));
            gpio_set_irq_enabled(HYPERSPACE_BUTTON, GPIO_IRQ_EDGE_FALL, true);
        }
    }
}
                        
int main() {
    stdio_init_all();
    
    xShootingSemaphore = xSemaphoreCreateBinary();
    xHyperspaceSemaphore = xSemaphoreCreateBinary();


    xTaskCreate(shooting_task, "shooting_task", 4096, NULL, 1, NULL);
    xTaskCreate(rotate_task, "rotate_task", 4096, NULL, 1, NULL);
    xTaskCreate(thrust_task, "thrust_task", 4096, NULL, 1, NULL);
    xTaskCreate(hyperspace_task, "hyperspace_task", 4096, NULL, 1, NULL);


    vTaskStartScheduler();

    while (true)
        ;
}
                                        