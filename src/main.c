/*
 * 
 */
#include <avr/io.h> 
#include <avr/interrupt.h> 

#define F_CPU				18432200UL

#include <util/delay.h> 
#include <stdio.h>
#include <stdlib.h>

/******************************************************************************/
#define LED_POWER_CLR()			(PORTB |= (1 << 1))
#define LED_POWER_SET()			(PORTB &= ~(1 << 1))
#define LED_POWER_TOGLE()		(PORTB ^= (1 << 1))

#define LED_OK_CLR()			(PORTB |= (1 << 2))
#define LED_OK_SET()			(PORTB &= ~(1 << 2))
#define LED_OK_TOGLE()			(PORTB ^= (1 << 2))

#define RELAY_CLR()				(PORTB |= (1 << 4))
#define RELAY_SET()				(PORTB &= ~(1 << 4))
#define RELAY_TOGLE()			(PORTB ^= (1 << 4))

#define BUZ_CLR()				(PORTD |= (1 << 4))
#define BUZ_SET()				(PORTD &= ~(1 << 4))
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
 * [03] [04] [05]Program exited with return code: 1

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
#define RFID_HALFBIT_LEN		32
#define	RFID_SHORT				RFID_HALFBIT_LEN * 2 + 1
#define	RFID_LONG				RFID_HALFBIT_LEN * 4 + 2

#define RF_HEADER_BITCNT		9
#define RF_READ_ERROR			-1
#define RF_DEMOD				((PIND & _BV(PIND6)) != 0)

// data buffer length
#define BITBUF_LEN 70

// State machine states
#define		RS_SYNC			0
#define		RS_FIND			10
#define		RS_H			20
#define		RS_L			30
#define		RS_SHORT		40
#define		RS_LONG			50
#define		RS_PREAMB		60
#define		RS_READTAG		70
#define		RS_DECODE		80
#define		RS_DECODE1		90

// counts 125kHz pulses
volatile unsigned int 			count;					

// read bits counter
unsigned char 					bitCnt;	

// current machine state
unsigned char 					rfState;

// data buffer
unsigned char 					bitBuf[BITBUF_LEN+1];

/*
 * @brief	Swap nibbles in the byte
 * 
 * @in 		byte where nibbles should be swapped
 *
 * @return 	byte with swapped nibbles
 *
 */
unsigned char nibbleSwap(unsigned char a) 
{ 
	return (a<<4) | (a>>4); 
} 

/*
 * @brief	Timer0 interrupt handler
 */
ISR(TIMER0_COMP_vect)
{
	static unsigned char n = 0;
	if(n&1)
	{
		++count;
		n = 0;
	}
	else
		n = 1;
}

/*
 * @brief	Initialize Timer0 and RFID
 * 
 * @return 	void
 *
 */
void RFIDInit()
{
	DDRB |= (1 << 0);
	
	TCCR0 |= (1 << CS00)| (1 << WGM01) | (1 << COM00);
	TIMSK |= (1 << OCIE0);

	// Preload timer with precalculated value >
	OCR0 = 73; 
}

char RFIDRead5Bit()
{
	char system = 0;
	
	count = 0;
	bitCnt = 5;
	
	do{
		if(count > 96)
		{
			if(RF_DEMOD)
			{
				system = (system << 1) | 1;
		
				do{
					if(count > 144)
					{
						return 0;
					}
				}
				while(RF_DEMOD);
				count = 0;
			}
			else
			{
				system <<= 1;
				
				do{
					if(count > 144)
					{
						return 0;
					}
				}while(!RF_DEMOD);
				count = 0;
			}
		}
	}while(--bitCnt);
	
	system >>= 1;
		
	return system;
}

/*
 * @brief	Read bit from RFID tag
 * 
 * @return 	int, read bit value or -1 if timeout is exceeded.
 *
 */
unsigned char RFIDReadByte()
{
	unsigned char value = RF_DEMOD;
	count = 0;
	
	do
	{
		if(count > RFID_SHORT && value != RF_DEMOD)
		{
			value = (value == 0);
			break;
		}
		else if(count > RFID_LONG)
		{
			value = -1;
			break;
		}
	}while(1);
	
	return value;
}

char RFIDReadTag(unsigned char* p)
{
	char res = 0;
	unsigned char cikl = 60;
	unsigned char readState = 1;
	unsigned char i;
	
	do{
		switch(readState)
		{
			//vysok
			case 1:
				if(count < RFID_SHORT)
				{
					if(RF_DEMOD)
						break;
					*p = 2;
					readState = 3;
				}
				else if(count > RFID_LONG)
				{
					res = -1;
					break;
				}
				else
				{
					if(RF_DEMOD)
						break;
					*p = 3;
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
					*p = 1;
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
					*p = 0;
					readState = 6;
				}
				
				break;
			case 6:
				--cikl;
				p++;
				if(cikl)
				{
					readState = 5;
				}
				break;
			default:
				res = 1;
		}
	}while(!res);
	
	printf("AAA: ");
	for(i = 0; i < BITBUF_LEN-1; ++i) printf("%c", '0'+bitBuf[i]);
	printf("\n\n");
	return res;
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

void StateMachine()
{
	switch(rfState)
	{
		case RS_SYNC:
		{
			KBDParse();
			
			if(!RF_DEMOD) rfState = RS_H;
			break;
		}
		case RS_H:
			if(RF_DEMOD) rfState = RS_L;
			break;
		case RS_L:
			
			if(!RF_DEMOD)
			{
				rfState = RS_SHORT;
				count = 0;
			}
			break;
		
		case RS_SHORT:
		{
			if(count > RFID_SHORT)
			{
				if(!RF_DEMOD)
				{
					rfState = RS_LONG;
				}
				else
					rfState = RS_SYNC;
			}
			
			break;
		}
		
		case RS_LONG:
		{
			if(count > RFID_LONG)
			{
				if(RF_DEMOD)
				{
					rfState = RS_PREAMB;
					bitCnt = 0;
					count = 0;
				}
				else
				{
					rfState = RS_SYNC;
				}
			}
			
			break;
		}
		
		case RS_PREAMB:
		{
			unsigned char bit = RF_DEMOD;
			if(count > RFID_SHORT)
			{
				if(0 == bit)
				{
					++bitCnt;
					count = 0;
				}
				else
				{
					rfState = RS_SYNC;
				}
			}
			
			if(RF_HEADER_BITCNT == bitCnt)
			{
				
				LED_OK_SET();
				RFIDReadTag(bitBuf);

				_delay_ms(100);
				LED_OK_CLR();
			
				rfState = RS_DECODE;
			}
			break;
		}
		
		case RS_DECODE:
		{
			//RFIDDecode(bitBuf);
			
			printf("READED: ");
			unsigned char i;
			for(i = 0; i < BITBUF_LEN; ++i)
			{
				printf("%c", '0'+bitBuf[i]);
				bitBuf[i] = 0;
			}
			printf("\n");
			rfState = RS_SYNC;
			break;
		}

		default:
			rfState = RS_SYNC;
	}
	//printf("SM Leave %i. \n", count);
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
	//unsigned char cnt = 0;
	//unsigned char val = RF_DEMOD;
	printf("A1\n");
	//count = 0;
	while(1) 
	{
		StateMachine();
		_delay_us(2);
		/*
		unsigned char v2 = RF_DEMOD;
		if(val != v2)
		{
			bitBuf[cnt++] = count;
			count = 0;
			val = v2;
		}
		
		if(cnt == 250)
		{	
			unsigned char i;
			for(i = 0; i < BITBUF_LEN; ++i)
			{
				printf("%i => %i\n", i, bitBuf[i]);
				bitBuf[i] = 0;
			}
			printf("\n\n");
			count = 0;
			cnt = 0;
			val = v2 = 0;
		}
		//*/
	} 
	printf("A0\n");
	
	while(1)
	{
		printf("ERROR\n");
		_delay_ms(200);
	}
} 

