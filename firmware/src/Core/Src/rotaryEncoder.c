
#include "main.h"
#include "rotaryEncoder.h"
#include "comms.h"

int rotaryEncoderPrevCLK = 0;
int32_t rotaryEncoderPos = 0; // −32768, +32767

void initRotaryEncoder() {

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOC_CLK_ENABLE();

  GPIO_InitStruct.Pin = GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  rotaryEncoderPos = 0;
}

void EXTI15_10_IRQHandler(void)
{
	__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_14|GPIO_PIN_15);

	int clk = HAL_GPIO_ReadPin( GPIOC, GPIO_PIN_14 );
	int dt = HAL_GPIO_ReadPin( GPIOC, GPIO_PIN_15 );
	if ( clk != rotaryEncoderPrevCLK ) {
		if ( dt == clk ) {
			rotaryEncoderPos--;
		}
		else {
			rotaryEncoderPos++;
		}
	}
	rotaryEncoderPrevCLK = clk;
}


