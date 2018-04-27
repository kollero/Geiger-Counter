
//version 3 code, needs fine tuning to run in v4

/*
 * Geiger Counter v3b
 * Copyright (c) 2017
 * Author: Panu Leinonen
 * kollero@hotmail.com
 *
 */



#define TRUE  1
#define FALSE 0

#define OVERSAMPLING 3
#define I_memory 40 //samples

#define P_val 10 
#define I_val 3 
#define D_val 6 

#include <stdlib.h>
#include <math.h>
//#include <string.h>
#include <stdio.h>

#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdbool.h> 

//single sbm-20
//double tube = 0.344827586207; 
//dual sbm-20
double tube = 0.172413793104;

#include "u8g.h"
u8g_t u8g;

typedef struct{
	unsigned int bit0:1;
	unsigned int bit1:1;
	unsigned int bit2:1;
	unsigned int bit3:1;
	unsigned int bit4:1;
	unsigned int bit5:1;
	unsigned int bit6:1;
	unsigned int bit7:1;
} _io_reg;
#define REGISTER_BIT(rg,bt) ((volatile _io_reg*)&rg)->bit##bt

#define	SOUND_PORT REGISTER_BIT(PORTD,4)
#define BUZon SOUND_PORT = 1
#define BUZoff SOUND_PORT = 0

#define	LED_PORT REGISTER_BIT(PORTD,5)
#define LEDon LED_PORT = 1
#define LEDoff LED_PORT = 0

// reverses a string 'str' of length 'len'
void reverse(char *str, int len)
{
	int i=0, j=len-1, temp;
	while (i<j)
	{
		temp = str[i];
		str[i] = str[j];
		str[j] = temp;
		i++; j--;
	}
}


/*
// Converts a given integer x to string str[].  d is the number
// of digits required in output. If d is more than the number
// of digits in x, then 0s are added at the beginning.
int intToStr(int x, char str[], int d)
{
	int i = 0;
	while (x)
	{
		str[i++] = (x%10) + '0';
		x = x/10;
	}
	
	// If number of digits required is more, then
	// add 0s at the beginning
	while (i < d)
	str[i++] = '0';
	reverse(str, i);	
	str[i] = '\0';
	return i;
}*/


int  intToStr(int x, char str[], int  d)
{
	int  i = 0;
	while (x)
	{
		str[i++] = (x%10) + '0';
		x = x/10;
	}
	
	// If number of digits required is more, then
	// add 0s at the beginning
	while (i < d)
	str[i++] = '0';
	reverse(str, i);
	str[i] = '\0';
	return i;
}



// Converts a floating point number to string.
void ftoad(long double n, char *res, int afterpoint)
{
	// Extract integer part
	int  ipart = (long double)n*1000;
	
	// Extract floating part
	long double fpart =(long double) ((long double)(n*1000) - (long double)ipart);
	
	// convert integer part to string
	//int i = intToStr(ipart, res, 0);
	uint16_t  i = intToStr(ipart, res, 1); //forces to show 0 at the beginning

	// check for display option after point
	if (afterpoint != 0)
	{
		res[i] = '.';  // add dot
		
		// Get the value of fraction part upto given no.
		// of points after dot. The third parameter is needed
		// to handle cases like 233.007
		fpart = fpart* pow(10, afterpoint);
		
		intToStr((int)fpart, res + i + 1, afterpoint);
	}
}

// Converts a floating point number to string.
void ftoa(float n, char *res, int afterpoint)
{
	// Extract integer part
	int ipart = (int)n;
	
	// Extract floating part
	float fpart =(float) (n - (float)ipart);
	
	// convert integer part to string

	//int i = intToStr(ipart, res, 0);

	int i = intToStr(ipart, res, 1); //forces to show 0 at the beginning

	// check for display option after point
	if (afterpoint != 0)
	{
		res[i] = '.';  // add dot
		
		// Get the value of fraction part upto given no.
		// of points after dot. The third parameter is needed
		// to handle cases like 233.007
		fpart = fpart * pow(10, afterpoint);
		
		intToStr((int)fpart, res + i + 1, afterpoint);
	}
}

void delay_us(int us){
	for (int i=0; i < us; i++){
		_delay_us(1);
	}
}

void delay_ms(int ms){
	for (int i=0; i < ms; i++){
		_delay_ms(1);
	}
}	
	
	/*
	//moving funcion
	//changed
	int kk=0;
	for(int jj=0, jj<i,jj++)
	{
		if(res[jj]='\0')
		{
			kk=jj;
			break;
		}
		
	}
	
	
	for(int ii=i, ii>=0,ii--)
	{
		res[i]=res[ii]
	}
	*/

void LCD_clear(void)
{	
	u8g_SetDefaultBackgroundColor(&u8g);
	u8g_DrawBox(&u8g,0,0,132,64); //clears the screen
	u8g_SetDefaultForegroundColor(&u8g);
	u8g_SetFont(&u8g, u8g_font_6x10);
	//u8g_DrawBox(&u8g, 5,10,20,10);
	
}



void battery_full_marking(void)
{
	u8g_SetFont(&u8g, u8g_font_7x13);
	u8g_DrawBox(&u8g,68,5,12,7);      //up to 93 on the right
	u8g_DrawBox(&u8g,66,7,2,3);
}

void battery_half_marking(void)
{
	u8g_SetFont(&u8g, u8g_font_7x13);
	u8g_DrawFrame(&u8g,68,5,12,7);      //up to 93 on the right
	//u8g_drawTriangle(75,12, 70,12, 70,5);
	u8g_DrawBox(&u8g,68,5,6,7);
	u8g_DrawBox(&u8g,66,7,2,3); 
	
}


void battery_low_marking(void)
{
	u8g_SetFont(&u8g, u8g_font_7x13);
	u8g_DrawFrame(&u8g,68,5,12,7);      //up to 93 on the right
	u8g_DrawBox(&u8g,66,7,2,3);
}

void slidevalue(float naas)
{
	float xx=(float)(naas*6.4); //with scale of 0:126, 0 to 20usv/h(high danger, should relocate), 10 at the mid which is danger level!!
	if(xx >= 20)
	{
		xx=126;
	}
	int x =(int) xx;
	u8g_DrawFrame(&u8g,0,20,127,6);
	u8g_DrawBox(&u8g,1,21,x,4);
}

uint32_t totalinmin[60];
double actual_cps3=0,
usv3=0;

uint32_t totalinmin2[30];


uint16_t xx=120;
uint16_t yy=35;
uint16_t y_start=14;

void histogram(){
	
	
	
	//uint16_t total_max=1;
	uint16_t the_value=0;
	//total_max=totalinmin2[0];
	//for(int k = 0; k <= 29; k++){
	//	if(totalinmin2[k] > total_max){
	//		total_max=totalinmin2[k];	
	//	} 	
	//}
	//if(total_max< 1){
	//	total_max=1;
	//}

	u8g_UndoRotation(&u8g);
	for(int k = 0; k <= 29; k++){
			
			actual_cps3=(double)((totalinmin2[k]*1000)/(1000-(totalinmin2[k]*0.190)));
			usv3=(double)(tube*actual_cps3);
			
			the_value=yy*usv3/50;
			if(the_value>yy){
				the_value=yy;
			}
			
			//the_value=(yy*totalinmin2[k])/total_max;
			
			if(the_value > 0){
				u8g_DrawBox(&u8g,124-2*k,y_start,2,the_value);
				//intToStr(the_value, bah, 2); //seconds
				//u8g_DrawStr(&u8g, k, 30, bah);
			}
			//else if(the_value==0){
			//	u8g_SetColorIndex(&u8g, 0);
			//	u8g_DrawBox(&u8g,127-2*k,y_start,2,yy);
			//	u8g_SetColorIndex(&u8g, 1);
					
			//}	
			
	}
	u8g_SetColorIndex(&u8g, 0);
	u8g_DrawBox(&u8g,66,y_start,2,yy); //6
	u8g_SetColorIndex(&u8g, 1);
	u8g_SetRot180(&u8g); //flip screen, if required
	
	//char bah[10];	
	//intToStr(the_value, bah, 2); //seconds
	//u8g_DrawStr(&u8g, 1, 30, bah);
	
	
}



//#include "SH1106_SPI.h"

/************************************************************************************************************************************************/
/* Global Objects                                                       																		*/
/************************************************************************************************************************************************/
volatile int32_t    mean_I_err[I_memory],
					PID=0,					
					err=0,
					err_old=0,
					mean_I_error=0.0,
					I_err=0.0,
					P_err=0,
					D_err=0;


volatile uint32_t  timer_1s=0,		//these are test counters
				   timer_1min=0,
				   timer_1h=0,
				   battery_voltage_lvl= 0,
				   geiger_pulse=0,
				   geiger_pulses=0,
				   geiger_cps=0,
				   geiger_cps2=0;
				   
volatile int16_t   duty=0, //initial number
				   testing_min=0,
				   testing=0,	
				   fb_voltage=0,	
				   duty_max=600; 			  //maximum duty cycle/ICR1	39%	  
				  				   
volatile uint16_t  timer_seconds=0,	//these house the real time values from when the device was started
				   timer_minutes=0,
				   timer_hours=0,
				   totalnum=0,
				   totalnum2=0,
				   volt_value=0,
				   duty_change_counter=0;
				   
uint8_t now2=0;
				   
volatile bool		 first_min = 0,
					now=0;				   
//const uint32_t MILLION=1000000;

float    		   bat=0.0;
				   
				   
double				actual_cps=0.0,
					actual_cps2=0.0,	
					HV_vol=0.0,
					right_voltage=400.0,  //~400v calculated output from resistors remember to change to right value	
					dosage=0.0,	
					dosage_1=0.0,
					total_dose=0.0,
					dosage_total=0.0;
					
					
					
long double 		usv=0.0,
usv_1=0.0;					
			   
bool			   cmd_pulse=0,
					setnewhv=0,
				   //buzzer=0,
				   led=0,
				   batterylevelcheck=0,
				   timernow=0,
				   dos_trig_2=0,
				   dos_trig=0;


				 
void u8g_setup(void)
{
	
	
	// SCK: PORTB, Bit 5 --> PN(1,5)
	// MOSI: PORTB, Bit 3 --> PN(1,3)
	// CS: PORTD, Bit 6 --> PN(3,6)
	// A0: PORTD, Bit 7 --> PN(3,7) //DC!!
	// RST: PB0,  --> PN(1,0)
	//    Arguments for u8g_InitHWSPI are: CS, A0, Reset
	
	
	//u8g_InitHWSPI(&u8g, &u8g_dev_ssd1306_128x64_hw_spi, U8G_PIN_NONE,PN(3,6), PN(1,0));
	u8g_InitHWSPI(&u8g, &u8g_dev_sh1106_128x64_hw_spi,U8G_PIN_NONE,PN(3,7), PN(1,0));
	u8g_SetRot180(&u8g); //flip screen, if required
	//u8g_SetColorIndex(&u8g, 1); 
		
}

void system_setup() {
	
DDRC = 0b00000000;
PORTC = 0b00000000; //output 0 and input tri-state

//pd2=detection2(in), pd3=detection1(in), pd4= speaker(out), pd5=led(output), pd7=D/C(out)
DDRD = 0b10110000; //0b11110000;
PORTD = 0b00000000; //output 0 and input tri-state

//pb0=resetdisplay(out), pb1 = pwm(out), pb3=MOSI(out), pb5=sck(out)
DDRB = 0b00101011;
PORTB = 0b00000000; //output 0 and input tri-state


/* reminder
adc7= HV-feedback(33k/10033k,binary value 110011000=408=400v)
adc6=battery vref(voltage level, 1.742v@adc=9v(2.4k/12.4k), 10bit adc) (1.742/3.3)*1024=540
ADMUX selects adc7=0111 and adc6=0110;
*/

//ADMUX adlar=0, mux 0111 HV-feedback, mux 0110 battery voltage feedback
ADMUX=0b01000111;
//autotriggering disabled starts conversion right away, since 1st ADC-value is usually bad
ADCSRA=0b11000111; //with 128 clk division from 16MHz is 125kHz

//PWM
//fast PWM, non-inverting mode set, 16bit timer
ICR1=800; //@25kHz 
OCR1A = 0; //initial duty cycle
TCCR1A = 0b10000010; //ICR1 selected as top value
TCCR1B = 0b00011001; //falling edge capture, clk/1 prescaling selected clk/ICR1 with this works at higher voltage gains
TIMSK1 = 0b00000010; //compare match A interrupts
//TCCR1B = 0b00011101; //falling edge capture, clk/1024*1024

//external interrupts 
EICRA=0b00001010; //falling edges on int1 (pd3) and int0 (pd2)
EIMSK=0b00000011; //mask register
EIFR=0b00000011; //interrupt flag


//timer control 8 bit
//output is 20MHz/(256*125)=1.6ms tick
OCR2A = 124; //runs to value 0:124, to get exact ms
TCCR2A = 0b00000010; //CTC operation mode
TCCR2B = 0b10000110; // force compare match A, prescaler to 256
TIMSK2= 0b00000010; //masked compare match A 

//chirp on 0
//OCR0A = 0; //runs to value 0
//TCCR0A = 0b10000011; //max 256
//TCCR0B = 0b10000010; // 64 prescaler
//TIMSK0= 0b00000010; //

}


int main(void){

delay_ms(300);
for (int i=0;i< I_memory-1;i++){
	mean_I_err[i]=(int32_t)0.0;
}


delay_ms(200);
system_setup();
u8g_setup();

//Hardware spi enabled, clk/16 mode, master mode, LMP8358 will require min 200ns for clk period
//SPCR=0b01010000; //0b01010001; //rising edge sampling
//SPSR=0b00000000;


//PORTD = (1<<PD5); //LED on




//temp chars for printing
char battery[10];
char secs[10];
char mins[10];
char hours[10];
char usvs[10]="0";
char HV[10]="0";
char DOS[10]="0";

  for (int i = 0; i < 60; i++ ) {
	  totalinmin[i] = 0; 
  }

//char fb_test[20]; //test
//char pulse_test[20]; //test
//char collisions[20]; //test


		//	battery voltage level check at start
		ADMUX=0b01000110; 			  // muxer to battery voltage feedback
		//SMCR |= (1 << SE);
		//SMCR |= (1 << SM0); //adc-noise reduction sleep mode		
		ADCSRA |= (1 << ADSC);
		//ADCSRA |= ADSC;               // start an ADC conversion
		while(ADCSRA & _BV(ADSC));    // wait until conversion is complete
		//ADCSRA |= ADSC;               // start an ADC conversion
		ADCSRA |= (1<<ADSC);
		while(ADCSRA & _BV(ADSC));    // wait until conversion is complete
		//SMCR |= (0 << SE);			//back from sleep
		battery_voltage_lvl = ADC;    // save the value to battery_voltage_lvl
		ADMUX=0b01000111; 			  // back to HV feedback
		//ADCSRA |= (1<<ADSC);
		//ADCSRA |= ADSC;               // start an ADC conversion
		//while(ADCSRA & _BV(ADSC));    // wait until conversion is complete
		batterylevelcheck = 1;	

cli();  // disable interrupts
sei(); //enable interrupt

	//main loop
	 while(1)
	 {
		if(cmd_pulse==1)	//triggered once a second
		{

			
			totalinmin[59]=geiger_pulses; //collisions per second
			geiger_pulses=0;
			for (int k = 0; k <= 58; k++)
			{
				totalinmin[k]=totalinmin[k+1];
			}

			for(int k = 59; k >= 0; k--)
			{
				//if(totalinmin[k] != 0)
				//{
					totalnum=totalnum+totalinmin[k];
					//continue;
				//}	
			}
			
			geiger_cps2=totalnum;
			totalnum=0;		
			actual_cps2=((geiger_cps2*1000)/(1000-(geiger_cps2*0.190)));	
			//total dosage calculation
			dosage_total=(actual_cps2*tube); 
			//174.46 CPM/uSv/hr)-> (cp_total/348.92)((usv/hr)),((c/m)/hr=(1/60)/1)
			//1/(348.92*60)=0.0000477664412091
			
			if(first_min == 0) {
				dosage=(double)((dosage_total*60)/timer_seconds);
			}
			
			dosage=(double)(dosage_total/60);
			
			//dosage=(double)(dosage_total*100000); //testing 

			
			testing_min=0;
			

			testing++;
			if (testing>=5) //once every 5 seconds
			{	
				
				totalnum2=0;
				for(int k = 59; k >= 55; k--)
				{
					totalnum2+=totalinmin[k];
					
				}
				
				totalinmin2[29]=totalnum2; //collisions per second
				
				for (int k = 0; k <= 28; k++)
				{
					totalinmin2[k]=totalinmin2[k+1];
				}
				 
				geiger_cps=geiger_pulse; //collisions per second
				actual_cps=(long double)(geiger_cps/(1000-(geiger_cps*0.190))); 
				//actual_cps=(float)(geiger_cps/(1-(geiger_cps*0.000190))); 
				//190us dead time on SBM-20 before new detection can be made
				//now we have Collisions Per Second as float
				usv=(long double)((tube*actual_cps)/testing); //174.46 CPM/uSv/hr 
				///174.46 CPM/uSv/hr /60)=2.9-> 1/2.9=0.344 usv/hr
				// close to the average of SBM-20, Co60 and Ra226 sensitivity
				// dosage is now in usv/hr (micro)	
				//usv=(long double)usv*1000;
				geiger_pulse=0;			//reset collision counter/
				testing=0;
				 
			}
			

			if(batterylevelcheck)
			{
		
				bat=(float)0.0255*battery_voltage_lvl; //battery voltage level if resistors have low % tolerances
				//((2.4k+9.91k)/2.4k)*1024/5
		
				
				batterylevelcheck=0;	
			}
			
			//			
						
			//intToStr(duty,pulse_test, 0);	//test
			//intToStr(fb_voltage, fb_test, 5);	//test
			//intToStr(geiger_cps, collisions, 5);	//test
			

		cmd_pulse=0;
		
		//now=1;
		//OCR0A=20;
		//delay_ms(10);
		//OCR0A=0;
		
		}
		
		if (led)
		{
			
			led=0;
			int gah=1;
			int test=15;  
			int microtest=90;		
			LEDon;
			
			//main chirp
			while(!led && gah <= test){
				BUZon;
				//delay_ms(1);
				//delay_ms(gah);
				delay_us(microtest);
				BUZoff;	
				//delay_us(105);
				delay_us(microtest-gah*7);
				gah++;
			}
			gah=1;
			//trailing edge
			while(!led && gah <= test*2){
				BUZon;
				//delay_ms(1);
				//delay_ms(gah);
				delay_us(microtest-gah*8);
				BUZoff;
				//delay_us(105);
				delay_us(microtest);
				gah++;
			}
			LEDoff;	
			
		}
		
		ftoad(usv, usvs, 2);
		if(usv>= 0.1){
			ftoad(usv, usvs, 1);
		}
		
		if(usv>= 1){
			usv_1=usv;
			ftoad(usv_1, usvs, 1);
		}
		
		ftoa(dosage, DOS, 3);
		if(dosage >= 10 && dosage < 100){
			ftoa(dosage, DOS, 2);
		}
		
		
		ftoa(bat, battery, 2); //convert float to string
		intToStr(timer_seconds, secs, 2); //seconds
		intToStr(timer_minutes, mins, 2); //mins
		intToStr(timer_hours, hours, 2); //hours
		ftoa(HV_vol, HV, 0);
		
		//printing to display
		u8g_FirstPage(&u8g);
		do
		{
			u8g_SetFont(&u8g, u8g_font_7x13);
			u8g_DrawStr(&u8g, 0, 13, hours);
			u8g_DrawStr(&u8g, 14, 13, ":");
			u8g_DrawStr(&u8g, 21, 13, mins);
			u8g_DrawStr(&u8g, 35, 13, ":");
			u8g_DrawStr(&u8g, 42,13, secs);
			if (bat >= 2.3)
			{
				battery_full_marking();
			}
			if (bat < 2.3 && bat >= 2.1)
			{
				battery_half_marking();
			}
			if (bat < 2.1)
			{
				battery_low_marking();
			}
			u8g_DrawStr(&u8g, 84, 13, battery);
			u8g_DrawStr(&u8g, 120, 13, "V");
			
			//slidevalue(dosage);	//shows rad value as slider from last minute
			
				
			histogram();
						
			if( usv >= 1 )
			{	
				//usv_1=(long double)usv/1000;			
				//ftoad(usv_1, usvs, 1);
				u8g_SetFont(&u8g, u8g_font_10x20);
				u8g_DrawStr(&u8g, 87, 33, usvs);
				u8g_DrawStr(&u8g, 87, 45, "m");
			}
			if(usv < 1 && usv >= 0.1 )
			{
				u8g_SetFont(&u8g, u8g_font_10x20); //biggest font
				u8g_DrawStr(&u8g, 77, 33, usvs);
				u8g_SetFont(&u8g, u8g_font_9x18);
				u8g_DrawStr(&u8g, 88, 45, "µ");
			}
			if(usv < 0.1  && usv >= 0.01 )
			{
				u8g_SetFont(&u8g, u8g_font_10x20); //biggest font
				u8g_DrawStr(&u8g, 77, 33, usvs);
				u8g_SetFont(&u8g, u8g_font_9x18);
				u8g_DrawStr(&u8g, 88, 45, "µ");
			}
			if(usv < 0.01  && usv >= 0.001 )
			{
				u8g_SetFont(&u8g, u8g_font_10x20); //biggest font
				u8g_DrawStr(&u8g, 87, 33, usvs);
				u8g_SetFont(&u8g, u8g_font_9x18);
				u8g_DrawStr(&u8g, 88, 45, "µ");
			}
			if(usv < 0.001)
			{
				u8g_SetFont(&u8g, u8g_font_10x20); //biggest font
				u8g_DrawStr(&u8g, 87, 33, usvs);
				u8g_SetFont(&u8g, u8g_font_9x18);
				u8g_DrawStr(&u8g, 88, 45, "µ");
			}
			
			u8g_SetFont(&u8g, u8g_font_7x13);
			u8g_DrawStr(&u8g, 98, 45, "Sv/h"); //usv/h
			

//u8g_DrawStr(&u8g, 1, 48, fb_test); //test
//u8g_DrawStr(&u8g, 1, 48, pulse_test); //test
//u8g_DrawStr(&u8g, 1, 48, collisions); //test
		
			u8g_DrawStr(&u8g, 0, 62, "HV:");
			u8g_DrawStr(&u8g, 22, 62, HV);
			u8g_DrawStr(&u8g, 44, 62, "V");
			
			//u8g_SetFont(&u8g, u8g_font_6x13O);
			u8g_SetFont(&u8g, u8g_font_6x13);
			
			
			if(dosage >= 1000 && dosage < 10000)		//total dosage count for msv's
			{		
				dosage_1=(double)dosage/1000;
				ftoa(dosage_1, DOS, 2);
				u8g_DrawStr(&u8g, 70, 62, DOS);
				u8g_DrawStr(&u8g, 96, 62, "mSv/h");
			}
			if(dosage < 1000 && dosage >= 100)
			{
				u8g_DrawStr(&u8g, 54, 62, DOS);
				u8g_DrawStr(&u8g, 96, 62, "µSv/h");	
			}
			if(dosage < 100 && dosage >= 10)
			{
				u8g_DrawStr(&u8g, 64, 62, DOS);
				u8g_DrawStr(&u8g, 96, 62, "µSv/h");
			}
			if(dosage < 10 )
			{
				u8g_DrawStr(&u8g, 64, 62, DOS);
				u8g_DrawStr(&u8g, 96, 62, "µSv/h");
			}
			
			
		} while ( u8g_NextPage(&u8g) );
		u8g_Delay(500); //u8g_Delay(200);
		
		
	}
	//SMCR=0b00000001; //Idle mode
	//_delay_ms(500);
}
/*
ISR(TIMER0_COMPA_vect){	
	
	if(OCR0A==20 && now==1){
		now=0;
		OCR0A+=20;	
	}
	if(OCR0A>=200 && now==0){
		OCR0A-=20;		
	}	
	
	
}
*/

ISR(TIMER2_COMPA_vect) { //interrupts every 1.6ms
	timer_1s++;
	//zero the timer if 1 second has been reached
	
	
	if(timer_1s >= 625)		//625*1.6ms=1s
	{
		timer_seconds++; //increase if under 59 sec	
	
		cmd_pulse=1; //1 second has passed time to refresh display
		
		timer_1s-=625;
		if(timer_seconds > 59)
		{
			timer_seconds-=60; //zero if
			timer_1min++;
			first_min=1;
			timer_minutes++; //increase if under 59 min
			if(timer_minutes > 59)
			{
				timer_minutes-=60; //zero if
				timer_hours++;
				if(timer_hours > 98) // no more than 2 digits for hours in display
				{
					timer_hours=0; //zero the timer
				}
				
				//1hour reached
				timer_1h=0;
				
			}
			
			//	battery voltage level check every 1 min
			ADMUX=0b01000110; 			  // muxer to battery voltage feedback
			ADCSRA |= (1 << ADSC);	 // start an ADC conversion
			while(ADCSRA & _BV(ADSC));    // wait until conversion is complete
			ADCSRA |= (1 << ADSC);	 // start an ADC conversion
			while(ADCSRA & _BV(ADSC));    // wait until conversion is complete
			battery_voltage_lvl = ADC;    // save the value to battery_voltage_lvl
			ADMUX=0b01000111; 			  // back to HV feedback
			//ADCSRA |= (1 << ADSC);	 // start an ADC conversion
			batterylevelcheck = 1;
			timer_1min=0;				  // set minute timer to 0

		timer_1h++;
		}
		}
	
}


ISR(TIMER1_COMPA_vect){	
	
	duty_change_counter++;
	
	if(duty_change_counter >= 100){ //25kHz/100=250Hz 
		duty_change_counter=0;	
		ADMUX=0b01000111;
		//_delay_us(1);
		volt_value=0;
		fb_voltage=0;
		while(volt_value < OVERSAMPLING){
			ADCSRA |= (1 << ADSC);	 //start an ADC conversion
			while(ADCSRA & _BV(ADSC));    //wait until conversion is complete
			fb_voltage+=ADC;
			volt_value++;
		}
		fb_voltage=round(fb_voltage/OVERSAMPLING);
		HV_vol=(double)1.05*fb_voltage;
	
		// measured 5v actual vcc and resistor values
		//((fb_voltage*5)*((9680000+46.4E3)/46.4E3))/1024=voltage
		//5*209.620689655=0.977
	
		//PID
		err_old=err;
		err=right_voltage-HV_vol;
		P_err=err; //Proportional is directly the difference times P_val (bang bang value)
	
		mean_I_err[I_memory-1]=(int32_t)err;
		mean_I_error=0;
		for (int i=0;i< I_memory-1;i++){
			mean_I_err[i]=(int32_t)mean_I_err[i+1];
			mean_I_error+=(int32_t)mean_I_err[i];
		}
		mean_I_error+=(int32_t)err;
		I_err=(int32_t)mean_I_error;
		D_err=(int32_t)err-err_old;
	
		PID=(int32_t)P_val*P_err + I_val*I_err + D_val*D_err;
	
	
		if( PID > duty_max )
		{
			PID=duty_max;
		}
		else if(PID < 0){
			PID=0;
		}
		duty=(uint16_t)PID;
		OCR1A=duty; //OCR1A is 16-bit register and now houses the "right" PWM duty cycle for HV
		}
}



// external interrupts, falling edges on either
// we have a top limit of 2^32-1 pulses.

ISR (INT0_vect) {
	geiger_pulse++;
	geiger_pulses++;
	led=1;
}

ISR (INT1_vect) {
	geiger_pulse++;
	geiger_pulses++;
	led=1;
}
