/* motor_warm_up - an application for the Pololu Orangutan SVP
 *
 * This application uses the Pololu AVR C/C++ Library.  For help, see:
 * -User's guide: http://www.pololu.com/docs/0J20
 * -Command reference: http://www.pololu.com/docs/0J18
 *
 * Created: 3/4/2015 8:27:13 PM
 *  Author: Jesse
 */

#include <pololu/orangutan.h>
#include <stdio.h>

int main()
{
	play_from_program_space(PSTR(">g32>>c32"));  // Play welcoming notes.
	
	lcd_init_printf();
	clear();
	
	
	// WIRE		DESC				CONNECTION
	// ------	----------------	----------
	// yellow	encoder A output	PIND3
	// white	encoder B output	PIND2
	// blue		encoder Vcc			n/a
	// green	encoder GND			n/a
	
	// initialize encoder - note that we only really care about the first two args,
	//  since we're connected to PIND3 and PIND2 only
	encoders_init(PIND3, PIND2, PIND1, PIND0);
	
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
			printf("Encoder:%d",encoders_get_counts_m1());
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
