/*
 * geigerV14.cpp
 *
 * Created: 20.9.2018 20.53.21
 * Author : kei
 */ 

#define OVERSAMPLING 3
#define I_memory 40 //samples

#define P_val 10
#define I_val 3
#define D_val 6

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdbool.h>

#include "u8g.h"
u8g_t u8g;



//single sbm-20
double tube1 = 0.344827586207;
//dual sbm-20
double tube2 = 0.172413793104;

double input_voltage_calc = 0.00647; //(10k/8k)*(5v/1024)=0.00610351
double battery_voltage_lvl_calc=0.02576; //(124/24)*(5v/1024)=

#define LIPO_ARRAY_LENGTH 8

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


volatile uint32_t	timer_1s=0,		//these are test counters
					timer_1min=0,
					timer_1h=0,
					
					geiger_pulse1=0,
					geiger_pulses1=0,
					geiger_pulse2=0,
					geiger_pulses2=0,
					geiger_cps=0,
					geiger_cps2=0,
					total_time_in_seconds=0;

volatile int16_t	duty=0, //initial number
					testing=0,
					fb_voltage=0,
					duty_max=600; 			  //maximum duty cycle/ICR1	39%

volatile uint16_t	timer_seconds=0,	//these house the real time values from when the device was started
					timer_minutes=0,
					timer_hours=0,
					totalnum=0,
					totalnum2=0,
					volt_value=0,
					duty_change_counter=0;
					

uint16_t 			battery_voltage_lvl_ADC=0,
					broken_counter=0;

uint8_t				now2=0;

volatile bool		first_minute_passed = 0,
					refresh_display=1,
					high_voltage_duty_change=0,
					check_pulses=0,
					v_inputted=0,
					charging=0;
					
volatile bool		 first_min = 0,
now=0;
		  
double			actual_cps=0.0,
				actual_cps2=0.0,
				HV_vol=0.0,
				right_voltage=400.0,  //~400v calculated output from resistors remember to change to right value
				dosage=0.0,
				dosage_1=0.0,
				total_dose=0.0,
				dosage_total=0.0,
				bat=0.0,
				charging_voltage=0.0,
				battery_voltage_lvl= 0.0;	

 double 		usv=0.0,
					usv_1=0.0;

bool				cmd_pulse=0,
					setnewhv=0,
					led=0,
					batterylevelcheck=0,
					timernow=0,
					dos_trig_2=0,
					dos_trig=0,
					battery_voltage_check_first_time=1,
					detected_as_broken=0;
					
					
uint32_t totalinmin[60];
uint32_t totalinmin2[30];
double actual_cps3=0,
usv3=0;
uint16_t xx=120;
uint16_t yy=35;
uint16_t y_start=14;

//temp chars for printing
char battery[10]={0,0,0,0,0,0,0,0,0,0};	
char secs[10]={0,0,0,0,0,0,0,0,0,0};	
char mins[10]={0,0,0,0,0,0,0,0,0,0};	
char hours[10]={0,0,0,0,0,0,0,0,0,0};	
char usvs[10]={0,0,0,0,0,0,0,0,0,0};	
char HV[10]={0,0,0,0,0,0,0,0,0,0};	
char DOS[10]={0,0,0,0,0,0,0,0,0,0};	
char CHAR_VOLT[10]={0,0,0,0,0,0,0,0,0,0};	

char string_battery_ADC[10]={0,0,0,0,0,0,0,0,0,0};	
	
char string_bat_percent[10]={0,0,0,0,0,0,0,0,0,0};	
					

/************************************************************************************************************************************************/
/* Global Objects                                                       																		*/
/************************************************************************************************************************************************/
#include "lipo.h"
#include "functions.cpp"
#include "pitches.h"
#include "test_sounds.h"

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
	
	//pb0=rst disp, pb1 pwm, pb3 mosi, pb5 sck
	DDRB = (1<<PORTB0)|(1<<PORTB1)|(1<<PORTB3)|(1<<PORTB5);
	PORTB =0x00;
	//pc0 feedback
	DDRC =0x00;
	PORTC =0x00;
	//pd1=input v (int),pd2 and pd3 tubes (extint) , pd5 speaker, pd6 led, pd7 dc 
	DDRD = (1<<PORTD5)|(1<<PORTD6)|(1<<PORTD7);
	PORTD =(1<<PORTD6);
	
	//adc set up
	
	//ADMUX adlar=0, mux 0000 HV-feedback, mux 0110 battery voltage feedback, input V adc5 0101
	ADMUX=(1<<REFS0);
	ADCSRA=(1<<ADEN)|(1<<ADSC)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0); //with 128 clk division
	
	//PWM
	//fast PWM, non-inverting mode set, 16bit timer
	ICR1=800; //@25kHz top
	OCR1A = 0; //initial duty cycle
	TCCR1A = (1<<COM1A1)|(1<<WGM11); //ICR1 selected as top value
	TCCR1B =(1<<WGM13)|(1<<WGM12)|(1<<CS10);
	TIMSK1 =(1<<OCIE1A);

	//external interrupts
	EICRA=(1<<ISC11)|(1<<ISC01); //falling edges on int1 (pd3) and int0 (pd2)
	EIMSK=(1<<INT1)|(1<<INT0); //mask register
	EIFR=(1<<INTF1)|(1<<INTF0); //clear interrupt flags

	//PCint interrupts
	PCICR=(1<<PCIE2)|(1<<PCIE1);
	PCMSK2=(1<<PCINT17); //PD1
	//PCMSK1=(1<<PCINT12); //PC4
	
	
	//timer 2 control 8 bit
	//output is 20MHz/(256*125)=1.6ms tick
	OCR2A = 124; //runs to value 0:124, to get exact ms
	TCCR2A = (1<<WGM21);
	TCCR2B =(1<<FOC2A)|(1<<CS22)|(1<<CS21); //force compare match A, prescaler to 256
	TIMSK2= (1<<OCIE2A); //masked compare match A
	
	/*
	//chirp or music from timer 0 B output
	OCR0A =255; //max value
	OCR0B=0; //initial duty cycle
	TCCR0A = (1<<COM0B1)|(1<<WGM00); //phase correct  pwm   |(1<<WGM01)
	TCCR0B =(1<<WGM02)|(1<<CS02);// 256 prescaler (must be same as timer 2)
	//TIMSK0=(1<<OCIE0B); //compare match B interrupt
	*/

}



int main(void)
{
	//delay_ms(1000);
	delay_ms(300);
	for (int i=0;i< I_memory-1;i++){
		mean_I_err[i]=(int32_t)0.0;
	}
	 for (int i = 0; i < 60; i++ ) {
		 totalinmin[i] = 0;
	 }
	delay_ms(200);
	system_setup();
	u8g_setup();
	delay_ms(200);
	LCD_clear();
	delay_ms(200);
	
	if(VOLTAGE_APPLIED_PIN){
		v_inputted=1;
		LEDoff;
	}
	/*if(CHARGING_STATE_PIN){
		charging=1;
		LEDoff;
	}*/
	
	//check_battery_voltage();
	
	//cli();
	sei(); //enable interrupts
	
    while (1) 
    {
		if(high_voltage_duty_change){
			high_voltage_duty_change=0;
			high_voltage_duty();
		}
		
		if(check_pulses){
			check_pulses=0;
			/*
			totalinmin[59]=geiger_pulses1+geiger_pulses2; //collisions per second
			
			for (int k = 0; k <= 58; k++)
			{
				totalinmin[k]=totalinmin[k+1];
			}

			for(int k = 59; k >= 0; k--)
			{
				totalnum=totalnum+totalinmin[k];
			}
			*/
			
			
			totalnum+=geiger_pulses1+geiger_pulses2;
			geiger_pulses1=0;
			geiger_pulses2=0;
			
		
			geiger_cps2=totalnum;
			totalnum=0;
			actual_cps2=((geiger_cps2*1000)/(1000-(geiger_cps2*0.190)));
			//total dosage calculation
			dosage_total+=actual_cps2*tube2;
			//174.46 CPM/uSv/hr)-> (cp_total/348.92)((usv/hr)),((c/m)/hr=(1/60)/1)
			//1/(348.92*60)=0.0000477664412091
		
			//dosage=(double)(dosage_total/60);
			//if(first_minute_passed == 0) {
			//	dosage=(double)((dosage_total*60)/timer_seconds);
			//}
			
			total_time_in_seconds=timer_seconds+60*timer_minutes+360*timer_hours;
			dosage=(double)(dosage_total/total_time_in_seconds);
		
					

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
			
				geiger_cps=geiger_pulse1+geiger_pulse2; //collisions per second
				actual_cps=(long double)(geiger_cps/(1000-(geiger_cps*0.190)));
				//actual_cps=(float)(geiger_cps/(1-(geiger_cps*0.000190)));
				//190us dead time on SBM-20 before new detection can be made
				//now we have Collisions Per Second as float
				usv=(long double)((tube2*actual_cps)/testing); //174.46 CPM/uSv/hr
				///174.46 CPM/uSv/hr /60)=2.9-> 1/2.9=0.344 usv/hr
				// close to the average of SBM-20, Co60 and Ra226 sensitivity
				// dosage is now in usv/hr (micro)
				//usv=(long double)usv*1000;
				
				if(!detected_as_broken && ((geiger_pulse1 == 0 && geiger_pulse2 !=0) || (geiger_pulse1 != 0 && geiger_pulse2 ==0))){
					broken_counter++;
				}
				else if(!detected_as_broken && geiger_pulse1 !=0 && geiger_pulse2 !=0){
					broken_counter=0;
				}
				if(broken_counter>=10 && !detected_as_broken){
					broken_counter=0;
					//a tube is broken
					tube2=tube1;
					detected_as_broken=1;					
				}
				
				geiger_pulse1=0;			//reset collision counter/
				geiger_pulse2=0;
				testing=0;
				battery_voltage_check_first_time=0;
			}
		}
		if(batterylevelcheck || (battery_voltage_check_first_time && testing>=4))
		{
			check_battery_voltage();
			//bat=(float)0.0255*battery_voltage_lvl; //battery voltage level if resistors have low % tolerances
			//((2.4k+9.91k)/2.4k)*1024/5
			batterylevelcheck=0;
		}
			
		if (led)
		{
			int gah=1;
			int test=15;
			int microtest=90;
			LEDon;
			led=0;
			
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
		
		if(refresh_display){
			refresh_display=0;
			if(v_inputted){
				LEDoff;
				check_input_voltage();
				check_battery_voltage();
			}
			
			display_draw();
		}
		
    }
	
}

ISR(TIMER2_COMPA_vect) { //interrupts every 1.6ms
	
	timer_1s++;
	//zero the timer if 1 second has been reached
	if(timer_1s >= 625)		//625*1.6ms=1s
	{
		timer_seconds++; //increase if under 59 sec
		refresh_display=1; //1 second has passed time to refresh display
		check_pulses=1;
		timer_1s=0;
		if(timer_seconds > 59)
		{
			timer_seconds=0; //zero if
			first_minute_passed=1;
			batterylevelcheck=1;
			timer_minutes++; //increase if under 59 min
			if(timer_minutes > 59)
			{
				timer_minutes=0; //zero if
				timer_hours++;
				if(timer_hours > 98) // no more than 2 digits for hours in display
				{
					timer_hours=0; //zero the timer
				}
				//1hour reached
				timer_1h=0;
			}
			

			timer_1h++;
		}
	}
	
}

ISR(TIMER1_COMPA_vect){	
	high_voltage_duty_change=1;
	//high_voltage_duty();
}


// external interrupts, falling edges on either
// we have a top limit of 2^32-1 pulses.

ISR (INT0_vect) {
	geiger_pulse1++;
	geiger_pulses1++;
	
		led=1;

}

ISR (INT1_vect) {
	geiger_pulse2++;
	geiger_pulses2++;
	
		led=1;


}

//5v applied to charging port
ISR (	PCINT2_vect){
	
	if(VOLTAGE_APPLIED_PIN){
		v_inputted=1;
	}
	else{
		v_inputted=0;
	}	
}
/*
//charging
ISR (	PCINT1_vect){
	if(CHARGING_STATE_PIN){
		charging=1;
	}
	else{
		charging=0;
	}
	
	
}*/

