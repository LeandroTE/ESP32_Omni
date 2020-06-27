
# Software to control Primitus Omni Robot

---

**This library must be built with the esp-idf release/v4.0 branch.**

ESP-IDF 4.0 is currently in beta and instructions are found [here](
https://docs.espressif.com/projects/esp-idf/en/v4.0-beta1/get-started/index.html)
---

**This repository contains the code to control the Primitus Omni Robot, and is based in the the board TTGO t-display**

This project use the following projects as references:
* https://github.com/jeremyjh/ESP32_TFT_library
* https://github.com/loboris/ESP32_TFT_library

---

## Hardware Description
| Name                  | Pin    |
| ----------            | ------ |
| TFT_MISO              | N/A    |
| TFT_MOSI              | 19     |
| TFT_SCLK              | 18     |
| TFT_CS                | 5      |
| TFT_DC                | 16     |
| TFT_RST               | N/A    |
| TFT_BL                | 4      |
| PWM1 (EN1)            | 21     |
| PWM2 (EN2)            | 22     |
| PWM3 (EN3)            | 34     |
| PWM4 (LIDAR Motor)    | 34     |
| BUTTON1               | 35     |
| BUTTON2               | 0      |
| PH1                   | 25     |
| PH2                   | 26     |
| PH3                   | 27     |
| TX Lidar              | 2      |
| RX Lidar              | 17      |



