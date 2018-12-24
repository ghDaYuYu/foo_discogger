#include "stdafx.h"

#include "preview_dialog.h"
#include "tag_mappings_dialog.h"
#include "tasks.h"
#include "track_matching_dialog.h"
#include "string_encoded_array.h"

#include <GdiPlus.h>
#pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;


#define WRITE_UPDATE_COL_WIDTH  85


inline void CPreviewTagsDialog::load_size() {
	int x = CONF.preview_tags_dialog_width;
	int y = CONF.preview_tags_dialog_height;
	if (x != 0 && y != 0) {
		SetWindowPos(nullptr, 0, 0, x, y, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
		CRect rectClient;
		GetClientRect(&rectClient);
		DlgResize_UpdateLayout(rectClient.Width(), rectClient.Height());
	}
	if (CONF.preview_tags_dialog_col1_width != 0) {
		ListView_SetColumnWidth(GetDlgItem(IDC_PREVIEW_LIST), 0, CONF.preview_tags_dialog_col1_width);
		ListView_SetColumnWidth(GetDlgItem(IDC_PREVIEW_LIST), 1, CONF.preview_tags_dialog_col2_width);
	}
	else {
		update_list_width(true);
	}
}

inline void CPreviewTagsDialog::save_size(int x, int y) {
	CONF.preview_tags_dialog_width = x;
	CONF.preview_tags_dialog_height = y;
	conf_changed = true;
}

CPreviewTagsDialog::~CPreviewTagsDialog() {
	if (conf_changed) {
		CONF.save();
	}
	g_discogs->preview_tags_dialog = nullptr;
}

LRESULT CPreviewTagsDialog::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	tag_results_list = GetDlgItem(IDC_PREVIEW_LIST);

	listview_helper::insert_column(tag_results_list, 0, "Tag Name", 0);
	listview_helper::insert_column(tag_results_list, 1, "Value(s)", 0);

	ListView_SetExtendedListViewStyleEx(tag_results_list, LVS_EX_GRIDLINES, LVS_EX_GRIDLINES);
	ListView_SetExtendedListViewStyleEx(tag_results_list, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

	CheckRadioButton(IDC_VIEW_NORMAL, IDC_VIEW_DEBUG, CONF.preview_mode == PREVIEW_NORMAL ? IDC_VIEW_NORMAL :
		CONF.preview_mode == PREVIEW_DIFFERENCE ? IDC_VIEW_DIFFERENCE :
		CONF.preview_mode == PREVIEW_ORIGINAL ? IDC_VIEW_ORIGINAL :
		IDC_VIEW_DEBUG);

	if (multi_mode) {
		::ShowWindow(uGetDlgItem(IDC_EDIT_TAG_MAPPINGS_BUTTON), false);
		::ShowWindow(uGetDlgItem(IDC_BACK_BUTTON), false);
		::ShowWindow(uGetDlgItem(IDC_SKIP_BUTTON), true);
		::ShowWindow(uGetDlgItem(IDCANCEL), false);
	}
	else {
		::ShowWindow(uGetDlgItem(IDC_EDIT_TAG_MAPPINGS_BUTTON), true);
		::ShowWindow(uGetDlgItem(IDC_BACK_BUTTON), true);
		::ShowWindow(uGetDlgItem(IDC_SKIP_BUTTON), false);
		::ShowWindow(uGetDlgItem(IDCANCEL), true);
	}

	::SetFocus(GetDlgItem(IDC_WRITE_TAGS_BUTTON));

	initialize();

	DlgResize_Init(true, true);
	load_size();
	modeless_dialog_manager::g_add(m_hWnd);
	show();
	return TRUE;
}

bool CPreviewTagsDialog::initialize() {
	if (!tag_writer) {
		return get_next_tag_writer();
	}

	bool release_has_anv = tag_writer->release->has_anv();
	CheckDlgButton(IDC_REPLACE_ANV_CHECK, release_has_anv && CONF.replace_ANVs);

	// Album art
	if (tag_writer->release->small_art.get_size() > 0 && CONF.display_art) {
		pfc::string8 temp_path, temp_file;
		uGetTempPath(temp_path);
		uGetTempFileName(temp_path, "fb2k", 0, temp_file);

		FILE *fd;
		if (fopen_s(&fd, temp_file.get_ptr(), "wb") != 0) {
			pfc::string8 msg("can't open ");
			msg << temp_file;
			log_msg(msg);
		}
		if (fwrite(tag_writer->release->small_art.get_ptr(), tag_writer->release->small_art.get_size(), 1, fd) != 1) {
			pfc::string8 msg("error writing ");
			msg << temp_file;
			log_msg(msg);
		}
		fclose(fd);

		pfc::stringcvt::string_wide_from_ansi tempf(temp_file);
		Bitmap bm(tempf);
		HBITMAP hBm;
		if (bm.GetHBITMAP(Color::Black, &hBm) == Ok) {
			uSendDlgItemMessage(IDC_ALBUM_ART, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBm);
		}
		else {
			log_msg("GdiPlus error (GetHBITMAP)");
		}
	}
	insert_tag_results();
	return true;
}

void CPreviewTagsDialog::insert_tag_results() {
	ListView_DeleteAllItems(tag_results_list);
	const size_t count = tag_writer->tag_results.get_count();
	for (unsigned int i = 0; i < count; i++) {
		insert_tag_result(i, tag_writer->tag_results[i]);
	}
}

/*pfc::string8 parse_strenar(const pfc::string8 &input) {
	pfc::string8 output;
	for (unsigned int i = 0; i < input.get_length(); i++) {
		if (input[i] == LIST_SEP_PRETTY) {
			output.add_char(LIST_SEP);
		}
		else if (input[i] == LIST_START_PRETTY) {
			output.add_char(LIST_START);
		}
		else if (input[i] == LIST_END_PRETTY) {
			output.add_char(LIST_END);
		}
		else {
			output.add_char(input[i]);
		}
	}
	return output;
}*/

void CPreviewTagsDialog::TableEdit_GetField(size_t item, size_t subItem, pfc::string_base & out, size_t & lineCount) {
	if (item < tag_writer->tag_results.get_count() && subItem == 1 && tag_writer->tag_results[item]->value.get_size() > 1) {
		return;
	}
	else {
		InPlaceEdit::CTableEditHelperV2_ListView::TableEdit_GetField(item, subItem, out, lineCount);
	}
}

void CPreviewTagsDialog::TableEdit_SetField(size_t item, size_t subItem, const char * value) {
	InPlaceEdit::CTableEditHelperV2_ListView::TableEdit_SetField(item, subItem, value);
	if (item < tag_writer->tag_results.get_count()) {
		if (subItem == 1) {
			tag_writer->tag_results[item]->value.force_reset();
			tag_writer->tag_results[item]->value.append_single(string_encoded_array(value));
		}
	}
}

pfc::string8 print_stenar(const pfc::array_t<string_encoded_array> &input) {
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

pfc::string8 print_normal(const pfc::array_t<string_encoded_array> &input) {
	pfc::string8 result = "";
	if (input.get_size() == 1) {
		result = input[0].print();
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
		if (parts.get_size()) {
			result << "(multiple values) " << join(parts, "; ");
		}
	}
	return result;
}

pfc::string8 print_difference(const pfc::array_t<string_encoded_array> &input, const pfc::array_t<string_encoded_array> &old_input) {
	const size_t count = input.get_count();
	const size_t old_count = old_input.get_count();
	const size_t max_count = max(count, old_count);
	
	// TODO: differentiate no change from change to blank

	pfc::string8 result = "";
	if (max_count == 1) {
		if (!STR_EQUAL(input[0].print(), old_input[0].print())) {
			result = input[0].print();
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
	}
	return result;
}

void CPreviewTagsDialog::insert_tag_result(int pos, const tag_result_ptr &result) {
	if (CONF.preview_mode == PREVIEW_NORMAL) {
		listview_helper::insert_item2(tag_results_list, pos, result->tag_entry->tag_name, print_normal(result->value), 0);
	}
	else if (CONF.preview_mode == PREVIEW_DIFFERENCE) {
		listview_helper::insert_item2(tag_results_list, pos, result->tag_entry->tag_name, print_difference(result->value, result->old_value), 0);
	}
	else if (CONF.preview_mode == PREVIEW_ORIGINAL) {
		listview_helper::insert_item2(tag_results_list, pos, result->tag_entry->tag_name, print_normal(result->old_value), 0);
	}
	else {
		listview_helper::insert_item2(tag_results_list, pos, result->tag_entry->tag_name, print_stenar(result->value), 0);
	}
}

void CPreviewTagsDialog::refresh_item(int pos) {
	const tag_result_ptr &result = tag_writer->tag_results[pos];
	listview_helper::set_item_text(tag_results_list, pos, 0, result->tag_entry->tag_name);
	if (CONF.preview_mode == PREVIEW_NORMAL) {
		listview_helper::set_item_text(tag_results_list, pos, 1, print_normal(result->value));
	}
	else if (CONF.preview_mode == PREVIEW_DIFFERENCE) {
		listview_helper::set_item_text(tag_results_list, pos, 1, print_difference(result->value, result->old_value));
	}
	else if (CONF.preview_mode == PREVIEW_ORIGINAL) {
		listview_helper::set_item_text(tag_results_list, pos, 1, print_normal(result->old_value));
	}
	else {
		listview_helper::set_item_text(tag_results_list, pos, 1, print_stenar(result->value));
	}
}

void CPreviewTagsDialog::tag_mappings_updated() {
	generating_tags = true;
	service_ptr_t<generate_tags_task> task = new service_impl_t<generate_tags_task>(this, tag_writer, use_update_tags);
	task->start();
}

void CPreviewTagsDialog::update_list_width(bool initialize) {
	CRect client_rectangle;
	::GetClientRect(tag_results_list, &client_rectangle);
	int width = client_rectangle.Width();

	int c1, c2;
	if (initialize) {
		c1 = width / 3;
		c2 = width / 3 * 2;
		ListView_SetColumnWidth(tag_results_list, 0, c1);
		ListView_SetColumnWidth(tag_results_list, 1, c2);
	}
	else {
		c1 = ListView_GetColumnWidth(tag_results_list, 0);
		c2 = ListView_GetColumnWidth(tag_results_list, 1);
	}
	CONF.preview_tags_dialog_col1_width = c1;
	CONF.preview_tags_dialog_col2_width = c2;
	conf_changed = true;
}

LRESULT CPreviewTagsDialog::OnCheckReplaceANVs(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	CONF.replace_ANVs = IsDlgButtonChecked(IDC_REPLACE_ANV_CHECK) != 0;
	conf_changed = true;
	tag_mappings_updated();
	return FALSE;
}

LRESULT CPreviewTagsDialog::OnEditTagMappings(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (!g_discogs->tag_mappings_dialog) {
		g_discogs->tag_mappings_dialog = new CNewTagMappingsDialog(core_api::get_main_window());
	}
	return FALSE;
}

LRESULT CPreviewTagsDialog::OnChangePreviewMode(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	CONF.preview_mode = IsDlgButtonChecked(IDC_VIEW_NORMAL) ? PREVIEW_NORMAL :
		IsDlgButtonChecked(IDC_VIEW_DIFFERENCE) ? PREVIEW_DIFFERENCE :
		IsDlgButtonChecked(IDC_VIEW_ORIGINAL) ? PREVIEW_ORIGINAL :
		PREVIEW_DEBUG;

	conf_changed = true;
	insert_tag_results();
	return FALSE;
}

LRESULT CPreviewTagsDialog::OnWriteTags(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (multi_mode) {
		get_next_tag_writer();
	}
	else {
		service_ptr_t<write_tags_task> task = new service_impl_t<write_tags_task>(tag_writer);
		task->start();
		destroy_all();
	}
	return TRUE;
}

void CPreviewTagsDialog::cb_generate_tags() {
	generating_tags = false;
	insert_tag_results();
	enable(true);
}

LRESULT CPreviewTagsDialog::OnBack(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	go_back();
	return TRUE;
}


bool CPreviewTagsDialog::init_count() {
	if (!CONF.update_tags_preview_changes) {
		return false;
	}
	for (size_t i = 0; i < tag_writers.get_count(); i++) {
		if (!tag_writers[i]->skip && tag_writers[i]->changed) {
			multi_count++;
		}
	}
	return multi_count != 0;
}

bool CPreviewTagsDialog::get_next_tag_writer() {
	while (tw_index < tag_writers.get_count()) {
		tag_writer = tag_writers[tw_index++];
		if (tag_writer->skip || !tag_writer->changed) {
			tw_skip++;
			continue;
		}
		update_window_title();
		initialize();
		return true;
	}
	finished_tag_writers();
	destroy();
	return false;
}

void CPreviewTagsDialog::finished_tag_writers() {
	service_ptr_t<write_tags_task_multi> task = new service_impl_t<write_tags_task_multi>(tag_writers);
	task->start();
}


LRESULT CPreviewTagsDialog::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (multi_mode) {
		pfc::string8 msg;
		msg << "Are you sure you want to cancel tagging of " << multi_count << " releases?";
		if (tag_writers.get_count() == 1 || uMessageBox(m_hWnd, msg, "Update Tags", MB_OKCANCEL | MB_ICONINFORMATION)) {
			destroy();
		}
	}
	else {
		destroy_all();
	}
	return TRUE;
}

LRESULT CPreviewTagsDialog::OnMultiSkip(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	PFC_ASSERT(multi_mode);
	tag_writer->skip = true;
	get_next_tag_writer();
	return TRUE;
}

LRESULT CPreviewTagsDialog::OnListClick(LPNMHDR lParam) {
	NMITEMACTIVATE* info = reinterpret_cast<NMITEMACTIVATE*>(lParam);
	if (info->iItem >= 0) {
		if (CONF.preview_mode == PREVIEW_NORMAL && TableEdit_IsColumnEditable(info->iSubItem)) {
			const tag_result_ptr &result = tag_writer->tag_results[info->iItem];
			if (!result->tag_entry->freeze_tag_name) {
				trigger_edit(info->iItem, info->iSubItem);
			}
		}
	}
	return FALSE;
}

bool CPreviewTagsDialog::delete_selection() {
	const int index = ListView_GetSingleSelection(tag_results_list);
	if (index < 0) {
		return false;
	}
	ListView_DeleteItem(tag_results_list, index);
	erase(tag_writer->tag_results, index);
	//tag_writer->tag_results.remove_by_idx(index);

	int total = ListView_GetItemCount(tag_results_list);
	if (index < total) {
		listview_helper::select_single_item(tag_results_list, index);
	}
	else if (total > 0) {
		listview_helper::select_single_item(tag_results_list, total - 1);
	}
	return true;
}

LRESULT CPreviewTagsDialog::OnListKeyDown(LPNMHDR lParam) {
	NMLVKEYDOWN * info = reinterpret_cast<NMLVKEYDOWN*>(lParam);
	switch (info->wVKey) {
	case VK_DELETE:
		return delete_selection() ? TRUE : FALSE;

	case VK_F2:
		{
			int index = ListView_GetSingleSelection(tag_results_list);
			if (index >= 0) {
				trigger_edit(index, IsKeyPressed(VK_SHIFT) ? 0 : 1);
			}
		}
		return TRUE;

	default:
		return FALSE;
	}
}

LRESULT CPreviewTagsDialog::OnColumnResized(LPNMHDR lParam) {
	int width1 = ListView_GetColumnWidth(GetDlgItem(IDC_PREVIEW_LIST), 0);
	int width2 = ListView_GetColumnWidth(GetDlgItem(IDC_PREVIEW_LIST), 1);
	bool width_changed = (width1 != CONF.preview_tags_dialog_col1_width ||
		width2 != CONF.preview_tags_dialog_col2_width);
	if (width_changed) {
		CONF.preview_tags_dialog_col1_width = width1;
		CONF.preview_tags_dialog_col2_width = width2;
		conf_changed = true;
	}
	return FALSE;
}
LRESULT CPreviewTagsDialog::OnEdit(UINT uMsg, WPARAM wParam, LPARAM lParam) {
	TableEdit_Start(wParam, lParam);
	return FALSE;
}


#define DISABLED_RGB	RGB(150, 150, 150)


LRESULT CPreviewTagsDialog::OnCustomDraw(int idCtrl, LPNMHDR lParam, BOOL& bHandled) {
	if (generating_tags) {
		return CDRF_DODEFAULT;
	}
	LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;
	int pos = (int)lplvcd->nmcd.dwItemSpec;
	int sub_item;
	const tag_mapping_entry *entry = tag_writer->tag_results[pos]->tag_entry;

	switch (lplvcd->nmcd.dwDrawStage) {
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT:
		return CDRF_NOTIFYSUBITEMDRAW;

	case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
		sub_item = lplvcd->iSubItem;
		if ((sub_item == 0 || sub_item == 1) && entry->freeze_tag_name) {
			lplvcd->clrText = DISABLED_RGB;
			return CDRF_NEWFONT;
		}
		/*else if (sub_item == 2 && entry.freeze_write && entry.freeze_update) {
			lplvcd->clrText = DISABLED_RGB;
			return CDRF_NEWFONT;
		}*/
		/*else {
			lplvcd->clrText = DEFAULT_RGB;
			return CDRF_NEWFONT;
		}*/
	}
	return CDRF_DODEFAULT;
}

void CPreviewTagsDialog::OnFinalMessage(HWND /*hWnd*/) {
	modeless_dialog_manager::g_remove(m_hWnd);
	delete this;
}

void CPreviewTagsDialog::enable(bool is_enabled) {
	::uEnableWindow(GetDlgItem(IDC_WRITE_TAGS_BUTTON), is_enabled);
	::uEnableWindow(GetDlgItem(IDCANCEL), is_enabled);
	::uEnableWindow(GetDlgItem(IDC_BACK_BUTTON), is_enabled);
	::uEnableWindow(GetDlgItem(IDC_EDIT_TAG_MAPPINGS_BUTTON), is_enabled);
	::uEnableWindow(GetDlgItem(IDC_REPLACE_ANV_CHECK), is_enabled);
	::uEnableWindow(GetDlgItem(IDC_PREVIEW_LIST), is_enabled);
}

void CPreviewTagsDialog::destroy_all() {
	if (!multi_mode) {
		g_discogs->track_matching_dialog->destroy_all();
	}
	MyCDialogImpl<CPreviewTagsDialog>::destroy();
}

void CPreviewTagsDialog::go_back() {
	PFC_ASSERT(!multi_mode);
	destroy();
	g_discogs->track_matching_dialog->show();
}
