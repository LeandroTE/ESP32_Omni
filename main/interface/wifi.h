/***********************************************************************************************************************
 * @file 	wifi.h
 * @brief
 * @author	Leandro
 * @date	27/09/2020
 * @company
 *
 **********************************************************************************************************************/

#ifndef WIFI_H_
#define WIFI_H_

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/***********************************************************************************************************************
 * COSNTANTS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * MACROS
 **********************************************************************************************************************/
#define EXAMPLE_ESP_WIFI_SSID "God Save The Queen"    // Wifi SSID to connect
#define EXAMPLE_ESP_WIFI_PASS "peg@ladrao"            // Wifi password
#define EXAMPLE_ESP_MAXIMUM_RETRY 5                   // Max number of retries

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

/***********************************************************************************************************************
 * TYPES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * GLOBALS VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * PUBLIC FUNCTIONS
 **********************************************************************************************************************/

// void wifi_init_sta(void);

/***********************************************************************************************************************
 * END OF FILE
 **********************************************************************************************************************/
#endif /* WIFI_H_ */