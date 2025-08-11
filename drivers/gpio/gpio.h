#pragma once
 
#include <ch32v00x.h>

#define GPIO_LCD_PORT GPIOC
#define GPIO_LCD_RS_PIN GPIO_Pin_5
#define GPIO_LCD_E_PIN GPIO_Pin_4
#define GPIO_LCD_D4_PIN GPIO_Pin_3
#define GPIO_LCD_D5_PIN GPIO_Pin_2
#define GPIO_LCD_D6_PIN GPIO_Pin_1
#define GPIO_LCD_D7_PIN GPIO_Pin_0

#define GPIO_USER_LED_PIN GPIO_Pin_7 // TODO remove this

#define GPIO_ENC_BUTTON_PORT GPIOC
#define GPIO_ENC_BUTTON_PIN GPIO_Pin_6

void gpio_init(void);
 