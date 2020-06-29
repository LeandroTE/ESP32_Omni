/***********************************************************************************************************************
 * @file 	display.h
 * @brief
 * @author	Leandro
 * @date	20/06/2020
 * @company
 *
 **********************************************************************************************************************/

#ifndef DISPLAY_H_
#define DISPLAY_H_

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "spiffs_vfs.h"
#include "tft.h"
#include "tftspi.h"

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

void disp_header(char *info);

/***********************************************************************************************************************
 * END OF FILE
 **********************************************************************************************************************/
#endif /* DISPLAY_H_ */