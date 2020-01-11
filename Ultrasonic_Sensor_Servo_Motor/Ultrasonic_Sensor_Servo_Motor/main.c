#include <avr/io.h>
#include "rcservo.h"
#include <util/delay.h>
#include <avr/interrupt.h>

#define TC0 139 //TCNT0 초기값 조정

unsigned long int timer = 0;	//시간변수
unsigned char dist = 0;	//거리
unsigned char number = 0;	//상승 하강 에지 플레그
unsigned char angle[4] = {0xFF, 0xFF, 0xFF, 0xFF};	//세그먼트 표시 초기화
unsigned long int width;	//펄스폭, 서보모터 위치 결정

ISR(INT6_vect)	//
{
	if(number == 0)	//초음파 센서 출력 상승 에지
	{
		timer = 0;	//시간 초기화
		TIMSK = 0x01;	//오버플로우 인터럽트 인에블
		TCCR0 = 0b00000010;	//일반모드, 8분주
		TCNT0 = TC0;	//TCNT0 초기값(8분주와 16MHz일때 58us 주기 생성)
		EICRB = (2 << ISC60);	//INT6 하강에지 설정
		EIMSK = (1 << INT6);	//INT6 인에블
		number = 1;	//플레그 토글
	}
	
	else if(number == 1)	//초음파 센서 출력 하강 에지
	{
		TIMSK = 0x00;	//오버플로우 타임 인터럽트 디스에이블
		
		EICRB = (3 << ISC60);	//IINT6 상승에지 설정
		EIMSK = (1 << INT6);	//INT6 켜기
		number = 0;	//플레그 토글
	}
}

ISR(TIMER0_OVF_vect)	//오버플로우 타임 인터럽트
{
	TCNT0 = TC0;	//0이 아닐 경우 반드시 써줘야 한다.
	timer++;	//클락당 1증가(1cm 이동거리 시간과 동일)
}

void distance();
void display();

int main(void)
{ 
	RCServoInit(10000);
	   
	while (1) 
    {
		DDRD = 0xFF;	//세그먼트 표현부분 출력
		DDRG = 0xFF;	//세가먼트 전원부분 출력
		DDRE = 0x0F; //초음파 센서 입, 출력 연결 부분 입력


		EICRB = (3 << ISC60); //INT 6 상승에지 설정
		EIMSK = (1 << INT6);	//INT 6 켜기
		TIMSK |= 0b00000100;	//
		sei();	//인터럽트 인에이블



		for(width = 700 ; width < 2300 ; width += 10) //최소 최대 사이에서 펄스폭 10씩 증가
		{
			RCServoSetOnWidth(width);	//모터 위치 설정
			

			//초음파센서에 입력신호 출력해준다
			PORTE = 0x08;	//입력 트리거 상승 에지
			_delay_us(10);
			PORTE = 0x00;	//입력 트리거 하강 에지

			dist = timer / ((10000 / 170) / ((256 - TC0) / 2));	//거리계산 cm 단위

			distance();
			display();
		}

		for(width = 2300 ; width > 700 ; width -= 10)	//최소 최대 사이에서 펄스폭 10씩 감소
		{
			RCServoSetOnWidth(width);
			
			PORTE = 0x08;	//입력 트리거 상승 에지
			_delay_us(10);
			PORTE = 0x00;	//입력 트리거 하강 에지

			dist = timer / ((10000 / 170) / ((256 - TC0) / 2));

			distance();
			display();
		}
    }
}


void distance()	//특정 지점에서 거리 세그먼트 출력값으로 바꿔주는 함수
{
	if(width == 710)
	{
		if(dist <= 10)	angle[3] = 0b11110111;
		else if(dist > 10 && dist <= 100) angle[3] = 0b10111111;
		else if(dist > 100)	angle[3] = 0b11111110;
	}
	if(width == 1250)
	{
		if(dist <= 10)	angle[2] = 0b11110111;
		else if(dist > 10 && dist <= 100) angle[2] = 0b10111111;
		else if(dist > 100)	angle[2] = 0b11111110;
	}
	if(width == 1750)
	{
		if(dist <= 10)	angle[1] = 0b11110111;
		else if(dist > 10 && dist <= 100) angle[1] = 0b10111111;
		else if(dist > 100)	angle[1] = 0b11111110;
	}
	if(width == 2290)
	{
		if(dist <= 10)	angle[0] = 0b11110111;
		else if(dist > 10 && dist <= 100) angle[0] = 0b10111111;
		else if(dist > 100)	angle[0] = 0b11111110;
	}
}


void display()	//세그먼트 출력 함수
{
	
	PORTG = 0b00000001;
	PORTD = angle[3];
	_delay_ms(5);
	PORTG = 0b00000010;
	PORTD = angle[2];
	_delay_ms(5);
	PORTG = 0b00000100;
	PORTD = angle[1];
	_delay_ms(5);
	PORTG = 0b00001000;
	PORTD = angle[0];
	_delay_ms(5);
}
