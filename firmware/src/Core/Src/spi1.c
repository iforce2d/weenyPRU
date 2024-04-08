#include "main.h"
#include "rgbled.h"

SPI_HandleTypeDef hspi1;
DMA_HandleTypeDef hdma_spi1_tx;


void HAL_SPI_MspInit_rgbled(SPI_HandleTypeDef* hspi) {

	GPIO_InitTypeDef GPIO_InitStruct = {0};

	__HAL_RCC_SPI1_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();

	//SPI1 GPIO Configuration
	//PA5     ------> SPI1_SCK
	//PA7     ------> SPI1_MOSI

	GPIO_InitStruct.Pin = GPIO_PIN_7;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    //__HAL_AFIO_REMAP_SPI1_ENABLE();

	hdma_spi1_tx.Instance = DMA1_Channel3;
	hdma_spi1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
	hdma_spi1_tx.Init.PeriphInc = DMA_PINC_DISABLE;
	hdma_spi1_tx.Init.MemInc = DMA_MINC_ENABLE;
	hdma_spi1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
	hdma_spi1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
	hdma_spi1_tx.Init.Mode = DMA_NORMAL;
	hdma_spi1_tx.Init.Priority = DMA_PRIORITY_LOW;
	if (HAL_DMA_Init(&hdma_spi1_tx) != HAL_OK) {
		Error_Handler();
	}

	__HAL_LINKDMA(hspi,hdmatx,hdma_spi1_tx);
}


HAL_StatusTypeDef HAL_SPI_Init_rgbled(SPI_HandleTypeDef *hspi) {

	hspi->Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;

	//if (hspi->State == HAL_SPI_STATE_RESET) {
		hspi->Lock = HAL_UNLOCKED;
		HAL_SPI_MspInit_rgbled(hspi);
	//}

	hspi->State = HAL_SPI_STATE_BUSY;

	__HAL_SPI_DISABLE(hspi);

	WRITE_REG(hspi->Instance->CR1, (hspi->Init.Mode | hspi->Init.Direction | hspi->Init.DataSize |
			  hspi->Init.CLKPolarity | hspi->Init.CLKPhase | (hspi->Init.NSS & SPI_CR1_SSM) |
			  hspi->Init.BaudRatePrescaler | hspi->Init.FirstBit  | hspi->Init.CRCCalculation));

	WRITE_REG(hspi->Instance->CR2, ((hspi->Init.NSS >> 16U) & SPI_CR2_SSOE));

	CLEAR_BIT(hspi->Instance->I2SCFGR, SPI_I2SCFGR_I2SMOD);

	hspi->ErrorCode = HAL_SPI_ERROR_NONE;
	hspi->State     = HAL_SPI_STATE_READY;

	return HAL_OK;
}


void MX_SPI1_Init_rgbled() {

	__HAL_RCC_DMA1_CLK_ENABLE();
	HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 2, 0);
	HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);

	hspi1.Instance = SPI1;
	hspi1.Init.Mode = SPI_MODE_MASTER;
	hspi1.Init.Direction = SPI_DIRECTION_1LINE;
	hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
	hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
	hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
	hspi1.Init.NSS = SPI_NSS_SOFT;
	hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
	hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
	hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	hspi1.Init.CRCPolynomial = 10;

	if (HAL_SPI_Init_rgbled(&hspi1) != HAL_OK) {
		Error_Handler();
	}
}



void DMA1_Channel3_IRQHandler(void) {
	HAL_DMA_IRQHandler(&hdma_spi1_tx);
}




