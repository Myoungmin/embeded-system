#include <avr/io.h>
#include "u8g2.h"
#include <util/delay.h>
#include <math.h>

#define DISPLAY_CLK_DIR DDRD
#define DISPLAY_CLK_PORT PORTD
#define DISPLAY_CLK_PIN 0

#define DISPLAY_DATA_DIR DDRD
#define DISPLAY_DATA_PORT PORTD
#define DISPLAY_DATA_PIN 1

#define P_CPU_NS (1000000000UL / F_CPU)

u8g2_t u8g2;

uint8_t u8x8_avr_delay(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
	uint8_t cycles;

	switch(msg)
	{
		case U8X8_MSG_DELAY_NANO:     // delay arg_int * 1 nano second
			// At 20Mhz, each cycle is 50ns, the call itself is slower.
			break;
		case U8X8_MSG_DELAY_100NANO:    // delay arg_int * 100 nano seconds
			// Approximate best case values...
#define CALL_CYCLES 26UL
#define CALC_CYCLES 4UL
#define RETURN_CYCLES 4UL
#define CYCLES_PER_LOOP 4UL

			cycles = (100UL * arg_int) / (P_CPU_NS * CYCLES_PER_LOOP);

			if(cycles > CALL_CYCLES + RETURN_CYCLES + CALC_CYCLES) 
				break;

			__asm__ __volatile__ (
			"1: sbiw %0,1" "\n\t" // 2 cycles
			"brne 1b" : "=w" (cycles) : "0" (cycles) // 2 cycles
			);
			break;
		case U8X8_MSG_DELAY_10MICRO:    // delay arg_int * 10 micro seconds
			for(int i=0 ; i < arg_int ; i++)
				_delay_us(10);
			break;
		case U8X8_MSG_DELAY_MILLI:      // delay arg_int * 1 milli second
			for(int i=0 ; i < arg_int ; i++)
				_delay_ms(1);
			break;
		default:
			return 0;
	}
	return 1;
}


uint8_t u8x8_avr_gpio_and_delay(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
	// Re-use library for delays

	switch(msg)
	{
		case U8X8_MSG_GPIO_AND_DELAY_INIT:  // called once during init phase of u8g2/u8x8
			DISPLAY_CLK_DIR |= 1<<DISPLAY_CLK_PIN;
			DISPLAY_DATA_DIR |= 1<<DISPLAY_DATA_PIN;
			break;              // can be used to setup pins
		case U8X8_MSG_GPIO_I2C_CLOCK:        // Clock pin: Output level in arg_int
			if(arg_int)
				DISPLAY_CLK_PORT |= (1<<DISPLAY_CLK_PIN);
			else
				DISPLAY_CLK_PORT &= ~(1<<DISPLAY_CLK_PIN);
			break;
		case U8X8_MSG_GPIO_I2C_DATA:        // Data pin: Output level in arg_int
			if(arg_int)
				DISPLAY_DATA_PORT |= (1<<DISPLAY_DATA_PIN);
			else
				DISPLAY_DATA_PORT &= ~(1<<DISPLAY_DATA_PIN);
			break;
		
		default:
			if (u8x8_avr_delay(u8x8, msg, arg_int, arg_ptr))	// check for any delay msgs
				return 1;
			u8x8_SetGPIOResult(u8x8, 1);      // default return value
			break;
	}
	return 1;
}


void u8g2_prepare(void) {
	u8g2_SetFont(&u8g2, u8g2_font_6x10_tf);
	u8g2_SetFontRefHeightExtendedText(&u8g2);
	u8g2_SetDrawColor(&u8g2, 1);
	u8g2_SetFontPosTop(&u8g2);
	u8g2_SetFontDirection(&u8g2, 0);

}


unsigned short tetriminos[7][4] = {

	{0x6600, 0x6600, 0x6600, 0x6600},       // O
	{0xF000, 0x4444, 0xF000, 0x4444},        // I
	{0x6C00, 0x8C40, 0x6C00, 0x8C40},    // S
	{0xC600, 0x4C80, 0xC600, 0x4C80},    // Z
	{0xE400, 0x8C80, 0x4E00, 0x4C40},    // T
	{0x2E00, 0xC440, 0xE800, 0x88C0},    // L
	{0x8E00, 0x44C0, 0xE200, 0xC880},    // J

};



unsigned long int game_board[64];

unsigned char shape;                  // 테트리미노스의 7가지 모양
unsigned char pattern;                 // 테트리미노스의 4가지 패턴
unsigned char cur_line;               // 테트리니노스의 현재 라인
unsigned char cur_col;                // 테트리니노스의 현재 칸
unsigned short temp_line[4];          // 테트리미노스 라인 임시 저장소


 void NewTetriminos(void)
 {
	shape = TCNT0 % 7;    // 단순한 블럭 랜덤 생성 (여기서 TMR2는 dsPIC33F256GP710 Timer2 레지스터 값 임)
	pattern = 0;                // 패턴은 초기 패턴 값으로 설정

	temp_line[0] = (tetriminos[shape][pattern] & 0xF000) >> 6;
	temp_line[1] = (tetriminos[shape][pattern] & 0x0F00) >> 2;
	temp_line[2] = (tetriminos[shape][pattern] & 0x00F0) << 2;
	temp_line[3] = (tetriminos[shape][pattern] & 0x000F) << 6;

	cur_line = 0;                 // 테트리미노스 현재 라인 (최상위 라인)
	cur_col = 9;                // 테트리미노스의 현재 칸  (그림 상 붉은 색으로 하이라이트 된 부분이 위치할 곳)


 }

 signed char Collision(void)
 {

	 if( ((game_board[cur_line] & temp_line[0]) != 0) | ((game_board[cur_line + 1] & temp_line[1]) != 0) |
	 ((game_board[cur_line + 2] & temp_line[2]) != 0) | ((game_board[cur_line + 3] & temp_line[3]) != 0) )
	 return -1;         // 충돌
	 else

	 return 0;  // 충돌 없음
 }




int main(void)
{
	
	u8g2_Setup_ssd1306_i2c_128x64_noname_f( &u8g2, U8G2_R0, u8x8_byte_sw_i2c, u8x8_avr_gpio_and_delay );
	u8g2_SetI2CAddress(&u8g2, 0x78);
	u8g2_InitDisplay(&u8g2);
	u8g2_SetPowerSave(&u8g2, 0);
	
	u8g2_prepare();
	

	for (int i = 0; i < 63; i++ ) game_board[i] = 0xF0000F;   
	game_board[63] = 0xFFFFFF;                              


	while(1)
	{
		for(int i = 0; i < 24; i++)
		{
			for(int j = 0; j < 64; j++)
			{
				if((game_board[j] & ((unsigned long int)1<< i)) != 0)
				{
					u8g2_DrawPixel(&u8g2, 2 * j, 16 + 2 * i);
					u8g2_DrawPixel(&u8g2, 2 * j, 16 + 2 * i + 1);
					u8g2_DrawPixel(&u8g2, 2 * j + 1, 16 + 2 * i);
					u8g2_DrawPixel(&u8g2, 2 * j + 1, 16 + 2 * i + 1);
				}
			}
		}
		u8g2_DrawPixel(&u8g2, 127, 63);
		u8g2_SendBuffer(&u8g2);
		u8g2_ClearBuffer(&u8g2);
	}
}