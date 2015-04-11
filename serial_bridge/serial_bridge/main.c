#include <pololu/orangutan.h>  
#include <stdio.h>

#define BAUD_RATE 38400
/*  
 * Serial bridge - use the Orangutan to bridge serial communication between its
 * USB serial to its UART0 serial.
 *
 * UART0 is:
 * RX  PD0
 * TX  PD1
 */   


char uart0_receive_buffer[32];
char usb_comm_receive_buffer[32];

// receive_buffer_position
unsigned char uart0_receive_buffer_position = 0;
unsigned char usb_comm_receive_buffer_position = 0;

// send_buffer
char uart0_send_buffer[32];
char usb_comm_send_buffer[32];

// wait_for_sending_to_finish:  Waits for the bytes in the send buffer to
// finish transmitting on USB_COMM or UART0
void wait_for_sending_to_finish() {
	while(!serial_send_buffer_empty(UART0) || !serial_send_buffer_empty(USB_COMM)) {
		serial_check();
	}
}



void check_for_new_bytes_received() {
	// input:  UART0
	// output: USB_COMM
	while(serial_get_received_bytes(UART0) != uart0_receive_buffer_position) {
		// Process the new byte that has just been received.
		usb_comm_send_buffer[0] = uart0_receive_buffer[uart0_receive_buffer_position];
		serial_send(USB_COMM, usb_comm_send_buffer, 1);

		// Increment receive_buffer_position, but wrap around when it gets to
		// the end of the buffer. 
		if (uart0_receive_buffer_position == sizeof(uart0_receive_buffer)-1) {
			uart0_receive_buffer_position = 0;
		} else {
			uart0_receive_buffer_position++;
		}
	}
	
	// input:  USB_COMM
	// output: UART0
	while(serial_get_received_bytes(USB_COMM) != usb_comm_receive_buffer_position) {
		// Process the new byte that has just been received.
		uart0_send_buffer[0] = usb_comm_receive_buffer[usb_comm_receive_buffer_position];
		serial_send(UART0, uart0_send_buffer, 1);

		// Increment receive_buffer_position, but wrap around when it gets to
		// the end of the buffer. 
		if (usb_comm_receive_buffer_position == sizeof(usb_comm_receive_buffer)-1) {
			usb_comm_receive_buffer_position = 0;
		} else {
			usb_comm_receive_buffer_position++;
		}
	}
}

int main()
{
//	play_from_program_space(PSTR(">g32>>c32"));
	lcd_init_printf();
	clear();
	printf("Silly Pants");
	
	// power PA3 pin - may be used to power peripheral
	DDRA |= (1<<3);
	PORTA ^= (1<<3);


	// Set the baud rate; make life easy and set it the same for
	// both ends of the bridge
	serial_set_baud_rate(UART0, BAUD_RATE);
	serial_set_baud_rate(USB_COMM, BAUD_RATE);
	
	// Set UART0 to SERIAL_CHECK mode - default is interrupt driven
	// NOTE: USB_COMM is in SERIAL_CHECK by default (and always)
	serial_set_mode(UART0,SERIAL_CHECK);

	// Start receiving bytes in the ring buffers
	serial_receive_ring(UART0, uart0_receive_buffer, sizeof(uart0_receive_buffer));
	serial_receive_ring(USB_COMM, usb_comm_receive_buffer, sizeof(usb_comm_receive_buffer));

    while(1)
    {
		// see https://www.pololu.com/docs/0J18/10
		serial_check();

		// Deal with any new bytes received.
		check_for_new_bytes_received();

		// update something on the LCD about once a second so we know if the board freezes up
		if(get_ms()%1000L < 100) {
			clear();
			printf("%u",get_ms());
		}
    }
}