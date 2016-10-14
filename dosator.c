/*
 * dosator.c
 *
 * Created: 13.10.2016 20:27:32
 * Author: Falcon
 */
#include <mega328p.h>
#include <delay.h>

#define SERVO_MAX 0xA0
#define SERVO_MIN 0x20

//аноды
#define A4 PORTB.0
#define A2 PORTB.4
#define A3 PORTB.3
#define A1 PORTC.2=PORTC.3
#define A0 PORTC.4=PORTC.5

//сегменты
#define a PORTC.1 //право верх
#define b PORTC.0 //верх
#define c PORTD.2 //лево верх
#define d PORTD.3 //лево низ
#define e PORTD.4 //низ
#define f PORTD.5 //право низ
#define g PORTD.6 //центр

unsigned char shift, time;
unsigned int time_count;
signed char min, sec;

void data_conv(unsigned char, unsigned char);
void dig_send(unsigned char);

// Timer 0 overflow interrupt service routine
interrupt [TIM0_OVF] void timer0_ovf_isr(void)
{
  TCNT0=0xE8;
  shift++;
  if (shift==5) shift=1;
  time_count++;
  if (time_count>650) 
    { 
    time_count=0;
    sec--;
    } 
  A1=A2=A3=A4=0;
  switch (shift) 
    {
    case 1: A1=1;
    dig_send(1);
           break;
    case 2: A2=1;
    dig_send(2);
           break;
    case 3: A3=1;
    dig_send(3);
           break;
    case 4: A4=1;
    dig_send(4);
           break;
    }
}

// Timer1 output compare A interrupt service routine
interrupt [TIM1_COMPA] void timer1_compa_isr(void)
{
}

unsigned int data, i, step_time, last_time;
unsigned char ocr=0x50, total_vol;
unsigned char dig[5];

unsigned char numbers[10][2]=
{
0b00, 0b10000, //0
0b10, 0b10111, //1
0b00, 0b01001, //2
0b00, 0b00011, //3
0b10, 0b00110, //4 
0b01, 0b00010, //5
0b01, 0b00000, //6
0b00, 0b10111, //7
0b00, 0b00000, //8
0b00, 0b00010  //9
};
      
void main(void)
{
// Timer/Counter 0,  15,625 kHz  Timer Period: 16 ms 
TCCR0B=(1<<CS02) | (1<<CS00);
TCNT0=0xF0; //начало счета с...
// Timer/Counter 0 Interrupt(s) initialization
TIMSK0=(0<<OCIE0B) | (0<<OCIE0A) | (1<<TOIE0);

// Timer/Counter 1, 15,625 kHz Timer Period: 16,384 ms OC1A Period: 16,384 ms Compare A Match Interrupt: On
TCCR1A=(1<<COM1A1) | (1<<WGM11) | (1<<WGM10);
TCCR1B=(1<<WGM12) | (1<<CS12);
OCR1AL=ocr; //приравниваем к значению, которое было перед отключением
// Timer/Counter 1 Interrupt(s) initialization
TIMSK1=(1<<OCIE1A);

DDRD=255;
DDRB=255;
DDRC.0=DDRC.1=DDRC.2=DDRC.3=DDRC.4=DDRC.5=1;
PORTB=0;
PORTC.0=PORTC.1=1;
PORTD=255;
OCR1AL=SERVO_MAX;
delay_ms(500);
OCR1AL=SERVO_MAX-20;

min=2;
sec=0;

// Global enable interrupts
#asm("sei");

//for (OCR1AL=0xB0; OCR1AL>0x20; OCR1AL--) 
//{
//  delay_ms(25);
//}

total_vol=SERVO_MAX-SERVO_MIN; //общее количество шагов сервопривода
last_time=min*60; //время предыдущего шага - первое значение
step_time=(float)last_time/(float)total_vol; //время между шагами в миллисекундах


while (1)
  {
   if (sec<0) 
     {
     sec=59; 
     min--;
     }
   if ((last_time - min*60 - sec) > step_time)
     {
     if (OCR1AL>SERVO_MIN) OCR1AL--;
     last_time=min*60+sec;  
     }
   data_conv(min, 1);
   data_conv(sec, 0);
   
  }
} /////// КОНЕЦ ГЛАВНОГО ЦИКЛА ///////

void data_conv(unsigned char value, unsigned char group)
{
  dig[1+group*2] = value%10;
  dig[2+group*2] = value/10;
}

void dig_send(unsigned char place)
{
 PORTC.0=(numbers[dig[place]][0] & 0b01);
 PORTC.1=((numbers[dig[place]][0] & 0b10) >> 1);
 PORTD=(numbers[dig[place]][1] << 2); 
}