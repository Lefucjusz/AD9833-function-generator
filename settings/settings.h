#pragma once

#include <stdint.h>

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
