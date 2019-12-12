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


__flash const unsigned int tetriminos[7][4] = {

	{0x6600, 0x6600, 0x6600, 0x6600},       // O
	{0xF000, 0x4444, 0xF000, 0x4444},        // I
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
unsigned long int main_board[64] = {0};	//테르리미노스가 굳어진 후 저장된 게임보드
unsigned long int game_board[64];	//테트리미노스가 움직이면서 변화하는 게임보드
unsigned char crush = 0;	//부딪힘을 나타내는 플레그
unsigned char new_block = 0;	//새로운 블록이 생성되야함을 나타내는 플레그
unsigned char game_over = 0;	//게임이 종료되었음을 나타내는 플레그
unsigned char next_block = 0;
unsigned char next_board[8][8] = {0};

 

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
	 
	 for(int i = 0; i < 64; i++)
	 {
		 game_board[i] = main_board[i];	//굳어진후 저장된 보드를 변화하는 보드로 복사
	 }

	 cur_col--;

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

	 
	 game_board[cur_line] |= temp_line[0];	//현재라인 아래로 이동 후 변화하는 보드에 반영
	 game_board[cur_line + 1] |= temp_line[1];
	 game_board[cur_line + 2] |= temp_line[2];
	 game_board[cur_line + 3] |= temp_line[3];

 }

 ISR(INT5_vect)
 {
	 for(int i = 0; i < 64; i++)
	 {
		 game_board[i] = main_board[i];	//굳어진후 저장된 보드를 변화하는 보드로 복사
	 }

	 pattern++;	//회전으로 상태 변화
	 if(pattern == 4) pattern = 0; //마지막에서 처음으로

	 for(int i = 0; i < 4; i++)	//테트리미노스 라인 임시 저장소 초기화
	 {
		 temp_line[i] = 0;
	 }

	 temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0xF000) >> 12) << (cur_col - 3);	//이 과정이 필요 없고 뒤에서 한번에 충돌감지 이후 넣어줘도 상관없어 보이지만 충돌 여부 판별하는 함수가 temp_line와 메인보드가 겹치는지 파악하기때문에 temp_line에 변화된 사항을 반영해야 한다.
	 temp_line[1] = ((unsigned long int)(tetriminos[shape][pattern] & 0x0F00) >> 8) << (cur_col - 3);
	 temp_line[2] = ((unsigned long int)(tetriminos[shape][pattern] & 0x00F0) >> 4) << (cur_col - 3);
	 temp_line[3] = ((unsigned long int)(tetriminos[shape][pattern] & 0x000F)) << (cur_col - 3);

	 if(Collision() == 1) pattern--;


	 for(int i = 0; i < 4; i++)	//테트리미노스 라인 임시 저장소 초기화
	 {
		 temp_line[i] = 0;
	 }

	 temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0xF000) >> 12) << (cur_col - 3);
	 temp_line[1] = ((unsigned long int)(tetriminos[shape][pattern] & 0x0F00) >> 8) << (cur_col - 3);
	 temp_line[2] = ((unsigned long int)(tetriminos[shape][pattern] & 0x00F0) >> 4) << (cur_col - 3);
	 temp_line[3] = ((unsigned long int)(tetriminos[shape][pattern] & 0x000F)) << (cur_col - 3);

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

	 temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0xF000) >> 12) << (cur_col - 3);	//이 과정이 필요 없고 뒤에서 한번에 충돌감지 이후 넣어줘도 상관없어 보이지만 충돌 여부 판별하는 함수가 temp_line와 메인보드가 겹치는지 파악하기때문에 temp_line에 변화된 사항을 반영해야 한다.
	 temp_line[1] = ((unsigned long int)(tetriminos[shape][pattern] & 0x0F00) >> 8) << (cur_col - 3);
	 temp_line[2] = ((unsigned long int)(tetriminos[shape][pattern] & 0x00F0) >> 4) << (cur_col - 3);
	 temp_line[3] = ((unsigned long int)(tetriminos[shape][pattern] & 0x000F)) << (cur_col - 3);

	 while(Collision() == 0) cur_line++;

	 cur_line--;
 }

 ISR(INT7_vect)
 {
	 for(int i = 0; i < 64; i++)
	 {
		 game_board[i] = main_board[i];	//굳어진후 저장된 보드를 변화하는 보드로 복사
	 }

	 cur_col++;

	 for(int i = 0; i < 4; i++)	//테트리미노스 라인 임시 저장소 초기화
	 {
		 temp_line[i] = 0;
	 }

	 temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0xF000) >> 12) << (cur_col - 3);	//이 과정이 필요 없고 뒤에서 한번에 충돌감지 이후 넣어줘도 상관없어 보이지만 충돌 여부 판별하는 함수가 temp_line와 메인보드가 겹치는지 파악하기때문에 temp_line에 변화된 사항을 반영해야 한다.
	 temp_line[1] = ((unsigned long int)(tetriminos[shape][pattern] & 0x0F00) >> 8) << (cur_col - 3);
	 temp_line[2] = ((unsigned long int)(tetriminos[shape][pattern] & 0x00F0) >> 4) << (cur_col - 3);
	 temp_line[3] = ((unsigned long int)(tetriminos[shape][pattern] & 0x000F)) << (cur_col - 3);

	 if(Collision() == 1) cur_col--;
	 
	 for(int i = 0; i < 4; i++)	//테트리미노스 라인 임시 저장소 초기화
	 {
		 temp_line[i] = 0;
	 }

	 temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0xF000) >> 12) << (cur_col - 3);
	 temp_line[1] = ((unsigned long int)(tetriminos[shape][pattern] & 0x0F00) >> 8) << (cur_col - 3);
	 temp_line[2] = ((unsigned long int)(tetriminos[shape][pattern] & 0x00F0) >> 4) << (cur_col - 3);
	 temp_line[3] = ((unsigned long int)(tetriminos[shape][pattern] & 0x000F)) << (cur_col - 3);

	 
	 game_board[cur_line] |= temp_line[0];	//현재라인 아래로 이동 후 변화하는 보드에 반영
	 game_board[cur_line + 1] |= temp_line[1];
	 game_board[cur_line + 2] |= temp_line[2];
	 game_board[cur_line + 3] |= temp_line[3];
 }



 ISR(INT3_vect)
 {
	 
 }


 ISR(TIMER1_COMPA_vect)	//OCR1A값에 따라 블록 이동속도가 결정된다.
 {
	 if(new_block == 0)
	 {
		 for(int i = 0; i < 64; i++)
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

		 for(int i = 0; i < 64; i++)
		 {
			 game_board[i] = main_board[i];	//굳어진후 저장된 보드를 변화하는 보드로 복사
		 }

		 game_board[cur_line] |= temp_line[0];	//현재라인 아래로 이동 후 변화하는 보드에 반영
		 game_board[cur_line + 1] |= temp_line[1];
		 game_board[cur_line + 2] |= temp_line[2];
		 game_board[cur_line + 3] |= temp_line[3];

		 /*for(int i = 0; i < 31; i++)	//pattern = 1, 3 일때 너무 오른쪽으로 가면 옆에 벽 뚫리는 현상 방지 위해
		 {
			 game_board[0][i] = 1;
			 game_board[11][i] = 1;
		 }

		 for(int i = 0; i < 12; i++)
		 {
			 game_board[i][31] = 1;
		 }*/
	 }
 }



 void draw_map()
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

 void NewTetriminos()
 {
	 new_block = 0;	//새로운 블록 플레그 끄기
	 shape = next_block;
	 next_block = TCNT0 % 7;					//다음에 올 테트리미노스 랜덤 출력
	 pattern = 0;	//기본 회전모향 설정
	 cur_line = 0;                 // 테트리미노스 현재 라인 (최상위 라인)
	 cur_col = 12;                // 테트리미노스의 현재 칸


	 for(int i = 0; i < 4; i++)	//테트리미노스 라인 임시 저장소 초기화
	 {
		 temp_line[i] = 0;
	 }

	 temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0xF000) >> 12) << (cur_col - 3);	//이 과정이 필요 없고 뒤에서 한번에 충돌감지 이후 넣어줘도 상관없어 보이지만 충돌 여부 판별하는 함수가 temp_line와 메인보드가 겹치는지 파악하기때문에 temp_line에 변화된 사항을 반영해야 한다.
	 temp_line[1] = ((unsigned long int)(tetriminos[shape][pattern] & 0x0F00) >> 8) << (cur_col - 3);
	 temp_line[2] = ((unsigned long int)(tetriminos[shape][pattern] & 0x00F0) >> 4) << (cur_col - 3);
	 temp_line[3] = ((unsigned long int)(tetriminos[shape][pattern] & 0x000F)) << (cur_col - 3);

	 
	game_over |= game_board[cur_line] & temp_line[0];	//게임보드에 있는 테트리미노스와 임시저장소에 생긴 테트리미노스가 겹치는지 확인하고 겹치면 게임오버 플레그 켜짐
	game_over |= game_board[cur_line + 1] & temp_line[1];	//or 연산으로 어디든 겹치면 플레그 켜진다
	game_over |= game_board[cur_line + 2] & temp_line[2];
	game_over |= game_board[cur_line + 3] & temp_line[3];
	

	 game_board[cur_line] |= temp_line[0];	//현재라인 아래로 이동 후 변화하는 보드에 반영
	 game_board[cur_line + 1] |= temp_line[1];
	 game_board[cur_line + 2] |= temp_line[2];
	 game_board[cur_line + 3] |= temp_line[3];

 }

 

int main(void)
{
	
	char cmd;

	//UART_Init();

	TCCR0 = 0x07;	//타이머 카운터 0 1024분주로 켜기

	TCCR1A |= (0 << COM1A0) | (0 << WGM10);	//CTC, OC1A핀 차단
	TCCR1B |= (1 << WGM12) | (5 << CS10); //CTC, 1024분주
	OCR1A = 0x8FF;
	TIMSK |= (1 << OCIE1A);	//타이머 카운터 1 출력비교 A 매치 인터럽트 인에이블

	DDRD = 0x00;	//인터럽트 4,5,6,7 켜기위해 포트 입력으로
	DDRE = 0x00;	//인터럽트 3 켜기위해 포트 입력으로

	EICRB = (2 << ISC40) | (2 << ISC50) | (2 << ISC60) | (2 << ISC70);
	
	EICRA = (2 << ISC30);

	EIMSK = (1 << INT3) | (1 << INT4) | (1 << INT5) | (1 << INT6) | (1 << INT7);

	sei();

	u8g2_Setup_ssd1306_i2c_128x64_noname_f( &u8g2, U8G2_R0, u8x8_byte_sw_i2c, u8x8_avr_gpio_and_delay );
	u8g2_SetI2CAddress(&u8g2, 0x78);
	u8g2_InitDisplay(&u8g2);
	u8g2_SetPowerSave(&u8g2, 0);
	
	u8g2_prepare();
	                              


	while(1)
	{
		game_over = 0;	//게임종료 플레그 끄기


		for (int i = 0; i < 63; i++ ) main_board[i] = 0xF0000F;	//메인보드 초기화
		main_board[63] = 0xFFFFFF;




		while(game_over == 0)	//게임종료 플레그가 꺼저있을동안 반복
		{
			for(int i = 0; i < 63; i++)
			{
				if(main_board[i] == 0xFFFFFF)
				//1줄이 모두 완성되어서 깨질 줄이 있는지 확인
				{
					main_board[i] = 0xF0000F;
					for(int k = i; k > 0; k--)
					{
						main_board[k] = main_board[k - 1];	//깨진 줄의 위에 줄들 아래로 이동(행렬상 열 증가)
					}
				}
			}

			for(int i = 0; i < 64; i++)
			{
				game_board[i] = main_board[i];	//굳어진후 저장된 보드를 변화하는 보드로 복사
			}
			
			NewTetriminos();	//새로운 테트리미노스 생성
			while(new_block == 0)	//새로운 블록 프레그 꺼져있는 동안 반복
			{
				draw_map();	//반영된 변화하는 보드 화면으로 출력
			}
		}
	}
}