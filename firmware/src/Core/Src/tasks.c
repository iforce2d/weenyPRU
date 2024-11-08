
#include "tasks.h"
#include "config.h"
#include "comms.h"
#include "digitalout.h"
#include "stepgen.h"
#include "led.h"
#include "rgbled.h"
#include "jog.h"
#include "TMC2209.h"
#include "XGZP.h"
#include "DS3502.h"
#include "HX71x.h"
#include "config.h"

extern TIM_HandleTypeDef htim1;

bool haveComms = false; // this is used to stop spindle and steppers if SPI link to RPi drops for more than a few cycles

Stepgen sg_0;
Stepgen sg_1;
Stepgen sg_2;
Stepgen sg_3;

TMC_UART tmc_0;
TMC_UART tmc_1;
TMC_UART tmc_2;
TMC_UART tmc_3;

#define TMC_DEFAULT_MICROSTEPS	8		// 0, 2, 4, 8, 16, 32, 64, 128, 256
#define TMC_DEFAULT_CURRENT		200		// 0 - 2000

void setupTasks() {

	initStepgen(&sg_0, 0, GPIOA, GPIO_PIN_4, TIM_CHANNEL_1); // X: dir on PA4, step on PA0
	initStepgen(&sg_1, 1, GPIOA, GPIO_PIN_5, TIM_CHANNEL_2); // Y: dir on PA5, step on PA1
	initStepgen(&sg_2, 2, GPIOA, GPIO_PIN_6, TIM_CHANNEL_3); // Z: dir on PA6, step on PA2
	initStepgen(&sg_3, 3, GPIOB, GPIO_PIN_5, TIM_CHANNEL_4); // A: dir on PB5, step on PA3

#ifdef USE_UART_TMC
	HAL_Delay(15); // TMCs take a while to wake up?

	initTMCUART(&tmc_0, 0x00, TMC_DEFAULT_MICROSTEPS, TMC_DEFAULT_CURRENT);
	initTMCUART(&tmc_1, 0x01, TMC_DEFAULT_MICROSTEPS, TMC_DEFAULT_CURRENT);
	initTMCUART(&tmc_2, 0x02, TMC_DEFAULT_MICROSTEPS, TMC_DEFAULT_CURRENT);
	initTMCUART(&tmc_3, 0x03, TMC_DEFAULT_MICROSTEPS, TMC_DEFAULT_CURRENT);
#endif
}

typedef struct PortAndPin {
	GPIO_TypeDef* port;
	uint16_t pin;
	uint32_t pull;
} PortAndPin;

// You can move these rows between the input and output arrays to allocate
// pins between input and output as desired. The order they appear here is
// the order that LinuxCNC will refer to them as. Eg. the first row in the
// digitalIns array will become weeny.input.00
//
// The labels D1, D2 etc are the silkscreen labels on the weenyPRU board.
//
// If using UART control for TMC drivers, or I2C for pressure sensor, the
// rows for those relevant pins must not be included in these arrays.
//
// *** DO NOT PUT MORE THAN 16 ENTRIES IN ONE ARRAY HERE *** their indices will
// be used as bit positions in a uint16 value for communication with the RPi.

// GPIO_NOPULL is zero, so these could be omitted if that's what you want.
// Other options are GPIO_PULLUP or GPIO_PULLDOWN
PortAndPin digitalIns[] = {
	{ GPIOB, GPIO_PIN_2,  GPIO_NOPULL }, // D1
	{ GPIOB, GPIO_PIN_10, GPIO_NOPULL }, // D2
	{ GPIOB, GPIO_PIN_11, GPIO_NOPULL }, // D3
	{ GPIOA, GPIO_PIN_11, GPIO_NOPULL }, // D4
	{ GPIOA, GPIO_PIN_9,  GPIO_NOPULL }, // D5
	{ GPIOA, GPIO_PIN_10, GPIO_NOPULL }, // D6
	{ GPIOA, GPIO_PIN_12, GPIO_NOPULL }, // D7
#ifndef USE_ANALOG
	{ GPIOB, GPIO_PIN_0,  GPIO_NOPULL }, // ADC1
	{ GPIOB, GPIO_PIN_1,  GPIO_NOPULL }, // ADC2
#endif
};

// The pull up/down options are ignored for outputs.
PortAndPin digitalOuts[] = {
	{ GPIOA, GPIO_PIN_15 }, // D8
#ifndef USE_LOADCELL
	{ GPIOB, GPIO_PIN_3  }, // D9  or SPI1 CLK
	{ GPIOB, GPIO_PIN_4  }, // D10 or SPI1 MISO
#endif
	{ GPIOB, GPIO_PIN_7  }, // D11
#ifndef USE_UART_TMC
	{ GPIOB, GPIO_PIN_6  }, // D12 or TMC_UART
#endif
#if !(defined(USE_I2C_XGZP) || defined(USE_I2C_DS3502))
	{ GPIOB, GPIO_PIN_8  }, // D13 or I2C1_SCL
	{ GPIOB, GPIO_PIN_9  }, // D14 or I2C1_SDA
#endif
#ifndef USE_RGBLED
	{ GPIOA, GPIO_PIN_7  }, // RGB
#endif
#ifndef USE_ROTARY_ENCODER
	{ GPIOC, GPIO_PIN_14  }, // DT
	{ GPIOC, GPIO_PIN_15  }, // CLK
#endif
};

void setupGPIOPins() {

	GPIO_InitTypeDef GPIO_InitStruct = {0};

	for (int i = 0; i < (sizeof(digitalIns)/sizeof(PortAndPin)); i++) {
		PortAndPin pap = digitalIns[i];
		GPIO_InitStruct.Pin = pap.pin;
		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
		GPIO_InitStruct.Pull = pap.pull;
		HAL_GPIO_Init(pap.port, &GPIO_InitStruct);
	}

	for (int i = 0; i < (sizeof(digitalOuts)/sizeof(PortAndPin)); i++) {
		PortAndPin pap = digitalOuts[i];
		GPIO_InitStruct.Pin = pap.pin;
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		HAL_GPIO_Init(pap.port, &GPIO_InitStruct);
	}
}

// Crude scheduling for 'slow' tasks in doMainLoopTasks. These will be incremented at 1kHz in the servo thread,
// they can each decide when enough time has passed that they need to act (and set this value back to zero).
uint32_t msSince_digitalInputRead = 0;
uint32_t msSince_rgbLEDUpdate = 0;
uint32_t msSince_pwmUpdate = 0;
uint32_t msSince_DS3502Update = 0;
uint32_t msSince_TMCUpdate = 0;
uint32_t msSince_XGZPUpdate = 0;

void doServoThreadTasks() { // 1 kHz interrupt, return asap

	static uint32_t servoThreadCount = 0;
	static int noCommsCount = 0;

	msSince_digitalInputRead++;
	msSince_rgbLEDUpdate++;
	msSince_pwmUpdate++;
	msSince_DS3502Update++;
	msSince_TMCUpdate++;
	msSince_XGZPUpdate++;

	// this value will only roll over every 50 days, but let's try to avoid it anyway
	servoThreadCount++;
	if ( servoThreadCount > 999 )
		servoThreadCount = 0;

	if ( packetCount < 1 )
		noCommsCount++;
	else
		noCommsCount = 0;

	haveComms = noCommsCount < 10; // allow missing up to 10 expected packets

	if ( haveComms ) {
		CONNECTED_LED_ON;
	}
	else {
		rxData.jointEnable = 0;
		rxData.outputs = 0;
		CONNECTED_LED_OFF;
	}

	packetCount = 0;

#ifdef USE_ANALOG
	if ( (servoThreadCount % 20) == 0 ) { // 50Hz
		doJogUpdate();
	}
#endif

	for (int i = 0; i < (sizeof(digitalOuts)/sizeof(PortAndPin)); i++) {
		PortAndPin pap = digitalOuts[i];
		HAL_GPIO_WritePin(pap.port, pap.pin, (rxData.outputs & (1 << i)) ? GPIO_PIN_SET : GPIO_PIN_RESET);
	}

#ifdef USE_LOADCELL
	updateHX71x(); // do this last to have more chance of catching HX71x being ready, and have SPI active after we exit rather than during
#endif
}

void doBaseThreadTasks() { // 50kHz interrupt, no screwing around here!

	prepStep(&sg_0);
	prepStep(&sg_1);
	prepStep(&sg_2);
	prepStep(&sg_3);

	TIM2->CNT = 0;
}


void doMainLoopTasks() {

#if (defined(USE_I2C_XGZP) || defined(USE_I2C_DS3502))
	// don't do two I2C things in same loop
	bool didSomethingOnI2C = false;
#endif

	if ( msSince_digitalInputRead > 1 ) {
		for (int i = 0; i < (sizeof(digitalIns)/sizeof(PortAndPin)); i++) {
			PortAndPin pap = digitalIns[i];
			int digitalInput = HAL_GPIO_ReadPin(pap.port, pap.pin);
			if ( digitalInput ) // high means not pressed
				txData.inputs &= ~(uint8_t)(1 << i);
			else
				txData.inputs |= (uint8_t)(1 << i);
		}
		msSince_digitalInputRead = 0;
	}

	if ( msSince_pwmUpdate > 40 ) {
		int pwmCompare = haveComms ? ((rxData.spindleSpeed / 65535.0f) * 3600) : 0;
		__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, pwmCompare);
		msSince_pwmUpdate = 0;
	}

#ifdef USE_I2C_DS3502
	if ( msSince_DS3502Update > 30 ) {
		if ( ! didSomethingOnI2C ) {
			float val = haveComms ? (rxData.spindleSpeed / 65535.0f) : 0;
			updateDS3502(val);
			didSomethingOnI2C = true;
			msSince_DS3502Update = 0;
		}
	}
#endif

#ifdef USE_RGBLED
	// RGB LED transmit increases with number of LEDs --> with DMA, 16 LEDs take 520us
	if ( msSince_rgbLEDUpdate > 20 ) {
		if ( haveComms )
			updateRGBLEDs();
		else
			turnOffAllRGBLEDs();
		doRGBLEDOutput(); // DMA, takes a little time. I2C1 must not be enabled during this phase!
		msSince_rgbLEDUpdate = 0;
	}
#endif

#ifdef USE_I2C_XGZP
	// Update pressure sensor at about 100Hz
	if ( msSince_XGZPUpdate > 10 ) {
		if ( ! didSomethingOnI2C ) { // skip this time if I2C is already active
			updateXGZP();
			didSomethingOnI2C = true;
			msSince_XGZPUpdate = 0;
		}
	}
#endif

#ifdef USE_UART_TMC
	// Update the Trinamic TMC2209 drivers if necessary
	if ( msSince_TMCUpdate > 1000 ) {
		set_microstep_code( &tmc_0, rxData.microsteps[0] );
		set_microstep_code( &tmc_1, rxData.microsteps[1] );
		set_microstep_code( &tmc_2, rxData.microsteps[2] );
		set_microstep_code( &tmc_3, rxData.microsteps[3] );

		// these take about 1ms each
		set_rms_current( &tmc_0, rxData.rmsCurrent[0] * 10 );
		set_rms_current( &tmc_1, rxData.rmsCurrent[1] * 10 );
		set_rms_current( &tmc_2, rxData.rmsCurrent[2] * 10 );
		set_rms_current( &tmc_3, rxData.rmsCurrent[3] * 10 );

		msSince_TMCUpdate = 0;
	}
#endif

}

