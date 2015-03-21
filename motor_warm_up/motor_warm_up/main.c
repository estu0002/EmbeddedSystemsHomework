/* motor_warm_up - an application for the Pololu Orangutan SVP
 *
 * Created: 3/4/2015 8:27:13 PM
 *  Author: Jesse
 */

#include <pololu/orangutan.h>
#include <stdio.h>
#include <stdbool.h>

// absolute encoder counts since program initialization
volatile int16_t g_counts_m1 = 0;

// flag set in ISR to toggle LCD update in cyclic executive
volatile bool doLcdUpdate = false;

// speed variables
volatile int16_t g_speed_calc_prev_encoder_val = 0;
volatile int16_t g_speed_calc_current_speed = 0;

// prototypes for init functions
void init_timer0();
void init_timer3();
void init_encoder();

int main()
{
	play_from_program_space(PSTR(">g32>>c32"));  // Play welcoming notes.
	
	lcd_init_printf();
	clear();
	
	// initialize stuff
	init_timer0();
	init_timer3();
	init_encoder();
	
	// motor drive signal, values from -255 to +255
	int16_t motor_drive_signal = 0;
	
	// var to store button press - declared outside of loop to reduce allocation overhead
	unsigned char button_pressed;
		
	while(1)
	{			
		if(doLcdUpdate) {
			clear();
			lcd_goto_xy(0,0);
			printf("Dr:%d,Enc:%d\n",motor_drive_signal,g_counts_m1);
			printf("Speed:%d",g_speed_calc_current_speed);

			doLcdUpdate = false;
		}
		
		button_pressed = get_single_debounced_button_press(ANY_BUTTON);
		
		switch (button_pressed) {
			case BUTTON_A:
			motor_drive_signal -= 5;
			break;
			case BUTTON_B:
			motor_drive_signal *= -1;
			break;
			case BUTTON_C:
			motor_drive_signal += 5;
			break;
		}
		

		if(motor_drive_signal > 255) {
			motor_drive_signal = 255;
		} else if(motor_drive_signal < -255) {
			motor_drive_signal = -255;
		}
		
		set_m1_speed(motor_drive_signal);

	}
}

/************* INITIALIZATION FUNCTIONS *****************/

void init_timer0() {
	// set up 8 bit time

	// TCCR0B - timer counter control register B
	// FOC0A (dont change) - default 0
	// FOC0B (dont change) - default 0
	//        -
	//        -
	// WGM02  0
	// CS02   1  - clock select - prescale /1024
	// CS01   0
	// CS00   1
	// = 0000 0101
	//    0    5
	TCCR0B = 0x05;


	// TCCR0A - timer counter control register A
	// COM0A1 1 - clear on compare match
	// COM0A0 0
	// COM0B1 (dont change) - default 0
	// COM0B0 (dont change) - default 0
	//        -
	//        -
	// WGM01  1 - CTC, TOP from OCRA
	// WGM00  0
	// = 1000 0010
	//    8    2
	TCCR0A = 0x82;

	// output compare value
	OCR0A = 0xc2; // 10ms; note that 13ms is about the slowest for an 8 bit timer

	// enable the interrupt for the timer
	// TIMSK0 - Timer/counter interrupt mask register
	//        -
	//        -
	//        -
	//        -
	//        -
	// OCIE0B (dont change)
	// OCIE0A 1
	// TOIE0  (dont change)
	// = 0000 0010
	TIMSK0 |= (1 << 1);
	
}

void init_timer3() {
	// set up 16 bit timer 
	
	// TCCR3B - timer counter control register B
	// ICNC3  0
	// ICES3 (dont change) - default 0
	//        -
	// WGM33  0  - WGM mode 4, CTC, TOP from OCR1A (see bits in TCCR1A also)
	// WGM32  1
	// CS32   1  - clock select - prescale /1024
	// CS31   0
	// CS30   1
	// = 0000 1101
	//    0    d
	TCCR3B = 0x0d;
	
	// TCCR3A - timer counter control register A
	// COM3A1 1 - clear on compare match
	// COM3A0 0
	// COM3B1 (dont change) - default 0
	// COM3B0 (dont change) - default 0
	//        -
	//        -
	// WGM31  0 - WGM mode 4, CTC, TOP from OCR3A (see bits in TCCR1B also)
	// WGM30  0
	// = 1000 0000
	//    8    0
	TCCR3A = 0x80;
	
	// output compare register (from AVR Calc)
	//OCR3A = 0x1312; // 250ms
	//OCR3A = 0x00c2; // 10ms = 10,000us
	OCR3A = 0x0013; // 1ms = 1,000us
	
	// enable the interrupt for the timer
	// TIMSK3 - Timer/counter interrupt mask register
	//        -
	//        -
	// ICIE3  -
	//        -
	//        -
	// OCIE3B -
	// OCIE3A 1
	// TOIE3  (dont change)
	// = 0000 0010
	TIMSK3 |= (1 << 1);

}

void init_encoder() {
	// WIRE		DESC				CONNECTION
	// ------	----------------	----------
	// yellow	encoder A output	PIND3
	// white	encoder B output	PIND2
	// blue		encoder Vcc			n/a
	// green	encoder GND			n/a
	
	// since encoder is connected to PIND2 and PIND3, set the pin change interrupt control
	// register up such that interrupts are generated for these pins (p.69)
	//
	// we know that PIND3 is PCINT27 and PIND2 is PCINT26 (p.86)
	//
	// PCICR
	//        -
	//        -
	//        -
	//        -
	// PCIE3  1  PCINT31:24 pin causes interrupt
	// PCIE2  0
	// PCIE1  0
	// PCIE0  0
	// 0000 1000
	//=  0    8
	PCICR |= 0x08;
	
	// PCMSK3 - Pin change mask register 3
	//  PCINT31 0
	//  PCINT30 0
	//  PCINT29 0
	//  PCINT28 0
	//  PCINT27 1
	//  PCINT26 1
	//  PCINT25 0
	//  PCINT24 0
	// 0000 1100
	//=  0    C
	PCMSK3 = 0x0C;
	
	return;
}

/************* INTERRUPT SERVICE ROUTINES *****************/

// booleans to keep track of our previous encoder values
bool g_last_m1a_val = 0;
bool g_last_m1b_val = 0;

// ISR for Pin Change Interrupt 3
ISR(PCINT3_vect) {
	// current state of the encoder
	bool m1a_val = is_digital_input_high(IO_D3); 
	bool m1b_val = is_digital_input_high(IO_D2); 

	// which way are we turning?
	bool plus_m1 = m1a_val ^ g_last_m1b_val;
	bool minus_m1 = m1b_val ^ g_last_m1a_val;

	// increment or decrement encoder count
	if(plus_m1) {
		g_counts_m1++;
	}
	if(minus_m1) {
		g_counts_m1--;
	}

	// and save state for next ISR run
	g_last_m1a_val = m1a_val;
	g_last_m1b_val = m1b_val;
}


// ISR for timer 0
ISR(TIMER0_COMPA_vect) {
	doLcdUpdate = true;
}

// ISR for timer 3
ISR(TIMER3_COMPA_vect) {
	// calculate the current speed
	g_speed_calc_current_speed = (g_counts_m1 - g_speed_calc_prev_encoder_val) / 1; // denominator must match resolution of timer
	g_speed_calc_prev_encoder_val = g_counts_m1;
}