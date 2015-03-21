/* motor_warm_up - an application for the Pololu Orangutan SVP
 *
 * Created: 3/4/2015 8:27:13 PM
 *  Author: Jesse
 */

#include <pololu/orangutan.h>
#include <stdio.h>
#include <stdbool.h>

volatile int16_t g_counts_m1 = 0;

void init_encoder();

int main()
{
	play_from_program_space(PSTR(">g32>>c32"));  // Play welcoming notes.
	
	lcd_init_printf();
	clear();
	
	init_encoder();
	
	// speed of motor, values from -255 to +255
	int16_t motor_speed = 0;
	
	// var to store button press - declared outside of loop to reduce allocation overhead
	unsigned char button_pressed;
	
	uint32_t loop_count = 0;
	while(1)
	{
		// This loop count stuff is a total hack to slow LCD printing down
		//  enough so that it's readable (i.e., doesn't flicker).
		// I'm fine with a hack since this exercise is a "warm up".  I'd use a
		//  timer otherwise.
		if(loop_count == 0) {
			clear();
			lcd_goto_xy(0,0);
			printf("Speed:%d\n",motor_speed);
			printf("Encoder:%d",g_counts_m1);
		} else {
			loop_count++;
		}
		
		button_pressed = get_single_debounced_button_press(ANY_BUTTON);
		
		switch (button_pressed) {
			case BUTTON_A:
			motor_speed -= 5;
			break;
			case BUTTON_B:
			motor_speed *= -1;
			break;
			case BUTTON_C:
			motor_speed += 5;
			break;
		}
		

		if(motor_speed > 255) {
			motor_speed = 255;
		} else if(motor_speed < -255) {
			motor_speed = -255;
		}
		
		set_m1_speed(motor_speed);

	}
}

/************* INITIALIZATION FUNCTIONS *****************/
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
