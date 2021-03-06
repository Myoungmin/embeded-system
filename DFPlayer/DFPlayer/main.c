#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>


#define CMD_NEXT	0x01
#define CMD_PRE		0x02
#define CMD_PLAY	0x0d
#define CMD_PAUSE	0x0E
#define CMD_STOP	0x16
#define CMD_TF		0x09

char cmd_list[10] = {CMD_PLAY, CMD_STOP, CMD_PAUSE, CMD_NEXT, CMD_PRE,};


void UART_Init(void)
{
	//UCSR0B = 0x18;
	//UCSR0C = 0x06;
	//UBRR0L = 103;

	UCSR1B = 0x08;
	UCSR1C = 0x06;
	UBRR1L = 103;
}

void UART0_Putch(char ch)
{
	while((UCSR1A & 0x20) == 0x0);

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

void SendCommand01()
{
	unsigned int checksum = 0;
	char temp[20];

	UART0_Putch(0x7E);
	UART0_Putch(0xFF);
	UART0_Putch(0x06);	
	UART0_Putch(0x03);
	UART0_Putch(0x00);
	UART0_Putch(0x00);
	UART0_Putch(0x01);
	
	checksum = 0 - (0xFF + 0x06 + 0x03 + 0x00 + 0x01);

	UART0_Putch((char)(checksum >> 8));
	UART0_Putch(checksum&0xFF);

	UART0_Putch(0xEF);
}


int main(void)
{
    char cmd;

    UART_Init();

    
    
    SendCommand(CMD_TF);
    _delay_ms(200);
    SendCommand01();
	_delay_ms(200);
	while(1);
    
}

