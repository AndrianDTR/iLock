#include <avr/io.h> 
#include <avr/interrupt.h> 

#define F_CPU				18432200UL

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
#define USART_BAUDRATE 			115200UL 
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

void printB2B(unsigned char value, char* outArray)
{
	int i;
	const int CHAR_BIT = 8;
	
	for(i = CHAR_BIT-1; i >=0; --i)
	{
		outArray[i] = '0' + (value & 0x01);
		value >>= 1;
	}

	outArray[CHAR_BIT] = 0;
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
#define RF_DEMOD				((PIND & (1<<PIND6)) != 0)

volatile unsigned char count;					// counts 125kHz pulses
volatile unsigned char bitCnt;		// read bits counter


/*
#define ARRAYSIZE 256
//************************* CONVERT RAW DATA TO BINARY *************************
// Converts the raw 'pulse per wave' count (5,6,or 7) to binary data (0, or 1)  
//******************************************************************************
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

//******************************* FIND START TAG *******************************
// This function goes through the buffer and tries to find a group of fifteen   
// or more 1's in a row. This sigifies the start tag. If you took the fifteen   
// ones in multibit they would come out to be '111' in single-bit               
//******************************************************************************
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

//************************ PARSE MULTIBIT TO SINGLE BIT ************************
// This function takes in the start tag and starts parsing the multi-bit code   
// to produce the single bit result in the outputBuffer array the resulting     
// code is single bit manchester code                                           
//******************************************************************************
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

//******************************* Analize Input ********************************
// analizeInput(void) parses through the global variable and gets the 45 bit    
// id tag.                                                                      
// 1) Converts raw pulse per wave count (5,6,7) to binary data (0,1)            
// 2) Finds a start tag in the code                                             
// 3) Parses the data from multibit code (11111000000000000111111111100000) to  
//     singlebit manchester code (100110) untill it finds an end tag            
// 4) Converts manchester code (100110) to binary code (010)                    
//******************************************************************************
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
//******************************************************************************/

ISR(TIMER0_COMP_vect)
{
	++count;
}

void RFIDInit()
{
	DDRB |= (1 << 0);
	
	TCCR0 |= (1 << CS00)| (1 << WGM01) | (1 << COM00); // Set up timer at Fcpu/64 
	TIMSK |= (1 << OCIE0);
	OCR0 = 73; // Preload timer with precalculated value 
}

#define BITBUF_LEN 140
unsigned char bitBuf[BITBUF_LEN];

char RFIDReadTag(unsigned char* p)
{
	char res = 0;
	unsigned char cikl = 60;
	unsigned char readState = 0;
	unsigned char i;
	
	do{
		switch(readState)
		{
			//loop3
			case 0:
				count = 0;
				readState = 1;
				break;
			//vysok
			case 1:
				if(count < 70)
				{
					if(RF_DEMOD)
						break;
					*p++ = 2;
					readState = 3;
				}
				else if(count >= 130)
				{
					res = -1;
					break;
				}
				else
				{
					if(RF_DEMOD)
						break;
					*p++ = 3;
					readState = 6;
				}
				break;
			//nizk
			case 3:
				count = 0;
				readState = 4;
				break;
			//loop4
			case 4:
				if(count > 70)
				{
					if(!RF_DEMOD)
						break;
					*p++ = 1;
					readState = 6;
					break;
				}
				else if(count >= 130)
				{
					res = -1;
					break;
				}
				else
				{
					if(!RF_DEMOD)
						break;
					*p++ = 0;
					readState = 6;
				}
				
				break;
			case 6:
				--cikl;
				if(cikl) readState = 0;
				break;
			default:
				res = 1;
		}
	}while(!res);
	
	printf("AAA: ");
	for(i = 0; i < BITBUF_LEN-1; ++i) printf("%c", '0'+bitBuf[i]);
	printf("\n");
	return res;
}

unsigned char nibbleSwap(unsigned char a) 
{ 
	return (a<<4) | (a>>4); 
} 

char RFIDDecode(unsigned char* pp)
{
	char res = 0;
	unsigned char* p = pp;
	unsigned char nibble = 2;
	bitCnt = 200;
	
	const unsigned char DBL = BITBUF_LEN/2 + 3;
	unsigned char decBuf[DBL];
	unsigned char* d = decBuf;
	unsigned char i;
	for(i = 0; i < DBL; ++i) decBuf[i] = 0;
	for(i = 0; i < BITBUF_LEN; ++i) p[i] = 0;
	
	unsigned char system = 0;
	unsigned char data = 0;
	unsigned char temp = 0;
	
	do{
		switch(*p++)
		{
			//byt0_0
			case 0:
				system <<= 1;
				temp = (temp << 1)|1;
			
			//byt0
			case 1:
				system <<= 1;
				temp = (temp << 1)|1;
				break;
			
			//byt1
			case 2:
				system = (system << 1)|1;
				temp = (temp << 1)|1;
			
			//byt1_1
			case 3:
				system = (system << 1)|1;
				temp = (temp << 1)|1;
				break;
		}
		
		if(temp & 0x10)
		{
			unsigned char x = system;
			x &= 0x1E;
			x >>= 1;
			
			if(--nibble)
			{
				data = x << 4;
			}
			else
			{
				data |= x;
				*d++ = data;
				nibble = 2;
			}
			system &= 1;
			temp = 1;
		}
	}while(--bitCnt);

	bitCnt = 15;
	p = pp;
	d = decBuf;
	do
	{
		system = 0;
		nibble = 2;
		do
		{
			data = *d++;
			if((data & 0x80) == 0x80)
				system <<= 1;
			else
				system = (system << 1)| 1;
			if((data & 0x20) == 0x20)
				system <<= 1;
			else
				system = (system << 1)| 1;
			if((data & 0x08) == 0x08)
				system <<= 1;
			else
				system = (system << 1)| 1;
			if((data & 0x02) == 0x02)
				system <<= 1;
			else
				system = (system << 1)| 1;
			
			if(!--bitCnt) break;
	
		}while(--nibble);
		*p++ = system;
	}while(!bitCnt);
	
	d = decBuf;
	p = pp;
	bitCnt = 6;
	data = 0xFF;
	*d++ = data;
	data = *p++;
	system = 0xFE & data;
	system >>= 1;
	system |= 128;
	
	do
	{
		*d++ = system;
		system = *p++;
		data = nibbleSwap(data & 1) << 3;
		temp = ((system & 0xFE) << 1) | data;
		data = system;
		system = temp;
	}while(--bitCnt);
	*d++ = system;
	
	printf("DECODE: ");
	for(i=0; i < DBL/2; ++i)
	{
		printf("%02X ", decBuf[i]);
	}
	printf("\n");
	//*/
	return res;
}

#define		RS_NONE			0
#define		RS_SYNC			1
#define		RS_FIND			2
#define		RS_SHORT		3
#define		RS_LONG			4
#define		RS_PREAMB		5
#define		RS_HEADER		6
#define		RS_BIT1			7
#define		RS_DECODE		8
#define		RS_DECODE1		9

unsigned char rfState;

void StateMachine()
{
	switch(rfState)
	{
		case RS_SYNC:
		{
			bitCnt = 0;
			KBDParse();
			if(RF_DEMOD)
			{
				rfState = RS_FIND;
			}
			
			break;
		}
		
		case RS_FIND:
		{
			if(!RF_DEMOD)
			{
				rfState = RS_SHORT;
				count = 0;;
			}
			
			break;
		}
		
		case RS_SHORT:
		{
			if(count < 65)
			{
				rfState = RS_LONG;
			}
			else
			{
				rfState = RS_SYNC;
			}	
			
			break;
		}
		
		case RS_LONG:
		{
			if(count > 140)
			{
				rfState = RS_SYNC;
			}
			else if(RF_DEMOD)
			{
				rfState = RS_PREAMB;
				count = 0;
			}
			
			break;
		}
		
		case RS_PREAMB:
		{
			if(count >= 70)
				rfState = RS_BIT1;
			else if(!RF_DEMOD)
				rfState = RS_SYNC;
			
			break;
		}
		case RS_BIT1:
		{
			if(RF_DEMOD)
			{
				rfState = RS_HEADER;
				count = 0;
			}
			break;
		}
		case RS_HEADER:
		{
			if(count >= 70)
			{
				rfState = RS_LONG;
				count = 0;
			}
			else if(RF_DEMOD)
			{
				++bitCnt;
			}
			if(bitCnt == 9)
			{
				int res = RFIDReadTag(bitBuf); 
				
				if(1 == res) 		rfState = RS_DECODE;
				else if(-1 == res)	rfState = RS_SYNC;
			}
			else
			{
				rfState = RS_BIT1;
			}
			break;
		}
		case RS_DECODE:
		{
			RFIDDecode(bitBuf);
			unsigned char i;
			for(i = 0; i < BITBUF_LEN; ++i)
			{
				bitBuf[i] = 0;
			}
			
			rfState = RS_SYNC;
		}
		case RS_DECODE1:
		{
			
		}
	}
	//printf("SM Leave %i == %i.\n", count, count_1);
}

void init()
{
	DDRB = 0xFF;
	PORTB = 0xFF;
	DDRD = 0x3F;
	PORTD = 0x00;

	USARTInit();

	count = 0;
	bitCnt = 0;
	rfState = RS_SYNC;
	
	RFIDInit();
	
	LED_OK_CLR();
	LED_POWER_CLR();
	RELAY_CLR();
	BUZ_CLR();
	
	unsigned char i = 0;
	for(;i < BITBUF_LEN; ++i)
		bitBuf[i] = 0;
}

int main(void) 
{
	init();
	
	sei(); // Enable global interrupts 
	
	printf("A1\n");
	while(1) 
	{
		StateMachine();
		_delay_us(2);
	} 
	printf("A0\n");
	
	while(1)
	{
		printf("ERROR\n");
		_delay_ms(200);
	}
} 

