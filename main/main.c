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


#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_system.h"
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "spiffs_vfs.h"
#include "system.h"
#include "wifi.h"
#include "tft.h"
#include "tftspi.h"
#include "RPLidar.h"
#include "display.h"

#include "lwip/err.h"
#include "lwip/sys.h"

/***********************************************************************************************************************
 * COSNTANTS
 **********************************************************************************************************************/

static const char *TAG = "wifi station";

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
// ==== Event Handle ====
static EventGroupHandle_t s_wifi_event_group;


static int s_retry_num = 0;

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

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}


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

/***********************************************************************************************************************
 * LOCALS FUNCTIONS
 **********************************************************************************************************************/

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    /* The event will not be processed after unregister */
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);
}






void app_main() {
    gpio_config_t io_conf;    // Declare GPIO config structure

    // ==== System Initialization ====
    tft_init();            // TFT Init
    pwm_init();            // PWM Init
    GPIO_Init(io_conf);    // GPIO Init
    uart_init();           // UART Config
        
    esp_err_t ret = nvs_flash_init(); //Initialize NVS
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
                &lidar_taskHandle);    // Create Lidar task

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
    printf("PRIMITUS OMNI, LEANDRO 06/2020\r\n");
    printf("==============================\r\n");

    // ==== Wifi Inicialization ====
    wifi_init_sta();    // Start wifi
    printf("\r\n==============================\r\n");
    printf("Wifi sequence start\r\n");
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

    pwm_duty[3] = 80.0;    // Set channel 3 (PWM Lidar) to 70% duty cycle
    set_PWM_duty(pwm_duty[3], 3);
    sprintf(tmp_buff, "PWM 1: %3.1f %%", (float)pwm_duty[3]);    // Update diplay
    TFT_fillRect(0, 0, tft_width - 1, TFT_getfontheight() + 4, tft_bg);
    TFT_print(tmp_buff, 0, FIRST_LINE);
}

/***********************************************************************************************************************
 * END OF FILE
 **********************************************************************************************************************/
