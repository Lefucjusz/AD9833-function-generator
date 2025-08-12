#include "gui.h"
#include <delay.h>
#include <hd44780.h>
#include <encoder.h>
#include <dds.h>
#include <settings.h>
#include <error_handler.h>
#include <utils.h>
#include <math.h>

/* NOTE: In practice, the -3dB point of the analog front-end used in this module is around 1 MHz.
 * The slowest component is the MCP40101 digital potentiometer, which has an upper cutoff frequency
 * of approximately 1 MHz. If you need to generate higher-frequency signals, you will need to
 * design your own analog path using faster components... or deal with heavily distorted output. */
#define GUI_FREQ_MIN_VALUE 1
#define GUI_FREQ_MAX_VALUE 9999999
#define GUI_FREQ_DIGITS_NUM 7
#define GUI_FREQ_LAST_DIGIT_INDEX (GUI_FREQ_DIGITS_NUM - 1)
#define GUI_FREQ_COMMA_1_POS 0 // Positions of frequency digits after which the comma should be put
#define GUI_FREQ_COMMA_2_POS 3

#define GUI_AMPL_SCALE_FACTOR 10 // Scale and store amplitude as integer to simplify math
#define GUI_AMPL_MIN_VALUE 0
#define GUI_AMPL_MAX_VALUE ((uint32_t)(DDS_PGA_MAX_OUTPUT_AMPL_V * GUI_AMPL_SCALE_FACTOR))
#define GUI_AMPL_MAX_VALUE_SQUARE ((uint32_t)(DDS_PGA_MAX_SQUARE_OUTPUT_AMPL_V * GUI_AMPL_SCALE_FACTOR))
#define GUI_AMPL_DIGITS_NUM 2
#define GUI_AMPL_LAST_DIGIT_INDEX (GUI_AMPL_DIGITS_NUM - 1)
#define GUI_AMPL_DP_POS 0 // Position of amplitude digit after which the decimal point should be put

#define GUI_DISP_RESOLUTION_X 2
#define GUI_DISP_RESOLUTION_Y 16

/* Coordinates of items on display */
#define GUI_DISP_FREQ_X 1
#define GUI_DISP_FREQ_END_Y 14
#define GUI_DISP_AMPL_X 2
#define GUI_DISP_AMPL_END_Y 8
#define GUI_DISP_WAVEFORM_X 2
#define GUI_DISP_WAVEFORM_Y 13
#define GUI_DISP_OUTPUT_X 2
#define GUI_DISP_OUTPUT_Y 16

#define GUI_SETTING_TIMEOUT_MS 5000

typedef enum
{
	GUI_SINE_WAVE_CHAR_1 = HD44780_CUSTOM_GLYPH_0,
	GUI_SINE_WAVE_CHAR_2,
	GUI_TRIANGLE_WAVE_CHAR_1,
	GUI_TRIANGLE_WAVE_CHAR_2,
	GUI_SQUARE_WAVE_CHAR_1,
	GUI_SQUARE_WAVE_CHAR_2,
	GUI_OUTPUT_OFF_CHAR,
	GUI_OUTPUT_ON_CHAR,
	GUI_WAVEFORM_CHARS_COUNT
} gui_custom_chars_t;

typedef enum
{
	GUI_SET_MODE_OFF,
	GUI_SET_FREQUENCY,
	GUI_SET_AMPLITUDE,
	GUI_SET_WAVEFORM
} gui_state_t;

typedef enum
{
	GUI_REDRAW_PARTIAL,
	GUI_REDRAW_FULL
} gui_redraw_mode_t;

typedef struct
{
	uint32_t frequency;
	uint32_t amplitude;
	gui_state_t state;
	uint8_t selected_digit; // 0 - least significant
	dds_mode_t waveform;
	bool output_enabled;
	uint32_t last_activity_tick;
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
		{0x00, 0x0E, 0x11, 0x11, 0x11, 0x0E, 0x00, 0x00},
		{0x00, 0x0E, 0x1F, 0x1F, 0x1F, 0x0E, 0x00, 0x00}
};

static void gui_load_custom_chars(void)
{
	for (size_t i = GUI_SINE_WAVE_CHAR_1; i < GUI_WAVEFORM_CHARS_COUNT; ++i) {
		hd44780_load_custom_glyph(waveform_chars[i], i);
	}

	hd44780_clear();
}

static void gui_display_frequency(bool show_zeros)
{
	bool leading_zeros_end = false;

	for (size_t i = 0; i < GUI_FREQ_DIGITS_NUM; ++i) {
		const uint32_t divisor = utils_powu(10, GUI_FREQ_LAST_DIGIT_INDEX - i);
		const uint8_t digit = (ctx.frequency / divisor) % 10;

		/* First non-zero digit found, stop omitting zeros */
		if (digit != 0) {
			leading_zeros_end = true;
		}

		/* Display the digit if it's the last one, or showing leading zeros is enabled, or if there's no more leading zeros */
		if ((divisor == 1) || show_zeros || leading_zeros_end) {
			hd44780_write_char(digit + '0');
		}

		/* Display comma separators, but only if there are any preceding digits */
		if (((i == GUI_FREQ_COMMA_1_POS) || (i == GUI_FREQ_COMMA_2_POS)) && (show_zeros || leading_zeros_end)) {
			hd44780_write_char(',');
		}
	}
}

static void gui_display_amplitude(void)
{
	for (size_t i = 0; i < GUI_AMPL_DIGITS_NUM; ++i) {
		const uint32_t divisor = utils_powu(10, GUI_AMPL_LAST_DIGIT_INDEX - i);
		const uint8_t digit = (ctx.amplitude / divisor) % 10;

		hd44780_write_char(digit + '0');

		/* Display decimal point */
		if (i == GUI_AMPL_DP_POS) {
			hd44780_write_char('.');
		}
	}
}

static void gui_display_waveform(void)
{
	hd44780_gotoxy(GUI_DISP_WAVEFORM_X, GUI_DISP_WAVEFORM_Y);

	switch (ctx.waveform) {
		case DDS_MODE_SINE:
			hd44780_write_char(GUI_SINE_WAVE_CHAR_1);
			hd44780_write_char(GUI_SINE_WAVE_CHAR_2);
			break;
		case DDS_MODE_TRIANGLE:
			hd44780_write_char(GUI_TRIANGLE_WAVE_CHAR_1);
			hd44780_write_char(GUI_TRIANGLE_WAVE_CHAR_2);
			break;
		case DDS_MODE_HALF_SQUARE:
		case DDS_MODE_SQUARE:
			hd44780_write_char(GUI_SQUARE_WAVE_CHAR_1);
			hd44780_write_char(GUI_SQUARE_WAVE_CHAR_2);
			break;
		default:
			break;
	}
}

static void gui_display_output_state(void)
{
	hd44780_gotoxy(GUI_DISP_OUTPUT_X, GUI_DISP_OUTPUT_Y);
	hd44780_write_char(ctx.output_enabled ? GUI_OUTPUT_ON_CHAR : GUI_OUTPUT_OFF_CHAR);
}

static bool gui_increment_value(uint32_t *value, int32_t increment, uint8_t selected_digit, uint32_t limit_lo, uint32_t limit_hi)
{
	const uint32_t multiplier = utils_powu(10, selected_digit);
	const uint32_t new_value = *value + increment * multiplier;

	if ((new_value < limit_lo) || (new_value > limit_hi)) {
		return false;
	}

	*value = new_value;

	return true;
}

static void gui_redraw_display(uint8_t cursor_x, uint8_t cursor_y, gui_redraw_mode_t mode)
{
	const bool set_mode_active = (ctx.state != GUI_SET_MODE_OFF);

	/* Draw upper row */
	if (mode == GUI_REDRAW_PARTIAL) {
		hd44780_gotoxy(1, 1); // Go to the beginning of the upper row
	}
	else {
		hd44780_clear();
	}
	hd44780_show_cursor(set_mode_active);
	hd44780_write_string("Freq:");
	gui_display_frequency(set_mode_active);
	hd44780_write_string("Hz");

	/* Draw lower row */
	hd44780_gotoxy(2, 1);  // Go to the beginning of the lower row
	hd44780_write_string("Ampl:");
	gui_display_amplitude();
	hd44780_write_string("Vp");
	gui_display_waveform();
	gui_display_output_state();

	/* No need to explicitly position the cursor if it's not visible */
	if (set_mode_active) {
		hd44780_gotoxy(cursor_x, cursor_y);
	}
}

static uint8_t gui_frequency_digit_to_column(uint8_t digit_index)
{
	/* Compensate for comma separators */
	if (digit_index >= (GUI_FREQ_LAST_DIGIT_INDEX - GUI_FREQ_COMMA_1_POS)) {
		return GUI_DISP_FREQ_END_Y - digit_index - 2;
	}
	else if (digit_index >= (GUI_FREQ_LAST_DIGIT_INDEX - GUI_FREQ_COMMA_2_POS)) {
		return GUI_DISP_FREQ_END_Y - digit_index - 1;
	}

	return GUI_DISP_FREQ_END_Y - digit_index;
}

static uint8_t gui_amplitude_digit_to_column(uint8_t digit_index)
{
	/* Compensate for decimal point */
	if (digit_index >= (GUI_AMPL_LAST_DIGIT_INDEX - GUI_AMPL_DP_POS)) {
		return GUI_DISP_AMPL_END_Y - digit_index - 1;
	}
	return GUI_DISP_AMPL_END_Y - digit_index;
}

static int gui_load_settings(void)
{
	uint32_t value;

	int err = settings_read(&value, SETTINGS_FREQUENCY);
	if (err) {
		return err;
	}
	ctx.frequency = value;

	err = settings_read(&value, SETTINGS_AMPLITUDE);
	if (err) {
		return err;
	}
	ctx.amplitude = value;

	err = settings_read(&value, SETTINGS_WAVEFORM);
	if (err) {
		return err;
	}
	ctx.waveform = value;

	return 0;
}

static int gui_store_settings(void)
{
	int err = settings_write(ctx.frequency, SETTINGS_FREQUENCY);
	if (err) {
		return err;
	}

	err = settings_write(ctx.amplitude, SETTINGS_AMPLITUDE);
	if (err) {
		return err;
	}

	err = settings_write(ctx.waveform, SETTINGS_WAVEFORM);
	if (err) {
		return err;
	}

	return 0;
}

static int gui_configure_dds(void)
{
	int err = dds_set_frequency(ctx.frequency, DDS_CH0);
	if (err) {
		return err;
	}

	err = dds_set_mode(ctx.waveform);
	if (err) {
		return err;
	}

	err = dds_set_amplitude((float)ctx.amplitude / GUI_AMPL_SCALE_FACTOR);
	if (err) {
		return err;
	}

	err = dds_set_output_enable(ctx.output_enabled);
	if (err) {
		return err;
	}

	return 0;
}

static int gui_handle_setting_timeout(void)
{
	if (ctx.state == GUI_SET_MODE_OFF) {
		return 0;
	}

	if ((delay_get_ticks() - ctx.last_activity_tick) < GUI_SETTING_TIMEOUT_MS) {
		return 0;
	}

	/* Disable setting mode and roll back all changes */
	ctx.state = GUI_SET_MODE_OFF;
	const int err = gui_load_settings();
	if (err) {
		return err;
	}

	gui_redraw_display(0, 0, GUI_REDRAW_FULL);

	return 0;
}

static void gui_button_callback(encoder_button_action_t type)
{
	int err;

	ctx.last_activity_tick = delay_get_ticks();

	switch (ctx.state) {
		case GUI_SET_MODE_OFF:
			if (type == ENCODER_BUTTON_CLICK) {
				ctx.output_enabled = !ctx.output_enabled;
				err = dds_set_output_enable(ctx.output_enabled);
				if (err) {
					error_handler_message("DDS enable fail");
				}
				gui_redraw_display(0, 0, GUI_REDRAW_PARTIAL);
			}
			else {
				ctx.selected_digit = 0;
				ctx.state = GUI_SET_FREQUENCY;
				gui_redraw_display(GUI_DISP_FREQ_X, gui_frequency_digit_to_column(ctx.selected_digit), GUI_REDRAW_PARTIAL);
			}
			break;

		case GUI_SET_FREQUENCY:
			if (type == ENCODER_BUTTON_CLICK) {
				++ctx.selected_digit;
				if (ctx.selected_digit < GUI_FREQ_DIGITS_NUM) {
					gui_redraw_display(GUI_DISP_FREQ_X, gui_frequency_digit_to_column(ctx.selected_digit), GUI_REDRAW_PARTIAL);
				}
				else {
					ctx.selected_digit = 0;
					ctx.state = GUI_SET_AMPLITUDE;
					gui_redraw_display(GUI_DISP_AMPL_X, gui_amplitude_digit_to_column(ctx.selected_digit), GUI_REDRAW_PARTIAL);
				}
			}
			else {
				ctx.selected_digit = 0;
				ctx.state = GUI_SET_AMPLITUDE;
				gui_redraw_display(GUI_DISP_AMPL_X, gui_amplitude_digit_to_column(ctx.selected_digit), GUI_REDRAW_PARTIAL);
			}
			break;

		case GUI_SET_AMPLITUDE:
			if (type == ENCODER_BUTTON_CLICK) {
				++ctx.selected_digit;
				if (ctx.selected_digit < GUI_AMPL_DIGITS_NUM) {
					gui_redraw_display(GUI_DISP_AMPL_X, gui_amplitude_digit_to_column(ctx.selected_digit), GUI_REDRAW_PARTIAL);
				}
				else {
					ctx.state = GUI_SET_WAVEFORM;
					gui_redraw_display(GUI_DISP_WAVEFORM_X, GUI_DISP_WAVEFORM_Y, GUI_REDRAW_PARTIAL);
				}
			}
			else {
				ctx.state = GUI_SET_WAVEFORM;
				gui_redraw_display(GUI_DISP_WAVEFORM_X, GUI_DISP_WAVEFORM_Y, GUI_REDRAW_PARTIAL);
			}
			break;

		case GUI_SET_WAVEFORM:
			ctx.state = GUI_SET_MODE_OFF;

			/* Clamp down value that might have been set in square mode, but does not apply to other modes */
			if ((ctx.waveform != DDS_MODE_SQUARE) && (ctx.amplitude > GUI_AMPL_MAX_VALUE)) {
				ctx.amplitude = GUI_AMPL_MAX_VALUE;
			}

			err = gui_store_settings();
			if (err) {
				error_handler_message("NVS store fail");
			}
			err = gui_configure_dds();
			if (err) {
				error_handler_message("DDS config fail");
			}
			gui_redraw_display(0, 0, GUI_REDRAW_FULL);
			break;

		default:
			break;
	}
}

static void gui_rotation_callback(encoder_direction_t direction, uint32_t count, int32_t increment)
{
	/* Prevent changing values accidentally when moving to next field */
	if (!encoder_button_is_idle()) {
		return;
	}

	ctx.last_activity_tick = delay_get_ticks();

	switch (ctx.state) {
		case GUI_SET_MODE_OFF:
			// Do nothing
			break;

		case GUI_SET_FREQUENCY:
			if (gui_increment_value(&ctx.frequency, increment, ctx.selected_digit, GUI_FREQ_MIN_VALUE, GUI_FREQ_MAX_VALUE)) {
				gui_redraw_display(GUI_DISP_FREQ_X, gui_frequency_digit_to_column(ctx.selected_digit), GUI_REDRAW_PARTIAL);
			}
			break;

		case GUI_SET_AMPLITUDE: {
			const uint32_t max_amplitude = (ctx.waveform == DDS_MODE_SQUARE) ? GUI_AMPL_MAX_VALUE_SQUARE : GUI_AMPL_MAX_VALUE;
			if (gui_increment_value(&ctx.amplitude, increment, ctx.selected_digit, GUI_AMPL_MIN_VALUE, max_amplitude)) {
				gui_redraw_display(GUI_DISP_AMPL_X, gui_amplitude_digit_to_column(ctx.selected_digit), GUI_REDRAW_PARTIAL);
			}
			break;
		}

		case GUI_SET_WAVEFORM:
			ctx.waveform = UTILS_CLAMP(ctx.waveform + increment, DDS_MODE_SINE, DDS_MODE_SQUARE);
			gui_redraw_display(GUI_DISP_WAVEFORM_X, GUI_DISP_WAVEFORM_Y, GUI_REDRAW_PARTIAL);
			break;

		default:
			break;
	}
}

int gui_init(void)
{
	gui_load_custom_chars();

	encoder_set_button_callback(gui_button_callback);
	encoder_set_rotation_callback(gui_rotation_callback);

	/* Load initial settings */
	int err = gui_load_settings();
	if (err) {
		return err;
	}
	ctx.state = GUI_SET_MODE_OFF;
	ctx.output_enabled = false;

	err = gui_configure_dds();
	if (err) {
		return err;
	}

	gui_redraw_display(0, 0, GUI_REDRAW_FULL);

	return 0;
}

bool gui_is_idle(void)
{
	return (ctx.state == GUI_SET_MODE_OFF) && encoder_button_is_idle();
}

void gui_task(void)
{
	encoder_task();

	const int err = gui_handle_setting_timeout();
	if (err) {
		error_handler_message("NVS read fail");
	}
}
