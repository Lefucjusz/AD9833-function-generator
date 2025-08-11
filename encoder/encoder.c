#include "encoder.h"
#include <gpio.h>
#include <string.h>

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
	volatile bool rotated;
	uint16_t last_count;
	encoder_rotation_callback_t rotation_callback;
	encoder_button_callback_t button_callback;
	uint32_t button_event_tick;
	encoder_button_state_t button_state;
} encoder_ctx_t;

static encoder_ctx_t ctx;

static bool encoder_is_button_pressed(void)
{
	return (GPIO_ReadInputDataBit(GPIO_ENC_BUTTON_PORT, GPIO_ENC_BUTTON_PIN) == Bit_RESET);
}

// static void encoder_rotation_update(void)
// {
// 	if (ctx.rotation_callback == NULL) {
// 		return;
// 	}

// 	if (ctx.rotated) {
// 		ctx.rotated = false;

// 		const uint16_t count = __HAL_TIM_GET_COUNTER(ENCODER_TIMER_HANDLE);
// 		const int16_t increment = count - ctx.last_count;
// 		const encoder_direction_t direction = (increment > 0) ? ENCODER_CW : ENCODER_CCW;

// 		ctx.rotation_callback(direction, count, increment);

// 		ctx.last_count = count;
// 	}
// }

static void encoder_button_update(void)
{
	switch (ctx.button_state) {
		case ENCODER_BUTTON_STATE_IDLE:
			if (encoder_is_button_pressed()) {
				ctx.button_event_tick = HAL_GetTick();
				ctx.button_state = ENCODER_BUTTON_STATE_DEBOUNCE;
			}
			break;
		case ENCODER_BUTTON_STATE_DEBOUNCE:
			if ((HAL_GetTick() - ctx.button_event_tick) >= ENCODER_BUTTON_DEBOUNCE_TIME_MS) {
				if (encoder_is_button_pressed()) {
					ctx.button_event_tick = HAL_GetTick();
					ctx.button_state = ENCODER_BUTTON_STATE_CLICKED;
				}
				else {
					ctx.button_state = ENCODER_BUTTON_STATE_IDLE;
				}
			}
			break;
		case ENCODER_BUTTON_STATE_CLICKED:
			if ((HAL_GetTick() - ctx.button_event_tick) >= ENCODER_BUTTON_HOLD_TIME_MS) {
				ctx.button_state = ENCODER_BUTTON_STATE_HELD;
			}
			else if (!encoder_is_button_pressed()) {
				ctx.button_state = ENCODER_BUTTON_STATE_RELEASED;
			}
			break;
		case ENCODER_BUTTON_STATE_HELD:
			if (ctx.button_callback != NULL) {
				ctx.button_callback(ENCODER_BUTTON_HOLD);
			}
			ctx.button_state = ENCODER_BUTTON_STATE_WAITING;
			break;
		case ENCODER_BUTTON_STATE_WAITING:
			if (!encoder_is_button_pressed()) {
				ctx.button_state = ENCODER_BUTTON_STATE_IDLE;
			}
			break;
		case ENCODER_BUTTON_STATE_RELEASED:
			if (ctx.button_callback != NULL) {
				ctx.button_callback(ENCODER_BUTTON_CLICK);
			}
			ctx.button_state = ENCODER_BUTTON_STATE_IDLE;
			break;
		default:
			break;
	}
}

int encoder_init(void)
{
	memset(&ctx, 0, sizeof(ctx));

	return 0; //HAL_TIM_Encoder_Start_IT(ENCODER_TIMER_HANDLE, TIM_CHANNEL_ALL);
}

void encoder_set_rotation_callback(encoder_rotation_callback_t callback)
{
	ctx.rotation_callback = callback;
}

void encoder_set_button_callback(encoder_button_callback_t callback)
{
	ctx.button_callback = callback;
}

bool encoder_button_is_idle(void)
{
	return (ctx.button_state == ENCODER_BUTTON_STATE_IDLE) && !encoder_is_button_pressed();
}

void encoder_task(void)
{
	// encoder_rotation_update();
	encoder_button_update();
}

// void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
// {
// 	ctx.rotated = true;
// }
