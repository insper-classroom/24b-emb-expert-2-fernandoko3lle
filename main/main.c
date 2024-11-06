/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include <queue.h>
#include "hardware/pwm.h"

#include "ili9341.h"
#include "gfx.h"

#define GRAPH_WIDTH 320      // Width of the LCD screen
#define GRAPH_HEIGHT 240     // Height of the LCD screen
#define MAX_POT_VALUE 4095   // Maximum ADC value for 12-bit resolution
#define COLOR_WHITE 0xFFFF   // Color code for white (assuming RGB565 format)
#define COLOR_AXES 0x07E0    // Color code for axes (e.g., green)
#define COLOR_TEXT 0xFFFF    // Color code for text (white)
#define COLOR_GRAPH 0x001F   // Color code for graph line (blue)

QueueHandle_t xQueuePot;

int filtro(int data){
    static int vetor[20] = {0}; 
    static int index = 0;     

    vetor[index] = data;
    index = (index + 1) % 20; 

    int sum = 0;
    for(int i = 0; i < 20; i++) {
        sum += vetor[i];
    }
    int data_filtrado = sum / 20;

    return data_filtrado;
}


void potenciometro_task() {
    while (1) {
        int pot_value = 0;
        adc_select_input(0);
        pot_value = adc_read();

        printf("Valor do potenciômetro: %d\n", pot_value);
        xQueueSend(xQueuePot, &pot_value, 0);

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void draw_axes() {
    GFX_drawLine(30, 20, 30, GRAPH_HEIGHT - 30, COLOR_AXES);
    GFX_drawLine(30, GRAPH_HEIGHT - 30, GRAPH_WIDTH - 20, GRAPH_HEIGHT - 30, COLOR_AXES);

    GFX_setTextColor(COLOR_TEXT);
    GFX_setCursor(5, 20);
    GFX_printf("%d", MAX_POT_VALUE);

    GFX_setCursor(5, GRAPH_HEIGHT - 40);
    GFX_printf("0");
}

void display_title(int pot_value) {
    GFX_fillRect(0, 0, GRAPH_WIDTH, 20, 0x0000); // Fill with black

    char title_str[20];
    snprintf(title_str, sizeof(title_str), "Value: %d", pot_value);

    int text_width = strlen(title_str) * 6;
    int title_x = (GRAPH_WIDTH - text_width) / 2;

    GFX_setCursor(title_x, 5);

    GFX_setTextColor(COLOR_TEXT);
    GFX_printf("%s", title_str);
}

void display_task() {
    LCD_initDisplay();
    LCD_setRotation(1);
    GFX_createFramebuf();

    static int pot_values[GRAPH_WIDTH - 50] = {0}; 
    static int pot_index = 0;                      
    int pot_value;

    while (true) {
        if (xQueueReceive(xQueuePot, &pot_value, portMAX_DELAY) == pdPASS) {
            pot_values[pot_index] = pot_value;
            pot_index = (pot_index + 1) % (GRAPH_WIDTH - 50); 

            GFX_clearScreen();
            draw_axes();

            display_title(pot_value);

            int y_current, y_previous;
            int index;

            for (int x = 0; x < GRAPH_WIDTH - 50; x++) {
                int x_screen = GRAPH_WIDTH - 21 - x; 
                index = (pot_index - x - 1 + (GRAPH_WIDTH - 50)) % (GRAPH_WIDTH - 50);
                int value = pot_values[index];
                y_current = GRAPH_HEIGHT - 31 - (value * (GRAPH_HEIGHT - 50) / MAX_POT_VALUE);

                if (x > 0) {
                    GFX_drawLine(x_screen + 1, y_previous, x_screen, y_current, COLOR_GRAPH);
                }

                y_previous = y_current;
            }

            GFX_flush();
        }
    }
}


int main() {
    stdio_init_all();

    adc_init();
    adc_gpio_init(26);

    xQueuePot = xQueueCreate(1, sizeof(int));

    xTaskCreate(display_task, "DisplayTask", 1024, NULL, 1, NULL);
    xTaskCreate(potenciometro_task, "potenciometroTask", 1024, NULL, 1, NULL);
    vTaskStartScheduler();
   while(1){
   };
}

