#include "spi.h"
#include <ch32v00x.h>
#include <timer.h>
#include <errno.h>

#define SPI_TIMEOUT_MS 100

typedef struct
{
    timer_t timer;
} spi_ctx_t;

static spi_ctx_t ctx;

static int spi_wait_for_flag(uint32_t flag, FlagStatus status)
{
    timer_reset(&ctx.timer);

    while (1) {
        if (SPI_I2S_GetFlagStatus(SPI_HANDLE, flag) == status) {
            return 0;
        }
        if (timer_has_elapsed(&ctx.timer)) {
            return -ETIMEDOUT;
        }
    }
    
    return 0;
}

void spi_init(void)
{
    SPI_InitTypeDef spi_cfg = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

    spi_cfg.SPI_Direction = SPI_Direction_1Line_Tx;
    spi_cfg.SPI_Mode = SPI_Mode_Master;
    spi_cfg.SPI_DataSize = SPI_DataSize_16b;
    spi_cfg.SPI_CPOL = SPI_CPOL_Low;
    spi_cfg.SPI_CPHA = SPI_CPHA_1Edge;
    spi_cfg.SPI_NSS = SPI_NSS_Soft;
    spi_cfg.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
    spi_cfg.SPI_FirstBit = SPI_FirstBit_MSB;
    spi_cfg.SPI_CRCPolynomial = 7;
    SPI_Init(SPI_HANDLE, &spi_cfg);

    SPI_Cmd(SPI_HANDLE, ENABLE);

    timer_init(&ctx.timer, SPI_TIMEOUT_MS);
}

int spi_write(const void *data, size_t size)
{
    const uint16_t *data_ptr = data;

    for (size_t i = 0; i < size; ++i) {
        SPI_I2S_SendData(SPI_HANDLE, data_ptr[i]); // Why is it called SPI_I2S...?
        const int err = spi_wait_for_flag(SPI_I2S_FLAG_TXE, SET);
        if (err) {
            return err;
        }
    }

    return 0;
}
