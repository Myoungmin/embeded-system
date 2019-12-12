#include <avr/io.h>
#include "u8g2.h"
#include <util/delay.h>
#include <avr/interrupt.h>


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


__flash const unsigned short tetriminos[7][4] = {

	{0x6600, 0x6600, 0x6600, 0x6600},       // O
	{0xF000, 0x4444, 0xF000, 0x4444},        // I
	{0x6C00, 0x8C40, 0x6C00, 0x8C40},    // S
	{0xC600, 0x4C80, 0xC600, 0x4C80},    // Z
	{0xE400, 0x8C80, 0x4E00, 0x4C40},    // T
	{0x2E00, 0xC440, 0xE800, 0x88C0},    // L
	{0x8E00, 0x44C0, 0xE200, 0xC880},    // J

};





unsigned char shape;                  // 테트리미노스의 7가지 모양
unsigned char pattern;                 // 테트리미노스의 4가지 패턴
unsigned char cur_line;               // 테트리니노스의 현재 라인
unsigned char cur_col;                // 테트리니노스의 현재 칸
unsigned long int temp_line[4];          // 테트리미노스 라인 임시 저장소
unsigned long int main_board[64] = {0};	//테르리미노스가 굳어진 후 저장된 게임보드
unsigned long int game_board[64];	//테트리미노스가 움직이면서 변화하는 게임보드
unsigned char crush = 0;	//부딪힘을 나타내는 플레그
unsigned char new_block = 0;	//새로운 블록이 생성되야함을 나타내는 플레그
unsigned char game_over = 0;	//게임이 종료되었음을 나타내는 플레그
unsigned char next_block = 0;
unsigned char next_board[8][8] = {0};

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

 ISR(INT4_vect)
 {
	 
	 for(int i = 0; j < 64; i++)
	 {
		 game_board[i] = main_board[i];	//굳어진후 저장된 보드를 변화하는 보드로 복사
	 }


	 if(cur_col > 3) cur_col--;	//테트리미노스 현재칸 오른쪽으로 이동(3보다 클 시)
	 else if(cur_col == 3 && pattern == 1 || pattern == 3) cur_col--;

	 
	  for(int i = 0; i < 4; i++)	//테트리미노스 라인 임시 저장소 초기화
	  {
		  temp_line[i] = 0;
	  }


	 temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0xF000) >> 12) << (cur_col - 3);	//이 과정이 필요 없고 뒤에서 한번에 충돌감지 이후 넣어줘도 상관없어 보이지만 충돌 여부 판별하는 함수가 temp_line와 메인보드가 겹치는지 파악하기때문에 temp_line에 변화된 사항을 반영해야 한다.
	 temp_line[1] = ((unsigned long int)(tetriminos[shape][pattern] & 0x0F00) >> 8) << (cur_col - 3);
	 temp_line[2] = ((unsigned long int)(tetriminos[shape][pattern] & 0x00F0) >> 4) << (cur_col - 3);
	 temp_line[3] = ((unsigned long int)(tetriminos[shape][pattern] & 0x000F)) << (cur_col - 3);

	 
	 if(Collision() == 1) cur_col++;

	 for(int i = 0; i < 4; i++)	//테트리미노스 라인 임시 저장소 초기화
	 {
		 temp_line[i] = 0;
	 }

	 temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0xF000) >> 12) << (cur_col - 3);
	 temp_line[1] = ((unsigned long int)(tetriminos[shape][pattern] & 0x0F00) >> 8) << (cur_col - 3);
	 temp_line[2] = ((unsigned long int)(tetriminos[shape][pattern] & 0x00F0) >> 4) << (cur_col - 3);
	 temp_line[3] = ((unsigned long int)(tetriminos[shape][pattern] & 0x000F)) << (cur_col - 3);

	 
	 game_board[cur_line] = temp_line[0];	//현재라인 아래로 이동 후 변화하는 보드에 반영
	 game_board[cur_line + 1] = temp_line[1];
	 game_board[cur_line + 2] = temp_line[2];
	 game_board[cur_line + 3] = temp_line[3];

 }

 ISR(INT5_vect)
 {
	 for(int i = 0; i < 12; i++)
	 {
		 for(int j = 0; j< 32; j++)
		 {
			 game_board[i][j] = main_board[i][j];
		 }
	 }

	 pattern++;	//회전으로 상태 변화
	 if(pattern == 4) pattern = 0; //마지막에서 처음으로

	 for(int i = 0; i < 12; i++)	//테트리미노스 라인 임시 저장소 초기화
	 {
		 for(int j = 0; j < 4; j++)
		 {
			 temp_line[i][j] = 0;
		 }
	 }

	 for(int i = 0; i < 4; i++)	//이 과정이 필요 없고 뒤에서 한번에 충돌감지 이후 넣어줘도 상관없어 보이지만 충돌 여부 판별하는 함수가 temp_line와 메인보드가 겹치는지 파악하기때문에 temp_line에 변화된 사항을 반영해야 한다.
	 {
		 temp_line[cur_col-i][0] = tetriminos[shape][pattern][i];	//임시저장소에 테트리미노스 '왼쪽부터'(그래서 -i 로 역순이 된다) 지정된 칸에 넣기
		 temp_line[cur_col-i][1] = tetriminos[shape][pattern][i + 4];
		 temp_line[cur_col-i][2] = tetriminos[shape][pattern][i + 8];
		 temp_line[cur_col-i][3] = tetriminos[shape][pattern][i + 12];
	 }

	 if(Collision() == 1) pattern--;


	 for(int i = 0; i < 12; i++)	//테트리미노스 라인 임시 저장소 초기화
	 {
		 for(int j = 0; j < 4; j++)
		 {
			 temp_line[i][j] = 0;
		 }
	 }

	 for(int i = 0; i < 4; i++)
	 {
		 temp_line[cur_col-i][0] = tetriminos[shape][pattern][i];	//임시저장소에 테트리미노스 '왼쪽부터'(그래서 -i 로 역순이 된다) 지정된 칸에 넣기
		 temp_line[cur_col-i][1] = tetriminos[shape][pattern][i + 4];
		 temp_line[cur_col-i][2] = tetriminos[shape][pattern][i + 8];
		 temp_line[cur_col-i][3] = tetriminos[shape][pattern][i + 12];
	 }

	 for(int i = 0; i < 4; i++)	//현재라인 아래로 이동 후 변화하는 보드에 반영
	 {
		 if(cur_col-i > 0) game_board[cur_col-i][cur_line] |= temp_line[cur_col-i][0];
		 if(cur_col-i > 0) game_board[cur_col-i][cur_line + 1] |= temp_line[cur_col-i][1];
		 if(cur_col-i > 0) game_board[cur_col-i][cur_line + 2] |= temp_line[cur_col-i][2];
		 if(cur_col-i > 0) game_board[cur_col-i][cur_line + 3] |= temp_line[cur_col-i][3];
	 }
 }

 ISR(INT6_vect)
 {
	 for(int i = 0; i < 12; i++)	//테트리미노스 라인 임시 저장소 초기화
	 {
		 for(int j = 0; j < 4; j++)
		 {
			 temp_line[i][j] = 0;
		 }
	 }

	 for(int i = 0; i < 4; i++)	//이 과정이 필요 없고 뒤에서 한번에 충돌감지 이후 넣어줘도 상관없어 보이지만 충돌 여부 판별하는 함수가 temp_line와 메인보드가 겹치는지 파악하기때문에 temp_line에 변화된 사항을 반영해야 한다.
	 {
		 temp_line[cur_col-i][0] = tetriminos[shape][pattern][i];	//임시저장소에 테트리미노스 '왼쪽부터'(그래서 -i 로 역순이 된다) 지정된 칸에 넣기
		 temp_line[cur_col-i][1] = tetriminos[shape][pattern][i + 4];
		 temp_line[cur_col-i][2] = tetriminos[shape][pattern][i + 8];
		 temp_line[cur_col-i][3] = tetriminos[shape][pattern][i + 12];
	 }

	 while(Collision() == 0) cur_line++;

	 cur_line--;
 }

 ISR(INT7_vect)
 {
	 for(int i = 0; i < 12; i++)
	 {
		 for(int j = 0; j< 32; j++)
		 {
			 game_board[i][j] = main_board[i][j];
		 }
	 }

	 if(cur_col < 11) cur_col++;

	 for(int i = 0; i < 12; i++)	//테트리미노스 라인 임시 저장소 초기화
	 {
		 for(int j = 0; j < 4; j++)
		 {
			 temp_line[i][j] = 0;
		 }
	 }

	 for(int i = 0; i < 4; i++)	//이 과정이 필요 없고 뒤에서 한번에 충돌감지 이후 넣어줘도 상관없어 보이지만 충돌 여부 판별하는 함수가 temp_line와 메인보드가 겹치는지 파악하기때문에 temp_line에 변화된 사항을 반영해야 한다.
	 {
		 temp_line[cur_col-i][0] = tetriminos[shape][pattern][i];	//임시저장소에 테트리미노스 '왼쪽부터'(그래서 -i 로 역순이 된다) 지정된 칸에 넣기
		 temp_line[cur_col-i][1] = tetriminos[shape][pattern][i + 4];
		 temp_line[cur_col-i][2] = tetriminos[shape][pattern][i + 8];
		 temp_line[cur_col-i][3] = tetriminos[shape][pattern][i + 12];
	 }

	 if(Collision() == 1) cur_col--;
	 
	 for(int i = 0; i < 12; i++)	//테트리미노스 라인 임시 저장소 초기화
	 {
		 for(int j = 0; j < 4; j++)
		 {
			 temp_line[i][j] = 0;
		 }
	 }

	 for(int i = 0; i < 4; i++)
	 {
		 temp_line[cur_col-i][0] = tetriminos[shape][pattern][i];	//임시저장소에 테트리미노스 '왼쪽부터'(그래서 -i 로 역순이 된다) 지정된 칸에 넣기
		 temp_line[cur_col-i][1] = tetriminos[shape][pattern][i + 4];
		 temp_line[cur_col-i][2] = tetriminos[shape][pattern][i + 8];
		 temp_line[cur_col-i][3] = tetriminos[shape][pattern][i + 12];
	 }

	 
	 for(int i = 0; i < 4; i++)	//현재라인 아래로 이동 후 변화하는 보드에 반영
	 {
		 game_board[cur_col-i][cur_line] |= temp_line[cur_col-i][0];
		 game_board[cur_col-i][cur_line + 1] |= temp_line[cur_col-i][1];
		 game_board[cur_col-i][cur_line + 2] |= temp_line[cur_col-i][2];
		 game_board[cur_col-i][cur_line + 3] |= temp_line[cur_col-i][3];
	 }
 }



 ISR(INT3_vect)
 {
	 
 }


 ISR(TIMER1_COMPA_vect)	//OCR1A값에 따라 블록 이동속도가 결정된다.
 {
	 if(new_block == 0)
	 {
		 for(int i = 0; i < 12; i++)
		 {
			 for(int j = 0; j< 32; j++)
			 {
				 game_board[i][j] = main_board[i][j];	//굳어진후 저장된 보드를 변화하는 보드로 복사
			 }
		 }

		 cur_line++;	//현재라인 아래로 이동
		 
		 if(Collision() == 1)	//이동후 충돌 발생시
		 {
			 cur_line--;	//원래 라인으로 복귀
			 for(int i = 0; i < 4; i++)	//복귀후 임시저장소의 테트리미노스 메인보드에 저장
			 {
				 if(cur_col-i > 0) main_board[cur_col-i][cur_line] |= temp_line[cur_col-i][0];
				 if(cur_col-i > 0) main_board[cur_col-i][cur_line + 1] |= temp_line[cur_col-i][1];
				 if(cur_col-i > 0) main_board[cur_col-i][cur_line + 2] |= temp_line[cur_col-i][2];
				 if(cur_col-i > 0) main_board[cur_col-i][cur_line + 3] |= temp_line[cur_col-i][3];
			 }

			 new_block = 1;	//새로운 블록 플레그 켜짐
		 }

		 for(int i = 0; i < 12; i++)
		 {
			 for(int j = 0; j< 32; j++)
			 {
				 game_board[i][j] = main_board[i][j];	//굳어진후 저장된 보드를 변화하는 보드로 복사
			 }
		 }
		 for(int i = 0; i < 4; i++)	//현재라인 아래로 이동 후 변화하는 보드에 반영
		 {
			 if(cur_col-i > 0) game_board[cur_col-i][cur_line] |= temp_line[cur_col-i][0];
			 if(cur_col-i > 0) game_board[cur_col-i][cur_line + 1] |= temp_line[cur_col-i][1];
			 if(cur_col-i > 0) game_board[cur_col-i][cur_line + 2] |= temp_line[cur_col-i][2];
			 if(cur_col-i > 0) game_board[cur_col-i][cur_line + 3] |= temp_line[cur_col-i][3];
		 }

		 for(int i = 0; i < 31; i++)	//pattern = 1, 3 일때 너무 오른쪽으로 가면 옆에 벽 뚫리는 현상 방지 위해
		 {
			 game_board[0][i] = 1;
			 game_board[11][i] = 1;
		 }
		 for(int i = 0; i < 12; i++)
		 {
			 game_board[i][31] = 1;
		 }
	 }
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