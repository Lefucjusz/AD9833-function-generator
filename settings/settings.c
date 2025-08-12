#include "settings.h"
#include <i2c.h>
#include <dds.h>
#include <delay.h>
#include <stdbool.h>
#include <errno.h>

/* NOTE: This is a simple EEPROM settings storage implementation without wear-leveling.
 * Given that setting changes are infrequent and the 24C16 EEPROM supports approximately
 * 1,000,000 write cycles per cell, this approach is sufficient for this use case.
 *
 * This can probably implemented using internal Flash memory, similarly to the solution 
 * used in STM32s, but this way it was much quicker. Maybe I'll rewrite it one day. */

#define SETTINGS_EEPROM_WRITE_TIME_MS 5

#define SETTINGS_CHECKSUM_INDEX 0
#define SETTINGS_DATA_START_INDEX 1

#define SETTINGS_BYTES_PER_ENTRY sizeof(uint32_t)
#define SETTINGS_BYTES_PER_CHECKSUM sizeof(uint8_t)
#define SETTINGS_SIZE_BYTES ((SETTINGS_BYTES_PER_ENTRY * SETTINGS_COUNT) + SETTINGS_BYTES_PER_CHECKSUM)

#define SETTINGS_CHECKSUM_INIT_VALUE 0xBB

/* Defaults used to initialize the EEPROM */
#define SETTINGS_DEFAULT_FREQ 1000 // Hz
#define SETTINGS_DEFAULT_AMPL 10 // V * 10
#define SETTINGS_DEFAULT_WAVEFORM DDS_MODE_SINE

static uint8_t settings_compute_checksum(const uint8_t *data, size_t size)
{
    uint8_t checksum = SETTINGS_CHECKSUM_INIT_VALUE;
    for (size_t i = 0; i < size; ++i) {
        checksum ^= data[i];
    }
    return checksum;
}

static bool settings_check_integrity(void)
{
    int err;
    uint8_t buffer[SETTINGS_SIZE_BYTES];

    err = i2c_read(SETTINGS_CHECKSUM_INDEX, buffer, sizeof(buffer));
    if (err) {
        return false;
    }

    return (settings_compute_checksum(buffer, sizeof(buffer)) == 0);
}

static int settings_update_checksum(void)
{
    int err;
    uint8_t buffer[SETTINGS_SIZE_BYTES];

    err = i2c_read(SETTINGS_CHECKSUM_INDEX, buffer, sizeof(buffer));
    if (err) {
        return false;
    }

    const uint8_t checksum = settings_compute_checksum(&buffer[SETTINGS_DATA_START_INDEX], sizeof(buffer) - SETTINGS_DATA_START_INDEX);

    err = i2c_write(SETTINGS_CHECKSUM_INDEX, &checksum, SETTINGS_BYTES_PER_CHECKSUM);
    if (err) {
        return err;
    }

    delay_ms(SETTINGS_EEPROM_WRITE_TIME_MS);

    return 0;
}

static int settings_load_defaults(void)
{
    int err;

    err = settings_write(SETTINGS_DEFAULT_FREQ, SETTINGS_FREQUENCY);
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
    /* Check if EEPROM already populated, if not, populate with defaults */
    if (!settings_check_integrity()) {
        const int err = settings_load_defaults();
        if (err) {
            return err;
        }
    }

    return 0;
}

int settings_write(uint32_t value, settings_entry_t entry)
{
    int err;
    const uint8_t *value_ptr = (uint8_t *)&value;

    if (entry >= SETTINGS_COUNT) {
		return -EINVAL;
	}

    const uint8_t base_addr = SETTINGS_DATA_START_INDEX + SETTINGS_BYTES_PER_ENTRY * entry;

    for (size_t i = 0; i < SETTINGS_BYTES_PER_ENTRY; ++i) {
        err = i2c_write(base_addr + i, &value_ptr[i], sizeof(uint8_t));
        if (err) {
            return err;
        }
        delay_ms(SETTINGS_EEPROM_WRITE_TIME_MS);
    }

    err = settings_update_checksum();
    if (err) {
        return err;
    }

    return 0;
}

int settings_read(uint32_t *value, settings_entry_t entry)
{
    if ((entry >= SETTINGS_COUNT) || (value == NULL)) {
		return -EINVAL;
	}

    const uint8_t base_addr = SETTINGS_DATA_START_INDEX + SETTINGS_BYTES_PER_ENTRY * entry;
    
    return i2c_read(base_addr, value, SETTINGS_BYTES_PER_ENTRY);
}
