#pragma once

#include <stm32f1xx_hal.h>

typedef enum
{
	SETTINGS_FREQUENCY = 0,
	SETTINGS_AMPLITUDE,
	SETTINGS_WAVEFORM
} settings_entry_t;

HAL_StatusTypeDef settings_init(void);

HAL_StatusTypeDef settings_write(uint32_t value, settings_entry_t entry);
HAL_StatusTypeDef settings_read(uint32_t *value, settings_entry_t entry);
