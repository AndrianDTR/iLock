#include <avr/io.h> 
#include <avr/interrupt.h> 

#define F_CPU				18432200UL
#define T0CNT_PRELOAD_125	10

#include <util/delay.h> 
#include <stdio.h>
#include <stdlib.h>

/******************************************************************************/
#define LED_POWER_CLR()			(PORTB |= (1 << 1))
#define LED_POWER_SET()			(PORTB &= (0 << 1))
#define LED_POWER_TOGLE()		(PORTB ^= (1 << 1))

#define LED_OK_CLR()			(PORTB |= (1 << 2))
#define LED_OK_SET()			(PORTB &= (0 << 2))
#define LED_OK_TOGLE()			(PORTB ^= (1 << 2))

#define RELAY_CLR()				(PORTB |= (1 << 4))
#define RELAY_SET()				(PORTB &= (0 << 4))
#define RELAY_TOGLE()			(PORTB ^= (1 << 4))

#define BUZ_CLR()				(PORTD |= (1 << 4))
#define BUZ_SET()				(PORTD &= (0 << 4))
#define BUZ_TOGLE()				(PORTD ^= (1 << 4))
#define BEEP()					BUZ_SET(); _delay_ms(50); BUZ_CLR();
#define BEEP_LONG()				BUZ_SET(); _delay_ms(200); BUZ_CLR();

/******************************************************************************/
#define USART_BAUDRATE 			19200UL 
#define BAUD_PRESCALE 			(((F_CPU / (USART_BAUDRATE * 16UL))) - 1) 

#define USART_MAX_BUFLEN		25
char szUSARTCmd[USART_MAX_BUFLEN+1];
int nUSARTBufLen;

void ParseCmd(char* cmd);

int USARTSendByte(char u8Data, FILE *stream)
{
	if(u8Data == '\n')
	{
		USARTSendByte('\r', 0);
	}
	//wait while previous byte is completed
	while(!(UCSRA&(1<<UDRE)));
	// Transmit data
	UDR = u8Data;
	return 0;
}

int USARTReceiveByte(FILE *stream)
{
	uint8_t u8Data;
	// Wait for byte to be received
	while(!(UCSRA&(1<<RXC)));
	u8Data=UDR;
	//echo input data
	USARTSendByte(u8Data, stream);
	// Return received data
	return u8Data;
}

FILE usart_str = FDEV_SETUP_STREAM(USARTSendByte, USARTReceiveByte, _FDEV_SETUP_RW);

void USARTInit(void) 
{
	// Turn on the transmission and reception circuitry 
	UCSRB = (1 << RXEN) | (1 << TXEN);
	UCSRC = (0<<UMSEL) | (0<<USBS) | (3<<UCSZ0); 

	//UCSRB |= (1 << RXCIE);
	
	// Load upper 8-bits of the baud rate value into the high byte of the UBRR register 
	UBRRH = (BAUD_PRESCALE >> 8); 
	// Load lower 8-bits of the baud rate value into the low byte of the UBRR register 
	UBRRL = BAUD_PRESCALE; 
	nUSARTBufLen = 0;
	
	stdin=stdout=&usart_str;
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
	printf("CMD: %s\n", cmd);
	
	nUSARTBufLen = 0;
	//LED_POWER_CLR();
	//LED_OK_SET();
	
	BUZ_SET();
	_delay_ms(100);
	BUZ_CLR();
}

/******************************************************************************/
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

void KBDParse()
{
	static unsigned char send = 0;
	unsigned char dt = 0;
	
	dt = GetSKey();
	if(dt != 0 && dt != send)
	{
		USARTWrite(dt);
		BEEP();
	}
	_delay_us(1);
	send = dt;
}

/*******************************************************************************/

#define	RF_PULSE_PIN			0
#define	RF_PULSE_TOGGLE()		(PORTB ^= (1 << RF_PULSE_PIN))

#define ARRAYSIZE 256

char* begin; 
volatile int iter;      // the iterator for the placement of count in the array
volatile int count;     // counts 125kHz pulses
volatile int lastpulse; // last value of DEMOD_OUT
volatile int on;        // stores the value of DEMOD_OUT in the interrupt
/************************* CONVERT RAW DATA TO BINARY *************************\
| Converts the raw 'pulse per wave' count (5,6,or 7) to binary data (0, or 1)  |
\******************************************************************************/
void convertRawDataToBinary(char* buffer)
{
	int i;
	printf("C1\n");
	for (i = 1; i < ARRAYSIZE; i++)
	{
		printf("C1: %i, %c\n", i, buffer[i]);
		if (buffer[i] == 5)
		{
			printf("C2\n");
			buffer[i] = 0;
		}
		else if (buffer[i] == 7)
		{
			printf("C3\n");
			buffer[i] = 1;
		}
		else if (buffer[i] == 6)
		{
			printf("C4\n");
			buffer[i] = buffer[i-1];
		}
		else
		{
			printf("C5\n");
			buffer[i] = -2;
		}
	}
	printf("C0\n");
}

/******************************* FIND START TAG *******************************\
| This function goes through the buffer and tries to find a group of fifteen   |
| or more 1's in a row. This sigifies the start tag. If you took the fifteen   |
| ones in multibit they would come out to be '111' in single-bit               |
\******************************************************************************/
int findStartTag (char * buffer)
{
	int i;
	int inARow = 0;
	int lastVal = 0;
	
	for (i = 0; i < ARRAYSIZE; i++)
	{
		if (buffer [i] == lastVal)
		{
			inARow++;
		}
		else
		{
			// End of the group of bits with the same value
			if (inARow >= 15 && lastVal == 1)
			{
				// Start tag found
				break;
			}
			
			// group of bits was not a start tag, search next tag
			inARow = 1;
			lastVal = buffer[i];
		}
	}
	return i;
}

/************************ PARSE MULTIBIT TO SINGLE BIT ************************\
| This function takes in the start tag and starts parsing the multi-bit code   |
| to produce the single bit result in the outputBuffer array the resulting     |
| code is single bit manchester code                                           |
\******************************************************************************/
void parseMultiBitToSingleBit (char * buffer, int startOffset, int outputBuffer[]) 
{
	int i = startOffset; // the offset value of the start tag
	int lastVal = 0; // what was the value of the last bit
	int inARow = 0; // how many identical bits are in a row// this may need to be 1 but seems to work fine
	int resultArray_index = 0;
	for (;i < ARRAYSIZE; i++)
	{
		if (buffer [i] == lastVal)
		{
			inARow++;
		}
		else
		{
			// End of the group of bits with the same value
			if (inARow >= 4 && inARow <= 8)
			{
				// there are between 4 and 8 bits of the same value in a row
				// Add one bit to the resulting array
				outputBuffer[resultArray_index] = lastVal;
				resultArray_index += 1;
			}
			else if (inARow >= 9 && inARow <= 14)
			{
				// there are between 9 and 14 bits of the same value in a row
				// Add two bits to the resulting array
				outputBuffer[resultArray_index] = lastVal;
				outputBuffer[resultArray_index+1] = lastVal;
				resultArray_index += 2;
			}
			else if (inARow >= 15 && lastVal == 0)
			{
				// there are more then 15 identical bits in a row, and they are 0s
				// this is an end tag
				break;
			}
			
			// group of bits was not the end tag, continue parsing data
			inARow = 1;
			lastVal = buffer[i];
			if (resultArray_index >= 90)
			{
				//return;
			}
		}
	}
}

/******************************* Analize Input *******************************\
| analizeInput(void) parses through the global variable and gets the 45 bit   |
| id tag.                                                                     |
| 1) Converts raw pulse per wave count (5,6,7) to binary data (0,1)           |
| 2) Finds a start tag in the code                                            |
| 3) Parses the data from multibit code (11111000000000000111111111100000) to |
|     singlebit manchester code (100110) untill it finds an end tag           |
| 4) Converts manchester code (100110) to binary code (010)                   |
\*****************************************************************************/
void analizeInput (void) 
{
	printf("B1\n");
	int i;                // Generic for loop 'i' counter
	int resultArray[90];  // Parsed Bit code in manchester
	int finalArray[45];   //Parsed Bit Code out of manchester
	int finalArray_index = 0;

	// Initilize the arrays so that any errors or unchanged values show up as 2s
	for (i = 0; i < 90; i++) { resultArray[i] = 2; }
	for (i = 0; i < 45; i++) { finalArray[i]  = 2; }

	printf("B2\n");
	begin = 0;
	// Convert raw data to binary
	convertRawDataToBinary(begin);
	
	printf("B3\n");
	// Find Start Tag
	int startOffset = findStartTag(begin);
	LED_POWER_SET();

	printf("B4\n");
	// Parse multibit data to single bit data
	parseMultiBitToSingleBit(begin, startOffset, resultArray);

	printf("B5\n");
	// Error checking, see if there are any unset elements of the array
	for (i = 0; i < 88; i++)
	{ // ignore the parody bit ([88] and [89])
		if (resultArray[i] == 2)
		{
			return;
		}
	}
	
	printf("B6\n");
	//------------------------------------------
	// MANCHESTER DECODING
	//------------------------------------------
	for (i = 0; i < 88; i+=2)
	{ 
		// ignore the parody bit ([88][89])
		if (resultArray[i] == 1 && resultArray[i+1] == 0)
		{
			finalArray[finalArray_index] = 1;
		}
		else if(resultArray[i] == 0 && resultArray[i+1] == 1)
		{
			finalArray[finalArray_index] = 0;
		}
		else
		{
			// The read code is not in manchester, ignore this read tag and try again
			// free the allocated memory and end the function
			return;
		}
		finalArray_index++;
	}

	printf("B7\n");
	#ifdef Binary_Tag_Output         // Outputs the Read tag in binary over serial
	printBinary (finalArray);
	#endif

	#ifdef Hexadecimal_Tag_Output    // Outputs the read tag in Hexadecimal over serial
	printHexadecimal (finalArray);
	#endif

	#ifdef Decimal_Tag_Output
	printDecimal (finalArray);
	#endif

	#ifdef Whitelist_Enabled
	if (searchTag(getDecimalFromBinary(finalArray+UNIQUE_ID_OFFSET,UNIQUE_ID_LENGTH)))
	{
		whiteListSuccess ();
	}
	else
	{
		whiteListFailure();
	}
	#endif
	printf("B0\n");
}

ISR(TIMER0_COMP_vect)
{
	//Save the value of DEMOD_OUT to prevent re-reading on the same group
	on = ((PIND & (1<<PIND6)) != 0);
	
	// if wave is rising (end of the last wave)
	if(on != lastpulse)
	{
		//printf("X4\n");
		begin[++iter] = count; 
		// write the data to the array and reset the count
		count = 0;
		begin[iter] = 0;
	}
	//*/
	
	++count;
	lastpulse = on;
	//*/
}

void RFIDReset()
{
	int i;
	
	//reset the saved values to prevent errors when reading another card
	count = 0;
	iter = 0;
	
	for (i = 0; i < ARRAYSIZE; ++i)
	{
		begin[i] = 0;
	}
}

void RFIDInit()
{
	DDRB |= (1 << 0);
	
	TCCR0 |= (1 << CS00)| (1 << WGM01) | (1 << COM00); // Set up timer at Fcpu/64 
	TIMSK |= (1 << OCIE0);
	OCR0 = 73; // Preload timer with precalculated value 
	
	
	//========> VARIABLE INITILIZATION <=======//
	begin = malloc(sizeof(char) * ARRAYSIZE);
	RFIDReset();
}

int main(void) 
{
	int i;
	iter = 0;
	count = 0;

	USARTInit();
 
	DDRB = 0xFF;
	PORTB = 0xFF;
	DDRD = 0b00111100;
	PORTD = 0x00;
	
	LED_OK_CLR();
	LED_POWER_CLR();
	RELAY_CLR();
	BUZ_CLR();
	
	RFIDInit();
	
	sei(); // Enable global interrupts 
	
	printf("A1\n");
	while(1) 
	{
		KBDParse();
		
		//printf("A2: I:%i, C:%i\n", iter, count);
		// while the card is being read
		if(iter >= ARRAYSIZE)
		{
			printf("A3 %i \n", iter);
			// if the buffer is full
			//cli(); // disable interrupts
			//analizeInput();
			RFIDReset();
		}
				
		//printf("A4\n");
		//
		
		//sei();
		
		//*/
	} 
	printf("A0\n");
	
	while(1)
	{
		printf("ERROR\n");
	}
} 

