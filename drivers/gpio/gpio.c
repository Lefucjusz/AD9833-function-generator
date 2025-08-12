#include "gpio.h"
#include <ch32v00x.h>

void gpio_init(void)
{
    GPIO_InitTypeDef gpio_cfg = {0};
    EXTI_InitTypeDef exti_cfg = {0};
    NVIC_InitTypeDef nvic_cfg = {0};

    /* Enable PD7 as GPIO pin */
    FLASH_Unlock();
    FLASH_EraseOptionBytes();
    FLASH_UserOptionByteConfig(OB_IWDG_SW, OB_STOP_NoRST, OB_STDBY_NoRST, OB_RST_NoEN);
    FLASH_Lock();

    /* Enable clock for all used ports */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD | RCC_APB2Periph_AFIO, ENABLE);

    /* Configure LCD data GPIO */
    gpio_cfg.GPIO_Pin = GPIO_LCD_D4_PIN | GPIO_LCD_D5_PIN | GPIO_LCD_D6_PIN | GPIO_LCD_D7_PIN;
    gpio_cfg.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio_cfg.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_Init(GPIO_LCD_DATA_PORT, &gpio_cfg);

    /* Configure LCD control GPIO */
    gpio_cfg.GPIO_Pin = GPIO_LCD_RS_PIN | GPIO_LCD_E_PIN;
    gpio_cfg.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIO_LCD_CTRL_PORT, &gpio_cfg);

    /* Configure encoder GPIO */
    gpio_cfg.GPIO_Pin = GPIO_ENC_PHA_PIN | GPIO_ENC_PHB_PIN | GPIO_ENC_BUTTON_PIN;
    gpio_cfg.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIO_ENC_PORT, &gpio_cfg);

    /* Configure I2C GPIO */
    gpio_cfg.GPIO_Pin = GPIO_I2C_SDA_PIN | GPIO_I2C_SCL_PIN;
    gpio_cfg.GPIO_Mode = GPIO_Mode_AF_OD;
    gpio_cfg.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_Init(GPIO_I2C_PORT, &gpio_cfg);

    /* Configure SPI GPIO */
    gpio_cfg.GPIO_Pin = GPIO_SPI_MOSI_PIN | GPIO_SPI_SCK_PIN;
    gpio_cfg.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio_cfg.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_Init(GPIO_SPI_PORT, &gpio_cfg);

    /* Configure chip selects */
    gpio_cfg.GPIO_Pin = GPIO_SPI_DDS_CS_PIN | GPIO_SPI_PGA_CS_PIN;
    gpio_cfg.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio_cfg.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_Init(GPIO_SPI_PORT, &gpio_cfg);
    GPIO_WriteBit(GPIO_SPI_PORT, GPIO_SPI_PGA_CS_PIN, Bit_SET);
    GPIO_WriteBit(GPIO_SPI_PORT, GPIO_SPI_DDS_CS_PIN, Bit_SET);

    /* Configure EXTI for encoder inputs */
    GPIO_EXTILineConfig(GPIO_ENC_PORT_SOURCE, GPIO_ENC_BUTTON_PIN_SOURCE);
    GPIO_EXTILineConfig(GPIO_ENC_PORT_SOURCE, GPIO_ENC_PHA_PIN_SOURCE);
    GPIO_EXTILineConfig(GPIO_ENC_PORT_SOURCE, GPIO_ENC_PHB_PIN_SOURCE);
    exti_cfg.EXTI_Line = GPIO_ENC_PHA_PIN | GPIO_ENC_PHB_PIN | GPIO_ENC_BUTTON_PIN;
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
