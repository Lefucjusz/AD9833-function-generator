/*
 * HD44780.h
 *
 *  Created on: 16.05.2022
 *      Author: Lefucjusz
 */

#ifndef HD44780_H_
#define HD44780_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define HD44780_CGRAM_CHAR_SIZE 8 // Each custom char uses 8 bytes

typedef enum
{
	HD44780_CLEAR_DISPLAY_CMD = 0x01,
	HD44780_ENTRY_MODE_SET_CMD = 0x04,
	HD44780_DISPLAY_ON_OFF_CMD = 0x08,
	HD44780_CURSOR_OR_DISP_SHIFT_MODE_CMD = 0x10, // This is such a useless command I haven't implemented it yet
	HD44780_FUNCTION_SET_CMD = 0x20,
	HD44780_SET_CGRAM_ADDR_CMD = 0x40,
	HD44780_SET_DDRAM_ADDR_CMD = 0x80
} hd44780_commands;

typedef enum
{
	HD44780_ONE_LINE = 0x00,
	HD44780_TWO_LINES = 0x08
} hd44780_lines;

typedef enum
{
	HD44780_FLAGS_CLEARED = 0x00, // In this mode display scroll is off and cursor position is decreased
	HD44780_DISPLAY_SCROLL_ON = 0x01,
	HD44780_INCREASE_CURSOR_ON = 0x02
} hd44780_entry_mode_flags;

typedef enum
{
	HD44780_ALL_OFF = 0x00,
	HD44780_CURSOR_BLINK_ON = 0x01,
	HD44780_CURSOR_ON = 0x02,
	HD44780_DISPLAY_ON = 0x04
} hd44780_on_off_flags;

typedef enum
{
	HD44780_DISPLAY_16x1_TYPE_1,
	HD44780_DISPLAY_16x1_TYPE_2,
	HD44780_DISPLAY_16x2,
	HD44780_DISPLAY_16x4,
	HD44780_DISPLAY_20x2,
	HD44780_DISPLAY_20x4,
	HD44780_DISPLAY_40x2,
	HD44780_DISPLAY_TYPES_NUM // This value equals to number of display types
} hd44780_type_t;

typedef enum
{
	HD44780_INSTRUCTION,
	HD44780_CHARACTER
} hd44780_mode_t;

typedef enum
{
	HD44780_CUSTOM_GLYPH_0,
	HD44780_CUSTOM_GLYPH_1,
	HD44780_CUSTOM_GLYPH_2,
	HD44780_CUSTOM_GLYPH_3,
	HD44780_CUSTOM_GLYPH_4,
	HD44780_CUSTOM_GLYPH_5,
	HD44780_CUSTOM_GLYPH_6,
	HD44780_CUSTOM_GLYPH_7,
	HD44780_CUSTOM_GLYPHS_NUM // This value equals to number of custom glyphs in HD44780 controller (which is 8 BTW)
} hd44780_glyph_addr_t;

typedef enum
{
	HD44780_PIN_RS,
	HD44780_PIN_E,
	HD44780_PIN_D4,
	HD44780_PIN_D5,
	HD44780_PIN_D6,
	HD44780_PIN_D7,
	HD44780_PIN_NUM
} hd44780_pin_t;

typedef enum
{
	HD44780_LOW = 0,
	HD44780_HIGH
} hd44780_pin_state_t;

typedef struct
{
	void (*set_pin_state)(hd44780_pin_t pin, hd44780_pin_state_t state);
	void (*delay_us)(uint16_t us);
} hd44780_io_t;

typedef struct
{
	hd44780_io_t *io;
	hd44780_type_t type;
	uint8_t entry_mode_flags;
} hd44780_config_t;

/**
 * @brief Initializes HD44780 display library
 *
 * @param config Pointer to struct with proper configuration set
 */
void hd44780_init(const hd44780_config_t *config);

/**
 * @brief Writes one byte of data to display
 *
 * @param byte Value to be written to display
 *
 * @param mode Sets register to be written to - either instruction or character register
 */
void hd44780_write_byte(uint8_t byte, hd44780_mode_t mode);

/**
 * @brief Writes command (instruction) to HD44780 display, wrapper for HD44780_write_byte()
 *
 * @param command Command to be written
 */
void hd44780_write_cmd(uint8_t command);

/**
 * @brief Writes character to HD44780 display, wrapper for hd44780_write_byte()
 *
 * @param character Character to be written
 */
void hd44780_write_char(char character);

/**
 * @brief Clears the display and positions the cursor in first column of the first row
 */
void hd44780_clear(void);

/**
 * @brief Enables cursor visibility without blinking
 *
 * @param show True for cursor to be shown, false to be hidden
 */
void hd44780_show_cursor(bool show);

/**
 * @brief Moves the cursor to the requested position
 *
 * @param x Row to move to (indexed from 1!)
 * @param y Column to move to (indexed from 1!)
 */
void hd44780_gotoxy(size_t x, size_t y);

/**
 * @brief Displays integer adding leading zeros if required
 *
 * @param number Integer to be displayed
 * @param length Required number of digits in the displayed number
 * 				 If value to be displayed has fewer digits than required,
 * 				 leading zeros will be appended
 */
void hd44780_write_integer(int32_t number, size_t length);

/**
 * @brief Displays null-terminated string
 *
 * @param string Null-terminated string to be displayed
 */
void hd44780_write_string(const char *string);

/**
 * @brief Loads custom glyph to HD44780's CGRAM
 *
 * @param glyph_array Array containing glyph in required format
 * @param cgram_addr CGRAM address where the glyph will be stored
 */
void hd44780_load_custom_glyph(const uint8_t *glyph_array, hd44780_glyph_addr_t cgram_addr);

#endif /* HD44780_H_ */
