#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "i2c.h"
#include <avr/pgmspace.h>




#define CMD_NEXT	0x01
#define CMD_PRE		0x02
#define CMD_PLAY	0x0d
#define CMD_PAUSE	0x0E
#define CMD_STOP	0x16
#define CMD_TF		0x09

char cmd_list[10] = {CMD_PLAY, CMD_STOP, CMD_PAUSE, CMD_NEXT, CMD_PRE,};


void UART_Init(void)
{
	UCSR0B = 0x18;
	UCSR0C = 0x06;
	UBRR0L = 103;

	//UCSR1B = 0x08;
	//UCSR1C = 0x06;
	//UBRR1L = 103;
}

void UART0_Putch(char ch)
{
	while(!(UCSR0A & 0x20));

	UDR0 = ch;
}

void SendCommand(unsigned char cmd)
{
	unsigned int checksum = 0;
	char temp[20];

	UART0_Putch(0x7E);
	UART0_Putch(0xFF);
	UART0_Putch(0x06);
	UART0_Putch(cmd);
	UART0_Putch(0x00);
	UART0_Putch(0x00);
	
	if(cmd != CMD_TF){
		UART0_Putch(0x00);
		checksum = 0 - (0xFF + 0x06 + cmd);
	}
	else{
		UART0_Putch(0x02);
		checksum = 0 - (0xFF + 0x06 + cmd + 0x02);
	}

	UART0_Putch((char)(checksum >> 8));
	UART0_Putch(checksum&0xFF);
	UART0_Putch(0xEF);
}

void SendCommand01(unsigned char cmd, unsigned char param1, unsigned char param2)
{
	unsigned int checksum = 0;
	char temp[20];

	UART0_Putch(0x7E);
	UART0_Putch(0xFF);
	UART0_Putch(0x06);
	UART0_Putch(cmd);
	UART0_Putch(0x00);
	UART0_Putch(param1);
	UART0_Putch(param2);
	

	checksum = 0 - (0xFF + 0x06 + cmd + param1 + param2);

	UART0_Putch((char)(checksum >> 8));
	UART0_Putch(checksum&0xFF);
	UART0_Putch(0xEF);
}


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


static void ssd1306_reverse(bool enable)
{
	
	uint8_t cmd = 0xa0 | enable;

	ssd1306_cmd(&cmd, 1);
}

static void ssd1306_normal_display(bool enable)
{
	uint8_t cmd = 0xa6 | enable;

	ssd1306_cmd(&cmd, 1);
}

static void ssd1306_pins_hardware_configuration()
{
	uint8_t cmd = 0xda | 0x12;

	ssd1306_cmd(&cmd, 1);

}

static void ssd1306_output_scan_direction(bool enable)
{
	uint8_t cmd = 0xc8 | enable;

	ssd1306_cmd(&cmd, 1);
}

static void ssd1306_output_scan_direction2(bool enable)
{
	uint8_t cmd = 0xc0 | enable;

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
unsigned char screen[1024];






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


extern void get_ch_data2(char ch, uint8_t *buffer);
int oled_msg2(int row, int col, const char *fmt, ...)
{
	va_list     ap;
	char        buf[MAX_CH_PER_LINE + 1] = {0};
	int         rc, i;

	va_start(ap, fmt);
	rc = vsprintf(buf, fmt, ap);
	va_end(ap);

	if (rc > 0)
	{
		for (i=0; i<rc; i++)
		get_ch_data2(buf[i], &gddram[i * 9]);

		ssd1306_set_page_start(row);
		ssd1306_set_column_addr(col * 9);
		ssd1306_data(gddram, rc * 9);
	}

	return rc;
}


const unsigned char KO [] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x40, 0x40, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x40, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x12, 0x12, 0x00, 0x00, 0x1F,
	0x01, 0x03, 0x1E, 0x01, 0x1F, 0x00, 0x00, 0x1F, 0x11, 0x11, 0x1F, 0x00, 0x04, 0x1F, 0x15, 0x15,
	0x06, 0x00, 0x1F, 0x11, 0x11, 0x1F, 0x00, 0x04, 0x1F, 0x15, 0x15, 0x06, 0x00, 0x1F, 0x11, 0x11,
	0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x13, 0x12, 0x1E, 0x08, 0x41, 0x4E, 0x38, 0x0E, 0x01, 0x00,
	0x17, 0x15, 0x09, 0x01, 0x1F, 0x11, 0x04, 0x1F, 0x15, 0x15, 0x06, 0x00, 0x1F, 0x01, 0x03, 0x1E,
	0x01, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0xE0, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x00, 0x00, 0x00, 0x00, 0xE0, 0xE0, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
	0x03, 0x03, 0x03, 0x03, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xFF, 0xFF, 0xFF,
	0xE7, 0xE7, 0xE7, 0xE7, 0xE7, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x03, 0x03, 0x03, 0x03, 0x03, 0xFF, 0xFF, 0xFF,
	0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x1C, 0x1F, 0x1F, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x03, 0x03, 0xC3, 0xE3, 0xF3, 0xF3, 0x7B, 0x3B, 0x3B, 0x3B,
	0x38, 0x38, 0x38, 0x38, 0x70, 0xF7, 0xE7, 0xE7, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x07, 0x07, 0xE7, 0xE7, 0xE7, 0x07, 0x07, 0x07, 0x07, 0x07,
	0x07, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x0F, 0x1F, 0x1E, 0x3C, 0x38, 0x38, 0x38,
	0x38, 0x38, 0x38, 0x38, 0x1C, 0x1F, 0x0F, 0x0F, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x38, 0x38, 0x38, 0x38, 0x38,
	0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};







void draw_picture(char array[])
{
	unsigned char x,y;
	for(y=0;y<8;y++)
	{
		ssd1306_set_page_start(y);
		ssd1306_data(array+(y*COL_NUM), COL_NUM);
	}
}

void setup(void)
{
	twi_init();

	ssd1306_init();
	ssd1306_set_addr_mode(PAGE);
	

	clear_screen();
	
	ssd1306_output_scan_direction(0);		//위아래 역전
	ssd1306_reverse(1);		//왼쪽 오른쪽 역전
	ssd1306_normal_display(0);

}

__flash const unsigned int tetriminos[7][4] = {

	{0x6600, 0x6600, 0x6600, 0x6600},    // O
	{0xF000, 0x4444, 0xF000, 0x4444},    // I
	{0x6C00, 0x8C40, 0x6C00, 0x8C40},    // S
	{0xC600, 0x4C80, 0xC600, 0x4C80},    // Z
	{0x4E00, 0x8C80, 0xE400, 0x4C40},    // T
	{0x2E00, 0xC440, 0xE800, 0x88C0},    // L
	{0x8E00, 0x44C0, 0xE200, 0xC880},    // J

};


unsigned char shape;                  // 테트리미노스의 7가지 모양
unsigned char pattern;                 // 테트리미노스의 4가지 패턴
unsigned char cur_line;               // 테트리니노스의 현재 라인
unsigned char cur_col;                // 테트리니노스의 현재 칸
unsigned long int temp_line[4];          // 테트리미노스 라인 임시 저장소
unsigned long int main_board[32] = {0};	//테르리미노스가 굳어진 후 저장된 게임보드
unsigned long int game_board[32];	//테트리미노스가 움직이면서 변화하는 게임보드
unsigned char crush = 0;	//부딪힘을 나타내는 플레그
unsigned char new_block = 0;	//새로운 블록이 생성되야함을 나타내는 플레그
unsigned char game_over = 0;	//게임이 종료되었음을 나타내는 플레그
unsigned char next_block = 0;
unsigned char next_board[8] = {0};

static uint8_t horizon1[1] = {0x01};
static uint8_t horizon2[1] = {0x80};
static uint8_t vertical[1] = {0b01010101};
static uint8_t blank[1] = {0};

unsigned char Collision()	//충돌 여부 확인
{
	if( ((main_board[cur_line] & temp_line[0]) != 0) | ((main_board[cur_line + 1] & temp_line[1]) != 0) |
	((main_board[cur_line + 2] & temp_line[2]) != 0) | ((main_board[cur_line + 3] & temp_line[3]) != 0) )
	return 1;         // 충돌 1 리턴
	else
	return 0;  // 충돌 없음 0 리턴
}



ISR(INT4_vect)
{
	for(int i = 0; i < 32; i++)
	{
		game_board[i] = main_board[i];	//굳어진후 저장된 보드를 변화하는 보드로 복사
	}

	cur_col--;

	for(int i = 0; i < 4; i++)	//테트리미노스 라인 임시 저장소 초기화
	{
		temp_line[i] = 0;
	}

	if(cur_col < 3)
	{
		temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0xF000) >> (15 - cur_col));
		temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0x0F00) >> (11 - cur_col));
		temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0x00F0) >> (7 - cur_col));
		temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0x000F) >> (3 - cur_col));
	}
	else
	{
		temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0xF000) >> 12) << (cur_col - 3);	//이 과정이 필요 없고 뒤에서 한번에 충돌감지 이후 넣어줘도 상관없어 보이지만 충돌 여부 판별하는 함수가 temp_line와 메인보드가 겹치는지 파악하기때문에 temp_line에 변화된 사항을 반영해야 한다.
		temp_line[1] = ((unsigned long int)(tetriminos[shape][pattern] & 0x0F00) >> 8) << (cur_col - 3);
		temp_line[2] = ((unsigned long int)(tetriminos[shape][pattern] & 0x00F0) >> 4) << (cur_col - 3);
		temp_line[3] = ((unsigned long int)(tetriminos[shape][pattern] & 0x000F)) << (cur_col - 3);
	}
	
	if(Collision() == 1) cur_col++;

	for(int i = 0; i < 4; i++)	//테트리미노스 라인 임시 저장소 초기화
	{
		temp_line[i] = 0;
	}

	if(cur_col < 3)
	{
		temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0xF000) >> (15 - cur_col));
		temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0x0F00) >> (11 - cur_col));
		temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0x00F0) >> (7 - cur_col));
		temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0x000F) >> (3 - cur_col));
	}
	else
	{
		temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0xF000) >> 12) << (cur_col - 3);	//이 과정이 필요 없고 뒤에서 한번에 충돌감지 이후 넣어줘도 상관없어 보이지만 충돌 여부 판별하는 함수가 temp_line와 메인보드가 겹치는지 파악하기때문에 temp_line에 변화된 사항을 반영해야 한다.
		temp_line[1] = ((unsigned long int)(tetriminos[shape][pattern] & 0x0F00) >> 8) << (cur_col - 3);
		temp_line[2] = ((unsigned long int)(tetriminos[shape][pattern] & 0x00F0) >> 4) << (cur_col - 3);
		temp_line[3] = ((unsigned long int)(tetriminos[shape][pattern] & 0x000F)) << (cur_col - 3);
	}

	
	game_board[cur_line] |= temp_line[0];	//현재라인 아래로 이동 후 변화하는 보드에 반영
	game_board[cur_line + 1] |= temp_line[1];
	game_board[cur_line + 2] |= temp_line[2];
	game_board[cur_line + 3] |= temp_line[3];
	
}

ISR(INT5_vect)
{
	for(int i = 0; i < 32; i++)
	{
		game_board[i] = main_board[i];	//굳어진후 저장된 보드를 변화하는 보드로 복사
	}

	pattern++;	//회전으로 상태 변화
	if(pattern == 4) pattern = 0; //마지막에서 처음으로

	for(int i = 0; i < 4; i++)	//테트리미노스 라인 임시 저장소 초기화
	{
		temp_line[i] = 0;
	}

	if(cur_col < 3)
	{
		temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0xF000) >> (15 - cur_col));
		temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0x0F00) >> (11 - cur_col));
		temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0x00F0) >> (7 - cur_col));
		temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0x000F) >> (3 - cur_col));
	}
	else
	{
		temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0xF000) >> 12) << (cur_col - 3);	//이 과정이 필요 없고 뒤에서 한번에 충돌감지 이후 넣어줘도 상관없어 보이지만 충돌 여부 판별하는 함수가 temp_line와 메인보드가 겹치는지 파악하기때문에 temp_line에 변화된 사항을 반영해야 한다.
		temp_line[1] = ((unsigned long int)(tetriminos[shape][pattern] & 0x0F00) >> 8) << (cur_col - 3);
		temp_line[2] = ((unsigned long int)(tetriminos[shape][pattern] & 0x00F0) >> 4) << (cur_col - 3);
		temp_line[3] = ((unsigned long int)(tetriminos[shape][pattern] & 0x000F)) << (cur_col - 3);
	}

	if(Collision() == 1) pattern--;


	for(int i = 0; i < 4; i++)	//테트리미노스 라인 임시 저장소 초기화
	{
		temp_line[i] = 0;
	}

	if(cur_col < 3)
	{
		temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0xF000) >> (15 - cur_col));
		temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0x0F00) >> (11 - cur_col));
		temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0x00F0) >> (7 - cur_col));
		temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0x000F) >> (3 - cur_col));
	}
	else
	{
		temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0xF000) >> 12) << (cur_col - 3);	//이 과정이 필요 없고 뒤에서 한번에 충돌감지 이후 넣어줘도 상관없어 보이지만 충돌 여부 판별하는 함수가 temp_line와 메인보드가 겹치는지 파악하기때문에 temp_line에 변화된 사항을 반영해야 한다.
		temp_line[1] = ((unsigned long int)(tetriminos[shape][pattern] & 0x0F00) >> 8) << (cur_col - 3);
		temp_line[2] = ((unsigned long int)(tetriminos[shape][pattern] & 0x00F0) >> 4) << (cur_col - 3);
		temp_line[3] = ((unsigned long int)(tetriminos[shape][pattern] & 0x000F)) << (cur_col - 3);
	}

	game_board[cur_line] |= temp_line[0];	//현재라인 아래로 이동 후 변화하는 보드에 반영
	game_board[cur_line + 1] |= temp_line[1];
	game_board[cur_line + 2] |= temp_line[2];
	game_board[cur_line + 3] |= temp_line[3];
}

ISR(INT6_vect)
{
	for(int i = 0; i < 4; i++)	//테트리미노스 라인 임시 저장소 초기화
	{
		temp_line[i] = 0;
	}

	if(cur_col < 3)
	{
		temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0xF000) >> (15 - cur_col));
		temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0x0F00) >> (11 - cur_col));
		temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0x00F0) >> (7 - cur_col));
		temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0x000F) >> (3 - cur_col));
	}
	else
	{
		temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0xF000) >> 12) << (cur_col - 3);	//이 과정이 필요 없고 뒤에서 한번에 충돌감지 이후 넣어줘도 상관없어 보이지만 충돌 여부 판별하는 함수가 temp_line와 메인보드가 겹치는지 파악하기때문에 temp_line에 변화된 사항을 반영해야 한다.
		temp_line[1] = ((unsigned long int)(tetriminos[shape][pattern] & 0x0F00) >> 8) << (cur_col - 3);
		temp_line[2] = ((unsigned long int)(tetriminos[shape][pattern] & 0x00F0) >> 4) << (cur_col - 3);
		temp_line[3] = ((unsigned long int)(tetriminos[shape][pattern] & 0x000F)) << (cur_col - 3);
	}

	while(Collision() == 0) cur_line++;

	cur_line--;
}

ISR(INT7_vect)
{	
	for(int i = 0; i < 32; i++)
	{
		game_board[i] = main_board[i];	//굳어진후 저장된 보드를 변화하는 보드로 복사
	}

	cur_col++;

	for(int i = 0; i < 4; i++)	//테트리미노스 라인 임시 저장소 초기화
	{
		temp_line[i] = 0;
	}

	if(cur_col < 3)
	{
		temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0xF000) >> (15 - cur_col));
		temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0x0F00) >> (11 - cur_col));
		temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0x00F0) >> (7 - cur_col));
		temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0x000F) >> (3 - cur_col));
	}
	else
	{
		temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0xF000) >> 12) << (cur_col - 3);	//이 과정이 필요 없고 뒤에서 한번에 충돌감지 이후 넣어줘도 상관없어 보이지만 충돌 여부 판별하는 함수가 temp_line와 메인보드가 겹치는지 파악하기때문에 temp_line에 변화된 사항을 반영해야 한다.
		temp_line[1] = ((unsigned long int)(tetriminos[shape][pattern] & 0x0F00) >> 8) << (cur_col - 3);
		temp_line[2] = ((unsigned long int)(tetriminos[shape][pattern] & 0x00F0) >> 4) << (cur_col - 3);
		temp_line[3] = ((unsigned long int)(tetriminos[shape][pattern] & 0x000F)) << (cur_col - 3);
	}

	if(Collision() == 1) cur_col--;
	
	for(int i = 0; i < 4; i++)	//테트리미노스 라인 임시 저장소 초기화
	{
		temp_line[i] = 0;
	}

	if(cur_col < 3)
	{
		temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0xF000) >> (15 - cur_col));
		temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0x0F00) >> (11 - cur_col));
		temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0x00F0) >> (7 - cur_col));
		temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0x000F) >> (3 - cur_col));
	}
	else
	{
		temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0xF000) >> 12) << (cur_col - 3);	//이 과정이 필요 없고 뒤에서 한번에 충돌감지 이후 넣어줘도 상관없어 보이지만 충돌 여부 판별하는 함수가 temp_line와 메인보드가 겹치는지 파악하기때문에 temp_line에 변화된 사항을 반영해야 한다.
		temp_line[1] = ((unsigned long int)(tetriminos[shape][pattern] & 0x0F00) >> 8) << (cur_col - 3);
		temp_line[2] = ((unsigned long int)(tetriminos[shape][pattern] & 0x00F0) >> 4) << (cur_col - 3);
		temp_line[3] = ((unsigned long int)(tetriminos[shape][pattern] & 0x000F)) << (cur_col - 3);
	}

	
	game_board[cur_line] |= temp_line[0];	//현재라인 아래로 이동 후 변화하는 보드에 반영
	game_board[cur_line + 1] |= temp_line[1];
	game_board[cur_line + 2] |= temp_line[2];
	game_board[cur_line + 3] |= temp_line[3];
}






ISR(TIMER1_COMPA_vect)	//OCR1A값에 따라 블록 이동속도가 결정된다.
{
	if(new_block == 0)
	{
		for(int i = 0; i < 32; i++)
		{
			game_board[i] = main_board[i];	//굳어진후 저장된 보드를 변화하는 보드로 복사
		}

		cur_line++;	//현재라인 아래로 이동
		
		if(Collision() == 1)	//이동후 충돌 발생시
		{
			cur_line--;	//원래 라인으로 복귀
			main_board[cur_line] |= temp_line[0];	//현재라인 아래로 이동 후 변화하는 보드에 반영
			main_board[cur_line + 1] |= temp_line[1];
			main_board[cur_line + 2] |= temp_line[2];
			main_board[cur_line + 3] |= temp_line[3];

			new_block = 1;	//새로운 블록 플레그 켜짐
		}

		for(int i = 0; i < 32; i++)
		{
			game_board[i] = main_board[i];	//굳어진후 저장된 보드를 변화하는 보드로 복사
		}

		game_board[cur_line] |= temp_line[0];	//현재라인 아래로 이동 후 변화하는 보드에 반영
		game_board[cur_line + 1] |= temp_line[1];
		game_board[cur_line + 2] |= temp_line[2];
		game_board[cur_line + 3] |= temp_line[3];

	}
}





void clear_screen(void)
{
	uint8_t page;

	for(int i = 0; i < 1024; i++) screen[i] = 0;

	for (page=0; page<PAGE_NUM; page++)
	{
		ssd1306_set_page_start(page);
		ssd1306_data(screen+(128 * page), COL_NUM);
	}

	ssd1306_set_column_addr(0);
}

void clear_map()
{
	unsigned char MAP [6][128] = {0};

	for(int i = 0; i < 6; i++)
	{
		for(int j = 0; j < 128; j++)
		{
			if(j == 0) MAP[i][j] = vertical[0];
			else if(j == 127) MAP[i][j] = vertical[0];
			else if(i == 0) MAP[i][j] = horizon1[0];
			else if(i == 5) MAP[i][j] = horizon2[0];
		}
	}

	for(unsigned char y=2;y<8;y++)
	{
		ssd1306_set_page_start(y);
		ssd1306_data(MAP[y - 2], COL_NUM);
	}
}


void clear_map2()
{
	unsigned char MAP [8][128] = {0};

	for(int i = 2; i < 8; i++)
	{
		for(int j = 0; j < 128; j++)
		{
			if(j == 0) MAP[i][j] = vertical[0];
			else if(j == 127) MAP[i][j] = vertical[0];
			else if(i == 2) MAP[i][j] = horizon1[0];
			else if(i == 7) MAP[i][j] = horizon2[0];
		}
	}

	for(unsigned char y=0;y<8;y++)
	{
		ssd1306_set_page_start(y);
		ssd1306_data(MAP[y], COL_NUM);
	}
}













	

void draw_map()
{
	unsigned char MAP [6][128] = {0};
	for(int i=0;i<12;i++)
	{
		for(int j = 0; j < 32; j++)
		{
			if((game_board[j] & (unsigned long int)1 << i) != 0)
			{
				if(i % 2 == 0)
				{
					MAP[i/2][4*j] |= 0x0F;
					MAP[i/2][4*j + 1] |= 0x0F;
					MAP[i/2][4*j + 2] |= 0x0F;
					MAP[i/2][4*j + 3] |= 0x0F;
				}
				else
				{
					MAP[i/2][4*j] |= 0xF0;
					MAP[i/2][4*j + 1] |= 0xF0;
					MAP[i/2][4*j + 2] |= 0xF0;
					MAP[i/2][4*j + 3] |= 0xF0;
				}
			}
		}
	}


	for(unsigned char y=2;y<8;y++)
	{
		ssd1306_set_page_start(y);
		ssd1306_data(MAP[y - 2], COL_NUM);
	}
}


	void NewTetriminos()
	{
		new_block = 0;	//새로운 블록 플레그 끄기
		shape = next_block;
		next_block = TCNT0 % 7;					//다음에 올 테트리미노스 랜덤 출력
		pattern = 0;	//기본 회전모향 설정
		cur_line = 0;                 // 테트리미노스 현재 라인 (최상위 라인)
		cur_col = 6;                // 테트리미노스의 현재 칸

		for(int i = 0; i < 4; i++)	//테트리미노스 라인 임시 저장소 초기화
		{
			temp_line[i] = 0;
		}

		if(cur_col < 3)
		{
			temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0xF000) >> (15 - cur_col));
			temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0x0F00) >> (11 - cur_col));
			temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0x00F0) >> (7 - cur_col));
			temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0x000F) >> (3 - cur_col));
		}
		else
		{
			temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0xF000) >> 12) << (cur_col - 3);	//이 과정이 필요 없고 뒤에서 한번에 충돌감지 이후 넣어줘도 상관없어 보이지만 충돌 여부 판별하는 함수가 temp_line와 메인보드가 겹치는지 파악하기때문에 temp_line에 변화된 사항을 반영해야 한다.
			temp_line[1] = ((unsigned long int)(tetriminos[shape][pattern] & 0x0F00) >> 8) << (cur_col - 3);
			temp_line[2] = ((unsigned long int)(tetriminos[shape][pattern] & 0x00F0) >> 4) << (cur_col - 3);
			temp_line[3] = ((unsigned long int)(tetriminos[shape][pattern] & 0x000F)) << (cur_col - 3);
		}

		
		game_over |= game_board[cur_line] & temp_line[0];	//게임보드에 있는 테트리미노스와 임시저장소에 생긴 테트리미노스가 겹치는지 확인하고 겹치면 게임오버 플레그 켜짐
		game_over |= game_board[cur_line + 1] & temp_line[1];	//or 연산으로 어디든 겹치면 플레그 켜진다
		game_over |= game_board[cur_line + 2] & temp_line[2];
		game_over |= game_board[cur_line + 3] & temp_line[3];
		

		game_board[cur_line] |= temp_line[0];	//현재라인 아래로 이동 후 변화하는 보드에 반영
		game_board[cur_line + 1] |= temp_line[1];
		game_board[cur_line + 2] |= temp_line[2];
		game_board[cur_line + 3] |= temp_line[3];

	}


	void NextTetriminos()
	{
		for(int i = 0; i < 8; i++)	//넥스트 보드 초기화
		{
			next_board[i] = 0;
		}

		
		next_board[2] |= (unsigned char)((tetriminos[next_block][0] & 0xF000) >> 10);
		next_board[3] |= (unsigned char)((tetriminos[next_block][0] & 0x0F00) >> 6);
		next_board[4] |= (unsigned char)((tetriminos[next_block][0] & 0x00F0) >> 2);
		next_board[5] |= (unsigned char)((tetriminos[next_block][0] & 0x000F) << 2);


		unsigned char MAP [2][128] = {0};
		

		MAP[0][2] |= 0xFF;	//위에 보드 2칸 줄은 너무 두꺼워서 한줄로 만들었다
		MAP[1][2] |= 0xFF;
		MAP[0][17] |= 0xFF;
		MAP[1][17] |= 0xFF;

		for(int i = 2; i < 18; i++)
		{
			MAP[0][i] |= 0x01;
			MAP[1][i] |= 0x80;
		}


		for(int i = 0;i < 8; i++)
		{
			for(int j = 0; j < 8; j++)
			{
				if((next_board[i] & (1 << j)) != 0)
				{
					if(j < 4)
					{
						MAP[1][2 + (2 * i)] |= (0x03 << 2 * (3 - j));
						MAP[1][2 + (2 * i + 1)] |= (0x03 << 2 * (3 - j));
					}
					else
					{
						MAP[0][2 + (2 * i)] |= (0x03 << 2 * (7 - j));
						MAP[0][2 + (2 * i + 1)] |= (0x03 << 2 * (7 - j));
					}
				}
			}
		}

		for(unsigned char y=0;y<2;y++)
		{
			ssd1306_set_page_start(y);
			ssd1306_data(MAP[y], 128);
		}
	}

	


	







int main(void)
{
	char cmd;

	UART_Init();

	TCCR0 = 0x07;	//타이머 카운터 0 1024분주로 켜기

	TCCR1A |= (0 << COM1A0) | (0 << WGM10);	//CTC, OC1A핀 차단
	TCCR1B |= (1 << WGM12) | (5 << CS10); //CTC, 1024분주
	OCR1A = 0xFFF;
	TIMSK |= (1 << OCIE1A);	//타이머 카운터 1 출력비교 A 매치 인터럽트 인에이블

	DDRD = 0x00;	//인터럽트 4,5,6,7 켜기위해 포트 입력으로
	DDRE = 0x00;	//인터럽트 3 켜기위해 포트 입력으로

	EICRB = (2 << ISC40) | (2 << ISC50) | (2 << ISC60) | (2 << ISC70);
	
	EICRA = (2 << ISC30);

	EIMSK = (1 << INT3) | (1 << INT4) | (1 << INT5) | (1 << INT6) | (1 << INT7);

	sei();

	setup();

	while(1)
	{

		game_over = 0;	//게임종료 플레그 끄기


		for (int i = 0; i < 31; i++ ) main_board[i] = 0x801;	//메인보드 초기화
		main_board[31] = 0xFFF;




		while(game_over == 0)	//게임종료 플레그가 꺼저있을동안 반복
		{
			for(int i = 0; i < 31; i++)
			{
				if(main_board[i] == 0xFFF)
				//1줄이 모두 완성되어서 깨질 줄이 있는지 확인
				{
					main_board[i] = 0x801;
					for(int k = i; k > 0; k--)
					{
						main_board[k] = main_board[k - 1];	//깨진 줄의 위에 줄들 아래로 이동(행렬상 열 증가)
					}
				}
			}

			for(int i = 0; i < 32; i++)
			{
				game_board[i] = main_board[i];	//굳어진후 저장된 보드를 변화하는 보드로 복사
			}
			
			NewTetriminos();	//새로운 테트리미노스 생성
			NextTetriminos();
			while(new_block == 0)	//새로운 블록 프레그 꺼져있는 동안 반복
			{
				draw_map();	//반영된 변화하는 보드 화면으로 출력
			}
		}
	}
}
		
