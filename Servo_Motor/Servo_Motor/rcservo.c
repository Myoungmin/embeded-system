//======================================= 
// rcservo.c : 타이머/카운터1 Fast PWM 모드 
//				RC servo 3채널 구동 
//=======================================


#include <avr/io.h>
#include "rcservo.h"

void RCServoInit(unsigned short period)
{
	unsigned short oncount;

	DDRB |= (1<<PORTB5) | (1<<PORTB6) | (1<<PORTB7);// OC1A, OC1B, OC1C 출력설정
	//타이머카운터 1 설정 
	TCCR1A = 0xAA;// Fast PWM 모드, OC1A/B/C 
	
	// 비교 출력 모드 2 사용 
	TCCR1B = 0x18;// WGM13:0 = 14 (ICR1로 TOP 설정)
	
	// Fast PWM의 TOP은 ICR1으로 설정 
	// TOP은 PWM의 주기로부터 설정
	
	ICR1 = period * 2;//주기 period usec
	
	//초기 펄스폭 중립 
	oncount = NEUTRAL_WIDTH * 2;

	OCR1A = oncount;
	OCR1B = oncount;
	OCR1C = oncount;

	TCCR1B |= (2<<CS10);// 분주비 8로 타이머 시작
}

////////////////////////////////////////////////////////////////////////// 
//usec 단위로 펄스폭을 조절 
//onWidth - 펄스폭 배열(usec 단위) 
//////////////////////////////////////////////////////////////////////////


void RCServoSetOnWidth(unsigned short onWidth)
{
	OCR1A = onWidth * 2;// RC servo #0
}
