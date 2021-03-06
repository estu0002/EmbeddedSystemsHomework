/* class4_lab - an application for the Pololu Orangutan SVP
 *
 * This application uses the Pololu AVR C/C++ Library.  For help, see:
 * -User's guide: http://www.pololu.com/docs/0J20
 * -Command reference: http://www.pololu.com/docs/0J18
 *
 * Created: 2/13/2015 11:06:00 AM
 *  Author: Jesse
 */

#include <pololu/orangutan.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "serial.h"

#undef BLINK_RED_BUSY_WAIT
#define BLINK_RED_INTERRUPT

/**** TODO - MOVE TO #include ****/
#define FOR_COUNT_10MS 5585
volatile uint32_t __ii;
#define WAIT_10MS {for(__ii=0;__ii<FOR_COUNT_10MS;__ii++);}
	
#define COMMAND_ZERO 'Z'
#define COMMAND_PRINT 'P'
#define COMMAND_TOGGLE 'T'

#define COLOR_RED 'R'
#define COLOR_YELLOW 'Y'
#define COLOR_GREEN 'G'
#define COLOR_ALL 'A'
/**** END - MOVE TO #include ****/

// global variable for controlling release of the red blink task in the cyclic executive
volatile bool g_release_red_blink = false;

// global counter variables for each LED blink
volatile uint16_t g_blink_count_red = 0;
volatile uint16_t g_blink_count_yellow = 0;
volatile uint16_t g_blink_count_green = 0;

// global count variables for red and yellow LED (since these will be multiples of 1ms and 100ms, respectively)
volatile uint16_t g_count_red = 0;
volatile uint16_t g_count_yellow = 0;

// global control variables for each color time
volatile uint16_t g_tgt_on_time_ms_red = 10;
volatile uint16_t g_tgt_on_time_ms_yellow = 100;
volatile uint16_t g_tgt_on_time_ms_green = 500;

struct command_input {
	char command_code;
	char command_color;
	char command_blink_ms[5];
	uint8_t command_blink_ms_pos;
};

struct command_input *g_command_input;

int main()
{
	lcd_init_printf();
	//clear();
	//printf("Test");
	//delay_ms(1000);
	//clear();

	// enable system interrupts
	sei();
	
	// set up memory for our command input
	g_command_input = calloc(1,sizeof(struct command_input));


	
	/***** PART 1 *****/
//	volatile long i=1;
//	unsigned long start_time = get_ms();
//	for(;i<=4000000;i++) {
//		// do nothing
//	}
//	unsigned long end_time = get_ms();
	/***** END PART 1 *****/
	
	/***** PART 2 *****/
#ifdef BLINK_RED_INTERRUPT
	// set up 8 bit timer with 1ms resolution
	
	// TCCR0B - timer counter control register B
	// FOC0A (dont change) - default 0
	// FOC0B (dont change) - default 0
	//        -
	//        -
	// WGM02  0
	// CS02   1  - clock select - prescale /1024
	// CS01   0
	// CS00   1
	// = 0000 0101
	//    0    5
	TCCR0B = 0x05;	
	
		
	// TCCR0A - timer counter control register A
	// COM0A1 1 - clear on compare match
	// COM0A0 0
	// COM0B1 (dont change) - default 0 
	// COM0B0 (dont change) - default 0
	//        -
	//        -
	// WGM01  1 - CTC, TOP from OCRA
	// WGM00  0
	// = 1000 0010
	//    8    2
	TCCR0A = 0x82;
	
	// formula to calculate OCR0A (TOP for this 8 bit timer) 20000000/(1024*(1000/g_tgt_on_time_ms_red)
	// since 20M exceeds the storage of a 16 bit integer, we divide 20M/1024 = 19531.25
	// which gives us a constant of 19531 after rounding (decision not to fool with floating point)
	// NOTE: anything >13 causes an overflow of the 8 bit OCR0A register
	//OCR0A = 19531/(1000/g_tgt_on_time_ms_red);
	// UPDATE: I was interpreting the requirements wrong.  We'll fix the interrupts to fire at 1ms
	OCR0A = 19531/(1000/1);
	
	// enable the interrupt for the timer
	// TIMSK0 - Timer/counter interrupt mask register
	//        -
	//        -
	//        -
	//        -
	//        -
	// OCIE0B (dont change)
	// OCIE0A 1
	// TOIE0  (dont change)
	// = 0000 0010
	TIMSK0 |= (1 << 1);
#endif	
	
	// Set PORTA2 (red) as output
	DDRA |= (1 << 2);
		
	// make sure we start with that red off
	PORTA &= !(1<<2);
	
	/***** END PART 2 *****/
	
	/***** PART 3 *****/
	
	// set up 16 bit timer with 1ms resolution
	
	// TCCR3B - timer counter control register B
	// ICNC3  0
	// ICES3 (dont change) - default 0
	//        -
	// WGM33  0  - WGM mode 4, CTC, TOP from OCR1A (see bits in TCCR1A also)
	// WGM32  1
	// CS32   1  - clock select - prescale /256
	// CS31   0
	// CS30   0
	// = 0000 1100
	//    0    c
	TCCR3B = 0x0c;
	
	// TCCR3A - timer counter control register A
	// COM3A1 1 - clear on compare match
	// COM3A0 0
	// COM3B1 (dont change) - default 0
	// COM3B0 (dont change) - default 0
	//        -
	//        -
	// WGM31  0 - WGM mode 4, CTC, TOP from OCR1A (see bits in TCCR1B also)
	// WGM30  0
	// = 1000 0000
	//    8    0
	TCCR3A = 0x80;
	
	// formula to calculate OCR3A (TOP for this 16 bit timer) 20000000/(256*(1000/g_tgt_on_time_ms_yellow)
	// since 20M exceeds the storage of a 16 bit integer, we divide 20M/256 = 78125
	// UPDATE: I was interpreting the requirements wrong.  We'll fix the interrupts to fire at 100ms
	//OCR3A = 78125/(1000/g_tgt_on_time_ms_yellow);
	OCR3A = 78125/(1000/100);
	
	
	// enable the interrupt for the timer
	// TIMSK3 - Timer/counter interrupt mask register
	//        -
	//        -
	// ICIE3  -
	//        -
	//        -
	// OCIE3B -
	// OCIE3A 1
	// TOIE3  (dont change)
	// = 0000 0010
	TIMSK3 |= (1 << 1);
	
	// Set PORTA2 (yellow) as output
	DDRA |= (1<<0);
	
	// make sure we start with that yellow off
	PORTA &= !(1<<0);
	/***** END PART 3 *****/
	
	/***** PART 4 *****/
	// set up PWM

	// turn on pin 5 as output - leave others unchanged
	DDRD |= (1 << 5); 

	// TCCR1A (p.128, 16.12.1)
	// COM1A1 1 - clear on compare match (set output to low level)
	// COM1A0 0
	// COM1B1 - only used for non-pwm, so don't care (default 0)
	// COM1B0 - only usef for non-pwm, so don't care (default 0)
	//        - default is 0
	//        - default is 0
	// WGM11  1 WGM1 to 14, which is Fast PWM (p.130) (implies update of OCR1A at BOTTOM)
	// WGM10  0
	// = 1000 0010
	//    8    2
	TCCR1A = 0x82;
	
	// TCCR1B (p.130, 16.12.2)
	// ICNC1  0 - thinking we don't care, so lets leave at default of 0
	// ICES1  0 - thinking we don't care, so lets leave at default of 0
	//        - default is 0
	// WGM13  1 WGM1 to 14, which is Fast PWM (p.130) (implies update of OCR1A at BOTTOM)
	// WGM12  1
	// CS12   1 - clock select - prescale /1024  (p.131, table 16-6)
	// CS11   0
	// CS10   1
	// = 0001 1101
	//    1    d
	TCCR1B = 0x1d;
		
	//spot on sawtooth where we toggle output
	// (g_tgt_on_time_ms_green*2/1000) converts ms to Hz - e.g., 500ms = 1Hz
	// (1000/(g_tgt_on_time_ms_green*2)
	// Uses formula from 16.10.2 on p.120
	OCR1A = (20000000/( (1000/(g_tgt_on_time_ms_green*2)) *2*1024))-1;
	
	//Since we're in WGM1 mode of 14, TOP is from ICR1
	ICR1 = 2*OCR1A; 
	
	// we want to generate an interrupt each time we match on output compare register 1 a
	// TIMSK1 - Timer/counter interrupt mask register
	//        -
	//        -
	// ICIE1  0
	//        -
	//        -
	// OCIE1B -
	// OCIE1A 1 - compare A match interrupt enabled
	// TOI13  1 - timer1 overflow interrupt enabled 
	// = 0000 0011
	//    0    3
	TIMSK1 = 0x03;
	

	/***** END PART 4 *****/
	
	// boiler plate serial initialization stuff
	// Set the baud rate to 9600 bits per second.  Each byte takes ten bit
	// times, so you can get at most 960 bytes per second at this speed.
	serial_set_baud_rate(USB_COMM, 9600);

	// Start receiving bytes in the ring buffer.
	serial_receive_ring(USB_COMM, receive_buffer, sizeof(receive_buffer));

	while(1)
	{
#ifdef BLINK_RED_INTERRUPT
		if(g_release_red_blink) {
			g_release_red_blink = false;
			// red is on PORTA0, toggle signal
			PORTA ^= (1<<2);
			g_blink_count_red++;
		}
#endif

#ifdef BLINK_RED_BUSY_WAIT
		for(int i=0;i<50;i++) {
			WAIT_10MS
		}
		// red is on PORTA0, toggle signal
		PORTA ^= (1<<2);
		g_blink_count_red++;
#endif
		
		// USB_COMM is always in SERIAL_CHECK mode, so we need to call this
		// function often to make sure serial receptions and transmissions
		// occur.
		serial_check();

		// Deal with any new bytes received.
		serial_check_for_new_bytes_received();
	}
}

#ifdef BLINK_RED_INTERRUPT
/* ISR for red LED */
ISR(TIMER0_COMPA_vect) {
	cli();
	
	g_count_red++;
	if(g_count_red == (g_tgt_on_time_ms_red/1)) {
		g_release_red_blink = true;
		g_count_red = 0;	
	}
	
	sei();
}
#endif

/* ISR for yellow LED */
ISR(TIMER3_COMPA_vect) {
	cli();
	
	// for experiment busy-wait in yellow LED ISR
//	for(int i=0;i<51;i++) {
//		WAIT_10MS
//	}
	
	g_count_yellow++;
	
	if(g_count_yellow == (g_tgt_on_time_ms_yellow/100)) {
		// blink directly in the ISR per spec
		PORTA ^= (1<<0);
		g_count_yellow = 0;
		
		g_blink_count_yellow++;
	}
	
	sei();
}

/* ISRs for green LED */
ISR(TIMER1_COMPA_vect) {
	cli();
	
	// for experiment busy-wait in green LED ISR
//	for(int i=0;i<51;i++) {
//		WAIT_10MS
//	}
	
	g_blink_count_green++;
	
	sei();	
}

ISR(TIMER1_OVF_vect) {
	cli();
	
	g_blink_count_green++;
	
	sei();
}

void process_command() {
	uint16_t tgt_on_time_ms = atoi(g_command_input->command_blink_ms);
	
	switch(g_command_input->command_code) {
		case COMMAND_PRINT:
			switch(g_command_input->command_color) {
				case COLOR_ALL:
					memset(serial_send_buffer,0,sizeof(serial_send_buffer));
					sprintf(serial_send_buffer, "\r\nR:%u Y:%u G:%u\r\n", g_blink_count_red, g_blink_count_yellow, g_blink_count_green);
					serial_wait_for_sending_to_finish();
					serial_send(USB_COMM, serial_send_buffer, strlen(serial_send_buffer));			
				break;
			
				case COLOR_RED:
					memset(serial_send_buffer,0,sizeof(serial_send_buffer));
					sprintf(serial_send_buffer, "\r\nR:%u\r\n", g_blink_count_red);
					serial_wait_for_sending_to_finish();
					serial_send(USB_COMM, serial_send_buffer, strlen(serial_send_buffer));
				break;
			
				case COLOR_YELLOW:
					memset(serial_send_buffer,0,sizeof(serial_send_buffer));
					sprintf(serial_send_buffer, "\r\nY:%u\r\n", g_blink_count_yellow);
					serial_wait_for_sending_to_finish();
					serial_send(USB_COMM, serial_send_buffer, strlen(serial_send_buffer));
				break;
			
				case COLOR_GREEN:
					memset(serial_send_buffer,0,sizeof(serial_send_buffer));
					sprintf(serial_send_buffer, "\r\nG:%u\r\n", g_blink_count_green);
					serial_wait_for_sending_to_finish();
					serial_send(USB_COMM, serial_send_buffer, strlen(serial_send_buffer));
				break;
			}
		break;
		
		case COMMAND_ZERO:
			switch(g_command_input->command_color) {
				case COLOR_ALL:
					g_blink_count_red = 0;
					g_blink_count_yellow = 0;
					g_blink_count_green = 0;
				break;
			
				case COLOR_RED:
					g_blink_count_red = 0;
				break;
			
				case COLOR_YELLOW:
					g_blink_count_yellow = 0;
				break;
			
				case COLOR_GREEN:
					g_blink_count_green = 0;
				break;
			}
			memset(serial_send_buffer,0,sizeof(serial_send_buffer));
			sprintf(serial_send_buffer, "\r\nZero Complete\r\n");
			serial_wait_for_sending_to_finish();
			serial_send(USB_COMM, serial_send_buffer, strlen(serial_send_buffer));
		break;
		
		case COMMAND_TOGGLE:
			switch(g_command_input->command_color) {
				case COLOR_ALL:
					g_tgt_on_time_ms_red = tgt_on_time_ms;
					g_tgt_on_time_ms_yellow = tgt_on_time_ms;
					g_tgt_on_time_ms_green = tgt_on_time_ms;
				break;
			
				case COLOR_RED:
					g_tgt_on_time_ms_red = tgt_on_time_ms;
				break;
			
				case COLOR_YELLOW:
					g_tgt_on_time_ms_yellow = tgt_on_time_ms;
				break;
			
				case COLOR_GREEN:
					g_tgt_on_time_ms_green = tgt_on_time_ms;
				break;
			}
			// just in case they updated the green rate, let's update the compare registers for the PWM
			OCR1A = (20000000/( (1000/(g_tgt_on_time_ms_green*2)) *2*1024))-1;
			ICR1 = 2*OCR1A;
			
			memset(serial_send_buffer,0,sizeof(serial_send_buffer));
			sprintf(serial_send_buffer, "\r\nToggle Complete\r\n");
			serial_wait_for_sending_to_finish();
			serial_send(USB_COMM, serial_send_buffer, strlen(serial_send_buffer));
		break;
	}
	
	// lastly - clear the command
	memset(g_command_input,0,sizeof(struct command_input));
}

void serial_process_received_byte(char byte)
{
	switch(byte) {
		// If the character z,Z,p,P,t,T received, then we are dealing with a command code
		case 'z':
		case 'Z':
			g_command_input->command_code = COMMAND_ZERO;
		break;
		
		case 'p':
		case 'P':
			g_command_input->command_code = COMMAND_PRINT;
		break;
		
		case 't':
		case 'T':
			g_command_input->command_code = COMMAND_TOGGLE;
		break;
		
		// If the character r,R,y,Y,g,G,a,A received, then we are dealing with a color code
		case 'r':
		case 'R':
			g_command_input->command_color = COLOR_RED;
		break;
		
		case 'y':
		case 'Y':
			g_command_input->command_color = COLOR_YELLOW;
		break;
		
		case 'g':
		case 'G':
			g_command_input->command_color = COLOR_GREEN;
		break;
		
		case 'a':
		case 'A':
			g_command_input->command_color = COLOR_ALL;
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
			g_command_input->command_blink_ms[g_command_input->command_blink_ms_pos] = byte;
			g_command_input->command_blink_ms_pos++;
		break;
		
		case '\r':
			process_command();
		break;
		
	}
	
	// Always echo the character back
	serial_wait_for_sending_to_finish();
	serial_send_buffer[0] = byte;
	serial_send(USB_COMM, serial_send_buffer, 1);
}

