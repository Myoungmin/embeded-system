#include <avr/io.h>
#include <avr/interrupt.h>
unsigned char string[1024];
unsigned long int i = 0;

void main(void)
{
	//DDRD = 0b00001000; TX RX 이거 써주면 안 된다
	//DDRE = 0b00000010;
	// USART 초기화
	UCSR0A = 0x0;
	UCSR0B = 0b10011000;	 // RXCIE0 : 수신 완료 인터럽트 인에이블
	// 송수신 인에이블 TXEN0 = 1, RXEN0=1
	UCSR0C = 0b00000110; // 비동기 데이터 8비트 모드
	UBRR0H = 0;// X-TAL = 16MHz 일때, BAUD = 9600
	UBRR0L = 103;

	UCSR1A = 0x0;
	UCSR1B = 0b10011000;	 // RXCIE1 : 수신 완료 인터럽트 인에이블
	// 송수신 인에이블 TXEN1 = 1, RXEN1=1
	UCSR1C = 0b00000110; // 비동기 데이터 8비트 모드
	UBRR1H = 0;// X-TAL = 16MHz 일때, BAUD = 9600
	UBRR1L = 103;


	sei();

	while(1);

	
}

ISR(USART0_RX_vect)
{
	
	string[i] = UDR0; // USART0으로 수신

	if(string[i] == '\n')
	{
		
		for(int j = 0; j <= i;j++ )
		{
			while((UCSR1A & 0x20) == 0x0); //UERE1=1 될 때까지 대기
			UDR1 = string[j]; // USART1로 송신
		}
		i = 0;
	}
	else i++;
}

ISR(USART1_RX_vect)
{
	
	string[i] = UDR1; // USART1로 수신

	if(string[i] == '\n')
	{
		
		for(int j = 0; j <= i;j++ )
		{
			while((UCSR0A & 0x20) == 0x0); //UERE0=1 될 때까지 대기
			UDR0 = string[j]; // USART0으로 송신
		}
		i = 0;
	}
	else i++;
}
