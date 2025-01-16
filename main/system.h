/***********************************************************************************************************************
 * @file 	System.h
 * @brief	System functions
 * @author	Leandro
 * @date	20/06/2020
 * @company
 *
 **********************************************************************************************************************/

#ifndef SYSYEM_H_
#define SYSYEM_H_

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tft.h"
#include "tftspi.h"

#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_vfs_fat.h"

/***********************************************************************************************************************
 * COSNTANTS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * MACROS
 **********************************************************************************************************************/

// ================== TFT ==================
#define SPI_BUS TFT_HSPI_HOST    // Define which spi bus to use TFT_VSPI_HOST or TFT_HSPI_HOST

// ================== PWM ==================

#define LEDC_HS_TIMER LEDC_TIMER_0
#define LEDC_HS_MODE LEDC_HIGH_SPEED_MODE
#define LEDC_HS_CH0_GPIO (2)
#define LEDC_HS_CH0_CHANNEL LEDC_CHANNEL_0
#define LEDC_HS_CH1_GPIO (15)
#define LEDC_HS_CH1_CHANNEL LEDC_CHANNEL_1

#define LEDC_LS_TIMER LEDC_TIMER_1
#define LEDC_LS_MODE LEDC_LOW_SPEED_MODE

#define LEDC_LS_CH2_GPIO (13)
#define LEDC_LS_CH2_CHANNEL LEDC_CHANNEL_2
#define LEDC_LS_CH3_GPIO (12)
#define LEDC_LS_CH3_CHANNEL LEDC_CHANNEL_3

#define LEDC_TEST_CH_NUM (4)
#define MAX_DUTY_CYCLE (8192)

// ================== GPIO ==================
#define GPIO_INPUT_IO_0 35    // Button 1
#define GPIO_INPUT_IO_1 0     // Button2
#define GPIO_INPUT_PIN_SEL ((1ULL << GPIO_INPUT_IO_0) | (1ULL << GPIO_INPUT_IO_1))
#define ESP_INTR_FLAG_DEFAULT 0

#define GPIO_OUTPUT_IO_0 25
#define GPIO_OUTPUT_IO_1 26
#define GPIO_OUTPUT_IO_2 27
#define GPIO_OUTPUT_IO_3 17
#define GPIO_OUTPUT_PIN_SEL                                                                                            \
    ((1ULL << GPIO_OUTPUT_IO_0) | (1ULL << GPIO_OUTPUT_IO_1) | (1ULL << GPIO_OUTPUT_IO_2) | (1ULL << GPIO_OUTPUT_IO_3))

#define BUTTON1 0
#define BUTTON2 35

// ================== UART ==================
static const int RX_BUF_SIZE = 500;

#define TXD_PIN (GPIO_NUM_22)
#define RXD_PIN (GPIO_NUM_21)

/***********************************************************************************************************************
 * TYPES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * GLOBALS VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * PUBLIC FUNCTIONS
 **********************************************************************************************************************/

// ================== TFT ==================
void tft_init();

// ================== PWM ==================
void pwm_init();
void set_PWM_duty(float duty, int channel);

// ================== GPIO ==================
void GPIO_Init(gpio_config_t io_conf);

// ================== UART ==================
void uart_init();
int sendData(const char *data);

// =============== FILE SYSTEM ==============
esp_err_t init_fs(void);

/***********************************************************************************************************************
 * END OF FILE
 **********************************************************************************************************************/
#endif /* SYSYEM_H_ */
