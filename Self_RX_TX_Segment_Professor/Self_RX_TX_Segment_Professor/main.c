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

char my_number;
char your_number;
unsigned char display_on_number = 1;

unsigned char start_RX0;
unsigned char A_mode0;
unsigned char B_mode0;
unsigned char C_mode0;
unsigned char up_count0;
unsigned char down_count0;


unsigned char start_RX1;
unsigned char A_mode1;
unsigned char B_mode1;
unsigned char C_mode1;
unsigned char up_count1;
unsigned char down_count1;


void UART1_Putch(char ch)	//�۽� �⺻�Լ�
{
	while(!(UCSR1A & 0x20));

	UDR1 = ch;
}


void SendCMD(char cmd, char data)	//��Ģ ���� �Ϳ� �ش��ϴ� �۽��Լ� �����
{
	UART1_Putch(0x02);
	UART1_Putch(cmd);
	if(cmd == 'A') UART1_Putch(data + '0');
	else if(cmd == 'B')	UART1_Putch(data);
	UART1_Putch(0x03);
}







ISR(INT0_vect)
{
	up_count0 = 0;
	down_count0 = 0;

	if(++my_number == 10) my_number = 0;
	
	SendCMD('A', my_number);

}


ISR(INT4_vect)
{
	
	up_count0 = 0;
	down_count0 = 0;

	SendCMD('C', 0);
	
}

ISR(INT5_vect)
{
	up_count0 = 0;
	down_count0 = 1;

	SendCMD('B', '1');
	
}

ISR(INT6_vect)
{
	
	up_count0 = 1;
	down_count0 = 0;

	SendCMD('B', '0');
	
}

ISR(INT7_vect)
{
	up_count0 = 0;
	down_count0 = 0;

	if(my_number != 0) --my_number;
	else my_number = 9;

	SendCMD('A', my_number);
}


char RX = 0;
char rx_complete = 0;
char cmd, data;
char select = 0;



ISR(USART1_RX_vect)
{
	/*unsigned char ch;
	ch = UDR1; // ����
	if(ch == 0x02 && start_RX1 == 0) start_RX1 = 1;	//start_RX1 == 0 �� ������ ���� ����
	else if(start_RX1 == 1 && ch == 0x03)
	{
		start_RX1 = 0;
		A_mode0 = 0;
		B_mode0 = 0;
		C_mode0 = 0;
	}

	if(start_RX1 == 1 && ch == 'A') A_mode1 = 1;
	else if(start_RX1 == 1 && ch == 'B') B_mode1 = 1;
	else if(start_RX1 == 1 && ch == 'C') C_mode1 = 1;

	if(start_RX1 == 1 && A_mode1 == 1) 
	{
		B_mode0 = 0;
		C_mode0 = 0;
		up_count1 = 0;
		down_count1 = 0;
		your_number = ch - '0';
	}

	else if(start_RX1 == 1 && B_mode1 == 1 && ch == '0') 
	{
		A_mode0 = 0;
		C_mode0 = 0;
		up_count1 = 1;
		down_count1 = 0;
	}
	else if(start_RX1 == 1 && B_mode1 == 1 && ch == '1')
	{
		A_mode0 = 0;
		C_mode0 = 0;
		up_count1 = 0;
		down_count1 = 1;
	}
	else if(start_RX1 == 1 && C_mode1 == 1) 
	{
		A_mode0 = 0;
		B_mode0 = 0;
		up_count1 = 0;
		down_count1 = 0;
	}*/

	unsigned char ch;
	ch = UDR1; // ����

	if(RX == 0 && ch == 0x02) RX = 1;	//RX == 0 ���ǿ��� �����ؾ��Ѵ�, �̰� ���� ���ߴ�.
	else if(RX == 1)
	{
		if(ch == 0x03)
		{
			rx_complete = 1;	//���ۺ�Ʈ ���� �� �ٷ� ������Ʈ ������� �̰͵� ��ȣ �м��Ѵ�
			RX = 0;	//�ٽ� ��� �ܰ��
		}
		else
		{
			cmd = ch;	//������Ʈ�� �����ʾҴٸ� ������ Ŀ�ǵ�� ����
			RX = 2;	//Ŀ�ǵ带 �޾Ҵٸ� �ܰ踦 �ϳ��� �����Ų��
		}
	}
	else if(RX == 2)	//�ܰ� 2�� ���
	{
		if(ch == 0x03)	//������Ʈ�� ���� ���
		{
			rx_complete = 1;	//������Ʈ�� �޾Ҵٸ� ��ȣ �м� �Ѵ�
			RX = 0;	//�ٽ� ��� �ܰ��
		}
		else  //���� ��Ʈ�� ���� �ʾҴٸ�
		{
			data = ch;	//���� ��ȣ�� ������ ��ȣ
			RX = 3;	//���� �ܰ�� 
		}
	}
	else if(RX == 3)	//�ܰ� 3�� ���
	{
		if(ch == 0x03) rx_complete = 1;	//������Ʈ�� �޾Ҵٸ� ��ȣ �м� �Ѵ�
		RX = 0;	//�ٽ� ��� �ܰ� ������ش�
	}

	if(rx_complete == 1)	//��ȣ �м��ϴ� �ܰ�
	{
		if(cmd == 'A') your_number = data - '0';	//�̺κ� ������ ���̳��µ�? data�� ���� �ѹ� �� ��ħ
		else if(cmd == 'B') select = data - '0' + 1;	//1: up, 2:down
		else if(cmd == 'C') select = 0;	//0 : stop

		rx_complete = 0;	//��ȣ�м� �÷��� ����
	}
}

ISR(TIMER1_OVF_vect)
{
	TCNT1 = 49911; //Ÿ�̸�ī���� 1�ʸ���� ���� �ʱⰪ �缳��

	if(up_count0 == 1)
	{
		//if(++my_number == 10) my_number = 0;
	}
	
	else if(down_count0 == 1)
	{
		//if(my_number != 0) --my_number;
		//else my_number = 9;
	}

	/*if(up_count1 == 1)
	{
		if(++your_number == 10) your_number = 0;
	}
	
	else if(down_count1 == 1)
	{
		if(your_number != 0) --your_number;
		else your_number = 9;
	}*/

	if(select == 1)
	{
		if(++your_number == 10) your_number = 0;
	}
	
	else if(select== 2)
	{
		if(your_number != 0) --your_number;
		else your_number = 9;
	}
	
	
}

ISR(TIMER2_OVF_vect)
{
	TCNT2 = 178;	//Ÿ�̸�ī���� 4.99ms�� ���������� ���� �ʱⰪ �缳��

	if(display_on_number == 1)
	{
		PORTG = display_on_number;
		PORTA = ~segment2[my_number];
		display_on_number = 8;
	}
	
	else if(display_on_number == 8)
	{
		PORTG = display_on_number;
		PORTA = ~segment2[your_number];
		display_on_number = 1;
	}
}



int main(void)
{
	
	DDRA = 0xFF;	//���׸�Ʈ ǥ���κ� ���
	DDRG = 0xFF;	//������Ʈ �����κ� ���
	DDRE = 0x0; //����ġ ���� �κ� �Է�

	EICRA = (2 << ISC00);	 //INT 3 �ϰ�����
	EICRB = (2 << ISC40) | (2 << ISC50) | (2 << ISC60) | (2 << ISC70); //INT 4,5,6,7 �ϰ�����
	EIMSK = (1 << INT0) | (1 << INT4) |  (1 << INT5) |  (1 << INT6) |  (1 << INT7);	//INT ��� �ѱ�

	TCCR1A = 0x0;
	TCCR1B = 0x05; //�Ϲݸ��, 1024����
	TCNT1 = 49911; //Ÿ�̸�ī���� 1�ʸ���� ���� �ʱⰪ
	
	TCCR2 = 0x05;	//�Ϲݸ��, 1024����
	TCNT1 = 178;	//4.99ms�� ���������� ���� �ʱⰪ

	TIMSK = 0b01000100; //TOIE1 = 1, TOIE2 = 1, Ÿ�̸� ī���� 1 2 �����÷ο� ���ͷ�Ʈ �ο��̺�


	sei();	//��� INT Ȱ��ȭ

	UART_Init();

	while(1)
	{
		
	}
}

void UART_Init(void)
{
	

	UCSR1B = 0x98;	//RXCIEN = 1 ���ſϷ� ���ͷ�Ʈ �ο��̺�, RXEN1 = 1 ���ű� �ο��̺�, TXEN1 = 1 �۽ű� �ο��̺�
	UCSR1C = 0x06;
	UBRR1L = 103;
}
