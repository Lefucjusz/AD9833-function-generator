#include <ch32v00x.h>
#include <delay.h>
#include <gpio.h>
#include <i2c.h>
#include <spi.h>
#include <hd44780_io.h>
#include <hd44780.h>
#include <encoder.h>
#include <dds.h>
#include <settings.h>
#include <gui.h>
#include <error_handler.h>

static void enter_sleep_mode(void)
{
	delay_suspend_tick();
	__WFI();
	delay_resume_tick();
}

int main(void)
{
	const hd44780_config_t display_config = {
		.io = hd44780_io_get(),
		.type = HD44780_DISPLAY_16x2,
		.entry_mode_flags = HD44780_INCREASE_CURSOR_ON,
	};

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	SystemCoreClockUpdate();

	delay_init();
	gpio_init();
	i2c_init(SETTINGS_I2C_SPEED_HZ, SETTINGS_EEPROM_I2C_ADDR, I2C_REG_SIZE_8BIT);
	spi_init();
	hd44780_init(&display_config);
	encoder_init();

	int err = dds_init();
	if (err) {
		error_handler_message("DDS init fail");
	}
	err = settings_init();
	if (err) {
		error_handler_message("NVS init fail");
	}
	err =  gui_init();
	if (err) {
		error_handler_message("GUI init fail");
	}

	/* 
	 * TODO: 
	 * - add watchdog
	 */

	while (1) {
		gui_task();

		if (gui_is_idle()) {
			enter_sleep_mode();
		}
	}
}
