#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

//mp3 조정 명령어들
#define CMD_NEXT	0x01
#define CMD_PRE		0x02
#define CMD_PLAY	0x0d
#define CMD_PAUSE	0x0E
#define CMD_STOP	0x16
#define CMD_TF		0x09

char cmd_list[10] = {CMD_PLAY, CMD_STOP, CMD_PAUSE, CMD_NEXT, CMD_PRE,};


//   0 1 2 3 4  5  6  7
unsigned char C[8]={1,2,4,8,16,32,64,128};
unsigned char R[8]={254,253,251,247,239,223,191,127};

int cnt1;		// 일반모드타이머로 초를 세기 위해
int cnt2;		// 지옥모드 타이머


int ending;		// 게임 끝남 변수

//초기값 3 이후에 다른 수를 안 넣으면 이상한 led가 켜져서
//먹이위치값을 그대로 넣어줬음
int i[15] = {3,1,4,2,5,0,5,1,3,6,2,4,0,6,2};
int j[15] = {3,2,6,1,4,6,0,2,7,3,4,1,5,7,3};

int k,n;		// 조건문 사용시 쓸 변수들

unsigned char state;		// 상태

unsigned char blue;

int hell_mode;	//먹이를 20개 이상 먹었을 경우 달성되는 악마난이도

int age; //먹이 먹은 횟수

unsigned char age1[15]={6,1,4,2,5,0,5,1,3,6,2,4,0,6,2};		//x축 먹이 위치
unsigned char age2[15]={4,2,6,1,4,6,0,2,7,3,4,1,5,7,3};		//y축 먹이 위치

void way(void);		//자동으로 지렁이가 움직이게 하기
void body_atk(void);		// 지렁이 머리가 몸통에 부딪칠경우

void game_over(void);		//게임 끝나는 함수

void UART1_Putch(char); // 1바이트 송신

// mp3 USART 함수
void SendCommand(unsigned char cmd)
{
	unsigned int checksum = 0;
	char temp[20];

	//USART통신으로 DF player에 송신
	UART1_Putch(0x7E);		//START command
	UART1_Putch(0xFF);		//Version Information
	UART1_Putch(0x06);		//data length
	UART1_Putch(cmd);
	UART1_Putch(0x00);
	UART1_Putch(0x00);
	
	if(cmd != CMD_TF){
		UART1_Putch(0x00);
		checksum = 0 - (0xFF + 0x06 + cmd);
	}
	else{
		UART1_Putch(0x02);
		checksum = 0 - (0xFF + 0x06 + cmd + 0x02);
	}

	UART1_Putch((char)(checksum >> 8));
	UART1_Putch(checksum&0xFF);
	UART1_Putch(0xEF);
	

	/*
	//union 이용방법
	union uCheckSum checksum;
	if(cmd != CMD_TF){
		UART1_Putch(0x00);
		checksum.checksum = 0 - (0xFF + 0x06 + cmd);
	}
	else{
		UART1_Putch(0x02);
		checksum.checksum = 0 - (0xFF + 0x06 + cmd + 0x02);
	} 

	UART1_Putch(checksum.arrayChecksum[1]);
	UART1_Putch(checksum.arrayChecksum[0]);
	UART1_Putch(0xEF);
	*/
}

//원하는 음악 틀때 쓰는 함수
void SendCommand2(unsigned char cmd, unsigned char param1, unsigned char param2)
{
	unsigned int checksum = 0;
	char temp[20];

	UART1_Putch(0x7E);
	UART1_Putch(0xFF);
	UART1_Putch(0x06);
	UART1_Putch(cmd);
	UART1_Putch(0x00);
	UART1_Putch(param1);
	UART1_Putch(param2);
	
	checksum = 0 - (0xFF + 0x06 + cmd + param1 + param2);

	UART1_Putch((char)(checksum >> 8));
	UART1_Putch(checksum&0xFF);
	UART1_Putch(0xEF);
	

	/*
	//union 이용방법
	union uCheckSum checksum;
	if(cmd != CMD_TF){
		UART1_Putch(0x00);
		checksum.checksum = 0 - (0xFF + 0x06 + cmd);
	}
	else{
		UART1_Putch(0x02);
		checksum.checksum = 0 - (0xFF + 0x06 + cmd + 0x02);
	} 

	UART1_Putch(checksum.arrayChecksum[1]);
	UART1_Putch(checksum.arrayChecksum[0]);
	UART1_Putch(0xEF);
	*/
}

int main(void)
{
	// I/O포트 초기화
	DDRC = 0xFF;		// 출력포트 C 배열 관리 x축
	DDRB = 0xFF;		// 출력포트 R 배열 관리 y축

	PORTC = 0x00;
	PORTB = 0xFF;
	
	//인터럽트 초기화
	EICRB = (2<<ISC40) | (2<<ISC50) | (2<<ISC60) | (2<<ISC70);
	//INT 4~7 하강 엣지사용

	EIMSK = (1<<INT4) | (1<<INT5) | (1<<INT6) | (1<<INT7);
	//INT 4~7 인터럽트 활성화

	//USART 통신
	//rx,tx1번사용 mp3용
	UCSR1B = 0x08;		//TXEN1=1 송신단자로 동작
	UCSR1C = 0x06;		//UCSZ11=1, USCZ10=1 8비트 전송
	UBRR1L = 103;
	// X-TAL = 16MHz 일때, BAUD = 9600
	
	//rx,tx0번사용 휴대폰과 블루투스 연경용
	UCSR0A=0x0;
	//UCSR0B=0b10011000;		//RXCIE0:수신 완료 인터럽트 인에이블
	// 송수신 인에이블 TXEN0 = 1, RXEN0=1
	UCSR0B = 0b00010000; // 수신 인에이블 RXEN0=1

	UCSR0C=0b00000110;	//비동기 데이터 8비트 모드
	UBRR0H=0;	// X-TAL = 16MHz 일때, BAUD = 9600 
	UBRR0L=103;

	//타이머/카운터 초기화
	TIMSK = 0x01;		//OCIE0 타이머카운터0 출력 비교매치 인터럽트 인에이블비트
	TCCR0 = 0x07;		//CS02=1,CS01=1,CS00=1 1024분주비를 씀
	TCNT0 = 0x00;		//TCNT0를 0x00으로 초기화

	sei();
	
	//_delay_ms(200);

	SendCommand(CMD_TF);
	_delay_ms(200);
	SendCommand2(0x03,0x00, 0x01);		//001 음악을 재생시키겠다.
	_delay_ms(200);
	
	while (1)
	{
		state = UDR0;
		//i[0] &&j[0]이 뱀의 머리 역할을 한다.
		for(n=0;n<=age;n++)
		{
			PORTC = C[i[n]];
			PORTB = R[j[n]];
			_delay_ms(1);
		}
		
		
		//뱀의 머리가 먹이와 만나면
		if((C[i[0]]==C[age1[age]]) && (R[j[0]]==R[age2[age]]))
		{
			if(++age == 15) 
			{
				age=0;		//먹이를 15개 이상먹으면 초기화
				state=0;	//난이도 업글 시작하면서 멈춤상태
				SendCommand2(0x03,0x00, 0x03);	//지옥모드음악재생
				hell_mode=1;	//헬모드실행
				_delay_ms(500);		//바로바뀌면 사용자가 어려워하으로 딜레이를줌
			}		
		}

		// 먹이 위치
		PORTC = C[age1[age]];
		PORTB = R[age2[age]];

		_delay_ms(1);

		way(); //자동으로 지렁이가 움직이게 하기

		body_atk();		// 지렁이 머리가 몸통에 부딪칠경우

		game_over(); //게임 끝나는 함수
		

	}
}
/*
//수신 인터럽트
ISR(USART0_RX_vect)
{
	state = UDR0;
	//blue = UDR0;
	//if(blue =='A') state=6;
	//if(blue =='B') state=7;
	//if(blue =='C') state=4;
	//if(blue =='D') state=5;



}
*/



// 한 바이트 송신 mp3에 송신용
void UART1_Putch(char data)
{
	while((UCSR1A & 0x20) == 0x0);//UDRE0 = 1이 될 때까지 대기
	UDR1 = data; // 데이터 전송
}


ISR(INT4_vect)		// 오른쪽버튼
{
	if(state !=5) state=4;
	
}
ISR(INT5_vect)		// 왼쪽버튼
{
	if(state !=4)state=5;
		
}
ISR(INT6_vect)		// 위쪽 버튼
{
	if(state !=7) state=6;
	
}
ISR(INT7_vect)		// 아래 버튼
{
	if(state !=6) state=7;
	
}

// 타이머/카운터0 오버플로우 인터럽트 처리, 주기 = 1/16 * 1024 * 256 = 16.384ms
ISR(TIMER0_OVF_vect)
{
	TCNT0= 0x0;
	if(hell_mode==0) cnt1++;
	else cnt2++;
}

void way(void)		//자동으로 지렁이가 움직임
{
	if((cnt1 >= 15) || (cnt2 >=10))		//cnt가 61이 1초
	{
		cnt1=0;
		cnt2=0;
		if(state==4)		//오른쪽버튼 입력시
		{
			for(k=age;k>0;k--)		//먹이 먹은 양에 따라 뒤에 몸통들이 표시됨
			{
				i[k] = i[k-1];
				j[k] = j[k-1];
			}
			if(++(i[0])>=8) ending=1;		//벽에 박으면 죽음
		}
		else if (state==5)		//왼쪽버튼 입력시
		{
			for(k=age;k>0;k--)
			{
				i[k] = i[k-1];
				j[k] = j[k-1];
			}
			
			if(--(i[0])<0) ending=1;
		}
		else if (state==6)		//위쪽 버튼 입력시
		{
			for(k=age;k>0;k--)
			{
				i[k] = i[k-1];
				j[k] = j[k-1];
			}
			
			if(--(j[0])<0) ending=1;
		}
		else if (state==7)		//아래 버튼 입력시
		{
			for(k=age;k>0;k--)
			{
				i[k] = i[k-1];
				j[k] = j[k-1];
			}
			
			if(++(j[0])>=8) ending=1;
		}
	}
}

void body_atk(void)		// 지렁이 머리가 몸통에 부딪칠경우
{
	for(k=1; k<age; k++)
	{
		if((i[0]==i[k]) && (j[0]==j[k]))  ending=1;
	}
}

void game_over(void)		//게임 끝나는 함수
{
	if(ending==1)
	{	
		//게임이 끝났을 경우 mp3에 엔딩음악 재생
		SendCommand2(0x03,0x00, 0x02); 
		while(1)		// 죽었을 경우 매트릭스에 번쩍번쩍
		{
			PORTC = 0xFF;
			PORTB = 0x00;
			_delay_ms(500);
			PORTC = 0x00;
			PORTB = 0xFF;
			_delay_ms(500);
		}
	}
}