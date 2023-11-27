
#include "tasks.h"
#include "comms.h"
#include "digitalout.h"
#include "stepgen.h"
#include "led.h"
#include "rgbled.h"
#include "jog.h"
#include "TMC2209.h"

extern TIM_HandleTypeDef htim1;

bool haveComms = false;

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
	initStepgen(&sg_0, 0, GPIOA, GPIO_PIN_4, TIM_CHANNEL_1); // A: dir on PA4, step on PA0
	initStepgen(&sg_1, 1, GPIOA, GPIO_PIN_5, TIM_CHANNEL_2); // Z: dir on PA5, step on PA1
	initStepgen(&sg_2, 2, GPIOA, GPIO_PIN_6, TIM_CHANNEL_3); // Y: dir on PA6, step on PA2
	initStepgen(&sg_3, 3, GPIOA, GPIO_PIN_7, TIM_CHANNEL_4); // X: dir on PA7, step on PA3

	HAL_Delay(15); // TMCs take a while to wake up?

	initTMCUART(&tmc_0, 0x00, TMC_DEFAULT_MICROSTEPS, TMC_DEFAULT_CURRENT);
	initTMCUART(&tmc_1, 0x01, TMC_DEFAULT_MICROSTEPS, TMC_DEFAULT_CURRENT);
	initTMCUART(&tmc_2, 0x02, TMC_DEFAULT_MICROSTEPS, TMC_DEFAULT_CURRENT);
	initTMCUART(&tmc_3, 0x03, TMC_DEFAULT_MICROSTEPS, TMC_DEFAULT_CURRENT);

}

typedef struct PortAndPin {
	GPIO_TypeDef* port;
	uint16_t pin;
} PortAndPin;

PortAndPin digitalIns[] = {
	{ GPIOB, GPIO_PIN_2  }, // D1
	{ GPIOB, GPIO_PIN_10 }, // D2
	{ GPIOB, GPIO_PIN_11 }, // D3
	{ GPIOA, GPIO_PIN_11 }, // D4
	{ GPIOA, GPIO_PIN_9  }, // D5
	{ GPIOA, GPIO_PIN_10 }, // D6
	{ GPIOA, GPIO_PIN_12 }, // D7
};

PortAndPin digitalOuts[] = {
	{ GPIOA, GPIO_PIN_15 }, // D8
	{ GPIOB, GPIO_PIN_3  }, // D9
	{ GPIOB, GPIO_PIN_4  }, // D10
	{ GPIOB, GPIO_PIN_6  }, // D11
	{ GPIOB, GPIO_PIN_7  }, // D12
	{ GPIOB, GPIO_PIN_8  }, // D13
	{ GPIOB, GPIO_PIN_9  }, // D14
};

uint32_t servoThreadCount = 0;

void doServoThreadTasks() { // 1000Hz interrupt, return asap

	static int noCommsCount = 0;

	servoThreadCount++;

	if ( packetCount < 1 )
		noCommsCount++;
	else
		noCommsCount = 0;

	haveComms = noCommsCount < 10; // allow missing up to 10 expected packets

	if ( haveComms ) {
		LED_ON;
	}
	else {
		rxData.jointEnable = 0;
		rxData.outputs = 0;
		LED_OFF;
	}

	packetCount = 0;

	if ( servoThreadCount % 10 == 0 ) {// 100Hz
		doJogUpdate();
	}

	for (int i = 0; i < (sizeof(digitalOuts)/sizeof(PortAndPin)); i++) {
		PortAndPin pap = digitalOuts[i];
		HAL_GPIO_WritePin(pap.port, pap.pin, (rxData.outputs & (1 << i)) ? GPIO_PIN_SET : GPIO_PIN_RESET);
	}
}

void doBaseThreadTasks() { // 50kHz interrupt, no screwing around here!
	prepStep(&sg_0);
	prepStep(&sg_1);
	prepStep(&sg_2);
	prepStep(&sg_3);

	TIM2->CNT = 0;
}

void updateRGBLEDs() {

	uint8_t brightness = 100;
	uint8_t rgb[3];

    for (int i = 0; i < 48; i++)
    {
        int bytePos = i / 8;
        int bitPos = i % 8;
    	int ledPos = i / 3;
    	int colorPos = i % 3;

    	if ( rxData.rgb[bytePos] & (1 << bitPos))
        	rgb[colorPos] = brightness;
        else
        	rgb[colorPos] = 0;

        if ( colorPos == 2 )
        	setRGBLedColor(ledPos, rgb[0], rgb[1], rgb[2]);
    }
}

void doMainLoopTasks() {

	// servoThreadCount is a value that will change every 1ms in the servo thread. Make
	// use of that to read digital inputs at a rate that is no faster than necessary.

	static int lastDigitalInputRead = 0;

	if ( lastDigitalInputRead != servoThreadCount ) {

		for (int i = 0; i < (sizeof(digitalIns)/sizeof(PortAndPin)); i++) {
			PortAndPin pap = digitalIns[i];
			int digitalInput = HAL_GPIO_ReadPin(pap.port, pap.pin);
			if ( digitalInput ) // high means not pressed
				txData.inputs &= ~(uint8_t)(1 << i);
			else
				txData.inputs |= (uint8_t)(1 << i);
		}

		lastDigitalInputRead = servoThreadCount;
	}



	// Here we update the spindle PWM and the status LEDs. These tasks don't need to be done super fast, so spread
	// them out such that one task is done about every 20ms (for two tasks, about 40ms between updates of each task)

	static int lastSlowUpdatesTime = 0;

	if ( lastSlowUpdatesTime != servoThreadCount ) {
		int counter50Hz = servoThreadCount % 20; // 50Hz

		if ( counter50Hz == 0 ) {
			if ( haveComms ) {
				updateRGBLEDs();
			}
			else {
				turnOffAllRGBLEDs();
			}
			doRGBLEDOutput();
		}
		else if ( counter50Hz == 10 ) {
			int pwmCompare = (rxData.spindleSpeed / 65535.0f) * 3600;
			__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, pwmCompare);
		}

		lastSlowUpdatesTime = servoThreadCount;
	}



	// Update the Trinamic TMC2209 drivers if necessary

	static int lastTMCUpdatesTime = 0;

	if ( lastTMCUpdatesTime != servoThreadCount ) { // make sure we don't do this multiple times inside one servo thread loop

		if ( servoThreadCount % 1000 == 0 ) { // 1Hz

			set_microstep_code( &tmc_0, rxData.microsteps[0] );
			set_microstep_code( &tmc_1, rxData.microsteps[1] );
			set_microstep_code( &tmc_2, rxData.microsteps[2] );
			set_microstep_code( &tmc_3, rxData.microsteps[3] );

			// these take about 1ms each
			set_rms_current( &tmc_0, rxData.rmsCurrent[0] * 10 );
			set_rms_current( &tmc_1, rxData.rmsCurrent[1] * 10 );
			set_rms_current( &tmc_2, rxData.rmsCurrent[2] * 10 );
			set_rms_current( &tmc_3, rxData.rmsCurrent[3] * 10 );
		}

		lastTMCUpdatesTime = servoThreadCount;
	}
}

