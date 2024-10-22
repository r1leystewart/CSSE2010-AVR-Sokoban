/*
 * project.c
 *
 * Authors: Peter Sutton, Luke Kamols, Jarrod Bennett, Cody Burnett,
 *          Bradley Stone, Yufeng Gao
 * Modified by: Riley Stewart
 *
 * Main project event loop and entry point.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#define F_CPU 8000000UL
#include <util/delay.h>

#include "game.h"
#include "startscrn.h"
#include "ledmatrix.h"
#include "buttons.h"
#include "serialio.h"
#include "terminalio.h"
#include "timer0.h"
#include "timer1.h"
#include "timer2.h"
#include "buzzer.h"
#include "joystick.h"


// Function prototypes - these are defined below (after main()) in the order
// given here.
void initialise_hardware(void);
void start_screen(void);
void new_game(int level);
void play_game(void);
void handle_game_over(void);

//Global variable step counter
uint8_t step_counter;

//Global variable play time in seconds
uint8_t play_time;

//Global variable ssd numbers
uint8_t seven_seg[10] = {63,6,91,79,102,109,125,7,127,111};
	
//Global variable level
uint8_t current_level;

//Global variable, turns sound effects on or off
bool buzzer_enabled;

//Global variable rest value of joystick
uint8_t rest_value_x;
uint8_t rest_value_y;

/////////////////////////////// main //////////////////////////////////
int main(void)
{
	//Set a seed for the random number generator
	srand(get_current_time());
	
	// Setup hardware and callbacks. This will turn on interrupts.
	initialise_hardware();

	// Show the start screen. Returns when the player starts the game.
	start_screen();
	
	//Set the level to 1
	current_level = 1;
	
	//Enable buzzer sounds
	buzzer_enabled = true;

	// Loop forever and continuously play the game.
	while (1)
	{
		new_game(current_level);
		play_game();
		handle_game_over();
	}
}

void initialise_hardware(void)
{
	init_ledmatrix();
	init_buttons();
	init_serial_stdio(19200, false);
	init_timer0();
	init_timer1();
	init_timer2();
	init_buzzer();
	init_joystick();

	// Turn on global interrupts.
	sei();
	
	//Set rest values for joystick (not working)
	ADMUX &= ~1;
	ADCSRA |= (1<<ADSC);
	while(ADCSRA & (1<<ADSC)) {;}
	rest_value_x = ADC;
	
	ADMUX |= 1;
	ADCSRA |= (1<<ADSC);
	while(ADCSRA & (1<<ADSC)) {;}
	rest_value_y = ADC;
}

void start_screen(void)
{
	// Hide terminal cursor and set display mode to default.
	hide_cursor();
	normal_display_mode();

	// Clear terminal screen and output the title ASCII art.
	clear_terminal();
	display_terminal_title(3, 5);
	move_terminal_cursor(11, 5);
	// Change this to your name and student number. Remember to remove the
	// chevrons - "<" and ">"!
	printf_P(PSTR("CSSE2010/7201 Project by Riley Stewart - 48828662"));

	// Setup the start screen on the LED matrix.
	setup_start_screen();

	// Clear button presses registered as the result of powering on the
	// I/O board. This is just to work around a minor limitation of the
	// hardware, and is only done here to ensure that the start screen is
	// not skipped when you power cycle the I/O board.
	clear_button_presses();

	// Wait until a button is pushed, or 's'/'S' is entered.
	while (1)
	{
		// Check for button presses. If any button is pressed, exit
		// the start screen by breaking out of this infinite loop.
		if (button_pushed() != NO_BUTTON_PUSHED)
		{
			break;
		}

		// No button was pressed, check if we have terminal inputs.
		if (serial_input_available())
		{
			// Terminal input is available, get the character.
			int serial_input = fgetc(stdin);

			// If the input is 's'/'S', exit the start screen by
			// breaking out of this loop.
			if (serial_input == 's' || serial_input == 'S')
			{
				break;
			}
		}

		// No button presses and no 's'/'S' typed into the terminal,
		// we will loop back and do the checks again. We also update
		// the start screen animation on the LED matrix here.
		update_start_screen();
	}
}

void new_game(int level)
{
	// Clear the serial terminal.
	hide_cursor();
	clear_terminal();

	// Initialise the game and display.
	initialise_game(level);
	move_terminal_cursor(10, 1);
	printf("Level: %d", current_level);
	
	//Play start sound
	DDRD |= (1 << 6); 
	play_start_sound(buzzer_enabled);

	// Clear all button presses and serial inputs, so that potentially
	// buffered inputs aren't going to make it to the new game.
	clear_button_presses();
	clear_serial_input_buffer();
	
	//Reset step counter
	step_counter = 0;
}

void play_game(void)
{
	//Prepare variables for joystick
	uint16_t value_x;
	uint16_t value_y;
	
	//Initialise step counter
	step_counter = 0;
	uint8_t value = 0;
	uint8_t digit = 0; /* 0 = right, 1 = left */
	DDRC = 0xFF;
	DDRD = (1 << 5);
	
	uint32_t last_flash_time = get_current_time();
	uint32_t last_target_flash_time = get_current_time();
	uint32_t last_animation_flash = get_current_time();
	uint32_t last_input = 0;
	
	bool accept_input = true;
	
	play_time = 0;
	char play_time_str[20];
	
	uint16_t sensitivity_diagonal = 200;
	uint16_t sensitivity_regular = 400;

	// We play the game until it's over.
	while (!is_game_over())
	{
		// We need to check if any buttons have been pushed, this will
		// be NO_BUTTON_PUSHED if no button has been pushed. If button
		// 0 has been pushed, we get BUTTON0_PUSHED, and likewise, if
		// button 1 has been pushed, we get BUTTON1_PUSHED, and so on.
		ButtonState btn = button_pushed();
		int serial_input = -1;

		if (serial_input_available()) {
			serial_input = fgetc(stdin);
		}
		
		if (tolower(serial_input) == 'q') {
			buzzer_enabled = 1 - buzzer_enabled;
		}
		
		if (tolower(serial_input) == 'p') {
			while (1) {
				if (serial_input_available()) {
					if (tolower(fgetc(stdin)) == 'p') {
						break;
					}
				}
				//Keep ssd looping
				if(digit == 0) {
					value = step_counter % 10;
					} else {
					value = (step_counter / 10) % 10;
				}
				PORTC = seven_seg[value];
				PORTD = (digit << 5);
				digit = 1 - digit;
			}
		}
		
		//Detect values x and y from joystick
		//value x:
		ADMUX &= ~1;
		// Start the ADC conversion
		ADCSRA |= (1<<ADSC);
		while(ADCSRA & (1<<ADSC)) {
			; /* Wait until conversion finished */
		}
		value_x = ADC; // read the value
		
		//value y:
		ADMUX |= 1;
		// Start the ADC conversion
		ADCSRA |= (1<<ADSC);
		while(ADCSRA & (1<<ADSC)) {
			; /* Wait until conversion finished */
		}
		value_y = ADC; // read the value
		
		move_terminal_cursor(0,1);
		printf("%d", value_x);
		
		if ((value_x < rest_value_x-sensitivity_diagonal && value_y > rest_value_y+sensitivity_diagonal) && accept_input) {
			if (move_diagonal(0,-1,1,0)) {
				step_counter += 2;
				DDRD |= (1 << 6);
				play_move_sound(buzzer_enabled);
				last_input = get_current_time();
				accept_input = false;
			}
			last_flash_time = get_current_time();
		} else if ((value_x < rest_value_x-sensitivity_diagonal && value_y < rest_value_y-sensitivity_diagonal) && accept_input) {
			if (move_diagonal(0,-1,-1,0)) {
				step_counter += 2;
				DDRD |= (1 << 6);
				play_move_sound(buzzer_enabled);
				last_input = get_current_time();
				accept_input = false;
			}
			last_flash_time = get_current_time();
		} else if ((value_x > rest_value_x+sensitivity_diagonal && value_y < rest_value_y-sensitivity_diagonal) && accept_input) {
			if (move_diagonal(0,1,-1,0)) {
				step_counter += 2;
				DDRD |= (1 << 6);
				play_move_sound(buzzer_enabled);
				last_input = get_current_time();
				accept_input = false;
			}
			last_flash_time = get_current_time();
		} else if ((value_x > rest_value_x+sensitivity_diagonal && value_y > rest_value_y+sensitivity_diagonal) && accept_input) {
			if (move_diagonal(0,1,1,0)) {
				step_counter += 2;
				DDRD |= (1 << 6);
				play_move_sound(buzzer_enabled);
				last_input = get_current_time();
				accept_input = false;
			}
			last_flash_time = get_current_time();
		} else if ((btn == BUTTON0_PUSHED || tolower(serial_input) == 'd' || value_x > rest_value_x+sensitivity_regular) && accept_input) {
			if (move_player(0, 1)) {
				step_counter++; 
				DDRD |= (1 << 6); 
				play_move_sound(buzzer_enabled);
				last_input = get_current_time();
				accept_input = false;
			}
			last_flash_time = get_current_time();
		} else if ((btn == BUTTON1_PUSHED || tolower(serial_input) == 's' || value_y < rest_value_y-sensitivity_regular) && accept_input) {
			if (move_player(-1, 0)) {
				step_counter++; 
				DDRD |= (1 << 6); 
				play_move_sound(buzzer_enabled);
				last_input = get_current_time();
				accept_input = false;
			}
			last_flash_time = get_current_time();
		} else if ((btn == BUTTON2_PUSHED || tolower(serial_input) == 'w' || value_y > rest_value_y+sensitivity_regular) && accept_input) {
			if (move_player(1, 0)) {
				step_counter++; 
				DDRD |= (1 << 6); 
				play_move_sound(buzzer_enabled);
				last_input = get_current_time();
				accept_input = false;
			}
			last_flash_time = get_current_time();
		} else if ((btn == BUTTON3_PUSHED || tolower(serial_input) == 'a' || value_x < rest_value_x-sensitivity_regular) && accept_input) {
			if (move_player(0, -1)) {
				step_counter++; 
				DDRD |= (1 << 6); 
				play_move_sound(buzzer_enabled);
				last_input = get_current_time();
				accept_input = false;
			}
			last_flash_time = get_current_time();
		}

		uint32_t current_time = get_current_time();
		if (current_time >= last_flash_time + 200)
		{
			// 200ms (0.2 seconds) has passed since the last time
			// we flashed the player icon, flash it now.
			flash_player();

			// Update the most recent icon flash time.
			last_flash_time = current_time;
		}
		if (current_time >= last_target_flash_time + 500) {
			flash_targets();
			last_target_flash_time = current_time;
		}
		if (current_time >= last_input + 100) {
			accept_input = true;
		}
		
		//Display step counter on seven segment display
		if(digit == 0) {
			value = step_counter % 10;
			} else {
			value = (step_counter / 10) % 10;
		}
		PORTC = seven_seg[value];
		PORTD = (digit << 5);
		/* Change the digit flag for next time. if 0 becomes 1, if 1 becomes 0. */
		digit = 1 - digit;
		
		//Increment timer if necessary
		if (get_current_time() % 1000 == 0) {
			move_terminal_cursor(22, 1);
			sprintf(play_time_str, "%d", play_time);
			printf("%s", play_time_str);
			play_time++;
			_delay_ms(10);
		}
		DDRD &= (11111101);
	}
	DDRD |= (1 << 6); 
	play_victory_sound(buzzer_enabled);
	DDRD &= (11111101);
	handle_game_over();
}

void increment_step_counter(void) {
	step_counter++;
}

void handle_game_over(void)
{
	move_terminal_cursor(14, 10);
	printf_P(PSTR("GAME OVER"));
	move_terminal_cursor(15, 10);
	printf_P(PSTR("Press 'r'/'R' to restart, 'e'/'E' to exit,"));
	move_terminal_cursor(16, 10);
	printf_P(PSTR("or press 'n'/'N' to progress to level 2"));
	
	//calculate and print score
	int score = 0;
	if (200-step_counter > 0) {
		score += 200-step_counter;
	}
	if (1200-play_time > 0) {
		score += 1200-play_time;
	}
	move_terminal_cursor(18, 10);
	printf("Score: %d", score);

	//For ssd
	int digit = 0;
	int value;
	// Do nothing until a valid input is made.
	while (1)
	{
		// Get serial input. If no serial input is ready, serial_input
		// would be -1 (not a valid character).
		int serial_input = -1;
		if (serial_input_available())
		{
			serial_input = fgetc(stdin);
		}

		// Check serial input.
		if (toupper(serial_input) == 'R') {
			new_game(current_level);
			play_game();
		} else if (toupper(serial_input) == 'E') {
			current_level = 1;
			start_screen();
			new_game(current_level);
			play_game();
		} else if (toupper(serial_input) == 'N') {
			current_level = 2;
			new_game(current_level);
			play_game();
		}
		
		if(digit == 0) {
			value = step_counter % 10;
			} else {
			value = (step_counter / 10) % 10;
		}
		PORTC = seven_seg[value];
		PORTD = (digit << 5);
		digit = 1- digit;
	}
}