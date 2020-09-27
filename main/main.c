/***********************************************************************************************************************
 ===  Primitus Omni Project====
 * @file 	main.c
 * @brief	System functions
 * @author	Leandro
 * @date	20/06/2020
 * @company
 *
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "RPLidar.h"
#include "display.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "spiffs_vfs.h"
#include "system.h"
#include "tft.h"
#include "tftspi.h"

/***********************************************************************************************************************
 * COSNTANTS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * MACROS
 **********************************************************************************************************************/
#define FIRST_LINE 5
#define RX_BUFFER_SZ 1000

// ==== TASK PRIORITIES ====
#define GPIO_TASK_PRIORITY 7
#define RX_TASK_PRIORITY 6
#define LIDAR_TASK_PRIORITY 5
/***********************************************************************************************************************
 * TYPES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * GLOBALS VARIABLES
 **********************************************************************************************************************/
// ==== Variables ====
static char tmp_buff[64];
float pwm_duty[4] = {0.0, 0.0, 0.0, 0.0};

struct lidarStateMachine lidarStateMachine;

// ==== Task Handle ====
TaskHandle_t gpio_taskHandle = NULL;
TaskHandle_t lidar_taskHandle = NULL;

// ==== Queue Handle ====
static xQueueHandle gpio_evt_queue = NULL;
static xQueueHandle rx_buffer_queue = NULL;

/***********************************************************************************************************************
 * ISR'S
 **********************************************************************************************************************/

static void IRAM_ATTR gpio_isr_handler(void *arg) {
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

/***********************************************************************************************************************
 * TASKS
 **********************************************************************************************************************/

static void gpio_task(void *arg) {
    static uint32_t button1LastTimePressed = 0;        // Debounce counter button 1
    static uint32_t button2LastTimePressed = 0;        // Debounce counter button 2
    static uint8_t button2state = 0;
    uint32_t io_num;
    while (1) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            if (io_num == BUTTON1 && gpio_get_level(io_num) == 1) {              // Check if GPIO0 was pressed
                if (xTaskGetTickCount() - button1LastTimePressed > 100) {        // Simple debounce cnt using RTOS ticks
                    button1LastTimePressed = xTaskGetTickCount();
                    printf("Button 1 pressed.\n");
                    // if (pwm_duty[0] == 90.0) {
                    //    pwm_duty[0] = 0.0;        // Set channel 0 (PWM Lidar) to 0% duty cycle
                    //} else {
                    //    pwm_duty[0] = 90.0;        // Set channel 0 (PWM Lidar) to 70% duty cycle
                    //}
                    sendRequest(RPLIDAR_CMD_RESET, NULL, 0, &lidarStateMachine);
                    set_PWM_duty(pwm_duty[0], 0);
                    sprintf(tmp_buff, "PWM 1: %3.1f %%", (float)pwm_duty[0]);        // Update diplay
                    TFT_fillRect(0, 0, tft_width - 1, TFT_getfontheight() + 4, tft_bg);
                    TFT_print(tmp_buff, 0, FIRST_LINE);
                }
            } else if (io_num == BUTTON2 && gpio_get_level(io_num) == 1) {        // Check if GPIO35 was pressed
                if (xTaskGetTickCount() - button2LastTimePressed > 100) {        // Simple debounce cnt using RTOS Ticks
                    button2LastTimePressed = xTaskGetTickCount();
                    printf("Button 2 pressed.\n");
                    if (button2state == 0) {
                        sendRequest(RPLIDAR_CMD_SCAN, NULL, 0, &lidarStateMachine);
                    }
                }
            }
        }
    }
}

static void rx_task(void *arg) {
    static const char *RX_TASK_TAG = "RX_TASK";
    int i = 0;
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t *data = (uint8_t *)malloc(RX_BUF_SIZE + 1);
    while (1) {
        const int rxBytes = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 50 / portTICK_RATE_MS);

        if (rxBytes > 0) {
            for (i = 0; i < rxBytes; i++) {
                xQueueSend(rx_buffer_queue, &data[i], (TickType_t)0);        // Send bytes to queue buffer
            }
            printf("\r\n==============================\r\n");
            ESP_LOGI(RX_TASK_TAG, "Read %d bytes", rxBytes);
            ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_INFO);
            printf("\r\n==============================\r\n");
            if (rxBytes > 200) {
                sendRequest(RPLIDAR_CMD_STOP, NULL, 0, &lidarStateMachine);
                printf("Stop command sent\n");
            }

            vTaskResume(lidar_taskHandle);        // Wake Lidar task to treat rx buffer
        }
    }
    free(data);
}

static void lidar_task(void *arg) {
    char bufferTemp[1];
    while (1) {
        vTaskSuspend(lidar_taskHandle);
        // printf("Lidar Processing Task\n");
        // printf("Messages waiting: %d\n", uxQueueMessagesWaiting(rx_buffer_queue));
        while (uxQueueMessagesWaiting(rx_buffer_queue) != 0) {                        // Read all bytes in Queue
            xQueueReceive(rx_buffer_queue, (void *)bufferTemp, (TickType_t)5);        // Read one byte from queue
            lidarSendByteToStateMachine(bufferTemp[0], &lidarStateMachine);           // Send to lidar state machine
            // printf("Messages waiting: %d\n", uxQueueMessagesWaiting(rx_buffer_queue));
        }
    }
}

/***********************************************************************************************************************
 * LOCALS FUNCTIONS
 **********************************************************************************************************************/

void app_main() {
    gpio_config_t io_conf;        // Declare GPIO config structure

    // ==== System Initialization ====
    tft_init();                // TFT Init
    pwm_init();                // PWM Init
    GPIO_Init(io_conf);        // GPIO Init
    uart_init();               // UART Config

    // ==== Queue Creation ====
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));        // create a queue to handle gpio event from isr
    rx_buffer_queue = xQueueCreate(RX_BUFFER_SZ, sizeof(uint8_t));        // Create uart rx buffer

    // ==== Task Creation ====
    xTaskCreate(gpio_task, "gpio_task", 1024 * 2, NULL, GPIO_TASK_PRIORITY,
                &gpio_taskHandle);                                                       // Create gpio task
    xTaskCreate(rx_task, "uart_rx_task", 1024 * 2, NULL, RX_TASK_PRIORITY, NULL);        // Create RX Task
    xTaskCreate(lidar_task, "lidar_task", 1024 * 2, NULL, LIDAR_TASK_PRIORITY,
                &lidar_taskHandle);        // Create Lidar task

    // ==== ISR inicialization ====
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);        // install gpio isr service
    gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler,
                         (void *)GPIO_INPUT_IO_0);        // hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_IO_1, gpio_isr_handler,
                         (void *)GPIO_INPUT_IO_1);        // hook isr handler for specific gpio pin

    // ==== Application configuration ====
    lidarBeginStateMachine(&lidarStateMachine);        // Initialize Lidar state machinne

    vTaskDelay(500 / portTICK_RATE_MS);
    printf("\r\n==============================\r\n");
    printf("PRIMITUS OMNI, LEANDRO 06/2020\r\n");
    printf("==============================\r\n");

    TFT_setRotation(1);
    disp_header("PRIMITUS OMNI v0.1");
    TFT_setFont(DEFAULT_FONT, NULL);
    int tempy = TFT_getfontheight() + 4;
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

    pwm_duty[3] = 80.0;        // Set channel 3 (PWM Lidar) to 70% duty cycle
    set_PWM_duty(pwm_duty[3], 3);
    sprintf(tmp_buff, "PWM 1: %3.1f %%", (float)pwm_duty[3]);        // Update diplay
    TFT_fillRect(0, 0, tft_width - 1, TFT_getfontheight() + 4, tft_bg);
    TFT_print(tmp_buff, 0, FIRST_LINE);
}

/***********************************************************************************************************************
 * END OF FILE
 **********************************************************************************************************************/
