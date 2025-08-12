#include "dds.h"
#include <gpio.h>
#include <spi.h>
#include <utils.h>
#include <string.h>
#include <errno.h>

#define DDS_SPI_TIMEOUT_MS 100

#define DDS_XTAL_FREQ_FACTOR ((float)DDS_FREQ_REG_MAX_VALUE / DDS_XTAL_FREQ_HZ)
#define DDS_PHASE_FACTOR ((float)DDS_PHASE_REG_MAX_VALUE / DDS_MAX_PHASE_DEG)

/* Control bits */
#define DDS_B28_CTRL_BIT		(1 << 13)
#define DDS_HLB_CTRL_BIT		(1 << 12)
#define DDS_FSELECT_CTRL_BIT	(1 << 11)
#define DDS_PSELECT_CTRL_BIT	(1 << 10)
#define DDS_RESET_CTRL_BIT		(1 << 8)
#define DDS_SLEEP1_CTRL_BIT		(1 << 7)
#define DDS_SLEEP12_CTRL_BIT	(1 << 6)
#define DDS_OPBITEN_CTRL_BIT	(1 << 5)
#define DDS_DIV2_CTRL_BIT		(1 << 3)
#define DDS_MODE_CTRL_BIT		(1 << 1)

/* Frequency register bits */
#define DDS_FREQ0_REG_MASK		(0x0001 << 14)
#define DDS_FREQ1_REG_MASK		(0x0002 << 14)
#define DDS_FREQ_REG_VALUE_MASK	(~(DDS_FREQ0_REG_MASK | DDS_FREQ1_REG_MASK))

/* Phase register bits */
#define DDS_PHASE0_REG_MASK			(0x000D << 12)
#define DDS_PHASE1_REG_MASK			(0x000F << 12)
#define DDS_PHASE_REG_VALUE_MASK 	(~(DDS_PHASE0_REG_MASK | DDS_PHASE1_REG_MASK))

/* PGA command bits */
#define DDS_PGA_WRITE_CMD		(0x01 << 4)
#define DDS_PGA_SHUTDOWN_CMD	(0x02 << 4)
#define DDS_PGA_POT_1_SELECT	(0x01 << 0)

typedef enum
{
	DDS_SPI_MODE_UNKNOWN = 0,
	DDS_SPI_MODE_0, // Required by MCP41010
	DDS_SPI_MODE_2 	// Required by AD9833
} dds_spi_mode_t;

typedef struct
{
	dds_mode_t mode;
	dds_channel_t freq_ch;
	dds_channel_t phase_ch;
	uint16_t ctrl_reg;
	float freq[DDS_CHANNEL_COUNT];
	float phase[DDS_CHANNEL_COUNT];
	float amplitude;
	dds_spi_mode_t spi_mode;
} dds_ctx_t;

static dds_ctx_t ctx;

static int pga_spi_write(uint16_t data)
{
	if (ctx.spi_mode != DDS_SPI_MODE_0) {
		SPI_HANDLE->CTLR1 &= ~SPI_CPOL_High;
		ctx.spi_mode = DDS_SPI_MODE_0;
	}

	GPIO_WriteBit(GPIO_SPI_PORT, GPIO_SPI_PGA_CS_PIN, Bit_RESET);
	const int err = spi_write(&data, 1); // SPI operates in 16-bit mode, hence size is 1
	GPIO_WriteBit(GPIO_SPI_PORT, GPIO_SPI_PGA_CS_PIN, Bit_SET);

	return err;
}

static int dds_spi_write(uint16_t data)
{
	if (ctx.spi_mode != DDS_SPI_MODE_2) {
		SPI_HANDLE->CTLR1 |= SPI_CPOL_High;
		ctx.spi_mode = DDS_SPI_MODE_2;
	}

	GPIO_WriteBit(GPIO_SPI_PORT, GPIO_SPI_DDS_CS_PIN, Bit_RESET);
	const int err = spi_write(&data, 1); // SPI operates in 16-bit mode, hence size is 1
	GPIO_WriteBit(GPIO_SPI_PORT, GPIO_SPI_DDS_CS_PIN, Bit_SET);

	return err;
}

static int dds_write_frequency(uint32_t frequency, dds_channel_t channel)
{
	uint16_t hi_word;
	uint16_t lo_word;

	/* Select register to write */
	switch (channel) {
		case DDS_CH0:
			hi_word = lo_word = DDS_FREQ0_REG_MASK;
			break;
		case DDS_CH1:
			hi_word = lo_word = DDS_FREQ1_REG_MASK;
			break;
		default:
			return -EINVAL;
	}

	/* Set value of each word */
	hi_word |= (frequency >> DDS_FREQ_REG_BITS_PER_WORD) & DDS_FREQ_REG_VALUE_MASK;
	lo_word |= frequency & DDS_FREQ_REG_VALUE_MASK;

	/* Write to chip */
	int err = dds_spi_write(lo_word);
	if (err) {
		return err;
	}
	err = dds_spi_write(hi_word);
	if (err) {
		return err;
	}

	return 0;
}

static int dds_write_phase(uint16_t phase, dds_channel_t channel)
{
	uint16_t reg_val;

	/* Select register to write */
	switch (channel) {
		case DDS_CH0:
			reg_val = DDS_PHASE0_REG_MASK;
			break;
		case DDS_CH1:
			reg_val = DDS_PHASE1_REG_MASK;
			break;
		default:
			return -EINVAL;
	}

	/* Set phase value */
	reg_val |= phase & DDS_PHASE_REG_VALUE_MASK;

	/* Write to chip */
	const int err = dds_spi_write(reg_val);
	if (err) {
		return err;
	}

	return 0;
}


int dds_init(void)
{
	/* Reset the chip */
	int err = dds_spi_write(ctx.ctrl_reg | DDS_RESET_CTRL_BIT);
	if (err) {
		return err;
	}

	/* Reset shadow variables to keep in sync with hardware */
	memset(&ctx, 0, sizeof(ctx));

	/* Release the reset, enable 28-bit frequency register mode, disable output */
	ctx.ctrl_reg |= (DDS_B28_CTRL_BIT | DDS_SLEEP1_CTRL_BIT | DDS_SLEEP12_CTRL_BIT);
	err = dds_spi_write(ctx.ctrl_reg);
	if (err) {
		return err;
	}

	return 0;
}

int dds_set_mode(dds_mode_t mode)
{
	if ((mode < 0) || (mode >= DDS_MODE_COUNT)) {
		return -EINVAL;
	}

	/* Clear all mode bits in control register */
	ctx.ctrl_reg &= ~(DDS_OPBITEN_CTRL_BIT | DDS_DIV2_CTRL_BIT | DDS_MODE_CTRL_BIT);

	switch (mode) {
		case DDS_MODE_SINE:
			// No need to set anything
			break;
		case DDS_MODE_TRIANGLE:
			ctx.ctrl_reg |= DDS_MODE_CTRL_BIT;
			break;
		case DDS_MODE_SQUARE:
			ctx.ctrl_reg |= (DDS_OPBITEN_CTRL_BIT | DDS_DIV2_CTRL_BIT);
			break;
		case DDS_MODE_HALF_SQUARE:
			ctx.ctrl_reg |= DDS_OPBITEN_CTRL_BIT;
			break;
		default:
			break;
	}

	const int err = dds_spi_write(ctx.ctrl_reg);
	if (err) {
		return err;
	}

	ctx.mode = mode;

	return 0;
}

dds_mode_t dds_get_mode(void)
{
	return ctx.mode;
}

int dds_set_frequency_channel(dds_channel_t channel)
{
	if ((channel < 0) || (channel >= DDS_CHANNEL_COUNT)) {
		return -EINVAL;
	}

	switch (channel) {
		case DDS_CH0:
			ctx.ctrl_reg &= ~DDS_FSELECT_CTRL_BIT;
			break;
		case DDS_CH1:
			ctx.ctrl_reg |= DDS_FSELECT_CTRL_BIT;
			break;
		default:
			break;
	}

	const int err = dds_spi_write(ctx.ctrl_reg);
	if (err) {
		return err;
	}

	ctx.freq_ch = channel;

	return 0;
}

dds_channel_t dds_get_frequency_channel(void)
{
	return ctx.freq_ch;
}

int dds_set_frequency(float frequency, dds_channel_t channel)
{
	if ((channel < 0) || (channel >= DDS_CHANNEL_COUNT)) {
		return -EINVAL;
	}

	frequency = UTILS_CLAMP(frequency, 0.0f, DDS_MAX_OUTPUT_FREQ_HZ);

	const uint32_t reg_val = utils_roundf(frequency * DDS_XTAL_FREQ_FACTOR);
	const int err = dds_write_frequency(reg_val, channel);
	if (err) {
		return err;
	}

	ctx.freq[channel] = frequency;

	return 0;
}

float dds_get_frequency(dds_channel_t channel)
{
	if ((channel < 0) || (channel >= DDS_CHANNEL_COUNT)) {
		return -1.0f;
	}

	return ctx.freq[channel];
}

int dds_set_phase_channel(dds_channel_t channel)
{
	if ((channel < DDS_CH0) || (channel > DDS_CH1)) {
		return -EINVAL;
	}

	switch (channel) {
		case DDS_CH0:
			ctx.ctrl_reg &= ~DDS_PSELECT_CTRL_BIT;
			break;
		case DDS_CH1:
			ctx.ctrl_reg |= DDS_PSELECT_CTRL_BIT;
			break;
		default:
			break;
	}

	const int err = dds_spi_write(ctx.ctrl_reg);
	if (err) {
		return err;
	}

	ctx.phase_ch = channel;

	return err;
}

dds_channel_t dds_get_phase_channel(void)
{
	return ctx.phase_ch;
}

int dds_set_phase(float phase, dds_channel_t channel)
{
	if ((channel < 0) || (channel >= DDS_CHANNEL_COUNT)) {
		return -EINVAL;
	}

	/* Normalize to <0; 360> */
	phase = wrap_phase_degrees(phase);

	const uint16_t reg_val = utils_roundf(phase * DDS_PHASE_FACTOR);
	const int err = dds_write_phase(reg_val, channel);
	if (err) {
		return err;
	}

	ctx.phase[channel] = phase;

	return 0;
}

float dds_get_phase(dds_channel_t channel)
{
	if ((channel < 0) || (channel >= DDS_CHANNEL_COUNT)) {
		return -1.0f;
	}

	return ctx.phase[channel];
}

int dds_set_amplitude(float amplitude)
{
	float voltage_per_step;

	if ((ctx.mode == DDS_MODE_SQUARE) || (ctx.mode == DDS_MODE_HALF_SQUARE)) {
		amplitude = UTILS_CLAMP(amplitude, 0.0f, DDS_PGA_MAX_SQUARE_OUTPUT_AMPL_V);
		voltage_per_step = DDS_PGA_SQUARE_VOLTAGE_PER_STEP_V;
	}
	else {
		amplitude = UTILS_CLAMP(amplitude, 0.0f, DDS_PGA_MAX_OUTPUT_AMPL_V);
		voltage_per_step = DDS_PGA_VOLTAGE_PER_STEP_V;
	}

	const uint16_t pot_val = utils_roundf(amplitude / voltage_per_step);
	const uint16_t cmd = DDS_PGA_WRITE_CMD | DDS_PGA_POT_1_SELECT;

	const int err = pga_spi_write((cmd << 8) | pot_val);
	if (err) {
		return err;
	}

	ctx.amplitude = pot_val * voltage_per_step;

	return 0;
}

float dds_get_amplitude(void)
{
	return ctx.amplitude;
}

int dds_set_output_enable(bool enable)
{
	int err;

	if (enable) {
		ctx.ctrl_reg &= ~(DDS_SLEEP1_CTRL_BIT | DDS_SLEEP12_CTRL_BIT);
		err = dds_spi_write(ctx.ctrl_reg);
		if (err) {
			return err;
		}

		/* Switch back to selected mode, setting proper OPBITEN value */
		err = dds_set_mode(ctx.mode);
		if (err) {
			return err;
		}
	}
	else {
		/* If the sleep mode is entered in one of square wave modes, and MSB of DAC data
		 * happens to be high, the output will remain high too. Disable square wave mode
		 * by clearing OPBITEN mode to avoid that. */
		ctx.ctrl_reg &= ~DDS_OPBITEN_CTRL_BIT;
		ctx.ctrl_reg |= (DDS_SLEEP1_CTRL_BIT | DDS_SLEEP12_CTRL_BIT);
		err = dds_spi_write(ctx.ctrl_reg);
		if (err) {
			return err;
		}
	}

	return 0;
}

bool dds_get_output_enable(void)
{
	return (ctx.ctrl_reg & (DDS_SLEEP1_CTRL_BIT | DDS_SLEEP12_CTRL_BIT)) == 0;
}
