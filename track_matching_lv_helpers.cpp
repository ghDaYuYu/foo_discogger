#include "stdafx.h"

#include "track_matching_lv_helpers.h"

	pfc::string8 duration_to_str(int seconds) {
		int hours = seconds / 3600;
		seconds %= 3600;
		int minutes = seconds / 60;
		seconds = seconds % 60;
		pfc::string8 result = "";
		if (hours) {
			result << hours << ":";
		}
		if (hours && minutes < 10) {
			result << "0";
		}
		result << minutes << ":";
		if (seconds < 10) {
			result << "0";
		}
		result << seconds;
		return result;
	}

	void list_swap_items(HWND list, unsigned int pos1, unsigned int pos2) {
		const unsigned LOCAL_BUFFER_SIZE = 4096;
		LVITEM lvi1, lvi2;
		UINT uMask = LVIF_TEXT | LVIF_IMAGE | LVIF_INDENT | LVIF_PARAM | LVIF_STATE;
		char buffer1[LOCAL_BUFFER_SIZE + 1];
		char buffer2[LOCAL_BUFFER_SIZE + 1];
		lvi1.pszText = (LPWSTR)buffer1;
		lvi2.pszText = (LPWSTR)buffer2;
		lvi1.cchTextMax = sizeof(buffer1);
		lvi2.cchTextMax = sizeof(buffer2);

		lvi1.mask = uMask;
		lvi1.stateMask = (UINT)-1;
		lvi1.iItem = pos1;
		lvi1.iSubItem = 0;
		BOOL result1 = ListView_GetItem(list, &lvi1);

		lvi2.mask = uMask;
		lvi2.stateMask = (UINT)-1;
		lvi2.iItem = pos2;
		lvi2.iSubItem = 0;
		BOOL result2 = ListView_GetItem(list, &lvi2);

		if (result1 && result2) {
			lvi1.iItem = pos2;
			lvi2.iItem = pos1;
			lvi1.mask = uMask;
			lvi2.mask = uMask;
			lvi1.stateMask = (UINT)-1;
			lvi2.stateMask = (UINT)-1;
			//swap the items
			ListView_SetItem(list, &lvi1);
			ListView_SetItem(list, &lvi2);

			int total_columns = Header_GetItemCount(ListView_GetHeader(list));
			// Loop for swapping each column in the items.
			for (int i = 1; i < total_columns; i++) {
				buffer1[0] = '\0';
				buffer2[0] = '\0';
				ListView_GetItemText(list, pos1, i, (LPWSTR)buffer1, LOCAL_BUFFER_SIZE);
				ListView_GetItemText(list, pos2, i, (LPWSTR)buffer2, LOCAL_BUFFER_SIZE);
				ListView_SetItemText(list, pos2, i, (LPWSTR)buffer1);
				ListView_SetItemText(list, pos1, i, (LPWSTR)buffer2);
			}
		}
	}
	//Moves the selected items up one level
	void move_selected_items_up(HWND list)
	{

		for (int i = 0; i < ListView_GetItemCount(list); i++)
		{
			if (ListView_IsItemSelected(list, i))//identify the selected item
			{
				//swap with the top item(move up)
				if (i > 0 && !ListView_IsItemSelected(list, i - 1))
				{
					list_swap_items(list, i, i - 1);
					ListView_SetItemState(list, i - 1, LVIS_SELECTED, 0x000F);
				}
			}
		}
	}

	//Moves the selected items up one level
	void move_selected_items_down(HWND list)
	{
		int startindex = ListView_GetItemCount(list) - 1;
		for (int i = startindex; i > -1; i--)
		{
			if (ListView_IsItemSelected(list, i))//identify the selected item
			{
				//swap with the lower item(move down)
				if (i < startindex && !ListView_IsItemSelected(list, i + 1))
				{
					list_swap_items(list, i, i + 1);
					ListView_SetItemState(list, i + 1, LVIS_SELECTED, 0x000F);
				}

			}
		}
	}

	void remove_items(HWND list, bool bcrop) {

		const size_t count = ListView_GetItemCount(list);
		bit_array_bittable delmask(count);
		for (size_t i = 0; i < count; i++) {
			if (bcrop)
				delmask.set(i, !ListView_IsItemSelected(list, i));
			else
				delmask.set(i, ListView_IsItemSelected(list, i));
		}

		t_size del = 0;
		t_size nextfocus = pfc_infinite;
		for (size_t i = 0; i < count; i++) {
			if (delmask.get(i)) {
				ListView_DeleteItem(list, i - del);
				if (nextfocus != pfc_infinite) nextfocus = i;
				del++;
			}
			else
			{
				if (bcrop && nextfocus == pfc_infinite) nextfocus = i;
			}
		}

		if (!bcrop && ListView_GetItemCount(list) > 0) {
			ListView_SetItemState(list, nextfocus > 0 ? nextfocus - 1 : 0, LVIS_FOCUSED, 0x000F);
		}
	}

	void select_all_items(HWND list) {
		ListView_SetItemState(list, -1, LVIS_SELECTED, 0x000F);
		ListView_SetItemState(list, 0, LVIS_FOCUSED, 0x000F);
	}

	void invert_selected_items(HWND list) {
		LVITEM lvi;
		ZeroMemory(&lvi, sizeof(lvi));
		lvi.mask = LVIF_STATE;
		lvi.stateMask = LVIS_SELECTED;
		const size_t count = ListView_GetItemCount(list);
		t_size newfsel = pfc_infinite;
		for (size_t i = 0; i < count; i++) {
			ListView_IsItemSelected(list, i) ? lvi.state = 0 : lvi.state = 0x000f;
			if (lvi.state == 0x000f && newfsel == pfc_infinite) newfsel = i;
			::SendMessage(list, LVM_SETITEMSTATE, (WPARAM)i, (LPARAM)&lvi);
		}

		ListView_SetItemState(list, newfsel, LVIS_FOCUSED, 0x000F);
	}
