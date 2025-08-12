#include "i2c.h"
#include <ch32v00x.h>
#include <delay.h>
#include <timer.h>
#include <stdbool.h>
#include <errno.h>

#define I2C_HANDLE I2C1
#define I2C_TIMEOUT_MS 1000

typedef struct
{
    uint16_t dev_addr;
    i2c_reg_size_t reg_size;
    timer_t timer;
} i2c_ctx_t;

static i2c_ctx_t ctx;

static int i2c_wait_for_event(uint32_t event)
{
    timer_reset(&ctx.timer);

    while (1) {
        if (I2C_CheckEvent(I2C_HANDLE, event) == READY) {
            return 0;
        }
        if (timer_has_elapsed(&ctx.timer)) {
            return -ETIMEDOUT;
        }
    }

    return 0;
}

static int i2c_wait_for_flag(uint32_t flag, FlagStatus status)
{
    timer_reset(&ctx.timer);

    while (1) {
        if (I2C_GetFlagStatus(I2C_HANDLE, flag) == status) {
            return 0;
        }
        if (timer_has_elapsed(&ctx.timer)) {
            return -ETIMEDOUT;
        }
    }
    
    return 0;
}

static int i2c_read_byte(uint8_t *byte, uint16_t reg_addr)
{
    int err;

    /* Wait until bus not busy */
    err = i2c_wait_for_flag(I2C_FLAG_BUSY, RESET);
    if (err) {
        return err;
    }

    do {
        /* Generate start condition */
        I2C_GenerateSTART(I2C_HANDLE, ENABLE);
        err = i2c_wait_for_event(I2C_EVENT_MASTER_MODE_SELECT);
        if (err) {
            break;
        }

        /* Send device address */
        I2C_Send7bitAddress(I2C_HANDLE, ctx.dev_addr, I2C_Direction_Transmitter);
        err = i2c_wait_for_event(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED);
        if (err) {
            break;
        }

        /* Send register address */
        if (ctx.reg_size == I2C_REG_SIZE_16BIT) {
            I2C_SendData(I2C_HANDLE, reg_addr >> 8);
            err = i2c_wait_for_event(I2C_EVENT_MASTER_BYTE_TRANSMITTED);
            if (err) {
                break;
            }
        }
        I2C_SendData(I2C_HANDLE, reg_addr & 0xFF);
        err = i2c_wait_for_event(I2C_EVENT_MASTER_BYTE_TRANSMITTED);
        if (err) {
            break;
        }

        /* Generate repeated start condition */
        I2C_GenerateSTART(I2C_HANDLE, ENABLE);
        err = i2c_wait_for_event(I2C_EVENT_MASTER_MODE_SELECT);
        if (err) {
            break;
        }

        /* Send device address */
        I2C_Send7bitAddress(I2C_HANDLE, ctx.dev_addr, I2C_Direction_Receiver);
        err = i2c_wait_for_event(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED);
        if (err) {
            break;
        }

        /* Wait for data */
        timer_reset(&ctx.timer);
        do {
            if (I2C_GetFlagStatus(I2C_HANDLE, I2C_FLAG_RXNE) != RESET) {
                break;
            }
            I2C_AcknowledgeConfig(I2C_HANDLE, DISABLE); // Why is this needed?
            if (timer_has_elapsed(&ctx.timer)) {
                err = -ETIMEDOUT;
            }
        } while (!err);

        if (err) {
            break;
        }

        /* Read the data */
        *byte = I2C_ReceiveData(I2C_HANDLE);
    } while (0);
    
    /* Stop transmission */
    I2C_GenerateSTOP(I2C_HANDLE, ENABLE);

    return 0;
}

static int i2c_write_byte(uint8_t byte, uint8_t reg_addr)
{
    int err;

    /* Wait until bus not busy */
    err = i2c_wait_for_flag(I2C_FLAG_BUSY, RESET);
    if (err) {
        return err;
    }

    do {
         /* Generate start condition */
        I2C_GenerateSTART(I2C_HANDLE, ENABLE);
        err = i2c_wait_for_event(I2C_EVENT_MASTER_MODE_SELECT);
        if (err) {
            break;
        }

        /* Send device address */
        I2C_Send7bitAddress(I2C_HANDLE, ctx.dev_addr, I2C_Direction_Transmitter);
        err = i2c_wait_for_event(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED);
        if (err) {
            break;
        }

        /* Send register address */
        if (ctx.reg_size == I2C_REG_SIZE_16BIT) {
            I2C_SendData(I2C_HANDLE, reg_addr >> 8);
            err = i2c_wait_for_event(I2C_EVENT_MASTER_BYTE_TRANSMITTED);
            if (err) {
                break;
            }
        }
        I2C_SendData(I2C_HANDLE, reg_addr & 0xFF);
        err = i2c_wait_for_event(I2C_EVENT_MASTER_BYTE_TRANSMITTED);
        if (err) {
            break;
        }

        /* Send data */
        if (I2C_GetFlagStatus(I2C_HANDLE, I2C_FLAG_TXE) != RESET) {
            I2C_SendData(I2C_HANDLE, byte);
        }
        err = i2c_wait_for_event(I2C_EVENT_MASTER_BYTE_TRANSMITTED);
        if (err) {
            break;
        } 
    } while (0);

    /* Stop transmission */
    I2C_GenerateSTOP(I2C_HANDLE, ENABLE);

    return err;
}

void i2c_init(uint32_t speed_hz, uint16_t dev_addr, i2c_reg_size_t reg_size)
{
    I2C_InitTypeDef i2c_cfg = {0};

    ctx.dev_addr = dev_addr;
    ctx.reg_size = reg_size;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

    i2c_cfg.I2C_ClockSpeed = speed_hz;
    i2c_cfg.I2C_Mode = I2C_Mode_I2C;
    i2c_cfg.I2C_DutyCycle = I2C_DutyCycle_2;
    i2c_cfg.I2C_OwnAddress1 = ctx.dev_addr;
    i2c_cfg.I2C_Ack = I2C_Ack_Enable;
    i2c_cfg.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_Init(I2C_HANDLE, &i2c_cfg);

    I2C_Cmd(I2C_HANDLE, ENABLE);
    I2C_AcknowledgeConfig(I2C_HANDLE, ENABLE);

    timer_init(&ctx.timer, I2C_TIMEOUT_MS);
}

int i2c_read(uint16_t reg_addr, void *data, size_t size)
{
    int err;
    uint8_t *data_ptr = data;

    for (size_t i = 0; i < size; ++i) {
        err = i2c_read_byte(&data_ptr[i], reg_addr + i);
        if (err) {
            return err;
        }
    }

    return 0;
}

int i2c_write(uint16_t reg_addr, const void *data, size_t size)
{
    int err;
    const uint8_t *data_ptr = data;

    for (size_t i = 0; i < size; ++i) {
        err = i2c_write_byte(data_ptr[i], reg_addr + i);
        if (err) {
            return err;
        }
    }

    return 0;
}
