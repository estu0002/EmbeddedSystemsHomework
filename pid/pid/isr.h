/*
 * isr.h
 *
 * Created: 3/21/2015 4:27:42 PM
 *  Author: Jesse
 */ 


#ifndef ISR_H_
#define ISR_H_

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

#endif /* ISR_H_ */