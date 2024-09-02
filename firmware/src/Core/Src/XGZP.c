
#include "config.h"

#ifdef USE_I2C_XGZP

#include "main.h"
#include "i2c.h"
#include "XGZP.h"
#include "comms.h"

#define XGZP_I2C_BUS	hi2c1
#define XGZP_ADDRESS 	(0x6D << 1)
#define XGZP_TIMEOUT	1

extern I2C_HandleTypeDef XGZP_I2C_BUS;

float xgzpPressure = 0;;
bool pressureConversionInProgress = false;

bool xgzpConversionReady() {

	uint8_t data[4];
	data[0] = 0x30;
	if ( HAL_OK != HAL_I2C_Master_Transmit(&XGZP_I2C_BUS, XGZP_ADDRESS, data, 1, XGZP_TIMEOUT) ) {
		pressureConversionInProgress = false;
		return false;
	}

	if ( HAL_OK != HAL_I2C_Master_Receive(&XGZP_I2C_BUS, XGZP_ADDRESS, data, 1, XGZP_TIMEOUT) ) {
		pressureConversionInProgress = false;
		return false;
	}

	return (data[0] & 0x08) == 0;
}

void updateXGZP() {

	I2C2_ClearBusyFlag(&XGZP_I2C_BUS);

	if ( ! pressureConversionInProgress ) {
		uint8_t data[4];
		data[0] = 0x30;
		data[1] = 0x0A;
		if ( HAL_OK == HAL_I2C_Master_Transmit(&XGZP_I2C_BUS, XGZP_ADDRESS, data, 2, XGZP_TIMEOUT) ) {
			pressureConversionInProgress = true;
		}
		return;
	}

	if ( ! xgzpConversionReady() )
		return;

	pressureConversionInProgress = false;

	uint8_t data[4];
	data[0] = 0x06;
	if ( HAL_OK != HAL_I2C_Master_Transmit(&XGZP_I2C_BUS, XGZP_ADDRESS, data, 1, XGZP_TIMEOUT) ) {
		return;
	}

	if ( HAL_OK != HAL_I2C_Master_Receive(&XGZP_I2C_BUS, XGZP_ADDRESS, data, 3, XGZP_TIMEOUT) ) {
		return;
	}

	double K = 65536.0;

	uint8_t pressure_H = data[0];
	uint8_t pressure_M = data[1];
	uint8_t pressure_L = data[2];
	long int pressure_adc = pressure_H * 65536 + pressure_M * 256 + pressure_L;
	if (pressure_adc > 8388608)
		xgzpPressure = (pressure_adc - 16777216) / K;
	else
		xgzpPressure = pressure_adc / K;

	int v = (xgzpPressure * 500) + 50000;
	if ( v < 0 )
		v = 0;
	else if ( v > 65535)
		v = 65535;
	txData.vacuum = (uint16_t)v;
}

//void updateXGZP_() {
//	// SPI1 MOSI for RGBLED on PB5 will not work while I2C1 is enabled, wtf !?
//	// https://community.st.com/t5/stm32cubemx-mcus/stm32f103-no-mosi-output-on-spi1-when-using-i2c1-with-cubemx/td-p/456788
//	__HAL_RCC_I2C1_CLK_ENABLE();
//	__HAL_I2C_ENABLE(&XGZP_I2C_BUS);
//
//	updateXGZP_();
//
//	__HAL_I2C_DISABLE(&XGZP_I2C_BUS);
//	__HAL_RCC_I2C1_CLK_DISABLE();
//}

#endif // USE_I2C_XGZP
