#include <stdint.h>

#define SPI_REG     0x0D
struct spi
{
    uint8_t     spcr;
    uint8_t     spsr;
    uint8_t     spdr;
};


#define SPIE        0x80
#define SPE         0x40
#define DORD        0x20
#define MSTR        0x10
#define CPOL        0x08
#define CPHA        0x04
enum sck_freq
{
    DIV_4,
    DIV_16,
    DIV_64,
    DIV_128
};

#define SPIF        0x80
#define WCOL        0x40
#define SPI2X       0x01

uint8_t spi_rw(uint8_t data);
void spi_init(void);


