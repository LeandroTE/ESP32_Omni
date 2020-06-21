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


static void disp_header(char *info){
	TFT_fillScreen(TFT_BLACK);
	TFT_resetclipwin();

	tft_fg = TFT_YELLOW;
	tft_bg = (color_t){64, 64, 64};

	if (tft_width < 240)
		TFT_setFont(DEF_SMALL_FONT, NULL);
	else
		TFT_setFont(DEFAULT_FONT, NULL);
	TFT_fillRect(0, 0, tft_width - 1, TFT_getfontheight() + 8, tft_bg);
	TFT_drawRect(0, 0, tft_width - 1, TFT_getfontheight() + 8, TFT_CYAN);

	TFT_print(info, CENTER, 4);

	tft_bg = TFT_BLACK;
	TFT_setclipwin(0, TFT_getfontheight() + 9, tft_width - 1, tft_height - TFT_getfontheight() - 10);
}

void app_main(){
	
	// ==== System Initialization ====
	tft_init();										// TFT Init
	pwm_init();										// PWM Init

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
