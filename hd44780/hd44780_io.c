/*
 * hd44780_io.c
 *
 *  Created on: Feb 15, 2023
 *      Author: lefucjusz
 */

#include "hd44780_io.h"
#include <gpio.h>
#include <delay.h>

#define HD44780_GPIO_PORT GPIO_LCD_PORT

typedef struct
{
	uint16_t gpio_pin;
	hd44780_pin_t display_pin;
} hd44780_gpio_map_t;

static const hd44780_gpio_map_t gpio_map[HD44780_PIN_NUM] =
{
		{.gpio_pin = GPIO_LCD_D4_PIN, .display_pin = HD44780_PIN_D4},
		{.gpio_pin = GPIO_LCD_D5_PIN, .display_pin = HD44780_PIN_D5},
		{.gpio_pin = GPIO_LCD_D6_PIN, .display_pin = HD44780_PIN_D6},
		{.gpio_pin = GPIO_LCD_D7_PIN, .display_pin = HD44780_PIN_D7},
		{.gpio_pin = GPIO_LCD_RS_PIN, .display_pin = HD44780_PIN_RS},
		{.gpio_pin = GPIO_LCD_E_PIN, .display_pin = HD44780_PIN_E}
};

static void set_pin_state(hd44780_pin_t pin, hd44780_pin_state_t state)
{
	for (size_t i = 0; i < HD44780_PIN_NUM; ++i) {
		if (gpio_map[i].display_pin == pin) {
			GPIO_WriteBit(HD44780_GPIO_PORT, gpio_map[i].gpio_pin, (BitAction)state);
			break;
		}
	}
}

static void delay_us(uint16_t us)
{
	if (us < 1000) {
		delay_us(1);
	}
	else {
		delay_ms(us / 1000); // TODO cleanup
	}
}

hd44780_io_t *hd44780_io_get(void)
{
	static hd44780_io_t io = {
			.set_pin_state = set_pin_state,
			.delay_us = delay_us
	};

	return &io;
}
