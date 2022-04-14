#include "stdafx.h"

#include "preview_modal_tag_dialog.h"
#include "preview_modal_tag_dialog_ILO_uilist.h"

size_t ILOD_preview_modal::listGetItemCount(ctx_t ctx) {

	CPreviewModalTagDialog* dlg = (TParent*)this;
	HWND wnd = dlg->m_hWnd;

	return m_vtracks_p->size();;
}

// fb2k lib - get subitems

pfc::string8 ILOD_preview_modal::listGetSubItemText(ctx_t ctx, size_t item, size_t subItem) {

	if (subItem == 0) {
	
		return std::to_string(item).c_str();
	}
	else if (subItem == 1) {
	
		return m_vtracks_p->at(item);

	}
	else if (subItem == 2) {

		pfc::array_t<string_encoded_array>* arr_sea_val;
		pfc::array_t<string_encoded_array>* cmp_old_sea_val;

		if (get_mode() == 0) {
			arr_sea_val = &m_item_result->value;
		}
		else if (get_mode() == 1) {
			arr_sea_val = &m_item_result->value;
			cmp_old_sea_val = &m_item_result->old_value;
		}
		else if (get_mode() == 2) {
			arr_sea_val = &m_item_result->old_value;
		}
		else {
			arr_sea_val = &m_item_result->value;
		}

		pfc::string8 buffer;
		pfc::string8 cmp_old_buffer;

		if (arr_sea_val->size() == 1) {
			buffer = (*arr_sea_val)[0].print();
		}
		else {
			buffer = (*arr_sea_val)[item].print();
		}

		if (get_mode() == 1) {
			if (cmp_old_sea_val->size() == 1) {
				cmp_old_buffer = (*cmp_old_sea_val)[0].print();

			}
			else {
				cmp_old_buffer = (*cmp_old_sea_val)[item].print();
			}

			if (STR_EQUAL(buffer, cmp_old_buffer)) buffer = "";
			else {
				if (!buffer.get_length()) buffer = "<empty>";				
			} 
		}
		return buffer.c_str();
	}
	else {
		return "na";
	}
}

// fb2k lib - can reorder

bool ILOD_preview_modal::listCanReorderItems(ctx_t) {

	return true;

}

// fb2k lib - reorder

bool ILOD_preview_modal::listReorderItems(ctx_t ctx, const size_t* order, size_t count) {

	return true;
}

// fb2k lib - remove

bool ILOD_preview_modal::listRemoveItems(ctx_t ctx, pfc::bit_array const& mask) {

	return true;
}

// fb2k lib - action, edit, clicked - not implemented

void ILOD_preview_modal::listItemAction(ctx_t, size_t item) {
	//m_list.TableEdit_Start(item, 1);
}
void ILOD_preview_modal::listSubItemClicked(ctx_t, size_t item, size_t subItem) {
	//if (subItem == 1) {m_list.TableEdit_Start(item, subItem); }
}
void ILOD_preview_modal::listSetEditField(ctx_t ctx, size_t item, size_t subItem, const char* val) {
	//if (subItem == 1) {m_data[item].m_value = val; }
}
bool ILOD_preview_modal::listIsColumnEditable(ctx_t, size_t subItem) {
	return false;
}

// view mode set/get

int ILOD_preview_modal::get_mode() {

	return m_view_mode;
}

//todo: mutex

void ILOD_preview_modal::set_mode(int view_mode) {

	m_view_mode = view_mode;
}