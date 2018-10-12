
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

#define	LED_PORT REGISTER_BIT(PORTD,6)
#define LEDon LED_PORT = 1
#define LEDoff LED_PORT = 0

#define	SOUND_PORT REGISTER_BIT(PORTD,5)
#define BUZon SOUND_PORT = 1
#define BUZoff SOUND_PORT = 0

#define	CHARGING_STATE_PIN REGISTER_BIT(PINC,4)

#define	VOLTAGE_APPLIED_PIN REGISTER_BIT(PIND,1)

void high_voltage_duty(){
	duty_change_counter++;
	
	if(duty_change_counter >= 100){ //25kHz/100=250Hz
		duty_change_counter=0;
		ADMUX=0b01000000;
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
		
		if(!v_inputted){
			duty=(uint16_t)PID;
			OCR1A=duty; //OCR1A is 16-bit register and now houses the "right" PWM duty cycle for HV
		}
		else{
			OCR1A=0;
			mean_I_error=0;
		}
		
	}
	
}


void check_battery_voltage(){
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
		battery_voltage_lvl_ADC = ADC;    // save the value to battery_voltage_lvl
		ADMUX=0b01000000; 			  // back to HV feedback
		//ADCSRA |= (1<<ADSC);
		//ADCSRA |= ADSC;               // start an ADC conversion
		//while(ADCSRA & _BV(ADSC));    // wait until conversion is complete
		battery_voltage_lvl=(double)battery_voltage_lvl_ADC*battery_voltage_lvl_calc;	
		//batterylevelcheck=1;
}




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
	u8g_DrawBox(&u8g,66,7,2,3);
	u8g_DrawBox(&u8g,68,5,11,7);      
}

void battery_80_90_marking(void)
{
	u8g_SetFont(&u8g, u8g_font_7x13);
	u8g_DrawBox(&u8g,66,7,2,3);
	u8g_DrawFrame(&u8g,68,5,11,7);      
	u8g_DrawBox(&u8g,70,5,8,7);
}
void battery_70_80_marking(void)
{
	u8g_SetFont(&u8g, u8g_font_7x13);
	u8g_DrawBox(&u8g,66,7,2,3);
	u8g_DrawFrame(&u8g,68,5,11,7);      
	u8g_DrawBox(&u8g,71,5,7,7);
}
void battery_60_70_marking(void)
{
	u8g_SetFont(&u8g, u8g_font_7x13);
	u8g_DrawBox(&u8g,66,7,2,3);
	u8g_DrawFrame(&u8g,68,5,11,7);      
	u8g_DrawBox(&u8g,72,5,6,7);
}

void battery_50_60_marking(void)
{
	u8g_SetFont(&u8g, u8g_font_7x13);
	u8g_DrawBox(&u8g,66,7,2,3);
	u8g_DrawFrame(&u8g,68,5,11,7);      
	u8g_DrawBox(&u8g,73,5,5,7);
}

void battery_40_50_marking(void)
{
	u8g_SetFont(&u8g, u8g_font_7x13);
	u8g_DrawBox(&u8g,66,7,2,3);
	u8g_DrawFrame(&u8g,68,5,11,7);      
	u8g_DrawBox(&u8g,74,5,4,7);
}

void battery_30_40_marking(void)
{
	u8g_SetFont(&u8g, u8g_font_7x13);
	u8g_DrawBox(&u8g,66,7,2,3);
	u8g_DrawFrame(&u8g,68,5,11,7);      
	u8g_DrawBox(&u8g,75,5,3,7);
}

void battery_20_30_marking(void)
{
	u8g_SetFont(&u8g, u8g_font_7x13);
	u8g_DrawBox(&u8g,66,7,2,3);
	u8g_DrawFrame(&u8g,68,5,11,7);      
	u8g_DrawBox(&u8g,76,5,2,7);
}
void battery_10_20_marking(void)
{
	u8g_SetFont(&u8g, u8g_font_7x13);
	u8g_DrawBox(&u8g,66,7,2,3);
	u8g_DrawFrame(&u8g,68,5,11,7);      
	u8g_DrawBox(&u8g,77,5,1,7);
}

void battery_low_marking(void)
{
	u8g_SetFont(&u8g, u8g_font_7x13);
	u8g_DrawBox(&u8g,66,7,2,3);
	u8g_DrawFrame(&u8g,68,5,11,7);      
}

void battery_half_marking(void)
{
	u8g_SetFont(&u8g, u8g_font_7x13);	
	u8g_DrawBox(&u8g,66,7,2,3);
	u8g_DrawFrame(&u8g,68,5,11,7);
	u8g_DrawBox(&u8g,75,5,5,7);
}

#define move_by_x 1

void battery_charging_marking(void)
{
	u8g_SetFont(&u8g, u8g_font_7x13);	
	u8g_DrawLine(&u8g,68,5,73,5);
	u8g_DrawLine(&u8g,76,5,80,5);	
	u8g_DrawLine(&u8g,68,6,72,6);
	u8g_DrawLine(&u8g,75,6,80,6);	
	u8g_DrawLine(&u8g,66,7,71,7);
	u8g_DrawLine(&u8g,74,7,80,7);	
	u8g_DrawLine(&u8g,66,8,70,8);
	u8g_DrawLine(&u8g,74,8,80,8);
	u8g_DrawLine(&u8g,66,9,73,9);
	u8g_DrawLine(&u8g,76,9,80,9);
	u8g_DrawLine(&u8g,68,10,72,10);
	u8g_DrawLine(&u8g,75,10,80,10);
	u8g_DrawLine(&u8g,68,11,71,11);
	u8g_DrawLine(&u8g,74,11,80,11);
}

void battery_charging_marking2(void)
{
	u8g_DrawLine(&u8g,68,5,72,5);
	u8g_DrawLine(&u8g,74,5,78,5);
	u8g_DrawLine(&u8g,68,6,72,6);
	u8g_DrawLine(&u8g,74,6,78,6);
	u8g_DrawLine(&u8g,66,7,73,7);
	u8g_DrawLine(&u8g,76,7,78,7);
	u8g_DrawLine(&u8g,66,8,68,8);
	u8g_DrawLine(&u8g,77,8,78,8);
	u8g_DrawLine(&u8g,66,9,69,9);
	u8g_DrawLine(&u8g,72,9,78,9);
	u8g_DrawLine(&u8g,68,10,70,10);
	u8g_DrawLine(&u8g,73,10,78,10);
	u8g_DrawLine(&u8g,68,11,71,11);
	u8g_DrawLine(&u8g,73,11,78,11);
}
//descending both values
double dsp_lookup(double (*table)[2], double x, int16_t array_size){
	int16_t i=0;
	double m=0.0;	
	while(i < array_size){  
		//find the point where adc value is bigger than table value for the first time
		if ( x == table[i][0] ){   //make sure the point isn't past the end of the table
			return table[i][1];//no need for further calculations %
		}
		if(x > table[i][0]){
			break;//found it, remember that the pointer is actually -1 to the value
		}
		i++;
	}
	if ( i == array_size ){   //make sure the point isn't past the end of the table
		return table[array_size-1][1];
	}
	if ( i == 0 ){  //make sure the point isn't before the beginning of the table
		return (double)table[i][1]; //return first value
	}
	//slope k=(y2-y1)/(x2-x1)
	m =(double) (table[i-1][1]-table[i][1]) / ( table[i-1][0] - table[i][0]); //keep slope positive
	//y=kx+b	
	return (double) round((m * (x - table[i][0]))+ table[i][1]); //this is the solution to the point slope formula	
}

void draw_battery(void)
{
	double bat_percent=0;
	bat_percent=dsp_lookup(lipo_ADC_to_percentages, battery_voltage_lvl, LIPO_ARRAY_LENGTH);
	
	u8g_SetFont(&u8g, u8g_font_7x13);
		
	//ftoa(battery_voltage_lvl,string_battery_ADC,2);
	//u8g_DrawStr(&u8g, 66,13, string_battery_ADC);
	//u8g_DrawStr(&u8g, 94,13, "v");
	
	//add icon when it's charging
	if(v_inputted){
		battery_charging_marking2();
		//battery_full_marking();
	}
	else if(bat_percent >= 90){
		battery_full_marking();
	}
	else if(bat_percent >= 80 && bat_percent < 90 ){
		battery_80_90_marking();
	}
	else if(bat_percent >= 70 && bat_percent < 80 ){
		battery_70_80_marking();
	}
	else if(bat_percent >= 60 && bat_percent < 70 ){
		battery_60_70_marking();
	}
	else if(bat_percent >= 50 && bat_percent < 60 ){
		battery_50_60_marking();
	}
	else if(bat_percent >= 40 && bat_percent < 50 ){
		battery_40_50_marking();
	}
	else if(bat_percent >= 30 && bat_percent < 40 ){
		battery_30_40_marking();
	}
	else if(bat_percent >= 20 && bat_percent < 30 ){
		battery_20_30_marking();
	}
	else if(bat_percent >= 10 && bat_percent < 20 ){
		battery_10_20_marking();
	}
	else if(bat_percent < 10){
		battery_low_marking();
	}
	
	ftoa(bat_percent,string_bat_percent,0);
	if(bat_percent > 99){
		u8g_DrawStr(&u8g, 96,13, string_bat_percent);
	}
	else if(bat_percent > 9 && bat_percent <=99){
		u8g_DrawStr(&u8g, 103,13, string_bat_percent);
	}
	else if(bat_percent <= 9){
		u8g_DrawStr(&u8g, 110,13, string_bat_percent);
	}
	
	u8g_DrawStr(&u8g, 120,13, "%");
	
	//ftoa(battery_voltage_lvl,string_bat_percent,2);
	//u8g_DrawStr(&u8g, 2,26, string_bat_percent);
	
	//device is being charged
	if(charging)
	{
		
		//drawString( 170, 1, string_charge_current_value,  main_draw_color, ILI9341_YELLOW, 1);
	}
	else
	{
		//drawString( 170, 1, "    ",  main_draw_color, menu_bg, 1);
	}

}

void check_input_voltage(){
	
	uint16_t charging_voltage_ADC=0;
	//	battery voltage level check at start
		ADMUX=0b01000101; 			  // muxer 
		//SMCR |= (1 << SE);
		//SMCR |= (1 << SM0); //adc-noise reduction sleep mode		
		ADCSRA |= (1 << ADSC);
		//ADCSRA |= ADSC;               // start an ADC conversion
		while(ADCSRA & _BV(ADSC));    // wait until conversion is complete
		//ADCSRA |= ADSC;               // start an ADC conversion
		ADCSRA |= (1<<ADSC);
		while(ADCSRA & _BV(ADSC));    // wait until conversion is complete
		//SMCR |= (0 << SE);			//back from sleep
		charging_voltage_ADC = ADC;    // save the value to battery_v
		ADMUX=0b01000000; 			  // back to HV feedback
	
	charging_voltage=charging_voltage_ADC*input_voltage_calc;
	ftoa( charging_voltage, CHAR_VOLT,2); //convert float to string
	
	
}





void display_draw(){
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
			
			if(!battery_voltage_check_first_time){
				draw_battery();
			}
			if(detected_as_broken){
				battery_charging_marking();
			}
			//slidevalue(dosage);	//shows rad value as slider from last minute
			
						
			if( usv >= 1 )
			{	
				//usv_1=(long double)usv/1000;			
				//ftoad(usv_1, usvs, 1);
				u8g_SetFont(&u8g, u8g_font_fur25n);
				u8g_DrawStr(&u8g, 30, 45, usvs);
				u8g_SetFont(&u8g, u8g_font_7x13);
				u8g_DrawStr(&u8g, 99, 30, "mSv");
			}
			else if(usv < 1 && usv >= 0.1 )
			{
				u8g_SetFont(&u8g, u8g_font_fur25n);
				u8g_DrawStr(&u8g, 30, 45, usvs);
				u8g_SetFont(&u8g, u8g_font_7x13);
				u8g_DrawStr(&u8g, 99, 30, "uSv");//µ
			}
			else if(usv < 0.1  && usv >= 0.01 )
			{
				u8g_SetFont(&u8g, u8g_font_fur25n);
				u8g_DrawStr(&u8g, 30, 45, usvs);
				u8g_SetFont(&u8g, u8g_font_7x13);
				u8g_DrawStr(&u8g, 99, 30, "uSv");//µ
			}
			else if(usv < 0.01  && usv >= 0.001 )
			{
				u8g_SetFont(&u8g, u8g_font_fur25n);
				u8g_DrawStr(&u8g, 30, 45, usvs);
				u8g_SetFont(&u8g, u8g_font_7x13);
				u8g_DrawStr(&u8g, 99, 30, "uSv"); //µ
			}
			else if(usv < 0.001)
			{
				u8g_SetFont(&u8g, u8g_font_fur25n);
				u8g_DrawStr(&u8g, 30, 45, usvs);
				u8g_SetFont(&u8g, u8g_font_7x13);
				u8g_DrawStr(&u8g, 99, 30, "uSv");//µ
			}
			
			u8g_SetFont(&u8g, u8g_font_7x13);
			u8g_DrawStr(&u8g, 99, 44, "/h"); //usv/h
			

//u8g_DrawStr(&u8g, 1, 48, fb_test); //test
//u8g_DrawStr(&u8g, 1, 48, pulse_test); //test
//u8g_DrawStr(&u8g, 1, 48, collisions); //test
		
			u8g_DrawStr(&u8g, 0, 62, "HV:");
			u8g_DrawStr(&u8g, 22, 62, HV);
			u8g_DrawStr(&u8g, 44, 62, "V");
			
			//u8g_SetFont(&u8g, u8g_font_6x13O);
			u8g_SetFont(&u8g, u8g_font_6x13);
			
			//lowest right values
			if(dosage >= 1000 && dosage < 10000)		//total dosage count for msv's
			{		
				dosage_1=(double)dosage/1000;
				ftoa(dosage_1, DOS, 2);
				u8g_DrawStr(&u8g, 70, 62, DOS);
				u8g_DrawStr(&u8g, 96, 62, "mSv/h");
			}
			else if(dosage < 1000 && dosage >= 100)
			{
				u8g_DrawStr(&u8g, 54, 62, DOS);
				u8g_DrawStr(&u8g, 96, 62, "uSv/h");	//µ
			}
			else if(dosage < 100 && dosage >= 10)
			{
				u8g_DrawStr(&u8g, 64, 62, DOS);
				u8g_DrawStr(&u8g, 96, 62, "uSv/h");//µ
			}
			else if(dosage < 10 )
			{
				u8g_DrawStr(&u8g, 64, 62, DOS);
				u8g_DrawStr(&u8g, 96, 62, "uSv/h");//µ
			}
			
			//draw input voltage if charging
			if(v_inputted){
				u8g_DrawStr(&u8g, 0, 30, CHAR_VOLT);
				u8g_DrawStr(&u8g, 10, 43, "v");
				LEDoff;
			}
			
			
			
			
		} while ( u8g_NextPage(&u8g) );
		u8g_Delay(10); //u8g_Delay(200);
	
}




