/* pid - an application for the Pololu Orangutan SVP
 *
 * PID controller for a motor
 *
 * Created: 3/21/2015 2:17:45 PM
 *  Author: Jesse
 */

#undef ASSIGNMENT_PART_2      
/* DEFINED: set timer 3 to 10ms
								  UNDEFD: set timer 3 to 100ms */
#undef PID_RELEASE_EVERY_10TH 
/* DEFINED: release the PID loop every 10th time TIMER3 fires
								  UNDEFD: release the PID loop every time TIMER3 fires */

#include <pololu/orangutan.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "motorPwm.h"
#include "motorDriveSignal.h"
#include "pid.h"
#include "init.h"

// absolute encoder counts since program initialization
volatile int g_counts_m1 = 0;

#include "interpolator.h"
#include "serial.h"

// set to true if the g_counts_m1 gets overflowed 
volatile bool g_counts_m1_overflowed = false;

// set to true if the g_counts_m1 gets underflowed
volatile bool g_counts_m1_underflowed = false;

// flag set in ISR to toggle LCD update in cyclic executive
volatile bool g_releaseLcdUpdate = false;

// flag set in ISR to toggle release of PID in cyclic executive
volatile bool g_releasePID = false;

// flag set in ISR to toggle release of trajectory interpolator in cyclic executive
volatile bool g_releaseTrajectory = false;

// speed variables
volatile int g_speed_calc_prev_encoder_val = 0;
volatile int g_speed_calc_current_speed = 0;

int main()
{
	play_from_program_space(PSTR(">g32>>c32"));  // Play welcoming notes.
	
	lcd_init_printf();
	clear();
	
	// initialize stuff
	init_timer0();
	init_timer3();
	init_encoder();
	
	// initialize motor PWM
	init_motor_pwm();
	
	// var to store button press - declared outside of loop to reduce allocation overhead
	unsigned char button_pressed;
	
	// set up our PID
	myPID = calloc(1, sizeof(SPid));
	memset(myPID, 0, sizeof(SPid)); // zero everything out, just for good measure
	
	// seed the PID with some default values
	myPID->pGain = 2.0;
	myPID->dGain = 4.6;
	myPID->iMax = 500.0;
	myPID->iMin = -500.0;
	
	// command is what we're trying to get our system to do
	myPID->command = 0.0;
	
	// set up memory for our command input
	g_command_input = calloc(1, sizeof(struct command_input));
	
	// set up max number of log records
	myPID->log_max_records = 500;
	
	// set up memory for our logging and zero it out
	myPID->log_data = calloc(myPID->log_max_records, sizeof(PIDlog));
	memset(myPID->log_data, 0, myPID->log_max_records*sizeof(PIDlog));
	
	// boiler plate serial initialization stuff
	serial_set_baud_rate(USB_COMM, 115200);

	// Start receiving bytes in the ring buffer.
	serial_receive_ring(USB_COMM, receive_buffer, sizeof(receive_buffer));
	
	while(1)
	{			
		if(g_releaseLcdUpdate) {
			clear();
			lcd_goto_xy(0,0);

			printf("S:%d E:%d\n",g_speed_calc_current_speed, g_counts_m1);
			//printf("C:%d D:%d M:%d",myPID->command, myPID->drive, myPID->measured);
			printf("C:%d D:%d",myPID->command, myPID->drive);
			
			g_releaseLcdUpdate = false;
		}
		
		if(g_releasePID) {
			
			// set our measured value based on current mode
			if(myPID->mode == PID_MODE_SPEED) {
				myPID->measured = g_speed_calc_current_speed;
			} else if(myPID->mode == PID_MODE_POSITION) {
				myPID->measured = g_counts_m1;
			}
			
			// calculate the drive for the motor
			myPID->drive = updatePID(myPID,
					(myPID->command - myPID->measured),
					myPID->measured);
			
			// log the current state of things
			if(myPID->logging_is_enabled) {
				if(myPID->log_pos < myPID->log_max_records) {
					myPID->log_data[myPID->log_pos].command = myPID->command;
					myPID->log_data[myPID->log_pos].measured = myPID->measured;
					myPID->log_data[myPID->log_pos].drive = myPID->drive;
				
					myPID->log_pos++;
				}
			}
			
			// update the motor torque signal
			if(myPID->mode == PID_MODE_SPEED) {
				// for speed mode, we want to sustain speed so use incremental adjustments
				adjust_m1_torque(myPID->drive);				
			} else if(myPID->mode == PID_MODE_POSITION) {
				// for position mode, we want to get that signal down to zero fast, so use an absolute value
				set_m1_torque(myPID->drive);
			}

			
			g_releasePID = false;
		}
		
		button_pressed = get_single_debounced_button_press(ANY_BUTTON);
		
		switch (button_pressed) {
			case BUTTON_A:
			myPID->command -= 100;
			break;
			case BUTTON_B:
			myPID->command *= -1;
			break;
			case BUTTON_C:
			myPID->command += 100;
			break;
		}
		
		// USB_COMM is always in SERIAL_CHECK mode, so we need to call this
		// function often to make sure serial receptions and transmissions
		// occur.
		serial_check();

		// Deal with any new bytes received.
		serial_check_for_new_bytes_received();

		if(g_releaseTrajectory) {		
			// handle trajectory mode if we're in it
			if(g_trajectory_mode) {
				if(interp_current_target_index == -1) {
					// we haven't started on our trajectory yet
					// record the starting position
					interp_start_pos = g_counts_m1;
				
					// and advance our index to the first trajectory
					interp_current_target_index = 0;
				}
			
			
				if(abs(g_counts_m1 - myPID->command) <= 5) {
					// we are where we wanted to be within our threshold
					// (in this case we have just picked 5 to be that threshold)
					
					// grab the time right now
					unsigned long now = get_ms();
					if(interp_ms_at_trajectory == 0) {
						interp_ms_at_trajectory = now;
					} else if((now - interp_ms_at_trajectory) >= 500) {
						// we have been at this trajectory for 500ms, let's go to the next one
						myPID->command = interp_start_pos + interp_trajectory[interp_current_target_index];
						
						interp_current_target_index++;
						interp_ms_at_trajectory = 0;
						
						if(interp_current_target_index == interp_trajectory_size) {
							// we are done with our trajectories, so bail out of this mode
							g_trajectory_mode = false;
							
							// and reset things in case we need to enter this mode again
							interp_current_target_index = -1;
						}
					}									
				}								
			}
			
			g_releaseTrajectory = false;
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
volatile unsigned int PIDReleaseCounter = 0;
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
#ifndef ASSIGNMENT_PART_2
		g_speed_calc_current_speed = (g_counts_m1 - g_speed_calc_prev_encoder_val) * 10; // since we're on a 100ms timer, need to multiply by 10 to get counts/sec
#else
		g_speed_calc_current_speed = (g_counts_m1 - g_speed_calc_prev_encoder_val) * 100; // since we're on a 10ms timer, need to multiply by 100 to get counts/sec
#endif
	}
	g_speed_calc_prev_encoder_val = g_counts_m1;
	
#ifdef PID_RELEASE_EVERY_10TH
	// release the speed PID controller every 100ms; since we're on a 10ms timer, do it every 10th time
	PIDReleaseCounter++;
	if(PIDReleaseCounter == 10) {
		PIDReleaseCounter = 0;
		g_releasePID = true;
	}
#else
	g_releasePID = true;
#endif

	// release the trajectory interpolator
	g_releaseTrajectory = true;
}
