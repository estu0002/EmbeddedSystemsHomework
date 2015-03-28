/*
 * motorPwm.h
 *
 * Created: 3/28/2015 11:46:01 AM
 *  Author: Jesse
 */ 


#ifndef MOTORPWM_H_
#define MOTORPWM_H_

#include <pololu/orangutan.h>
#include <math.h>
#include <stdlib.h>

#define PWM IO_D7
#define DIR IO_C7
#define FORWARD LOW
#define REVERSE HIGH

void init_motor_pwm() {
	TCCR2A = 0x03;
	TCCR2B = 0x02;

	set_digital_output(DIR, FORWARD);
	set_digital_output(PWM, LOW);
}

void my_set_m1_speed(int speed) {
	bool reverse = speed < 0;
	speed = fmin(abs(speed), 0xFF);
	OCR2A = speed;

	if (speed == 0) {
		TCCR2A &= ~(1 << COM2A1);
	}
	else {
		TCCR2A |= 1 << COM2A1;
		set_digital_output(DIR, reverse ? REVERSE : FORWARD);
	}
}


#endif /* MOTORPWM_H_ */