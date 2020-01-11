#include <util/delay.h>
#include "spi.h"

typedef int bool;
enum
{
    false,
    true
};

/*============================================================================*/
#define PORTB_REG   0x16
struct port
{
    uint8_t   pin;
    uint8_t   ddr;
    uint8_t   port;
};
volatile struct port *const portb = (void*)PORTB_REG;

#define SCK     0x20
#define MISO    0x10
#define MOSI    0x08
#define CS      0x04
#define DC      0x02
#define RST     0x01

/*============================================================================*/
static inline void cs_high(void)
{
    portb->port |= CS;
}

static inline void cs_low(void)
{
    portb->port &= ~CS;
}

static inline void dc_high(void)
{
    portb->port |= DC;
}

static inline void dc_low(void)
{
    portb->port &= ~DC;
}

static inline void rst_high(void)
{
    portb->port |= RST;
}

static inline void rst_low(void)
{
    portb->port &= ~RST;
}

/*============================================================================*/
static void port_init(void)
{
    portb->ddr  = (SCK | MOSI | CS | DC | RST);
    portb->port = (CS | DC | RST);
}

/*============================================================================*/
/* Fundamental Command */
#define SSD1306_CMD_ENTIRE_ON           0xA4
#define SSD1306_CMD_DISPLAY_ON          0xAE

/* Charge Pump Command */
#define SSD1306_CMD_CHARGEPUMP          0x8D

/*============================================================================*/
void ssd1306_reset(void)
{
    rst_low();
    _delay_ms(1);
    rst_high();
}

void ssd1306_cmd(uint8_t *cmd, uint8_t len)
{
    int     i;

    dc_low();
    cs_low();

    for (i=0; i<len; i++)
        spi_rw(*cmd++);

    cs_high();
}

/*============================================================================*/
static void ssd1306_chargepump_enable(bool enable)
{
    uint8_t     cmd_buf[2];

    cmd_buf[0] = SSD1306_CMD_CHARGEPUMP;
    cmd_buf[1] = 0x10 | (enable ? 4 : 0);

    ssd1306_cmd(cmd_buf, 2);
}

static void ssd1306_entire_on(bool enable)
{
    uint8_t     cmd = SSD1306_CMD_ENTIRE_ON | enable;

    ssd1306_cmd(&cmd, 1);
}

static void ssd1306_display_on(bool enable)
{
    uint8_t     cmd = SSD1306_CMD_DISPLAY_ON | enable;

    ssd1306_cmd(&cmd, 1);
}

void ssd1306_init(void)
{
    _delay_ms(1);

    ssd1306_reset();

    ssd1306_chargepump_enable(true);
    ssd1306_entire_on(true);
    ssd1306_display_on(true);
}

/*============================================================================*/
void setup(void)
{
    port_init();
    spi_init();

    ssd1306_init();
}

void loop(void)
{
}

int main(void)
{
    setup();

    while (1) 
        loop();
}

