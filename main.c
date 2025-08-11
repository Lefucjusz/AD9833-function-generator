#include <ch32v00x.h>
#include <delay.h>
#include <gpio.h>
#include <encoder.h>
#include <hd44780_io.h>
#include <hd44780.h>

static void cb(encoder_button_action_t type)
{
	hd44780_gotoxy(1, 1);
	hd44780_write_string(type == ENCODER_BUTTON_CLICK ? "click" : "hold ");
}

static void cb2(encoder_direction_t direction, uint32_t count, int32_t increment)
{
	hd44780_gotoxy(2, 1);
	hd44780_write_integer(count, 0);
}

int main(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	SystemCoreClockUpdate();
	delay_init();
	gpio_init();
	
	const hd44780_config_t display_config = {
		.io = hd44780_io_get(),
		.type = HD44780_DISPLAY_16x2,
		.entry_mode_flags = HD44780_INCREASE_CURSOR_ON,
	};
	hd44780_init(&display_config);

	encoder_init();

	encoder_set_button_callback(cb);
	encoder_set_rotation_callback(cb2);

	while (1) {
		encoder_task();
	}
}
