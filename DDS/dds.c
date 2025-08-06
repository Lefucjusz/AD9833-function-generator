#include "dds.h"
#include "gpio.h"
#include "spi.h"
#include <math.h>
#include <string.h>

#define DDS_SPI_TIMEOUT_MS 250

#define DDS_XTAL_FREQ_FACTOR ((float)DDS_FREQ_REG_MAX_VALUE / DDS_XTAL_FREQ_HZ)

/* Control bits */
#define B28_CTRL_BIT 		(1 << 13)
#define HLB_CTRL_BIT 		(1 << 12)
#define FSELECT_CTRL_BIT	(1 << 11)
#define PSELECT_CTRL_BIT 	(1 << 10)
#define RESET_CTRL_BIT 		(1 << 8)
#define SLEEP1_CTRL_BIT 	(1 << 7)
#define SLEEP12_CTRL_BIT 	(1 << 6)
#define OPBITEN_CTRL_BIT 	(1 << 5)
#define DIV2_CTRL_BIT 		(1 << 3)
#define MODE_CTRL_BIT 		(1 << 1)

/* Frequency register bits */
#define FREQ0_REG_MASK		(1 << 14)
#define FREQ1_REG_MASK 		(2 << 14)
#define FREQ_REG_VALUE_MASK (~(FREQ0_REG_MASK | FREQ1_REG_MASK))

// TODO move these to utils
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define CLAMP(x, lo, hi) MIN(hi, MAX(lo, x))

typedef struct
{
	dds_mode_t mode;
	dds_channel_t freq_ch;
	dds_channel_t phase_ch;
	uint16_t ctrl_reg;
	float freq[DDS_CHANNEL_COUNT];
	float phase[DDS_CHANNEL_COUNT];
} dds_ctx_t;

static dds_ctx_t ctx;

//static HAL_StatusTypeDef pga_spi_write(uint16_t data)
//{
//	HAL_StatusTypeDef status;
//
//	HAL_GPIO_WritePin(PGA_CS_GPIO_Port, PGA_CS_Pin, GPIO_PIN_RESET);
//	status = HAL_SPI_Transmit(&hspi1, (uint8_t *)&data, 1, DDS_SPI_TIMEOUT_MS); // SPI operates in 16-bit mode, hence size is 1
//	HAL_GPIO_WritePin(PGA_CS_GPIO_Port, PGA_CS_Pin, GPIO_PIN_SET);
//
//	return status;
//}

static HAL_StatusTypeDef dds_spi_write(uint16_t data)
{
	HAL_StatusTypeDef status;

	HAL_GPIO_WritePin(DDS_CS_GPIO_Port, DDS_CS_Pin, GPIO_PIN_RESET);
	status = HAL_SPI_Transmit(&hspi1, (uint8_t *)&data, 1, DDS_SPI_TIMEOUT_MS);  // SPI operates in 16-bit mode, hence size is 1
	HAL_GPIO_WritePin(DDS_CS_GPIO_Port, DDS_CS_Pin, GPIO_PIN_SET);

	return status;
}

static HAL_StatusTypeDef dds_write_frequency(uint32_t frequency, uint8_t channel)
{
	uint16_t hi_word;
	uint16_t lo_word;

	/* Select register to write */
	if (channel == 0) {
		hi_word = lo_word = FREQ0_REG_MASK;
	}
	else {
		hi_word = lo_word = FREQ1_REG_MASK;
	}

	/* Set value of each word */
	hi_word |= (frequency >> DDS_FREQ_REG_BITS_PER_WORD) & FREQ_REG_VALUE_MASK;
	lo_word |= frequency & FREQ_REG_VALUE_MASK;

	/* Write to chip */
	HAL_StatusTypeDef status;
	status = dds_spi_write(lo_word);
	if (status != HAL_OK) {
		return status;
	}
	status = dds_spi_write(hi_word);
	if (status != HAL_OK) {
		return status;
	}

	return HAL_OK;
}

HAL_StatusTypeDef dds_init(void)
{
	HAL_StatusTypeDef status;

	/* Reset the chip */
	status = dds_spi_write(ctx.ctrl_reg | RESET_CTRL_BIT);
	if (status != HAL_OK) {
		return status;
	}

	/* Reset shadow variables to keep in sync with hardware */
	memset(&ctx, 0, sizeof(ctx));

	/* Release the reset and enable 28-bit frequency register mode */
	ctx.ctrl_reg |= B28_CTRL_BIT;
	status = dds_spi_write(ctx.ctrl_reg);
	if (status != HAL_OK) {
		return status;
	}

	return HAL_OK;
}

HAL_StatusTypeDef dds_set_mode(dds_mode_t mode)
{
	if ((mode < 0) || (mode >= DDS_MODE_COUNT)) {
		return HAL_ERROR;
	}

	/* Clear all mode bits in control register */
	ctx.ctrl_reg &= ~(SLEEP1_CTRL_BIT | SLEEP12_CTRL_BIT | OPBITEN_CTRL_BIT | DIV2_CTRL_BIT | MODE_CTRL_BIT);

	switch (mode) {
		case DDS_MODE_DISABLED:
			ctx.ctrl_reg |= (SLEEP1_CTRL_BIT | SLEEP12_CTRL_BIT);
			break;
		case DDS_MODE_SINE:
			// No need to set anything
			break;
		case DDS_MODE_TRIANGLE:
			ctx.ctrl_reg |= MODE_CTRL_BIT;
			break;
		case DDS_MODE_HALF_SQUARE:
			ctx.ctrl_reg |= OPBITEN_CTRL_BIT;
			break;
		case DDS_MODE_SQUARE:
			ctx.ctrl_reg |= (OPBITEN_CTRL_BIT | DIV2_CTRL_BIT);
			break;
		default:
			break;
	}

	const HAL_StatusTypeDef status = dds_spi_write(ctx.ctrl_reg);
	if (status != HAL_OK) {
		return status;
	}

	ctx.mode = mode;

	return HAL_OK;
}

dds_mode_t dds_get_mode(void)
{
	return ctx.mode;
}

HAL_StatusTypeDef dds_set_frequency_channel(dds_channel_t channel)
{
	if ((channel < 0) || (channel >= DDS_CHANNEL_COUNT)) {
		return HAL_ERROR;
	}

	switch (channel) {
		case DDS_CH0:
			ctx.ctrl_reg &= ~FSELECT_CTRL_BIT;
			break;
		case DDS_CH1:
			ctx.ctrl_reg |= FSELECT_CTRL_BIT;
			break;
		default:
			break;
	}

	const HAL_StatusTypeDef status = dds_spi_write(ctx.ctrl_reg);
	if (status != HAL_OK) {
		return status;
	}

	ctx.freq_ch = channel;

	return HAL_OK;
}

dds_channel_t dds_get_frequency_channel(void)
{
	return ctx.freq_ch;
}

HAL_StatusTypeDef dds_set_frequency(float frequency, dds_channel_t channel)
{
	if ((channel < 0) || (channel >= DDS_CHANNEL_COUNT)) {
		return HAL_ERROR;
	}

	frequency = CLAMP(frequency, 0.0f, DDS_MAX_OUTPUT_FREQ_HZ);

	const uint32_t reg_val = roundf(frequency * DDS_XTAL_FREQ_FACTOR);
	const HAL_StatusTypeDef status = dds_write_frequency(reg_val, channel);
	if (status != HAL_OK) {
		return status;
	}

	ctx.freq[channel] = frequency;

	return HAL_OK;
}

float dds_get_frequency(dds_channel_t channel)
{
	if ((channel < 0) || (channel >= DDS_CHANNEL_COUNT)) {
		return -1.0f;
	}

	return ctx.freq[channel];
}

HAL_StatusTypeDef dds_set_phase_channel(dds_channel_t channel)
{
	if ((channel < DDS_CH0) || (channel > DDS_CH1)) {
		return HAL_ERROR;
	}

	switch (channel) {
		case DDS_CH0:
			ctx.ctrl_reg &= ~PSELECT_CTRL_BIT;
			break;
		case DDS_CH1:
			ctx.ctrl_reg |= PSELECT_CTRL_BIT;
			break;
		default:
			break;
	}

	const HAL_StatusTypeDef status = dds_spi_write(ctx.ctrl_reg);
	if (status != HAL_OK) {
		return status;
	}

	ctx.phase_ch = channel;

	return HAL_OK;
}

dds_channel_t dds_get_phase_channel(void)
{
	return ctx.phase_ch;
}

HAL_StatusTypeDef dds_set_phase(float phase, dds_channel_t channel)
{
	if ((channel < 0) || (channel >= DDS_CHANNEL_COUNT)) {
		return HAL_ERROR;
	}

	// TODO

	return HAL_ERROR;
}

float dds_get_phase(dds_channel_t channel)
{
	if ((channel < 0) || (channel >= DDS_CHANNEL_COUNT)) {
		return -1.0f;
	}

	return ctx.phase[channel];
}

HAL_StatusTypeDef dds_set_amplitude(float amplitude)
{
	return HAL_ERROR; // TODO
}

float dds_get_amplitude(void)
{
	return -1.0f; // TODO
}
