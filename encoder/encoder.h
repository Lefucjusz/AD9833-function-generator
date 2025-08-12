#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
	ENCODER_CW,
	ENCODER_CCW
} encoder_direction_t;

typedef enum
{
	ENCODER_BUTTON_CLICK,
	ENCODER_BUTTON_HOLD
} encoder_button_action_t;

typedef void (*encoder_rotation_callback_t)(encoder_direction_t direction, uint32_t count, int32_t increment);
typedef void (*encoder_button_callback_t)(encoder_button_action_t type);

void encoder_init(void);

void encoder_set_rotation_callback(encoder_rotation_callback_t callback);
void encoder_set_button_callback(encoder_button_callback_t callback);

bool encoder_button_is_idle(void);

void encoder_task(void);

void EXTI7_0_IRQHandler(void) __attribute__((interrupt));
