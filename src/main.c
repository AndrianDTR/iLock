#include <avr/io.h> 
#include <avr/interrupt.h> 

#define F_CPU				18432200UL
#define T1CNT_PRELOAD_125	65489

#include <util/delay.h> 
#include <stdio.h>

/*------------------------------------------------------------------------------*/
#define LED_POWER_CLR()			(PORTB |= (1 << 1))
#define LED_POWER_SET()			(PORTB &= (0 << 1))
#define LED_POWER_TOGLE()		(PORTB ^= (1 << 1))

#define LED_OK_CLR()			(PORTB |= (1 << 2))
#define LED_OK_SET()			(PORTB &= (0 << 2))
#define LED_OK_TOGLE()			(PORTB ^= (1 << 2))

#define BUZ_CLR()				(PORTD |= (1 << 4))
#define BUZ_SET()				(PORTD &= (0 << 4))
#define BUZ_TOGLE()				(PORTD ^= (1 << 4))
#define BEEP()					BUZ_SET(); _delay_ms(50); BUZ_CLR();

/*------------------------------------------------------------------------------*/
#define	RF_PULSE_PIN			0
#define	RF_PULSE_TOGGLE()		(PORTB ^= (1 << RF_PULSE_PIN))

//RFID 125 kHz pulse generator
ISR(TIMER1_OVF_vect) 
{ 
	PORTB ^= 0x01; // Toggle PINB0
	TCNT1  = T1CNT_PRELOAD_125; // Reload timer with precalculated value 
}

void RFIDInit()
{
	DDRB |= (1 << 0); // Set LED as output 

	TIMSK |= (1 << TOIE1); // Enable overflow interrupt 
	TCNT1 = T1CNT_PRELOAD_125; // Preload timer with precalculated value 
	TCCR1B |= (1 << CS10); // Set up timer at Fcpu/64 
}

/*------------------------------------------------------------------------------*/

#define KEYPAD_ROWS		4
#define KEYPAD_COLS		3

//*******************************
#define KEYPAD_PORT		PORTC
#define KEYPAD_DDR		DDRC
#define KEYPAD_PIN		PINC

/*******************************************
 * Function return the keycode of keypressed
 * on the Keypad. Keys are numbered as follows

 * [00] [01] [02]
 * [03] [04] [05]
 * [06] [07] [08]
 * [09] [10] [11]
 * 
 * Arguments:
 * 	None
 * 	Return:
 * 		Any number between 0-11 depending on keypressed.
 * 		
 * 		255 (hex 0xFF) if NO keypressed.
 * 
 * 	Precondition:
 * 		None. Can be called without any setup.
 *
 ********************************************/
uint8_t GetKeyPressed()
{
	uint8_t row, col;
	KEYPAD_PORT |= 0x0F;

	for(col = 0; col < KEYPAD_COLS; col++)
	{
		KEYPAD_DDR &= ~(0x7F);
		KEYPAD_DDR |= (0x40 >> col);
		
		for(row = 0; row < KEYPAD_ROWS; row++)
		{
			if(!(KEYPAD_PIN & (0x08 >> row)))
			{
				return (row * 3 + col);
			}
		}
	}
	
	return 0xFF;
}

unsigned char GetSKey()
{
	switch(GetKeyPressed())
	{
		case 0:  return '#';
		case 1:  return '0';
		case 2:  return '*';
		case 3:  return '9';
		case 4:  return '8';
		case 5:  return '7';
		case 6:  return '6';
		case 7:  return '5';
		case 8:  return '4';
		case 9:  return '3';
		case 10: return '2';
		case 11: return '1';
	}
	return 0;
}

uint8_t GetIKey()
{
	switch(GetKeyPressed())
	{
		case 0:  return 0x0B;
		case 1:  return 0;
		case 2:  return 0x0A;
		case 3:  return 9;
		case 4:  return 8;
		case 5:  return 7;
		case 6:  return 6;
		case 7:  return 5;
		case 8:  return 4;
		case 9:  return 3;
		case 10: return 2;
		case 11: return 1;
	}
	return 255;
}

/***********************************************************************************/
#define USART_BAUDRATE 9600UL 
#define BAUD_PRESCALE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1) 

#define USART_MAX_BUFLEN	25
char szUSARTCmd[USART_MAX_BUFLEN+1];
int nUSARTBufLen;

void ParseCmd(char* cmd);

void USARTInit(void) 
{
	// Turn on the transmission and reception circuitry 
	UCSRB = (1 << RXEN) | (1 << TXEN);
	UCSRC = (0<<UMSEL) | (0<<USBS) | (3<<UCSZ0); 

	UCSRB |= (1 << RXCIE);
	
	// Load upper 8-bits of the baud rate value into the high byte of the UBRR register 
	UBRRH = (BAUD_PRESCALE >> 8); 
	// Load lower 8-bits of the baud rate value into the low byte of the UBRR register 
	UBRRL = BAUD_PRESCALE; 
	nUSARTBufLen = 0;
}

void USARTWrite(unsigned char data)
{
	// Do nothing until UDR is ready for more data to be written to it 
	while((UCSRA & (1 << UDRE)) == 0); 
	UDR = data; 
}

ISR(USART_RX_vect)
{
	char status = UCSRA;
	char data = UDR;

	if ((status & ((1<<FE) | (1<<PE) | (1<<DOR))) == 0) 
	{
		if(data == 13) 
		{
			USARTWrite(13);
			USARTWrite(10);
			szUSARTCmd[nUSARTBufLen] = 0;
			
			ParseCmd(szUSARTCmd);
			
			USARTWrite(13);
			USARTWrite(10);
			USARTWrite('#');
		}
		else if(nUSARTBufLen < USART_MAX_BUFLEN)
		{
			szUSARTCmd[nUSARTBufLen] = data;
			USARTWrite(szUSARTCmd[nUSARTBufLen]);
			++nUSARTBufLen;
		}
	}
}

void ParseCmd(char* cmd)
{
	int i;
	
	USARTWrite('C');
	USARTWrite('M');
	USARTWrite('D');
	USARTWrite(':');
	for(i = 0; i < nUSARTBufLen; ++i)
		USARTWrite(cmd[i]);
	
	nUSARTBufLen = 0;
	//LED_POWER_CLR();
	//LED_OK_SET();
	
	BUZ_SET();
	_delay_ms(100);
	BUZ_CLR();
	
}

/*******************************************************************************/

int main (void) 
{ 
	DDRB = 0xFF;
	PORTB = 0xFF;
	DDRD = 0b10111100;
	PORTD = 0xFF;
	
	RFIDInit();
	USARTInit();
	sei(); // Enable global interrupts 

	unsigned char dt = 0;
	unsigned char gSend = 0;
	
	printf("Andrian #");
	
	while(1) 
	{ 
		dt = GetSKey();
		if(dt != 0 && dt != gSend)
		{
			USARTWrite(dt);
			BEEP();
		}
		_delay_us(1);
		gSend = dt;
	} 
} 

