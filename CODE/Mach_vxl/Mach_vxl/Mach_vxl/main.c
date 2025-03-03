#include "phong.h"

#define LED_SEG_PORT PORTA
#define LED_PORT PORTB
#define LED_SEG_DDR DDRA
#define LED_DDR DDRB
const uint8_t LED_database[11] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F, 0x40};
int LED_pos;
int value_pos[6];
int hour, minute, second;
int setHour, setMinute, setSecond;

#define KEYPAD_PORT PORTC
#define KEYPAD_PIN PINC
#define KEYPAD_DDR DDRC
int KeyPad_data[4][3]=
{
	{1,2,3},
	{4,5,6},
	{7,8,9},
	{10,0,10}
};
bool IsPause, IsSetTime;
int CheckRow, CheckCol;
int row,col;
int mark;

int Minimum(int a, int b)
{
	if(a>b)
	return b;
	else
	return a;
}

void ValuePosByTime(int hour, int minute, int second)
{
	value_pos[0]=hour/10;
	value_pos[1]=hour%10;
	value_pos[2]=minute/10;
	value_pos[3]=minute%10;
	value_pos[4]=second/10;
	value_pos[5]=second%10;
}

void UpdateTime()
{
	hour = Minimum(23, value_pos[0]*10 + value_pos[1]);
	minute = Minimum(59, value_pos[2]*10 + value_pos[3]);
	second = Minimum(59, value_pos[4]*10 + value_pos[5]);
}

void UpdateSetTime()
{
	setHour = Minimum(23, value_pos[0]*10 + value_pos[1]);
	setMinute = Minimum(59, value_pos[2]*10 + value_pos[3]);
	setSecond = Minimum(59, value_pos[4]*10 + value_pos[5]);
}

void LED_init()
{
	TCCR1B=0x04;
	TIMSK=0x04;
	TCNT1H=0x85;
	TCNT1L=0xED;
	sei();
	LED_SEG_DDR = 0xFF;
	LED_DDR = 0xFF;
	LED_SEG_PORT = 0x00;
	LED_PORT = 0xFF;
}

void KEYPAD_init()
{
	KEYPAD_DDR = 0x0F;
	KEYPAD_PORT = 0xFF;
	INT0_Init(4);
	INT1_Init(4);
	IsPause = false;
	IsSetTime = false;
}

void Start()
{
	hour = 0;minute = 0;second = 0;
	setHour = 0;setMinute = 0;setSecond = 0;
	ValuePosByTime(hour, minute, second);
	LED_init();
	KEYPAD_init();
	DDRD |= (1<<4);
}

void Display1LED(int LED_pos, uint8_t DATA)
{
	LED_PORT = 0xFF ^ (1<<(LED_pos-1));
	LED_SEG_PORT = DATA;
	if(LED_pos%2 ==0)
	{
		LED_SEG_PORT |= (1 << 7);
	}
	_delay_ms(1);
	LED_PORT = 0xFF;
	LED_SEG_PORT = 0x00;
}

void Display6LED()
{
	for(LED_pos=1; LED_pos<=6; LED_pos++)
	{
		Display1LED(LED_pos, LED_database[value_pos[LED_pos-1]]);
	}
}
void CheckKeypad()
{
	if(IsPause == true || IsSetTime == true)
	{
		for(row = 0; row<4; row++)
		{
			KEYPAD_PORT = 0xFF ^ (1 << row);
			_delay_ms(2);
			CheckRow = KEYPAD_PIN & 0xF0;
			if(CheckRow!= 0xF0)
			{
				for(col = 0; col<3; col++)
				{
					CheckCol = (0xFF ^ (1<<(col+4))) & 0xF0;
					if(CheckCol == CheckRow)
					{
						value_pos[mark] = KeyPad_data[row][col];
						if(mark == 0 && IsSetTime == true)
						{
							value_pos[1] = 10;
							value_pos[2] = 10;
							value_pos[3] = 10;
							value_pos[4] = 10;
							value_pos[5] = 10;
							if(value_pos[0]==10)
							{
								mark=-1;
							}
						}
						if(KeyPad_data[row][col] == 10 && mark > 0)
						{
							mark--;
							value_pos[mark] = KeyPad_data[row][col];
						}
						else
						{
							if(mark == 5)
							{
								if(IsPause == true)
								{
									UpdateTime();
									IsPause = false;
								}
								if(IsSetTime == true)
								{
									UpdateSetTime();
									IsSetTime = false;
								}
							}
							else
							{
								mark++;
							}
						}
					}
				}
				while((KEYPAD_PIN & 0xF0) != 0xF0);
			}
		}
	}
}

// TIMER1_OVF
ISR(TIMER1_OVF_vect)
{
	TCNT1H=0X85;
	TCNT1L=0XED;
	if(IsPause == false)
	{
		second = (second+1)%60;
		if(second == 0)
		{
			minute = (minute+1)%60;
			if(minute == 0)
			{
				hour = (hour+1)%24;
			}
		}
		if(IsSetTime == false)
		{
			ValuePosByTime(hour, minute, second);
		}
		if(hour == setHour && minute == setMinute && second == setSecond)
		{
			PORTD |= (1<<4);
		}
	}
}
// INT0_vect
ISR(INT0_vect)
{
	if(IsPause == false)
	{
		IsPause = true;
		IsSetTime = false;
		mark = 0;
		value_pos[0] = 10;
		value_pos[1] = 10;
		value_pos[2] = 10;
		value_pos[3] = 10;
		value_pos[4] = 10;
		value_pos[5] = 10;
	}
	else
	{
		IsPause = false;
		ValuePosByTime(hour, minute, second);
	}
}
// INT1_vect
ISR(INT1_vect)
{
	if(IsSetTime == false)
	{
		IsSetTime = true;
		IsPause = false;
		mark = 0;
		ValuePosByTime(setHour, setMinute, setSecond);
		if(PIND & (1 << PD4))
		{
			PORTD &= ~(1<<4);
			IsSetTime = false;
		}
	}
	else
	{
		IsSetTime = false;
		ValuePosByTime(hour, minute, second);
	}
}

int main(void)
{
	Start();
	while(1)
	{
		Display6LED();
		CheckKeypad();
	}
}