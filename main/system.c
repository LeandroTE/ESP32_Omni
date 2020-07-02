/***********************************************************************************************************************
 * @file 	System.c
 * @brief	System functions
 * @author	Leandro
 * @date	20/06/2020
 * @company
 *
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/

#include "system.h"

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

// ==== Prepare and set configuration of timers for LED Controller ====
ledc_timer_config_t ledc_timer = {
    .duty_resolution = LEDC_TIMER_13_BIT,        // resolution of PWM duty
    .freq_hz = 1000,                             // frequency of PWM signal
    .speed_mode = LEDC_LS_MODE,                  // timer mode
    .timer_num = LEDC_LS_TIMER,                  // timer index
    .clk_cfg = LEDC_AUTO_CLK,                    // Auto select the source clock
};

ledc_channel_config_t ledc_channel[LEDC_TEST_CH_NUM] = {
    {.channel = LEDC_HS_CH0_CHANNEL,        // Select controller's channel number
     .duty = 0,                             // Set duty cycle to zero
     .gpio_num = LEDC_HS_CH0_GPIO,          // Select GPIO number where pwm is connected
     .speed_mode = LEDC_HS_MODE,            // Set speed mode
     .hpoint = 0,
     .timer_sel = LEDC_HS_TIMER},
    {.channel = LEDC_HS_CH1_CHANNEL,        // Repete for others channels
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

/***********************************************************************************************************************
 * PUBLIC FUNCTIONS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * @brief Responsible for the initialization of the TFT Display
 *
 * @param
 *
 * @return void
 *
 * ********************************************************************************************************************/
void tft_init() {
    esp_err_t ret;

    tft_max_rdclock = 8000000;        // Set maximum spi clock for display read

    TFT_PinsInit();        // Pins MUST be initialized before SPI
                           // interface initialization

    spi_lobo_device_handle_t spi;

    spi_lobo_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,        // set SPI MISO pin
        .mosi_io_num = PIN_NUM_MOSI,        // set SPI MOSI pin
        .sclk_io_num = PIN_NUM_CLK,         // set SPI CLK pin
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 6 * 1024,
    };
    spi_lobo_device_interface_config_t devcfg = {
        .clock_speed_hz = 8000000,                // Initial clock out at 8 MHz
        .mode = 0,                                // SPI mode 0
        .spics_io_num = -1,                       // we will use external CS pin
        .spics_ext_io_num = PIN_NUM_CS,           // external CS pin
        .flags = LB_SPI_DEVICE_HALFDUPLEX,        // ALWAYS SET to HALF DUPLEX MODE for  spi
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

    tft_bg = TFT_BLACK;
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

/***********************************************************************************************************************
 * @brief Responsible for the initialization of the PWM
 *
 * @param
 * @return void
 *
 * 1. Start with initializing LEDC module:
 *    a. Set the timer of LEDC first, this determines the frequency and resolution of PWM.
 *    b. Then set the LEDC channel you want to use, and bind with one of the timers.
 *
 * 2. You need first to install a default fade function,
 *    then you can use fade APIs.
 **********************************************************************************************************************/
void pwm_init() {
    int ch;

    ledc_timer_config(&ledc_timer);        // Set configuration of timer0 for
                                           // high speed channels

    // ==== Prepare and set configuration of timer1 for low speed channels ====
    ledc_timer.speed_mode = LEDC_HS_MODE;
    ledc_timer.timer_num = LEDC_HS_TIMER;
    ledc_timer_config(&ledc_timer);

    // ==== Set LED Controller with previously prepared configuration ====
    for (ch = 0; ch < LEDC_TEST_CH_NUM; ch++) {
        ledc_channel_config(&ledc_channel[ch]);
    }
}

/***********************************************************************************************************************
 * @brief Set the duty cycle in percentagae of the pwm
 *
 * @param duty      - duty cycle to set in percentage ex: 99.5%
 * @param channel   - channel to set duty cycle
 *
 * @return void
 *
 **********************************************************************************************************************/
void set_PWM_duty(float duty, int channel) {
    int duty_cycle;

    duty_cycle = (duty / 100 * MAX_DUTY_CYCLE);
    printf("Duty cycle = %d\n", duty_cycle);
    ledc_set_duty(ledc_channel[channel].speed_mode, ledc_channel[channel].channel, duty_cycle);
    ledc_update_duty(ledc_channel[channel].speed_mode, ledc_channel[channel].channel);
}

/***********************************************************************************************************************
 * @brief Function to set GPIO
 *
 * @param io_conf     gpio_config_t struct
 *
 * @return void
 *
 **********************************************************************************************************************/
void GPIO_Init(gpio_config_t io_conf) {
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;        // disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;        // interrupt of rising edge
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;        // bit mask of the pins, use GPIO35/0 here
    io_conf.mode = GPIO_MODE_INPUT;                   // set as input mode
    io_conf.pull_up_en = 1;                           // enable pull-up mode
    gpio_config(&io_conf);                            // configure GPIO with the given settings

    io_conf.mode = GPIO_MODE_OUTPUT;        // set as output mode

    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;        // bit mask of the pins that you
                                                       // want to set,e.g.GPIO25/26/27
    io_conf.pull_down_en = 0;                          // disable pull-down mode
    io_conf.pull_up_en = 0;                            // disable pull-up mode
    gpio_config(&io_conf);                             // configure GPIO with the given settings

    gpio_set_intr_type(GPIO_INPUT_IO_0, GPIO_INTR_ANYEDGE);        // change gpio intrrupt type for one pin
}

/***********************************************************************************************************************
 * @brief Responsible for the initialization of the UART
 *
 * @param
 * @return void
 *
 **********************************************************************************************************************/
void uart_init(void) {
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    // We won't use a buffer for sending data.
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

/***********************************************************************************************************************
 * @brief Send data to uart
 *
 * @param logName       - log name
 * @param data          - data to be send via uart
 *
 * @return void
 *
 **********************************************************************************************************************/
int sendData( const char* data) {
    int len = strlen(data);
    const int txBytes = uart_write_bytes(UART_NUM_1, data, len);
    return txBytes;
}
/***********************************************************************************************************************
 * END OF FILE
 **********************************************************************************************************************/