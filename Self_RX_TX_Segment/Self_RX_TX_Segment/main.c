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

unsigned char my_number;
unsigned char your_number;
unsigned char start_RX0;
unsigned char end_RX0;
unsigned char start_RX1;
unsigned char end_RX1;

ISR(INT3_vect)
{
	my_number++;
	if(my_number == 10) my_number = 0;
	
	while((UCSR0A & 0x20) == 0x0); //UERE0=1 될 때까지 대기
	UDR0 = 0x02; // 송신
	while((UCSR0A & 0x20) == 0x0); //UERE0=1 될 때까지 대기
	UDR0 = 'A'; // 송신
	while((UCSR0A & 0x20) == 0x0); //UERE0=1 될 때까지 대기
	UDR0 = '0' + my_number; // 송신
	while((UCSR0A & 0x20) == 0x0); //UERE0=1 될 때까지 대기
	UDR0 = 0x03; // 송신

}


ISR(INT4_vect)
{
	my_number--;
	if(my_number < 0 || my_number >= 10) my_number = 9;
	while((UCSR0A & 0x20) == 0x0); //UERE0=1 될 때까지 대기
	UDR0 = 0x02; // 송신
	while((UCSR0A & 0x20) == 0x0); //UERE0=1 될 때까지 대기
	UDR0 = 'A'; // 송신
	while((UCSR0A & 0x20) == 0x0); //UERE0=1 될 때까지 대기
	UDR0 = '0' + my_number; // 송신
	while((UCSR0A & 0x20) == 0x0); //UERE0=1 될 때까지 대기
	UDR0 = 0x03; // 송신
}

ISR(INT5_vect)
{
	
}

ISR(INT6_vect)
{
	
}

ISR(INT7_vect)
{
	
}

ISR(USART0_RX_vect)
{
	unsigned char ch;
	ch = UDR0; // 수신

}


ISR(USART1_RX_vect)
{
	unsigned char ch;
	ch = UDR1; // 수신

}


void display();

int main(void)
{
	
	DDRD = 0xFF;	//세그먼트 표현부분 출력
	DDRG = 0xFF;	//세가먼트 전원부분 출력
	DDRE = 0x0; //스위치 연결 부분 입력

	EICRA = (2 << ISC30);	 //INT 3 하강에지
	EICRB = (2 << ISC40) | (2 << ISC50) | (2 << ISC60) | (2 << ISC70); //INT 4,5,6,7 하강에지
	EIMSK = (1 << INT3) | (1 << INT4) |  (1 << INT5) |  (1 << INT6) |  (1 << INT7);	//INT 모두 켜기

	sei();	//모든 INT 활성화

	UART_Init();

	while(1)
	{
		display();	//세그먼트 화면 출력
	}
}

void UART_Init(void)
{
	UCSR0B = 0x18;	//RXCIEN = 1 수신완료 인터럽트 인에이블, RXEN1 = 1 수신기 인에이블, TXEN1 = 1 송신기 인에이블
	UCSR0C = 0x06;
	UBRR0L = 103;

	UCSR1B = 0x18;	//RXCIEN = 1 수신완료 인터럽트 인에이블, RXEN1 = 1 수신기 인에이블, TXEN1 = 1 송신기 인에이블
	UCSR1C = 0x06;
	UBRR1L = 103;
}

void display()
{
	PORTG = 0b00000001;
	PORTD = ~segment2[my_number];
	_delay_ms(5);
		
	PORTG = 0b00001000;
	PORTD = ~segment2[your_number];
	_delay_ms(5);	
}