
#include "config.h"

#ifdef USE_I2C_DS3502

#include "main.h"
#include "i2c.h"
#include "comms.h"

#define DS3502_I2C_BUS	hi2c1
#define DS3502_ADDRESS 	(0x28 << 1)
#define DS3502_TIMEOUT	1

extern I2C_HandleTypeDef DS3502_I2C_BUS;

#define DS3502_WR_REG 		0x00
#define DS3502_CTRL_REG 	0x02

#define DS3502_CTRLMODE_BOTH 	0x00 // set both wiper value and initial value
#define DS3502_CTRLMODE_WIPER	0x80 // set only wiper value

void initDS3502() {
	// write zero to both wiper and initval
	uint8_t ctrlMode = DS3502_CTRLMODE_BOTH;
	uint8_t wiperVal = 0;
	HAL_I2C_Mem_Write( &DS3502_I2C_BUS, DS3502_ADDRESS, DS3502_CTRL_REG, I2C_MEMADD_SIZE_8BIT, &ctrlMode, 1, DS3502_TIMEOUT );
	HAL_I2C_Mem_Write( &DS3502_I2C_BUS, DS3502_ADDRESS, DS3502_WR_REG,   I2C_MEMADD_SIZE_8BIT, &wiperVal, 1, DS3502_TIMEOUT );

	HAL_Delay(20);

	// now change to only setting wiper value
	ctrlMode = DS3502_CTRLMODE_WIPER;
	HAL_I2C_Mem_Write( &DS3502_I2C_BUS, DS3502_ADDRESS, DS3502_CTRL_REG, I2C_MEMADD_SIZE_8BIT, &ctrlMode, 1, DS3502_TIMEOUT );
}

void updateDS3502(float val) {

	I2C2_ClearBusyFlag(&DS3502_I2C_BUS);

	uint8_t wiperVal = val * 127;
	HAL_I2C_Mem_Write( &DS3502_I2C_BUS, DS3502_ADDRESS, DS3502_WR_REG,   I2C_MEMADD_SIZE_8BIT, &wiperVal, 1, DS3502_TIMEOUT );
}

#endif // USE_I2C_DS3502
