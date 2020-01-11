#include <avr/io.h>
#include <util/delay.h>


int main(void)
{
	unsigned char phase = 0;	//단계변수
	unsigned char state1 = 0;	//이전 스위치 값
	unsigned char state2 = 0;
	unsigned char state3 = 0;
	unsigned char state4 = 0;

	unsigned char number1 = 0;	//입력한 번호
	unsigned char number2 = 0;
	unsigned char number3 = 0;
	unsigned char number4 = 0;
	unsigned char save1 = 1;	//비밀번호
	unsigned char save2 = 1;
	unsigned char save3 = 1;
	unsigned char save4 = 1;
	unsigned char segment3[11] = {
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
		0b10000000	//dot
	};


	DDRD = 0xFF;	//세그먼트 숫자선택
	DDRG = 0xFF;	//세그먼트 전원
	DDRE = 0x0;	//스위치 입력

	PORTD = 0xFF;
	PORTG = 0x0F;

	while(1)
	{
		while(phase == 0)	//첫번째 숫자 선택단계
		{
			PORTG = 0b00001000;	//첫번째 세그먼트 전원
			PORTD = ~segment3[number1];	//숫자표시
			_delay_ms(5);

			PORTD = ~segment3[10];	//첫번째 점 표시
			_delay_ms(5);

			PORTG = 0b00000100;
			PORTD = ~segment3[number2];
			_delay_ms(5);

			PORTG = 0b00000010;
			PORTD = ~segment3[number3];
			_delay_ms(5);

			PORTG = 0b00000001;
			PORTD = ~segment3[number4];
			_delay_ms(5);


			if(((PINE & 0xF0) == 0b01110000) && (state1 == 0))	//1번 스위치 눌렸을때
			{
				_delay_ms(10);
				state1 = 1;
				number1++;	//첫번째 숫자증가
				if(number1 == 10) number1 = 0; //10일때 다시 0으로
				if(number2 == 10) number2 = 0;
				if(number3 == 10) number3 = 0;
				if(number4 == 10) number4 = 0;
			}
			if(((PINE & 0xF0) == 0b10110000) && (state2 == 0))	//2번 스위치 눌렸을때
			{
				_delay_ms(10);
				state2 = 1;
				phase = 1;	//다음단계로 이동
			}
			if(((PINE & 0xF0) == 0b11010000) && (state3 == 0))	//3번째 스위치 눌렸을때
			{
				_delay_ms(10);
				state3 = 1;
				phase = 4;	//비밀번호 확인단계로
			}
			if((PINE & 0xF0) == 0xF0)
			{
				state1 = 0;
				state2 = 0;
				state3 = 0;
				state4 = 0;
			}
		}
		while(phase == 1)	//두번째 숫자단계
		{
			PORTG = 0b00001000;
			PORTD = ~segment3[number1];
			_delay_ms(5);

			PORTG = 0b00000100;
			PORTD = ~segment3[number2];
			_delay_ms(5);

			PORTD = ~segment3[10];
			_delay_ms(5);

			PORTG = 0b00000010;
			PORTD = ~segment3[number3];
			_delay_ms(5);

			PORTG = 0b00000001;
			PORTD = ~segment3[number4];
			_delay_ms(5);


			if(((PINE & 0xF0) == 0b01110000) && (state1 == 0))	//1번 스위치 눌렸을때
			{
				_delay_ms(10);
				state1 = 1;
				number2++;	//두번째 숫자증가
				if(number1 == 10) number1 = 0; //10일때 다시 0으로
				if(number2 == 10) number2 = 0;
				if(number3 == 10) number3 = 0;
				if(number4 == 10) number4 = 0;
			}
			if(((PINE & 0xF0) == 0b10110000) && (state2 == 0))	//2번 스위치 눌렸을때
			{
				_delay_ms(10);
				state2 = 1;
				phase = 2;	//다음단계로 이동
			}
			if(((PINE & 0xF0) == 0b11010000) && (state3 == 0))	//3번째 스위치 눌렸을때
			{
				_delay_ms(10);
				state3 = 1;
				phase = 4;	//비밀번호 확인단계로
			}
			if((PINE & 0xF0) == 0xF0)
			{
				state1 = 0;
				state2 = 0;
				state3 = 0;
				state4 = 0;
			}
		}
		while(phase == 2)	//세번째 숫자 단계
		{
			PORTG = 0b00001000;
			PORTD = ~segment3[number1];
			_delay_ms(5);

			PORTG = 0b00000100;
			PORTD = ~segment3[number2];
			_delay_ms(5);

			PORTG = 0b00000010;
			PORTD = ~segment3[number3];
			_delay_ms(5);

			PORTD = ~segment3[10];
			_delay_ms(5);

			PORTG = 0b00000001;
			PORTD = ~segment3[number4];
			_delay_ms(5);

			if(((PINE & 0xF0) == 0b01110000) && (state1 == 0))	//1번 스위치 눌렸을때
			{
				_delay_ms(10);
				state1 = 1;
				number3++;	//세번째 숫자증가
				if(number1 == 10) number1 = 0; //10일때 다시 0으로
				if(number2 == 10) number2 = 0;
				if(number3 == 10) number3 = 0;
				if(number4 == 10) number4 = 0;
			}
			if(((PINE & 0xF0) == 0b10110000) && (state2 == 0))	//2번 스위치 눌렸을때
			{
				_delay_ms(10);
				state2 = 1;
				phase = 3;	//다음단계로 이동
			}
			if(((PINE & 0xF0) == 0b11010000) && (state3 == 0))	//3번째 스위치 눌렸을때
			{
				_delay_ms(10);
				state3 = 1;
				phase = 4;	//비밀번호 확인단계로
			}
			if((PINE & 0xF0) == 0xF0)
			{
				state1 = 0;
				state2 = 0;
				state3 = 0;
				state4 = 0;
			}
		}
		while(phase == 3)	//4번째 숫자 단계
		{
			PORTG = 0b00001000;
			PORTD = ~segment3[number1];
			_delay_ms(5);

			PORTG = 0b00000100;
			PORTD = ~segment3[number2];
			_delay_ms(5);

			PORTG = 0b00000010;
			PORTD = ~segment3[number3];
			_delay_ms(5);

			PORTG = 0b00000001;
			PORTD = ~segment3[number4];
			_delay_ms(5);

			PORTD = ~segment3[10];
			_delay_ms(5);


			if(((PINE & 0xF0) == 0b01110000) && (state1 == 0))	//1번 스위치 눌렸을때
			{
				_delay_ms(10);
				state1 = 1;
				number4++;	//네번째 숫자증가
				if(number1 == 10) number1 = 0; //10일때 다시 0으로
				if(number2 == 10) number2 = 0;
				if(number3 == 10) number3 = 0;
				if(number4 == 10) number4 = 0;
			}
			if(((PINE & 0xF0) == 0b10110000) && (state2 == 0))	//2번 스위치 눌렸을때
			{
				_delay_ms(10);
				state2 = 1;
				phase = 0;	//처음단계로 이동
			}
			if(((PINE & 0xF0) == 0b11010000) && (state3 == 0))	//3번째 스위치 눌렸을때
			{
				_delay_ms(10);
				state3 = 1;
				phase = 4;	//비밀번호 확인단계로
			}
			if((PINE & 0xF0) == 0xF0)
			{
				state1 = 0;
				state2 = 0;
				state3 = 0;
				state4 = 0;
			}
		}
		while(phase == 4)	//비밀번호 확인단계
		{
			if((number1 == save1) && (number2 == save2) && (number3 == save3) && (number4 == save4)) //비밀번호 일치할때
			{
				PORTG = 0b00001000;
				PORTD = ~(0b00111111);  //OPEN 표시
				_delay_ms(5);
				PORTG = 0b00000100;
				PORTD = ~(0b01110011);
				_delay_ms(5);
				PORTG = 0b00000010;
				PORTD = ~(0b01111001);
				_delay_ms(5);
				PORTG = 0b00000001;
				PORTD = ~(0b00110111);
				_delay_ms(5);
				if(((PINE & 0xF0) == 0b11100000) && (state4 == 0))	//비밀번호 일치하는 상황에서 4번째 스위치 눌렀을때
				{
					_delay_ms(10);
					state4 = 1;
					phase = 5;	//비밀번호 변경 단계로 이동
				}
				else if((PINE & 0xF0) == 0xF0)
				{
					state4 = 0;
				}

			}
			else  //비밀번호 일치하지 않을 때
			{
				PORTG = 0b00001000;
				PORTD = ~(0b00110111);	//NONO 표시
				_delay_ms(5);
				PORTG = 0b00000100;
				PORTD = ~(0b00111111);
				_delay_ms(5);
				PORTG = 0b00000010;
				PORTD = ~(0b00110111);
				_delay_ms(5);
				PORTG = 0b00000001;
				PORTD = ~(0b00111111);
				_delay_ms(5);

			}

			if(((PINE & 0xF0) == 0b11010000) && (state3 == 0))	//3번째 스위치 눌렀을 때 다시 비밀번호 입력 단계로
			{
				_delay_ms(10);
				state3 = 1;
				phase = 0;
			}
			else if((PINE & 0xF0) == 0xF0)
			{
				state3 = 0;
			}
		}
		while(phase == 5)	//비밀번호 변경 단계 첫번째 숫자 수정
		{
			PORTG = 0b00001000;
			PORTD = ~segment3[save1];	//비밀번호 숫자 표시
			_delay_ms(5);

			PORTG = 0b00000100;
			PORTD = ~segment3[save2];
			_delay_ms(5);

			PORTD = ~segment3[10];
			_delay_ms(5);

			PORTG = 0b00000010;
			PORTD = ~segment3[save3];
			_delay_ms(5);

			PORTD = ~segment3[10];
			_delay_ms(5);

			PORTG = 0b00000001;
			PORTD = ~segment3[save4];
			_delay_ms(5);

			PORTD = ~segment3[10];
			_delay_ms(5);




			if(((PINE & 0xF0) == 0b01110000) && (state1 == 0))	//첫번째 스위치 누를 때
			{
				_delay_ms(10);
				state1 = 1;
				save1++;	//첫번째 비밀번호 증가
				if(save1 == 10) save1 = 0;	//10일때 다시 0으로
				if(save2 == 10) save2 = 0;
				if(save3 == 10) save3 = 0;
				if(save4 == 10) save4 = 0;
			}
			if(((PINE & 0xF0) == 0b10110000) && (state2 == 0))	//두번째 스위치 누를 때
			{
				_delay_ms(10);
				state2 = 1;
				phase = 6;	//다음 비밀번호로 이동
			}
			if(((PINE & 0xF0) == 0b11010000) && (state3 == 0)) //3번째 스위치 누를 때
			{
				_delay_ms(10);
				state3 = 1;
				phase = 9;	//비밀번호 변경 완료 단계로 이동
			}
			if((PINE & 0xF0) == 0xF0)
			{
				state1 = 0;
				state2 = 0;
				state3 = 0;
				state4 = 0;
			}
		}
		while(phase == 6)	//비밀번호 2번째 숫자 변경
		{
			PORTG = 0b00001000;
			PORTD = ~segment3[save1];
			_delay_ms(5);

			PORTD = ~segment3[10];
			_delay_ms(5);

			PORTG = 0b00000100;
			PORTD = ~segment3[save2];
			_delay_ms(5);

			PORTG = 0b00000010;
			PORTD = ~segment3[save3];
			_delay_ms(5);

			PORTD = ~segment3[10];
			_delay_ms(5);

			PORTG = 0b00000001;
			PORTD = ~segment3[save4];
			_delay_ms(5);

			PORTD = ~segment3[10];
			_delay_ms(5);

			if(((PINE & 0xF0) == 0b01110000) && (state1 == 0))	//첫번째 스위치 누를 때
			{
				_delay_ms(10);
				state1 = 1;
				save2++;	//두번째 비밀번호 증가
				if(save1 == 10) save1 = 0;	//10일때 다시 0으로
				if(save2 == 10) save2 = 0;
				if(save3 == 10) save3 = 0;
				if(save4 == 10) save4 = 0;
			}
			if(((PINE & 0xF0) == 0b10110000) && (state2 == 0))	//두번째 스위치 누를 때
			{
				_delay_ms(10);
				state2 = 1;
				phase = 7;	//다음 비밀번호로 이동
			}
			if(((PINE & 0xF0) == 0b11010000) && (state3 == 0)) //3번째 스위치 누를 때
			{
				_delay_ms(10);
				state3 = 1;
				phase = 9;	//비밀번호 변경 완료 단계로 이동
			}
			if((PINE & 0xF0) == 0xF0)
			{
				state1 = 0;
				state2 = 0;
				state3 = 0;
				state4 = 0;
			}
		}
		while(phase == 7)	//비밀번호 3번째 숫자 변경
		{
			PORTG = 0b00001000;
			PORTD = ~segment3[save1];
			_delay_ms(5);

			PORTD = ~segment3[10];
			_delay_ms(5);

			PORTG = 0b00000100;
			PORTD = ~segment3[save2];
			_delay_ms(5);

			PORTD = ~segment3[10];
			_delay_ms(5);

			PORTG = 0b00000010;
			PORTD = ~segment3[save3];
			_delay_ms(5);

			PORTG = 0b00000001;
			PORTD = ~segment3[save4];
			_delay_ms(5);

			PORTD = ~segment3[10];
			_delay_ms(5);

			if(((PINE & 0xF0) == 0b01110000) && (state1 == 0))	//첫번째 스위치 누를 때
			{
				_delay_ms(10);
				state1 = 1;
				save3++;	//세번째 비밀번호 증가
				if(save1 == 10) save1 = 0;	//10일때 다시 0으로
				if(save2 == 10) save2 = 0;
				if(save3 == 10) save3 = 0;
				if(save4 == 10) save4 = 0;
			}
			if(((PINE & 0xF0) == 0b10110000) && (state2 == 0))	//두번째 스위치 누를 때
			{
				_delay_ms(10);
				state2 = 1;
				phase = 8;	//다음 비밀번호로 이동
			}
			if(((PINE & 0xF0) == 0b11010000) && (state3 == 0)) //3번째 스위치 누를 때
			{
				_delay_ms(10);
				state3 = 1;
				phase = 9;	//비밀번호 변경 완료 단계로 이동
			}
			if((PINE & 0xF0) == 0xF0)
			{
				state1 = 0;
				state2 = 0;
				state3 = 0;
				state4 = 0;
			}
		}
		while(phase == 8)	//비밀번호 4번째 숫자 변경
		{
			PORTG = 0b00001000;
			PORTD = ~segment3[save1];
			_delay_ms(5);

			PORTD = ~segment3[10];
			_delay_ms(5);

			PORTG = 0b00000100;
			PORTD = ~segment3[save2];
			_delay_ms(5);

			PORTD = ~segment3[10];
			_delay_ms(5);

			PORTG = 0b00000010;
			PORTD = ~segment3[save3];
			_delay_ms(5);

			PORTD = ~segment3[10];
			_delay_ms(5);

			PORTG = 0b00000001;
			PORTD = ~segment3[save4];
			_delay_ms(5);

			if(((PINE & 0xF0) == 0b01110000) && (state1 == 0))	//첫번째 스위치 누를 때
			{
				_delay_ms(10);
				state1 = 1;
				save4++;	//네번째 비밀번호 증가
				if(save1 == 10) save1 = 0;	//10일때 다시 0으로
				if(save2 == 10) save2 = 0;
				if(save3 == 10) save3 = 0;
				if(save4 == 10) save4 = 0;
			}
			if(((PINE & 0xF0) == 0b10110000) && (state2 == 0))	//두번째 스위치 누를 때
			{
				_delay_ms(10);
				state2 = 1;
				phase = 5;	//처음 비밀번호로 이동
			}
			if(((PINE & 0xF0) == 0b11010000) && (state3 == 0)) //3번째 스위치 누를 때
			{
				_delay_ms(10);
				state3 = 1;
				phase = 9;	//비밀번호 변경 완료 단계로 이동
			}
			if((PINE & 0xF0) == 0xF0)
			{
				state1 = 0;
				state2 = 0;
				state3 = 0;
				state4 = 0;
			}
		}
		while(phase == 9)	//비밀번호 변경 완료 단계
		{
			PORTG = 0b00001000;
			PORTD = ~(0b00111001);	//네모 표시 출력
			_delay_ms(5);
			PORTG = 0b00000100;
			PORTD = ~(0b00001001);
			_delay_ms(5);
			PORTG = 0b00000010;
			PORTD = ~(0b00001001);
			_delay_ms(5);
			PORTG = 0b00000001;
			PORTD = ~(0b00001111);
			_delay_ms(5);


			if(((PINE & 0xF0) == 0b11010000) && (state3 == 0))	//3번째 스위치 누를 때
			{
				_delay_ms(10);
				state3 = 1;
				phase = 0;	// 다시 비밀번호 입력 단계로 이동
			}
			else if((PINE & 0xF0) == 0xF0)
			{
				state3 = 0;
			}
		}
	}
}














