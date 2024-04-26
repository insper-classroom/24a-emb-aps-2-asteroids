/* Main header file for FreeRTOS LED blink project */

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>
#include <string.h>
#include <stdio.h>
#include "hardware/adc.h"
#include "pico/stdlib.h"
#include "hc06.h"

#define SHOOTING_BUTTON 8
#define HYPERSPACE_BUTTON 15
#define ENCA_PIN 7
#define ENCB_PIN 6
#define THRUST_PIN 26

#define UART_ID uart1
#define BAUD_RATE 9600
#define UART_TX_PIN 0
#define UART_RX_PIN 1

// Function declarations
void btn_callback(uint gpio, uint32_t events);
void hc06_initialization();
