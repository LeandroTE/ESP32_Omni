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

    RPLidar_header.syncByte = RPLIDAR_CMD_SYNC_BYTE;        // Load Start Flag in the resquest packet struct
    RPLidar_header.cmd_flag = cmd;                          // Load the cmd to resquest strcut
    packetSize += 2;                                        // Increment the packet size counter

    if (cmd & RPLIDAR_CMDFLAG_HAS_PAYLOAD) {
        RPLidar_header.size = payloadsize;        // Load payload size
        packetSize++;
        checksum ^= RPLIDAR_CMD_SYNC_BYTE;
        checksum ^= cmd;
        checksum ^= (payloadsize & 0xFF);        // Initialize checksum with first 3 bytes

        for (size_t pos = 0; pos < payloadsize; ++pos) {        // Upgrade checksum with payload
            checksum ^= ((uint8_t *)payload)[pos];
            RPLidar_header.data[pos] = ((uint8_t *)payload)[pos];        // Load payload in packet strcutures
            packetSize++;
        }
        RPLidar_header.checksum = checksum;        // Load checksum in packet structure
        packetSize++;
    }
    uart_write_bytes(UART_NUM_1, (char *)&RPLidar_header, packetSize);        // Send packet struture via UART
    return RESULT_OK;
}

/***********************************************************************************************************************
 * END OF FILE
 **********************************************************************************************************************/