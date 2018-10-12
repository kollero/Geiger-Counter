void mario(){
	
	 uint16_t size = sizeof(underworld_melody) / sizeof(int);
	 for (uint16_t thisNote = 0; thisNote < size; thisNote++) {

		 // to calculate the note duration, take one second
		 // divided by the note type.
		 //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
		 uint16_t noteDuration = 1000/underworld_tempo[thisNote];

		play(noteDuration, underworld_melody[thisNote],1);

		 // to distinguish the notes, set a minimum time between them.
		 // the note's duration + 30% seems to work well:
		 int pauseBetweenNotes = noteDuration * 1.30;
		 delay_ms(pauseBetweenNotes);

		 // stop the tone playing:
		 
		 //buzz(melodyPin, 0,noteDuration);
	 }
	
	
}

	
	
		uint32_t tet=1;
	
	volatile uint32_t timer0_buzzer_PWM_pulses_count;
	volatile uint32_t buzzer_PWM_pulses_count;
	volatile uint32_t buzzer_PWM_pulses_count_max;

	uint32_t timer0_frequency=0;
	uint8_t duty_cycle[512];
	uint32_t tones_count=0;
	uint32_t how_many_tones2=10;
	
	uint32_t buzzer_PWM_delay_ms=1;
uint32_t F_CPU2=20000000;


void play( uint32_t sampling_frequency ,uint8_t (*duty_cycle2), uint16_t samples){
	uint8_t prescalarbits = 0b001;

	//get best possible division for pwm
	timer0_frequency =F_CPU2  / sampling_frequency/2;
	if (timer0_frequency > 255){
		timer0_frequency = F_CPU2 / sampling_frequency /2 /7;
		prescalarbits = 0b010; // clk/8:
		if (timer0_frequency > 255){
			timer0_frequency = F_CPU2 / sampling_frequency/2 /63;
			prescalarbits = 0b011; // clk/64:
			if (timer0_frequency > 255){
				timer0_frequency = F_CPU2 / sampling_frequency /2 /  255;
				prescalarbits = 0b100; // clk/256:
				if (timer0_frequency > 255){
					timer0_frequency = F_CPU2 / sampling_frequency/2 / 1023;
					prescalarbits = 0b101; // clk/256:
				}
			}
		}
	}
	TCCR0B = (TCCR0B & 0b11111000) | prescalarbits;
	for (uint32_t i = 0; i < samples; i++) {
		duty_cycle[i]=0.1*duty_cycle2[i];
	}
	
	if (samples > 0  )
	{
		duty_cycle[0]=timer0_frequency*duty_cycle[0]/255; //proportional pwm
		
		buzzer_PWM_delay_ms =  2*1000*samples/ sampling_frequency;
		OCR0A =(uint8_t) timer0_frequency; //input max value to count to
		OCR0B = duty_cycle[0];
		
		//OCR0B =timer0_frequency*pgm_read_byte(&pcm_samples[0])/255;
		
		timer0_buzzer_PWM_pulses_count =0;
		buzzer_PWM_pulses_count_max =samples;
		//buzzer_PWM_pulses_count=samples;
		TIMSK0=(1<<OCIE0B); //compare match B interrupt on
	}
	else
	{
		buzzer_PWM_pulses_count = 0;
	}
	
}

uint8_t dutycycle_now=0;

	
	
	
	
	
	
ISR(TIMER0_COMPB_vect)
{
	if (timer0_buzzer_PWM_pulses_count < buzzer_PWM_pulses_count_max )
	{
		timer0_buzzer_PWM_pulses_count++;
			//dutycycle_now=abs(pgm_read_byte(&pcm_samples[timer0_buzzer_PWM_pulses_count])-128);
			dutycycle_now=timer0_frequency*duty_cycle[timer0_buzzer_PWM_pulses_count]/255;
			if ( dutycycle_now <= timer0_frequency )
			{
				OCR0B = dutycycle_now;
			}
			else {
				OCR0B =(uint8_t) timer0_frequency;
			}
		
	}
	
	else
	{
		TIMSK0 &= ~(1 << OCIE0B);                 // disable the interrupt
		OCR0B=0;
	}
	
}
	