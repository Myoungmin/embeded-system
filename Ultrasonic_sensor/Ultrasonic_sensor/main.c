#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define TC0 139 //TCNT0 초기값 조정

unsigned char segment2[10] = {
	0b00111111, //0
	0b00000110,	//1
	0b01011011,	//2
	0b01001111,	//3
	0b01100110,	//4
	0b01101101,	//5
	0b01111101,	//6
	0b00000111,	//7
	0b01111111,	//8
	0b01101111, //9
};


unsigned long int timer = 0;	//시간변수

unsigned long int dist_sec = 0;	//음파 이동 거리 시간
unsigned char dist = 0;	//거리
unsigned char dist_1 = 0;	//거리 1의 자리
unsigned char dist_10 = 0;	//10의 자리
unsigned char dist_100 = 0;	//100의 자리
unsigned char number = 0;	//상승 하강 에지 플레그


ISR(INT4_vect)	//
{
	PORTE = 0x08;	//입력 트리거 상승 에지
	_delay_us(10);
	PORTE = 0x00;	//입력 트리거 하강 에지
}

ISR(INT5_vect)	//
{
	if(number == 0)	//초음파 센서 출력 상승 에지
	{
		timer = 0;
		TIMSK = 0x01;	//오버플로우 인터럽트 인에블
		TCCR0 = 0b00000010;	//일반모드, 8분주
		TCNT0 = TC0;	//TCNT0 초기값(8분주와 16MHz일때 58us 주기 생성)
		EICRB = (2 << ISC40) | (2 << ISC50);	//INT4, INT5 하강에지 설정
		EIMSK = (1 << INT4) |  (1 << INT5);	//INT4, INT5 인에블
		number = 1;	//플레그 토글
	}
	
	else if(number == 1)	//초음파 센서 출력 하강 에지
	{
		TIMSK = 0x00;	//오버플로우 타임 인터럽트 디스에이블
		
		EICRB = (2 << ISC40) | (3 << ISC50);	//INT4 하강에지 INT5 상승에지 설정
		EIMSK = (1 << INT4) |  (1 << INT5);	//INT 모두 켜기
		number = 0;	//플레그 토글
	}
}

ISR(TIMER0_OVF_vect)	//오버플로우 타임 인터럽트
{
	TCNT0 = TC0;	//0이 아닐 경우 반드시 써줘야 한다.
	timer++;	//클락당 1증가(1cm 이동거리 시간과 동일)
}



void display();


int main(void)
{
	
	DDRD = 0xFF;	//세그먼트 표현부분 출력
	DDRG = 0xFF;	//세가먼트 전원부분 출력
	DDRE = 0x0F; //스위치 연결 부분 입력

	EICRB = (2 << ISC40) | (3 << ISC50); //INT 4 하강에지, 5 상승에지 설정
	EIMSK = (1 << INT4) |  (1 << INT5);	//INT 모두 켜기


	sei();	//모든 INT 활성화

	while(1)
	{

		dist = timer / ((10000 / 170) / ((256 - TC0) / 2)); //총이동에 걸린 시간을 1cm이동에 걸리는 시간으로 나누어 초음파 센서와 장애물 거리를 계산(이동거리가 2배라 2로 나누어준다)
		display();	//세그먼트 화면 출력
		
	}
}

void display()
{
	dist_100 = dist / 100;
	dist_10 = (dist % 100) / 10;
	dist_1 = (dist % 100) % 10;


	PORTG = 0b00000001;
	PORTD = ~segment2[dist_1];
	_delay_ms(5);
	PORTG = 0b00000010;
	PORTD = ~segment2[dist_10];
	_delay_ms(5);
	PORTG = 0b00000100;
	PORTD = ~segment2[dist_100];
	_delay_ms(5);
}

