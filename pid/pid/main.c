/* pid - an application for the Pololu Orangutan SVP
 *
 * PID controller for a motor
 *
 * Created: 3/21/2015 2:17:45 PM
 *  Author: Jesse
 */


#include <pololu/orangutan.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "motorDriveSignal.h"
#include "pid.h"
#include "init.h"

// absolute encoder counts since program initialization
volatile int16_t g_counts_m1 = 0;

// flag set in ISR to toggle LCD update in cyclic executive
volatile bool g_releaseLcdUpdate = false;

// flag set in ISR to toggle release of speedPID in cyclic executive
volatile bool g_releaseSpeedPID = false;

// speed variables
volatile long g_speed_calc_prev_encoder_val = 0;
volatile long g_speed_calc_current_speed = 0;

int main()
{
	play_from_program_space(PSTR(">g32>>c32"));  // Play welcoming notes.
	
	lcd_init_printf();
	clear();
	printf("test");
	
	// initialize stuff
	init_timer0();
	init_timer3();
	init_encoder();
	
	printf("poop");
	
	// var to store button press - declared outside of loop to reduce allocation overhead
	unsigned char button_pressed;
	
	// set up our speed PID
	SPid speedPID;
	memset(&speedPID, 0, sizeof(speedPID)); // zero everything out, just for good measure
	speedPID.pGain = 0.05;
	
	double drive = 0.0;
	
	double speedCommand = 2500.0;
	while(1)
	{			
		if(g_releaseLcdUpdate) {
			clear();
			lcd_goto_xy(0,0);
			printf("Speed:%d\n",g_speed_calc_current_speed);
			printf("Drive:%.1f",drive);
			g_releaseLcdUpdate = false;
		}
		
		if(g_releaseSpeedPID) {
			
			drive = updatePID(&speedPID,
				(speedCommand - g_speed_calc_current_speed),
				g_speed_calc_current_speed);
			
			adjust_m1_torque((int)drive);
			
			g_releaseSpeedPID = false;
		}
		
		button_pressed = get_single_debounced_button_press(ANY_BUTTON);
		
		switch (button_pressed) {
			case BUTTON_A:
			adjust_m1_torque(-5);
			break;
			case BUTTON_B:
			reverse_m1_torque();
			break;
			case BUTTON_C:
			adjust_m1_torque(5);
			break;
		}
	}
}

/********************* INTERRUPT SERVICE ROUTINES & COMPANY *****************************/

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
	g_releaseLcdUpdate = true;
}

// ISR for timer 3
volatile uint8_t speedPIDReleaseCounter = 0;
ISR(TIMER3_COMPA_vect) {
	// calculate the current speed and adjust for encoder count overflows
	// NOTE: if you don't do this, there is risk of getting a wildly inaccurate
	//	reading from time to time.  Depending on the timing of this inaccurate
	//	reading and the configuration of the PID controller, it could result in
	//	a violent jerk of the motor.
	if(g_counts_m1 > g_speed_calc_prev_encoder_val) {
		// we have not experienced an overflow, so actually calculate the speed
		g_speed_calc_current_speed = (g_counts_m1 - g_speed_calc_prev_encoder_val) * 10L; // since we're on a 100ms timer, need to multiply by 10 to get counts/sec
		} else {
		// we have experienced an overflow, so lie like a rug
		// pretend that the speed has not changed since last time
		g_speed_calc_current_speed = g_speed_calc_current_speed;
	}
	g_speed_calc_prev_encoder_val = g_counts_m1;
	
	
	// release the speed PID controller
	speedPIDReleaseCounter++;
	if(speedPIDReleaseCounter == 2) {
		speedPIDReleaseCounter = 0;
		g_releaseSpeedPID = true;
	}
}