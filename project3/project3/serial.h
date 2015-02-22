/*
 * serial.h
 * 
 * Helper for doing serial I/O.  See the "serial2" example for details.
 *
 *
 * Created: 2/16/2015 2:52:32 PM
 *  Author: Jesse
 */ 

char receive_buffer[32];

unsigned char receive_buffer_position = 0;

char serial_send_buffer[32];


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