#include "encoder.h"
#include <tim.h>
#include <gpio.h>
#include <string.h>
#include <stdbool.h>

#define ENCODER_TIMER_HANDLE &htim3

#define ENCODER_BUTTON_DEBOUNCE_TIME_MS 100
#define ENCODER_BUTTON_HOLD_TIME_MS 750

typedef enum
{
	ENCODER_BUTTON_STATE_IDLE = 0,
	ENCODER_BUTTON_STATE_DEBOUNCE,
	ENCODER_BUTTON_STATE_CLICKED,
	ENCODER_BUTTON_STATE_HELD,
	ENCODER_BUTTON_STATE_WAITING, // Wait for release after hold to avoid repeating the hold action
	ENCODER_BUTTON_STATE_RELEASED
} encoder_button_state_t;

typedef struct
{
	GPIO_TypeDef *gpio;
	uint16_t pin;
	encoder_button_callback_t callback;
	uint32_t event_tick;
	encoder_button_state_t state;
} encoder_button_t;

typedef struct
{
	volatile bool rotated;
	uint16_t last_count;
	encoder_rotation_callback_t rotation_callback;
	encoder_button_t encoder_button;
} encoder_ctx_t;

static encoder_ctx_t ctx;

static void encoder_rotation_update(void)
{
	if (ctx.rotation_callback == NULL) {
		return;
	}

	if (ctx.rotated) {
		ctx.rotated = false;

		const uint16_t count = __HAL_TIM_GET_COUNTER(ENCODER_TIMER_HANDLE);
		const int16_t increment = count - ctx.last_count;
		const encoder_direction_t direction = (increment > 0) ? ENCODER_CW : ENCODER_CCW;

		ctx.rotation_callback(direction, count, increment);

		ctx.last_count = count;
	}
}

static void encoder_button_update(encoder_button_t *button)
{
	const bool button_pressed = (HAL_GPIO_ReadPin(button->gpio, button->pin) == GPIO_PIN_RESET);

	switch (button->state) {
		case ENCODER_BUTTON_STATE_IDLE:
			if (button_pressed) {
				button->event_tick = HAL_GetTick();
				button->state = ENCODER_BUTTON_STATE_DEBOUNCE;
			}
			break;
		case ENCODER_BUTTON_STATE_DEBOUNCE:
			if ((HAL_GetTick() - button->event_tick) >= ENCODER_BUTTON_DEBOUNCE_TIME_MS) {
				if (button_pressed) {
					button->event_tick = HAL_GetTick();
					button->state = ENCODER_BUTTON_STATE_CLICKED;
				}
				else {
					button->state = ENCODER_BUTTON_STATE_IDLE;
				}
			}
			break;
		case ENCODER_BUTTON_STATE_CLICKED:
			if ((HAL_GetTick() - button->event_tick) >= ENCODER_BUTTON_HOLD_TIME_MS) {
				button->state = ENCODER_BUTTON_STATE_HELD;
			}
			else if (!button_pressed) {
				button->state = ENCODER_BUTTON_STATE_RELEASED;
			}
			break;
		case ENCODER_BUTTON_STATE_HELD:
			if (button->callback != NULL) {
				button->callback(ENCODER_BUTTON_HOLD);
			}
			button->state = ENCODER_BUTTON_STATE_WAITING;
			break;
		case ENCODER_BUTTON_STATE_WAITING:
			if (!button_pressed) {
				button->state = ENCODER_BUTTON_STATE_IDLE;
			}
			break;
		case ENCODER_BUTTON_STATE_RELEASED:
			if (button->callback != NULL) {
				button->callback(ENCODER_BUTTON_CLICK);
			}
			button->state = ENCODER_BUTTON_STATE_IDLE;
			break;
		default:
			break;
	}
}

HAL_StatusTypeDef encoder_init(void)
{
	memset(&ctx, 0, sizeof(ctx));

	ctx.encoder_button.gpio = ENC_BUTTON_GPIO_Port;
	ctx.encoder_button.pin = ENC_BUTTON_Pin;

	return HAL_TIM_Encoder_Start_IT(ENCODER_TIMER_HANDLE, TIM_CHANNEL_ALL);
}

void encoder_set_rotation_callback(encoder_rotation_callback_t callback)
{
	ctx.rotation_callback = callback;
}

void encoder_set_button_callback(encoder_button_callback_t callback)
{
	ctx.encoder_button.callback = callback;
}

void encoder_task(void)
{
	encoder_rotation_update();
	encoder_button_update(&ctx.encoder_button);
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
	ctx.rotated = true;
}
