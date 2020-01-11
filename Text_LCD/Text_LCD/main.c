#include <avr/io.h>
#include <util/delay.h>

typedef unsigned char u_char;

#define FUNCSET 0x28 // Function Set
#define ENTMODE 0x06 // Entry Mode Set
#define ALLCLR 0x01 // All Clear
#define DISPON 0x0c // Display On
#define LINE2 0xC0 // 2nd Line Move
#define ENH  PORTC |= 0x04
#define ENL  PORTC &= 0xFB

void LCD_init(void); // LCD 초기화
void LCD_String(char []); // 문자열 출력
void Busy(void); // 2ms 딜레이 함수
void Command(u_char); // 명령어 전송
void Data(u_char); // 데이터 전송


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




// 문자열 출력 함수
void LCD_String(char str[])
{
	char i=0;

	while(str[i] != 0) Data(str[i++]);
}



//데이터 쓰기 함수
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





int main(void)
{
	LCD_init();
	LCD_String("Hello!! World"); // 첫번째라인에출력
	Command(LINE2);
	LCD_String("Atmel ATmega128"); // 두번째라인에출력
	while(1);
}

