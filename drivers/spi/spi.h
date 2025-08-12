#pragma once

#include <stddef.h>

#define SPI_HANDLE SPI1

void spi_init(void);

int spi_write(const void *data, size_t size);
