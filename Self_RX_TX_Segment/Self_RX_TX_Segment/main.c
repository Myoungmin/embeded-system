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
	
	while((UCSR0A & 0x20) == 0x0); //UERE0=1 �� ������ ���
	UDR0 = 0x02; // �۽�
	while((UCSR0A & 0x20) == 0x0); //UERE0=1 �� ������ ���
	UDR0 = 'A'; // �۽�
	while((UCSR0A & 0x20) == 0x0); //UERE0=1 �� ������ ���
	UDR0 = '0' + my_number; // �۽�
	while((UCSR0A & 0x20) == 0x0); //UERE0=1 �� ������ ���
	UDR0 = 0x03; // �۽�

}


ISR(INT4_vect)
{
	my_number--;
	if(my_number < 0 || my_number >= 10) my_number = 9;
	while((UCSR0A & 0x20) == 0x0); //UERE0=1 �� ������ ���
	UDR0 = 0x02; // �۽�
	while((UCSR0A & 0x20) == 0x0); //UERE0=1 �� ������ ���
	UDR0 = 'A'; // �۽�
	while((UCSR0A & 0x20) == 0x0); //UERE0=1 �� ������ ���
	UDR0 = '0' + my_number; // �۽�
	while((UCSR0A & 0x20) == 0x0); //UERE0=1 �� ������ ���
	UDR0 = 0x03; // �۽�
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
	ch = UDR0; // ����

}


ISR(USART1_RX_vect)
{
	unsigned char ch;
	ch = UDR1; // ����

}


void display();

int main(void)
{
	
	DDRD = 0xFF;	//���׸�Ʈ ǥ���κ� ���
	DDRG = 0xFF;	//������Ʈ �����κ� ���
	DDRE = 0x0; //����ġ ���� �κ� �Է�

	EICRA = (2 << ISC30);	 //INT 3 �ϰ�����
	EICRB = (2 << ISC40) | (2 << ISC50) | (2 << ISC60) | (2 << ISC70); //INT 4,5,6,7 �ϰ�����
	EIMSK = (1 << INT3) | (1 << INT4) |  (1 << INT5) |  (1 << INT6) |  (1 << INT7);	//INT ��� �ѱ�

	sei();	//��� INT Ȱ��ȭ

	UART_Init();

	while(1)
	{
		display();	//���׸�Ʈ ȭ�� ���
	}
}

void UART_Init(void)
{
	UCSR0B = 0x18;	//RXCIEN = 1 ���ſϷ� ���ͷ�Ʈ �ο��̺�, RXEN1 = 1 ���ű� �ο��̺�, TXEN1 = 1 �۽ű� �ο��̺�
	UCSR0C = 0x06;
	UBRR0L = 103;

	UCSR1B = 0x18;	//RXCIEN = 1 ���ſϷ� ���ͷ�Ʈ �ο��̺�, RXEN1 = 1 ���ű� �ο��̺�, TXEN1 = 1 �۽ű� �ο��̺�
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