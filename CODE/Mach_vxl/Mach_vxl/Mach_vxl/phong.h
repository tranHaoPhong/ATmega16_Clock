#ifndef PHONG_H_
#define PHONG_H_
#define F_CPU 8000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <avr/interrupt.h>
#include <stdbool.h>

uint8_t ADC_VREF_TYPE;
//Lưu ý Cho phép ngắt toàn cục bằng hàm sau:
//sei();
//-------------------------------------------------------------------------------------------------------------------------------
//Các hàm liên quan đến Ngắt ngoài
//Hằm ngắt có dạng ISR(INT0_vect), ISR(INT1_vect), ISR(INT2_vect)
void INT0_Init(int Type)
{
	DDRD &= ~(1 << PD2);   // Cấu hình PD2(INT0) là input
	PORTD |= (1 << PD2);   // Enable pull-up trên PD2(INT0)
	GICR |= (1 << INT0);   // Bật ngắt INT0
	GIFR |= (1 << INTF0);
	switch(Type)
	{
		case 1: MCUCR |= (0 << ISC01) | (0 << ISC00);break; //The low level of INT0 generates an interrupt request
		case 2: MCUCR |= (0 << ISC01) | (1 << ISC00);break;	//Any logical change on INT0 generates an interrupt request
		case 3: MCUCR |= (1 << ISC01) | (0 << ISC00);break; //The falling edge of INT0 generates an interrupt request
		case 4: MCUCR |= (1 << ISC01) | (1 << ISC00);break; //The rising edge of INT0 generates an interrupt request
	}
}
void INT1_Init(int Type)
{
	DDRD &= ~(1 << PD3);   // Cấu hình PD3(INT1) là input
	PORTD |= (1 << PD3);   // Enable pull-up trên PD3(INT1)
	GICR |= (1 << INT1);	// Bật ngắt INT1
	GIFR |= (1 << INTF1);
	switch(Type)
	{
		case 1: MCUCR |= (0 << ISC11) | (0 << ISC10);break;	//The low level of INT1 generates an interrupt request
		case 2: MCUCR |= (0 << ISC11) | (1 << ISC10);break;	//Any logical change on INT1 generates an interrupt request
		case 3: MCUCR |= (1 << ISC11) | (0 << ISC10);break; //The falling edge of INT1 generates an interrupt request
		case 4: MCUCR |= (1 << ISC11) | (1 << ISC10);break; //The rising edge of INT1 generates an interrupt request
	}
}
void INT2_Init(int Type)
{
	DDRB &= ~(1 << PB2);   // Cấu hình PB2(INT2) là input
	PORTB |= (1 << PB2);   // Enable pull-up trên PB2(INT2)
	GICR |= (1 << INT2);   // Bật ngắt INT2
	GIFR |= (1 << INTF2);
	switch(Type)
	{
		case 1: MCUCSR |= (1 << ISC2);break;	//The rising edge on INT2 activates the interrupt
		case 2: MCUCSR |= (0 << ISC2);break;	//The falling edge on INT2 activates the interrupt
	}
}
//----------------------------------------------------------------------------------------------------------------
//Các hàm liên quan đến bộ đọc ADC
void ADC_Init(int refs1, int refs0, int adlar)
{
	// Khi REFS1=0 và REFS0=0, nguồn tham chiếu được chọn là AVCC với điện áp tối đa là 5V.
	// Khi REFS1=0 và REFS0=1, nguồn tham chiếu được chọn là VCC (điện áp nguồn của vi điều khiển)
	// Khi REFS1=1 và REFS0=1, nguồn tham chiếu được chọn là nội bộ 2.56V của vi điều khiển.
	
	//(0 << REFS1)|(0 << REFS0) AREF, Internal Vref turned off
	//(0 << REFS1)|(1 << REFS0) AVCC with external capacitor at AREF pin
	//(1 << REFS1)|(0 << REFS0) Reserved
	//(1 << REFS1)|(1 << REFS0) Internal 2.56V Voltage Reference with external capacitor at AREF pin
	ADC_VREF_TYPE = (refs1<<REFS1) | (refs0<<REFS0) | (adlar<<ADLAR);
	ADMUX = ADC_VREF_TYPE;
	ADCSRA=(1<<ADEN) | (0<<ADSC) | (0<<ADATE) | (0<<ADIF) | (0<<ADIE) | (0<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);
}
unsigned int read_adc(unsigned char adc_input)
{
	ADMUX = adc_input | ADC_VREF_TYPE;
	
	ADCSRA|=(1<<ADSC);
	while ((ADCSRA & (1<<ADIF))==0);
	ADCSRA|=(1<<ADIF);
	return ADCW;
}
//-----------------------------------------------------------------------------------------------------------------------------
//Các hàm liên quan đến ngắt Timer
//Hàm ngắt Timer có dạng ISR(TIMER0_COMP_vect), ISR(TIMER1_OVF_vect)
void TIMER0_COMP_Init(unsigned int time_ms) 
{
	// Tính toán giá trị OCR0 (giá trị so sánh) dựa trên thời gian cần tạo ra ngắt
	unsigned int prescaler = 8; // Chọn prescaler là 8
	unsigned int counts = (F_CPU / 1000) * time_ms / prescaler;

	// Cài đặt thanh ghi TCCR0 (Timer/Counter Control Register)
	TCCR0 |= (1 << WGM01); // Cài đặt chế độ CTC (Clear Timer on Compare Match)
	TCCR0 |= (1 << CS01); // Chia tỷ lệ 8, sử dụng prescaler 8

	// Cài đặt giá trị so sánh vào thanh ghi OCR0
	OCR0 = counts - 1;

	// Bật ngắt so sánh của Timer/Counter0
	TIMSK |= (1 << OCIE0);

	// Cho phép ngắt toàn cục
	sei();
}
void TIMER0_OVF_Init(unsigned int time_ms) 
{
	// Tính toán số ticks cần cho thời gian tràn số tương ứng với thời gian (time_ms)
	unsigned int prescaler = 8; // Chọn prescaler là 8
	unsigned int ticks = (F_CPU / 1000) * time_ms / prescaler;

	// Tính toán giá trị cho thanh ghi TCNT0
	unsigned char initialValue = 256 - ticks;

	// Cài đặt thanh ghi TCCR0 (Timer/Counter Control Register)
	TCCR0 |= (1 << CS01); // Chia tỷ lệ 8, sử dụng prescaler 8

	// Cài đặt giá trị ban đầu cho thanh ghi TCNT0
	TCNT0 = initialValue;

	// Bật ngắt tràn số của Timer/Counter0
	TIMSK |= (1 << TOIE0);

	// Cho phép ngắt toàn cục
	sei();
}
void TIMER1_COMP_Init(unsigned int time_ms, int prescaler) 
{
	// Tính toán số lượng ticks cần cho thời gian mong muốn
	unsigned long ticks = (F_CPU / 1000) * time_ms / prescaler - 1;

	// Disable interrupts
	cli();

	// Cài đặt thanh ghi so sánh (OCR1A)
	OCR1A = (unsigned int)ticks;

	// Cài đặt chế độ CTC (Clear Timer on Compare Match)
	TCCR1B |= (1 << WGM12);

	// Cài đặt bộ chia tần số (prescaler)
	if (prescaler == 1)
	TCCR1B |= (1 << CS10);
	else if (prescaler == 8)
	TCCR1B |= (1 << CS11);
	else if (prescaler == 64)
	TCCR1B |= (1 << CS11) | (1 << CS10);
	else if (prescaler == 256)
	TCCR1B |= (1 << CS12);
	else if (prescaler == 1024)
	TCCR1B |= (1 << CS12) | (1 << CS10);
	//... tiếp tục với các giá trị prescaler khác nếu cần

	// Cho phép ngắt so sánh timer 1 (TIMSK)
	TIMSK |= (1 << OCIE1A);

	// Cho phép ngắt toàn cục
	sei();
}
void TIMER1_OVF_Init(unsigned int time_ms) {
	// Tính toán giá trị tối ưu cho thanh ghi TCNT1 dựa trên thời gian nhấp nháy (ms)
	unsigned int compare_value = (time_ms * F_CPU) / (1000UL * 8); // Chia cho 8 vì sử dụng chế độ chia tần 8

	// Thiết lập chế độ đếm và chế độ chia tần
	TCCR1B = 0x04;//TCCR1B |= (1 << CS11); Chế độ chia tần 8
	
	// Đặt giá trị đếm đến khi phát sinh ngắt
	TCNT1H = (compare_value >> 8); // Gán byte cao của compare_value vào thanh ghi TCNT1H
	TCNT1L = (compare_value & 0xFF); // Gán byte thấp của compare_value vào thanh ghi TCNT1L
	
	// Bật ngắt cho Timer 1 OVF
	TIMSK = 0x04;//TIMSK |= (1 << TOIE1); Ngắt Timer 1 OVF
	
	// Bật ngắt toàn cục
	sei();
}

//--------------------------------------------------------------------------------------------------------------------
//Các hàm liên quan đến truyền UART
//Hằm ngắt nhận có dạng ISR(USART_RXC_vect)
void UART_init(int BAUD_RATE)
{
	// Tính toán giá trị UBRR (USART Baud Rate Register)
	uint16_t ubrr = (uint16_t)(F_CPU / 16 / BAUD_RATE - 1);
	
	// Cài đặt tốc độ baud và chế độ 8-bit, không parity, 1-bit stop
	UBRRH = (uint8_t)(ubrr >> 8);
	UBRRL = (uint8_t)ubrr;
	UCSRB = (1 << RXEN) | (1 << RXCIE) | (1 << TXEN); // Enable RX and RX Complete Interrupt
	UCSRC = (1 << URSEL) | (1 << UCSZ1) | (1 << UCSZ0); // Khung truyền: 8-bit dữ liệu, không parity, 1-bit stop
	
}

void UART_Transmit(uint8_t data)
{
	while (!(UCSRA & (1 << UDRE))); // Chờ cho thanh ghi truyền (USART Data Register) trống
	UDR = data; // Ghi dữ liệu vào thanh ghi truyền
	//_delay_ms(100);
}
void UART_TransmitString(const char* str)
{
	while (*str)
	{
		UART_Transmit(*str++);
	}
	UART_Transmit('\r');  // Gửi ký tự xuống dòng
	UART_Transmit('\n');
}
void UART_TransmitNumber(int DATA)
{
	if(DATA == 0)
	UART_Transmit('0');
	else
	{
		int num;
		if(DATA < 0)
		{
			UART_Transmit('-');
			num = - DATA;
		}
		else
		num = DATA;
		char data[10];
		int index = 0;
		int MaxIndex = 0;
		while(num!=0)
		{
			data[index] = '0' + num%10;
			num = num/10;
			MaxIndex = index;
			index++;
		}
		for(index = MaxIndex; index>=0;index--)
		UART_Transmit(data[index]);
	}
	UART_Transmit('\r');  // Gửi ký tự xuống dòng
	UART_Transmit('\n');
}
unsigned char UART_receive()
{
	while (!(UCSRA & (1 << RXC)));
	return UDR;
}
#endif /* PHONG_H_ */