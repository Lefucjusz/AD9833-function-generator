#pragma once
 
#include <ch32v00x.h>

#define GPIO_LCD_PORT GPIOD
#define GPIO_LCD_RS_PIN GPIO_Pin_0
#define GPIO_LCD_E_PIN GPIO_Pin_2
#define GPIO_LCD_D4_PIN GPIO_Pin_3
#define GPIO_LCD_D5_PIN GPIO_Pin_4
#define GPIO_LCD_D6_PIN GPIO_Pin_5
#define GPIO_LCD_D7_PIN GPIO_Pin_6

#define GPIO_ENC_PORT GPIOC
#define GPIO_ENC_PORT_SOURCE GPIO_PortSourceGPIOC
#define GPIO_ENC_BUTTON_PIN GPIO_Pin_1
#define GPIO_ENC_PHA_PIN GPIO_Pin_2
#define GPIO_ENC_PHA_PIN_SOURCE GPIO_PinSource2
#define GPIO_ENC_PHB_PIN GPIO_Pin_3
#define GPIO_ENC_PHB_PIN_SOURCE GPIO_PinSource3


void gpio_init(void);
 