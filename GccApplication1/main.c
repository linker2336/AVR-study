/*
* GccApplication1.c
*
* Created: 2018-07-20 오후 1:16:02
* Author : linke
*/

#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#define __DELAY_BACKWARD_COMPATIBLE__		//_delay_ms에서 변수를 인자로 사용할 수 있게 한다.

#define setbit(x,y)		(x |= (1 << y))		//X포트의 Y번 비트를 1로 설정.
#define clearbit(x,y)	(x &= ~(1 << y))	//X포트의 Y번 비트를 0으로 설정.

unsigned char TIMER0_OVF_COMPENSATE;
unsigned long millis=0;



//인터럽트
void EXTI_Init(){
	sei();                  //글로벌 인터럽트 활성화
	DDRB=0b00000000;		//입력으로 세팅
	PORTB=0b11111111;		//내부풀업으로 세팅
	setbit(MCUCSR, ISC01);	//상승 엣지 검출시 ISR 활성화.
	setbit(MCUCSR, ISC00);	//상승 엣지 검출시 ISR 활성화.
	setbit(GICR, INT0);		//EXTI0 활성화.
}

ISR(INT0_vect){
	
}

ISR(BADISR_vect) {}


//Timer
void Timer_Init(uint32_t fosc){
	//Timer0 -> millis
	clearbit(TCCR0, CS02);								//64분주
	setbit(TCCR0, CS01);								//64분주
	setbit(TCCR0, CS00);								//64분주
	setbit(TIMSK, TOIE0);								//타이머 0 오버플로우 인터럽트 허용
	
	TIMER0_OVF_COMPENSATE = (uint8_t)(fosc/64000);		//16MHz 기준 250
	TCNT0 = 255-TIMER0_OVF_COMPENSATE;					//타이머가 64분주로 250를 세면 1ms를 의미한다.
}

ISR(TIMER0_OVF_vect){
	TCNT0 = 255-TIMER0_OVF_COMPENSATE;
	millis++;
}


//USART
void USART_Transmit_char(char data){				//8비트 데이터를 송신하는 함수
	while(!(UCSRA & (1<<UDRE)));					//UDRE0 비트가 1이 되면
	UDR = data;										//UDR에 데이터를 전송한다.
}

void USART_Transmit_String(char *str){
	while(*str != 0) USART_Transmit_char(*str++);	//8비트씩 끊어서 데이터를 'USART_Transmit_char'로 던진다.
	//NULL(문자열의 끝)을 만날 때 까지 반복한다.
}

void USART_Transmit_int(long value){				//음수라면 -먼저 출력 후 반전
	if ( value < 0 ) {
		USART_Transmit_String ( "-" );
		value = -value;
	}
	uint8_t length = 0;
	for ( uint8_t i=0; i<100; i++ ){				//자릿수를 length에 저장
		if (value/pow( 10 , i ) < 10 ) {
			length = i + 1;
			break;
		}
	}
	for ( uint8_t i=1; i<=length; i++) {
		uint8_t a = value/pow(10, length-i) - (uint8_t)(value/pow(10, (length-i) + 1))*10;
		USART_Transmit_char(a+0x30);				//아스키값으로 변환
	}
}

void USART_Init(uint32_t fosc, uint16_t bps){
	uint16_t temp;									//bps 계산 공간
	UCSRB = (1<<RXEN) | (1<<TXEN);
	temp = fosc/(bps*16) - 1;						//UBBR에 넣을 bps값 계산
	UBRRH = (temp>>8) & 0b11111111;					//UBRR0H에 bps값 대입
	UBRRL = temp & 0b11111111;						//UBRR0L에 bps값 대입
}


int main(void)
{
	// 	DDRB=0XFF;
	EXTI_Init();
	Timer_Init(F_CPU);
	USART_Init(F_CPU, 2400);
	while (1)
	{
		// 		PORTB = 0b00001111;
		// 		_delay_ms(1000);
		// 		PORTB = 0b11110000;
		// 		_delay_ms(1000);
		USART_Transmit_char('A');
		_delay_ms(10);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//RFM95(LORA) SPI통신코드(수정필요) **주의

//Digital INPUT.
setbit(DDR_RFM_MISO, N_RFM_MISO);

//Digital OUTPUT.
setbit(DDR_RFM_MOSI, N_RFM_MOSI);
setbit(DDR_RFM_SCK, N_RFM_SCK);
setbit(DDR_RFM_Reset, N_RFM_Reset);
setbit(DDR_RFM_CS, N_RFM_CS);

//Digital OUTPUT 초기값 High
setbit(PORT_RFM_Reset, N_RFM_Reset);
setbit(PORT_RFM_CS, N_RFM_CS);

//SPI 인터럽트 비활성화, SPI 활성화, MSB 먼저 전송, 마스터, CPOL=0, CPHA=0, 8분주(1MHz) **
SPCR = (0<<SPIE) | (1<<SPE) | (0<<DORD) | (1<<MSTR) | (0<<CPOL) | (0<<CPHA) | (0<<SPR1) | (1<<SPR0);
SPSR = (1<<SPI2X);

//RFM95 리셋
clearbit(PORT_RFM_Reset, N_RFM_Reset);   _delay_ms(10);
setbit(PORT_RFM_Reset, N_RFM_Reset);   _delay_ms(10);

//Sleep 모드
RFM_Write(RegOpMode, 0b00001000);   _delay_ms(10);
if(RFM_Read(RegOpMode) != 0b00001000) return 1;


//**
uint8_t   RFM_Write(uint8_t address, uint8_t data){
	//해당 주소에 데이터를 쓰고, 쓰기 이전 데이터를 반환한다.
	
	clearbit(PORT_RFM_CS, N_RFM_CS);   //CS 설정
	SPDR = (address | 0b10000000);      //전송 레지스터에 데이터 입력
	while(!(SPSR & (1<<SPIF)));         //전송 완료 플래그가 세워질 때 까지 대기.
	SPDR = data;                  //전송 레지스터에 데이터 입력
	while(!(SPSR & (1<<SPIF)));         //전송 완료 플래그가 세워질 때 까지 대기.
	setbit(PORT_RFM_CS, N_RFM_CS);      //CS 해제
	return SPDR;                  //전송 레지스터에 대치된 수신데이터 반환
}

uint8_t   RFM_Read(uint8_t address){
	//해당 주소의 데이터를 불러온다.
	
	clearbit(PORT_RFM_CS, N_RFM_CS);   //CS 설정
	SPDR = address;                  //전송 레지스터에 데이터 입력
	while(!(SPSR & (1<<SPIF)));         //전송 완료 플래그가 세워질 때 까지 대기.
	SPDR = 0x00;                  //전송 레지스터에 데이터 입력
	while(!(SPSR & (1<<SPIF)));         //전송 완료 플래그가 세워질 때 까지 대기.
	setbit(PORT_RFM_CS, N_RFM_CS);      //CS 해제
	return SPDR;                  //전송 레지스터에 대치된 수신데이터 반환
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "EEPROM.h"

static void eeprom_write(uint16_t _addr, uint8_t _data)
{
	while(EECR & (1<<EEPE));
	EEARH = _addr>>8;
	EEARL = _addr;
	EEDR = _data;
	EECR |= (1<<EEMPE);
	EECR |= (1<<EEPE);
}

uint8_t eeprom_read(uint16_t _addr)
{
	while(EECR & (1<<EEPE));
	EEARH = _addr>>8;
	EEARL = _addr;
	EECR |= (1<<EERE);
	return EEDR;
}

void eeprom_update(uint16_t _addr, uint8_t _data)
{
	if(_data != eeprom_read(_addr)) eeprom_write(_addr,_data);
}

void eeprom_write_i16(uint16_t _addr,int16_t _data)
{
	union{
		int16_t A;
		uint8_t a[2];
	}VAL;
	VAL.A = _data;
	//  eeprom_write(_addr,VAL.a[0]);
	//  eeprom_write(_addr + 1,VAL.a[1]);
	eeprom_update(_addr,VAL.a[0]);
	eeprom_update(_addr + 1,VAL.a[1]);
}

int16_t eeprom_read_i16(uint16_t _addr)
{
	union{
		int16_t A;
		uint8_t a[2];
	}VAL;
	VAL.a[0] = eeprom_read(_addr);
	VAL.a[1] = eeprom_read(_addr + 1);
	return VAL.A;
