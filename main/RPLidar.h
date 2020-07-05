/***********************************************************************************************************************
 * @file 	RPLidar.h
 * @brief	RPLidar functions
 * @author	Leandro
 * @date	01/07/2020
 * @company
 *
 **********************************************************************************************************************/

#ifndef RPLIDAR_H_
#define RPLIDAR_H_

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "display.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/***********************************************************************************************************************
 * COSNTANTS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * MACROS
 **********************************************************************************************************************/
// ================== RP-Lidar Input Packets ==================
#define RPLIDAR_CMD_SYNC_BYTE 0xA5
#define RPLIDAR_CMDFLAG_HAS_PAYLOAD 0x80

#define RPLIDAR_ANS_SYNC_BYTE1 0xA5
#define RPLIDAR_ANS_SYNC_BYTE2 0x5A

#define RPLIDAR_ANS_PKTFLAG_LOOP 0x1

// ================== Commands ==================

// Commands without payload and response
#define RPLIDAR_CMD_STOP 0x25
#define RPLIDAR_CMD_SCAN 0x20
#define RPLIDAR_CMD_FORCE_SCAN 0x21
#define RPLIDAR_CMD_RESET 0x40

// Commands without payload but have response
#define RPLIDAR_CMD_GET_DEVICE_INFO 0x50
#define RPLIDAR_CMD_GET_DEVICE_HEALTH 0x52

// ================== Response ==================
#define RPLIDAR_ANS_TYPE_MEASUREMENT 0x81

#define RPLIDAR_ANS_TYPE_DEVINFO 0x4
#define RPLIDAR_ANS_TYPE_DEVHEALTH 0x6

#define RPLIDAR_STATUS_OK 0x0
#define RPLIDAR_STATUS_WARNING 0x1
#define RPLIDAR_STATUS_ERROR 0x2

#define RPLIDAR_RESP_MEASUREMENT_SYNCBIT (0x1 << 0)
#define RPLIDAR_RESP_MEASUREMENT_QUALITY_SHIFT 2
#define RPLIDAR_RESP_MEASUREMENT_CHECKBIT (0x1 << 0)
#define RPLIDAR_RESP_MEASUREMENT_ANGLE_SHIFT 1

#define RESULT_OK 0
#define RESULT_FAIL_BIT 0x80000000
#define RESULT_ALREADY_DONE 0x20
#define RESULT_INVALID_DATA (0x8000 | RESULT_FAIL_BIT)
#define RESULT_OPERATION_FAIL (0x8001 | RESULT_FAIL_BIT)
#define RESULT_OPERATION_TIMEOUT (0x8002 | RESULT_FAIL_BIT)
#define RESULT_OPERATION_STOP (0x8003 | RESULT_FAIL_BIT)
#define RESULT_OPERATION_NOT_SUPPORT (0x8004 | RESULT_FAIL_BIT)
#define RESULT_FORMAT_NOT_SUPPORT (0x8005 | RESULT_FAIL_BIT)
#define RESULT_INSUFFICIENT_MEMORY (0x8006 | RESULT_FAIL_BIT)

#define IS_OK(x) (((x)&RESULT_FAIL_BIT) == 0)
#define IS_FAIL(x) (((x)&RESULT_FAIL_BIT))

/***********************************************************************************************************************
 * TYPES
 **********************************************************************************************************************/

typedef struct rplidar_cmd_packet_t {
    uint8_t syncByte;        // must be RPLIDAR_CMD_SYNC_BYTE
    uint8_t cmd_flag;
    uint8_t size;
    uint8_t data[0];
    uint8_t checksum;
} rplidar_cmd_packet_t;

typedef struct rplidar_ans_header_t {
    uint8_t syncByte1;        // must be RPLIDAR_ANS_SYNC_BYTE1
    uint8_t syncByte2;        // must be RPLIDAR_ANS_SYNC_BYTE2
    uint32_t size : 30;
    uint32_t subType : 2;
    uint8_t type;
} rplidar_ans_header_t;

typedef struct rplidar_response_measurement_node_t {
    uint8_t sync_quality;              // syncbit:1;syncbit_inverse:1;quality:6;
    uint16_t angle_q6_checkbit;        // check_bit:1;angle_q6:15;
    uint16_t distance_q2;
} rplidar_response_measurement_node_t;

typedef struct rplidar_response_device_info_t {
    uint8_t model;
    uint16_t firmware_version;
    uint8_t hardware_version;
    uint8_t serialnum[16];
} rplidar_response_device_info_t;

typedef struct rplidar_response_device_health_t {
    uint8_t status;
    uint16_t error_code;
} rplidar_response_device_health_t;

enum lidarProtocolStates {              // Lidar possible states
    IDLE,                               // Protocol state in idle
    WAITING_RESPONSE_DESCRIPTOR,        // Waiting for getting info response

};

enum lidarOperationStates {        // Lidar possible states
    IDLE_OP,                       // Non initialize when started
    NON_INIT,                      // Idle state
    WAITING_GET_INFO,              // Waiting for getting info response

};

struct lidarStateMachine {        // Maquina de estado da interface de comunicação
    enum lidarProtocolStates protocolState;
    enum lidarOperationStates operationState;
};

/***********************************************************************************************************************
 * GLOBALS VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * PUBLIC FUNCTIONS
 **********************************************************************************************************************/

uint32_t sendRequest(uint8_t cmd, const void *payload, size_t payloadsize);
void lidarBeginStateMachine(struct lidarStateMachine *stateMachine);
void lidarSendByteToStateMachine(uint8_t byte, struct lidarStateMachine *stateMachine);

/***********************************************************************************************************************
 * END OF FILE
 **********************************************************************************************************************/
#endif /* RPLIDAR_H_ */
