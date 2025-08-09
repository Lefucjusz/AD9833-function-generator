#include "settings.h"
#include <eeprom.h>
#include <dds.h>

/* Each EEPROM variable is 16-bit, we store everything as 32-bit */
#define SETTINGS_CELLS_PER_ENTRY 2
#define SETTINGS_ENTRIES_NUM (NB_OF_VAR / SETTINGS_CELLS_PER_ENTRY)

/* Defaults used to initialize the EEPROM */
#define SETTINGS_DEFAULT_FREQ 1000 // Hz
#define SETTINGS_DEFAULT_AMPL 100 // V * 100
#define SETTINGS_DEFAULT_WAVEFORM DDS_MODE_SINE

uint16_t VirtAddVarTab[NB_OF_VAR];

static HAL_StatusTypeDef settings_load_default(void)
{
	HAL_StatusTypeDef status = settings_write(SETTINGS_DEFAULT_FREQ, SETTINGS_FREQUENCY);
	if (status != HAL_OK) {
		return status;
	}
	status = settings_write(SETTINGS_DEFAULT_AMPL, SETTINGS_AMPLITUDE);
	if (status != HAL_OK) {
		return status;
	}
	status = settings_write(SETTINGS_DEFAULT_WAVEFORM, SETTINGS_WAVEFORM);
	if (status != HAL_OK) {
		return status;
	}

	return HAL_OK;
}

HAL_StatusTypeDef settings_init(void)
{
	/* Fill virtual address table */
	for (size_t i = 0; i < NB_OF_VAR; ++i) {
		VirtAddVarTab[i] = i;
	}

	HAL_StatusTypeDef status = HAL_FLASH_Unlock();
	if (status != HAL_OK) {
		return status;
	}

	status = EE_Init();
	if (status != HAL_OK) {
		return status;
	}

	/* Check if EEPROM already populated, if not, populate with defaults */
	uint32_t dummy;
	status = settings_read(&dummy, SETTINGS_FREQUENCY);
	if (status == NO_VALID_PAGE) {
		return HAL_ERROR;
	}
	else if (status == HAL_ERROR) {
		status = settings_load_default();
		if (status != HAL_OK) {
			return status;
		}
	}

	return HAL_OK;
}

HAL_StatusTypeDef settings_write(uint32_t value, settings_entry_t entry)
{
	if (entry >= SETTINGS_ENTRIES_NUM) {
		return HAL_ERROR;
	}

	const uint16_t base_addr = SETTINGS_CELLS_PER_ENTRY * entry;

	HAL_StatusTypeDef status = EE_WriteVariable(base_addr, (value >> 16) & 0xFFFF);
	if (status != HAL_OK) {
		return status;
	}
	status = EE_WriteVariable(base_addr + 1, value & 0xFFFF);
	if (status != HAL_OK) {
		return status;
	}

	return HAL_OK;
}

HAL_StatusTypeDef settings_read(uint32_t *value, settings_entry_t entry)
{
	if ((entry >= SETTINGS_ENTRIES_NUM) || (value == NULL)) {
		return HAL_ERROR;
	}

	uint16_t temp;
	const uint16_t base_addr = SETTINGS_CELLS_PER_ENTRY * entry;

	HAL_StatusTypeDef status = EE_ReadVariable(base_addr, &temp);
	if (status != HAL_OK) {
		return status;
	}
	*value = (uint32_t)temp << 16;

	status = EE_ReadVariable(base_addr + 1, &temp);
	if (status != HAL_OK) {
		return status;
	}
	*value |= temp;

	return HAL_OK;
}
