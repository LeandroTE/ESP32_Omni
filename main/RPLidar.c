/***********************************************************************************************************************
 * @file 	RPLidar.c
 * @brief	RPLidar functions
 * @author	Leandro
 * @date	01/07/2020
 * @company
 *
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/

#include "RPLidar.h"

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
 * @brief Send command to RPLidar via uart
 *
 * @param cmd           - cmd to be sent
 * @param payload       - pointer to payload
 * @param payloadsize   - payload size
 * @return              - result (uint8)
 *
 **********************************************************************************************************************/
uint32_t sendRequest(uint8_t cmd, const void *payload, size_t payloadsize) {
    rplidar_cmd_packet_t RPLidar_header;
    uint16_t packetSize = 0;
    uint8_t checksum = 0;

    if (payloadsize && payload) {
        cmd |= RPLIDAR_CMDFLAG_HAS_PAYLOAD;
    }

    RPLidar_header.syncByte = RPLIDAR_CMD_SYNC_BYTE;                            // Load Start Flag in the resquest packet struct
    RPLidar_header.cmd_flag = cmd;                                              // Load the cmd to resquest strcut
    packetSize += 2;                                                            // Increment the packet size counter

    if (cmd & RPLIDAR_CMDFLAG_HAS_PAYLOAD) {
        RPLidar_header.size = payloadsize;                                      // Load payload size
        packetSize++;
        checksum ^= RPLIDAR_CMD_SYNC_BYTE;
        checksum ^= cmd;
        checksum ^= (payloadsize & 0xFF);                                       // Initialize checksum with first 3 bytes

        for (size_t pos = 0; pos < payloadsize; ++pos) {                        // Upgrade checksum with payload
            checksum ^= ((uint8_t *)payload)[pos];
            RPLidar_header.data[pos] = ((uint8_t *)payload)[pos];               // Load payload in packet strcutures
            packetSize++;
        }
        RPLidar_header.checksum = checksum;                                     // Load checksum in packet structure
        packetSize++;
    }
    uart_write_bytes(UART_NUM_1, (char *)&RPLidar_header, packetSize);          // Send packet struture via UART
    return RESULT_OK;
}

/***********************************************************************************************************************
 * @brief Start state machine for lidar interface
 *
 * @param stateMachine  - state machine pointer
 * @return              - void
 *
 **********************************************************************************************************************/
void lidarBeginStateMachine(struct lidarStateMachine *stateMachine) {
    stateMachine->protocolState = NON_INIT;                                     // Start state as non initialize

    sendRequest(RPLIDAR_CMD_GET_DEVICE_INFO, NULL, 0);                          // Send info command
    stateMachine->protocolState = WAITING_RESPONSE_DESCRIPTOR;                  // Set protocol state to idle
    stateMachine->operationState = WAITING_GET_INFO;                             // Set operation state for wainting info response
}

/***********************************************************************************************************************
 * @brief Send byte to state machine
 *
 * @param byte          - byte to send to state machine
 * @param stateMachine  - state machine pointer
 * @return              - void
 *
 **********************************************************************************************************************/
void lidarSendByteToStateMachine(uint8_t byte, struct lidarStateMachine *stateMachine) {
    printf("Received byte in state machine: %x\n", byte);
    // State machine code
}

/***********************************************************************************************************************
 * END OF FILE
 **********************************************************************************************************************/