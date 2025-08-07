#pragma once

#include <stm32f1xx_hal.h>

typedef enum
{
	ENCODER_CW,
	ENCODER_CCW
} encoder_direction_t;

typedef void (*encoder_rotation_callback_t)(encoder_direction_t direction, uint16_t count, int16_t increment);
typedef void (*encoder_button_callback_t)(void);

HAL_StatusTypeDef encoder_init(void);

void encoder_set_rotation_callback(encoder_rotation_callback_t callback);
void encoder_set_button_callback(encoder_button_callback_t callback);

uint16_t encoder_get_count(void);

void encoder_task(void);
