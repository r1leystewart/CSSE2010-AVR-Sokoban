/*
 * game.c
 *
 * Authors: Jarrod Bennett, Cody Burnett, Bradley Stone, Yufeng Gao
 * Modified by: Riley Stewart
 *
 * Game logic and state handler.
 */ 

#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "ledmatrix.h"
#include "terminalio.h"


// ========================== NOTE ABOUT MODULARITY ==========================

// The functions and global variables defined with the static keyword can
// only be accessed by this source file. If you wish to access them in
// another C file, you can remove the static keyword, and define them with
// the extern keyword in the other C file (or a header file included by the
// other C file). While not assessed, it is suggested that you develop the
// project with modularity in mind. Exposing internal variables and functions
// to other .C files reduces modularity.


// ============================ GLOBAL VARIABLES =============================

// The game board, which is dynamically constructed by initialise_game() and
// updated throughout the game. The 0th element of this array represents the
// bottom row, and the 7th element of this array represents the top row.
static uint8_t board[MATRIX_NUM_ROWS][MATRIX_NUM_COLUMNS];

// The location of the player.
static uint8_t player_row;
static uint8_t player_col;

// A flag for keeping track of whether the player is currently visible.
static bool player_visible;


// ========================== GAME LOGIC FUNCTIONS ===========================

// This function paints a square based on the object(s) currently on it.
static void paint_square(uint8_t row, uint8_t col)
{	
	switch (board[row][col] & OBJECT_MASK)
	{
		case ROOM:
			ledmatrix_update_pixel(row, col, COLOUR_BLACK);
			break;
		case WALL:
			ledmatrix_update_pixel(row, col, COLOUR_WALL);
			break;
		case BOX:
			ledmatrix_update_pixel(row, col, COLOUR_BOX);
			break;
		case TARGET:
			ledmatrix_update_pixel(row, col, COLOUR_TARGET);
			break;
		case BOX | TARGET:
			ledmatrix_update_pixel(row, col, COLOUR_DONE);
			break;
		default:
			break;
	}
}

// This function initialises the global variables used to store the game
// state, and renders the initial game display.
void initialise_game(void) {
	
	// Short definitions of game objects used temporarily for constructing
	// an easier-to-visualise game layout.
	#define _	(ROOM)
	#define W	(WALL)
	#define T	(TARGET)
	#define B	(BOX)

	// The starting layout of level 1. In this array, the top row is the
	// 0th row, and the bottom row is the 7th row. This makes it visually
	// identical to how the pixels are oriented on the LED matrix, however
	// the LED matrix treats row 0 as the bottom row and row 7 as the top
	// row.
	static const uint8_t lv1_layout[MATRIX_NUM_ROWS][MATRIX_NUM_COLUMNS] =
	{
		{ _, W, _, W, W, W, _, W, W, W, _, _, W, W, W, W },
		{ _, W, T, W, _, _, W, T, _, B, _, _, _, _, T, W },
		{ _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _ },
		{ W, _, B, _, _, _, _, W, _, _, B, _, _, B, _, W },
		{ W, _, _, _, W, _, B, _, _, _, _, _, _, _, _, _ },
		{ _, _, _, _, _, _, T, _, _, _, _, _, _, _, _, _ },
		{ _, _, _, W, W, W, W, W, W, T, _, _, _, _, _, W },
		{ W, W, _, _, _, _, _, _, W, W, _, _, W, W, W, W }
	};

	// Undefine the short game object names defined above, so that you
	// cannot use use them in your own code. Use of single-letter names/
	// constants is never a good idea.
	#undef _
	#undef W
	#undef T
	#undef B

	// Set the initial player location (for level 1).
	player_row = 5;
	player_col = 2;

	// Make the player icon initially invisible.
	player_visible = false;

	// Copy the starting layout (level 1 map) to the board array, and flip
	// all the rows.
	for (uint8_t row = 0; row < MATRIX_NUM_ROWS; row++)
	{
		for (uint8_t col = 0; col < MATRIX_NUM_COLUMNS; col++)
		{
			board[MATRIX_NUM_ROWS - 1 - row][col] =
				lv1_layout[row][col];
		}
	}

	// Draw the game board (map).
	for (uint8_t row = 0; row < MATRIX_NUM_ROWS; row++)
	{
		for (uint8_t col = 0; col < MATRIX_NUM_COLUMNS; col++)
		{
			paint_square(row, col);
		}
	}
	
	//Draw the game board on the terminal
	draw_terminal_board();
}

// This function flashes the player icon. If the icon is currently visible, it
// is set to not visible and removed from the display. If the player icon is
// currently not visible, it is set to visible and rendered on the display.
// The static global variable "player_visible" indicates whether the player
// icon is currently visible.
void flash_player(void)
{
	player_visible = !player_visible;
	if (player_visible)
	{
		// The player is visible, paint it with COLOUR_PLAYER.
		ledmatrix_update_pixel(player_row, player_col, COLOUR_PLAYER);
	}
	else
	{
		// The player is not visible, paint the underlying square.
		paint_square(player_row, player_col);
	}
}

// This function handles player movements.
bool move_player(int8_t delta_row, int8_t delta_col)
{
	//                    Implementation Suggestions
	//                    ==========================
	//
	//    Below are some suggestions for how to implement the first few
	//    features. These are only suggestions, you are absolutely not
	//   required to follow them if you know what you're doing, they're
	//     just here to help you get started. The suggestions for the
	//       earlier features are more detailed than the later ones.
	//
	// +-----------------------------------------------------------------+
	// |            Move Player with Push Buttons/Terminal               |
	// +-----------------------------------------------------------------+
	// | 1. Remove the display of the player icon from the current       |
	// |    location.                                                    |
	// |      - You may find the function flash_player() useful.         |
	// | 2. Calculate the new location of the player.                    |
	// |      - You may find creating a function for this useful.        |
	// | 3. Update the player location (player_row and player_col).      |
	// | 4. Draw the player icon at the new player location.             |
	// |      - Once again, you may find the function flash_player()     |
	// |        useful.                                                  |
	// | 5. Reset the icon flash cycle in the caller function (i.e.,     |
	// |    play_game()).                                                |
	// +-----------------------------------------------------------------+
	//
	// +-----------------------------------------------------------------+
	// |                      Game Logic - Walls                         |
	// +-----------------------------------------------------------------+
	// | 1. Modify this function to return a flag/boolean for indicating |
	// |    move validity - you do not want to reset icon flash cycle on |
	// |    invalid moves.                                               |
	// | 2. Modify this function to check if there is a wall at the      |
	// |    target location.                                             |
	// | 3. If the target location contains a wall, print one of your 3  |
	// |    'hit wall' messages and return a value indicating an invalid |
	// |    move.                                                        |
	// | 4. Otherwise make the move, clear the message area of the       |
	// |    terminal and return a value indicating a valid move.         |
	// +-----------------------------------------------------------------+
	//
	// +-----------------------------------------------------------------+
	// |                      Game Logic - Boxes                         |
	// +-----------------------------------------------------------------+
	// | 1. Modify this function to check if there is a box at the       |
	// |    target location.                                             |
	// | 2. If the target location contains a box, see if it can be      |
	// |    pushed. If not, print a message and return a value           |
	// |    indicating an invalid move.                                  |
	// | 3. Otherwise push the box and move the player, then clear the   |
	// |    message area of the terminal and return a valid indicating a |
	// |    valid move.                                                  |
	// +-----------------------------------------------------------------+
	
	//Calculate next positions
	int next_row = modulo((player_row+delta_row), 8);
	int next_col = modulo((player_col+delta_col), 16);
	int next_next_row = modulo((next_row+delta_row), 8);
	int next_next_col = modulo((next_col+delta_col), 16);

	paint_square(player_row, player_col);
	move_terminal_cursor(20,0);
	clear_to_end_of_line();
	
	//checks for wall in front of player
	if (board[next_row][next_col] == WALL) {
		display_terminal_message("wall");
		return false;
		
	//checks for filled target in front of player
	} else if (board[next_row][next_col] == (BOX | TARGET)) {
		if (board[next_next_row][next_next_col] == WALL) {
			display_terminal_message("box_wall");
			return false;
		}
		board[next_row][next_col] = TARGET;
		paint_square(next_row, next_col);
		update_terminal_display(next_row, MATRIX_NUM_ROWS-next_row, 1);
		board[next_next_row][next_next_col] = BOX;
		paint_square(next_next_row, next_next_col);
		update_terminal_display(next_next_row, MATRIX_NUM_ROWS-next_next_row, 1);
		
	//checks for box in front of player
	} else if (board[next_row][next_col] == BOX) {
		if (board[next_next_row][next_next_col] == WALL) {
			display_terminal_message("box_wall");
			return false;
		} else if (board[next_next_row][next_next_col] == BOX) {
			display_terminal_message("box_box");
			return false;
		} else {
			board[next_row][next_col] = ROOM;
			if (board[next_next_row][next_next_col] == TARGET) {
				board[next_next_row][next_next_col] = (TARGET | BOX);
				paint_square(next_next_row, next_next_col);
				update_terminal_display(next_next_row, MATRIX_NUM_ROWS-next_next_row, 1);
			} else {
				board[next_next_row][next_next_col] = BOX;
				paint_square(next_next_row, next_next_col);
				update_terminal_display(next_next_row, MATRIX_NUM_ROWS-next_next_row, 1);
			}
		}
	}
	
	player_row = next_row;
	player_col = next_col;
	paint_square(player_row, player_col);
	update_terminal_display(player_row, MATRIX_NUM_ROWS-player_row, 1);
	flash_player();
	return true;
}

void display_terminal_message(char type[]) {
	if (strcmp(type, "wall") == 0) {
		int rand_num;
		int lb = 1;
		int ub = 3;
		rand_num = (rand() % (ub - lb + 1)) + lb;
		move_terminal_cursor(20, 1);
		if (rand_num == 1) {
			printf_P(PSTR("Player hit a wall"));
		} else if (rand_num == 2) {
			printf_P(PSTR("Wall hit"));
		} else if (rand_num == 3) {
			printf_P(PSTR("There is a wall in the way"));
		}
	} else if (strcmp(type, "box_wall") == 0) {
		move_terminal_cursor(20, 1);
		printf_P(PSTR("Cannot push box onto wall"));
	} else if (strcmp(type, "box_box") == 0) {
		move_terminal_cursor(20, 1);
		printf_P(PSTR("Cannot stack boxes"));
	}
	return;
}

// This function checks if the game is over (i.e., the level is solved), and
// returns true iff (if and only if) the game is over.
bool is_game_over(void)
{
	for (int row = 0; row < MATRIX_NUM_ROWS; row++) {
		for (int col = 0; col < MATRIX_NUM_COLUMNS; col++) {
			if (board[row][col] == TARGET) {
				return false;
			}
		}
	}
	return true;
}

//Calculates the modulus of a number for player movement
int modulo(int x,int y){
	return (x % y + y) % y;
}

//Paints the current board on the terminal display
void draw_terminal_board(void) {
	int GAME_BOARD_ROW = 1;
	int GAME_BOARD_COL = 1;
	for (int row = MATRIX_NUM_ROWS-1; row >= 0; row--) {
		update_terminal_display(row, GAME_BOARD_ROW, GAME_BOARD_COL);
		printf("\n");
		GAME_BOARD_ROW++;
	}
}

void update_terminal_display(int board_row, int terminal_row, int terminal_col) {
	move_terminal_cursor(terminal_row, terminal_col);
	clear_to_end_of_line();
	for (int column = 1; column <= MATRIX_NUM_COLUMNS-1; column++) {
		if (board[board_row][column] == ROOM) {
			printf("\033[100m   \033[0m");
		} else if (board[board_row][column] == WALL) {
			printf("\033[103m   \033[0m");
		} else if (board[board_row][column] == BOX) {
			printf("\033[43m   \033[0m");
		} else if (board[board_row][column] == TARGET) {
			printf("\033[41m   \033[0m");
		} else if (board[board_row][column] == (BOX | TARGET)) {
			printf("\033[102m   \033[0m");
		}
	}
}