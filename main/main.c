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
static xQueueHandle gpio_evt_queue = NULL;

/***************************************************************************************************
* ISR'S
***************************************************************************************************/

static void IRAM_ATTR gpio_isr_handler(void* arg){

    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

/***************************************************************************************************
* TASKS
***************************************************************************************************/

static void gpio_task(void* arg){

    uint32_t io_num;
    while(1) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            printf("GPIO[%d] intr, val: %d\n", io_num, gpio_get_level(io_num));
        }
    }
}

/***************************************************************************************************
* LOCALS FUNCTIONS
***************************************************************************************************/

void app_main(){
	gpio_config_t io_conf;														// Declare GPIO config structure


	// ==== System Initialization ====
	tft_init();																	// TFT Init
	pwm_init();																	// PWM Init
    GPIO_Init(io_conf);															// GPIO Init
	
	// ==== Task Creation ====
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));						//create a queue to handle gpio event from isr
    xTaskCreate(gpio_task, "gpio_task", 2048, NULL, 10, NULL);					//start gpio task

    
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);							//install gpio isr service
    gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, 
						 (void*) GPIO_INPUT_IO_0);								//hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_IO_1, gpio_isr_handler, 
						 (void*) GPIO_INPUT_IO_1);								//hook isr handler for specific gpio pin


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
