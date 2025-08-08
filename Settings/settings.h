#pragma once

#include <stm32f1xx_hal.h>

HAL_StatusTypeDef settings_init(void);

HAL_StatusTypeDef settings_write(uint32_t value, size_t address);
HAL_StatusTypeDef settings_read(uint32_t *value, size_t address);
