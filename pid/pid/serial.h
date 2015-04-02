/*
 * Helper for doing serial I/O.  See the "serial2" example for details.
 *
 * Created: 3/27/2015 5:36:44 PM
 *  Author: Jesse
 */ 


#ifndef SERIAL_H_
#define SERIAL_H_

#include <stdlib.h>
#include <string.h>

#define PGAIN_INCREMENT	.5 // amount to increase/decrease pGain
#define IGAIN_INCREMENT .0001 // amount to increase/decrease iGain
#define DGAIN_INCREMENT .1  // amount to increase/decrease dGain

char receive_buffer[32];

unsigned char receive_buffer_position = 0;

char serial_send_buffer[255];

struct command_input {
	char command_code;
	char command_ref_val[7];
	uint8_t command_ref_val_pos;
};

struct command_input *g_command_input;

SPid *myPID;

void serial_wait_for_sending_to_finish()
{
	while(!serial_send_buffer_empty(USB_COMM))
	serial_check();		// USB_COMM port is always in SERIAL_CHECK mode
}

// process_received_byte: Responds to a byte that has been received on
// USB_COMM.
void serial_process_received_byte(char byte);

void serial_check_for_new_bytes_received()
{
	while(serial_get_received_bytes(USB_COMM) != receive_buffer_position)
	{
		// Process the new byte that has just been received.
		serial_process_received_byte(receive_buffer[receive_buffer_position]);

		// Increment receive_buffer_position, but wrap around when it gets to
		// the end of the buffer.
		if (receive_buffer_position == sizeof(receive_buffer)-1)
		{
			receive_buffer_position = 0;
		}
		else
		{
			receive_buffer_position++;
		}
	}
}

void s_send() {
	serial_send(USB_COMM, serial_send_buffer, strlen(serial_send_buffer));
	serial_wait_for_sending_to_finish();
}

void s_clear_send_buf() {
	memset(serial_send_buffer,0,sizeof(serial_send_buffer));
}

/*
COMMAND REFERENCE:
L/l: Start/Stop Logging (print) the values of Pr, Pm, and T.
V/v: View the current values Kd, Kp, Vm, Pr, Pm, and T
R/r : Set the reference position (use unit "counts")
S/s : Set the reference speed (use unit "counts"/sec)
P: Increase Kp by an amount of your choice*
p: Decrease Kp by an amount of your choice
D: Increase Kd by an amount of your choice
d: Decrease Kd by an amount of your choice
I: Increase Ki by an amount of your choice
i: Decrease Ki by an amount of your choice
t: Execute trajectory
*/
void process_command() {
	double ref_val = atof(g_command_input->command_ref_val);
	switch(g_command_input->command_code) {
		
		case 't':
		case 'T':
			// we need to be in position mode to execute the trajectory
			myPID->mode = PID_MODE_POSITION;
			// and we should set our command to wherever we are currently at
			myPID->command = g_counts_m1;
			
			// and enable trajectory mode
			g_trajectory_mode = true;
		break;
		
		case 'l':
		case 'L':
			if(myPID->logging_is_enabled) {
				// logging is enabled now
				
				// ...so turn it off 
				myPID->logging_is_enabled = false;
				
				// ...and dump the log
				for(int i=0;i<=myPID->log_pos;i++) {
					s_clear_send_buf();
					sprintf(serial_send_buffer, "%d, %d, %d\r\n",
						myPID->log_data[i].command,
						myPID->log_data[i].measured,
						myPID->log_data[i].drive
					);
					s_send();
				}
				
				// ...and reset the log
				memset(myPID->log_data, 0, myPID->log_max_records*sizeof(PIDlog));
				myPID->log_pos = 0;
			} else {
				// logging is disabled, enable it
				myPID->logging_is_enabled = true;
			}
		break;
		
		case 'v':
		case 'V':
			//print Kd, Kp, Vm, Pr, Pm, and T
			// Pr - desired value
			// Pm - measured value
			s_clear_send_buf();
			sprintf(serial_send_buffer, "Kp:%.3f Ki:%.4f Kd:%.2f Pr:%d Pm:%d T:%d\n",
				myPID->pGain,
				myPID->iGain,
				myPID->dGain,
				myPID->command,
				myPID->measured,
				myPID->drive
			);
			s_send();
		break;
		
		case 'r':
		case 'R':
			// R/r : Set the reference position (use unit "counts")
			myPID->command = ref_val;
			myPID->mode = PID_MODE_POSITION;
		break;
		
		case 's':
		case 'S':
			//S/s : Set the reference speed (use unit "counts"/sec)
			myPID->command = ref_val;
			myPID->mode = PID_MODE_SPEED;
		break;
		
		case 'P':
			//P: Increase Kp by an amount of your choice*
			myPID->pGain += PGAIN_INCREMENT;
		break;
		
		case 'p':
			//p: Decrease Kp by an amount of your choice
			myPID->pGain -= PGAIN_INCREMENT;
		break;
		
		case 'D':
			//D: Increase Kd by an amount of your choice
			myPID->dGain += DGAIN_INCREMENT;
		break;
		
		case 'd':
			//d: Decrease Kd by an amount of your choice
			myPID->dGain -= DGAIN_INCREMENT;
		break;
		
		case 'I':
			//I: Increase Ki by an amount of your choice
			myPID->iGain += IGAIN_INCREMENT;
		break;
		
		case 'i':
			//i: Decrease Ki by an amount of your choice
			myPID->iGain -= IGAIN_INCREMENT;
		break;
		
	}
	
	// lastly - clear the command
	memset(g_command_input,0,sizeof(struct command_input));
}


void serial_process_received_byte(char byte)
{

	switch(byte) {
		case 'l':
		case 'L':
		case 'v':
		case 'V':
		case 'r':
		case 'R':
		case 's':
		case 'S':
		case 'P':
		case 'p':
		case 'D':
		case 'd':
		case 'I':
		case 'i':
		case 'T':
		case 't':
		g_command_input->command_code = byte;
		break;
		
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		g_command_input->command_ref_val[g_command_input->command_ref_val_pos] = byte;
		g_command_input->command_ref_val_pos++;
		break;
		
		case '\r':
		process_command();
		break;
		
	}
	
	// Always echo the character back
	s_clear_send_buf();
	serial_send_buffer[0] = byte;
	s_send();
}




#endif /* SERIAL_H_ */