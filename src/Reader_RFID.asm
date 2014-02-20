                ;##############################################
                ;##             Reader_RFID                  ##
                ;##                                          ##
                ;##############################################
                ;8.0 MZh int osc  , ��� �������� �� 8, BODLEVEL- 2.7v.
                ;* ������������ ������� ���������� � ��������� �� LM358
                ; � ���������� ������� ���������:
				; PB0- bipper
				; PB1- ���� ��������� �����������(��������� ������� rfid)
				; PB2- ����� ������� 125 ���, ��� ������� ������ (�� ����������� �� ������� 0) 
				; PD0- ������ ��� ��������� ������� ������ ���� ����� RFID � ��������� � ������ EEPROM �� ������ 80�				
;-------------------------   ������������� ����������

.include "tn2313def.inc"	; ������������� ����� ��������
.list					    ; ��������� ��������
;------------------------------------------------
.cseg 						; ����� �������� ������������ ����
.org 	0					; ���������� �������� ������ �� ����
;------------------------------------------------
.def	S			=R0
.def	temp		=R16
.def	timerL		=R17
.def	system	  	=R18
.def	cikl    	=R19
.def	bitcnt		=R20
.def	parity		=R21
.def	addr		=R22
.def	data		=R23

.equ	INPUT		=1	;PB1--���� �� �����������
;----------------------------------------------------
				rjmp	start		 
				rjmp	m1	 
				rjmp	m2	 
				rjmp	m3	 
				rjmp	m4	 
				rjmp	m5 
				rjmp	m6	 
				rjmp	m7	 
				rjmp	m8	 
				rjmp	m9	 
				rjmp	m10		
				rjmp	m11		
				rjmp	m12	
				rjmp	OC0Aaddrs	 
				rjmp	m14	 
				rjmp	m15	
				rjmp	m16	
				rjmp	m17	
				rjmp	m19	
;-----------------------------------------------------------------
m1:				reti
m2:				reti
m3:				reti
m4:				reti
m5:				reti
m6:				reti
m7:				reti
m8:				reti
m9:				reti
m10:			reti
m11:			reti
m12:			reti
	 
m14:			reti
m15:			reti
m16:			reti
m17:			reti
m18:			reti
m19:			reti
;*************************************************
start:			ldi temp,low(RAMEND)
				out SPL,temp
;-------------------------------------------------				
				ldi 	temp, 0x80; ���������� �����������
				out		ACSR, temp              
;-------------------------------------------------
				ldi		temp,0b11111101		; ����� ��1 �� ���� ��������� �� �����
				out 	DDRB,temp
				ldi		temp,0	
				out		PORTB,temp				 
				out		DDRD,temp			; ��� ����� ����� D �� ����
				ldi		temp,255	
				out		PORTD,temp			; ����������� ���������� �������� 
;------------------------------------------------
;						Timer0 OC0A
;------------------------------------------------
				ldi		temp,0b01000010		; 0b01000010	
				out		TCCR0A,temp
				ldi		temp,0b10000001		; 0b10000001	
				out		TCCR0B,temp			; ������ ��������
				ldi		temp,0b00000000
				out		TCNT0,temp			; �������� �������� ��������
				ldi		temp,32				;8.0 OSC	; ������� ������� 8��� �� 32 = 250 ���
				out		OCR0A,temp			; � �������� ����������
				ldi		temp,1<<OCIE0A		; ���������� ���������� �� ���������� � ������ � ������� 0
				out		TIMSK,temp
;-----------------------------------------------

;**************************************************************************************************										
klava:   		in      temp, PIND     		; ����-�� ������� ������� ������� 
				sbrs	temp, 0
				rjmp	sync
				rjmp	klava
;-------------------------------------------------------------------------------------------
sync:		    sei						; ����� ���������� �� ����������
				clr		bitcnt			; ������� ��������� ������ ��������� �����(���������)
H:				sbis	PINB, INPUT		; �������� �������� ������
				rjmp	H

L:				sbic	PINB,INPUT		; �������� ������� ������
				rjmp	L

				clr		timerL			; ������ ��������
korotkaja:		cpi		timerL,65		; ����� �������� ������ �������� �� ����� ��� �� 280 ��.������
				brsh	dlinnaya		; ���� �� ����� ������� ��������
				sbis 	PINB,INPUT
				rjmp	korotkaja		; ����������� ������
				rjmp	sync			; ������� ��������, ������ ����� ������� 
				
dlinnaya:		cpi		timerL,140		; ������ � ��������: �� ����� ��� �� 560 ��.������ 
				brsh	sync			; ����� �� ������� �� �������	
				sbic	PINB,INPUT		
				rjmp	preambula		; ������������� �����������, ����� ���������� � ������ ���������
				rjmp	dlinnaya        ; <<************************************************************
;------------------------------------------------------------------------------------------------------				

preambula:		clr		timerL			; �������� ����� �� 260 ��.���
loop1:			cpi		timerL,70
				brsh	bit1            ; ���� �� ������� ��������� ���������"�������"
				sbic	PINB,INPUT
				rjmp	loop1
				rjmp	sync			; ���� �� ������� ��������� ��������� "����" - ������ �������
						
bit1:			sbic	PINB,INPUT		; �������� ����� ��������
				rjmp	bit1			
				
				clr		timerL			; ������� ��������
loop2:			cpi		timerL,70		; �������� ������� �������� � ������� 280 ��.������
				brsh	dl				; ���� �� ����� ������� ��������
				sbis 	PINB,INPUT
				rjmp	loop2			
				inc		bitcnt			; ��������� �������
				cpi		bitcnt,9
				breq	massiv		    ; ��� �������� ���������� ���������
				rjmp	bit1			; ������ ������
dl:				clr		bitcnt			
				rjmp	dlinnaya		; ��� �� ������� ������
																										
;-------------------------------------------------------
massiv:			clr		XH
				ldi		XL,0x60
				ldi		cikl,60

loop3:			clr		timerL
vysok:			cpi		timerL,70
				brsh	met11
				sbic	PINB,INPUT
				rjmp	vysok
				ldi		parity,2
				st		X+,parity
				rjmp	nizki
met11:			cpi		timerL,130
vyh:			brsh	sync
				sbic	PINB,INPUT
				rjmp	met11
				ldi		parity,3
				st		X+,parity
nizki:			clr		timerL
loop4:			cpi		timerL,70
				brsh	met10
				sbis	PINB,INPUT
				rjmp	loop4
				ldi		parity,1
				st		X+,parity
				dec		cikl
				brne	loop3
				rjmp	decodir
met10:			cpi		timerL,130
				brsh	vyh
				sbis	PINB,INPUT
				rjmp	met10
				clr		parity
				st		X+,parity
				dec		cikl
				brne	loop3
;---------------------------------------
decodir:		cli
				clr		temp
				ldi		YL,0xc5	
				ldi		bitcnt,128
				clr		system
				ldi		timerL,2												
				ldi		XL,0x60

loop5:			ld		data,X+		  	
				cpi		data,0			; ����������
				breq	byt0_0
				cpi		data,1
				breq	byt0
				cpi		data,2
				breq	byt1
				cpi		data,3
				breq	byt1_1
			
byt0_0:			clc	
				rol		system			; ������������ "0"
				sec
				rol		temp			; �������� ��������
byt0:			clc		
				rol		system			; ������������ "0"
				sec
				rol		temp			; �������� ��������
				brhs	contin			; � �������� �� ������������ �������
				dec		bitcnt			; ��������� ����� ������� 
				breq	decod2			; � ������ ���������� ���
				rjmp	loop5			; ��������� �� ���������

byt1_1:			sec
				rol		system			; ������������ "1"
				sec						
				rol		temp			; �������� ��������
byt1:			sec
				rol		system			; ������������ "1"
				sec
				rol		temp			; �������� ��������
				brhs	contin			; � �������� �� ������������ �������
				dec		bitcnt			; ��������� ����� ������� 
				breq	decod2			; � ������ ���������� 
				rjmp	loop5			; ��������� �� ���������						

contin:			dec		timerL			; �������������� � �������
				breq	con1
				push	system			; �������� ���������
				andi	system,0x1E		; �������� ������ 4 ������� ����
				ror		system          ; �������� �� ����� �������
				mov		parity,system   ; � ��������� � data
				swap	parity          ; ������� ������� ������
				pop		system			; ����������� 5 ���	
				andi	system,0x01		; �������� ������ 1�
				ldi		temp,1          ; ���������������� ������� ����� ����
				dec		bitcnt			; ��������� ����� ������� 
				breq	decod2			; � ������ ����������
				rjmp	loop5

con1:			push	system          ; �������� ���������
				andi	system,0x1E		; �������� ������ ������� 4 ����
				ror		system          ; �������� �� ����� �������
				or		parity,system   ; ���������� �� ������� ����������
				st		Y+,parity		; ��������� � ������ ������������� ����	
				pop		system			; ����������� ��������� 5 ���
				andi	system,0x01 	; �������� ������ ������� ��� 
				ldi		temp,1      	; ���������������� ������� ����� ���� 
				ldi		timerL,2		; ��������� ����� 2� ������� ����
				dec		bitcnt			; ��������� ����� ������� 
				breq	decod2			; � ������ ����������
				rjmp    loop5 			; ������� �� ��������� ���	 
;-------------------------------------------------
decod2:			ldi		cikl,15
				ldi		YL,0x60
				ldi		XL,0xc5
loop6:			clr		system
				ldi		timerL,2

bi7:			ld		data,X+
				sbrc	data,7
				rjmp	bi7_1
				clc
				rol		system
				rjmp	bi5

bi7_1:			sec
				rol 	system
bi5:			sbrc	data,5
				rjmp	bi5_1
				clc
				rol		system
				rjmp	bi3

bi5_1:			sec
				rol 	system	

bi3:			sbrc	data,3
				rjmp	bi3_1
				clc
				rol		system
				rjmp	bi1

bi3_1:			sec
				rol 	system					

bi1:			sbrc	data,1
				rjmp	bi1_1
				clc
				rol		system
				rjmp	bi

bi1_1:			sec
				rol 	system

bi:				dec		cikl
				breq	bi_1
				dec		timerL
				brne	bi7
				st		Y+,system
				rjmp	loop6
bi_1:			st		Y+,system
;----------------------------------

decod3:			ldi		YL,0x70   		; ���.����� �������� 
				ldi		XL,0x60			; ���.����� ���������
				ldi		cikl,6			; ���� �� 6�� ������ + 2 ���������� =8
				ser		data			; ������� ��� ��������� preambula 8 ���
				st		Y+,data         ; � ���
				
				ld		data,X+			; �� 1�� ����� 
				ldi		system,0xFE		
				and		system,data		; �������� 7 �������
				ror		system
				ldi		temp,128
				or		system,temp     ; � ������� � 9��� ����� ��������� � ��������� ��� ������ ���� 

loop7:			st		Y+,system		; ��������� � ���
				ld		system,X+		; ����������� ��������� ���� 
				andi	data,0x01		; �������� ������� ��� ����������� �����
				swap	data			; ��������� �� ����� 7�� �������
				rol		data
				rol		data
				rol		data
				ldi		temp,0xFE
				push	system			; �������� ��������� ��� ����������� �������������
				and		system,temp		; �������� � ������� ����� 7 ������� ���
				ror		system			; � �������� � ������� �����
				or		system,data		; ���������� ���������� � �������
				pop		data			; ����������� ������� ���������
				dec		cikl			; ��������� ������� � ��������� 
				breq	zapis			; ���� ��� 6�� ���������� �����
				rjmp	loop7			; ���� ����������												
;-------------------------------------------------
zapis:			st		Y+,system		; ��������� � ���
				cli
				
				ldi		cikl,8
				ldi		XL,0x70
				ldi		addr,0
				
loop9:			ld		data, X+
				rcall	eeprom_save
				dec		cikl
				brne	loop9				 	
;------------   ��� �������� ����� �������� 1 ��� -------------
				
				sei							; ���������� �� ����������
				rcall	zvuk
				ldi		temp,32			; ����� 0,25 ���
				rcall	pauza
				rcall	zvuk
				ldi		temp,32			; ����� 0,25 ���
				rcall	pauza
				rcall	zvuk
                ;------------------------------------------------------------
				ldi		temp,255			; ����� 0,25 ���
				rcall	pauza
				ldi		temp,255			; ����� 0,25 ���
				rcall 	pauza
				ldi		temp,255			; ����� 0,25 ���
				rcall	pauza
				ldi		temp,255			; ����� 0,25 ���
				rcall	pauza				
				ldi		temp,255			; ����� 0,25 ���
				rcall	pauza
				ldi		temp,255			; ����� 0,25 ���
				rcall 	pauza
				ldi		temp,255			; ����� 0,25 ���
				rcall	pauza
				ldi		temp,255			; ����� 0,25 ���
				rcall	pauza								
												
       			rjmp	klava

;-------------------------------------------------
eeprom_save:	sbic 	EECR,EEPE		; �������� ���������� EEPROM
				rjmp 	eeprom_save
				
				out 	EEARL,addr			
				out 	EEDR,data
				sbi 	EECR,EEMPE		; ��������� ���� ���������� ������
				sbi 	EECR,EEPE		; ������� ������ � EEPROM
				inc		addr			; ���������� ���������� ������	
				ret
;--------------------------------------------------
pauza:			clr		timerL
p1:				cpi		timerL, 255
				brne	p1
				dec 	temp
				brne	pauza
				ret
;-------------- �������� ������ ������������� 0,25 ������ ---------------
zvuk:			ldi		bitcnt,32
zv:				sbi		PORTB,0
				ldi		temp,1
				rcall	pauza
				cbi		PORTB,0
				ldi		temp,1
				rcall	pauza
				dec		bitcnt
				brne 	zv
				ret				 									
 ;********************************************************************
OC0Aaddrs:		in	S,SREG		; ������ �������� ������
				inc	timerL		; timer_Lo	;timerL
				out	SREG,S		; ������ � ������� ������
				reti
;********************************************************************
