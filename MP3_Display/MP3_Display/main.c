#include <avr/io.h>
#include "u8g2.h"
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>

#define DISPLAY_CLK_DIR DDRD
#define DISPLAY_CLK_PORT PORTD
#define DISPLAY_CLK_PIN 0

#define DISPLAY_DATA_DIR DDRD
#define DISPLAY_DATA_PORT PORTD
#define DISPLAY_DATA_PIN 1

#define P_CPU_NS (1000000000UL / F_CPU)

u8g2_t u8g2;

uint8_t u8x8_avr_delay(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
	uint8_t cycles;

	switch(msg)
	{
		case U8X8_MSG_DELAY_NANO:     // delay arg_int * 1 nano second
			// At 20Mhz, each cycle is 50ns, the call itself is slower.
			break;
		case U8X8_MSG_DELAY_100NANO:    // delay arg_int * 100 nano seconds
			// Approximate best case values...
#define CALL_CYCLES 26UL
#define CALC_CYCLES 4UL
#define RETURN_CYCLES 4UL
#define CYCLES_PER_LOOP 4UL

			cycles = (100UL * arg_int) / (P_CPU_NS * CYCLES_PER_LOOP);

			if(cycles > CALL_CYCLES + RETURN_CYCLES + CALC_CYCLES) 
				break;

			__asm__ __volatile__ (
			"1: sbiw %0,1" "\n\t" // 2 cycles
			"brne 1b" : "=w" (cycles) : "0" (cycles) // 2 cycles
			);
			break;
		case U8X8_MSG_DELAY_10MICRO:    // delay arg_int * 10 micro seconds
			for(int i=0 ; i < arg_int ; i++)
				_delay_us(10);
			break;
		case U8X8_MSG_DELAY_MILLI:      // delay arg_int * 1 milli second
			for(int i=0 ; i < arg_int ; i++)
				_delay_ms(1);
			break;
		default:
			return 0;
	}
	return 1;
}


uint8_t u8x8_avr_gpio_and_delay(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
	// Re-use library for delays

	switch(msg)
	{
		case U8X8_MSG_GPIO_AND_DELAY_INIT:  // called once during init phase of u8g2/u8x8
			DISPLAY_CLK_DIR |= 1<<DISPLAY_CLK_PIN;
			DISPLAY_DATA_DIR |= 1<<DISPLAY_DATA_PIN;
			break;              // can be used to setup pins
		case U8X8_MSG_GPIO_I2C_CLOCK:        // Clock pin: Output level in arg_int
			if(arg_int)
				DISPLAY_CLK_PORT |= (1<<DISPLAY_CLK_PIN);
			else
				DISPLAY_CLK_PORT &= ~(1<<DISPLAY_CLK_PIN);
			break;
		case U8X8_MSG_GPIO_I2C_DATA:        // Data pin: Output level in arg_int
			if(arg_int)
				DISPLAY_DATA_PORT |= (1<<DISPLAY_DATA_PIN);
			else
				DISPLAY_DATA_PORT &= ~(1<<DISPLAY_DATA_PIN);
			break;
		
		default:
			if (u8x8_avr_delay(u8x8, msg, arg_int, arg_ptr))	// check for any delay msgs
				return 1;
			u8x8_SetGPIOResult(u8x8, 1);      // default return value
			break;
	}
	return 1;
}





#define CMD_NEXT  0x01
#define CMD_PRE   0x02
#define CMD_PLAY  0x0d
#define CMD_PAUSE 0x0E
#define CMD_STOP  0x16
#define CMD_TF    0x09
#define CMD_V_UP    0x04
#define CMD_V_DOWN    0x05

char cmd_list[10] = {CMD_PLAY, CMD_STOP, CMD_PAUSE, CMD_NEXT, CMD_PRE, CMD_V_UP, CMD_V_DOWN};
char cmd;
char buf[100], tx_buf[100];
char rx_index, tx_index, tx_len;
volatile char rx_complete;

unsigned char button = 1;

unsigned char current_selection = 0;

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

ISR(INT4_vect)
{
  button++;
  if(button == 6) button =1;
}
ISR(INT7_vect)
{
  
}

ISR(USART1_RX_vect)
{
  char ch;

  ch = UDR0;

  buf[rx_index++] = ch;

  if(ch == '\r'){
    buf[rx_index-1] = 0;
    rx_index = 0;
    rx_complete = 1;    
  } 
}

ISR(USART1_UDRE_vect)
{
  UDR0 = tx_buf[tx_index++];

  if(tx_index == tx_len)
    UCSR0B &= ~(1 << UDRIE0);
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




void u8g2_prepare(void) {
	u8g2_SetFont(&u8g2, u8g2_font_6x10_tf);
	u8g2_SetFontRefHeightExtendedText(&u8g2);
	u8g2_SetDrawColor(&u8g2, 1);
	u8g2_SetFontPosTop(&u8g2);
	u8g2_SetFontDirection(&u8g2, 0);
}





int main(void)
{
	UART_Init();

	_delay_ms(200);

	SendCommand(CMD_TF);

	const char *string_list =
	"Play Music\n"
	"Stop Music\n"
	"Pause Music\n"
	"Next Music\n"
	"Previous Music\n"
	"volume up\n"
	"volume down";

	
	u8g2_Setup_ssd1306_i2c_128x64_noname_f( &u8g2, U8G2_R0, u8x8_byte_sw_i2c, u8x8_avr_gpio_and_delay );
	u8g2_SetI2CAddress(&u8g2, 0x78);
	u8g2_InitDisplay(&u8g2);
	u8g2_SetPowerSave(&u8g2, 0);
	
	
	u8g2_ClearBuffer(&u8g2);
	u8g2_prepare();
	u8g2_SendBuffer(&u8g2);
		
	while(1)
	{
		u8g2_ClearBuffer(&u8g2);
		u8g2_prepare();

		u8g2_UserInterfaceSelectionList(&u8g2, "KMM's MP3", 1, string_list);

		u8g2_SendBuffer(&u8g2);
	}
}