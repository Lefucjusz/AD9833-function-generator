#include "encoder.h"
#include <gpio.h>
#include <delay.h>
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
	volatile bool enc_cw_fall;
	volatile bool enc_ccw_fall;
	volatile uint32_t enc_count;
	uint32_t enc_last_count;
	volatile bool enc_rotated;
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

static void encoder_rotation_isr(uint32_t irq_pin)
{
	const uint8_t pha_state = GPIO_ReadInputDataBit(GPIO_ENC_ROT_PORT, GPIO_ENC_PHA_PIN);
	const uint8_t phb_state = GPIO_ReadInputDataBit(GPIO_ENC_ROT_PORT, GPIO_ENC_PHB_PIN);
	const uint8_t gpio_state = (phb_state << 1) | (pha_state << 0);

	if (irq_pin == GPIO_ENC_PHA_PIN) {
        if (!ctx.enc_cw_fall && (gpio_state == 0b10)) {
            ctx.enc_cw_fall = true;
        }

        if (ctx.enc_ccw_fall && (gpio_state == 0b00)) {
            ctx.enc_cw_fall = false;
            ctx.enc_ccw_fall = false;
            --ctx.enc_count;
            ctx.enc_rotated = true;
        }
    }
    else {
        if (!ctx.enc_ccw_fall && (gpio_state == 0b01)) {
            ctx.enc_ccw_fall = true;
        }

        if (ctx.enc_cw_fall && (gpio_state == 0b00)) {
            ctx.enc_cw_fall = false;
            ctx.enc_ccw_fall = false;
            ++ctx.enc_count;
            ctx.enc_rotated = true;
        }
    }
}

static void encoder_rotation_update(void)
{
	if (ctx.rotation_callback == NULL) {
		return;
	}

	if (ctx.enc_rotated) {
		ctx.enc_rotated = false;

		const uint32_t enc_count = ctx.enc_count;
		const int32_t increment = enc_count - ctx.enc_last_count;
		const encoder_direction_t direction = (increment > 0) ? ENCODER_CW : ENCODER_CCW;

		ctx.rotation_callback(direction, enc_count, increment);

		ctx.enc_last_count = enc_count;
	}
}

static void encoder_button_update(void)
{
	switch (ctx.button_state) {
		case ENCODER_BUTTON_STATE_IDLE:
			if (encoder_is_button_pressed()) {
				ctx.button_event_tick = delay_get_ticks();
				ctx.button_state = ENCODER_BUTTON_STATE_DEBOUNCE;
			}
			break;
		case ENCODER_BUTTON_STATE_DEBOUNCE:
			if ((delay_get_ticks() - ctx.button_event_tick) >= ENCODER_BUTTON_DEBOUNCE_TIME_MS) {
				if (encoder_is_button_pressed()) {
					ctx.button_event_tick = delay_get_ticks();
					ctx.button_state = ENCODER_BUTTON_STATE_CLICKED;
				}
				else {
					ctx.button_state = ENCODER_BUTTON_STATE_IDLE;
				}
			}
			break;
		case ENCODER_BUTTON_STATE_CLICKED:
			if ((delay_get_ticks() - ctx.button_event_tick) >= ENCODER_BUTTON_HOLD_TIME_MS) {
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
	encoder_rotation_update();
	encoder_button_update();
}

void EXTI7_0_IRQHandler(void)
{
	if (EXTI_GetITStatus(GPIO_ENC_PHA_PIN)) {
		encoder_rotation_isr(GPIO_ENC_PHA_PIN);
		EXTI_ClearITPendingBit(GPIO_ENC_PHA_PIN);
	}

	if (EXTI_GetITStatus(GPIO_ENC_PHB_PIN)) {
		encoder_rotation_isr(GPIO_ENC_PHB_PIN);
		EXTI_ClearITPendingBit(GPIO_ENC_PHB_PIN);
	}
}
