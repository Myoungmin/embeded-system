#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>


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















int main(void)
{
	char cmd;


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


	while(1)
	{
		new_block = 0;	//새로운 블록 플레그 끄기
		shape = next_block;
		next_block = TCNT0 % 7;					//다음에 올 테트리미노스 랜덤 출력
		pattern = 1;	//기본 회전모향 설정
		cur_line = 0;                 // 테트리미노스 현재 라인 (최상위 라인)
		cur_col = 2;                // 테트리미노스의 현재 칸


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

		game_board[cur_line] |= temp_line[0];	//현재라인 아래로 이동 후 변화하는 보드에 반영
		game_board[cur_line + 1] |= temp_line[1];
		game_board[cur_line + 2] |= temp_line[2];
		game_board[cur_line + 3] |= temp_line[3];

	}
}

