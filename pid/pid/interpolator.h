/*
 * interpolator.h
 *
 * Created: 4/1/2015 4:25:22 PM
 *  Author: Jesse
 */ 


#ifndef INTERPOLATOR_H_
#define INTERPOLATOR_H_

#include <stdbool.h>

// starting position: encoder value before we go to our first trajectory
int interp_start_pos;

// our trajectories in counts
int interp_trajectory[3] = {
	562,	        // 562   counts for 90  degrees
	562-2248,	    // -2248 counts for 360 degrees
	572-2248+31      // 31    counts for 5   degrees
};

// number of interpolation trajectories
int interp_trajectory_size = 3;

// which of the trajectories are we on?  -1 means we haven't started yet
int interp_current_target_index = -1;

// a mode indicator to reflect if we're working on trajectories or not
bool g_trajectory_mode = false;

unsigned long interp_ms_at_trajectory = 0;

#endif /* INTERPOLATOR_H_ */