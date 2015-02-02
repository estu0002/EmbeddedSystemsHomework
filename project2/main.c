/* project2 - an application for the Pololu Orangutan SVP
 *
 * This application assumes a green LED is in DDRD3 and a red LED is in DDRD2.
 *
 * Top button toggles whether or not to blink a LED.
 * Bottom button toggles which LED is in blink mode.
 * Serial input of character '+' increases blink interval.
 * Serial input of character '-' decreases blink interval.
 *
 * Created: 1/30/2015 11:17:38 AM
 *  Author: Jesse Estum
 */


#include <pololu/orangutan.h>
#include <stdbool.h>
#include <stdio.h>

int blink_interval_ms = 1000;
int blink_interval_step_size_ms = 10;   

// receive_buffer: A ring buffer that we will use to receive bytes on USB_COMM.
// The OrangutanSerial library will put received bytes in to
// the buffer starting at the beginning (receiveBuffer[0]).
// After the buffer has been filled, the library will automatically
// start over at the beginning.
char receive_buffer[32];

// receive_buffer_position: This variable will keep track of which bytes in the receive buffer
// we have already processed.  It is the offset (0-31) of the next byte
// in the buffer to process.
unsigned char receive_buffer_position = 0;

// send_buffer: A buffer for sending bytes on USB_COMM.
char send_buffer[32];

// wait_for_sending_to_finish:  Waits for the bytes in the send buffer to
// finish transmitting on USB_COMM.  We must call this before modifying
// send_buffer or trying to send more bytes, because otherwise we could
// corrupt an existing transmission.
void wait_for_sending_to_finish()
{
	while(!serial_send_buffer_empty(USB_COMM))
		serial_check();		// USB_COMM port is always in SERIAL_CHECK mode
}

// update the blink interval by the specified amount, but always keep it above zero.
// as the frequency changes, update the LCD to indicate this
void update_blink_interval(int update_amount_ms) {
	if((blink_interval_ms + update_amount_ms) > 0) {
		blink_interval_ms += update_amount_ms;
		lcd_goto_xy(8,1);
		printf("F:%05i",blink_interval_ms);
	}
}

// process_received_byte: Responds to a byte that has been received on
// USB_COMM. 
void process_received_byte(char byte)
{
	switch(byte)
	{
		// increase the blink interval by predefined blink_interval_step_size_ms
		case '+':
			update_blink_interval(blink_interval_step_size_ms);
			break;

		// decrease the blink interval by predefined blink_interval_step_size_ms
		case '-':
			update_blink_interval((-1)*blink_interval_step_size_ms);
			
		default:
			break;
	}
}

// straight from the serial example
void check_for_new_bytes_received()
{
	while(serial_get_received_bytes(USB_COMM) != receive_buffer_position)
	{
		// Process the new byte that has just been received.
		process_received_byte(receive_buffer[receive_buffer_position]);

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


int main()
{
	
	// Set the baud rate to 9600 bits per second.  Each byte takes ten bit
	// times, so you can get at most 960 bytes per second at this speed.
	serial_set_baud_rate(USB_COMM, 9600);

	// Start receiving bytes in the ring buffer.
	serial_receive_ring(USB_COMM, receive_buffer, sizeof(receive_buffer));
	
	play_from_program_space(PSTR(">g32>>c32"));       // why not play welcoming notes?

	bool led_color_selected_is_red = 0;				  // 1 for red, 0 for green
	bool we_are_blinking = 1;						  // 1 for blinking the LED, 0 for not
	
	// print something pretty to make this program a little more user friendly
	lcd_init_printf();
	clear();
	printf("Program 2\nJesse Estum");
	delay_ms(1000);
	clear();
	
	printf("T-Blink\n");
	printf("B-Color");

	// set DDRD pin 3 and pin 2 to output
	DDRD |= ((1 << 3) | (1 << 2));

	// set to zero
	time_reset();
	
	// program loop
	while(1)
	{
		// USB_COMM is always in SERIAL_CHECK mode, so we need to call this
		// function often to make sure serial receptions and transmissions
		// occur.
		serial_check();

		// Deal with any new bytes received.
		check_for_new_bytes_received();
		
		// grab a button press
		// NOTE: it's important to scope this var to the current loop iteration otherwise
		//       we will get seemingly "sticky" buttons
		unsigned char button = get_single_debounced_button_press(ANY_BUTTON);
		
		// update state variables based on button pressed, if any
		if(button == BOTTOM_BUTTON) {
			// bottom button press - toggle blinking of different color LED
			led_color_selected_is_red = !led_color_selected_is_red;	
		} else if(button == TOP_BUTTON) {
			// top button press - toggle blinking of LED at all
			we_are_blinking = !we_are_blinking;
		}
		
		// keep track of blink timing using the built in clock available with get_ms()
		if(we_are_blinking && (get_ms() >= blink_interval_ms)) { 
			// made it inside the block so we must blink
					
			// update the appropriate LED (red XOR green)
			unsigned char bitmask = 0;
			if(led_color_selected_is_red) {
				bitmask = (1 << 2);
			} else {
				bitmask = (1 << 3);
			}
			
			PORTD ^= bitmask;
			
			// reset timer
			time_reset();
		}

	}
}
