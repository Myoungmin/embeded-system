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





unsigned char shape;                  // ��Ʈ���̳뽺�� 7���� ���
unsigned char pattern;                 // ��Ʈ���̳뽺�� 4���� ����
unsigned char cur_line;               // ��Ʈ���ϳ뽺�� ���� ����
unsigned char cur_col;                // ��Ʈ���ϳ뽺�� ���� ĭ
unsigned long int temp_line[4];          // ��Ʈ���̳뽺 ���� �ӽ� �����
unsigned long int main_board[64] = {0};	//�׸����̳뽺�� ������ �� ����� ���Ӻ���
unsigned long int game_board[64];	//��Ʈ���̳뽺�� �����̸鼭 ��ȭ�ϴ� ���Ӻ���
unsigned char crush = 0;	//�ε����� ��Ÿ���� �÷���
unsigned char new_block = 0;	//���ο� ����� �����Ǿ����� ��Ÿ���� �÷���
unsigned char game_over = 0;	//������ ����Ǿ����� ��Ÿ���� �÷���
unsigned char next_block = 0;
unsigned char next_board[8][8] = {0};

 void NewTetriminos(void)
 {
	shape = TCNT0 % 7;    // �ܼ��� �� ���� ���� (���⼭ TMR2�� dsPIC33F256GP710 Timer2 �������� �� ��)
	pattern = 0;                // ������ �ʱ� ���� ������ ����

	temp_line[0] = (tetriminos[shape][pattern] & 0xF000) >> 6;
	temp_line[1] = (tetriminos[shape][pattern] & 0x0F00) >> 2;
	temp_line[2] = (tetriminos[shape][pattern] & 0x00F0) << 2;
	temp_line[3] = (tetriminos[shape][pattern] & 0x000F) << 6;

	cur_line = 0;                 // ��Ʈ���̳뽺 ���� ���� (�ֻ��� ����)
	cur_col = 9;                // ��Ʈ���̳뽺�� ���� ĭ  (�׸� �� ���� ������ ���̶���Ʈ �� �κ��� ��ġ�� ��)


 }

 signed char Collision(void)
 {

	 if( ((game_board[cur_line] & temp_line[0]) != 0) | ((game_board[cur_line + 1] & temp_line[1]) != 0) |
	 ((game_board[cur_line + 2] & temp_line[2]) != 0) | ((game_board[cur_line + 3] & temp_line[3]) != 0) )
	 return -1;         // �浹
	 else

	 return 0;  // �浹 ����
 }

 ISR(INT4_vect)
 {
	 
	 for(int i = 0; j < 64; i++)
	 {
		 game_board[i] = main_board[i];	//�������� ����� ���带 ��ȭ�ϴ� ����� ����
	 }


	 if(cur_col > 3) cur_col--;	//��Ʈ���̳뽺 ����ĭ ���������� �̵�(3���� Ŭ ��)
	 else if(cur_col == 3 && pattern == 1 || pattern == 3) cur_col--;

	 
	  for(int i = 0; i < 4; i++)	//��Ʈ���̳뽺 ���� �ӽ� ����� �ʱ�ȭ
	  {
		  temp_line[i] = 0;
	  }


	 temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0xF000) >> 12) << (cur_col - 3);	//�� ������ �ʿ� ���� �ڿ��� �ѹ��� �浹���� ���� �־��൵ ������� �������� �浹 ���� �Ǻ��ϴ� �Լ��� temp_line�� ���κ��尡 ��ġ���� �ľ��ϱ⶧���� temp_line�� ��ȭ�� ������ �ݿ��ؾ� �Ѵ�.
	 temp_line[1] = ((unsigned long int)(tetriminos[shape][pattern] & 0x0F00) >> 8) << (cur_col - 3);
	 temp_line[2] = ((unsigned long int)(tetriminos[shape][pattern] & 0x00F0) >> 4) << (cur_col - 3);
	 temp_line[3] = ((unsigned long int)(tetriminos[shape][pattern] & 0x000F)) << (cur_col - 3);

	 
	 if(Collision() == 1) cur_col++;

	 for(int i = 0; i < 4; i++)	//��Ʈ���̳뽺 ���� �ӽ� ����� �ʱ�ȭ
	 {
		 temp_line[i] = 0;
	 }

	 temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0xF000) >> 12) << (cur_col - 3);
	 temp_line[1] = ((unsigned long int)(tetriminos[shape][pattern] & 0x0F00) >> 8) << (cur_col - 3);
	 temp_line[2] = ((unsigned long int)(tetriminos[shape][pattern] & 0x00F0) >> 4) << (cur_col - 3);
	 temp_line[3] = ((unsigned long int)(tetriminos[shape][pattern] & 0x000F)) << (cur_col - 3);

	 
	 game_board[cur_line] = temp_line[0];	//������� �Ʒ��� �̵� �� ��ȭ�ϴ� ���忡 �ݿ�
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

	 pattern++;	//ȸ������ ���� ��ȭ
	 if(pattern == 4) pattern = 0; //���������� ó������

	 for(int i = 0; i < 12; i++)	//��Ʈ���̳뽺 ���� �ӽ� ����� �ʱ�ȭ
	 {
		 for(int j = 0; j < 4; j++)
		 {
			 temp_line[i][j] = 0;
		 }
	 }

	 for(int i = 0; i < 4; i++)	//�� ������ �ʿ� ���� �ڿ��� �ѹ��� �浹���� ���� �־��൵ ������� �������� �浹 ���� �Ǻ��ϴ� �Լ��� temp_line�� ���κ��尡 ��ġ���� �ľ��ϱ⶧���� temp_line�� ��ȭ�� ������ �ݿ��ؾ� �Ѵ�.
	 {
		 temp_line[cur_col-i][0] = tetriminos[shape][pattern][i];	//�ӽ�����ҿ� ��Ʈ���̳뽺 '���ʺ���'(�׷��� -i �� ������ �ȴ�) ������ ĭ�� �ֱ�
		 temp_line[cur_col-i][1] = tetriminos[shape][pattern][i + 4];
		 temp_line[cur_col-i][2] = tetriminos[shape][pattern][i + 8];
		 temp_line[cur_col-i][3] = tetriminos[shape][pattern][i + 12];
	 }

	 if(Collision() == 1) pattern--;


	 for(int i = 0; i < 12; i++)	//��Ʈ���̳뽺 ���� �ӽ� ����� �ʱ�ȭ
	 {
		 for(int j = 0; j < 4; j++)
		 {
			 temp_line[i][j] = 0;
		 }
	 }

	 for(int i = 0; i < 4; i++)
	 {
		 temp_line[cur_col-i][0] = tetriminos[shape][pattern][i];	//�ӽ�����ҿ� ��Ʈ���̳뽺 '���ʺ���'(�׷��� -i �� ������ �ȴ�) ������ ĭ�� �ֱ�
		 temp_line[cur_col-i][1] = tetriminos[shape][pattern][i + 4];
		 temp_line[cur_col-i][2] = tetriminos[shape][pattern][i + 8];
		 temp_line[cur_col-i][3] = tetriminos[shape][pattern][i + 12];
	 }

	 for(int i = 0; i < 4; i++)	//������� �Ʒ��� �̵� �� ��ȭ�ϴ� ���忡 �ݿ�
	 {
		 if(cur_col-i > 0) game_board[cur_col-i][cur_line] |= temp_line[cur_col-i][0];
		 if(cur_col-i > 0) game_board[cur_col-i][cur_line + 1] |= temp_line[cur_col-i][1];
		 if(cur_col-i > 0) game_board[cur_col-i][cur_line + 2] |= temp_line[cur_col-i][2];
		 if(cur_col-i > 0) game_board[cur_col-i][cur_line + 3] |= temp_line[cur_col-i][3];
	 }
 }

 ISR(INT6_vect)
 {
	 for(int i = 0; i < 12; i++)	//��Ʈ���̳뽺 ���� �ӽ� ����� �ʱ�ȭ
	 {
		 for(int j = 0; j < 4; j++)
		 {
			 temp_line[i][j] = 0;
		 }
	 }

	 for(int i = 0; i < 4; i++)	//�� ������ �ʿ� ���� �ڿ��� �ѹ��� �浹���� ���� �־��൵ ������� �������� �浹 ���� �Ǻ��ϴ� �Լ��� temp_line�� ���κ��尡 ��ġ���� �ľ��ϱ⶧���� temp_line�� ��ȭ�� ������ �ݿ��ؾ� �Ѵ�.
	 {
		 temp_line[cur_col-i][0] = tetriminos[shape][pattern][i];	//�ӽ�����ҿ� ��Ʈ���̳뽺 '���ʺ���'(�׷��� -i �� ������ �ȴ�) ������ ĭ�� �ֱ�
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

	 for(int i = 0; i < 12; i++)	//��Ʈ���̳뽺 ���� �ӽ� ����� �ʱ�ȭ
	 {
		 for(int j = 0; j < 4; j++)
		 {
			 temp_line[i][j] = 0;
		 }
	 }

	 for(int i = 0; i < 4; i++)	//�� ������ �ʿ� ���� �ڿ��� �ѹ��� �浹���� ���� �־��൵ ������� �������� �浹 ���� �Ǻ��ϴ� �Լ��� temp_line�� ���κ��尡 ��ġ���� �ľ��ϱ⶧���� temp_line�� ��ȭ�� ������ �ݿ��ؾ� �Ѵ�.
	 {
		 temp_line[cur_col-i][0] = tetriminos[shape][pattern][i];	//�ӽ�����ҿ� ��Ʈ���̳뽺 '���ʺ���'(�׷��� -i �� ������ �ȴ�) ������ ĭ�� �ֱ�
		 temp_line[cur_col-i][1] = tetriminos[shape][pattern][i + 4];
		 temp_line[cur_col-i][2] = tetriminos[shape][pattern][i + 8];
		 temp_line[cur_col-i][3] = tetriminos[shape][pattern][i + 12];
	 }

	 if(Collision() == 1) cur_col--;
	 
	 for(int i = 0; i < 12; i++)	//��Ʈ���̳뽺 ���� �ӽ� ����� �ʱ�ȭ
	 {
		 for(int j = 0; j < 4; j++)
		 {
			 temp_line[i][j] = 0;
		 }
	 }

	 for(int i = 0; i < 4; i++)
	 {
		 temp_line[cur_col-i][0] = tetriminos[shape][pattern][i];	//�ӽ�����ҿ� ��Ʈ���̳뽺 '���ʺ���'(�׷��� -i �� ������ �ȴ�) ������ ĭ�� �ֱ�
		 temp_line[cur_col-i][1] = tetriminos[shape][pattern][i + 4];
		 temp_line[cur_col-i][2] = tetriminos[shape][pattern][i + 8];
		 temp_line[cur_col-i][3] = tetriminos[shape][pattern][i + 12];
	 }

	 
	 for(int i = 0; i < 4; i++)	//������� �Ʒ��� �̵� �� ��ȭ�ϴ� ���忡 �ݿ�
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


 ISR(TIMER1_COMPA_vect)	//OCR1A���� ���� ��� �̵��ӵ��� �����ȴ�.
 {
	 if(new_block == 0)
	 {
		 for(int i = 0; i < 12; i++)
		 {
			 for(int j = 0; j< 32; j++)
			 {
				 game_board[i][j] = main_board[i][j];	//�������� ����� ���带 ��ȭ�ϴ� ����� ����
			 }
		 }

		 cur_line++;	//������� �Ʒ��� �̵�
		 
		 if(Collision() == 1)	//�̵��� �浹 �߻���
		 {
			 cur_line--;	//���� �������� ����
			 for(int i = 0; i < 4; i++)	//������ �ӽ�������� ��Ʈ���̳뽺 ���κ��忡 ����
			 {
				 if(cur_col-i > 0) main_board[cur_col-i][cur_line] |= temp_line[cur_col-i][0];
				 if(cur_col-i > 0) main_board[cur_col-i][cur_line + 1] |= temp_line[cur_col-i][1];
				 if(cur_col-i > 0) main_board[cur_col-i][cur_line + 2] |= temp_line[cur_col-i][2];
				 if(cur_col-i > 0) main_board[cur_col-i][cur_line + 3] |= temp_line[cur_col-i][3];
			 }

			 new_block = 1;	//���ο� ��� �÷��� ����
		 }

		 for(int i = 0; i < 12; i++)
		 {
			 for(int j = 0; j< 32; j++)
			 {
				 game_board[i][j] = main_board[i][j];	//�������� ����� ���带 ��ȭ�ϴ� ����� ����
			 }
		 }
		 for(int i = 0; i < 4; i++)	//������� �Ʒ��� �̵� �� ��ȭ�ϴ� ���忡 �ݿ�
		 {
			 if(cur_col-i > 0) game_board[cur_col-i][cur_line] |= temp_line[cur_col-i][0];
			 if(cur_col-i > 0) game_board[cur_col-i][cur_line + 1] |= temp_line[cur_col-i][1];
			 if(cur_col-i > 0) game_board[cur_col-i][cur_line + 2] |= temp_line[cur_col-i][2];
			 if(cur_col-i > 0) game_board[cur_col-i][cur_line + 3] |= temp_line[cur_col-i][3];
		 }

		 for(int i = 0; i < 31; i++)	//pattern = 1, 3 �϶� �ʹ� ���������� ���� ���� �� �ո��� ���� ���� ����
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