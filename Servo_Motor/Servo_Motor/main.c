#include <avr/io.h>
#include "rcservo.h"
#include <util/delay.h>
#include <avr/interrupt.h>


int main(void)
{ 
	RCServoInit(10000);
	   
	while (1) 
    {
		// 스위치를 이용한 width 변경부 추가
		RCServoSetOnWidth(MIN_WIDTH);// 펄스폭 설정
		_delay_ms(1000);
		//width[0] = MIN_WIDTH;
		RCServoSetOnWidth(NEUTRAL_WIDTH);
		_delay_ms(1000);
		//width[0] = MAX_WIDTH;
		RCServoSetOnWidth(MAX_WIDTH);
		_delay_ms(1000);
		// 펄스폭 표시 디스플레이 추가
    }
}

