#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>
#include <avr/eeprom.h>

#define EEPROM (const uint8_t*)1

#define CMD_NEXT	0x01
#define CMD_PRE		0x02
#define CMD_PLAY	0x0d
#define CMD_PAUSE	0x0E
#define CMD_STOP	0x16
#define CMD_TF		0x09

char cmd_list[10] = {CMD_PLAY, CMD_STOP, CMD_PAUSE, CMD_NEXT, CMD_PRE,};


void UART_Init(void)
{
	UCSR0B = 0x18;	//수신, 송신 인에블
	UCSR0C = 0x06;
	UBRR0H = 0;
	UBRR0L = 103;

	UCSR1B |= (1<<RXCIE1) | (1<<RXEN1);	//수신, 수신인터럽트 인에블
	UCSR1C = 0x06;	//8비트 전송
	UBRR1H = 0;
	UBRR1L = 103;
}

void UART0_Putch(char ch)	//송신 기본함수
{
	while(!(UCSR0A & 0x20));

	UDR0 = ch;
}

void UART1_Putch(char ch)	//송신 기본함수
{
	while(!(UCSR1A & 0x20));

	UDR1 = ch;
}


void SendCommand(unsigned char cmd)
{
	unsigned int checksum = 0;
	char temp[20];

	UART0_Putch(0x7E);
	UART0_Putch(0xFF);
	UART0_Putch(0x06);
	UART0_Putch(cmd);
	UART0_Putch(0x00);
	UART0_Putch(0x00);
	
	if(cmd != CMD_TF){
		UART0_Putch(0x00);
		checksum = 0 - (0xFF + 0x06 + cmd);
	}
	else{
		UART0_Putch(0x02);
		checksum = 0 - (0xFF + 0x06 + cmd + 0x02);
	}

	UART0_Putch((char)(checksum >> 8));
	UART0_Putch(checksum&0xFF);
	UART0_Putch(0xEF);
}


void SendCommand01(unsigned char cmd, unsigned char param1, unsigned char param2)
{
	unsigned int checksum = 0;
	char temp[20];

	UART0_Putch(0x7E);
	UART0_Putch(0xFF);
	UART0_Putch(0x06);
	UART0_Putch(cmd);
	UART0_Putch(0x00);
	UART0_Putch(param1);
	UART0_Putch(param2);
	

	checksum = 0 - (0xFF + 0x06 + cmd + param1 + param2);

	UART0_Putch((char)(checksum >> 8));
	UART0_Putch(checksum&0xFF);
	UART0_Putch(0xEF);
}




typedef unsigned char u_char;

#define FUNCSET 0x28 // Function Set
#define ENTMODE 0x06 // Entry Mode Set
#define ALLCLR 0x01 // All Clear
#define DISPON 0x0c // Display On
#define LINE2 0xC0 // 2nd Line Move
#define ENH PORTC |= 0x04
#define ENL PORTC &= 0xFB

void LCD_init(void); // LCD 초기화
void LCD_String(char []); // 문자열 출력
void Busy(void); // 2ms 딜레이 함수
void Command(u_char); // 명령어 전송
void Data(u_char); // 데이터 전송


char ch = 0;
char state = 0;
char password[11] ={'0', '0', '0', '0', '\0'};	//비밀번호
char input[11];	//입력값
char old_password[11] = {0};
char new_password[11] = {0};	//새로운 비밀번호
char RX = 0;	//블루투스 프로토콜 단계
char rx_complete = 0;	//블루투스 수신이 완료 플래그

enum{close, open};	//도어락 상태


ISR(USART1_RX_vect)	
//주의할점!! 블루투스 한꺼번에 인식 못한 이유가 중간 과정 잘 가는지 확인하려고 과정별로 LCD출력 하느라 속도가 늦어져 인식을 못했다.
{
	unsigned char ch;	//수신저장변수
	static char old_index = 0;	//이전 비밀번호 자리
	static char new_index = 0;	//새 비밀번호 자리

	ch = UDR1; // 수신

	if(RX == 0 && ch == 'C') //C를 받았을때
	{
		RX = 1;
		for(int i = 0; i < 11; i++)
		{
			old_password[i] = 0;
		}
		for(int i = 0; i < 11; i++)
		{
			new_password[i] = 0;
		}
	}
	else if(RX == 1)
	{
		if(ch == 'E')	//E를 받았을때 아직 프로토콜이 완료 안된 상태라 초기화
		{
			rx_complete = 1;
			RX = 0;
		}
		else if(ch == 'R')	//비밀번호가 맞는지 확인하는 단계
		{
			RX = 2;
			old_index = 0;
		}
		else  //명령어가 아니면 비밀번호로 간주하여 입력값에 넣는다
		{
			old_password[old_index] = ch;
			old_index++;
		}
	}
	else if(RX == 2)	//단계 2일 경우
	{
		if(ch == 'E')	//E를 받았을때 블루투스 완료 플래그 온
		{
			rx_complete = 1;
			RX = 0;
		}
		else if(ch == 'R')	//R이 나올 단계 아니므로 초기화
		{
			RX = 0;
			new_index = 0;
		}
		else  //명령어가 아니면 새로운 비밀번호로 간주하여 입력값에 넣는다
		{
			new_password[new_index] = ch;
			new_index++;
		}
	}

	if(rx_complete == 1)	//블루투스 완료시 새로운 비밀번호 입력하고 eeprom에도 저장
	{
		if(strcmp(password, old_password) == 0)
		{
			for(int i = 0; i <= new_index; i++)
			{
				password[i] = new_password[i];
			}
			for(uint8_t i = 0; i < 11; i++)
			{
				eeprom_update_byte(EEPROM + i,password[i]);
			}
			Command(ALLCLR);
			LCD_String("PW :");
			LCD_String(password);
			Command(LINE2);
			new_index = 0;	//비밀번호 자리 초기화
			rx_complete = 0;	//신호분석 플래그 지움
		}
		else
		{
			RX = 0;
			rx_complete = 0;	//신호분석 플래그 지움
			new_index = 0;
			Command(ALLCLR);
			LCD_String("PW :");
			LCD_String(password);
			Command(LINE2);
		}

	}
}


ISR(TIMER0_OVF_vect)
{
	static char scan = 0;	//스캔하는 줄, 타이버 인터럽트 발생때마다 스캔줄이 바뀌어서 static으로 초기화 방지
	static char input_spot = 0;	//비밀번호 자리
	char Key=0;	//키패드 입력값 저장소
	DDRA=0xF0;         // 비트0,1,2,3 출력으로 지정
	PORTA = 0xFF;

	if(scan == 0)
	{
		PORTA &= ~0x10; // 1번째 줄 선택
		if((PINA & 0x01)==0)Key='1';
		if((PINA & 0x02)==0)Key='2';
		if((PINA & 0x04)==0)Key='3';
		PORTA = 0xFF; // 1번째 줄 해제
	}
	
	if(scan == 1)
	{
		PORTA &= ~0x20; // 2번째 줄 선택
		if((PINA & 0x01)==0)Key='4';
		else if((PINA & 0x02)==0)Key='5';
		else if((PINA & 0x04)==0)Key='6';
		PORTA=0xFF; // 2번째 줄 해제
	}
	
	if(scan == 2)
	{
		PORTA &= ~0x40; // 3번째 줄 선택
		if((PINA & 0x01)==0)Key='7';
		else if((PINA & 0x02)==0)Key='8';
		else if((PINA & 0x04)==0)Key='9';
		PORTA=0xFF; // 3번째 줄 해제
	}
	
	if(scan == 3)
	{
		PORTA &= ~0x80; // 4번째 줄 선택
		if((PINA & 0x01)==0)Key='0';
		else if((PINA & 0x02)==0)Key='Y';
		else if((PINA & 0x04)==0)Key='N';
		PORTA=0xFF; // 4번째 줄 해제
	}
	scan++;
	if(scan == 4) scan = 0;
	ch = Key;

	if(state == close)	//닫혀있는 상태
	{
		OCR1A = 2000;	//1ms 펄스, 서보모터 제어
		if(ch == 'Y')	//확인버튼 비밀번호 확인
		{
			_delay_ms(200);	//이중 입력 방지를 위한 딜레이 수정하고 싶은 부분
			Command(ALLCLR);
			input[input_spot] = '\0';
			if(strcmp(password, input) == 0)	//맞으면 열려있는 상태로 전환
			{
				state = open;
				input_spot = 0;
				LCD_String("OPEN");
				Command(LINE2);
			}
			else  //틀리면 초기화
			{
				input_spot = 0;
				LCD_String("PW :");
				LCD_String(password);
				Command(LINE2);
			}


		}
		else if(ch == 'N')	//취소버튼 초기화
		{
			Command(ALLCLR);
			LCD_String("PW :");
			LCD_String(password);
			Command(LINE2);
		}
		else if(ch != 0)	//앞에경우와 가만히 있는경우(ch = 0) 제외하면 비밀번호 입력
		{
			
			if(input_spot == 10)	//입력자리가 11번째일때 다시 1번째 자리로
			{
				Command(ALLCLR);
				LCD_String("PW :");
				LCD_String(password);
				Command(LINE2);
				input_spot = 0;	//입력자리 초기화
			}
			Data(ch);	//입력한거 화면표시
			input[input_spot] = ch;	//입력값 추가
			SendCommand01(0x0F, 0x01, 0x03);	//입력될때 mp3 스위치음
			input_spot++;	//입력자리 증가
			ch = 0;	//입력값 초기화
			_delay_ms(200);	//이중 입력 방지를 위한 딜레이 수정하고 싶은 부분
		}
	}

	else if(state == open)	//열려있는 상태
	{
		OCR1A = 4000;	//2ms 펄스, 서보모터 제어
		if(ch == 'Y')	//새로운 비밀번호 입력 완료
		{
			
			for(int i = 0; i <= input_spot; i++)
			{
				password[i] = new_password[i];	//새로운 비밀번호로 갱신
			}
			for(int i = 0; i < 11; i++)
			{
				new_password[i] = 0;	//새로운 비밀번호 받는 자리 초기화 마지막 Null값이 되게
			}
			for(uint8_t i = 0; i < 11; i++)
			{
				eeprom_update_byte(EEPROM + i,password[i]);	//새로운 비밀번호 eeprom으로 저장
			}

			Command(ALLCLR);	//화면 초기화
			state = close;	//새로운 비밀번호 입력후 다시 닫힌상태로
			
			input_spot = 0;	//입력자리 초기화
			LCD_String("PW :");	//닫힌상태 화면 표시
			LCD_String(password);
			Command(LINE2);
		}
		else if(ch == 'N')	//취소버튼
		{
			Command(ALLCLR);
			LCD_String("PW :");
			LCD_String(password);
			Command(LINE2);
			state = close;	//닫힌상태로
			input_spot = 0;	//입력자리 초기화
		}
		else if(ch != 0)
		{
			
			if(input_spot == 10)	//입력자리 초기화
			{
				Command(ALLCLR);
				LCD_String("PW :");
				LCD_String(password);
				Command(LINE2);
				input_spot = 0;
			}
			Data(ch);	//입력한거 화면표시
			new_password[input_spot] = ch;	//입력값 새로운 비밀번호에 추가
			SendCommand01(0x0F, 0x01, 0x03);	//입력될때 mp3 스위치 음
			input_spot++;	//입력자리 증가
			ch = 0;	//입력값 초기화
			_delay_ms(200);	//이중 입력 방지를 위한 딜레이 수정하고 싶은 부분
		}
	}
	
}



int main(void)
{
	char cmd;

	UART_Init();	//UART 초기화

	SendCommand(CMD_TF);	//mp3 준비
	_delay_ms(20);
	//SendCommand01(0x06, 0x00, 0x0F);	//mp3 효과음 줄이기 필요하다면
	//_delay_ms(200);
	
	DDRB = 0xFF;	//이걸 안써서 한참동안 뻘짓했다...
	TCCR0 = 0x07; // 일반모드, 1024분주
	TCCR1A = 0b10000010;	//비교매치에서 OC1A 출력 클리어 TOP에서 셋
	TCCR1B = 0b00011010;	//Mode 14(Fast PWM ICR1이 TOP을 저장하는 레지스로 사용되는 동작모드 Icn핀 차단), 8분주
	TCCR1C = 0x00;
	ICR1 = 40000;	//Mode 14에서는 Top값이 ICR이므로 65536중에서 40000까지만 올라간다
	OCR1A = 2000;	//처음 high 상태였다가 40000까지 올라가는 중 2000일때 클리어 되어 40000까지 갈때까지 low상태 그래서  8분주라 20ms 1ms PWM 생성
	TIMSK |= (1 << TOIE0); // TOIE0 = 1(오버플로우 인터럽트 인에이블)
	sei(); // 전역 인터럽트 인에이블 비트 I 셋

	

	for(uint8_t i = 0; i < 11; i++)
	{
		password[i] = eeprom_read_byte(EEPROM + i);
	}

	LCD_init();
	LCD_String("PW :"); // 첫번째라인에출력
	LCD_String(password);
	Command(LINE2);

	while(1);
}

// LCD 초기화
void LCD_init(void)
{
	DDRC = 0xFF; // 포트 C 출력 설정
	PORTC = 0x0;
	_delay_ms(15);
	Command(0x20);
	_delay_ms(5);
	Command(0x20);
	_delay_us(100);
	Command(0x20);
	Command(FUNCSET);
	Command(DISPON);
	Command(ALLCLR);
	Command(ENTMODE);
}

// 문자열 출력 함수
void LCD_String(char str[])
{
	char i=0;

	while(str[i] != 0) Data(str[i++]);
}

// 인스트럭션 쓰기 함수
void Command(u_char byte)
{
	Busy();
	// 인스트럭션 상위 4비트
	PORTC = 0x00; // RS=RW=0
	PORTC |= (byte & 0xF0); // 명령어 상위 4비트

	_delay_us(1);
	ENH; // E = 1

	_delay_us(1);
	ENL; // E = 0

	// 인스트럭션 하위 4비트
	PORTC = 0x00; // RS=RW=0
	PORTC |= (byte << 4); // 명령어 하위 4비트

	_delay_us(1);
	ENH; // E = 1

	_delay_us(1);
	ENL; // E = 0
}

void Data(u_char byte)
{
	Busy();
	// 데이터 상위 4비트
	PORTC = 0x01; // RS=1, RW=0
	PORTC |= (byte & 0xF0); // 데이터 상위 4비트
	_delay_us(1);
	ENH; // E = 1
	_delay_us(1);
	ENL; // E = 0

	// 데이터 하위 4비트
	PORTC = 0x01; // RS=1, RW=0
	PORTC |= (byte << 4); // 데이터 하위 4비트
	_delay_us(1);
	ENH; // E = 1
	_delay_us(1);
	ENL; // E = 0
}
// Busy Flag Check -> 일반적인 BF를 체크하는 것이 아니라
// 일정한 시간 지연을 이용한다.
void Busy(void)
{
	_delay_ms(2);
}
