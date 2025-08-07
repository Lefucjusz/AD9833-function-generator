#include "gui.h"
#include <hd44780.h>
#include <hd44780_io.h>
#include <encoder.h>
#include <stm32f1xx_hal.h>

typedef enum
{
	GUI_SINE_CHAR_1 = HD44780_CUSTOM_GLYPH_0,
	GUI_SINE_CHAR_2,
	GUI_TRIANGLE_CHAR_1,
	GUI_TRIANGLE_CHAR_2,
	GUI_SQUARE_CHAR_1,
	GUI_SQUARE_CHAR_2,
	GUI_WAVEFORM_CHARS_COUNT
} gui_waveform_chars_t;

typedef struct
{
	hd44780_config_t lcd_config;
} gui_ctx_t;

static gui_ctx_t ctx;

static const uint8_t waveform_chars[GUI_WAVEFORM_CHARS_COUNT][HD44780_CGRAM_CHAR_SIZE] =
{
		{0x00, 0x00, 0x0E, 0x11, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x11, 0x0E, 0x00, 0x00},
		{0x00, 0x04, 0x0A, 0x11, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x11, 0x0A, 0x04, 0x00},
		{0x0E, 0x0A, 0x0A, 0x1B, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x1B, 0x0A, 0x0A, 0x0E, 0x00}
};

static void gui_load_custom_chars(void)
{
	for (size_t i = GUI_SINE_CHAR_1; i < GUI_WAVEFORM_CHARS_COUNT; ++i) {
		hd44780_load_custom_glyph(waveform_chars[i], i);
	}

	hd44780_clear();
}

static void gui_on_click(void)
{
	hd44780_gotoxy(1, 1);
	hd44780_write_string("Click!");
}

static void gui_on_rotate(encoder_direction_t direction, uint16_t count, int16_t increment)
{
	hd44780_gotoxy(2, 1);
	hd44780_write_integer(count, 0);
	hd44780_write_char(' ');
}

void gui_init(void)
{
	ctx.lcd_config.io = hd44780_io_get();
	ctx.lcd_config.type = HD44780_DISPLAY_16x2;
	ctx.lcd_config.entry_mode_flags = HD44780_INCREASE_CURSOR_ON;

	hd44780_init(&ctx.lcd_config);
	gui_load_custom_chars();

	encoder_init();
	encoder_set_button_callback(gui_on_click);
	encoder_set_rotation_callback(gui_on_rotate);

//	while (1) {
//		hd44780_gotoxy(1, 1);
//		hd44780_write_char(0x00);
//		hd44780_write_char(0x01);
//		HAL_Delay(1000);
//
//		hd44780_gotoxy(1, 1);
//		hd44780_write_char(0x02);
//		hd44780_write_char(0x03);
//		HAL_Delay(1000);
//
//		hd44780_gotoxy(1, 1);
//		hd44780_write_char(0x04);
//		hd44780_write_char(0x05);
//		HAL_Delay(1000);
//	}
}

void gui_task(void)
{
	encoder_task();
}
