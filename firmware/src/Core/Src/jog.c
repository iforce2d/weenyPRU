#include "jog.h"

#include "main.h"
#include "comms.h"
#include "rotaryEncoder.h"

extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;

const float deadband = 0.25;

float applyDeadband(float raw) {
	float increment = (raw / 500.0f);
	if ( increment > 1 )
		increment = 1;
	else if ( increment < -1 )
		increment = -1;
	if ( increment > deadband )
		increment -= deadband;
	else if ( increment < -deadband )
		increment += deadband;
	else
		increment = 0;
	return increment * 25;
}

void doJogUpdate() {

	static uint16_t updateCount = 0;
	static float adc1Center = 2047;
	static float adc2Center = 2047;

	static float jogpos0 = 0;
	static float jogpos1 = 0;
	static float jogpos2 = 0;
	static float jogpos3 = 0;

	static int whichADC = 0;

	if ( whichADC == 0 ) {
		HAL_ADC_Start(&hadc1);
		HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
		uint16_t raw = HAL_ADC_GetValue(&hadc1); // range 0-4095
		if ( updateCount < 500 ) {
			adc1Center = 0.9f * adc1Center + raw * 0.1f;
		}
		else
			jogpos0 += applyDeadband(raw - adc1Center);

		txData.jogcounts[0] = jogpos0;
	}
	else {
		HAL_ADC_Start(&hadc2);
		HAL_ADC_PollForConversion(&hadc2, HAL_MAX_DELAY);
		uint16_t raw = HAL_ADC_GetValue(&hadc2); // range 0-4095
		if ( updateCount < 500 ) {
			adc2Center = 0.9f * adc2Center + raw * 0.1f;
			updateCount++;
		}
		else
			jogpos1 += applyDeadband(raw - adc2Center);

		txData.jogcounts[1] = jogpos1;
	}

	whichADC = 1 - whichADC;


	float encoderIncrement = rotaryEncoderPos;
	rotaryEncoderPos = 0;
	jogpos2 += encoderIncrement;

	txData.jogcounts[2] = jogpos2;
	txData.jogcounts[3] = jogpos3;

//	float buttonIncrement = 0;
//
//	int button1 = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_10);
//	int button2 = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_11);
//	if ( button1 ^ button2 ) {
//		if ( button1 )
//			buttonIncrement =  1;
//		else
//			buttonIncrement = -1;
//	}
//
//	int switchPos = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_4);
//	if ( switchPos) {
//		jogpos2 += buttonIncrement;
//		jogpos3 += encoderIncrement;
//	}
//	else {
//		jogpos2 += encoderIncrement;
//		jogpos3 += buttonIncrement;
//	}
//
//	txData.jogcounts[2] = jogpos2;
//	txData.jogcounts[3] = jogpos3;


}

