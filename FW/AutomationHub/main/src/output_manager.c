/* GPIO Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "inc/output_manager.h"

#define OUTPUT_TASK_STACK_SIZE    (2048)
#define OUTPUT_TASK_PRIORITY      (10)

#define COMMON_ENABLE       12
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<OUT_1) | (1ULL<<OUT_2) | (1ULL<<OUT_3) | (1ULL<<OUT_4) | (1ULL<<COMMON_ENABLE))
/*
 * Let's say, GPIO_OUTPUT_IO_0=18, GPIO_OUTPUT_IO_1=19
 * In binary representation,
 * 1ULL<<GPIO_OUTPUT_IO_0 is equal to 0000000000000000000001000000000000000000 and
 * 1ULL<<GPIO_OUTPUT_IO_1 is equal to 0000000000000000000010000000000000000000
 * GPIO_OUTPUT_PIN_SEL                0000000000000000000011000000000000000000
 * */

static QueueHandle_t output_evt_queue = NULL;

void init_gpio(void);

void output_manager_task(void* arg)
{
    output_command_t command;
    init_gpio();
    for(;;) {
        if(xQueueReceive(output_evt_queue, &command, portMAX_DELAY)) {
            // printf("GPIO[%d]\n", command.ioNumber);
            // printf("State %d\n", command.state);
            if (command.ioNumber == OUT_1) {
                gpio_set_level(OUT_1, command.state);
            } else if (command.ioNumber == OUT_2) {
                gpio_set_level(OUT_2, command.state);
            } else if (command.ioNumber == OUT_3) {
                gpio_set_level(OUT_3, command.state);
            } else if (command.ioNumber == OUT_4) {
                gpio_set_level(OUT_4, command.state);
            } else if (command.ioNumber == COMMON_ENABLE) {
                gpio_set_level(COMMON_ENABLE, command.state);
            }
        }
    }
}

// Latches output to the specified state
bool set_output(uint32_t ioNumber, uint8_t state)
{
    if(output_evt_queue == NULL) {
        printf("Output event queue not initialized.\n");
        return false;
    }
    output_command_t command;
    command.ioNumber = ioNumber;
    command.mode = latch;
    command.time = 0;
    command.state = state;
    // printf("Output command %d, %d.\n",command.ioNumber, command.state);
    return xQueueSend(output_evt_queue, &command, 0);
}

void init_gpio(void)
{
    //zero-initialize the config structure.
    gpio_config_t io_conf = {};
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

    //create a queue to handle gpio event from isr
    output_evt_queue = xQueueCreate(10, sizeof(output_command_t));

    // Default common to enabled, this puts 12v on the Common terminal.
    gpio_set_level(COMMON_ENABLE, 1);
    printf("Minimum free heap size: %"PRIu32" bytes\n", esp_get_minimum_free_heap_size());
    printf("GPIO initialized.\n");
}

void init_output_task(void)
{
    xTaskCreate(output_manager_task, "output_manager_task", OUTPUT_TASK_STACK_SIZE, NULL, OUTPUT_TASK_PRIORITY, NULL);
}