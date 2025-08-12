#pragma once

#include <stdint.h>
#include <stddef.h>

typedef enum
{
    I2C_REG_SIZE_8BIT,
    I2C_REG_SIZE_16BIT
} i2c_reg_size_t;

void i2c_init(uint32_t speed, uint16_t dev_addr, i2c_reg_size_t reg_size);

int i2c_read(uint16_t reg_addr, void *data, size_t size);
int i2c_write(uint16_t reg_addr, const void *data, size_t size);
