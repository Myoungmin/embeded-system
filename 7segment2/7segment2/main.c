#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>


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

long int t = 0;  //초단위 시각 변수
long int l = 300; //알람시각 변수
unsigned char hour_10 = 0;	//시간 10의 자리
unsigned char hour_1 = 0;	//시간 1의 자리
unsigned char min_10 = 0;	//분 10의 자리
unsigned char min_1 = 0;	//분 1의 자리

unsigned char phase = 0;	//각 자리 단계 변수
unsigned char mode = 0;	//알람 시간 모드변경 변수
unsigned char beep = 0;	//알람 on/off 변수


ISR(INT4_vect)	//시간 알람 변화 인터럽트
{
	if(phase == 0 && mode == 0) t = t + 60;	//시간 각 자리
	if(phase == 1 && mode == 0) t = t + 600;
	if(phase == 2 && mode == 0) t = t + 3600;
	if(phase == 3 && mode == 0) t = t + 36000;
	if(t >= 86400) t = 0;	//24시가 넘을때 초기화

	if(phase == 0 && mode == 1) l = l + 60;	//알람 각 자리
	if(phase == 1 && mode == 1) l = l + 600;
	if(phase == 2 && mode == 1) l = l + 3600;
	if(phase == 3 && mode == 1) l = l + 36000;
	if(l >= 86400) l = 0;	//24시가 넘을때 초기화
}

ISR(INT5_vect)	//자리 단계 인터럽트
{
	phase++;
	if(phase == 4) phase = 0;
}

ISR(INT6_vect)	//알람 시간 모드 변경 인터럽트
{
	mode++;
	if(mode == 2) mode = 0;
}

ISR(INT7_vect)	//알람 on/off 인터럽트
{
	beep = 0;
}


void display();

int main(void)
{
	
	DDRB = 0xFF;	//세그먼트 표현부분 출력
	DDRA = 0xFF;	//세가먼트 전원부분 출력
	DDRE = 0x0; //스위치 연결 부분 입력

	EICRB = (2 << ISC40) | (2 << ISC50) | (2 << ISC60) | (2 << ISC70); //INT 4,5,6,7 하강에지
	EIMSK = (1 << INT4) |  (1 << INT5) |  (1 << INT6) |  (1 << INT7);	//INT 모두 켜기

	sei();	//모든 INT 활성화

	while(1)
	{
		t++;
		if(t >= 86400) t = 0; //24시 0으로
		
		display();	//세그먼트 화면 출력
	}
}

void display()
{
	if(mode == 0 && beep ==0)
	{
		hour_10 = t / 36000;
		hour_1 = (t % 36000) / 3600;
		min_10 = ((t % 36000) % 3600) / 600;
		min_1 = (((t % 36000) % 3600) % 600) / 60;
		PORTA = 0b00000001;
		PORTB = ~segment2[min_1];
		_delay_ms(5);
		PORTA = 0b00000010;
		PORTB = ~segment2[min_10];
		_delay_ms(5);
		PORTA = 0b00000100;
		PORTB = ~segment2[hour_1];
		_delay_ms(5);
		PORTA = 0b00001000;
		PORTB = ~segment2[hour_10];
		_delay_ms(5);
		if(phase == 0) PORTA = 0b00000001;	//각 자릭 단계에 해당될때 세그먼트 전원 키기
		if(phase == 1) PORTA = 0b00000010;
		if(phase == 2) PORTA = 0b00000100;
		if(phase == 3) PORTA = 0b00001000;
		PORTB = 0b01111111;	//켜진 세그먼트에 dot 하나만 출력
		_delay_ms(5);
	}
	if(mode == 1 && beep == 0)
	{
		hour_10 = l / 36000;
		hour_1 = (l % 36000) / 3600;
		min_10 = ((l % 36000) % 3600) / 600;
		min_1 = (((l % 36000) % 3600) % 600) / 60;
		PORTA = 0b00000001;
		PORTB = ~segment2[min_1];
		_delay_ms(5);
		PORTA = 0b00000010;
		PORTB = ~segment2[min_10];
		_delay_ms(5);
		PORTA = 0b00000100;
		PORTB = ~segment2[hour_1];
		_delay_ms(5);
		PORTA = 0b00001000;
		PORTB = ~segment2[hour_10];
		_delay_ms(5);
		if(phase == 0) PORTA = 0b00000001;
		if(phase == 1) PORTA = 0b00000010;
		if(phase == 2) PORTA = 0b00000100;
		if(phase == 3) PORTA = 0b00001000;
		PORTB = 0b01111111;
		_delay_ms(5);
	}
	if(l == t)	//알람시간이 되었을 때
	{
		beep = 1;
	}
	if(beep == 1)	//알람 시간	세그먼트 표시
	{
		static unsigned char a = 0;
		if(a == 0)
		{
			for(int i = 0; i < 5; i++)
			{
				PORTA = 0b00001000;
				PORTB = 0b10111111;
				_delay_ms(5);
			}
		}
		if(a == 1)
		{
			for(int i = 0; i < 5; i++)
			{
				PORTA = 0b00000100;
				PORTB = 0b10111111;
				_delay_ms(5);
			}
		}
		if(a == 2)
		{
			for(int i = 0; i < 5; i++)
			{
				PORTA = 0b00000010;
				PORTB = 0b10111111;
				_delay_ms(5);
			}
		}
		if(a == 3)
		{
			for(int i = 0; i < 5; i++)
			{
				PORTA = 0b00000001;
				PORTB = 0b10111111;
				_delay_ms(5);
			}
		}
		if(a == 4)
		{
			for(int i = 0; i < 5; i++)
			{
				PORTA = 0b00001111;
				PORTB = 0b10111111;
				_delay_ms(5);
			}
		}
		a++;
		if(a == 5) a = 0;
	}
}


