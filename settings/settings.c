#include "settings.h"
#include <eeprom.h>
#include <dds.h>
#include <errno.h>
#include <stddef.h>
#include <assert.h>

/* Each EEPROM variable is 16-bit, we store everything as 32-bit */
#define SETTINGS_CELLS_PER_ENTRY 2
#define SETTINGS_ENTRIES_NUM (NB_OF_VAR / SETTINGS_CELLS_PER_ENTRY)

/* Defaults used to initialize the EEPROM */
#define SETTINGS_DEFAULT_FREQ 1000 // Hz
#define SETTINGS_DEFAULT_AMPL 10 // V * 10
#define SETTINGS_DEFAULT_WAVEFORM DDS_MODE_SINE

uint16_t VirtAddVarTab[NB_OF_VAR];

static_assert(SETTINGS_COUNT <= SETTINGS_ENTRIES_NUM, "Not enough EEPROM variables for the required settings number. Adjust NB_OF_VAR in eeprom.h");

static int settings_load_default(void)
{
	int err = settings_write(SETTINGS_DEFAULT_FREQ, SETTINGS_FREQUENCY);
	if (err) {
		return err;
	}
	err = settings_write(SETTINGS_DEFAULT_AMPL, SETTINGS_AMPLITUDE);
	if (err) {
		return err;
	}
	err = settings_write(SETTINGS_DEFAULT_WAVEFORM, SETTINGS_WAVEFORM);
	if (err) {
		return err;
	}

	return 0;
}

int settings_init(void)
{
	/* Fill virtual address table */
	for (size_t i = 0; i < NB_OF_VAR; ++i) {
		VirtAddVarTab[i] = i;
	}

	FLASH_Unlock_Fast();

	if (EE_Init() != FLASH_COMPLETE) {
		return -EIO;
	}

	/* Check if EEPROM already populated, if not, populate with defaults */
	uint32_t dummy;
	int err = settings_read(&dummy, SETTINGS_FREQUENCY);
	if (err == -EIO) {
		return -EIO;
	}
	else if (err == -ENOENT) {
		err = settings_load_default();
		if (err) {
			return err;
		}
	}

	return 0;
}

int settings_write(uint32_t value, settings_entry_t entry)
{
	if (entry >= SETTINGS_ENTRIES_NUM) {
		return -EINVAL;
	}

	const uint16_t base_addr = SETTINGS_CELLS_PER_ENTRY * entry;

	if (EE_WriteVariable(base_addr, (value >> 16) & 0xFFFF) != FLASH_COMPLETE) {
		return -EIO;
	}
	if (EE_WriteVariable(base_addr + 1, value & 0xFFFF) != FLASH_COMPLETE) {
		return -EIO;
	}

	return 0;
}

int settings_read(uint32_t *value, settings_entry_t entry)
{
	if ((entry >= SETTINGS_ENTRIES_NUM) || (value == NULL)) {
		return -EINVAL;
	}

	uint16_t temp;
	const uint16_t base_addr = SETTINGS_CELLS_PER_ENTRY * entry;

	uint16_t err = EE_ReadVariable(base_addr, &temp);
	if (err == NO_VALID_PAGE) {
		return -EIO;
	}
	else if (err) {
		return -ENOENT;
	}
	*value = (uint32_t)temp << 16;

	err = EE_ReadVariable(base_addr + 1, &temp);
	if (err == NO_VALID_PAGE) {
		return -EIO;
	}
	else if (err) {
		return -ENOENT;
	}
	*value |= temp;

	return 0;
}
