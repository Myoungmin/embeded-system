#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define CMD_NEXT	0x01
#define CMD_PRE		0x02
#define CMD_PLAY	0x0d
#define CMD_PAUSE	0x0E
#define CMD_STOP	0x16
#define CMD_TF		0x09

char cmd_list[10] = {CMD_PLAY, CMD_STOP, CMD_PAUSE, CMD_NEXT, CMD_PRE,};

union uCheckSum
{
	unsigned int checksum;
	unsigned char arrayChecksum[2];
};


void UART_Init(void)
{
	UCSR0B = 0x18;
	UCSR0C = 0x06;
	UBRR0L = 103;

	UCSR1B = 0x08;
	UCSR1C = 0x06;
	UBRR1L = 103;
}

void UART0_Putch(char ch)
{
	while(!(UCSR0A & 0x20));

	UDR0 = ch;
}

char UART0_Getch(void)
{
	while(!(UCSR0A & 0x80));

	return UDR0;
}

void UART0_Puts(char str[])
{
	int i=0;

	while(str[i] != 0)
	UART0_Putch(str[i++]);
}

void UART1_Putch(char ch)
{
	while(!(UCSR1A & 0x20));

	UDR1 = ch;
}

void SendCommand(unsigned char cmd)
{
	unsigned int checksum = 0;
	char temp[20];

	UART1_Putch(0x7E);
	UART1_Putch(0xFF);
	UART1_Putch(0x06);
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

void DisplayMenu(void)
{
	UART0_Puts("\r\n===========================");
	UART0_Puts("\r\n= 1 : Play Music          =");
	UART0_Puts("\r\n= 2 : Stop Music          =");
	UART0_Puts("\r\n= 3 : Pause Music         =");
	UART0_Puts("\r\n= 4 : Next Music          =");
	UART0_Puts("\r\n= 5 : Previous Music      =");
	UART0_Puts("\r\n= 6 : volume up      =");
	UART0_Puts("\r\n= 7 : volume down       =");
	UART0_Puts("\r\n===========================");
	UART0_Puts("\r\n\r\nCommand > ");
}

int main(void)
{
	char cmd;

	UART_Init();

	_delay_ms(200);

	SendCommand(CMD_TF);
	
	while(1){
		DisplayMenu();
		cmd = UART0_Getch();
		SendCommand(cmd_list[cmd - '0' - 1]);
		//PORTD = ~PORTD;
	}
}

