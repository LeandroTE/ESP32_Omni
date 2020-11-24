
# Software to control iOmni Robot

* More detail in [hackday.io] (https://hackaday.io/project/173420-iomni)
---

**This library was built with the esp-idf 4.2 commit bce69d9fc1a70ef7ba57b4e1dfbf73bae0b2b414**
---

**This repository contains the code to control the iOmni Robot, and is based in the the board TTGO t-display**

This project use the following projects as references:
* https://github.com/jeremyjh/ESP32_TFT_library
* https://github.com/loboris/ESP32_TFT_library
* https://github.com/espressif/esp-idf/tree/master/examples/protocols/http_server/restful_server

---

## Hardware Description
| Name                  | Pin    |
| ----------            | ------ |
| TFT_MOSI              | 19     |
| TFT_SCLK              | 18     |
| TFT_CS                | 5      |
| TFT_DC                | 16     |
| TFT_BL                | 4      |
|                       |        |
| PWM1 (EN1)            | 2      |
| PWM2 (EN2)            | 15     |
| PWM3 (EN3)            | 13     |
| PWM4 (LIDAR Motor)    | 12     |
| BUTTON1               | 35     |
| BUTTON2               | 0      |
| PH1                   | 25     |
| PH2                   | 26     |
| PH3                   | 27     |
| Sleep                 | 17     |
| TX Lidar              | 2      |
| RX Lidar              | 17     |








