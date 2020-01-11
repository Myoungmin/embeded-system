#include <avr/io.h>
#include "u8g2.h"
#include <util/delay.h>
#include <math.h>

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


void u8g2_prepare(void) {
	u8g2_SetFont(&u8g2, u8g2_font_6x10_tf);
	u8g2_SetFontRefHeightExtendedText(&u8g2);
	u8g2_SetDrawColor(&u8g2, 1);
	u8g2_SetFontPosTop(&u8g2);
	u8g2_SetFontDirection(&u8g2, 0);

}




int main(void)
{
	
	u8g2_Setup_ssd1306_i2c_128x64_noname_f(&u8g2, U8G2_R0, u8x8_byte_sw_i2c, u8x8_avr_gpio_and_delay );
	u8g2_SetI2CAddress(&u8g2, 0x78);
	u8g2_InitDisplay(&u8g2);
	u8g2_SetPowerSave(&u8g2, 0);
	
	u8g2_prepare();

	/*while(1){
		u8g2_DrawBox(&u8g2, 3,7,25,15);
	}*/
	
	DDRD = 0x00;	//인터럽트 4,5,6,7 켜기위해 포트 입력으로
	EIMSK = (1 << INT4) | (1 << INT5) | (1 << INT6) | (1 << INT7);

	u8g2_UserInterfaceSelectionList(&u8g2, "Myoungmin's Game", 1, "SNAKE GAME\nTETRIS");

}