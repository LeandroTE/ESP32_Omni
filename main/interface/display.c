/***********************************************************************************************************************
 * @file 	display.c
 * @brief	Library to help with the display interface
 * @author	Leandro
 * @date	20/06/2020
 * @company
 *
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/

#include "display.h"

/***********************************************************************************************************************
 * COSNTANTS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * MACROS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * TYPES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * GLOBALS VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * PUBLIC FUNCTIONS
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * @brief Create HEADER with retangule box in TFT Display
 *
 * @param info - String to be display in yhe header
 *
 * @return void
 *
 **********************************************************************************************************************/

void disp_header(char *info) {
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
    TFT_setclipwin(0, TFT_getfontheight() + 9, tft_width - 1, tft_height);
}
/***********************************************************************************************************************
 * @brief Function to update display, this function also create the visual of the display
 *
 * @param display_data - Display data, this structure is the interface for the display module
 *
 * @return void
 *
 **********************************************************************************************************************/

void update_disp(struct display_data_t *display_data) {
    static uint8_t state = DISP_NON_INIT;    // State machine for the display for initialization
    char tmp_buff[64];
    int tempy = TFT_getfontheight() + 4;

    if (state == DISP_NON_INIT) {
        TFT_setRotation(1);
        disp_header("iOmni v0.1");
        TFT_setFont(DEFAULT_FONT, NULL);
        tft_fg = TFT_GREENYELLOW;
        sprintf(tmp_buff, "PWM Lidar: %3.1f %%", (float)display_data->pwm_duty[0]);    // Update diplay
        TFT_print(tmp_buff, 0, FIRST_LINE);
        sprintf(tmp_buff, "PWM 1: %3.1f %%", (float)display_data->pwm_duty[1]);
        TFT_print(tmp_buff, 0, FIRST_LINE + tempy);
        sprintf(tmp_buff, "PWM 2: %3.1f %%", (float)display_data->pwm_duty[2]);
        TFT_print(tmp_buff, 0, FIRST_LINE + 2 * tempy);
        sprintf(tmp_buff, "PWM 3: %3.1f %%", (float)display_data->pwm_duty[3]);
        TFT_print(tmp_buff, 0, FIRST_LINE + 3 * tempy);

        // sprintf(tmp_buff, "IP:" IPSTR, IP2STR(&event->ip_info.ip));
        // tft_fg = TFT_YELLOW;
        // TFT_print(tmp_buff, 0, 77);
        state = DISP_INIT;
    } else if (state == DISP_INIT) {
        tft_fg = TFT_GREENYELLOW;

        TFT_fillRect(TFT_getStringWidth("PWM Lidar: "), FIRST_LINE, TFT_getStringWidth("XXXXX"), tempy, tft_bg);
        sprintf(tmp_buff, "%3.1f %%", (float)display_data->pwm_duty[0]);    // Update diplay
        TFT_print(tmp_buff, TFT_getStringWidth("PWM Lidar: "), FIRST_LINE);

        TFT_fillRect(TFT_getStringWidth("PWM 1: "), FIRST_LINE + 1 * tempy, TFT_getStringWidth("XXXXX"), tempy, tft_bg);
        sprintf(tmp_buff, "%3.1f %%", (float)display_data->pwm_duty[1]);
        TFT_print(tmp_buff, TFT_getStringWidth("PWM 1: "), FIRST_LINE + tempy);

        TFT_fillRect(TFT_getStringWidth("PWM 2: "), FIRST_LINE + 2 * tempy, TFT_getStringWidth("XXXXX"), tempy, tft_bg);
        sprintf(tmp_buff, "%3.1f %%", (float)display_data->pwm_duty[2]);
        TFT_print(tmp_buff, TFT_getStringWidth("PWM 2: "), FIRST_LINE + 2 * tempy);

        TFT_fillRect(TFT_getStringWidth("PWM 3: "), FIRST_LINE + 3 * tempy, TFT_getStringWidth("XXXXX"), tempy, tft_bg);
        sprintf(tmp_buff, "%3.1f %%", (float)display_data->pwm_duty[3]);
        TFT_print(tmp_buff, TFT_getStringWidth("PWM 3: "), FIRST_LINE + 3 * tempy);

        // sprintf(tmp_buff, "PWM 2: %3.1f %%", (float)display_data->pwm_duty[2]);
        // TFT_print(tmp_buff, 0, FIRST_LINE + 2 * tempy);

        // sprintf(tmp_buff, "PWM 3: %3.1f %%", (float)display_data->pwm_duty[3]);
        // TFT_print(tmp_buff, 0, FIRST_LINE + 3 * tempy);
    }
}

/***********************************************************************************************************************
 * END OF FILE
 **********************************************************************************************************************/