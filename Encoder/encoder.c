#include "encoder.h"
#include <tim.h>
#include <gpio.h>
#include <string.h>
#include <stdbool.h>

#define ENCODER_BUTTON_DEBOUNCE_TIME_MS 150

typedef struct
{
	encoder_rotation_callback_t rotation_callback;
	encoder_button_callback_t button_callback;
	volatile uint16_t count;
	uint16_t last_count;
	volatile bool button_pressed;
	uint32_t last_button_tick;
} encoder_ctx_t;

static encoder_ctx_t ctx;

HAL_StatusTypeDef encoder_init(void)
{
	memset(&ctx, 0, sizeof(ctx));

	return HAL_TIM_Encoder_Start_IT(&htim3, TIM_CHANNEL_ALL);
}

void encoder_set_rotation_callback(encoder_rotation_callback_t callback)
{
	ctx.rotation_callback = callback;
}

void encoder_set_button_callback(encoder_button_callback_t callback)
{
	ctx.button_callback = callback;
}

uint16_t encoder_get_count(void)
{
	return ctx.count;
}

void encoder_task(void)
{
	/* Handle encoder rotation */
	if (ctx.rotation_callback != NULL) {
		if (ctx.count != ctx.last_count) {
			const int16_t increment = ctx.count - ctx.last_count;
			const encoder_direction_t direction = (increment > 0) ? ENCODER_CW : ENCODER_CCW;

			ctx.rotation_callback(direction, ctx.count, increment);

			ctx.last_count = ctx.count;
		}
	}

	/* Handle encoder button */
	if (ctx.button_callback != NULL) {
		if (ctx.button_pressed) {
			ctx.button_callback();

			ctx.button_pressed = false;
		}
	}

}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
	ctx.count = __HAL_TIM_GET_COUNTER(htim);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	const uint32_t current_tick = HAL_GetTick();
	if ((current_tick - ctx.last_button_tick) < ENCODER_BUTTON_DEBOUNCE_TIME_MS) {
		return;
	}

	if (GPIO_Pin == ENC_BUTTON_Pin) {
		ctx.button_pressed = true;
	}

	ctx.last_button_tick = current_tick;
}
