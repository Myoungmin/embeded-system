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


void UART1_Putch(char ch)	//송신 기본함수
{
	while(!(UCSR1A & 0x20));

	UDR1 = ch;
}


void SendCMD(char cmd, char data)	//규칙 만든 것에 해당하는 송신함수 만든것
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
	ch = UDR1; // 수신
	if(ch == 0x02 && start_RX1 == 0) start_RX1 = 1;	//start_RX1 == 0 이 조건을 생각 못함
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
	ch = UDR1; // 수신

	if(RX == 0 && ch == 0x02) RX = 1;	//RX == 0 조건에서 시작해야한다, 이걸 생각 못했다.
	else if(RX == 1)
	{
		if(ch == 0x03)
		{
			rx_complete = 1;	//시작비트 받은 후 바로 정지비트 받을경우 이것도 신호 분석한다
			RX = 0;	//다시 대기 단계로
		}
		else
		{
			cmd = ch;	//정지비트를 받지않았다면 다음은 커맨드로 저장
			RX = 2;	//커맨드를 받았다면 단계를 하나더 진행시킨다
		}
	}
	else if(RX == 2)	//단계 2일 경우
	{
		if(ch == 0x03)	//정지비트를 받을 경우
		{
			rx_complete = 1;	//정지비트를 받았다면 신호 분석 한다
			RX = 0;	//다시 대기 단계로
		}
		else  //정지 비트를 받지 않았다면
		{
			data = ch;	//다음 신호는 데이터 신호
			RX = 3;	//다음 단계로 
		}
	}
	else if(RX == 3)	//단계 3일 경우
	{
		if(ch == 0x03) rx_complete = 1;	//정지비트를 받았다면 신호 분석 한다
		RX = 0;	//다시 대기 단계 만들어준다
	}

	if(rx_complete == 1)	//신호 분석하는 단계
	{
		if(cmd == 'A') your_number = data - '0';	//이부분 내꺼랑 차이나는듯? data로 변수 한번 더 거침
		else if(cmd == 'B') select = data - '0' + 1;	//1: up, 2:down
		else if(cmd == 'C') select = 0;	//0 : stop

		rx_complete = 0;	//신호분석 플래그 지움
	}
}

ISR(TIMER1_OVF_vect)
{
	TCNT1 = 49911; //타이머카운터 1초만들기 위한 초기값 재설정

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
	TCNT2 = 178;	//타이머카운터 4.99ms로 동적구동을 위한 초기값 재설정

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
	
	DDRA = 0xFF;	//세그먼트 표현부분 출력
	DDRG = 0xFF;	//세가먼트 전원부분 출력
	DDRE = 0x0; //스위치 연결 부분 입력

	EICRA = (2 << ISC00);	 //INT 3 하강에지
	EICRB = (2 << ISC40) | (2 << ISC50) | (2 << ISC60) | (2 << ISC70); //INT 4,5,6,7 하강에지
	EIMSK = (1 << INT0) | (1 << INT4) |  (1 << INT5) |  (1 << INT6) |  (1 << INT7);	//INT 모두 켜기

	TCCR1A = 0x0;
	TCCR1B = 0x05; //일반모드, 1024분주
	TCNT1 = 49911; //타이머카운터 1초만들기 위한 초기값
	
	TCCR2 = 0x05;	//일반모드, 1024분주
	TCNT1 = 178;	//4.99ms로 동적구동을 위한 초기값

	TIMSK = 0b01000100; //TOIE1 = 1, TOIE2 = 1, 타이머 카운터 1 2 오버플로우 인터럽트 인에이블


	sei();	//모든 INT 활성화

	UART_Init();

	while(1)
	{
		
	}
}

void UART_Init(void)
{
	

	UCSR1B = 0x98;	//RXCIEN = 1 수신완료 인터럽트 인에이블, RXEN1 = 1 수신기 인에이블, TXEN1 = 1 송신기 인에이블
	UCSR1C = 0x06;
	UBRR1L = 103;
}
