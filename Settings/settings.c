#include "settings.h"
#include <eeprom.h>

/* Each EEPROM variable is 16-bit, we store everything as 32-bit */
#define SETTINGS_CELLS_PER_ENTRY 2
#define SETTINGS_ENTRIES_NUM (NB_OF_VAR / SETTINGS_CELLS_PER_ENTRY)

uint16_t VirtAddVarTab[NB_OF_VAR];

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

	return HAL_OK;
}

HAL_StatusTypeDef settings_write(uint32_t value, size_t address)
{
	if (address >= SETTINGS_ENTRIES_NUM) {
		return HAL_ERROR;
	}

	const uint16_t base_addr = SETTINGS_CELLS_PER_ENTRY * address;

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

HAL_StatusTypeDef settings_read(uint32_t *value, size_t address)
{
	if ((address >= SETTINGS_ENTRIES_NUM) || (value == NULL)) {
		return HAL_ERROR;
	}

	uint16_t temp;
	const uint16_t base_addr = SETTINGS_CELLS_PER_ENTRY * address;

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
