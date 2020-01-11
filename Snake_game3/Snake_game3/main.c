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
	//clear_screen();
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
#define PAUSE 112
#define ESC 27
#define MAP_X 0
#define MAP_Y 0

int food_x, food_y;
int length = 3;
unsigned long speed = 300;
int score;
int best_score=0;
int last_score=0;
int dir = RIGHT;
int game = 1;
int key;

static uint8_t horizon1[1] = {0x01};
static uint8_t horizon2[1] = {0x80};
static uint8_t vertical[1] = {0b01010101};
static uint8_t blank[1] = {0};

uint8_t x[200] = {32, 31, 30};
uint8_t y[200] = {12, 12, 12};
//unsigned char MAP [6][128] = {0};

void title();
void reset();
void draw_map();
void move(int dir);
void pause();
void game_over();
void food();
void clear_map();

ISR(INT5_vect)
{
	game = 1;
	if(dir != DOWN)
	{
		SendCommand01(0x0F, 0x01, 0x03);
		
		dir = UP;
	}
}

ISR(INT6_vect)
{
	game = 1;
	if(dir != UP)
	{
		SendCommand01(0x0F, 0x01, 0x03);
		
		dir = DOWN;
	}
}

ISR(INT7_vect)
{	
	game = 1;
	if(dir != RIGHT)
	{
		SendCommand01(0x0F, 0x01, 0x03);
		
		dir = LEFT;
	}
}

ISR(INT4_vect)
{
	game = 1;
	if(dir != LEFT)
	{
		SendCommand01(0x0F, 0x01, 0x03);
		
		dir = RIGHT;
	}
}

ISR(INT3_vect)
{
	game = 1;
}


void title()
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

	draw_map();
	
	food(); // food 생성

}

void draw_map()
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


void move(int dir)
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
		game_over();
		return; //game_over에서 게임을 다시 시작하게 되면 여기서부터 반복되므로
		//return을 사용하여 move의 나머지 부분이 실행되지 않도록 합니다.
	}
	for(int i=1;i<length;i++)
	{ //자기몸과 충돌했는지 검사
		if(x[0]==x[i] && y[0]==y[i])
		{
			game_over();
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


void game_over()
{
	SendCommand01(0x06, 0x00, 0x0F);
	_delay_ms(200);
	SendCommand01(0x0F, 0x01, 0x04);
	game = 0;
	clear_map();
	oled_msg(4, 6, "GAME OVER");
	ssd1306_set_column_addr(0);
	while(game != 1);

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
	draw_picture(KO);
	_delay_ms(400);
	clear_screen();
	title();
}


int main(void)
{
	char cmd;

	UART_Init();

	long int val_x;
	long int val_y;
	
	
	while(1)
	{
		SendCommand(CMD_TF);
		_delay_ms(20);
		SendCommand01(0x06, 0x00, 0x0F);
		_delay_ms(200);
		SendCommand01(0x0F, 0x01, 0x01);
		_delay_ms(200);

		DDRD = 0x00;
		DDRE = 0x00;
		EICRB = (2 << ISC40) | (2 << ISC50) | (2 << ISC60) | (2 << ISC70);
		
		EICRA = (2 << ISC30);

		EIMSK = (1 << INT3) | (1 << INT4) | (1 << INT5) | (1 << INT6) | (1 << INT7);

		sei();

		

		TCCR0 = 0x07;
		//TCNT0 = 0x00;

		setup();
		
		draw_picture(KO);
		_delay_ms(1500);
		clear_screen();
		title();
		
		dir = RIGHT;

		while(game == 1)
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


			move(dir);
			//SendCommand01(0x0F, 0x01, 0x02);
			draw_map();
			_delay_ms(80);
		}
	}
	
}

void clear_map()
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