#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <util/delay.h>
//#include <avr/io.h>
#include "i2c.h"

typedef int bool;
enum
{
    false,
    true 
};

/*============================================================================*/
#define SSD1306_I2C_ADDR    0x3C
#define SSD1306_I2C_CMD     0x00
#define SSD1306_I2C_DATA    0x40


/*============================================================================*/
/* Fundamental Command */
#define SSD1306_CMD_ENTIRE_ON           0xA4
#define SSD1306_CMD_DISPLAY_ON          0xAE

/* Addressing Setting Command */
#define SSD1306_CMD_ADDR_MODE           0x20
#define SSD1306_CMD_COL_ADDR            0x21
#define SSD1306_CMD_ROW_ADDR            0x22
enum
{
    HORIZONTAL,
    VERTICAL,
    PAGE
};

#define SSD1306_CMD_COL_LO_ADDR         0x00
#define SSD1306_CMD_COL_HI_ADDR         0x10
#define SSD1306_CMD_PAGE_START_ADDR     0xB0

/* Charge Pump Command */
#define SSD1306_CMD_CHARGEPUMP          0x8D

/*============================================================================*/
void ssd1306_cmd(uint8_t *cmd, uint8_t len)
{
    twi_write(SSD1306_I2C_ADDR, SSD1306_I2C_CMD, cmd, len);
}

void ssd1306_data(uint8_t *data, uint16_t len)
{
    twi_write(SSD1306_I2C_ADDR, SSD1306_I2C_DATA, data, len);
}

void ssd1306_reset(void)
{
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

static void ssd1306_set_addr_mode(uint8_t mode)
{
    uint8_t     cmd_buf[2];

    cmd_buf[0] = SSD1306_CMD_ADDR_MODE;
    cmd_buf[1] = mode;

    ssd1306_cmd(cmd_buf, 2);
}

static void ssd1306_set_page_start(uint8_t page)
{
    uint8_t     cmd = SSD1306_CMD_PAGE_START_ADDR | page;

    ssd1306_cmd(&cmd, 1);
}

static void ssd1306_set_column_addr(uint8_t column)
{
    uint8_t     cmd;
 
    cmd = SSD1306_CMD_COL_LO_ADDR | (column & 0xF);
    ssd1306_cmd(&cmd, 1);
 
    cmd = SSD1306_CMD_COL_HI_ADDR | ((column >> 4) & 0xF);
    ssd1306_cmd(&cmd, 1);
}


void ssd1306_init(void)
{
    _delay_ms(1);

    ssd1306_reset();

    ssd1306_chargepump_enable(true);
    ssd1306_entire_on(false);
    ssd1306_display_on(true);
}

/*============================================================================*/
#define PAGE_NUM        8
#define COL_NUM         128
#define CHAR_WIDTH      6
#define MAX_CH_PER_LINE 21

static uint8_t gddram[COL_NUM];

void clear_screen(void)
{
    uint8_t page;
    uint8_t col;

    for (col=0; col<COL_NUM; col++)
        gddram[col] = 0;

    for (page=0; page<PAGE_NUM; page++)
    {
        ssd1306_set_page_start(page);
        ssd1306_data(gddram, COL_NUM);
    }

    ssd1306_set_column_addr(0);
}

extern void get_ch_data(char ch, uint8_t *buffer);
int oled_msg(int row, int col, const char *fmt, ...)
{
    va_list     ap;
    char        buf[MAX_CH_PER_LINE + 1];
    int         rc, i;

    va_start(ap, fmt);
    rc = vsprintf(buf, fmt, ap);
    va_end(ap);

    if (rc > 0)
    {
        for (i=0; i<rc; i++)
            get_ch_data(buf[i], &gddram[i * CHAR_WIDTH]);

        ssd1306_set_page_start(row);
        ssd1306_set_column_addr(col * CHAR_WIDTH);
        ssd1306_data(gddram, rc * CHAR_WIDTH);
    }

    return rc;
}


void setup(void)
{
    twi_init();

    ssd1306_init();
    ssd1306_set_addr_mode(PAGE);

    clear_screen();

    oled_msg(0, 0, "> Positioned Text <");
    oled_msg(2, 2, "Even :");
    oled_msg(3, 2, "Odd  :");
	oled_msg(4, 2, "KO Myoung-min");
}

void loop(void)
{
    static int loop = 0;

    oled_msg(2, 9, "%6d", loop * 2);
    oled_msg(3, 9, "%6d", loop * 2 + 1);
    loop++;

    _delay_ms(1000);
}

int main(void)
{
    setup();

    while (1)
        loop();
}

