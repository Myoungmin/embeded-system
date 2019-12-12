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

 

 unsigned char Collision()	//�浹 ���� Ȯ��
 {
	 if( ((main_board[cur_line] & temp_line[0]) != 0) | ((main_board[cur_line + 1] & temp_line[1]) != 0) |
	 ((main_board[cur_line + 2] & temp_line[2]) != 0) | ((main_board[cur_line + 3] & temp_line[3]) != 0) )
	 return 1;         // �浹 1 ����
	 else
	 return 0;  // �浹 ���� 0 ����
 }

 ISR(INT4_vect)
 {
	 
	 for(int i = 0; i < 64; i++)
	 {
		 game_board[i] = main_board[i];	//�������� ����� ���带 ��ȭ�ϴ� ����� ����
	 }

	 cur_col--;

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

	 
	 game_board[cur_line] |= temp_line[0];	//������� �Ʒ��� �̵� �� ��ȭ�ϴ� ���忡 �ݿ�
	 game_board[cur_line + 1] |= temp_line[1];
	 game_board[cur_line + 2] |= temp_line[2];
	 game_board[cur_line + 3] |= temp_line[3];

 }

 ISR(INT5_vect)
 {
	 for(int i = 0; i < 64; i++)
	 {
		 game_board[i] = main_board[i];	//�������� ����� ���带 ��ȭ�ϴ� ����� ����
	 }

	 pattern++;	//ȸ������ ���� ��ȭ
	 if(pattern == 4) pattern = 0; //���������� ó������

	 for(int i = 0; i < 4; i++)	//��Ʈ���̳뽺 ���� �ӽ� ����� �ʱ�ȭ
	 {
		 temp_line[i] = 0;
	 }

	 temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0xF000) >> 12) << (cur_col - 3);	//�� ������ �ʿ� ���� �ڿ��� �ѹ��� �浹���� ���� �־��൵ ������� �������� �浹 ���� �Ǻ��ϴ� �Լ��� temp_line�� ���κ��尡 ��ġ���� �ľ��ϱ⶧���� temp_line�� ��ȭ�� ������ �ݿ��ؾ� �Ѵ�.
	 temp_line[1] = ((unsigned long int)(tetriminos[shape][pattern] & 0x0F00) >> 8) << (cur_col - 3);
	 temp_line[2] = ((unsigned long int)(tetriminos[shape][pattern] & 0x00F0) >> 4) << (cur_col - 3);
	 temp_line[3] = ((unsigned long int)(tetriminos[shape][pattern] & 0x000F)) << (cur_col - 3);

	 if(Collision() == 1) pattern--;


	 for(int i = 0; i < 4; i++)	//��Ʈ���̳뽺 ���� �ӽ� ����� �ʱ�ȭ
	 {
		 temp_line[i] = 0;
	 }

	 temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0xF000) >> 12) << (cur_col - 3);
	 temp_line[1] = ((unsigned long int)(tetriminos[shape][pattern] & 0x0F00) >> 8) << (cur_col - 3);
	 temp_line[2] = ((unsigned long int)(tetriminos[shape][pattern] & 0x00F0) >> 4) << (cur_col - 3);
	 temp_line[3] = ((unsigned long int)(tetriminos[shape][pattern] & 0x000F)) << (cur_col - 3);

	game_board[cur_line] |= temp_line[0];	//������� �Ʒ��� �̵� �� ��ȭ�ϴ� ���忡 �ݿ�
	game_board[cur_line + 1] |= temp_line[1];
	game_board[cur_line + 2] |= temp_line[2];
	game_board[cur_line + 3] |= temp_line[3];
 }

 ISR(INT6_vect)
 {
	 for(int i = 0; i < 4; i++)	//��Ʈ���̳뽺 ���� �ӽ� ����� �ʱ�ȭ
	 {
		 temp_line[i] = 0;
	 }

	 temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0xF000) >> 12) << (cur_col - 3);	//�� ������ �ʿ� ���� �ڿ��� �ѹ��� �浹���� ���� �־��൵ ������� �������� �浹 ���� �Ǻ��ϴ� �Լ��� temp_line�� ���κ��尡 ��ġ���� �ľ��ϱ⶧���� temp_line�� ��ȭ�� ������ �ݿ��ؾ� �Ѵ�.
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
		 game_board[i] = main_board[i];	//�������� ����� ���带 ��ȭ�ϴ� ����� ����
	 }

	 cur_col++;

	 for(int i = 0; i < 4; i++)	//��Ʈ���̳뽺 ���� �ӽ� ����� �ʱ�ȭ
	 {
		 temp_line[i] = 0;
	 }

	 temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0xF000) >> 12) << (cur_col - 3);	//�� ������ �ʿ� ���� �ڿ��� �ѹ��� �浹���� ���� �־��൵ ������� �������� �浹 ���� �Ǻ��ϴ� �Լ��� temp_line�� ���κ��尡 ��ġ���� �ľ��ϱ⶧���� temp_line�� ��ȭ�� ������ �ݿ��ؾ� �Ѵ�.
	 temp_line[1] = ((unsigned long int)(tetriminos[shape][pattern] & 0x0F00) >> 8) << (cur_col - 3);
	 temp_line[2] = ((unsigned long int)(tetriminos[shape][pattern] & 0x00F0) >> 4) << (cur_col - 3);
	 temp_line[3] = ((unsigned long int)(tetriminos[shape][pattern] & 0x000F)) << (cur_col - 3);

	 if(Collision() == 1) cur_col--;
	 
	 for(int i = 0; i < 4; i++)	//��Ʈ���̳뽺 ���� �ӽ� ����� �ʱ�ȭ
	 {
		 temp_line[i] = 0;
	 }

	 temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0xF000) >> 12) << (cur_col - 3);
	 temp_line[1] = ((unsigned long int)(tetriminos[shape][pattern] & 0x0F00) >> 8) << (cur_col - 3);
	 temp_line[2] = ((unsigned long int)(tetriminos[shape][pattern] & 0x00F0) >> 4) << (cur_col - 3);
	 temp_line[3] = ((unsigned long int)(tetriminos[shape][pattern] & 0x000F)) << (cur_col - 3);

	 
	 game_board[cur_line] |= temp_line[0];	//������� �Ʒ��� �̵� �� ��ȭ�ϴ� ���忡 �ݿ�
	 game_board[cur_line + 1] |= temp_line[1];
	 game_board[cur_line + 2] |= temp_line[2];
	 game_board[cur_line + 3] |= temp_line[3];
 }



 ISR(INT3_vect)
 {
	 
 }


 ISR(TIMER1_COMPA_vect)	//OCR1A���� ���� ��� �̵��ӵ��� �����ȴ�.
 {
	 if(new_block == 0)
	 {
		 for(int i = 0; i < 64; i++)
		 {
			 game_board[i] = main_board[i];	//�������� ����� ���带 ��ȭ�ϴ� ����� ����
		 }

		 cur_line++;	//������� �Ʒ��� �̵�
		 
		 if(Collision() == 1)	//�̵��� �浹 �߻���
		 {
			 cur_line--;	//���� �������� ����
			 main_board[cur_line] |= temp_line[0];	//������� �Ʒ��� �̵� �� ��ȭ�ϴ� ���忡 �ݿ�
			 main_board[cur_line + 1] |= temp_line[1];
			 main_board[cur_line + 2] |= temp_line[2];
			 main_board[cur_line + 3] |= temp_line[3];

			 new_block = 1;	//���ο� ��� �÷��� ����
		 }

		 for(int i = 0; i < 64; i++)
		 {
			 game_board[i] = main_board[i];	//�������� ����� ���带 ��ȭ�ϴ� ����� ����
		 }

		 game_board[cur_line] |= temp_line[0];	//������� �Ʒ��� �̵� �� ��ȭ�ϴ� ���忡 �ݿ�
		 game_board[cur_line + 1] |= temp_line[1];
		 game_board[cur_line + 2] |= temp_line[2];
		 game_board[cur_line + 3] |= temp_line[3];

		 /*for(int i = 0; i < 31; i++)	//pattern = 1, 3 �϶� �ʹ� ���������� ���� ���� �� �ո��� ���� ���� ����
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
	 new_block = 0;	//���ο� ��� �÷��� ����
	 shape = next_block;
	 next_block = TCNT0 % 7;					//������ �� ��Ʈ���̳뽺 ���� ���
	 pattern = 0;	//�⺻ ȸ������ ����
	 cur_line = 0;                 // ��Ʈ���̳뽺 ���� ���� (�ֻ��� ����)
	 cur_col = 12;                // ��Ʈ���̳뽺�� ���� ĭ


	 for(int i = 0; i < 4; i++)	//��Ʈ���̳뽺 ���� �ӽ� ����� �ʱ�ȭ
	 {
		 temp_line[i] = 0;
	 }

	 temp_line[0] = ((unsigned long int)(tetriminos[shape][pattern] & 0xF000) >> 12) << (cur_col - 3);	//�� ������ �ʿ� ���� �ڿ��� �ѹ��� �浹���� ���� �־��൵ ������� �������� �浹 ���� �Ǻ��ϴ� �Լ��� temp_line�� ���κ��尡 ��ġ���� �ľ��ϱ⶧���� temp_line�� ��ȭ�� ������ �ݿ��ؾ� �Ѵ�.
	 temp_line[1] = ((unsigned long int)(tetriminos[shape][pattern] & 0x0F00) >> 8) << (cur_col - 3);
	 temp_line[2] = ((unsigned long int)(tetriminos[shape][pattern] & 0x00F0) >> 4) << (cur_col - 3);
	 temp_line[3] = ((unsigned long int)(tetriminos[shape][pattern] & 0x000F)) << (cur_col - 3);

	 
	game_over |= game_board[cur_line] & temp_line[0];	//���Ӻ��忡 �ִ� ��Ʈ���̳뽺�� �ӽ�����ҿ� ���� ��Ʈ���̳뽺�� ��ġ���� Ȯ���ϰ� ��ġ�� ���ӿ��� �÷��� ����
	game_over |= game_board[cur_line + 1] & temp_line[1];	//or �������� ���� ��ġ�� �÷��� ������
	game_over |= game_board[cur_line + 2] & temp_line[2];
	game_over |= game_board[cur_line + 3] & temp_line[3];
	

	 game_board[cur_line] |= temp_line[0];	//������� �Ʒ��� �̵� �� ��ȭ�ϴ� ���忡 �ݿ�
	 game_board[cur_line + 1] |= temp_line[1];
	 game_board[cur_line + 2] |= temp_line[2];
	 game_board[cur_line + 3] |= temp_line[3];

 }

 

int main(void)
{
	
	char cmd;

	//UART_Init();

	TCCR0 = 0x07;	//Ÿ�̸� ī���� 0 1024���ַ� �ѱ�

	TCCR1A |= (0 << COM1A0) | (0 << WGM10);	//CTC, OC1A�� ����
	TCCR1B |= (1 << WGM12) | (5 << CS10); //CTC, 1024����
	OCR1A = 0x8FF;
	TIMSK |= (1 << OCIE1A);	//Ÿ�̸� ī���� 1 ��º� A ��ġ ���ͷ�Ʈ �ο��̺�

	DDRD = 0x00;	//���ͷ�Ʈ 4,5,6,7 �ѱ����� ��Ʈ �Է�����
	DDRE = 0x00;	//���ͷ�Ʈ 3 �ѱ����� ��Ʈ �Է�����

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
		game_over = 0;	//�������� �÷��� ����


		for (int i = 0; i < 63; i++ ) main_board[i] = 0xF0000F;	//���κ��� �ʱ�ȭ
		main_board[63] = 0xFFFFFF;




		while(game_over == 0)	//�������� �÷��װ� ������������ �ݺ�
		{
			for(int i = 0; i < 63; i++)
			{
				if(main_board[i] == 0xFFFFFF)
				//1���� ��� �ϼ��Ǿ ���� ���� �ִ��� Ȯ��
				{
					main_board[i] = 0xF0000F;
					for(int k = i; k > 0; k--)
					{
						main_board[k] = main_board[k - 1];	//���� ���� ���� �ٵ� �Ʒ��� �̵�(��Ļ� �� ����)
					}
				}
			}

			for(int i = 0; i < 64; i++)
			{
				game_board[i] = main_board[i];	//�������� ����� ���带 ��ȭ�ϴ� ����� ����
			}
			
			NewTetriminos();	//���ο� ��Ʈ���̳뽺 ����
			while(new_block == 0)	//���ο� ��� ������ �����ִ� ���� �ݺ�
			{
				draw_map();	//�ݿ��� ��ȭ�ϴ� ���� ȭ������ ���
			}
		}
	}
}