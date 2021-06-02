#include "stdafx.h"
#include <CommCtrl.h>

#include "preview_dialog.h"
#include "tag_mappings_dialog.h"
#include "tasks.h"
#include "track_matching_dialog.h"
#include "string_encoded_array.h"

#include <GdiPlus.h>
#pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;

struct cfg_prev_col {
	int ndx = 0;
	int id = 0;
	int icol = 0;
	pfc::string8 name = "";
	pfc::string8 titleformat = "";
	float width = 0.0f;
	int celltype = 1;
	int custtype = 0;
	bool factory = false;
	bool default = false;
	bool enabled = true;
	bool defuser = false;
};

using cfg_prev_ColMap = std::map<std::int32_t, cfg_prev_col>;

const int TOTAL_DEF_COLS = 9;
const int COL_STAT_COLS = 4;
const int COL_STAT_POS = 2;

cfg_prev_col column_configs[TOTAL_DEF_COLS] = {
	//#, ndx, icol, name, tf, width, celltype, factory, default, enabled, defuser
	{1, 1, 0,"Tag Name", "", 20.0f, 0x0000, 0, true, true, false, false},
	{2, 2, 1,"Value(s)", "", 40.0f, 0x0000, 0, true, true, false, false},
	{3, 3, 2,"Write", "", 40.0f, 0x0000, 0, true, false, false, false},
	{4, 4, 3,"Update", "", 40.0f, 0x0000, 0, true, false, false, false},
	{5, 5, 4,"Skip W/U", "", 40.0f, 0x0000, 0, true,false, false, false},
	{6, 6, 5,"Equal", "", 40.0f, 0x0000, 0, true, false, false, false},
	{7, 7, 6,"---x", "", 20.0f, 0x0000, 0, false, false, false, false},
	{8, 8, 7,"---y", "", 50.0f, 0x0000, 0, false, false, false, false},
	{100, 10, 0,"---z", "", 200.0f, 0x0000, 0, false, false, false, false},
};

struct preview_lv_def {
	bool autoscale = false;
	cfg_prev_ColMap colmap = {};
	uint32_t flags = 0;
} cfg_lv;

static void defaultCfgCols(preview_lv_def& cfg_out) {
	cfg_out.autoscale = false;
	cfg_out.colmap.clear();
	for (int it = 0; it < PFC_TABSIZE(column_configs); it++) {
		if (!column_configs[it].name.has_prefix("---")) {
			cfg_prev_col tmp_cfgcol{
				column_configs[it].id,
				column_configs[it].ndx,
				column_configs[it].icol,
				column_configs[it].name,
				column_configs[it].titleformat,
				column_configs[it].width,
				column_configs[it].celltype,
				column_configs[it].custtype,
				column_configs[it].factory,
				column_configs[it].default,
				column_configs[it].enabled,
				column_configs[it].defuser,
			};
			cfg_out.colmap.emplace(cfg_out.colmap.size(), tmp_cfgcol);
		}
	}
}

inline void CPreviewTagsDialog::load_size() {

	int width = conf.preview_tags_dialog_width;
	int height = conf.preview_tags_dialog_height;
	CRect rcCfg(0, 0, width, height);
	::CenterWindow(this->m_hWnd, rcCfg, core_api::get_main_window());

	if (conf.preview_tags_dialog_col1_width != 0) {
		ListView_SetColumnWidth(GetDlgItem(IDC_PREVIEW_LIST), 0, conf.preview_tags_dialog_col1_width);
		ListView_SetColumnWidth(GetDlgItem(IDC_PREVIEW_LIST), 1, conf.preview_tags_dialog_col2_width);
	}
	else {
		CRect client_rectangle;
		::GetClientRect(tag_results_list, &client_rectangle);
		int width = client_rectangle.Width();

		int c1 = width / 3;
		int	c2 = width / 3 * 2;
		ListView_SetColumnWidth(tag_results_list, 0, c1);
		ListView_SetColumnWidth(tag_results_list, 1, c2);
	}
}

inline bool CPreviewTagsDialog::build_current_cfg() {
	bool bres = false;

	//check global settings
	
	bool local_replace_ANV = IsDlgButtonChecked(IDC_REPLACE_ANV_CHECK);
	if (CONF.replace_ANVs!= conf.replace_ANVs &&
		conf.replace_ANVs != local_replace_ANV) {
		bres = bres || true;
	}

	//get current ui dimensions
	RECT rectDlg = {};
	GetClientRect(&rectDlg);
	int dlgcx = rectDlg.right;
	int dlgcy = rectDlg.bottom;
	int dlgwidth = rectDlg.right - rectDlg.left;
	int dlgheight = rectDlg.bottom - rectDlg.top;

	int colwidth1 = ListView_GetColumnWidth(GetDlgItem(IDC_PREVIEW_LIST), 0);
	int colwidth2 = ListView_GetColumnWidth(GetDlgItem(IDC_PREVIEW_LIST), 1);

	//preview mode
	if (int preview_mode = get_preview_mode(); preview_mode != conf.preview_mode) {
		conf.preview_mode = preview_mode;
		bres = bres || true;
	}
	
	//columns
	if (colwidth1 != conf.preview_tags_dialog_col1_width || colwidth2 != conf.preview_tags_dialog_col2_width) {
		conf.preview_tags_dialog_col1_width = colwidth1;
		conf.preview_tags_dialog_col2_width = colwidth2;
		bres = bres || true;
	}
	//dlg size
	if (dlgheight != conf.preview_tags_dialog_height || dlgwidth != conf.preview_tags_dialog_width) {
		conf.preview_tags_dialog_width = dlgwidth;
		conf.preview_tags_dialog_height = dlgheight;
		bres = bres || true;
	}

	//show tm stats
	const bool benabled = uButton_GetCheck(m_hWnd, IDC_CHECK_PREV_DLG_SHOW_STATS);
	if (benabled != conf.edit_tags_dlg_show_tm_stats) {
		conf.edit_tags_dlg_show_tm_stats = benabled;
		bres = bres || true;
	}

	return bres;
}

CPreviewTagsDialog::~CPreviewTagsDialog() {
	g_discogs->preview_tags_dialog = nullptr;
}

LRESULT CPreviewTagsDialog::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	
	conf = CONF;
	
	defaultCfgCols(cfg_lv);

	tag_results_list = GetDlgItem(IDC_PREVIEW_LIST);

	ListView_SetExtendedListViewStyleEx(tag_results_list, LVS_EX_GRIDLINES, LVS_EX_GRIDLINES);
	ListView_SetExtendedListViewStyleEx(tag_results_list, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

	reset_default_columns(tag_results_list, true);

	set_preview_mode(conf.preview_mode);

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

	if (conf.edit_tags_dlg_show_tm_stats) {
		//todo: revise hacky
		uButton_SetCheck(m_hWnd, IDC_CHECK_PREV_DLG_SHOW_STATS, true);
		BOOL bdummy = false;
		OnCheckPreviewShowStats(0, 0, NULL, bdummy);
	}

	// todo: integrate into enable(bool, bool)
	// note: test generate_tags_task::on_success(HWND p_wnd)
	if (tag_writer->will_modify)
		::SetFocus(GetDlgItem(IDC_WRITE_TAGS_BUTTON));
	else
		::SetFocus(GetDlgItem(IDCANCEL));

	::EnableWindow(GetDlgItem(IDC_WRITE_TAGS_BUTTON), tag_writer->will_modify);

	return TRUE;
}

bool CPreviewTagsDialog::initialize() {
	if (!tag_writer) {
		return get_next_tag_writer();
	}

	bool release_has_anv = tag_writer->release->has_anv();
	CheckDlgButton(IDC_REPLACE_ANV_CHECK, release_has_anv && conf.replace_ANVs);
	CheckDlgButton(IDC_CHECK_PREV_DLG_DIFF_TRACKS, cfg_preview_dialog_track_map);

	// Album art
	if (tag_writer->release->small_art.get_size() > 0 && (conf.save_album_art || conf.embed_album_art)) {
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
	insert_tag_results(true);
	return true;
}
void CPreviewTagsDialog::reset_tag_result_stats() {
	totalwrites = 0;
	totalupdates = 0;
}

void CPreviewTagsDialog::display_tag_result_stats() {
	HWND stat_ctrl = uGetDlgItem(IDC_PREVIEW_STATS);

	if (cfg_preview_dialog_track_map) {

		uSetDlgItemText(m_hWnd, IDC_PREVIEW_STATS, "");
	}
	else {

		HWND stat_ctrl = uGetDlgItem(IDC_PREVIEW_STATS);
		std::string strstat = std::to_string(totalwrites);
		strstat.append(" writes, ");
		strstat.append(std::to_string(totalupdates));
		strstat.append(" updates");

		uSetDlgItemText(m_hWnd, IDC_PREVIEW_STATS, strstat.c_str());
	}
}

void CPreviewTagsDialog::insert_tag_results(bool computestat) {
	ListView_DeleteAllItems(tag_results_list);
	if (computestat) {
		if (cfg_preview_dialog_track_map) {
			v_stats.clear();
		}
		else {
			reset_tag_result_stats();

			if (cfg_preview_dialog_comp_track_map_in_v23)
				v_stats.clear();
		}
		
		compute_stats(tag_writer->tag_results);
	}

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
static pfc::array_t<string_encoded_array>GetPreviewValue(const tag_result_ptr& result) {
	pfc::array_t<string_encoded_array> value;
	if (!result->result_approved && result->changed)
		return result->old_value;
	else
		return result->value;
}
void CPreviewTagsDialog::TableEdit_GetField(size_t item, size_t subItem, pfc::string_base & out, size_t & lineCount) {
	pfc::array_t<string_encoded_array> value = GetPreviewValue(tag_writer->tag_results[item]);
	if (item < tag_writer->tag_results.get_count() && subItem == 1 && value.get_size() > 1) {
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

void CPreviewTagsDialog::insert_tag_result(int pos, const tag_result_ptr &result) {
	int preview_mode = get_preview_mode();
	if (preview_mode == PREVIEW_NORMAL) {
		listview_helper::insert_item2(tag_results_list, pos, result->tag_entry->tag_name, print_normal(GetPreviewValue(result)), 0);
	}
	else if (preview_mode == PREVIEW_DIFFERENCE) {
		listview_helper::insert_item2(tag_results_list, pos, result->tag_entry->tag_name, print_difference(result->value, result->old_value), 0);
	}
	else if (preview_mode == PREVIEW_ORIGINAL) {
		listview_helper::insert_item2(tag_results_list, pos, result->tag_entry->tag_name, print_normal(result->old_value), 0);
	}
	else {
		listview_helper::insert_item2(tag_results_list, pos, result->tag_entry->tag_name, print_stenar(result->value), 0);
	}
	
	if (cfg_preview_dialog_show_stats) {
		listview_helper::set_item_text(tag_results_list, pos, 2, v_stats.size() > 0 ? pfc::toString(v_stats.at(pos).totalwrites).c_str() : "n/a");
		listview_helper::set_item_text(tag_results_list, pos, 3, v_stats.size() > 0 ? pfc::toString(v_stats.at(pos).totalupdates).c_str() : "n/a");
		listview_helper::set_item_text(tag_results_list, pos, 4, v_stats.size() > 0 ? pfc::toString(v_stats.at(pos).totalskips).c_str() : "n/a");
		listview_helper::set_item_text(tag_results_list, pos, 5, v_stats.size() > 0 ? pfc::toString(v_stats.at(pos).totalequal).c_str() : "n/a");
	}

}

void CPreviewTagsDialog::refresh_item(int pos) {
	int mode = get_preview_mode();
	const tag_result_ptr &result = tag_writer->tag_results[pos];
	listview_helper::set_item_text(tag_results_list, pos, 0, result->tag_entry->tag_name);
	if (mode == PREVIEW_NORMAL) {
		pfc::array_t<string_encoded_array> value = GetPreviewValue(result);
		listview_helper::set_item_text(tag_results_list, pos, 1, print_normal(value));
	}
	else if (mode == PREVIEW_DIFFERENCE) {
		listview_helper::set_item_text(tag_results_list, pos, 1, print_difference(result->value, result->old_value));
	}
	else if (mode == PREVIEW_ORIGINAL) {
		listview_helper::set_item_text(tag_results_list, pos, 1, print_normal(result->old_value));
	}
	else {
		listview_helper::set_item_text(tag_results_list, pos, 1, print_stenar(result->value));
	}

	if (cfg_preview_dialog_show_stats) {
		listview_helper::set_item_text(tag_results_list, pos, 2, "1");
		listview_helper::set_item_text(tag_results_list, pos, 3, "2");
		listview_helper::set_item_text(tag_results_list, pos, 4, "3");
		listview_helper::set_item_text(tag_results_list, pos, 5, "4");
	
	}
}

void CPreviewTagsDialog::tag_mappings_updated() {
	generating_tags = true;
	service_ptr_t<generate_tags_task> task = new service_impl_t<generate_tags_task>(this, tag_writer, use_update_tags);
	task->start();
}

void CPreviewTagsDialog::GlobalReplace_ANV(bool state) {
	CONF.replace_ANVs = state;
	if (g_discogs->configuration_dialog) {
		CDialogImpl* cfgdlg = pfc::downcast_guarded<CDialogImpl*>(g_discogs->configuration_dialog);
		SendMessage(cfgdlg->m_hWnd, 
			WM_CUSTOM_ANV_CHANGED, 0, state);
	}
}

LRESULT CPreviewTagsDialog::OnCheckReplaceANVs(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	bool local_replace_ANV = IsDlgButtonChecked(IDC_REPLACE_ANV_CHECK);
	if (CONF.replace_ANVs != local_replace_ANV) {
		conf.replace_ANVs = local_replace_ANV;
		GlobalReplace_ANV(local_replace_ANV);
		tag_mappings_updated();
	} 
	return FALSE;
}

LRESULT CPreviewTagsDialog::OnCheckTrackMap(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	cfg_preview_dialog_track_map = IsDlgButtonChecked(IDC_CHECK_PREV_DLG_DIFF_TRACKS);
	if (ListView_GetItemCount(tag_results_list) > 0) {
		ListView_RedrawItems(tag_results_list, 0, ListView_GetItemCount(tag_results_list));
		//TODO: tmp (1/2) fix empty v23 stats when Track map is on
		compute_stats(tag_writer->tag_results);
	}
	return FALSE;
}

LRESULT CPreviewTagsDialog::OnEditTagMappings(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (!g_discogs->tag_mappings_dialog) {
		g_discogs->tag_mappings_dialog = new CNewTagMappingsDialog(core_api::get_main_window());
	}
	else
	{
		::SetFocus(g_discogs->tag_mappings_dialog->m_hWnd);
	}
	return FALSE;
}

void CPreviewTagsDialog::set_preview_mode(int mode) {
	CheckRadioButton(IDC_VIEW_NORMAL, IDC_VIEW_DEBUG, mode == PREVIEW_NORMAL ? IDC_VIEW_NORMAL :
		mode == PREVIEW_DIFFERENCE ? IDC_VIEW_DIFFERENCE :
		mode == PREVIEW_ORIGINAL ? IDC_VIEW_ORIGINAL :
		IDC_VIEW_DEBUG);
}
int CPreviewTagsDialog::get_preview_mode() {
	return IsDlgButtonChecked(IDC_VIEW_NORMAL) ? PREVIEW_NORMAL :
		IsDlgButtonChecked(IDC_VIEW_DIFFERENCE) ? PREVIEW_DIFFERENCE :
		IsDlgButtonChecked(IDC_VIEW_ORIGINAL) ? PREVIEW_ORIGINAL :
		PREVIEW_DEBUG;
}

LRESULT CPreviewTagsDialog::OnChangePreviewMode(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int top = ListView_GetTopIndex(tag_results_list);
	int sel = ListView_GetSelectionMark(tag_results_list);

	SendMessage(tag_results_list, WM_SETREDRAW, FALSE, 0);
	insert_tag_results(false);

	SendMessage(tag_results_list, LVM_ENSUREVISIBLE,
		SendMessage(tag_results_list, LVM_GETITEMCOUNT, 0, 0) - 1, TRUE); //Rool to the end
	SendMessage(tag_results_list, LVM_ENSUREVISIBLE, top/* - 1*/, TRUE); //Roll back to top position

	ListView_SetItemState(tag_results_list, sel, LVNI_SELECTED | LVNI_FOCUSED, LVNI_SELECTED | LVNI_FOCUSED);
	SendMessage(tag_results_list, WM_SETREDRAW, TRUE, 0);
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
	insert_tag_results(true);
	enable(true, true);
}

LRESULT CPreviewTagsDialog::OnBack(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	go_back();
	return TRUE;
}


bool CPreviewTagsDialog::init_count() {
	if (!conf.update_tags_preview_changes) {
		return false;
	}
	for (size_t i = 0; i < tag_writers.get_count(); i++) {
		if (!tag_writers[i]->skip && tag_writers[i]->will_modify) {
			multi_count++;
		}
	}
	return multi_count != 0;
}

bool CPreviewTagsDialog::get_next_tag_writer() {
	while (tw_index < tag_writers.get_count()) {
		tag_writer = tag_writers[tw_index++];
		if (tag_writer->skip || !tag_writer->will_modify) {
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

void CPreviewTagsDialog::pushcfg() {
	bool conf_changed = build_current_cfg();
	//TODO: will always change
	//v.2.23 missing preview col width persistence
	if (conf_changed) {
		CONF.save(new_conf::ConfFilter::CONF_FILTER_PREVIEW, conf);
		CONF.load();
	}
}

LRESULT CPreviewTagsDialog::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	pushcfg();
	return 0;
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
		if (get_preview_mode() == PREVIEW_NORMAL && TableEdit_IsColumnEditable(info->iSubItem)) {
			//enable edits
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

LRESULT CPreviewTagsDialog::OnEdit(UINT uMsg, WPARAM wParam, LPARAM lParam) {
	t_size item = wParam;
	t_size subItem = lParam;
	if (tag_writer->tag_results[item]->result_approved)
		TableEdit_Start(wParam, lParam);
	return FALSE;
}


#define DISABLED_RGB	RGB(150, 150, 150)
#define CHANGE_NOT_APPROVED_RGB	RGB(150, 100, 100)

void CPreviewTagsDialog::compute_stats(tag_results_list_type tag_results) {
	//TODO: tmp (2/2) fix empty v23 stats when Track map is on
	reset_tag_result_stats();
	compute_stats_v23(tag_results);

	if (cfg_preview_dialog_track_map)
		compute_stats_track_map(tag_results);
	else {
		//compute_stats_v23(tag_results);
		if (cfg_preview_dialog_comp_track_map_in_v23)
			compute_stats_track_map(tag_results);
	}

}
void CPreviewTagsDialog::compute_stats_track_map(tag_results_list_type tag_results) {

	int createdtags = 0;
	int updatedtags = 0;

	v_stats.clear();

	const size_t tagcount = tag_writer->tag_results.get_count();

	for (unsigned int walk_tag = 0; walk_tag < tagcount; walk_tag++)
	{

		preview_stats stats;

		const tag_result_ptr res = tag_writer->tag_results[walk_tag];

		const tag_mapping_entry* entry = res->tag_entry;

		int subwrites = 0, subupdates = 0, subskips = 0, wereequal = 0;

		const size_t tk_count = res->r_approved.get_count();

		preview_stats before_tag_loop_stats = stats;

		for (size_t walk_tk = 0; walk_tk < tk_count; walk_tk++) {

			const bool r_approved = res->r_approved[walk_tk];

			string_encoded_array* oldvalue;
			string_encoded_array* value;

			if (res->old_value.get_size() == 1) {
				oldvalue = &(res->old_value[0]);
			}
			else {
				oldvalue = &(res->old_value[walk_tk]);
			}

			if (res->value.get_size() == 1) {
				value = &(res->value[0]);
			}
			else {
				value = &(res->value[walk_tk]);
			}

			const bool r_changed = oldvalue->has_diffs(*value);

			if (r_changed) {

				int old_value_cv_length = oldvalue->get_cvalue().get_length();

				if (old_value_cv_length == 0) {
					if (entry->enable_write && r_approved) {
						subwrites++;
					}
					else
						if (entry->enable_update)
							subskips++;
				}
				else {
					if (entry->enable_update && r_approved) {
						subupdates++;
					}
					else {
						subskips++;
					}
				}
			}
			else {

				if (r_approved)
					PFC_ASSERT(false);
				//no need to update old val = new val for this tag/track
				stats.totalequal++;
			}

			if (subwrites) {
				stats.totalsubwrites += subwrites;
				stats.totalwrites++;
				subwrites = 0;
			}
			if (subupdates) {
				stats.totalsubupdates += subupdates;
				stats.totalupdates++;
				subupdates = 0;
			}
			if (subskips) {
				stats.totalskips += subskips;
				subskips = 0;
			}
			if (wereequal) {
				stats.totalequal += wereequal;
				wereequal = 0;
			}
			
		} // track tag loop

		if (before_tag_loop_stats.totalwrites != stats.totalwrites)
			createdtags++;
		if (before_tag_loop_stats.totalupdates != stats.totalupdates)
			updatedtags++;

		v_stats.emplace_back(stats);


	} // tag loop
}

void CPreviewTagsDialog::compute_stats_v23(tag_results_list_type tag_results) {
	const size_t rescount = tag_writer->tag_results.get_count();
	for (unsigned int i = 0; i < rescount; i++) {

		const tag_result_ptr res = tag_writer->tag_results[i];
		const tag_mapping_entry* entry = res->tag_entry;
		bool discarded = !res->result_approved &&
			res->changed;
		if (!discarded) {

			if (!stricmp_utf8(entry->tag_name, "DISCOGS_RATING")
				|| !stricmp_utf8(entry->tag_name, "DISCOGS_TRACK_CREDITS")) {
				int kk = 1;
			}
	
			if (res->result_approved) {
				int oldlen = res->old_value.get_ptr()->get_cvalue().get_length();
				if (res->old_value.size() == 0 || oldlen == 0) {
					if (entry->enable_write)
						totalwrites++;
				}
				else {
					if (entry->enable_update)
						totalupdates++;
				}
			}
			else {
				//an approved non changed??
				totalupdates = totalupdates;
			}
		}
	} // tag writer tag results count loop
}

LRESULT CPreviewTagsDialog::OnCustomDraw(int idCtrl, LPNMHDR lParam, BOOL& bHandled) {
	if (generating_tags) {
		//TODO: mutex generating_tags
		return CDRF_DODEFAULT;
	}

	LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;
	int pos = (int)lplvcd->nmcd.dwItemSpec;
	int sub_item;
	bool bresults = tag_writer->tag_results.size() > 0;
	const tag_mapping_entry* entry = bresults ?
		tag_writer->tag_results[pos]->tag_entry: nullptr;
	
	switch (lplvcd->nmcd.dwDrawStage) {

	case CDDS_PREPAINT: {
		//TODO: create EnableEx(true | false | noresult)
		auto ctrl = uGetDlgItem(IDC_WRITE_TAGS_BUTTON);
		ctrl.EnableWindow(tag_writer->will_modify);
		ctrl = uGetDlgItem(IDC_REPLACE_ANV_CHECK);
		ctrl.EnableWindow(bresults && tag_writer->release->has_anv());
		ctrl = uGetDlgItem(IDC_CHECK_PREV_DLG_DIFF_TRACKS);
		ctrl.EnableWindow(cfg_preview_dialog_track_map || cfg_preview_dialog_comp_track_map_in_v23);
		ctrl = uGetDlgItem(IDC_VIEW_NORMAL);
		ctrl.EnableWindow(bresults);
		ctrl = uGetDlgItem(IDC_VIEW_DIFFERENCE);
		ctrl.EnableWindow(bresults);
		ctrl = uGetDlgItem(IDC_VIEW_ORIGINAL);
		ctrl.EnableWindow(bresults);
		ctrl = uGetDlgItem(IDC_VIEW_DEBUG);
		ctrl.EnableWindow(bresults);
		display_tag_result_stats();
		return CDRF_NOTIFYITEMDRAW;
	}

	case CDDS_ITEMPREPAINT:
		return CDRF_NOTIFYSUBITEMDRAW;

	case CDDS_SUBITEM | CDDS_ITEMPREPAINT: {
		sub_item = lplvcd->iSubItem;
		if (entry && (sub_item == 0 || sub_item == 1)) {
			bool bresview = get_preview_mode() == PREVIEW_NORMAL; /*BST_CHECKED == uButton_GetCheck(m_hWnd, IDC_VIEW_NORMAL);*/
			// todo: revise skipped changes color highlight
			bool bchgdiscarded = false && !tag_writer->tag_results[pos]->result_approved &&
				tag_writer->tag_results[pos]->changed;

			//if (sub_item == 0) compute_item(bresults, bchgdiscarded, tag_writer->tag_results[pos], entry);
			if (entry->freeze_tag_name) {
				if (bchgdiscarded && bresview)
					lplvcd->clrText = CHANGE_NOT_APPROVED_RGB;
				else
					lplvcd->clrText = DISABLED_RGB;
				return CDRF_NEWFONT;
			}
			else if (bchgdiscarded) {
				if (bresview) {
					lplvcd->clrText = CHANGE_NOT_APPROVED_RGB;
					return CDRF_NEWFONT;
				}
			}
		}
		if (entry && cfg_preview_dialog_show_stats &&
			(sub_item > 1 && sub_item < 6)) {

			return CDRF_NEWFONT;

		}
		break;
	}
	case CDDS_POSTPAINT:
		if (!cfg_preview_dialog_track_map)
			display_tag_result_stats();
	}
	return CDRF_DODEFAULT;
}

void CPreviewTagsDialog::OnFinalMessage(HWND /*hWnd*/) {
	modeless_dialog_manager::g_remove(m_hWnd);
	delete this;
}

void CPreviewTagsDialog::enable(bool is_enabled, bool change_focus) {
	::uEnableWindow(GetDlgItem(IDC_WRITE_TAGS_BUTTON), !is_enabled ? FALSE : tag_writer->will_modify/*is_enabled*/);
	::uEnableWindow(GetDlgItem(IDCANCEL), is_enabled);
	::uEnableWindow(GetDlgItem(IDC_BACK_BUTTON), is_enabled);
	::uEnableWindow(GetDlgItem(IDC_EDIT_TAG_MAPPINGS_BUTTON), is_enabled);
	::uEnableWindow(GetDlgItem(IDC_REPLACE_ANV_CHECK), is_enabled);
	::uEnableWindow(GetDlgItem(IDC_CHECK_PREV_DLG_DIFF_TRACKS), is_enabled);
	::uEnableWindow(GetDlgItem(IDC_PREVIEW_LIST), is_enabled);

	if (change_focus) {
		if (tag_writer->will_modify)
			::SetFocus(GetDlgItem(IDC_WRITE_TAGS_BUTTON));
		else
			::SetFocus(GetDlgItem(IDCANCEL));
	}
}

void CPreviewTagsDialog::destroy_all() {
	if (!multi_mode) {
		if (g_discogs->track_matching_dialog)
			g_discogs->track_matching_dialog->destroy_all();
	}
	MyCDialogImpl<CPreviewTagsDialog>::destroy();
}

void CPreviewTagsDialog::go_back() {
	PFC_ASSERT(!multi_mode);
	destroy();
	if (g_discogs->track_matching_dialog) {
		g_discogs->track_matching_dialog->show();
	}
}

void CPreviewTagsDialog::reset_default_columns(HWND wndlist, bool breset) {

	cfg_prev_ColMap::iterator it;

	for (int i = 0; i < COL_STAT_COLS; i++) {
		ListView_DeleteColumn(wndlist, COL_STAT_POS);
	}

	update_sorted_icol_map(breset);

	for (unsigned int i = 0; i < vec_icol_subitems.size(); i++)
	{
		cfg_prev_col walk_cfg = cfg_lv.colmap.at(vec_icol_subitems[i].first);
		if (breset) {
			if (ListView_GetColumnCount(wndlist) < COL_STAT_POS) {
				if (walk_cfg.icol < COL_STAT_POS && walk_cfg.enabled) {
					//insert column
					int icol = listview_helper::fr_insert_column(wndlist, walk_cfg.icol,
						walk_cfg.name, (unsigned int)walk_cfg.width, LVCFMT_LEFT);
					cfg_lv.colmap.at(vec_icol_subitems[i].first).enabled = true;
				}
			}
		}
		else {
			if (walk_cfg.icol >= COL_STAT_POS && walk_cfg.enabled) {
				//insert column
				int icol = listview_helper::fr_insert_column(wndlist, walk_cfg.icol,
				walk_cfg.name, (unsigned int)walk_cfg.width, LVCFMT_LEFT);
				cfg_lv.colmap.at(vec_icol_subitems[i].first).enabled = true;
			}
		}
	}
}

void CPreviewTagsDialog::update_sorted_icol_map(bool reset) {

	cfg_prev_ColMap& colvals = cfg_lv.colmap;
	cfg_prev_ColMap::iterator it;

	vec_icol_subitems.clear();
	for (it = colvals.begin(); it != colvals.end(); it++)
	{
		bool bpush = false;
		if (reset) {
			if (it->second.default) {
				it->second.enabled = true;
				vec_icol_subitems.push_back(std::make_pair(it->first, it->second.icol));
			}
			else {
				it->second.enabled = false;
			}
		}
		else {
			if (it->second.enabled) {
				vec_icol_subitems.push_back(std::make_pair(it->first, it->second.icol));
			}
			else {
				//
			}
		}
	}

	std::sort(vec_icol_subitems.begin(), vec_icol_subitems.end(), sortByVal);

}

LRESULT CPreviewTagsDialog::OnCheckPreviewShowStats(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	
	cfg_preview_dialog_show_stats = !cfg_preview_dialog_show_stats;

	for (int i = 0; i < COL_STAT_COLS; i++) {
		cfg_prev_col& cfgColClicked = cfg_lv.colmap.at(COL_STAT_POS + i);
		cfgColClicked.enabled = !cfgColClicked.enabled;
	}

	if (!cfg_preview_dialog_show_stats) {
		reset_default_columns(tag_results_list, true);
	}
	else {
		
		reset_default_columns(tag_results_list, false);

		int def_statswidth = 0;
		for (int cc = COL_STAT_POS; cc < COL_STAT_POS + COL_STAT_COLS; cc++) {
			cfg_lv.colmap.at(cc).enabled = true;
			ListView_SetColumnWidth(tag_results_list, cc, cfg_lv.colmap.at(cc).width);
			def_statswidth += (int)cfg_lv.colmap.at(cc).width;
		}
		
		int newicol = ListView_GetColumnCount(tag_results_list);
		int def_fieldwidth = ListView_GetColumnWidth(tag_results_list, 0);
		int def_valuewidth = ListView_GetColumnWidth(tag_results_list, 1);
		int def_field_value_width = def_fieldwidth + def_valuewidth;

		CRect reclist;
		::GetWindowRect(tag_results_list, reclist);

		int framewidth = reclist.Width();
		int delta = framewidth - def_fieldwidth - def_statswidth - 40;

		if (delta < 0) {
			ListView_SetColumnWidth(tag_results_list, 1, delta);
			ListView_SetColumnWidth(tag_results_list, 0, def_fieldwidth);
		}
		else {
			ListView_SetColumnWidth(tag_results_list, 1, delta);
		}
		for (int cc = COL_STAT_POS; cc < COL_STAT_POS + COL_STAT_COLS; cc++) {
			ListView_SetColumnWidth(tag_results_list, cc, cfg_lv.colmap.at(cc).width);
		}

		update_sorted_icol_map(false /*sort without reset*/);
	}

	ListView_DeleteAllItems(tag_results_list);
	insert_tag_results(true);

	ListView_RedrawItems(tag_results_list, 0, ListView_GetGroupCount(tag_results_list));
	return FALSE;
}
