
#include "main.h"
#include "tasks.h"
#include "comms.h"
#include "led.h"
#include "rotaryEncoder.h"
#include "rgbled.h"

extern TIM_HandleTypeDef htim1; // spindle PWM
extern TIM_HandleTypeDef htim2; // timer compare used to reset step pins after step, all 4 channels
extern TIM_HandleTypeDef htim3; // servo thread 1kHz
extern TIM_HandleTypeDef htim4; // base thread 50kHz

void threadsStart() {
	HAL_TIM_Base_Start_IT(&htim1);
	HAL_TIM_Base_Start_IT(&htim3);
	HAL_TIM_Base_Start_IT(&htim4);

	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);

	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
}

void doSetup() {

	HAL_NVIC_SetPriority(TIM4_IRQn, 0, 0); // base thread

	HAL_NVIC_SetPriority(TIM3_IRQn, 1, 0); // servo thread
	HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 0, 0); // comms rx
	HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 0, 0); // comms tx

	HAL_NVIC_SetPriority(EXTI15_10_IRQn, 1, 0); // rotary encoder

	HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 2, 0); // RGB LEDs

	LED_OFF;
	initRotaryEncoder();
	initRGBLED();
#ifdef USE_I2C_DS3502
	initDS3502();
#endif
	setupTasks();
	commsStart();
	threadsStart();
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim == &htim3 ) {// servo thread
		doServoThreadTasks();
	}
	else if (htim == &htim4 ) {// base thread
		doBaseThreadTasks();
	}
}

