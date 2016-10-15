/*
 * dosator.c
 *
 * Created: 13.10.2016 20:27:32
 * Author: Falcon
 */
#include <mega328p.h>
#include <delay.h>

#define SERVO_MAX 0xB0
#define SERVO_MIN 0x22

//������
#define UP    !PINC.4
#define DOWN  !PIND.7
#define ENTER !PINC.5

//�����
#define A4 PORTB.0
#define A2 PORTB.4
#define A3 PORTB.3
#define A1 PORTC.2
#define A0 PORTC.3

//��������
#define a PORTC.1 //����� ����
#define b PORTC.0 //����
#define c PORTD.2 //���� ����
#define d PORTD.3 //���� ���
#define e PORTD.4 //���
#define f PORTD.5 //����� ���
#define g PORTD.6 //�����

unsigned char shift, time;
unsigned int time_count;
signed char min, sec;

void data_conv(unsigned char, unsigned char);
void dig_send(unsigned char);
void putchar(unsigned char, unsigned char);

// Timer 0 overflow interrupt service routine
interrupt [TIM0_OVF] void timer0_ovf_isr(void)
{
  TCNT0=0xE8;
  shift++;
  if (shift==5) shift=0;
  time_count++;
  if (time_count>650) 
    { 
    time_count=0;
    sec--;
    } 
  A0=A1=A2=A3=A4=0;
  switch (shift) 
    {
    case 0: A0=1;
    dig_send(0);
           break;
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

float step_time;
unsigned int data, i, last_time;
eeprom unsigned char ocr=0xB0;
unsigned char total_vol, dose_steps=1;
unsigned char dig[5]; //�� ���������� �������� (������) - ����� ��� �������� ���� �������� � ������� numbers[][] - ��������, ������� �������� ����� �� ������� �� ��������������� ����� 0..4

//������ ������� - ����� ��� ����� �, ������ - ��� ����� D. ������� ��� ��� ����� D - �� ��� ����������, ��� pull-up ������ �� PD7
unsigned char numbers[][2]=
{
0b00, 0b110000, //0
0b10, 0b110111, //1
0b00, 0b101001, //2
0b00, 0b100011, //3
0b10, 0b100110, //4 
0b01, 0b100010, //5
0b01, 0b100000, //6
0b00, 0b110111, //7
0b00, 0b100000, //8
0b00, 0b100010, //9
0b10, 0b110111, //: (10) ��� ����� A0
0b11, 0b101111, //- (11) ��� �1-�4
0b11, 0b101000, //t (12)
0b10, 0b110011, //_�1 � : ��� �0 (13)
0b00, 0b110100, //_�2,3,4 � : ��� �0 (14)
0b10, 0b110110, //_�3 � : ��� �0 (15)
0b00, 0b110111, //_�4 � : ��� �0 (16)
0b00, 0b100000, //��� "_" � : ��� �0 (17)
0b11, 0b111111  //NULL (18) - �� ���� ��������� ������� �� ��������
};
      
void main(void)
{
// Timer/Counter 0,  15,625 kHz  Timer Period: 16 ms 
TCCR0B=(1<<CS02) | (1<<CS00);
TCNT0=0xF0; //������ ����� �...
// Timer/Counter 0 Interrupt(s) initialization
TIMSK0=(0<<OCIE0B) | (0<<OCIE0A) | (1<<TOIE0);

// Timer/Counter 1, 15,625 kHz Timer Period: 16,384 ms OC1A Period: 16,384 ms Compare A Match Interrupt: On
TCCR1A=(1<<COM1A1) | (1<<WGM11) | (1<<WGM10);
TCCR1B=(1<<WGM12) | (1<<CS12);
OCR1AL=ocr; //������������ � ��������, ������� ���� ����� �����������
// Timer/Counter 1 Interrupt(s) initialization
TIMSK1=(1<<OCIE1A);

DDRB=0b00111111;
DDRC=0b00001111;
DDRD=0b01111111;

PORTC=0b11110000;
PORTD=255;

min=5;
sec=0;

data_conv(18, 0); //�������������� ����� � ������ �������������
putchar(11, 1); //� �������� "-" �� ���� ��������
putchar(11, 2);
putchar(11, 3);
putchar(11, 4);

// Global enable interrupts
#asm("sei");

//��� ��������� �������� ������ � ���������� ���� �� ����
for ( ; ocr>0x20; ocr--) 
  {
  OCR1AL=ocr;
  delay_ms(20);
  }

data_conv(15, 0); //�������������� ����� � ������ �������������

//��������� �������, �� ���������� �������� ��������
while(!ENTER)
 {if (UP)              //��� �������� ������� ��������� �� 1
  {if (min<99) min++;  
   delay_ms(250);
   while(UP)           //��� ��������� - ���������� �����
    {data_conv(min, 2);
    delay_ms(100);
    if (min<99) min++;
    }
  } 
  
  if (DOWN)
  {if (min>1)  min--; 
   delay_ms(250);
   while(DOWN)
    {data_conv(min, 2);
    delay_ms(100);
    if (min>1)  min--;
    }
  }
 data_conv(min, 2);
 }
 
 data_conv(17, 0); //�������������� ����� � ��� �������������
 delay_ms(1000);
 data_conv(14, 0); //�������������� ����� � ������ ������������� 
 delay_ms(1);
 
 while(ENTER); //���� ���� �������� ������
 
 delay_ms(1);
 
 //��������� ������
 while(!ENTER)
 {if (UP)              //��� �������� ������� ��������� �� 1
  {if (ocr<SERVO_MAX) OCR1AL=++ocr;  
   delay_ms(250);
   while(UP)           //��� ��������� - ���������� �����
    {delay_ms(25);
    if (ocr<SERVO_MAX) OCR1AL=++ocr;
    }
  } 
  
  if (DOWN)
  {if (ocr>SERVO_MIN)  OCR1AL=--ocr; 
   delay_ms(250);
   while(DOWN)
    {delay_ms(25);
    if (ocr>SERVO_MIN) OCR1AL=--ocr;
    }
  }

 }
 
 for (i=0; i<7; i++)
   {
   data_conv(17, 0); //�������������� ����� � ��� �������������
   delay_ms(150);
   data_conv(18, 0); //�������������� ����� � ��� �������������
   delay_ms(50);
   }
 
 while(ENTER); //���� ���� �������� ������
 
 delay_ms(1);

data_conv(17, 0); //�������������� ����� � ��� �������������
delay_ms(500);

data_conv(sec, 1);
data_conv(10, 0); //�������������� ����� � ��� �������������

if (ocr>(SERVO_MAX-30)) OCR1AL=ocr=SERVO_MAX-30; //����� �������� ����������� � ������ �������� ���� ������

total_vol=ocr-SERVO_MIN; //����� ���������� ����� ������������
last_time=min*60; //����� ����������� ���� - ������ ��������
step_time=(float)last_time/(float)total_vol; //����� ����� ������ � �������������

sec=0;
while (1)
  {
   if (sec<0) 
     {
     sec=59; 
     min--;
     if (min<0) 
       {
       sec=0; 
       min=0;
       while(1); //��������� ��������� - ������ ������ �� ������
       }
     }
   if ((last_time - min*60 - sec) > step_time)
     {
     if (ocr>SERVO_MIN) OCR1AL=ocr=ocr-1-(int)((ocr-SERVO_MIN)/(min*60+sec));
     last_time=min*60+sec;  
     }
   data_conv(min, 2);
   data_conv(sec, 1);
   if (sec%2==0) data_conv(10, 0); //��� � ��������� ������ - ������� �������������� �����
   else data_conv(18, 0);
  }
} /////// ����� �������� ����� ///////

void data_conv(unsigned char value, unsigned char group)
{ if (group!=0)
  {
  dig[group*2-1] = value%10;
  dig[group*2]   = value/10;
  }
  else 
  dig[0] = value;
}

void putchar(unsigned char value, unsigned char place)
{
 dig[place] = value;
}

void dig_send(unsigned char place)
{
 PORTC.0=(numbers[dig[place]][0] & 0b01);
 PORTC.1=((numbers[dig[place]][0] & 0b10) >> 1);
 PORTD=(numbers[dig[place]][1] << 2); 
}