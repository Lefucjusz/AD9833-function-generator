#pragma once

#include <stdbool.h>

#define DDS_XTAL_FREQ_HZ 25000000U
#define DDS_MAX_OUTPUT_FREQ_HZ (DDS_XTAL_FREQ_HZ / 2)
#define DDS_MAX_PHASE_DEG 360.0f
#define DDS_PHASE_REG_BITS 12
#define DDS_PHASE_REG_MAX_VALUE (1U << DDS_PHASE_REG_BITS)

#define DDS_FREQ_REG_BITS 28
#define DDS_FREQ_REG_BITS_PER_WORD (DDS_FREQ_REG_BITS / 2)
#define DDS_FREQ_REG_MAX_VALUE (1U << DDS_FREQ_REG_BITS)

/* Amplitude in square wave mode is rail-to-rail, in other modes 38mV to 650mV */
#define DDS_MAX_SQUARE_OUTPUT_AMPL_V 5.0f
#define DDS_MAX_OUTPUT_AMPL_V 0.65f

/* Gain of the output opamp stage, R2 = 5k, R1 = 1k, G = R2/R1 + 1 */
#define DDS_PGA_OPAMP_GAIN 6.0f

/* Attenuation due to opamp input loading, Rpot = 10k, Rin = ~290k, G = Rin / (Rin + Rpot) */
#define DDS_PGA_OPAMP_RIN_GAIN 0.96f

#define DDS_PGA_GAIN (DDS_PGA_OPAMP_GAIN * DDS_PGA_OPAMP_RIN_GAIN)

#define DDS_PGA_MAX_SQUARE_OUTPUT_AMPL_V DDS_MAX_SQUARE_OUTPUT_AMPL_V // In theory should be multiplied by DDS_PGA_OUTPUT_STAGE_GAIN, but power supply is the limitation
#define DDS_PGA_MAX_OUTPUT_AMPL_V (DDS_MAX_OUTPUT_AMPL_V * DDS_PGA_GAIN)

#define DDS_PGA_STEPS_NUM 256
#define DDS_PGA_SQUARE_VOLTAGE_PER_STEP_V ((DDS_MAX_SQUARE_OUTPUT_AMPL_V * DDS_PGA_GAIN) / DDS_PGA_STEPS_NUM)
#define DDS_PGA_VOLTAGE_PER_STEP_V ((DDS_MAX_OUTPUT_AMPL_V * DDS_PGA_GAIN) / DDS_PGA_STEPS_NUM)

typedef enum
{
	DDS_MODE_SINE,
	DDS_MODE_TRIANGLE,
	DDS_MODE_SQUARE,
	DDS_MODE_HALF_SQUARE,
	DDS_MODE_COUNT
} dds_mode_t;

typedef enum
{
	DDS_CH0 = 0,
	DDS_CH1,
	DDS_CHANNEL_COUNT
} dds_channel_t;

int dds_init(void);

int dds_set_mode(dds_mode_t mode);
dds_mode_t dds_get_mode(void);

int dds_set_frequency_channel(dds_channel_t channel);
dds_channel_t dds_get_frequency_channel(void);

int dds_set_frequency(float frequency, dds_channel_t channel);
float dds_get_frequency(dds_channel_t channel);

int dds_set_phase_channel(dds_channel_t channel);
dds_channel_t dds_get_phase_channel(void);

int dds_set_phase(float phase, dds_channel_t channel);
float dds_get_phase(dds_channel_t channel);

int dds_set_amplitude(float amplitude);
float dds_get_amplitude(void);

int dds_set_output_enable(bool enable);
bool dds_get_output_enable(void);
