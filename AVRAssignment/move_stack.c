/*
 * stack.c
 *
 * Created: 23/10/2024 2:02:06 PM
 *  Author: riley
 */ 

int max_size = 6;
int top = -1;

int get_empty_stack(void) {
	return {{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1}};
}

bool is_empty(int stack[][]) {
	if (top == -1) {
		return true;
	}
	return false;
}

bool is_full(int stack[][]) {
	if (top == max_size) {
		return true;
	}
	return false;
}
