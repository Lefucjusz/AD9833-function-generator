#include <ch32v00x.h>
#include <delay.h>
#include <gpio.h>
#include <i2c.h>
#include <spi.h>
#include <hd44780_io.h>
#include <hd44780.h>
#include <settings.h>
#include <dds.h>
#include <gui.h>

int main(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	SystemCoreClockUpdate();

	delay_init();
	gpio_init();
	i2c_init(SETTINGS_I2C_SPEED_HZ, SETTINGS_EEPROM_I2C_ADDR, I2C_REG_SIZE_8BIT);
	spi_init();
	
	const hd44780_config_t display_config = {
		.io = hd44780_io_get(),
		.type = HD44780_DISPLAY_16x2,
		.entry_mode_flags = HD44780_INCREASE_CURSOR_ON,
	};
	hd44780_init(&display_config);

	settings_init();
	dds_init();
	gui_init();

	/* 
	 * TODO: 
	 * - add watchdog
	 * - add low power mode or remove functions needed for it
	 * - add error handling
	 */

	while (1) {
		gui_task();
	}
}
