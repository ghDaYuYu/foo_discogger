#include "stdafx.h"

#include "utils.h"
#include "preview_list.h"
#include "preview_dialog.h"
#include "preview_ILO.h"

void ILOD_preview::Enabled(bool enable) {
	CPreviewList* uilist = (CPreviewList*)ilo_get_uilist();
	uilist->EnableWindow(enable);
}

void ILOD_preview::SetResults(const std::weak_ptr<void>& ptag_wwriter, PreView preview_mode, std::vector<preview_stats> m_vstats) {

	m_tag_writer = ptag_writer;
	m_preview_mode = preview_mode;
	m_v_stats = m_vstats;

	CPreviewList* uilist = (CPreviewList*)ilo_get_uilist();
	ListView_SetItemCountEx(uilist->m_hWnd, m_tag_writer->tag_results.get_count(), 0);
	uilist->Invalidate();
}

//IListControlOwnerDataSource overrides

size_t ILOD_preview::listGetItemCount(ctx_t ctx) {

	if (m_tag_writer && !m_tag_writer->atm_tag_results_ready) {

		return 0;
	}

	if (m_tag_writer) {
		return m_tag_writer->tag_results.get_count();
	}

	return 0;
}


// print debug

pfc::string8 print_stenar(const pfc::array_t<string_encoded_array>& input) {
	pfc::string8 result = "";
	if (input.get_size() == 1) {
		result = input[0].print_raw();
	}
	else {
		result << "(multiple values) ";
		const auto count = input.get_count();
		size_t done = 0;
		for (unsigned int i = 0; i < count; i++) {
			auto part = input[i].print_raw();
			if (part.get_length()) {
				if (done) {
					result << "; ";
				}
				result << part;
				done++;
			}
		}
		if (!done) {
			result = "";
		}
	}
	return result;
}

// print normal (original: input oldvalue, results: input value)

pfc::string8 print_normal(const pfc::array_t<string_encoded_array>& input) {
	pfc::string8 result = "";
	if (!input.size()) {
		//todo: identify crash scenario for null result
		return "#error";
	}
	if (input.get_size() == 1) {
		result = input[0].print();
	}
	else {
		bool allequal = true;
		pfc::string8 commonval = input[0].print();
		for (auto w : input) {
			if (!(allequal = (STR_EQUAL(w.print(), commonval)))) {
				break;
			}
		}

		if (allequal) {
			result = commonval;
		}
		else {
			pfc::array_t<pfc::string8> parts;
			const auto count = input.get_count();
			for (unsigned int i = 0; i < count; i++) {
				auto part = input[i].print();
				if (part.get_length()) {
					bool duplicate = false;
					for (size_t j = 0; j < parts.get_size(); j++) {
						if (STR_EQUAL(parts[j], part)) {
							duplicate = true;
							break;
						}
					}
					if (!duplicate) {
						parts.append_single(part);
					}
				}
			}
			if (parts.get_size() && !allequal) {
				result << "(multiple values) " << join(parts, "; ");
			}
		}
	}
	return result;
}

// print difference

pfc::string8 print_difference(const pfc::array_t<string_encoded_array>& input, const pfc::array_t<string_encoded_array>& old_input) {
	const size_t count = input.get_count();
	const size_t old_count = old_input.get_count();
	const size_t max_count = max(count, old_count);

	//todo: check no change vs change to blank

	pfc::string8 result = "";
	if (max_count == 1) {
		if (!STR_EQUAL(old_input[0].print(), pfc::lineEndingsToWin(input[0].print()))) {
			if (!input[0].print().get_length())
				result << "(changed) ";
			result << input[0].print();
		}
	}
	else {
		size_t changes = 0;
		pfc::array_t<pfc::string8> parts;
		for (unsigned int i = 0; i < max_count; i++) {
			size_t ii = count == 1 ? 0 : i;
			size_t oi = old_count == 1 ? 0 : i;
			if (!STR_EQUAL(input[ii].print(), old_input[oi].print())) {
				changes++;
				auto part = input[ii].print();
				if (part.get_length()) {
					bool duplicate = false;
					for (size_t j = 0; j < parts.get_size(); j++) {
						if (STR_EQUAL(parts[j], part)) {
							duplicate = true;
							break;
						}
					}
					if (!duplicate) {
						parts.append_single(part);
					}
				}
			}
		}
		if (parts.get_size()) {
			result << "(" << changes << " changed) " << join(parts, "; ");
		}
		else {
			if (changes) {
				if (!input[0].print().get_length())
					result << "(changed)";
			}
		}
	}
	return result;
}


static pfc::array_t<string_encoded_array>GetPreviewValue(const tag_result_ptr& result) {
	pfc::array_t<string_encoded_array> value;
	if (!result->result_approved && result->changed)
		return result->old_value;
	else
		return result->value;
}

// print result with mode param

pfc::string8 print_result_in_mode(const tag_result_ptr& result, PreView preview_mode) {

	pfc::string8 mode_result;

	if (preview_mode == PreView::Normal) {
		mode_result = print_normal(GetPreviewValue(result));
	}
	else if (preview_mode == PreView::Diff) {
		mode_result = print_difference(result->value, result->old_value);
	}
	else if (preview_mode == PreView::Original) {
		mode_result = print_normal(result->old_value);
	}
	else {
		mode_result = print_stenar(result->value);
	}
	return mode_result;
}

//public (> preview dialog context menu)
pfc::string8 ILOD_preview::GetListRow(size_t row, PreView mode) {
	pfc::string8 mode_result = print_result_in_mode(m_tag_writer->tag_results[row], mode).c_str();
	return mode_result;
}

// fb2k lib - get subitems

pfc::string8 ILOD_preview::listGetSubItemText(ctx_t ctx, size_t item, size_t subItem) {

	auto auilist = this->ilo_get_uilist();
	CPreviewList* uilist = (CPreviewList*)ilo_get_uilist();

	if (subItem == 0) {
		pfc::string8 mode_result = m_tag_writer->tag_results[item]->tag_entry->tag_name;
		return mode_result;
	}
	if (subItem == 1) {
		pfc::string8 mode_result = print_result_in_mode(m_tag_writer->tag_results[item], m_preview_mode).c_str();
		return mode_result;
	}
	if (subItem == 2) {
		pfc::string8 mode_result = m_v_stats.size() > 0 ? std::to_string(m_v_stats.at(item).m_totalwrites).c_str() : "n/a";
		return mode_result;
	}
	if (subItem == 3) {
		pfc::string8 mode_result = m_v_stats.size() > 0 ? std::to_string(m_v_stats.at(item).m_totalupdates).c_str() : "n/a";
		return mode_result;
	}
	if (subItem == 4) {
		pfc::string8 mode_result = m_v_stats.size() > 0 ? std::to_string(m_v_stats.at(item).totalskips).c_str() : "n/a";
		return mode_result;
	}
	if (subItem == 5) {
		pfc::string8 mode_result = m_v_stats.size() > 0 ? std::to_string(m_v_stats.at(item).totalequal).c_str() : "n/a";
		return mode_result;
	}
	return "";
}
// fb2k lib - action, edit, clicked - not implemented

void ILOD_preview::listSelChanged(ctx_t) {
	CPreviewList* uilist = (CPreviewList*)ilo_get_uilist();
	if (uilist->TableEdit_IsActive()) {
		uilist->TableEdit_Abort(true);
		uilist->UpdateItem(uilist->GetSingleSel());
	}
}

void ILOD_preview::listItemAction(ctx_t, size_t) {
	CPreviewList* uilist = (CPreviewList*)ilo_get_uilist();
	uilist->Default_Action();
}

bool ILOD_preview::listEditCanAdvanceHere(ctx_t, size_t item, size_t subItem, uint32_t whatHappened) {

	return subItem == 1 && is_result_editable(item);

	(void)item; (void)subItem, (void)whatHappened; return true;
}

uint32_t ILOD_preview::listGetEditFlags(ctx_t ctx, size_t item, size_t subItem) {
	return is_result_editable(item) ? 0 : InPlaceEdit::KFlagReadOnly;
}

pfc::string8 ILOD_preview::listGetEditField(ctx_t ctx, size_t item, size_t subItem, size_t& lineCount) {

	pfc::array_t<string_encoded_array> value = GetPreviewValue(m_tag_writer->tag_results[item]);
	if (item < m_tag_writer->tag_results.get_count() && subItem == 1 && value.get_size() > 1) {
		return "";
	}
	else {
		pfc::string8 out = listGetSubItemText(ctx, item, subItem);
		bool onelined = out.length() < CHARS_SHORTFIELD && out.find_first(';', 0) == pfc_infinite;
		lineCount = onelined ? 1 : LINES_LONGFIELD;
		return out;
	}
}

void ILOD_preview::listSetEditField(ctx_t ctx, size_t item, size_t subItem, const char* val) {
	CPreviewList* uilist = (CPreviewList*)ilo_get_uilist();
	if (!is_result_editable(item)) {
		uilist->TableEdit_Abort(false);
		return;
	}
	auto c = m_tag_writer->tag_results[item]->value.get_count();
	string_encoded_array sea_val;
	string_encoded_array* value = &(m_tag_writer->tag_results[item]->value[0]);

	if (value->has_array()) {

		std::vector<pfc::string8>vsplit;
		split(val, ";", 0, vsplit);
		for (auto w : vsplit) {
			sea_val.append_item(w.trim(' '));
		}
		sea_val.encode();
	}
	else {
		sea_val = val;
	}


	if (item < m_tag_writer->tag_results.get_count()) {
		if (subItem == 1) {

			m_tag_writer->tag_results[item]->value.force_reset();
			m_tag_writer->tag_results[item]->value.append_single(sea_val);
			((CPreviewTagsDialog*)this)->cb_generate_tags();
		}
	}
}

bool ILOD_preview::listIsColumnEditable(ctx_t, size_t subItem) {
	CPreviewList* uilist = (CPreviewList*)ilo_get_uilist();
	size_t item = uilist->GetSingleSel();
	return subItem == 1 && is_result_editable(item);
}

void ILOD_preview::listSubItemClicked(ctx_t ctx, size_t item, size_t subItem) {
	if (listIsColumnEditable(ctx, subItem)) {

		if (!m_tag_writer->tag_results[item]->tag_entry->freeze_tag_name) {

			//edit
			CPreviewList* uilist = (CPreviewList*)ilo_get_uilist();
			if (is_result_editable(item)) {
				uilist->TableEdit_Start(item, 1);
			}
		}
	}
}
bool ILOD_preview::is_result_editable(size_t item) {

	bool bres = item != ~0 && item < m_tag_writer->tag_results.get_count();
	bres &= m_preview_mode == PreView::Normal;
	bres &= !g_discogs->preview_modal_tag_dialog;
	bres &= !m_tag_writer->tag_results[item]->tag_entry->freeze_tag_name;
	return bres;
}

bool ILOD_preview::listIsSubItemGrayed(ctx_t, size_t item, size_t subItem) {
	return m_tag_writer->tag_results[item]->tag_entry->freeze_tag_name;
}
