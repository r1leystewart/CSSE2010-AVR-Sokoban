/*
 * buzzer.c
 *
 *  Author: Riley Stewart
 */ 

#include "buzzer.h"
#define F_CPU 8000000UL	// 8MHz
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdbool.h>


// For a given frequency (Hz), return the clock period (in terms of the
// number of clock cycles of a 1MHz clock)
uint16_t freq_to_clock_period(uint16_t freq) {
	return (1000000UL / freq);	// UL makes the constant an unsigned long (32 bits)
	// and ensures we do 32 bit arithmetic, not 16
}

// Return the width of a pulse (in clock cycles) given a duty cycle (%) and
// the period of the clock (measured in clock cycles)
uint16_t duty_cycle_to_pulse_width(float dutycycle, uint16_t clockperiod) {
	return (dutycycle * clockperiod) / 100;
}

void init_buzzer(void) {
	uint16_t freq = 2000;	// Hz
	float dutycycle = 2;	// %
	uint16_t clockperiod = freq_to_clock_period(freq);
	uint16_t pulsewidth = duty_cycle_to_pulse_width(dutycycle, clockperiod);

	// Make pin OC2B be an output
	//DDRD = (1 << 6);
	//Done by project.c

	// Set the maximum count value for timer/counter 2 to be one less than the clockperiod
	OCR2A = 0;

	// Set up timer/counter 2 for Fast PWM, counting from 0 to the value in OCR2A
	// before reseting to 0. Count at 1MHz (CLK/8).
	// Configure output OC2B to be clear on compare match and set on timer/counter
	// overflow (non-inverting mode).
	TCCR2A = (1 << COM2A1) | (1 << COM2B1) | (1 << WGM21) | (1 << WGM20);
	TCCR2B = (1 << WGM22) | (1 << CS21);
}

void play_move_sound(bool enabled) {
	if (enabled) {
		OCR2A = freq_to_clock_period(2000);
		_delay_ms(80);
		OCR2A = 0;
	}
}

void play_start_sound(bool enabled) {
	if (enabled) {
		OCR2A = freq_to_clock_period(2000);
		_delay_ms(300);
		OCR2A = freq_to_clock_period(400);
		_delay_ms(300);
		OCR2A = 0;
	}
}

void play_victory_sound(bool enabled) {
	if (enabled) {
		OCR2A = freq_to_clock_period(2000);
		_delay_ms(300);
		OCR2A = freq_to_clock_period(5000);
		_delay_ms(300);
		OCR2A = freq_to_clock_period(2000);
		_delay_ms(300);
		OCR2A = freq_to_clock_period(2500);
		_delay_ms(300);
		OCR2A = 0;
	}
}