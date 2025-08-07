#include "gui.h"
#include <hd44780.h>
#include <hd44780_io.h>
#include <encoder.h>
#include <stm32f1xx_hal.h>
#include <math.h>

typedef enum
{
	GUI_SINE_WAVE_CHAR_1 = HD44780_CUSTOM_GLYPH_0,
	GUI_SINE_WAVE_CHAR_2,
	GUI_TRIANGLE_WAVE_CHAR_1,
	GUI_TRIANGLE_WAVE_CHAR_2,
	GUI_SQUARE_WAVE_CHAR_1,
	GUI_SQUARE_WAVE_CHAR_2,
	GUI_LARGE_DOT_CHAR,
	GUI_WAVEFORM_CHARS_COUNT
} gui_custom_chars_t;

typedef struct
{
	hd44780_config_t lcd_config;
	int32_t frequency; // TODO use 0.1Hz resolution!
	bool frequency_changed;
	uint8_t digit;
	bool setting;
} gui_ctx_t;

static gui_ctx_t ctx;

static const uint8_t waveform_chars[GUI_WAVEFORM_CHARS_COUNT][HD44780_CGRAM_CHAR_SIZE] =
{
		{0x00, 0x00, 0x0E, 0x11, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x11, 0x0E, 0x00, 0x00},
		{0x00, 0x04, 0x0A, 0x11, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x11, 0x0A, 0x04, 0x00},
		{0x0E, 0x0A, 0x0A, 0x1B, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x1B, 0x0A, 0x0A, 0x0E, 0x00},
		{0x00, 0x0E, 0x1F, 0x1F, 0x1F, 0x0E, 0x00, 0x00}
};

static void gui_load_custom_chars(void)
{
	for (size_t i = GUI_SINE_WAVE_CHAR_1; i < GUI_WAVEFORM_CHARS_COUNT; ++i) {
		hd44780_load_custom_glyph(waveform_chars[i], i);
	}

	hd44780_clear();
}

static void gui_on_click(void)
{
	ctx.digit++;
	if (ctx.digit > 6) {
		ctx.digit = 0;
		ctx.setting = false;
	}
	ctx.frequency_changed = true;
}

/*
 * I'm fully aware everything below is probably the most ugly code I wrote this year
 * but it's just for testing and experimentation. This abomination will be rewritten
 * cleanly ASAP.
 */

static void gui_on_rotate(encoder_direction_t direction, uint16_t count, int16_t increment)
{
	uint32_t multiplier = powf(10.0f, ctx.digit); // TODO do it as *= 10
	uint32_t sum = ctx.frequency + increment * multiplier;
	if (sum < 0 || sum > 9999999) {
		return;
	}

	ctx.frequency = sum;

	ctx.frequency_changed = true;
}

static void gui_display_frequency(uint32_t frequency, bool leading_zeros)
{
	uint32_t div = 1000000;
	uint8_t digit;
	bool non_zero = false;

	for (size_t i = 0; i < 7; ++i) { // TODO while
		digit = (frequency / div) % 10;

		if (i == 6 || digit != 0 || leading_zeros || non_zero) {
			non_zero = true;
			hd44780_write_char(digit + '0');
		}
		if ((i == 0 || i == 3) && (digit != 0 || leading_zeros || non_zero)) {
			hd44780_write_char(',');
		}

		div /= 10;
	}
}

void gui_init(void)
{
	ctx.lcd_config.io = hd44780_io_get();
	ctx.lcd_config.type = HD44780_DISPLAY_16x2;
	ctx.lcd_config.entry_mode_flags = HD44780_INCREASE_CURSOR_ON;

	hd44780_init(&ctx.lcd_config);
	hd44780_show_cursor(true);
	gui_load_custom_chars();

	encoder_init();
	encoder_set_button_callback(gui_on_click);
	encoder_set_rotation_callback(gui_on_rotate);

	ctx.frequency_changed = true;
	ctx.setting = true;
}

void gui_task(void)
{
	encoder_task();

	if (ctx.frequency_changed) {
		if (!ctx.setting) {
			hd44780_clear();
			hd44780_show_cursor(false);
		}
		hd44780_gotoxy(1, 1);
		hd44780_write_string("Freq:");
		gui_display_frequency(ctx.frequency, ctx.setting);
//		hd44780_write_integer(ctx.frequency / 1000000, 0);
//		hd44780_write_char(',');
//		hd44780_write_integer((ctx.frequency % 1000000) / 1000, 3);
//		hd44780_write_char(',');
//		hd44780_write_integer(ctx.frequency % 1000, 3);
		hd44780_write_string("Hz");

		hd44780_gotoxy(2, 1);
		hd44780_write_string("Ampl:3.12Vp ");



		hd44780_gotoxy(2, 13);
		hd44780_write_char(GUI_SINE_WAVE_CHAR_1);
		hd44780_write_char(GUI_SINE_WAVE_CHAR_2);

		hd44780_gotoxy(2, 16);
		hd44780_write_char(GUI_LARGE_DOT_CHAR);

		uint8_t sum = ctx.digit;
		if (ctx.digit > 2) {
			sum += 1;
		}
		if (ctx.digit > 5) {
			sum += 1;
		}
		hd44780_gotoxy(1, 16 - 2 - sum);

		ctx.frequency_changed = false;
	}
}
