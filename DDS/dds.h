#pragma once

#include <stm32f1xx_hal.h>

// TODO doxygen

#define DDS_XTAL_FREQ_HZ 25000000U
#define DDS_MAX_OUTPUT_FREQ_HZ (DDS_XTAL_FREQ_HZ / 2)

#define DDS_FREQ_REG_BITS 28
#define DDS_FREQ_REG_BITS_PER_WORD (DDS_FREQ_REG_BITS / 2)

#define DDS_FREQ_REG_MAX_VALUE (1U << DDS_FREQ_REG_BITS)

typedef enum
{
	DDS_MODE_DISABLED = 0,
	DDS_MODE_SINE,
	DDS_MODE_TRIANGLE,
	DDS_MODE_HALF_SQUARE,
	DDS_MODE_SQUARE,
	DDS_MODE_COUNT
} dds_mode_t;

typedef enum
{
	DDS_CH0 = 0,
	DDS_CH1,
	DDS_CHANNEL_COUNT
} dds_channel_t;

HAL_StatusTypeDef dds_init(void);

HAL_StatusTypeDef dds_set_mode(dds_mode_t mode);
dds_mode_t dds_get_mode(void);

HAL_StatusTypeDef dds_set_frequency_channel(dds_channel_t channel);
dds_channel_t dds_get_frequency_channel(void);

HAL_StatusTypeDef dds_set_frequency(float frequency, dds_channel_t channel);
float dds_get_frequency(dds_channel_t channel);

HAL_StatusTypeDef dds_set_phase_channel(dds_channel_t channel);
dds_channel_t dds_get_phase_channel(void);

HAL_StatusTypeDef dds_set_phase(float phase, dds_channel_t channel);
float dds_get_phase(dds_channel_t channel);

HAL_StatusTypeDef dds_set_amplitude(float amplitude);
float dds_get_amplitude(void);
