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