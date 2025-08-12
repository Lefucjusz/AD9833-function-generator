#include <ch32v00x.h>
#include <delay.h>
#include <gpio.h>
#include <encoder.h>
#include <gui.h>
#include <hd44780_io.h>
#include <hd44780.h>

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

	/* 
	 * TODO: 
	 * - add SPI driver
	 * - add DDS driver
	 * - add some non-volatile storage (DS1307/24C01)
	 */

	gui_init();

	while (1) {
		gui_task();
	}
}
