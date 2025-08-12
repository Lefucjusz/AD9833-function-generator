#include "gpio.h"
#include <ch32v00x.h>

void gpio_init(void)
{
    GPIO_InitTypeDef gpio_cfg = {0};
    EXTI_InitTypeDef exti_cfg = {0};
    NVIC_InitTypeDef nvic_cfg = {0};

    /* Enable clock for all used ports */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD | RCC_APB2Periph_AFIO, ENABLE);   

    /* Configure LCD GPIO */
    gpio_cfg.GPIO_Pin = GPIO_LCD_RS_PIN | GPIO_LCD_E_PIN | GPIO_LCD_D4_PIN |
                        GPIO_LCD_D5_PIN | GPIO_LCD_D6_PIN | GPIO_LCD_D7_PIN;
    gpio_cfg.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio_cfg.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_Init(GPIO_LCD_PORT, &gpio_cfg);

    /* Configure encoder button GPIO */
    gpio_cfg.GPIO_Pin = GPIO_ENC_BUTTON_PIN;
    gpio_cfg.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIO_ENC_BUTTON_PORT, &gpio_cfg);

    /* Configure encoder rotation GPIO */
    gpio_cfg.GPIO_Pin = GPIO_ENC_PHA_PIN | GPIO_ENC_PHB_PIN;
    gpio_cfg.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIO_ENC_ROT_PORT, &gpio_cfg);

    /* Configure I2C GPIO */
    gpio_cfg.GPIO_Pin = GPIO_I2C_SDA_PIN | GPIO_I2C_SCL_PIN;
    gpio_cfg.GPIO_Mode = GPIO_Mode_AF_OD;
    gpio_cfg.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_Init(GPIO_I2C_PORT, &gpio_cfg);

    /* Configure EXTI for encoder rotation */
    GPIO_EXTILineConfig(GPIO_ENC_ROT_PORT_SOURCE, GPIO_ENC_PHA_PIN_SOURCE);
    GPIO_EXTILineConfig(GPIO_ENC_ROT_PORT_SOURCE, GPIO_ENC_PHB_PIN_SOURCE);
    exti_cfg.EXTI_Line =  GPIO_ENC_PHA_PIN | GPIO_ENC_PHB_PIN;
    exti_cfg.EXTI_Mode = EXTI_Mode_Interrupt;
    exti_cfg.EXTI_Trigger = EXTI_Trigger_Falling;
    exti_cfg.EXTI_LineCmd = ENABLE;
    EXTI_Init(&exti_cfg);

    /* Configure NVIC */
    nvic_cfg.NVIC_IRQChannel = EXTI7_0_IRQn;
    nvic_cfg.NVIC_IRQChannelPreemptionPriority = 1;
    nvic_cfg.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic_cfg);
}
