#include <stdbool.h>
#include <string.h>
#include "main.h"
#include "comms.h"
#include "led.h"

extern SPI_HandleTypeDef hspi2;

rxData_t incomingSPIBuffer;     // this buffer is filled by SPI DMA, but may be invalid
volatile rxData_t rxData;  		// once we know incomingSPIBuffer is valid, it is copied into this for actual use
volatile txData_t txData;

volatile unsigned int packetCount = 0;

uint8_t         rejectCnt;
bool            SPIdataError = false;

void commsStart() {
	SPIdataError = false;
	rxData.header = 0;
	rxData.jointEnable = 0;
	rxData.outputs = 0;
	incomingSPIBuffer.header = 0;
	memset((uint8_t*)&txData, 0, sizeof(txData_t));
    txData.header = PRU_DATA;
    HAL_SPI_TransmitReceive_DMA(&hspi2, (uint8_t*)&txData, (uint8_t*)&incomingSPIBuffer, sizeof(commPacket_t));
}

// Rather than sending WRITE and READ messages as separate transactions, the weeny driver always sends a WRITE
// and expects the READ result to be clocked in as the WRITE is being transmitted (HAL_SPI_TransmitReceive_DMA).
// This means the reply given by the realtime unit will be one loop (1 millisecond) behind.

// On the LinuxCNC side, the command variables are not populated until the first PRU_WRITE is given. We need
// to make sure that first packet is ignored.
bool haveWritePacket = false;

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) {

	switch (incomingSPIBuffer.header)
    {
      case PRU_WRITE:
    	haveWritePacket = true;

    	// fall through !

      case PRU_READ:

    	packetCount++;
        rejectCnt = 0;

        if ( haveWritePacket ) {
        	memcpy( (uint8_t*)&rxData, (uint8_t*)&incomingSPIBuffer, sizeof(rxData_t) );
        }

        break;

      default:
        rejectCnt++;
        if (rejectCnt > 5) {
            SPIdataError = true;
            rxData.jointEnable = 0;
        }
        // reset SPI somehow?
    }

    incomingSPIBuffer.header = 0;

    // Even though the transmission should be complete (we are in the completion callback here!), sometimes
    // the DMA was not returned to idle state resulting in the subsequent call to HAL_SPI_TransmitReceive_DMA
    // failing, causing the SPI connection to drop and a "Bad SPI payload" message on the LinuxCNC side.
    // This (mis-named) HAL_DMA_PollForTransfer ensures the DMA returns to the ready state.
    // https://github.com/STMicroelectronics/STM32CubeF1/issues/14
    HAL_DMA_PollForTransfer(hspi2.hdmatx, HAL_DMA_FULL_TRANSFER, 1);

    HAL_SPI_TransmitReceive_DMA(&hspi2, (uint8_t*)&txData, (uint8_t*)&incomingSPIBuffer, sizeof(commPacket_t));
}




