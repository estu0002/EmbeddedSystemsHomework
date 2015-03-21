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
	
	// initialize stuff
	init_timer0();
	init_timer3();
	init_encoder();
	
	// var to store button press - declared outside of loop to reduce allocation overhead
	unsigned char button_pressed;
	
	// set up our speed PID
	SPid speedPID;
	memset(&speedPID, 0, sizeof(speedPID)); // zero everything out, just for good measure
	speedPID.pGain = 0.05;
	
	double drive;
	
	double speedCommand = 2500.0;
	while(1)
	{			
		if(g_releaseLcdUpdate) {
			clear();
			lcd_goto_xy(0,0);
			//printf("Dr:%d,Enc:%d\n",motor_drive_signal,g_counts_m1);
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


