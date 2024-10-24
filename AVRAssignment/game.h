/*
 * game.h
 *
 * Authors: Jarrod Bennett, Cody Burnett, Bradley Stone, Yufeng Gao
 * Modified by: Riley Stewart
 *
 * Function prototypes for game functions available externally. You may wish
 * to add extra function prototypes here to make other functions available to
 * other files.
 */

#ifndef GAME_H_
#define GAME_H_

#include <stdint.h>
#include <stdbool.h>

// Object definitions.
#define ROOM       	(0U << 0)
#define WALL       	(1U << 0)
#define BOX        	(1U << 1)
#define TARGET     	(1U << 2)
#define OBJECT_MASK	(ROOM | WALL | BOX | TARGET)

// Colour definitions.
#define COLOUR_PLAYER	(COLOUR_DARK_GREEN)
#define COLOUR_WALL  	(COLOUR_YELLOW)
#define COLOUR_BOX   	(COLOUR_ORANGE)
#define COLOUR_TARGET	(COLOUR_RED)
#define COLOUR_DONE  	(COLOUR_GREEN)

void initialise_level(int level);

/// <summary>
/// Initialises the game.
/// </summary>
void initialise_game(int level);

/// <summary>
/// Moves the player based on row and column deltas.
/// </summary>
/// <param name="delta_row">The row delta.</param>
/// <param name="delta_col">The column delta.</param>
/// <returns>Whether the move was valid or not</returns>
bool move_player(int8_t delta_row, int8_t delta_col);

bool move_diagonal(int8_t delta_row_1, int8_t delta_col_1, int8_t delta_row_2, int8_t delta_col_2);

bool undo_move(void);

void add_to_move_list(uint8_t row, uint8_t col);

void add_previous_box_location(uint8_t row, uint8_t col, uint8_t current_row, uint8_t current_col);

void move_box(void);

bool check_wall_or_box(int row, int col);

/// <summary>
/// Displays a message in the message area of the terminal.
/// Contents of message depend on type parameter
/// </summary>
/// <param name="type">The type of message to be displayed.</param>
void display_terminal_message(char type[]);

/// <summary>
/// Detects whether the game is over (i.e., current level solved).
/// </summary>
/// <returns>Whether the game is over.</returns>
bool is_game_over(void);

///<summary>
///Calculates modulo for moving player across board
///</summary>
///<param name="x">First value in calculation</param>
///<param name="y">Second value in calculation</param>
///<returns>Modulo of x and y</returns>
int modulo(int x,int y);

/// <summary>
/// Flashes the player icon.
/// </summary>
void flash_player(void);

void flash_targets(void);

void draw_terminal_board(void);

void update_terminal_display(int board_row, int terminal_row, int terminal_col);

#endif /* GAME_H_ */

