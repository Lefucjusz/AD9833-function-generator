/*
 * hd44780_io.c
 *
 *  Created on: Feb 15, 2023
 *      Author: lefucjusz
 */

#include "hd44780_io.h"
#include <gpio.h>
#include <delay.h>

#define HD44780_US_PER_MS 1000

typedef struct
{
	GPIO_TypeDef *gpio;
	uint16_t gpio_pin;
	hd44780_pin_t display_pin;
} hd44780_gpio_map_t;

static const hd44780_gpio_map_t gpio_map[HD44780_PIN_NUM] =
{
		{.gpio = GPIO_LCD_DATA_PORT, .gpio_pin = GPIO_LCD_D4_PIN, .display_pin = HD44780_PIN_D4},
		{.gpio = GPIO_LCD_DATA_PORT, .gpio_pin = GPIO_LCD_D5_PIN, .display_pin = HD44780_PIN_D5},
		{.gpio = GPIO_LCD_DATA_PORT, .gpio_pin = GPIO_LCD_D6_PIN, .display_pin = HD44780_PIN_D6},
		{.gpio = GPIO_LCD_DATA_PORT, .gpio_pin = GPIO_LCD_D7_PIN, .display_pin = HD44780_PIN_D7},
		{.gpio = GPIO_LCD_CTRL_PORT, .gpio_pin = GPIO_LCD_RS_PIN, .display_pin = HD44780_PIN_RS},
		{.gpio = GPIO_LCD_CTRL_PORT, .gpio_pin = GPIO_LCD_E_PIN, .display_pin = HD44780_PIN_E}
};

static void hd44780_io_set_pin_state(hd44780_pin_t pin, hd44780_pin_state_t state)
{
	for (size_t i = 0; i < HD44780_PIN_NUM; ++i) {
		if (gpio_map[i].display_pin == pin) {
			GPIO_WriteBit(gpio_map[i].gpio, gpio_map[i].gpio_pin, (BitAction)state);
			break;
		}
	}
}

static void hd44780_io_delay_us(uint16_t us)
{
	const uint16_t ms = (us >= HD44780_US_PER_MS) ? (us / HD44780_US_PER_MS) : 1;

	delay_ms(ms);
}

hd44780_io_t *hd44780_io_get(void)
{
	static hd44780_io_t io = {
			.set_pin_state = hd44780_io_set_pin_state,
			.delay_us = hd44780_io_delay_us
	};

	return &io;
}
