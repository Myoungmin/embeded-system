#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define TC0 240	//TCNT0 초기값 조정(스톱워치 마이크로 초 설정위한 초기값)
#define TC1 139	//TCNT0 초기값 조정(초음파 센서용)


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

unsigned char version = 0;	//초음파, 스톱워치 모드 설정 변수 ,0일때 스탑워치 1일때 초음파


unsigned long int timer = 0;	//시간변수

unsigned long int dist_sec = 0;	//음파 이동 거리 시간
unsigned char dist = 0;	//거리
unsigned char dist_1 = 0;	//거리 1의 자리
unsigned char dist_10 = 0;	//10의 자리
unsigned char dist_100 = 0;	//100의 자리
unsigned char number = 0;	//상승 하강 에지 플레그




long int ut = 0;	//마이크로 세컨드 변수 가장 작은 단위
long int t = 0;  //초단위 시각 변수
long int t_1 = 0;	//초 저장 변수
long int t_2 = 0;
long int t_3 = 0;
long int stop = 0;	//스탑워치 잠시 멈출때 저장하는 변수
unsigned char sec_10 = 0;	//시간 10의 자리
unsigned char sec_1 = 0;	//시간 1의 자리
unsigned char msec_10 = 0;	//분 10의 자리
unsigned char msec_1 = 0;	//분 1의 자리

unsigned char phase = 0;	//3번 저장할 때 단계 변화 변수
unsigned char mode = 0;	//스탑워치 잠시 멈추게하기 위한 변수



ISR(INT4_vect)	//스톱워치에서느 초 멈추게하는 역할, 초음파 센서에서는 거리 측정 역할
{
	if(version == 0)	//스톱워치 초 멈추게하는 버튼역할
	{
		if(mode == 0)	//초 멈추게 하기(누를 때 초 저장)
		{
			mode = 1;
			stop = t;
		}
		else if(mode == 1)	//다시 누르면 처음부터 시작
		{
			mode = 0;
			t = 0;
			phase = 0;	//카운팅 되는 단계
		}
	}
	if(version == 1)	//초음파 센서 거리 측정 버튼 역할
	{
		PORTE = 0x08;	//입력 트리거 상승 에지
		_delay_us(10);
		PORTE = 0x00;	//입력 트리거 하강 에지

	}
}

ISR(INT5_vect)	//초 저장하는 인터럽트
{
	if(version == 0)	//스톱워치 초 저장하는 버튼역할
	{
		
			
		if(phase == 0) t_1 = t;	//첫번째 저장
		if(phase == 1) t_2 = t;
		if(phase == 2) t_3 = t;
		if(phase == 3)	t = 0;	//0초로
		phase++;	//초 저장하면 phase 변화
		if(phase == 4) phase = 0;	//0초로 다시
	}
}

ISR(INT6_vect)	//초음파 센서 출력값 입력 받는 인터럽트
{
	if(version == 1)	//초음파 센서 출력 상승 하강 받을 때
	{
		if(number == 0)	//초음파 센서 출력 상승 에지
		{
			timer = 0;
			TCCR0 = 0b00000010;	//일반모드, 8분주
			TCNT0 = TC1;	//TCNT0 초기값(8분주와 16MHz일때 58us 주기 생성)
			EICRB = (2 << ISC40) | (2 << ISC60) | (2 << ISC70);	//INT4, INT6, INT7 하강에지 설정
			EIMSK = (1 << INT4) |  (1 << INT6) |  (1 << INT7);	//INT4, INT6, INT7 인에블
			TIMSK = 0x01;	//오버플로우 인터럽트 인에블
			
			number = 1;	//플레그 토글
		}
		
		else if(number == 1)	//초음파 센서 출력 하강 에지
		{
			TIMSK = 0x00;	//오버플로우 타임 인터럽트 디스에이블
			EICRB = (2 << ISC40) | (3 << ISC60) | (2 << ISC70);	//INT4, INT7 하강에지 INT6 상승에지 설정
			EIMSK = (1 << INT4) |  (1 << INT6) |  (1 << INT7);	//INT 모두 켜기

			number = 0;	//플레그 토글
		}

	}
}

ISR(INT7_vect)	//(스톱워치, 초음파 센서) 바꿔주는 인터럽트
{
	if(version == 0)	//스톱워치일때 초음파 초기값 설정
	{
		cli();	//인터럽트 모드 해제
		
		DDRD = 0xFF;	//세그먼트 표현부분 출력
		DDRG = 0xFF;	//세가먼트 전원부분 출력
		DDRE = 0x0F; //스위치 연결 부분 입력
		DDRB = 0x10;	//OCR0사용하기 위해

		EICRB = (2 << ISC40) | (3 << ISC60) | (2 << ISC70); //INT 4, INT7 하강에지, 6 상승에지 설정
		EIMSK = (1 << INT4) |  (1 << INT6) |  (1 << INT7);	//INT 모두 켜기


		sei();	//모든 INT 활성화

		PORTE = 0x08;	//입력 트리거 상승 에지
		_delay_us(10);
		PORTE = 0x00;	//입력 트리거 하강 에지(이거 안 해주면 스톱워치에서 초음파 넘어올 때 번호가 떨리면서 순간적으로 바뀌는 현상 관찰됨 그래서 바뀌었을때 한번 읽어줬다)

		version = 1;	//초음파로 변경
	}
	else if(version == 1)	//초음파일때 스톱워치 초기값 설정
	{
		cli();
		
		DDRD = 0xFF;	//세그먼트 표현부분 출력
		DDRG = 0xFF;	//세그먼트 전원부분 출력
		DDRE = 0x0; //스위치 연결 부분 입력

		EICRB = (2 << ISC40) | (2 << ISC50) | (2 << ISC70); //INT 4, 5, 7 하강에지
		EIMSK = (1 << INT4) |  (1 << INT5) |  (1 << INT7);	//INT 4, 5, 7 켜기

		
		TCCR0 = 0x07;	//노멀모드 오버플로우 인터럽트 1024분주
		TCNT0 = TC0;	//스톱워치 마이크로 초 설정위한 초기값(계산상으로는 192로 해서 us 초 단위로 만들고 1000번 헛돌게 해서 1ms를 만들어서 하려 했지만 세그먼트 관측상으로는 많이 느렸다.)
		TIMSK = 0x01;	//타임 인터럽트 인에이블

		sei();	//모든 INT 활성화
		version = 0;	//스톱워치로 변경
	}
}

ISR(TIMER0_OVF_vect)	//오버 플로우 타임 인터럽트
{
	
	if(version == 0)	//스톱워치일때
	{
		ut++;	//마이크로 초 변수 증가
		if(ut = 1000)	//1000마이크로 초 경과 시
		{
			TCNT0 = TC0;	//0이 아닐 경우 반드시 써줘야 한다.
			ut = 0;	//마이크로 초 초기화
			t++;	//1밀리 초 증가
			if(t == 100000) t = 0;	//100초일때 초기화
		}
	}
	if(version == 1)	//초음파 센서일때
	{
		TCNT0 = TC1;	//0이 아닐 경우 반드시 써줘야 한다.
		timer++;	//클락당 1증가(1cm 이동거리 시간과 동일)
	}
}


void display0(long int t);
void display1();

int main(void)
{
	DDRD = 0xFF;	//세그먼트 표현부분 출력
	DDRG = 0xFF;	//세가먼트 전원부분 출력
	DDRE = 0x0; //스위치 연결 부분 입력
	DDRB = 0x10;	//OCR0사용하기 위해


	EICRB = (2 << ISC40) | (2 << ISC50) | (2 << ISC70); //INT 4, 5, 7 하강에지
	EIMSK = (1 << INT4) |  (1 << INT5) |  (1 << INT7);	//INT 4, 5 켜기

	
	TCCR0 = 0x07;	//노멀모드 오버플로우 인터럽트 1024분주
	TCNT0 = TC0;	//스톱워치 마이크로 초 설정위한 초기값
	TIMSK = 0x01;	//타임 인터럽트 인에이블

	sei();	//모든 INT 활성화
	
	while(1)
	{
		if(version == 0)	//스톱워치일때
		{
			if(phase == 3)	//저장된 3개의 초 표시
			{
				for(int i = 0; i < 50; i++) display0(t_1);
				for(int i = 0; i < 50; i++) display0(t_2);
				for(int i = 0; i < 50; i++) display0(t_3);
			}
			else if(mode == 1) display0(stop);	//멈췄을 때 숫자표시
			else display0(t);	//초 진행 중 표시
		}
		else if(version == 1)	//초음파 센서일때
		{
			dist = timer / ((10000 / 170) / ((256 - TC1) / 2)); //총이동에 걸린 시간을 1cm이동에 걸리는 시간으로 나누어 초음파 센서와 장애물 거리를 계산(이동거리가 2배라 2로 나누어준다)
			if(dist >= 20) dist = 20;	//20이상일때 20한정
			if(dist <= 2) dist = 2;	//2이하일때 2한정
			display1();	//세그먼트 화면 출력
			TCCR0 = 0b01100100;	//phase correct PWM,비교매치에서 OC0 클리어, 64분주
			OCR0 = dist * 8;	//거리에 따라 OCR값 증가하여 high 비율 증가 시킨다.
		}
	}
}

void display0(long int t)	//스톱워치 숫자표시 함수
{
	sec_10 = t / 10000;
	sec_1 = (t % 10000) / 1000;
	msec_10 = ((t % 10000) % 1000) / 100;
	msec_1 = (((t % 10000) % 1000) % 100) / 10;
	PORTG = 0b00000001;
	PORTD = ~segment2[msec_1];
	_delay_ms(5);
	PORTG = 0b00000010;
	PORTD = ~segment2[msec_10];
	_delay_ms(5);
	PORTG = 0b00000100;
	PORTD = ~segment2[sec_1];
	_delay_ms(5);
	PORTG = 0b00001000;
	PORTD = ~segment2[sec_10];
	_delay_ms(5);
	if(phase == 0) PORTG = 0b00000001;
	if(phase == 1) PORTG = 0b00000010;
	if(phase == 2) PORTG = 0b00000100;
	if(phase == 3) PORTG = 0b00001000;
	PORTD = 0b01111111;
	_delay_ms(5);
}

void display1()	//초음파 센서 숫자표시 함수
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



