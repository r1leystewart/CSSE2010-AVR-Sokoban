/*
 * buzzer.h
 *
 *  Author: Riley Stewart
 */ 

#include <stdbool.h>

void init_buzzer(void);

void play_move_sound(bool enabled);

void play_start_sound(bool enabled);

void play_victory_sound(bool enabled);