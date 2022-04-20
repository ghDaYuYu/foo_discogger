#include "stdafx.h"

#include "preview_modal_tag_dialog.h"
#include "preview_modal_tag_ILO.h"

size_t ILOD_preview_modal::listGetItemCount(ctx_t ctx) {

	return ilo_get_vtracks_desc().size();
}

// fb2k lib - get subitems

pfc::string8 ILOD_preview_modal::listGetSubItemText(ctx_t ctx, size_t item, size_t subItem) {

	if (subItem == 0) {
	
		return std::to_string(item).c_str();
	}
	else if (subItem == 1) {
	
		return ilo_get_vtracks_desc().at(item);
	}
	else if (subItem == 2) {

		pfc::array_t<string_encoded_array>* arr_sea_val = {};
		pfc::array_t<string_encoded_array>* cmp_old_sea_val = {};

		if (get_mode() == PreView::kNORMAL) {
			arr_sea_val = &m_item_result->value;
		}
		else if (get_mode() == PreView::kDIFFERENCE) {
			arr_sea_val = &m_item_result->value;
			cmp_old_sea_val = &m_item_result->old_value;
		}
		else if (get_mode() == PreView::kORIGINAL) {
			arr_sea_val = &m_item_result->old_value;
		}
		else {

			arr_sea_val = &m_item_result->value;
		}

		pfc::string8 buffer;
		pfc::string8 cmp_old_buffer;

		//see: preview_dialog (1000: print_normal)

		if (arr_sea_val->size() == 1) {

			buffer = (*arr_sea_val)[0].print();
		}
		else {
			buffer = (*arr_sea_val)[item].print();
		}

		if (get_mode() == PreView::kDIFFERENCE) {

			if (cmp_old_sea_val->size() == 1) {

				cmp_old_buffer = (*cmp_old_sea_val)[0].print();
			}
			else {
				cmp_old_buffer = (*cmp_old_sea_val)[item].print();
			}

			if (STR_EQUAL(buffer, cmp_old_buffer)) {
				buffer = "";
			}
			else if (!buffer.get_length()) {
				buffer = "<empty>";
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

	return false;
}

// fb2k lib - reorder

bool ILOD_preview_modal::listReorderItems(ctx_t ctx, const size_t* order, size_t count) {

	return true;
}

// fb2k lib - remove

bool ILOD_preview_modal::listRemoveItems(ctx_t ctx, pfc::bit_array const& mask) {

	return true;
}

// fb2k lib - action

void ILOD_preview_modal::listItemAction(ctx_t ctx, size_t item) {

	listSubItemClicked(ctx, item, 2);
}

// fb2k lib - subitem clicked

void ILOD_preview_modal::listSubItemClicked(ctx_t ctx, size_t item, size_t subItem) {

	if (listIsColumnEditable(ctx, subItem)) {

		if (!m_item_result->tag_entry->freeze_tag_name) {

			//edit
			ilo_get_ui_list()->TableEdit_Start(item, 2);
		}
	}
}

// get edit field

pfc::string8 ILOD_preview_modal::listGetEditField(ctx_t ctx, size_t item, size_t subItem, size_t& lineCount) {
	
	pfc::string8 str = listGetSubItemText(ctx, item, subItem);
	bool onelined = str.length() < 50 && str.find_first(';', 0) == pfc_infinite;
	lineCount = onelined ? 1 : LINES_LONGFIELD;
	return str;
	//return listGetSubItemText(ctx, item, subItem);
}

// set edit field

void ILOD_preview_modal::listSetEditField(ctx_t ctx, size_t item, size_t subItem, const char* val) {

	pfc::chain_list_v2_t<pfc::string8> splitval;
	pfc::splitStringByChar(splitval, val, ';');

	string_encoded_array sea_val;

	for (auto it = splitval.first(); it.is_valid(); it++) {
		pfc::string8 buffer = *it;
		sea_val.append_item(buffer);
		
	}

	sea_val.encode();

	//pfc::array_t<string_encoded_array> sfa_value = m_item_result->value;	
	//size_t valsize = sfa_value.get_count();

	if (m_item_result->value.get_count() == 1) {

		//src common to all tracks ?

		if (!STR_EQUAL(sea_val, m_item_result->value[0].print())) {

			//build per track values

			size_t newcount = ilo_get_ui_list()->GetItemCount();

			pfc::string8 prevstring = m_item_result->value[0].print();

			m_item_result->value.force_reset();

			for (size_t it = 0; it < newcount; it++) {
				string_encoded_array previtem(prevstring);
				previtem.encode();
				m_item_result->value.add_item(previtem);
			}

			// replace common value by per track

			m_item_result->value[item] = sea_val;
		}
	}
	else {
		m_item_result->value[item] = sea_val;
	}

	/*for (size_t i = 0; i < sfa_value.size(); i++) {
		auto w = sfa_value[i];
		w = w;
	}*/
}

// column editable

bool ILOD_preview_modal::listIsColumnEditable(ctx_t ctx, size_t subItem) {

	return (get_mode() == PreView::kNORMAL && subItem == 2);
}

// mode set/get

PreView ILOD_preview_modal::get_mode() {

	return m_view_mode;
}

//todo: mutex?

void ILOD_preview_modal::set_mode(PreView view_mode) {

	m_view_mode = view_mode;
}