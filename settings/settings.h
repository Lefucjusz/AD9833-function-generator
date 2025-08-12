#pragma once

#include <stdint.h>

#define SETTINGS_EEPROM_I2C_ADDR (0x50 << 1)
#define SETTINGS_I2C_SPEED_HZ 100000

typedef enum
{
	SETTINGS_FREQUENCY = 0,
	SETTINGS_AMPLITUDE,
	SETTINGS_WAVEFORM,
    SETTINGS_COUNT
} settings_entry_t;

int settings_init(void);

int settings_write(uint32_t value, settings_entry_t entry);
int settings_read(uint32_t *value, settings_entry_t entry);
