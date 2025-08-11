#include <ch32v00x.h>
// #include <debug.h>
#include <delay.h>
#include <gpio.h>
#include <hd44780_io.h>
#include <hd44780.h>

int main(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	SystemCoreClockUpdate();
	// Delay_Init();
	delay_init();
	gpio_init();
	
	// hd44780_config_t display_config = {
	// 	.io = hd44780_io_get(),
	// 	.type = HD44780_DISPLAY_16x2,
	// 	.entry_mode_flags = HD44780_INCREASE_CURSOR_ON,
	// };
	// hd44780_init(&display_config);

	// hd44780_write_string("Elo!");

	bool state = true;

	while (1) {

		GPIO_WriteBit(GPIOC, GPIO_USER_LED_PIN, state);

		state = !state;

		delay_ms(500);
	}
}
