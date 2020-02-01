#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "i2c.h"





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




#define LEFT 75
#define RIGHT 77
#define UP 72
#define DOWN 80

unsigned char screen = 0;


enum
{
	Menu_snake,
	Menu_tetris,
	Play_snake,
	Play_tetris,
	Over_snake,
	Over_tetris,
	Ready
};


int food_x, food_y;
int length = 3;
int dir = RIGHT;



static uint8_t horizon1[1] = {0x01};
static uint8_t horizon2[1] = {0x80};
static uint8_t vertical[1] = {0b01010101};
static uint8_t blank[1] = {0};

uint8_t x[200] = {32, 31, 30};
uint8_t y[200] = {12, 12, 12};


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


unsigned char Collision()	//충돌 여부 확인
{
	if( ((main_board[cur_line] & temp_line[0]) != 0) | ((main_board[cur_line + 1] & temp_line[1]) != 0) |
	((main_board[cur_line + 2] & temp_line[2]) != 0) | ((main_board[cur_line + 3] & temp_line[3]) != 0) )
	return 1;         // 충돌 1 리턴
	else
	return 0;  // 충돌 없음 0 리턴
}


void tetriminos_to_temp_line()
{
	for(int i = 0; i < 4; i++)	//테트리미노스 라인 임시 저장소 초기화
	{
		temp_line[i] = 0;
	}

	if(cur_col == 2)
	{
		temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0xF000) >> (15 - cur_col));
		temp_line[1] = ((unsigned long int)(tetriminos[shape][pattern] & 0x0F00) >> (11 - cur_col));
		temp_line[2] = ((unsigned long int)(tetriminos[shape][pattern] & 0x00F0) >> (7 - cur_col));
		temp_line[3] = ((unsigned long int)(tetriminos[shape][pattern] & 0x000F) >> (3 - cur_col));
	}
	else if(cur_col >= 3)
	{
		temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0xF000) >> 12) << (cur_col - 3);	//이 과정이 필요 없고 뒤에서 한번에 충돌감지 이후 넣어줘도 상관없어 보이지만 충돌 여부 판별하는 함수가 temp_line와 메인보드가 겹치는지 파악하기때문에 temp_line에 변화된 사항을 반영해야 한다.
		temp_line[1] = ((unsigned long int)(tetriminos[shape][pattern] & 0x0F00) >> 8) << (cur_col - 3);
		temp_line[2] = ((unsigned long int)(tetriminos[shape][pattern] & 0x00F0) >> 4) << (cur_col - 3);
		temp_line[3] = ((unsigned long int)(tetriminos[shape][pattern] & 0x000F)) << (cur_col - 3);
	}
}




void snake_title();
void snake_draw_map();
void snake_move(int dir);
void snake_game_over();
void food();
void snake_clear_map();




ISR(INT4_vect)
{
	SendCommand01(0x0F, 0x01, 0x03);
	if(screen == Menu_snake)
	{
		screen = Play_snake;
	}
	else if(screen == Menu_tetris)
	{
		screen = Play_tetris;
	}
	else if(screen == Play_snake)
	{
		if(dir != LEFT)
		{
			//SendCommand01(0x0F, 0x01, 0x03);
			
			dir = RIGHT;
		}
	}
	else if(screen == Play_tetris)
	{
		for(int i = 0; i < 32; i++)
		{
			game_board[i] = main_board[i];	//굳어진후 저장된 보드를 변화하는 보드로 복사
		}

		if(cur_col >= 3) cur_col--;
		else cur_col = 2;

		tetriminos_to_temp_line();
		
		if(Collision() == 1) cur_col++;

		tetriminos_to_temp_line();

		game_board[cur_line] |= temp_line[0];	//현재라인 아래로 이동 후 변화하는 보드에 반영
		game_board[cur_line + 1] |= temp_line[1];
		game_board[cur_line + 2] |= temp_line[2];
		game_board[cur_line + 3] |= temp_line[3];
	}
	else
	{
		screen = Ready;
	}
}




ISR(INT5_vect)
{
	SendCommand01(0x0F, 0x01, 0x03);
	if(screen == Menu_snake)
	{
		screen = Menu_tetris;
	}
	else if(screen == Menu_tetris)
	{
		screen = Menu_snake;
	}
	else if(screen == Play_snake)
	{
		if(dir != DOWN)
		{
			//SendCommand01(0x0F, 0x01, 0x03);
			
			dir = UP;
		}
	}
	else if(screen == Play_tetris)
	{
		for(int i = 0; i < 32; i++)
		{
			game_board[i] = main_board[i];	//굳어진후 저장된 보드를 변화하는 보드로 복사
		}

		pattern++;	//회전으로 상태 변화
		if(pattern == 4) pattern = 0; //마지막에서 처음으로

		tetriminos_to_temp_line();

		if(Collision() == 1) pattern--;


		tetriminos_to_temp_line();

		game_board[cur_line] |= temp_line[0];	//현재라인 아래로 이동 후 변화하는 보드에 반영
		game_board[cur_line + 1] |= temp_line[1];
		game_board[cur_line + 2] |= temp_line[2];
		game_board[cur_line + 3] |= temp_line[3];
	}
	
}

ISR(INT6_vect)
{
	SendCommand01(0x0F, 0x01, 0x03);
	if(screen == Menu_snake)
	{
		screen = Menu_tetris;
	}
	else if(screen == Menu_tetris)
	{
		screen = Menu_snake;
	}
	else if(screen == Play_snake)
	{
		if(dir != UP)
		{
			//SendCommand01(0x0F, 0x01, 0x03);
			
			dir = DOWN;
		}
	}
	else if(screen == Play_tetris)
	{
		for(int i = 0; i < 32; i++)
		{
			game_board[i] = main_board[i];	//굳어진후 저장된 보드를 변화하는 보드로 복사
		}
		
		tetriminos_to_temp_line();

		while(Collision() == 0) cur_line++;

		cur_line--;

		tetriminos_to_temp_line();

		game_board[cur_line] |= temp_line[0];	//현재라인 아래로 이동 후 변화하는 보드에 반영
		game_board[cur_line + 1] |= temp_line[1];
		game_board[cur_line + 2] |= temp_line[2];
		game_board[cur_line + 3] |= temp_line[3];
	}
}

ISR(INT7_vect)
{
	SendCommand01(0x0F, 0x01, 0x03);
	if(screen == Menu_snake)
	{
		screen = Play_snake;
	}
	else if(screen == Menu_tetris)
	{
		screen = Play_tetris;
	}
	else if(screen == Play_snake)
	{
		if(dir != RIGHT)
		{
			//SendCommand01(0x0F, 0x01, 0x03);
			
			dir = LEFT;
		}
	}
	else if(screen == Play_tetris)
	{
		for(int i = 0; i < 32; i++)
		{
			game_board[i] = main_board[i];	//굳어진후 저장된 보드를 변화하는 보드로 복사
		}

		cur_col++;

		tetriminos_to_temp_line();

		if(Collision() == 1) cur_col--;
		
		tetriminos_to_temp_line();

		
		game_board[cur_line] |= temp_line[0];	//현재라인 아래로 이동 후 변화하는 보드에 반영
		game_board[cur_line + 1] |= temp_line[1];
		game_board[cur_line + 2] |= temp_line[2];
		game_board[cur_line + 3] |= temp_line[3];
	}
	else
	{
		screen = Ready;
	}
}

ISR(TIMER1_COMPA_vect)	//OCR1A값에 따라 블록 이동속도가 결정된다.
{
	if(screen == Play_tetris)
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
}




void snake_title()
{
	oled_msg(0, 3, "KMM's SNAKE GAME");
	for(int i = 0; i < 128; i++)
	{
		ssd1306_set_page_start(2);
		ssd1306_set_column_addr(i);
		ssd1306_data(horizon1, 1);
		ssd1306_set_page_start(7);
		ssd1306_set_column_addr(i);
		ssd1306_data(horizon2, 1);
	}
	for(int i = 0; i < 8; i++)
	{
		ssd1306_set_page_start(i);
		ssd1306_set_column_addr(0);
		ssd1306_data(vertical, 1);
		ssd1306_set_column_addr(127);
		ssd1306_data(vertical, 1);
	}

	snake_draw_map();
	
	food(); // food 생성

}

void snake_draw_map()
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

	MAP[2*(food_y)/8][2*(food_x)] |= ((0x03 << (2*(food_y) % 8)));
	MAP[2*(food_y)/8][2*(food_x) + 1] |= ((0x03 << (2*(food_y) % 8)));


	for(int i=0;i<length;i++)	//뱀 몸통값 입력
	{
		MAP[2*(y[i])/8][2*(x[i])] |= ((0x03 << (2*(y[i]) % 8)));
		MAP[2*(y[i])/8][2*(x[i]) + 1] |= ((0x03 << (2*(y[i]) % 8)));
	}

	unsigned char x,y;
	for(y=2;y<8;y++)
	{
		ssd1306_set_page_start(y);
		ssd1306_data(MAP[y - 2], COL_NUM);
	}
}


void snake_move(int dir)
{
	
	

	if(x[0]==food_x&&y[0]==food_y)
	{ //food와 충돌했을 경우
		food(); //새로운 food 추가
		length++; //길이증가
		x[length-1]=x[length-2]; //새로만든 몸통에 값 입력
		y[length-1]=y[length-2];
	}
	if(x[0]==0||x[0]== 63||y[0]==0||y[0]==23)
	{ //벽과 충돌했을 경우
		snake_game_over();
		return; //game_over에서 게임을 다시 시작하게 되면 여기서부터 반복되므로
		//return을 사용하여 snake_move의 나머지 부분이 실행되지 않도록 합니다.
	}
	for(int i=1;i<length;i++)
	{ //자기몸과 충돌했는지 검사
		if(x[0]==x[i] && y[0]==y[i])
		{
			snake_game_over();
			return;
		}
	}
	

	for(int i=length - 1;i>0;i--){ //몸통좌표를 한칸씩 옮김
		x[i]=x[i-1];
		y[i]=y[i-1];
	}

	

	if(dir==LEFT) x[0]--; //방향에 따라 새로운 머리좌표(x[0],y[0])값을 변경
	else if(dir==RIGHT) x[0]++;
	else if(dir==UP) y[0]--;
	else if(dir==DOWN) y[0]++;
	

	
	
}

void food()
{
	int food_crush_on=0;//food가 뱀 몸통좌표에 생길 경우 on
	int r=0; //난수 생성에 사동되는 변수
	
	
	
	while(1){
		food_crush_on=0;
		srand(TCNT0); //난수표생성
		food_x=(rand()%62)+2;    //난수를 좌표값에 넣음
		food_y=(rand()%22)+2;
		
		for(int i=0;i<length;i++)
		{ //food가 뱀 몸통과 겹치는지 확인
			if(food_x==x[i]&&food_y==y[i])
			{
				food_crush_on=1; //겹치면 food_crush_on 를 on
				break;
			}
		}
		
		if(food_crush_on==1) continue; //겹쳤을 경우 while문을 다시 시작
		
		break;
		
	}
}


void snake_game_over()
{
	SendCommand01(0x06, 0x00, 0x0F);	//게임오버소리 너무 커서 절반으로 
	_delay_ms(200);
	SendCommand01(0x0F, 0x01, 0x04);
	snake_clear_map();

	ssd1306_set_column_addr(0);
	
	screen = Over_snake;
}




void Tetris_draw_map()
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

	tetriminos_to_temp_line();

	
	game_over |= game_board[cur_line] & temp_line[0];	//게임보드에 있는 테트리미노스와 임시저장소에 생긴 테트리미노스가 겹치는지 확인하고 겹치면 게임오버 플레그 켜짐
	game_over |= game_board[cur_line + 1] & temp_line[1];	//or 연산으로 어디든 겹치면 플레그 켜진다
	game_over |= game_board[cur_line + 2] & temp_line[2];
	game_over |= game_board[cur_line + 3] & temp_line[3];

	if(game_over != 0)
	{
		for (int i = 0; i < 31; i++ ) main_board[i] = 0x801;	//메인보드 초기화
		main_board[31] = 0xFFF;
		for (int i = 0; i < 31; i++ ) game_board[i] = 0x801;	//게임보드 초기화
		game_board[31] = 0xFFF;
		Tetris_draw_map();

		SendCommand01(0x06, 0x00, 0x0F);	//게임오버소리 너무 커서 절반으로 
		_delay_ms(200);
		SendCommand01(0x0F, 0x01, 0x04);

		screen = Over_tetris;
	}
	
	
	if(screen == Play_tetris)
	{
		game_board[cur_line] |= temp_line[0];	//현재라인 아래로 이동 후 변화하는 보드에 반영
		game_board[cur_line + 1] |= temp_line[1];
		game_board[cur_line + 2] |= temp_line[2];
		game_board[cur_line + 3] |= temp_line[3];
	}
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

	long int val_x;
	long int val_y;
	long int val_ADC_button;
	
	SendCommand(CMD_TF);
	_delay_ms(20);
	//SendCommand01(0x06, 0x00, 0x0F);
	//_delay_ms(200);
	SendCommand01(0x0F, 0x01, 0x01);
	_delay_ms(200);
	
	TCCR0 = 0x07;	//타이머 카운터 0 1024분주로 켜기

	TCCR1A |= (0 << COM1A0) | (0 << WGM10);	//CTC, OC1A핀 차단
	TCCR1B |= (1 << WGM12) | (5 << CS10); //CTC, 1024분주
	OCR1A = 0xFFF;
	

	DDRD = 0x00;	//인터럽트 4,5,6,7 켜기위해 포트 입력으로

	EICRB = (2 << ISC40) | (2 << ISC50) | (2 << ISC60) | (2 << ISC70);
	
	EICRA = (2 << ISC30);

	EIMSK = (1 << INT4) | (1 << INT5) | (1 << INT6) | (1 << INT7);

	sei();

	setup();
	draw_picture(KO);
	_delay_ms(1000);
	clear_screen();
	for(int i = 0; i < 128; i++)
	{
		ssd1306_set_page_start(2);
		ssd1306_set_column_addr(i);
		ssd1306_data(horizon1, 1);
		ssd1306_set_page_start(7);
		ssd1306_set_column_addr(i);
		ssd1306_data(horizon2, 1);
	}
	for(int i = 0; i < 8; i++)
	{
		ssd1306_set_page_start(i);
		ssd1306_set_column_addr(0);
		ssd1306_data(vertical, 1);
		ssd1306_set_column_addr(127);
		ssd1306_data(vertical, 1);
	}
	while(1)
	{
		if(screen == Ready)
		{
			SendCommand01(0x06, 0x00, 0x1E);	//게임 오버로 줄인 볼륨 다시 원상태
			clear_screen();
			for(int i = 0; i < 128; i++)
			{
				ssd1306_set_page_start(2);
				ssd1306_set_column_addr(i);
				ssd1306_data(horizon1, 1);
				ssd1306_set_page_start(7);
				ssd1306_set_column_addr(i);
				ssd1306_data(horizon2, 1);
			}
			for(int i = 0; i < 8; i++)
			{
				ssd1306_set_page_start(i);
				ssd1306_set_column_addr(0);
				ssd1306_data(vertical, 1);
				ssd1306_set_column_addr(127);
				ssd1306_data(vertical, 1);
			}
			screen = Menu_snake;
		}
		if(screen == Menu_snake)
		{	
			oled_msg(0, 3, "Myoungmin's GAME");
			oled_msg(3, 3, ">> SNAKE GAME");
			oled_msg(6, 3, "   TETRIS");
			ssd1306_set_page_start(0);
			ssd1306_set_column_addr(0);
		}
		else if (screen == Menu_tetris)
		{		
			oled_msg(0, 3, "Myoungmin's GAME");
			oled_msg(3, 3, "   SNAKE GAME");
			oled_msg(6, 3, ">> TETRIS");
			ssd1306_set_page_start(0);
			ssd1306_set_column_addr(0);
		}
		else if(screen == Play_snake)
		{
			TIMSK |= (0 << OCIE1A);	//타이머 카운터 1 출력비교 A 매치 인터럽트 인에이블
			x[0] = 32;
			x[1] = 31;
			x[2] = 30;
			y[0] = 12;
			y[1] = 12;
			y[2] = 12;

			for(int i = 3; i < 200; i++)
			{
				x[i] = 0;
			}
			for(int i = 3; i < 200; i++)
			{
				y[i] = 0;
			}

			length = 3;
			dir = RIGHT;

			clear_screen();

			snake_title();
			

			while(screen == Play_snake)
			{
				ADMUX = (1<<REFS0) | (0 << MUX0);//기준전압 AVCC 사용, ADC0 단극성 입력
				ADCSRA = (1<<ADEN) | (1<<ADSC) | (7 << ADPS0);	//ADCSRA = 0x87;// ADEN=1, 16MHz의 128분주 -> 125kHz
				
				while((ADCSRA & (1<<ADIF)) == 0);	//변환 종료를 기다림
				ADCSRA |= (1<<ADIF);	//ADIF 플래그를 지움
				
				val_x = ADC;
				if(val_x <=100 && dir != LEFT)
				{
					SendCommand01(0x0F, 0x01, 0x03);
					dir = RIGHT;
				}
				
				else if(val_x >=800 && dir != RIGHT)
				{
					SendCommand01(0x0F, 0x01, 0x03);
					dir = LEFT;
				}
				

				ADMUX = (1<<REFS0) | (1 << MUX0);//기준전압 AVCC 사용, ADC1 단극성 입력
				ADCSRA = (1<<ADEN) | (1<<ADSC) | (7 << ADPS0);	//ADCSRA = 0x87;// ADEN=1, 16MHz의 128분주 -> 125kHz
				
				while((ADCSRA & (1<<ADIF)) == 0);	//변환 종료를 기다림
				ADCSRA |= (1<<ADIF);	//ADIF 플래그를 지움

				val_y = ADC;
				if(val_y >=800 && dir != DOWN)
				{
					SendCommand01(0x0F, 0x01, 0x03);
					dir = UP;
				}
				else if(val_y <=100 && dir != UP)
				{
					SendCommand01(0x0F, 0x01, 0x03);
					dir = DOWN;
				}


				snake_move(dir);
				//SendCommand01(0x0F, 0x01, 0x02);
				snake_draw_map();
				_delay_ms(80);
			}
		}
		else if(screen == Play_tetris)
		{
			TIMSK |= (1 << OCIE1A);	//타이머 카운터 1 출력비교 A 매치 인터럽트 인에이블


			for (int i = 0; i < 31; i++ ) main_board[i] = 0x801;	//메인보드 초기화
			main_board[31] = 0xFFF;




			while(screen == Play_tetris)	//게임종료 플레그가 꺼저있을동안 반복
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
				if(screen != Play_tetris) break;
				NextTetriminos();
				while(new_block == 0)	//새로운 블록 프레그 꺼져있는 동안 반복
				{
					Tetris_draw_map();	//반영된 변화하는 보드 화면으로 출력
				}
			}
		}
		else if(screen == Over_snake)
		{
			clear_screen();
			while(screen == Over_snake)
			{
				oled_msg(0, 5, " SNAKE GAME");
				oled_msg(3, 5, "  G A M E ");
				oled_msg(6, 5, "  O V E R ");
				ssd1306_set_page_start(0);
				ssd1306_set_column_addr(0);
			}
			clear_screen();
		}
		else if(screen == Over_tetris)
		{
			game_over = 0;
			clear_screen();
			while(screen == Over_tetris)
			{
				oled_msg(0, 5, "   TETRIS ");
				oled_msg(3, 5, "  G A M E ");
				oled_msg(6, 5, "  O V E R ");
				ssd1306_set_page_start(0);
				ssd1306_set_column_addr(0);
			}
			clear_screen();
		}
	}
}

void snake_clear_map()
{
	for(int i = 1; i < 127; i++)
	{
		
		for(int j = 3; j < 7; j++)
		{
			ssd1306_set_page_start(j);
			ssd1306_set_column_addr(i);
			ssd1306_data(blank, 1);
		}
	}

	for(int i = 1; i < 127; i++)
	{
		ssd1306_set_page_start(2);
		ssd1306_set_column_addr(i);
		ssd1306_data(horizon1, 1);
		ssd1306_set_page_start(7);
		ssd1306_set_column_addr(i);
		ssd1306_data(horizon2, 1);
	}
}