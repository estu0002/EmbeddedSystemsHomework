/*
 * motorDriveSignal.h
 *
 * Created: 3/21/2015 3:38:51 PM
 *  Author: Jesse
 */ 

#ifndef MOTORDRIVESIGNAL_H_
#define MOTORDRIVESIGNAL_H_

int16_t current_m1_torque = 0;

/* prototypes */
void adjust_m1_torque(int16_t adjustmentAmount);
void set_m1_torque(int16_t torqueToSet);
void reverse_m1_torque();
int16_t get_m1_torque();

/* function definitions */
void adjust_m1_torque(int16_t adjustmentAmount) {
	set_m1_torque(get_m1_torque() + adjustmentAmount);
}

void set_m1_torque(int16_t torqueToSet) {
	if(torqueToSet > 255) {
		current_m1_torque = 255;
	} else if(torqueToSet < -255) {
		current_m1_torque = -255;
	} else {
		current_m1_torque = torqueToSet;
	}
	
	my_set_m1_speed(current_m1_torque);
}

void reverse_m1_torque() {
	set_m1_torque(get_m1_torque() * -1);
}

int16_t get_m1_torque() {
	return current_m1_torque;
}

#endif /* MOTORDRIVESIGNAL_H_ */