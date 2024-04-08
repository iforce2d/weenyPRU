#include "i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

// PB8     ------> I2C1_SCL
// PB9     ------> I2C1_SDA

#define I2C2_SCL_PORT		GPIOB
#define I2C2_SDA_PORT		GPIOB
#define I2C2_SCL_PIN		GPIO_PIN_8
#define I2C2_SDA_PIN		GPIO_PIN_9

void I2C2_ClearBusyFlag(I2C_HandleTypeDef *instance)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    int timeout =100;
    int timeout_cnt=0;

    // 1. Clear PE bit.
    instance->Instance->CR1 &= ~(0x0001);

    //  2. Configure the SCL and SDA I/Os as General Purpose Output Open-Drain, High level (Write 1 to GPIOx_ODR).
    GPIO_InitStruct.Mode         = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull         = GPIO_PULLUP;
    GPIO_InitStruct.Speed        = GPIO_SPEED_FREQ_HIGH;

    GPIO_InitStruct.Pin          = I2C2_SCL_PIN;
    HAL_GPIO_Init(I2C2_SCL_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(I2C2_SCL_PORT, I2C2_SCL_PIN, GPIO_PIN_SET);

    GPIO_InitStruct.Pin          = I2C2_SDA_PIN;
    HAL_GPIO_Init(I2C2_SDA_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(I2C2_SDA_PORT, I2C2_SDA_PIN, GPIO_PIN_SET);


    // 3. Check SCL and SDA High level in GPIOx_IDR.
    while (GPIO_PIN_SET != HAL_GPIO_ReadPin(I2C2_SCL_PORT, I2C2_SCL_PIN))
    {
        timeout_cnt++;
        if(timeout_cnt>timeout)
            return;
    }

    while (GPIO_PIN_SET != HAL_GPIO_ReadPin(I2C2_SDA_PORT, I2C2_SDA_PIN))
    {
        //Move clock to release I2C
        HAL_GPIO_WritePin(I2C2_SCL_PORT, I2C2_SCL_PIN, GPIO_PIN_RESET);
        for (int i = 0; i < 100; i++)
        	asm("nop");
        HAL_GPIO_WritePin(I2C2_SCL_PORT, I2C2_SCL_PIN, GPIO_PIN_SET);

        timeout_cnt++;
        if(timeout_cnt>timeout)
            return;
    }

    // 4. Configure the SDA I/O as General Purpose Output Open-Drain, Low level (Write 0 to GPIOx_ODR).
    HAL_GPIO_WritePin(I2C2_SDA_PORT, I2C2_SDA_PIN, GPIO_PIN_RESET);

    //  5. Check SDA Low level in GPIOx_IDR.
    while (GPIO_PIN_RESET != HAL_GPIO_ReadPin(I2C2_SDA_PORT, I2C2_SDA_PIN))
    {
        timeout_cnt++;
        if(timeout_cnt>timeout)
            return;
    }

    // 6. Configure the SCL I/O as General Purpose Output Open-Drain, Low level (Write 0 to GPIOx_ODR).
    HAL_GPIO_WritePin(I2C2_SCL_PORT, I2C2_SCL_PIN, GPIO_PIN_RESET);

    //  7. Check SCL Low level in GPIOx_IDR.
    while (GPIO_PIN_RESET != HAL_GPIO_ReadPin(I2C2_SCL_PORT, I2C2_SCL_PIN))
    {
        timeout_cnt++;
        if(timeout_cnt>timeout)
            return;
    }

    // 8. Configure the SCL I/O as General Purpose Output Open-Drain, High level (Write 1 to GPIOx_ODR).
    HAL_GPIO_WritePin(I2C2_SCL_PORT, I2C2_SCL_PIN, GPIO_PIN_SET);

    // 9. Check SCL High level in GPIOx_IDR.
    while (GPIO_PIN_SET != HAL_GPIO_ReadPin(I2C2_SCL_PORT, I2C2_SCL_PIN))
    {
        timeout_cnt++;
        if(timeout_cnt>timeout)
            return;
    }

    // 10. Configure the SDA I/O as General Purpose Output Open-Drain , High level (Write 1 to GPIOx_ODR).
    HAL_GPIO_WritePin(I2C2_SDA_PORT, I2C2_SDA_PIN, GPIO_PIN_SET);

    // 11. Check SDA High level in GPIOx_IDR.
    while (GPIO_PIN_SET != HAL_GPIO_ReadPin(I2C2_SDA_PORT, I2C2_SDA_PIN))
    {
        timeout_cnt++;
        if(timeout_cnt>timeout)
            return;
    }


    // 12. Configure the SCL and SDA I/Os as Alternate function Open-Drain.
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    GPIO_InitStruct.Pin = I2C2_SCL_PIN;
    HAL_GPIO_Init(I2C2_SCL_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = I2C2_SDA_PIN;
    HAL_GPIO_Init(I2C2_SDA_PORT, &GPIO_InitStruct);

    HAL_GPIO_WritePin(I2C2_SCL_PORT, I2C2_SCL_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(I2C2_SDA_PORT, I2C2_SDA_PIN, GPIO_PIN_SET);

    // 13. Set SWRST bit in I2Cx_CR1 register.
    instance->Instance->CR1 |= 0x8000;

    asm("nop");

    // 14. Clear SWRST bit in I2Cx_CR1 register.
    instance->Instance->CR1 &= ~0x8000;

    asm("nop");

    // 15. Enable the I2C peripheral by setting the PE bit in I2Cx_CR1 register
    instance->Instance->CR1 |= 0x0001;

    // Call initialization function.
    if (HAL_I2C_Init(instance) != HAL_OK)
    {
      Error_Handler();
    }

}



#ifdef __cplusplus
}
#endif




