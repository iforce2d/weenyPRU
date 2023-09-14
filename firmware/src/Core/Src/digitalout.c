
#include "digitalout.h"
#include "comms.h"

void initDigitalOut(
		DigitalOut* dg,
		int number,
		GPIO_TypeDef* port,
		uint16_t pin)
{
	dg->mask = 1 << number;
	dg->port = port;
	dg->pin = pin;
}

void updateDigitalOut(DigitalOut* dg)
{
	if ( rxData.outputs & dg->mask )
		HAL_GPIO_WritePin(dg->port, dg->pin, GPIO_PIN_SET);
	else
		HAL_GPIO_WritePin(dg->port, dg->pin, GPIO_PIN_RESET);
}


