/***************************************************************************************************
 * @file 	System.c
 * @brief	System functions
 * @author	Leandro
 * @date	20/06/2020
 * @company	
 *
 **************************************************************************************************/

/***************************************************************************************************
* INCLUDES
***************************************************************************************************/

#include "system.h"

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
 * @brief Responsible for the initialization of the TFT Display
 *
 * @param 
 *
 * @return void
 * 
 * ************************************************************************************************/
void tft_init()
{

    esp_err_t ret;

    tft_max_rdclock = 8000000;                      //Set maximum spi clock for display read

    TFT_PinsInit();                                 // Pins MUST be initialized before SPI
                                                    //interface initialization

    spi_lobo_device_handle_t spi;

    spi_lobo_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,                // set SPI MISO pin
        .mosi_io_num = PIN_NUM_MOSI,                // set SPI MOSI pin
        .sclk_io_num = PIN_NUM_CLK,                 // set SPI CLK pin
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 6 * 1024,
    };
    spi_lobo_device_interface_config_t devcfg = {
        .clock_speed_hz = 8000000,                  // Initial clock out at 8 MHz
        .mode = 0,                                  // SPI mode 0
        .spics_io_num = -1,                         // we will use external CS pin
        .spics_ext_io_num = PIN_NUM_CS,             // external CS pin
        .flags = LB_SPI_DEVICE_HALFDUPLEX,          // ALWAYS SET to HALF DUPLEX MODE for  spi
    };

    // ==== Initialize the SPI bus and attach the LCD to the SPI bus ====
    ret = spi_lobo_bus_add_device(SPI_BUS, &buscfg, &devcfg, &spi);
    assert(ret == ESP_OK);
    printf("SPI: display device added to spi bus (%d)\r\n", SPI_BUS);
    tft_disp_spi = spi;

    // ==== Test select/deselect ====
    ret = spi_lobo_device_select(spi, 1);
    assert(ret == ESP_OK);
    ret = spi_lobo_device_deselect(spi);
    assert(ret == ESP_OK);

    printf("SPI: attached display device, speed=%u\r\n", spi_lobo_get_speed(spi));
    printf("SPI: bus uses native pins: %s\r\n", spi_lobo_uses_native_pins(spi) ? "true" : "false");

    // ==== Initialize the Display ====
    printf("SPI: display init...\r\n");
    TFT_display_init();
    TFT_invertDisplay(1);
    printf("OK\r\n");

    // ==== Detect maximum read speed ====
    tft_max_rdclock = find_rd_speed();
    printf("SPI: Max rd speed = %u\r\n", tft_max_rdclock);

    // ==== Set SPI clock used for display operations ====
    spi_lobo_set_speed(spi, DEFAULT_SPI_CLOCK);
    printf("SPI: Changed speed to %u\r\n", spi_lobo_get_speed(spi));

    printf("\r\n---------------------\r\n");
    printf("Graphics demo started\r\n");
    printf("---------------------\r\n");

    tft_font_rotate = 0;
    tft_text_wrap = 0;
    tft_font_transparent = 0;
    tft_font_forceFixed = 0;
    tft_gray_scale = 0;
    TFT_setGammaCurve(DEFAULT_GAMMA_CURVE);
    TFT_setRotation(PORTRAIT);
    TFT_setFont(DEFAULT_FONT, NULL);
    TFT_resetclipwin();
}

/***************************************************************************************************
 * @brief Responsible for the initialization of the TFT Display
 *
 * @param 
 *
 * @return void
 * 
 *  
 * 1. Start with initializing LEDC module:
 *    a. Set the timer of LEDC first, this determines the frequency and resolution of PWM.
 *    b. Then set the LEDC channel you want to use, and bind with one of the timers.
 *
 * 2. You need first to install a default fade function,
 *    then you can use fade APIs.
  *************************************************************************************************/
void pwm_init()
{
    int ch;


    // ==== Prepare and set configuration of timers for LED Controller ====
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_13_BIT,       // resolution of PWM duty
        .freq_hz = 1000,                            // frequency of PWM signal
        .speed_mode = LEDC_LS_MODE,                 // timer mode
        .timer_num = LEDC_LS_TIMER,                 // timer index
        .clk_cfg = LEDC_AUTO_CLK,                   // Auto select the source clock
    };

    ledc_timer_config(&ledc_timer);                 // Set configuration of timer0 for
                                                    //high speed channels

    // ==== Prepare and set configuration of timer1 for low speed channels ====
    ledc_timer.speed_mode = LEDC_HS_MODE;
    ledc_timer.timer_num = LEDC_HS_TIMER;
    ledc_timer_config(&ledc_timer);

    /*
     * Prepare individual configuration
     * for each channel of LED Controller
     * by selecting:
     * - controller's channel number
     * - output duty cycle, set initially to 0
     * - GPIO number where LED is connected to
     * - speed mode, either high or low
     * - timer servicing selected channel
     *   Note: if different channels use one timer,
     *         then frequency and bit_num of these channels
     *         will be the same
     */
    ledc_channel_config_t ledc_channel[LEDC_TEST_CH_NUM] = {
        {.channel = LEDC_HS_CH0_CHANNEL,
         .duty = 0,
         .gpio_num = LEDC_HS_CH0_GPIO,
         .speed_mode = LEDC_HS_MODE,
         .hpoint = 0,
         .timer_sel = LEDC_HS_TIMER},
        {.channel = LEDC_HS_CH1_CHANNEL,
         .duty = 0,
         .gpio_num = LEDC_HS_CH1_GPIO,
         .speed_mode = LEDC_HS_MODE,
         .hpoint = 0,
         .timer_sel = LEDC_HS_TIMER},
        {.channel = LEDC_LS_CH2_CHANNEL,
         .duty = 0,
         .gpio_num = LEDC_LS_CH2_GPIO,
         .speed_mode = LEDC_LS_MODE,
         .hpoint = 0,
         .timer_sel = LEDC_LS_TIMER},
        {.channel = LEDC_LS_CH3_CHANNEL,
         .duty = 0,
         .gpio_num = LEDC_LS_CH3_GPIO,
         .speed_mode = LEDC_LS_MODE,
         .hpoint = 0,
         .timer_sel = LEDC_LS_TIMER},
    };

    // ==== Set LED Controller with previously prepared configuration ====
    for (ch = 0; ch < LEDC_TEST_CH_NUM; ch++)
    {
        ledc_channel_config(&ledc_channel[ch]);
    }

	printf("1. LEDC set duty = %d without fade\n", LEDC_TEST_DUTY);
	for (ch = 0; ch < LEDC_TEST_CH_NUM; ch++)
	{
		ledc_set_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel, LEDC_TEST_DUTY);
		ledc_update_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel);
	}
	vTaskDelay(1000 / portTICK_PERIOD_MS);

}

/***************************************************************************************************
* END OF FILE
***************************************************************************************************/