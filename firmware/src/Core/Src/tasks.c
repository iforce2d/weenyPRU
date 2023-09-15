
#include "tasks.h"
#include "comms.h"
#include "digitalout.h"
#include "stepgen.h"
#include "led.h"
#include "rgbled.h"
#include "jog.h"

extern TIM_HandleTypeDef htim1;

bool haveComms = false;

Stepgen sg_x;
Stepgen sg_y;
Stepgen sg_z;
Stepgen sg_a;

void setupTasks() {
	initStepgen(&sg_a, 0, GPIOA, GPIO_PIN_4, TIM_CHANNEL_1); // A: dir on PA4, step on PA0
	initStepgen(&sg_z, 1, GPIOA, GPIO_PIN_5, TIM_CHANNEL_2); // Z: dir on PA5, step on PA1
	initStepgen(&sg_y, 2, GPIOA, GPIO_PIN_6, TIM_CHANNEL_3); // Y: dir on PA6, step on PA2
	initStepgen(&sg_x, 3, GPIOA, GPIO_PIN_7, TIM_CHANNEL_4); // X: dir on PA7, step on PA3
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

int servoThreadCount = 0;

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
	prepStep(&sg_x);
	prepStep(&sg_y);
	prepStep(&sg_z);
	prepStep(&sg_a);

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

	int mod = servoThreadCount % 20; // about 50Hz

	if ( mod == 0 ) {
		if ( haveComms ) {
			updateRGBLEDs();
		}
		else {
			turnOffAllRGBLEDs();
		}
		doRGBLEDOutput();
	}
	else if ( mod == 10 ) {
		int pwmCompare = (rxData.spindleSpeed / 65535.0f) * 3600;
		__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, pwmCompare);
	}
}

