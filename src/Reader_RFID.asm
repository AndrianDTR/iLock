;##############################################
;##             Reader_RFID                  ##
;##                                          ##
;##############################################
; 8.0 MZh int osc  , без делителя на 8, BODLEVEL- 2.7v.
; * Используется внешний компаратор и усилитель на LM358
; В реализации проекта участвуют:
; PB0- bipper
; PB1- приём импульсов корпоратора(детектора сигнала rfid)
; PB2- выход сигнала 125 КГц, для запитки антены (на прерываниях по таймеру 0) 
; PD0- кнопка для включения команды чтения кода ключа RFID и занесение в ячейку EEPROM по адресу 80Н
;-------------------------------------------------

.include "tn2313def.inc"		; Присоединение файла описаний
.list							; Включение листинга

;------------------------------------------------
.cseg 							; Выбор сегмента программного кода
.org 	0						; Устанеовка текущего адреса на ноль

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

.equ	INPUT		=1			; PB1 - вход от компаратора
;-------------------------------------------------

rjmp		start
rjmp		m1	 
rjmp		m2	 
rjmp		m3	 
rjmp		m4	 
rjmp		m5 
rjmp		m6	 
rjmp		m7	 
rjmp		m8	 
rjmp		m9	 
rjmp		m10
rjmp		m11
rjmp		m12
rjmp		OC0Aaddrs
rjmp		m14	 
rjmp		m15	
rjmp		m16	
rjmp		m17	
rjmp		m19	

;-------------------------------------------------
m1:			reti
m2:			reti
m3:			reti
m4:			reti
m5:			reti
m6:			reti
m7:			reti
m8:			reti
m9:			reti
m10:		reti
m11:		reti
m12:		reti
	 
m14:		reti
m15:		reti
m16:		reti
m17:		reti
m18:		reti
m19:		reti

;-------------------------------------------------
start:		ldi 	temp,low(RAMEND)
			out 	SPL,temp

;-------------------------------------------------
			ldi		temp, 0x80		; Выключение компаратора
			out		ACSR, temp              

;-------------------------------------------------
			ldi		temp,0b11111101	; линия РВ1 на ввод остальные на вывод
			out 	DDRB,temp
			ldi		temp,0	
			out		PORTB,temp	 
			out		DDRD,temp		; Все линии порта D на ввод
			ldi		temp,255
			out		PORTD,temp		; подключение резисторов подтяжки 

;-------------------------------------------------
; Timer0 OC0A
;-------------------------------------------------
			ldi		temp,0b01000010	; 0b01000010	
			out		TCCR0A,temp
			ldi		temp,0b10000001	; 0b10000001	
			out		TCCR0B,temp		; запуск счётчика
			ldi		temp,0b00000000
			out		TCNT0,temp		; Загрузка счётного регистра
			ldi		temp,32			; деление частоты 8МГц на 32 = 250 КГц
			out		OCR0A,temp		; в регистре совпадения
			ldi		temp,1<<OCIE0A	; Разрешение прерывания по совпадению в канале А таймера 0
			out		TIMSK,temp

;-------------------------------------------------
klava:   	in      temp, PIND     	; Прог-ма ожидает нажатие клавиши 
			sbrs	temp, 0
			rjmp	sync
			rjmp	klava
;-------------------------------------------------
sync:		sei						; Общее разрешение на прерывание
			clr		bitcnt			; счётчик найденных подряд единичных битов(заголовка)
H:			sbis	PINB, INPUT		; Ожидание высокого уровня
			rjmp	H

L:			sbic	PINB,INPUT		; Ожидание низкого уровня
			rjmp	L

			clr		timerL			; чистка счётчика
korotkaja:	cpi		timerL,65		; Поиск короткой низкой площадки не более чем за 280 мл.секунд
			brsh	dlinnaya		; Уход на поиск длинной площадки
			sbis 	PINB,INPUT
			rjmp	korotkaja		; Продложение поиска
			rjmp	sync			; Найдена короткая, искать снова длинную 
				
dlinnaya:	cpi		timerL,140		; Искать в пределах: не более чем за 560 мл.секунд 
			brsh	sync			; выход за пределы по времени	
			sbic	PINB,INPUT		
			rjmp	preambula		; Синхронизация установлена, можно приступать к поиску заголовка
			rjmp	dlinnaya		

;-------------------------------------------------
preambula:	clr		timerL			; Ожидание спада за 260 мл.сек
loop1:		cpi		timerL,70
			brsh	bit1			; если за длинной площадкой находится"единица"
			sbic	PINB,INPUT
			rjmp	loop1
			rjmp	sync			; если за длинной площадкой находится "ноль" - начать сначала
			
bit1:		sbic	PINB,INPUT		; Ожидание спада импульса
			rjmp	bit1			
				
			clr		timerL			; очистка счётчика
loop2:		cpi		timerL,70		; Ожидание подъёма импульса в течении 280 мл.секунд
			brsh	dl				; Уход на поиск длинной площадки
			sbis 	PINB,INPUT
			rjmp	loop2			
			inc		bitcnt			; увеличить счётчик
			cpi		bitcnt,9
			breq	massiv			; При успешном нахождении заголовка
			rjmp	bit1			; Повтор поиска
dl:			clr		bitcnt			
			rjmp	dlinnaya		; При не удачном поиске

;-------------------------------------------------
massiv:		clr		XH
			ldi		XL,0x60
			ldi		cikl,60

loop3:		clr		timerL
vysok:		cpi		timerL,70
			brsh	met11
			sbic	PINB,INPUT
			rjmp	vysok
			ldi		parity,2
			st		X+,parity
			rjmp	nizki
met11:		cpi		timerL,130
vyh:		brsh	sync
			sbic	PINB,INPUT
			rjmp	met11
			ldi		parity,3
			st		X+,parity
nizki:		clr		timerL
loop4:		cpi		timerL,70
			brsh	met10
			sbis	PINB,INPUT
			rjmp	loop4
			ldi		parity,1
			st		X+,parity
			dec		cikl
			brne	loop3
			rjmp	decodir
met10:		cpi		timerL,130
			brsh	vyh
			sbis	PINB,INPUT
			rjmp	met10
			clr		parity
			st		X+,parity
			dec		cikl
			brne	loop3

;-------------------------------------------------
decodir:	cli
			clr		temp
			ldi		YL,0xc5	
			ldi		bitcnt,128
			clr		system
			ldi		timerL,2
			ldi		XL,0x60

loop5:		ld		data,X+
			cpi		data,0			; Фильтрация
			breq	byt0_0
			cpi		data,1
			breq	byt0
			cpi		data,2
			breq	byt1
			cpi		data,3
			breq	byt1_1
			
byt0_0:		clc	
			rol		system			; Заталкивание "0"
			sec
			rol		temp			; включить контроль
byt0:		clc		
			rol		system			; Заталкивание "0"
			sec
			rol		temp			; включить контроль
			brhs	contin			; и проверку на переполнение тетрады
			dec		bitcnt			; уменьшить общий счётчик 
			breq	decod2			; в случае завершения или
			rjmp	loop5			; повторять до окончания

byt1_1:		sec
			rol		system			; Заталкивание "1"
			sec						
			rol		temp			; включить контроль
byt1:		sec
			rol		system			; Заталкивание "1"
			sec
			rol		temp			; включить контроль
			brhs	contin			; и проверку на переполнение тетрады
			dec		bitcnt			; уменьшить общий счётчик 
			breq	decod2			; в случаи завершения 
			rjmp	loop5			; повторять до окончания

contin:		dec		timerL			; Принадлежность к тетраде
			breq	con1
			push	system			; временно сохранить
			andi	system,0x1E		; выделить только 4 старших бита
			ror		system			; сдвинуть на место младшей
			mov		parity,system	; и поместить в data
			swap	parity			; сделать старшей частью
			pop		system			; востановить 5 бит	
			andi	system,0x01		; выделить только 1й
			ldi		temp,1			; зарегистрировать наличие этого бита
			dec		bitcnt			; уменьшить общий счётчик 
			breq	decod2			; в случаи завершения
			rjmp	loop5

con1:		push	system			; временно сохранить
			andi	system,0x1E		; выделить только старшие 4 бита
			ror		system			; сдвинуть на место младшей
			or		parity,system	; объединить со старшей предыдущей
			st		Y+,parity		; сохранить в памяти сформированый байт	
			pop		system			; востановить последние 5 бит
			andi	system,0x01		; выделить только младший бит 
			ldi		temp,1      	; зарегистрировать наличие этого бита 
			ldi		timerL,2		; назначить новый 2х тактный цикл
			dec		bitcnt			; уменьшить общий счётчик 
			breq	decod2			; в случаи завершения
			rjmp    loop5 			; перейти на следующий бит	 

;-------------------------------------------------
decod2:		ldi		cikl,15
			ldi		YL,0x60
			ldi		XL,0xc5
loop6:		clr		system
			ldi		timerL,2

bi7:		ld		data,X+
			sbrc	data,7
			rjmp	bi7_1
			clc
			rol		system
			rjmp	bi5

bi7_1:		sec
			rol 	system
bi5:		sbrc	data,5
			rjmp	bi5_1
			clc
			rol		system
			rjmp	bi3

bi5_1:		sec
			rol 	system	

bi3:		sbrc	data,3
			rjmp	bi3_1
			clc
			rol		system
			rjmp	bi1

bi3_1:		sec
			rol 	system

bi1:		sbrc	data,1
			rjmp	bi1_1
			clc
			rol		system
			rjmp	bi

bi1_1:		sec
			rol 	system

bi:			dec		cikl
			breq	bi_1
			dec		timerL
			brne	bi7
			st		Y+,system
			rjmp	loop6
bi_1:		st		Y+,system

;-------------------------------------------------
decod3:		ldi		YL,0x70			; Нач.адрес приёмника 
			ldi		XL,0x60			; Нач.адрес источника
			ldi		cikl,6			; цикл из 6ти байтов + 2 предыдущих =8
			ser		data			; выслать код заголовка preambula 8 бит
			st		Y+,data			; в ОЗУ
			
			ld		data,X+			; Из 1го байта 
			ldi		system,0xFE		
			and		system,data		; выделить 7 старших
			ror		system
			ldi		temp,128
			or		system,temp     ; и сложить с 9тым битом заголовка и сохранить уже второй байт 

loop7:		st		Y+,system		; сохранить в ОЗУ
			ld		system,X+		; подготовить следующий байт 
			andi	data,0x01		; выделить младший бит предыдущего байта
			swap	data			; поставить на место 7го разряда
			rol		data
			rol		data
			rol		data
			ldi		temp,0xFE
			push	system			; временно сохранить для дальнейшего использования
			and		system,temp		; выделить в текущем байте 7 старших бит
			ror		system			; и сдвинуть в младшую часть
			or		system,data		; объединить предыдущий с текущим
			pop		data			; востановить текущий полностью
			dec		cikl			; уменьшить счётчик и проверить 
			breq	zapis			; если все 6ть обработаны выход
			rjmp	loop7			; либо продолжить
			
;-------------------------------------------------
zapis:		st		Y+,system		; сохранить в ОЗУ
			cli
			
			ldi		cikl,8
			ldi		XL,0x70
			ldi		addr,0
			
loop9:		ld		data, X+
			rcall	eeprom_save
			dec		cikl
			brne	loop9

;-------------------------------------------------
; Три коротких гудка частотой 1 КГц
			sei						; разрешение на прерывание
			rcall	zvuk
			ldi		temp,32			; пауза 0,25 сек
			rcall	pauza
			rcall	zvuk
			ldi		temp,32			; пауза 0,25 сек
			rcall	pauza
			rcall	zvuk

;-------------------------------------------------
			ldi		temp,255		; пауза 0,25 сек
			rcall	pauza
			ldi		temp,255		; пауза 0,25 сек
			rcall 	pauza
			ldi		temp,255		; пауза 0,25 сек
			rcall	pauza
			ldi		temp,255		; пауза 0,25 сек
			rcall	pauza				
			ldi		temp,255		; пауза 0,25 сек
			rcall	pauza
			ldi		temp,255		; пауза 0,25 сек
			rcall 	pauza
			ldi		temp,255		; пауза 0,25 сек
			rcall	pauza
			ldi		temp,255		; пауза 0,25 сек
			rcall	pauza

			rjmp	klava

;-------------------------------------------------
eeprom_save:sbic 	EECR,EEPE		; Ожидание готовности EEPROM
			rjmp 	eeprom_save
			
			out 	EEARL,addr			
			out 	EEDR,data
			sbi 	EECR,EEMPE		; Установка бита разрешения записи
			sbi 	EECR,EEPE		; команда записи в EEPROM
			inc		addr			; подготовка следующего адреса	
			ret

;-------------------------------------------------
pauza:		clr		timerL
p1:			cpi		timerL, 255
			brne	p1
			dec 	temp
			brne	pauza
			ret

;-------------------------------------------------
; звуковой сигнал длительностью 0,25 секунд
zvuk:		ldi		bitcnt,32
zv:			sbi		PORTB,0
			ldi		temp,1
			rcall	pauza
			cbi		PORTB,0
			ldi		temp,1
			rcall	pauza
			dec		bitcnt
			brne 	zv
			ret

;-------------------------------------------------
OC0Aaddrs:	in		S,SREG			; Чтение регистра флагов
			inc		timerL			; increment timerL
			out		SREG,S			; Запись в регистр Флагов
			reti
;-------------------------------------------------
;-------------------------------------------------
