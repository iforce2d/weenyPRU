#ifndef SRC_MESSAGES_H_
#define SRC_MESSAGES_H_

#define JOINTS			    4				// Number of joints - set this the same as LinuxCNC HAL component. Max 4 joints for F103C8

#define PRU_DATA		    0x64617461 	    // "data" SPI payload
#define PRU_READ            0x72656164      // "read" SPI payload
#define PRU_WRITE           0x77726974      // "writ" SPI payload
#define PRU_ESTOP           0x65737470      // "estp" SPI payload

// TX and RX packets are sent and received in the same SPI transfer, so they must have the same size.

typedef struct
{
    int32_t header;                 // 4 bytes
    int32_t jointFreqCmd[JOINTS];   // 16 bytes
    uint8_t jointEnable;            // 1 byte
    uint16_t outputs;               // 2 bytes
    uint8_t rgb[6];                 // 6 bytes (3 bits per LED x 16 LEDs = 48 bits)
    uint16_t spindleSpeed;          // 2 bytes

    uint8_t dummy[11];              // make up to same size as txData_t
} rxData_t;

typedef struct
{
    int32_t header;                 // 4 bytes
    int32_t jointFeedback[JOINTS];  // 16 bytes
    int32_t jogcounts[4];      		// 16 bytes
    uint16_t inputs;                // 2 bytes
    uint16_t adc[2];				// 4 bytes

} txData_t;


typedef union
{
	rxData_t rx;
	txData_t tx;
} commPacket_t;




#endif /* SRC_MESSAGES_H_ */
