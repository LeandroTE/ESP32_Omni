/***************************************************************************************************
 ===  Primitus Omni Project====
 * @file 	main.c
 * @brief	System functions
 * @author	Leandro
 * @date	20/06/2020
 * @company	
 *
 **************************************************************************************************/

/***************************************************************************************************
* INCLUDES
***************************************************************************************************/

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"

#include "freertos/task.h"
#include "system.h"
#include "tftspi.h"
#include "tft.h"
#include "spiffs_vfs.h"

#include "driver/ledc.h"
#include "driver/gpio.h"

/***************************************************************************************************
* COSNTANTS
***************************************************************************************************/

/***************************************************************************************************
* MACROS
***************************************************************************************************/

/***************************************************************************************************
* TYPES
***************************************************************************************************/

/***************************************************************************************************
* GLOBALS VARIABLES
***************************************************************************************************/

static char tmp_buff[64];

/***************************************************************************************************
* LOCALS FUNCTIONS
***************************************************************************************************/

static xQueueHandle gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void gpio_task_example(void* arg)
{
    uint32_t io_num;
    for(;;) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            printf("GPIO[%d] intr, val: %d\n", io_num, gpio_get_level(io_num));
        }
    }
}


void app_main(){

	// ==== System Initialization ====
	tft_init();										// TFT Init
	pwm_init();										// PWM Init

	// ==== GPIO Init ====
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    //interrupt of rising edge
    io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
    //bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    //set as input mode    
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    //change gpio intrrupt type for one pin
    gpio_set_intr_type(GPIO_INPUT_IO_0, GPIO_INTR_ANYEDGE);

    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    //start gpio task
    xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_IO_1, gpio_isr_handler, (void*) GPIO_INPUT_IO_1);

    //remove isr handler for gpio number.
    gpio_isr_handler_remove(GPIO_INPUT_IO_0);
    //hook isr handler for specific gpio pin again
    gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);


	vTaskDelay(500 / portTICK_RATE_MS);
	printf("\r\n==============================\r\n");
	printf("PRIMITUS OMNI, LEANDRO 06/2020\r\n");
	printf("==============================\r\n");
	printf("Pins used: miso=%d, mosi=%d, sck=%d, cs=%d\r\n", PIN_NUM_MISO, PIN_NUM_MOSI, PIN_NUM_CLK, PIN_NUM_CS);
	printf("==============================\r\n\r\n");

	TFT_setRotation(1);
	disp_header("PRIMITUS OMNI v0.1");
	TFT_setFont(DEFAULT_FONT, NULL);
	int tempy = TFT_getfontheight() + 4;
	tft_fg = TFT_ORANGE;
	TFT_print("ESP32", 0, LASTY + tempy);
	tft_fg = TFT_CYAN;
	TFT_print("TFT Demo", 0, LASTY + tempy);
	tft_fg = TFT_GREEN;
	sprintf(tmp_buff, "Read speed: %5.2f MHz", (float)tft_max_rdclock / 1000000.0);
	TFT_print(tmp_buff, 0, LASTY + tempy);

}

/***************************************************************************************************
* END OF FILE
***************************************************************************************************/
