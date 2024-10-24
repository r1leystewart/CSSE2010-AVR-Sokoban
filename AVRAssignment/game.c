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
#include "buzzer.h"


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

static bool targets_visible;

//List for keeping track of player moves
int move_list[6][2] = {{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1}};
int list_top = -1;

int box_list[6][4] = {{-1,-1,-1,-1},{-1,-1,-1,-1},{-1,-1,-1,-1},{-1,-1,-1,-1},{-1,-1,-1,-1},{-1,-1,-1,-1}};
int box_list_top = -1;

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

void initialise_level(int level) {
	// Short definitions of game objects used temporarily for constructing
	// an easier-to-visualise game layout.
	#define _	(ROOM)
	#define W	(WALL)
	#define T	(TARGET)
	#define B	(BOX)
	
	// The starting layout of level x. In this array, the top row is the
	// 0th row, and the bottom row is the 7th row. This makes it visually
	// identical to how the pixels are oriented on the LED matrix, however
	// the LED matrix treats row 0 as the bottom row and row 7 as the top
	// row.
	
	uint8_t level_layout[MATRIX_NUM_ROWS][MATRIX_NUM_COLUMNS];
	switch (level) {
			case 1:
			{
			uint8_t temp_layout[MATRIX_NUM_ROWS][MATRIX_NUM_COLUMNS] = {
				{ _, W, _, W, W, W, _, W, W, W, _, _, W, W, W, W },
				{ _, W, T, W, _, _, W, T, _, B, _, _, _, _, T, W },
				{ _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _ },
				{ W, _, B, _, _, _, _, W, _, _, B, _, _, B, _, W },
				{ W, _, _, _, W, _, B, _, _, _, _, _, _, _, _, _ },
				{ _, _, _, _, _, _, T, _, _, _, _, _, _, _, _, _ },
				{ _, _, _, W, W, W, W, W, W, T, _, _, _, _, _, W },
				{ W, W, _, _, _, _, _, _, W, W, _, _, W, W, W, W }
			};
			memcpy(level_layout, temp_layout, sizeof(level_layout));
			player_row = 5;
			player_col = 2;
			}
			break;
		case 2:
			{
			uint8_t temp_layout[MATRIX_NUM_ROWS][MATRIX_NUM_COLUMNS] = {
				{_, _, W, W, W, W, _, _, W, W, _, _, _, _, _, W },
				{_, _, W, _, _, W, _, W, W, _, _, _, _, B, _, _ },
				{_, _, W, _, B, W, W, W, _, _, T, W, _, T, W, W },
				{_, _, W, _, _, _, _, T, _, _, B, W, W, W, _, _ },
				{W, W, W, W, _, W, _, _, _, _, _, W, _, W, W, _ },
				{W, T, B, _, _, _, _, B, _, _, _, W, W, _, W, W },
				{W, _, _, _, T, _, _, _, _, _, _, B, T, _, _, _ },
				{W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W }
			};
			memcpy(level_layout, temp_layout, sizeof(level_layout));
			player_row = 6;
			player_col = 15;
			}
			break;
	}
	
	// Undefine the short game object names defined above, so that you
	// cannot use use them in your own code. Use of single-letter names/
	// constants is never a good idea.
	#undef _
	#undef W
	#undef T
	#undef B
	
	// Copy the starting layout (level map) to the board array, and flip
	// all the rows.
	for (uint8_t row = 0; row < MATRIX_NUM_ROWS; row++)
	{
		for (uint8_t col = 0; col < MATRIX_NUM_COLUMNS; col++)
		{
			board[MATRIX_NUM_ROWS - 1 - row][col] =
			level_layout[row][col];
		}
	}
}

// This function initialises the global variables used to store the game
// state, and renders the initial game display.
void initialise_game(int level) {
	
	initialise_level(level);

	// Make the player icon initially invisible.
	player_visible = false;

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

void flash_targets(void) {
	targets_visible = !targets_visible;
	for (int row = 0; row < MATRIX_NUM_ROWS; row++) {
		for (int col = 0; col < MATRIX_NUM_COLUMNS; col++) {
			if (board[row][col] == TARGET) {
				if (targets_visible) {
					ledmatrix_update_pixel(row, col, COLOUR_TARGET);
				} else {
					ledmatrix_update_pixel(row, col, COLOUR_BLACK);
				}
			}
		}
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
	
	bool box_moved = false;
	
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
		box_moved = true;
		
	//checks for box in front of player
	} else if (board[next_row][next_col] == BOX) {
		if (board[next_next_row][next_next_col] == WALL) {
			display_terminal_message("box_wall");
			return false;
		} else if (board[next_next_row][next_next_col] == BOX) {
			display_terminal_message("box_box");
			return false;
		} else {
			box_moved = true;
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
	
	if (box_moved) {
		add_previous_box_location(next_row, next_col, next_next_row, next_next_col);
		box_moved = false;
	} else {
		add_previous_box_location(-1,-1,-1,-1);
	}
	
	add_to_move_list(player_row, player_col);
	player_row = next_row;
	player_col = next_col;
	paint_square(player_row, player_col);
	update_terminal_display(player_row, MATRIX_NUM_ROWS-player_row, 1);	
	return true;
}

bool move_diagonal(int8_t delta_row_1, int8_t delta_col_1, int8_t delta_row_2, int8_t delta_col_2) {
	int first_move_row;
	int first_move_col;
	int second_move_row;
	int second_move_col;
	first_move_row = modulo((player_row+delta_row_1), 8);  //try moving in the first direction first
	first_move_col = modulo((player_col+delta_col_1), 16);
	if (check_wall_or_box(first_move_row, first_move_col)) {  //try first move
		second_move_row = modulo((first_move_row+delta_row_2), 8);
		second_move_col = modulo((first_move_col+delta_col_2), 16);
		if (check_wall_or_box(second_move_row, second_move_col)) {  //try second move
			paint_square(player_row, player_col);  //second move successful
			add_to_move_list(player_row, player_col);
			player_row = second_move_row;
			player_col = second_move_col;
			paint_square(player_row, player_col);
			update_terminal_display(player_row, MATRIX_NUM_ROWS-player_row, 1);
			flash_player();
			return true;
		}
	} 
	first_move_row = modulo((player_row+delta_row_2), 8);  //try moving in the second direction first
	first_move_col = modulo((player_col+delta_col_2), 16);
	if (check_wall_or_box(first_move_row, first_move_col)) {  //try first move
		second_move_row = modulo((first_move_row+delta_row_1), 8);
		second_move_col = modulo((first_move_col+delta_col_1), 16);
		if (check_wall_or_box(second_move_row, second_move_col)) {  //try second move
			paint_square(player_row, player_col);  //second move successful
			add_to_move_list(player_row, player_col);
			player_row = second_move_row;
			player_col = second_move_col;
			paint_square(player_row, player_col);
			update_terminal_display(player_row, MATRIX_NUM_ROWS-player_row, 1);
			flash_player();
			return true;
		}
	}
	return false;  //both directions failed, move cannot be made
}

bool undo_move(void) {
	if (list_top < 0 || move_list[list_top][0] == -1 || move_list[list_top][1] == -1) {
		return false;
	}
	if (list_top >= 0) {
		paint_square(player_row, player_col);
		player_row = move_list[list_top][0];
		player_col = move_list[list_top][1];
		list_top--;
	}
	
	if (!(box_list_top < 0 || box_list[box_list_top][0] == -1 || box_list[box_list_top][1] == -1)) {
		if (box_list_top >= 0) {
			move_box();
			box_list_top--;
		}
	}
	return true;
}

void add_to_move_list(uint8_t row, uint8_t col) {
	if (list_top < 5) {
		list_top++;
		move_list[list_top][0] = row;
		move_list[list_top][1] = col;
	} else {
		for (int i = 0; i < 5; i++) {
			move_list[i][0] = move_list[i+1][0];
			move_list[i][1] = move_list[i+1][1];
		}
		move_list[5][0] = row;
		move_list[5][1] = col;
	}
}

void add_previous_box_location(uint8_t row, uint8_t col, uint8_t current_row, uint8_t current_col) {
	if (box_list_top < 5) {
		box_list_top++;
		box_list[box_list_top][0] = row;
		box_list[box_list_top][1] = col;
		box_list[box_list_top][2] = current_row;
		box_list[box_list_top][3] = current_row;
	} else {
		for (int i = 0; i < 5; i++) {
			box_list[i][0] = box_list[i+1][0];
			box_list[i][1] = box_list[i+1][1];
			box_list[i][2] = box_list[i+1][2];
			box_list[i][3] = box_list[i+1][3];
		}
		box_list[5][0] = row;
		box_list[5][1] = col;
		box_list[5][2] = current_row;
		box_list[5][3] = current_col;
	}
}

void move_box(void) {
	board[box_list[box_list_top][0]][box_list[box_list_top][1]] = BOX;
	board[box_list[box_list_top][2]][box_list[box_list_top][3]] = ROOM;
	paint_square(box_list[box_list_top][0], box_list[box_list_top][1]);
	paint_square(box_list[box_list_top][2], box_list[box_list_top][3]);
}

bool check_wall_or_box(int row, int col) {
	if (board[row][col] == WALL) {
		display_terminal_message("wall_diagonal");
		return false;
	} else if (board[row][col] == BOX) {
		display_terminal_message("box_diagonal");
		return false;
	} else if (board[row][col] == (BOX | TARGET)) {
		display_terminal_message("box_diagonal");
		return false;
	}
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
	} else if (strcmp(type, "wall_diagonal") == 0) {
		move_terminal_cursor(20, 1);
		printf_P(PSTR("Diagonal move cannot be made"));
	} else if (strcmp(type, "box_diagonal") == 0) {
		move_terminal_cursor(20, 1);
		printf_P(PSTR("Cannot move boxes diagonally"));
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
	paint_square(player_row, player_col);
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