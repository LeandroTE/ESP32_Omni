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
#include "display.h"
#include "esp_log.h"

/***************************************************************************************************
* COSNTANTS
***************************************************************************************************/

/***************************************************************************************************
* MACROS
***************************************************************************************************/
#define FIRST_LINE 5
/***************************************************************************************************
* TYPES
***************************************************************************************************/

/***************************************************************************************************
* GLOBALS VARIABLES
***************************************************************************************************/
// ==== Variables ====
static char tmp_buff[64];
float pwm_duty[4]={0.0, 0.0, 0.0, 0.0};

// ==== Task Handle ====
TaskHandle_t gpio_taskHandle =NULL;
// ==== Queu Handle ====
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
	static uint32_t button1LastTimePressed =0;									// Debounce counter button 1
	static uint32_t button2LastTimePressed =0;									// Debounce counter button 2
    uint32_t io_num;
    while(1) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
			if(io_num == BUTTON1 && gpio_get_level(io_num)==1){					// Check if GPIO0 was pressed
				if(xTaskGetTickCount()-button1LastTimePressed>100){				// Simple debounce cnt using RTOS Ticks
					button1LastTimePressed = xTaskGetTickCount();
					printf("Button 1 pressed.\n");
					pwm_duty[0]+=10.0;
					if(pwm_duty[0] >100){
						pwm_duty[0]=100;
					}
					set_PWM_duty(pwm_duty[0], 0);
					sprintf(tmp_buff, "PWM 1: %3.1f %%", (float)pwm_duty[0]);
					TFT_fillRect(0, 0, tft_width - 1, TFT_getfontheight() + 4, tft_bg);
					TFT_print(tmp_buff, 0, FIRST_LINE);
				}
			}else if(io_num == BUTTON2 && gpio_get_level(io_num)==1){			// Check if GPIO35 was pressed
				if(xTaskGetTickCount()-button2LastTimePressed>100){				// Simple debounce cnt using RTOS Ticks
					button2LastTimePressed = xTaskGetTickCount();
					printf("Button 2 pressed.\n");
					//Button2 code
				}

			}
        }
    }
}

static void tx_task(void *arg)
{
    static const char *TX_TASK_TAG = "TX_TASK";
    esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
    while (1) {
        sendData(TX_TASK_TAG, "Hello world");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

static void rx_task(void *arg)
{
    static const char *RX_TASK_TAG = "RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t* data = (uint8_t*) malloc(RX_BUF_SIZE+1);
    while (1) {
        const int rxBytes = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 1000 / portTICK_RATE_MS);
        if (rxBytes > 0) {
            data[rxBytes] = 0;
            ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", rxBytes, data);
            ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_INFO);
        }
    }
    free(data);
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
	uart_init();																// UART Config
	
	// ==== Queue Creation ====
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));						//create a queue to handle gpio event from isr

	// ==== Task Creation ====
    xTaskCreate(gpio_task, "gpio_task", 2048, NULL, 10, gpio_taskHandle);		//start gpio task
	xTaskCreate(rx_task, "uart_rx_task", 1024*2, NULL, configMAX_PRIORITIES, NULL);
    xTaskCreate(tx_task, "uart_tx_task", 1024*2, NULL, configMAX_PRIORITIES-1, NULL);
    
	// ==== ISR inicialization ====
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);							//install gpio isr service
    gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, 
						 (void*) GPIO_INPUT_IO_0);								//hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_IO_1, gpio_isr_handler, 
						 (void*) GPIO_INPUT_IO_1);								//hook isr handler for specific gpio pin

	// ==== Application configuration ====
	vTaskDelay(500 / portTICK_RATE_MS);
	printf("\r\n==============================\r\n");
	printf("PRIMITUS OMNI, LEANDRO 06/2020\r\n");
	printf("==============================\r\n");

	TFT_setRotation(3);
	disp_header("PRIMITUS OMNI v0.1");
	TFT_setFont(DEFAULT_FONT, NULL);
	int tempy = TFT_getfontheight() + 4;
	printf("TFT_height: %d\n",TFT_getfontheight());
	tft_fg = TFT_GREENYELLOW;
	sprintf(tmp_buff, "PWM 1: %3.1f %%", (float)pwm_duty[0]);
	TFT_print(tmp_buff, 0, FIRST_LINE);
	sprintf(tmp_buff, "PWM 2: %3.1f %%", (float)pwm_duty[1]);
	TFT_print(tmp_buff, 0, FIRST_LINE + tempy);
	sprintf(tmp_buff, "PWM 3: %3.1f %%", (float)pwm_duty[2]);
	TFT_print(tmp_buff, 0, FIRST_LINE + 2 * tempy);
	sprintf(tmp_buff, "PWM 4: %3.1f %%", (float)pwm_duty[3]);
	TFT_print(tmp_buff, 0, FIRST_LINE + 3 * tempy);
	gpio_set_level(GPIO_OUTPUT_IO_0, 0);
	gpio_set_level(GPIO_OUTPUT_IO_1, 0);
	gpio_set_level(GPIO_OUTPUT_IO_2, 0);


}

/***************************************************************************************************
* END OF FILE
***************************************************************************************************/
