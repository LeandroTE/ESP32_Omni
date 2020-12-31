/***********************************************************************************************************************
 ===  iOmni Project====
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
#include "Globals.h"

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "RPLidar.h"
#include "display.h"
#include "system.h"
#include "tft.h"
#include "tftspi.h"
#include "wifi.h"

#include "lwip/apps/netbiosns.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "esp_netif.h"
#include "esp_spiffs.h"
#include "esp_vfs_fat.h"
#include "sdkconfig.h"

/***********************************************************************************************************************
 * COSNTANTS
 **********************************************************************************************************************/
static const char *TAG = "MAIN";

/***********************************************************************************************************************
 * MACROS
 **********************************************************************************************************************/
#define RX_BUFFER_SZ 1000
#define FIRST_LINE 5

// ==== TASK PRIORITIES ====
#define GPIO_TASK_PRIORITY 7
#define RX_TASK_PRIORITY 6
#define LIDAR_TASK_PRIORITY 5
#define DISPLAY_TASK_PRIORITY 4

// ==== HTTP Server =====
#define MDNS_INSTANCE "esp home web server"

/***********************************************************************************************************************
 * TYPES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * GLOBALS VARIABLES
 **********************************************************************************************************************/
// ==== Variables ====
static char tmp_buff[64];
motor_data_t motor_data;
float pwm_duty[4] = {0.0, 0.0, 0.0, 0.0};

struct lidarStateMachine lidarStateMachine;
display_data_t displayData;

// ==== Task Handle ====
TaskHandle_t gpio_taskHandle = NULL;
TaskHandle_t lidar_taskHandle = NULL;

// ==== Queue Handle ====
static xQueueHandle gpio_evt_queue = NULL;
static xQueueHandle rx_buffer_queue = NULL;

esp_err_t start_rest_server(const char *base_path);

/***********************************************************************************************************************
 * ISR'S
 **********************************************************************************************************************/

static void IRAM_ATTR gpio_isr_handler(void *arg) {
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

/***********************************************************************************************************************
 * EVENT HANDLE´s
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * TASKS
 **********************************************************************************************************************/

static void gpio_task(void *arg) {
    static uint32_t button1LastTimePressed = 0;    // Debounce counter button 1
    static uint32_t button2LastTimePressed = 0;    // Debounce counter button 2
    static uint8_t button2state = 0;
    uint32_t io_num;
    while (1) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            if (io_num == BUTTON1 && gpio_get_level(io_num) == 1) {          // Check if GPIO0 was pressed
                if (xTaskGetTickCount() - button1LastTimePressed > 100) {    // Simple debounce cnt using RTOS ticks
                    button1LastTimePressed = xTaskGetTickCount();
                    printf("Button 1 pressed.\n");
                    // if (pwm_duty[0] == 90.0) {
                    //    pwm_duty[0] = 0.0;        // Set channel 0 (PWM Lidar) to 0% duty cycle
                    //} else {
                    //    pwm_duty[0] = 90.0;        // Set channel 0 (PWM Lidar) to 70% duty cycle
                    //}
                    sendRequest(RPLIDAR_CMD_RESET, NULL, 0, &lidarStateMachine);
                    set_PWM_duty(pwm_duty[0], 0);
                    sprintf(tmp_buff, "PWM 1: %3.1f %%", (float)pwm_duty[0]);    // Update diplay
                    TFT_fillRect(0, 0, tft_width - 1, TFT_getfontheight() + 4, tft_bg);
                    TFT_print(tmp_buff, 0, FIRST_LINE);
                }
            } else if (io_num == BUTTON2 && gpio_get_level(io_num) == 1) {    // Check if GPIO35 was pressed
                if (xTaskGetTickCount() - button2LastTimePressed > 100) {     // Simple debounce cnt using RTOS Ticks
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
                xQueueSend(rx_buffer_queue, &data[i], (TickType_t)0);    // Send bytes to queue buffer
            }
            printf("\r\n==============================\r\n");
            ESP_LOGI(RX_TASK_TAG, "Read %d bytes", rxBytes);
            ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_INFO);
            printf("\r\n==============================\r\n");
            if (rxBytes > 200) {
                sendRequest(RPLIDAR_CMD_STOP, NULL, 0, &lidarStateMachine);
                printf("Stop command sent\n");
            }

            vTaskResume(lidar_taskHandle);    // Wake Lidar task to treat rx buffer
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
        while (uxQueueMessagesWaiting(rx_buffer_queue) != 0) {                    // Read all bytes in Queue
            xQueueReceive(rx_buffer_queue, (void *)bufferTemp, (TickType_t)5);    // Read one byte from queue
            lidarSendByteToStateMachine(bufferTemp[0], &lidarStateMachine);       // Send to lidar state machine
            // printf("Messages waiting: %d\n", uxQueueMessagesWaiting(rx_buffer_queue));
        }
    }
}

static void display_task(void *arg) {
    while(1){
        printf("Display Task\r\n");
        vTaskDelay(500 / portTICK_RATE_MS);
    }
}

/***********************************************************************************************************************
 * LOCALS FUNCTIONS
 **********************************************************************************************************************/

void app_main() {
    gpio_config_t io_conf;    // Declare GPIO config structure

    /* ==== System Initialization ====*/
    tft_init();            // TFT Init
    pwm_init();            // PWM Init
    GPIO_Init(io_conf);    // GPIO Init
    uart_init();           // UART Config

    esp_err_t ret = nvs_flash_init();    // Initialize NVS
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    // ==== Queue Creation ====
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));              // create a queue to handle gpio event from isr
    rx_buffer_queue = xQueueCreate(RX_BUFFER_SZ, sizeof(uint8_t));    // Create uart rx buffer

    // ==== Task Creation ====
    xTaskCreate(gpio_task, "gpio_task", 1024 * 2, NULL, GPIO_TASK_PRIORITY,
                &gpio_taskHandle);                                                   // Create gpio task
    xTaskCreate(rx_task, "uart_rx_task", 1024 * 2, NULL, RX_TASK_PRIORITY, NULL);    // Create RX Task
    xTaskCreate(lidar_task, "lidar_task", 1024 * 2, NULL, LIDAR_TASK_PRIORITY,
                &lidar_taskHandle);                                                            // Create Lidar task
    xTaskCreate(display_task, "display_task", 1024 * 2, NULL, DISPLAY_TASK_PRIORITY, NULL);    // Create Display Task

    // ==== ISR inicialization ====
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);    // install gpio isr service
    gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler,
                         (void *)GPIO_INPUT_IO_0);    // hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_IO_1, gpio_isr_handler,
                         (void *)GPIO_INPUT_IO_1);    // hook isr handler for specific gpio pin

    // ==== Application configuration ====
    lidarBeginStateMachine(&lidarStateMachine);    // Initialize Lidar state machinne

    vTaskDelay(500 / portTICK_RATE_MS);
    printf("\r\n==============================\r\n");
    printf("iOmni, LEANDRO 06/2020\r\n");
    printf("==============================\r\n\n");

    gpio_set_level(GPIO_OUTPUT_IO_0, 0);
    gpio_set_level(GPIO_OUTPUT_IO_1, 0);
    gpio_set_level(GPIO_OUTPUT_IO_2, 0);

    pwm_duty[3] = 80.0;    // Set channel 3 (PWM Lidar) to 80% duty cycle
    set_PWM_duty(pwm_duty[3], 3);
    displayData.pwm_duty[0] = pwm_duty[0];
    displayData.pwm_duty[1] = pwm_duty[1];
    displayData.pwm_duty[2] = pwm_duty[2];
    displayData.pwm_duty[3] = pwm_duty[3];
    update_disp(&displayData);

    // ==== Wifi Inicialization ====
    printf("\r\n==============================\r\n");
    printf("Wifi sequence start\r\n");
    printf("==============================\r\n\n");

    // initialise_mdns();
    netbiosns_init();
    netbiosns_set_name(CONFIG_EXAMPLE_MDNS_HOST_NAME);
    wifi_init_sta();    // Start wifi
    ESP_ERROR_CHECK(init_fs());
    ESP_ERROR_CHECK(start_rest_server(CONFIG_EXAMPLE_WEB_MOUNT_POINT));
}

/***********************************************************************************************************************
 * END OF FILE
 **********************************************************************************************************************/
