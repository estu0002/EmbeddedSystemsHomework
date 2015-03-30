/*
 * pid.h
 *
 * Created: 3/21/2015 3:45:51 PM
 *  Author: Jesse
 */ 

#ifndef PID_H_
#define PID_H_

#include <stdbool.h>

#define PID_MODE_SPEED 0
#define PID_MODE_POSITION 1

typedef struct {
	int command;
	int measured;
	int drive;
} PIDlog;

typedef struct {
	double dState;					// last position input
	double iState;					// integrator state
	double iMin, iMax;				// min and max allowable integrator state
	double pGain;					// proportional gain
	double iGain;					// integral gain
	double dGain;					// differential gain
	
	int command;					// the command we're going for
	int measured;					// the measured value we've got

	// this will be our output signal to the motor; some examples:
	//   0 means make no change to current motor signal
	//   1 means increase the motor signal by 1 unit
	//   -2 means decrease the motor signal by 2 units
	int drive;
	
	int mode;						// 0 for speed, 1 for position
	
	bool logging_is_enabled;		// true if we should be logging, false if not

	PIDlog *log_data;				// pointer to an array of PIDlog;
	unsigned int log_max_records;	// maximum amount of records into the array
	unsigned int log_pos;			// current position in the array
} SPid;

SPid *myPID;


double updatePID(SPid *pid, double error, double position) {
	double pTerm;
	double iTerm;
	double dTerm;
	
	// calculate proportional term
	pTerm = pid->pGain * error;
	
	// no update for proportional term since it requires no reference to previous state
	
	// update integral state (with appropriate limiting)
	pid->iState += error;
	if(pid->iState > pid->iMax) {
		pid->iState = pid->iMax;
		} else if(pid->iState < pid->iMin) {
		pid->iState = pid->iMin;
	}
	
	// calculate integral term
	iTerm = pid->iGain * pid->iState;
	
	// calculate derivative term
	dTerm = pid->dGain * (position - pid->dState);
	
	// update derivative state
	pid->dState = position;
	
	return pTerm + iTerm - dTerm;
}

#endif /* PID_H_ */