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
uint32_t sendRequest(uint8_t cmd, const void *payload, size_t payloadsize, struct lidarStateMachine *stateMachine) {
    rplidar_cmd_packet_t RPLidar_header;
    uint16_t packetSize = 0;
    uint8_t checksum = 0;

    if (payloadsize && payload) {
        cmd |= RPLIDAR_CMDFLAG_HAS_PAYLOAD;
    }

    RPLidar_header.syncByte = RPLIDAR_CMD_SYNC_BYTE;    // Load Start Flag in the resquest packet struct
    RPLidar_header.cmd_flag = cmd;                      // Load the cmd to resquest strcut
    packetSize += 2;                                    // Increment the packet size counter

    if (cmd & RPLIDAR_CMDFLAG_HAS_PAYLOAD) {
        RPLidar_header.size = payloadsize;    // Load payload size
        packetSize++;
        checksum ^= RPLIDAR_CMD_SYNC_BYTE;
        checksum ^= cmd;
        checksum ^= (payloadsize & 0xFF);    // Initialize checksum with first 3 bytes

        for (size_t pos = 0; pos < payloadsize; ++pos) {    // Upgrade checksum with payload
            checksum ^= ((uint8_t *)payload)[pos];
            RPLidar_header.data[pos] = ((uint8_t *)payload)[pos];    // Load payload in packet strcutures
            packetSize++;
        }
        RPLidar_header.checksum = checksum;    // Load checksum in packet structure
        packetSize++;
    }
    uart_write_bytes(UART_NUM_1, (char *)&RPLidar_header, packetSize);    // Send packet struture via UART

    if (cmd == RPLIDAR_CMD_GET_DEVICE_INFO) {
        stateMachine->protocolState = WAITING_RESPONSE_DESCRIPTOR;    // Set protocol state
        stateMachine->operationState = WAITING_GET_INFO;              // Set operation state for wainting info response
    } else if (cmd == RPLIDAR_CMD_GET_DEVICE_HEALTH) {
        stateMachine->protocolState = WAITING_RESPONSE_DESCRIPTOR;    // Set protocol state
        stateMachine->operationState = WAITING_GET_HEALTH;            // Set operation state for wainting helth response
    } else if (cmd == RPLIDAR_CMD_GET_SAMPLE_RATE) {
        stateMachine->protocolState = WAITING_RESPONSE_DESCRIPTOR;    // Set protocol state
        stateMachine->operationState = WAITING_SAMPLE_RATE;           // Set operation state
    } else if (cmd == RPLIDAR_CMD_SCAN) {
        stateMachine->protocolState = WAITING_RESPONSE_DESCRIPTOR;    // Set protocol state
        stateMachine->operationState = READING_SCAN_DATA;             // Set operation state
    }  else if (cmd == RPLIDAR_CMD_RESET) {
        stateMachine->protocolState = IDLE;        // Set protocol state to idle
        stateMachine->operationState = IDLE_OP;    // Set protocol state
    }

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
    stateMachine->protocolState = NON_INIT;    // Start state as non initialize

    sendRequest(RPLIDAR_CMD_GET_DEVICE_INFO, NULL, 0, stateMachine);    // Send info command
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
    static rplidar_resp_descriptor_t response_descriptor;
    static rplidar_response_measurement_node_t scanData;
    struct RPLidarMeasurement currentMeasurement;
    static uint8_t recvPos = 0;
    uint8_t *headerbuf = (uint8_t *)&response_descriptor;    // use pointer do deserialize struct

    // ==== Response descriptor ====
    if (stateMachine->protocolState == WAITING_RESPONSE_DESCRIPTOR &&
        byte == RPLIDAR_ANS_SYNC_BYTE1) {    // Check if fisrt sync byte
        recvPos = 0;
        headerbuf[recvPos++] = byte;
        stateMachine->protocolState = WAITING_SYNC_BYTE2;
        printf("Response Descriptor decode: \n");
        printf("Response header sync 1: %x\n", response_descriptor.syncByte1);
    } else if (stateMachine->protocolState == WAITING_SYNC_BYTE2 &&
               byte == RPLIDAR_ANS_SYNC_BYTE2) {    // Check if second sync byte
        headerbuf[recvPos++] = byte;
        stateMachine->protocolState = WAITING_LENGTH_MODE;
        printf("Response header sync 2: %x\n", response_descriptor.syncByte2);
    } else if (stateMachine->protocolState ==
               WAITING_LENGTH_MODE) {    // wait for response length and mode data as UNION
        headerbuf[recvPos++] = byte;
        if (recvPos == 6) {
            stateMachine->protocolState = WAITTING_DATA_TYPE;
            printf("Data Response Length: %d\n", response_descriptor.size);
            printf("Send Mode: %d\n", response_descriptor.subType);
        }
    } else if (stateMachine->protocolState == WAITTING_DATA_TYPE) {    // Waiting for data type
        headerbuf[recvPos++] = byte;
        if (stateMachine->operationState == READING_SCAN_DATA) {
            stateMachine->protocolState = WAITING_SET_FLAG;
            printf("protocolState = WAITING_SET_FLAG\n");
        } else {
            stateMachine->protocolState = WAITING_FOR_REPONSE;
            printf("protocolState = WAITING_FOR_REPONSE\n");
        }
        // Debug: erase later
        printf("Data Type: %x\n", response_descriptor.type);
        printf("Response Descriptor data: ");
        for (int i = 0; i < sizeof(response_descriptor); i++) {
            printf("%02x ", (unsigned int)((char *)&response_descriptor)[i]);
        }
        printf("\n");
        return;
    }

    // ==== Response Get Info ====
    headerbuf = (uint8_t *)&stateMachine->lidarConfig;    // Get point for config info
    if (stateMachine->protocolState == WAITING_FOR_REPONSE &&
        stateMachine->operationState == WAITING_GET_INFO) {    // Decode get info response
        recvPos = 0;                                           // Reset pointer counter
        headerbuf[recvPos++] = byte;                           // Read first byte from responsa data
        stateMachine->protocolState = READING_RESPONSE;        // Set state to receiving data
    } else if (stateMachine->protocolState == READING_RESPONSE &&
               stateMachine->operationState == WAITING_GET_INFO) {    // Reading data from Get Info response
        headerbuf[recvPos++] = byte;                                  // Read  responsa data
        if (recvPos == 20) {    // After received last byte set state machine to idle
            stateMachine->protocolState = IDLE;
            stateMachine->operationState = IDLE_OP;
            printf("\r\n==============================\r\n");
            printf("\nResponse data decode: \n");
            printf("Model: %d\n", stateMachine->lidarConfig.model);    // Print model number
            printf("FW v%d.%d\n", stateMachine->lidarConfig.firmware_version,
                   stateMachine->lidarConfig.firmware_revision);                   // Print FW ver X.Y
            printf("HW ver v%d\n", stateMachine->lidarConfig.hardware_version);    // Print HE version
            printf("Serial number: ");
            for (int i = 0; i < 16; i++) {
                printf("%X", stateMachine->lidarConfig.serialnum[i]);
            }
            printf("\nResponse data: ");
            for (int i = 0; i < sizeof(stateMachine->lidarConfig); i++) {
                printf("%02x ", (unsigned int)((char *)&stateMachine->lidarConfig)[i]);
            }
            printf("\n");
            return;
        }
    }

    // ==== Response Get Health ====
    headerbuf = (uint8_t *)&stateMachine->lidarHealth;    // Get point for config info
    if (stateMachine->protocolState == WAITING_FOR_REPONSE &&
        stateMachine->operationState == WAITING_GET_HEALTH) {    // Decode get info response
        recvPos = 0;                                             // Reset pointer counter
        headerbuf[recvPos++] = byte;                             // Read first byte from responsa data
        stateMachine->protocolState = READING_RESPONSE;          // Set state to receiving data
    } else if (stateMachine->protocolState == READING_RESPONSE &&
               stateMachine->operationState == WAITING_GET_HEALTH) {    // Reading data from Get Info response
        headerbuf[recvPos++] = byte;                                    // Read  responsa data
        if (recvPos == 3) {    // After received last byte set state machine to idle
            stateMachine->protocolState = IDLE;
            stateMachine->operationState = IDLE_OP;
            printf("\r\n==============================\r\n");
            printf("\nResponse data decode: \n");
            printf("Status: %d\n", stateMachine->lidarHealth.status);       // Print status
            printf("Error: %d\n", stateMachine->lidarHealth.error_code);    // Print Error Code
            printf("\nResponse data: ");
            for (int i = 0; i < sizeof(stateMachine->lidarHealth); i++) {
                printf("%02x ", (unsigned int)((char *)&stateMachine->lidarHealth)[i]);
            }
            printf("\n");
            return;
        }
    }

    // ==== Response Get Sample Rate ====
    headerbuf = (uint8_t *)&stateMachine->sampleRate;    // Get pointer for sample rate
    if (stateMachine->protocolState == WAITING_FOR_REPONSE &&
        stateMachine->operationState == WAITING_SAMPLE_RATE) {    // Decode get info response
        recvPos = 0;                                              // Reset pointer counter
        headerbuf[recvPos++] = byte;                              // Read first byte from responsa data
        stateMachine->protocolState = READING_RESPONSE;           // Set state to receiving data
    } else if (stateMachine->protocolState == READING_RESPONSE &&
               stateMachine->operationState == WAITING_SAMPLE_RATE) {    // Reading data from Get sample rate
        headerbuf[recvPos++] = byte;                                     // Read  responsa data
        if (recvPos == 4) {
            stateMachine->protocolState = IDLE;    // After received last byte set state machine to idle
            stateMachine->operationState = IDLE_OP;
            printf("\r\n==============================\r\n");
            printf("\nResponse data decode: \n");
            printf("Tstandard: %d\n", stateMachine->sampleRate.Tstandard);    // Print standard
            printf("Texpress: %d\n", stateMachine->sampleRate.Texpress);      // Print Texpress
            printf("\nResponse data: ");
            for (int i = 0; i < sizeof(stateMachine->sampleRate); i++) {
                printf("%02x ", (unsigned int)((char *)&stateMachine->sampleRate)[i]);
            }
            printf("\n");
            return;
        }
    }

    // ==== Response Get Scan data ====
    headerbuf = (uint8_t *)&scanData;    // Get pointer for scan data
    //printf("Protocal state = %d operation state%d\n", stateMachine->protocolState, stateMachine->operationState);
    if (stateMachine->protocolState == WAITING_SET_FLAG &&    // Wait for sync flag
        stateMachine->operationState == READING_SCAN_DATA) {
        printf("WAITING_SET_FLAG\n");
        if (byte & RPLIDAR_RESP_MEASUREMENT_CHECKBIT) {    //
            printf("Set flag found\n");
            recvPos = 0;                                       // Reset pointer counter
            headerbuf[recvPos++] = byte;                       // Read first byte from responsa data
            stateMachine->protocolState = READING_RESPONSE;    // Set state to receiving data
        }
    } else if (stateMachine->protocolState == WAITING_FOR_NEXT_SCAN_DATA &&
               stateMachine->operationState == READING_SCAN_DATA) {    // Read next scan data
        recvPos = 0;                                                   // Reset pointer counter
        headerbuf[recvPos++] = byte;                                   // Read first byte from responsa data
        stateMachine->protocolState = READING_RESPONSE;                // Set state to receiving data
    } else if (stateMachine->protocolState == READING_RESPONSE &&
               stateMachine->operationState == READING_SCAN_DATA) {    // Reading package from scan data
        headerbuf[recvPos++] = byte;                                   // Read  responsa data
        if (recvPos == sizeof(scanData)) {
            stateMachine->protocolState =
                WAITING_FOR_NEXT_SCAN_DATA;    // After received last byte set state machine to idle
            currentMeasurement.distance = scanData.distance_q2 / 4.0f;
            currentMeasurement.angle = (scanData.angle_q6_checkbit >> RPLIDAR_RESP_MEASUREMENT_ANGLE_SHIFT) / 64.0f;
            currentMeasurement.quality = (scanData.sync_quality >> RPLIDAR_RESP_MEASUREMENT_QUALITY_SHIFT);
            currentMeasurement.startBit = (scanData.sync_quality & RPLIDAR_RESP_MEASUREMENT_SYNCBIT);
            printf("d= %2.2f angle= %3.1f\n", currentMeasurement.distance,
                   currentMeasurement.angle);    // Print standard
            return;
        }
    }
    // printf("Received byte in state machine: %x\n", byte);
}

/***********************************************************************************************************************
 * END OF FILE
 **********************************************************************************************************************/