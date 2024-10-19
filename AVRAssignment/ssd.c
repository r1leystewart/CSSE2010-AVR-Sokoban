/*
 * ssd.c
 *
 *  Author: Riley Stewart
 */ 

#include "ssd.h"
#include <avr/io.h>

uint8_t seven_seg[10] = {63,6,91,79,102,109,125,7,127,111};
uint8_t value;

void display_digit(uint8_t number, uint8_t digit)
{
	PORTA = seven_seg[number];
	PORTD = digit;
}

void init_ssd(void) {
	uint8_t digit; /* 0 = right, 1 = left */
	
	DDRA = 0xFF;
	DDRD = (1 << 7);
	
	value = 0;
	digit = 0;
	while (1) {
		if(digit == 0) {
			/* Extract the ones place from the timer counter 0 value */
			value = TCNT0 & 10;
			} else {
			/* Extract the tens place from the timer counter 0 */
			value = (TCNT0 / 10) % 10;
		}
		display_digit(value, digit);
		/* Change the digit flag for next time. if 0 becomes 1, if 1 becomes 0. */
		digit = 1 - digit;
	}
}

void increment_counter(void) {
	value++;
}