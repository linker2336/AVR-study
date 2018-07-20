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

int main(void)
{
	// 	DDRB=0XFF;
	EXTI_Init();
	Timer_Init(F_CPU);
	while (1)
	{
		// 		PORTB = 0b00001111;
		// 		_delay_ms(1000);
		// 		PORTB = 0b11110000;
		// 		_delay_ms(1000);
		
	}
}

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









//Fusebit L = 0xFF, H = 0xD1 (외부클럭(16MHz), EEPROMSAVE)
//형식지정자 : (작음) %d, %ld, %lld, %lu (큼), (실수형->) %f, %lf, %Lf
/*********************************************************************************/


#include <util/delay.h>      //_delay_us(), _delay_ms() 등이 포함된 헤더.
#include <math.h>
#include <time.h>

void USART_Transmit_char(char data){         //8비트 데이터를 송신하는 함수
	while(!(UCSR0A & (1<<UDRE0)));            //UDRE0 비트가 1이 되면
	_delay_us(1);                        //글자깨짐 방지로 잠깐 기다리고
	UDR0 = data;                        //UDR에 데이터를 전송한다.
}

void USART_Transmit_String(char *str){
	while(*str != 0) USART_Transmit_char(*str++);   //8비트씩 끊어서 데이터를 송신한다.
	//NULL(문자열의 끝)을 만날 때 까지 반복한다.
}

void USART_Transmit_int(long value){
	if ( value < 0 ) {
		USART_Transmit_String ( "-" );
		value = - value;
	}
	uint8_t length = 0;
	for ( uint8_t i = 0 ; i < 100 ; i ++ ) {
		if ( value / pow ( 10 , i ) < 10 ) {
			length = i + 1;
			break;
		}
	}
	for ( uint8_t i = 1 ; i <= length ; i ++ ) {
		uint8_t a = value / pow ( 10 , length - i ) - ( uint8_t ) ( value / pow ( 10 , ( length - i ) + 1 ) ) * 10;
		USART_Transmit_char ( a + 0x30 );
	}
}

void USART_Init(uint32_t fosc, uint16_t bps){
	uint16_t temp;               //bps 계산 공간
	UCSR0B = (1<<RXEN0) | (1<<TXEN0);
	temp = fosc / (bps * 16) - 1;      //UBBR에 넣을 bps값 계산
	UBRR0H = (temp>>8) & 0b11111111;   //UBRR0H에 bps값 대입
	UBRR0L = temp & 0b11111111;         //UBRR0L에 bps값 대입
}