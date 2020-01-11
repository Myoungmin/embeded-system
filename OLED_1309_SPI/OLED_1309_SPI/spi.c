#include "spi.h"


volatile struct spi *const spi = (void*)SPI_REG;

uint8_t spi_rw(uint8_t data)
{
    spi->spdr = data;
    while (!(spi->spsr & SPIF));
    return spi->spdr;
}


void spi_init(void)
{
    spi->spcr = (SPE | MSTR | DIV_16);
    spi->spsr = 0;
}

