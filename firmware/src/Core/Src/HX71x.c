
#include "config.h"

#ifdef USE_LOADCELL

#include "main.h"
#include "spi1_hx71x.h"
#include "comms.h"

#define HX71x_DATA_PORT		GPIOB
#define HX71x_DATA_PIN		GPIO_PIN_4

void initHX71x() {

	MX_SPI1_Init_hx71x();
}

bool hx71x_wasReady = false;

extern SPI_HandleTypeDef hspi1;

uint8_t hx71x_data[4];

void updateHX71x() {

	// check if data became ready since last time
	bool isReady = (HAL_GPIO_ReadPin( HX71x_DATA_PORT, HX71x_DATA_PIN ) == GPIO_PIN_RESET);
//	if ( ! isReady || hx71x_wasReady ) {
//		hx71x_wasReady = false;
//		return;
//	}
//	hx71x_wasReady = true;
	if ( ! isReady )
		return;

	HAL_SPI_Receive_DMA( &hspi1, hx71x_data, sizeof(hx71x_data) );
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
	if ( hspi->Instance == SPI1 ) {
		int32_t tmpInt = ((uint32_t) hx71x_data[0] << 16) | ((uint32_t) hx71x_data[1] << 8) | (uint32_t) hx71x_data[2];
		tmpInt -= 0x800000;
		txData.loadcell = tmpInt;
	}
}

#endif // USE_LOADCELL
