#pragma once

#include <stdint.h>

void delay_init(void);

void delay_ms(uint32_t ms);
uint32_t delay_get_ticks(void);

void SysTick_Handler(void) __attribute__((interrupt));
