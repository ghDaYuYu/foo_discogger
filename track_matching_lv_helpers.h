#pragma once

	void list_swap_items(HWND list, unsigned int pos1, unsigned int pos2);
	//Moves the selected items up one level
	void move_selected_items_up(HWND list);
	//Moves the selected items up one level
	void move_selected_items_down(HWND list);
	void remove_items(HWND list, bool bcrop);
	void select_all_items(HWND list);
	void invert_selected_items(HWND list);
