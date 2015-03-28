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
#include "serial.h"

// absolute encoder counts since program initialization
volatile int g_counts_m1 = 0;

// set to true if the g_counts_m1 gets overflowed 
volatile bool g_counts_m1_overflowed = false;

// set to true if the g_counts_m1 gets underflowed
volatile bool g_counts_m1_underflowed = false;

// flag set in ISR to toggle LCD update in cyclic executive
volatile bool g_releaseLcdUpdate = false;

// flag set in ISR to toggle release of speedPID in cyclic executive
volatile bool g_releaseSpeedPID = false;

// speed variables
volatile int g_speed_calc_prev_encoder_val = 0;
volatile int g_speed_calc_current_speed = 0;
	
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
	
	// var to store button press - declared outside of loop to reduce allocation overhead
	unsigned char button_pressed;
	
	// set up our speed PID
	SPid speedPID;
	memset(&speedPID, 0, sizeof(speedPID)); // zero everything out, just for good measure
	serial_speedPID = &speedPID;  // make our serial pointer overlay the actual speed PID
	speedPID.pGain = 0.05;
	
	double drive = 0.0;
	
	//double speedCommand = 2500.0;
	speedPID.command = 2500.0;
	
	// set up memory for our command input
	g_command_input = calloc(1,sizeof(struct command_input));
	
	// boiler plate serial initialization stuff
	// Set the baud rate to 9600 bits per second.  Each byte takes ten bit
	// times, so you can get at most 960 bytes per second at this speed.
	serial_set_baud_rate(USB_COMM, 9600);

	// Start receiving bytes in the ring buffer.
	serial_receive_ring(USB_COMM, receive_buffer, sizeof(receive_buffer));
	
	while(1)
	{			
		if(g_releaseLcdUpdate) {
			clear();
			lcd_goto_xy(0,0);

			printf("S:%d E:%d\n",g_speed_calc_current_speed, g_counts_m1);
			printf("C:%.0f D:%.1f",speedPID.command, drive);
			
			g_releaseLcdUpdate = false;
		}
		
		if(g_releaseSpeedPID) {
			
			drive = updatePID(&speedPID,
				(speedPID.command - g_speed_calc_current_speed),
				g_speed_calc_current_speed);
			
			adjust_m1_torque((int)drive);
			
			g_releaseSpeedPID = false;
		}
		
		button_pressed = get_single_debounced_button_press(ANY_BUTTON);
		
		switch (button_pressed) {
			case BUTTON_A:
			speedPID.command -= 100;
			break;
			case BUTTON_B:
			speedPID.command *= -1;
			break;
			case BUTTON_C:
			speedPID.command += 100;
			break;
		}
		
		// USB_COMM is always in SERIAL_CHECK mode, so we need to call this
		// function often to make sure serial receptions and transmissions
		// occur.
		serial_check();

		// Deal with any new bytes received.
		serial_check_for_new_bytes_received();
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
		if((g_counts_m1 + 1) < g_counts_m1) {
			g_counts_m1_overflowed = true;
		}
		
		g_counts_m1++;
	}
	if(minus_m1) {
		if((g_counts_m1 - 1) > g_counts_m1) {
			g_counts_m1_underflowed = true;
		}
		
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
volatile unsigned int speedPIDReleaseCounter = 0;
ISR(TIMER3_COMPA_vect) {
	// calculate the current speed and adjust for encoder count overflows
	// NOTE: if you don't do this, there is risk of getting a wildly inaccurate
	//	reading from time to time.  Depending on the timing of this inaccurate
	//	reading and the configuration of the PID controller, it could result in
	//	a violent jerk of the motor.
	if(g_counts_m1_overflowed || g_counts_m1_underflowed) {
		// we have experienced an overflow/underflow, so lie like a rug
		// pretend that the speed has not changed since last time
		g_speed_calc_current_speed = g_speed_calc_current_speed;
		
		// and reset the overflow/underflow flags
		g_counts_m1_overflowed = false;
		g_counts_m1_underflowed = false;
	} else {
		// we have not experienced an overflow, so actually calculate the speed
		g_speed_calc_current_speed = (g_counts_m1 - g_speed_calc_prev_encoder_val) * 10; // since we're on a 100ms timer, need to multiply by 10 to get counts/sec
	}
	g_speed_calc_prev_encoder_val = g_counts_m1;
	
	// release the speed PID controller
	speedPIDReleaseCounter++;
	if(speedPIDReleaseCounter == 2) {
		speedPIDReleaseCounter = 0;
		g_releaseSpeedPID = true;
	}
}
