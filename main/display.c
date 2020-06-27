/***************************************************************************************************
 * @file 	display.c
 * @brief	Library to help with the display interface
 * @author	Leandro
 * @date	20/06/2020
 * @company	
 *
 **************************************************************************************************/

/***************************************************************************************************
* INCLUDES
***************************************************************************************************/

#include "display.h"

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

/***************************************************************************************************
* PUBLIC FUNCTIONS
***************************************************************************************************/
/***************************************************************************************************
 * @brief Create HEADER with retangule box in TFT Display
 *
 * @param info - String to be display in yhe header
 *
 * @return void
 * 
 * ************************************************************************************************/

void disp_header(char *info){
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


/***************************************************************************************************
* END OF FILE
***************************************************************************************************/