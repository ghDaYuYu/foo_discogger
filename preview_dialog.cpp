#include "stdafx.h"
#include <CommCtrl.h>

#include <GdiPlus.h>
#pragma comment(lib, "gdiplus.lib")

#include "string_encoded_array.h"
#include "tasks.h"
#include "tag_mappings_dialog.h"
#include "track_matching_dialog.h"
#include "preview_dialog.h"

static const GUID guid_cfg_window_placement_preview_dlg = { 0xcb1debbd, 0x1b0a, 0x4046, { 0x8c, 0x6f, 0x74, 0xee, 0x9d, 0x74, 0x7f, 0x83 } };
static cfg_window_placement cfg_window_placement_preview_dlg(guid_cfg_window_placement_preview_dlg);

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

	bool local_replace_ANV = IsDlgButtonChecked(IDC_REPLACE_ANV_CHECK);
	if (CONF.replace_ANVs != conf.replace_ANVs &&
		conf.replace_ANVs != local_replace_ANV) {
		bres |= true;
	}

	//get current dimensions
	RECT rcDlg = {};
	GetWindowRect(&rcDlg);
	int dlgcx = rcDlg.right;
	int dlgcy = rcDlg.bottom;
	int dlgwidth = rcDlg.right - rcDlg.left;
	int dlgheight = rcDlg.bottom - rcDlg.top;

	int colwidth1 = ListView_GetColumnWidth(GetDlgItem(IDC_PREVIEW_LIST), 0);
	int colwidth2 = ListView_GetColumnWidth(GetDlgItem(IDC_PREVIEW_LIST), 1);

	//preview mode
	if (int preview_mode = get_preview_mode(); preview_mode != conf.preview_mode) {
		conf.preview_mode = preview_mode;
		bres |= true;
	}
	
	//columns
	if (colwidth1 != conf.preview_tags_dialog_col1_width || colwidth2 != conf.preview_tags_dialog_col2_width) {
		conf.preview_tags_dialog_col1_width = colwidth1;
		conf.preview_tags_dialog_col2_width = colwidth2;
		bres |= true;
	}
	//dlg position
	if (rcDlg.left != LOWORD(conf.preview_tags_dialog_position) || rcDlg.top != HIWORD(conf.preview_tags_dialog_position)) {
		conf.preview_tags_dialog_position = MAKELPARAM(rcDlg.left, rcDlg.top);
		bres = bres || true;
	}
	//dlg size
	if (dlgwidth != LOWORD(conf.preview_tags_dialog_size) || dlgheight != HIWORD(conf.preview_tags_dialog_size)) {
		conf.preview_tags_dialog_size = MAKELPARAM(dlgwidth, dlgheight);
		bres = bres || true;
	}
	//show stats
	const bool benabled = uButton_GetCheck(m_hWnd, IDC_CHECK_PREV_DLG_SHOW_STATS);

	if (benabled) {
		int colw = ListView_GetColumnWidth(GetDlgItem(IDC_PREVIEW_LIST), 2);
		int colu = ListView_GetColumnWidth(GetDlgItem(IDC_PREVIEW_LIST), 3);
		int cols = ListView_GetColumnWidth(GetDlgItem(IDC_PREVIEW_LIST), 4);
		int cole = ListView_GetColumnWidth(GetDlgItem(IDC_PREVIEW_LIST), 5);

		if (conf.preview_tags_dialog_w_width != colw ||
			conf.preview_tags_dialog_u_width != colu ||
			conf.preview_tags_dialog_s_width != cols ||
			conf.preview_tags_dialog_e_width != cole) {

			conf.preview_tags_dialog_w_width = colw;
			conf.preview_tags_dialog_u_width = colu;
			conf.preview_tags_dialog_s_width = cols;
			conf.preview_tags_dialog_e_width = cole;

			bres |= true;
		}
	}

	if (benabled != conf.edit_tags_dlg_show_tm_stats) {
		conf.edit_tags_dlg_show_tm_stats = benabled;
		bres |= true;
	}
	//skip artwork
	if (CONF.album_art_skip_default_cust != conf.album_art_skip_default_cust) {
		bres |= true;
	}
	//attach edit mappings panel
	size_t attach_flagged = IsDlgButtonChecked(IDC_EDIT_TAGS_DIALOG_ATTACH_FLAG) ? (flag_tagmap_dlg_attached) : 0;
	size_t open_flagged = g_discogs->tag_mappings_dialog ? flag_tagmap_dlg_is_open : 0;
	if ((CONF.edit_tags_dialog_flags & (flag_tagmap_dlg_attached | flag_tagmap_dlg_is_open)) != (attach_flagged | open_flagged)) {
		conf.edit_tags_dialog_flags &= ~(flag_tagmap_dlg_attached | flag_tagmap_dlg_is_open);
		conf.edit_tags_dialog_flags |= (attach_flagged | open_flagged);
		bres |= true;
	}

	return bres;
}

CPreviewTagsDialog::~CPreviewTagsDialog() {
	DeleteObject(m_rec_icon);
	g_discogs->preview_tags_dialog = nullptr;
}

LRESULT CPreviewTagsDialog::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	SetIcon(g_discogs->icon);
	conf = CONF;
	
	defaultCfgCols(cfg_lv);
	cfg_lv.colmap.at(2).width = static_cast<float>(conf.preview_tags_dialog_w_width);
	cfg_lv.colmap.at(3).width = static_cast<float>(conf.preview_tags_dialog_u_width);
	cfg_lv.colmap.at(4).width = static_cast<float>(conf.preview_tags_dialog_s_width);
	cfg_lv.colmap.at(5).width = static_cast<float>(conf.preview_tags_dialog_e_width);

	tag_results_list = GetDlgItem(IDC_PREVIEW_LIST);

	ListView_SetExtendedListViewStyleEx(tag_results_list,
		LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_INFOTIP | LVS_EX_LABELTIP, LVS_EX_FULLROWSELECT |
		(conf.list_style != 1 ? LVS_EX_GRIDLINES : 0) | LVS_EX_INFOTIP | LVS_EX_LABELTIP);

	reset_default_columns(tag_results_list, true);
	set_preview_mode(conf.preview_mode);

	cfg_window_placement_preview_dlg.on_window_creation(m_hWnd, true);

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

	initialize();

	//add rec icon to write tags button
	HWND write_btn = GetDlgItem(IDC_WRITE_TAGS_BUTTON);
	::SendMessageW(write_btn, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)m_rec_icon);

	HWND hwndSmallPreview = GetDlgItem(IDC_ALBUM_ART);
	LPARAM stl = ::uGetWindowLong(hwndSmallPreview, GWL_STYLE);
	stl |= SS_GRAYFRAME;
	::uSetWindowLong(hwndSmallPreview, GWL_STYLE, stl/*WS_CHILD | WS_VISIBLE | SS_GRAYFRAME*/);

	DlgResize_Init(mygripp.enabled, true);
	load_size();
	show();

	if (conf.edit_tags_dlg_show_tm_stats) {

		uButton_SetCheck(m_hWnd, IDC_CHECK_PREV_DLG_SHOW_STATS, true);
		BOOL bdummy = false;
		OnCheckPreviewShowStats(0, 0, NULL, bdummy);
	}

	if (conf.edit_tags_dialog_flags & (flag_tagmap_dlg_attached)) {
		uButton_SetCheck(m_hWnd, IDC_EDIT_TAGS_DIALOG_ATTACH_FLAG, true);
		if (conf.edit_tags_dialog_flags & (flag_tagmap_dlg_is_open)) {
			BOOL bdummy = false;
			OnEditTagMappings(0, 0, NULL, bdummy);
		}
	}

	// todo: unify with enable(bool, bool)
	// note: might be called from generate_tags_task::on_success(HWND p_wnd)

	bool managed_artwork = !(CONFARTWORK == uartwork(CONF));
	LPARAM lpskip = conf.album_art_skip_default_cust;

	bool skip_artwork = ((LOWORD(lpskip) & 8) == 8 || (HIWORD(lpskip) & 8) == 8);
	uButton_SetCheck(m_hWnd, IDC_CHECK_PREV_DLG_SKIP_ARTWORK, managed_artwork || skip_artwork);

	bool write_button_enabled = check_write_tags_status();
	::EnableWindow(GetDlgItem(IDC_WRITE_TAGS_BUTTON), write_button_enabled);

	HWND hwndDefaultControl;
	if (write_button_enabled)
		hwndDefaultControl = GetDlgItem(IDC_WRITE_TAGS_BUTTON);
	else
		hwndDefaultControl = GetDlgItem(IDCANCEL);
	SendMessage(m_hWnd, WM_NEXTDLGCTL, (WPARAM)hwndDefaultControl, TRUE);

	return FALSE;
}

bool CPreviewTagsDialog::check_write_tags_status() {
	bool bres;
	bres = m_tag_writer->will_modify;
	bres |= (CONF.save_album_art || CONF.save_artist_art || CONF.embed_album_art || CONF.embed_artist_art);
	bres |= !(CONFARTWORK == uartwork(CONF));
	return bres;
}

bool CPreviewTagsDialog::initialize() {
	if (!m_tag_writer) {
		return get_next_tag_writer();
	}

	bool release_has_anv = m_tag_writer->release->has_anv();

	CheckDlgButton(IDC_REPLACE_ANV_CHECK, release_has_anv && conf.replace_ANVs);
	CheckDlgButton(IDC_CHECK_PREV_DLG_DIFF_TRACKS, cfg_preview_dialog_track_map);

	// Album art
	if (m_tag_writer->release->small_art.get_size() > 0 && (conf.save_album_art || conf.embed_album_art)) {

		pfc::string8 cache_path_small = Offline::get_thumbnail_cache_path_filenames(
			m_tag_writer->release->id, art_src::alb, LVSIL_NORMAL, 0)[0];

		extract_native_path(cache_path_small, cache_path_small);
		pfc::stringcvt::string_wide_from_ansi tempf(cache_path_small);
		
		Bitmap bm(tempf);
		HBITMAP hBm;
		if (bm.GetHBITMAP(Color::Black, &hBm) == Ok) {
			uSendDlgItemMessage(IDC_ALBUM_ART, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBm);
			DeleteObject(hBm);
		}
		else {
			log_msg("GdiPlus error (GetHBITMAP small album preview)");
		}
	}
	insert_tag_results(true);

	enable(true, true);

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
		v_stats.clear();
		compute_stats(m_tag_writer->tag_results);
	}

	const size_t count = m_tag_writer->tag_results.get_count();
	for (unsigned int i = 0; i < count; i++) {
		insert_tag_result(i, m_tag_writer->tag_results[i]);
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
	pfc::array_t<string_encoded_array> value = GetPreviewValue(m_tag_writer->tag_results[item]);
	if (item < m_tag_writer->tag_results.get_count() && subItem == 1 && value.get_size() > 1) {
		return;
	}
	else {
		InPlaceEdit::CTableEditHelperV2_ListView::TableEdit_GetField(item, subItem, out, lineCount);
	}
}

void CPreviewTagsDialog::TableEdit_SetField(size_t item, size_t subItem, const char * value) {
	InPlaceEdit::CTableEditHelperV2_ListView::TableEdit_SetField(item, subItem, value);
	if (item < m_tag_writer->tag_results.get_count()) {
		if (subItem == 1) {
			m_tag_writer->tag_results[item]->value.force_reset();
			m_tag_writer->tag_results[item]->value.append_single(string_encoded_array(value));
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
	service_ptr_t<generate_tags_task> task = new service_impl_t<generate_tags_task>(this, m_tag_writer, use_update_tags);
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
LRESULT CPreviewTagsDialog::OnCheckSkipArtwork(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	
	bool state = IsDlgButtonChecked(IDC_CHECK_PREV_DLG_SKIP_ARTWORK);
	LPARAM param = conf.album_art_skip_default_cust;
	int lo = LOWORD(param);
	int hi = HIWORD(param);

	if (state) lo |= 8;
	else lo &= ~8;
	
	conf.album_art_skip_default_cust = MAKELPARAM(lo, hi);

	return FALSE;
}

LRESULT CPreviewTagsDialog::OnCheckAttachMappingPanel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	bool state = IsDlgButtonChecked(IDC_EDIT_TAGS_DIALOG_ATTACH_FLAG);
	conf.edit_tags_dialog_flags = state ?
		conf.edit_tags_dialog_flags | (flag_tagmap_dlg_attached)
		: conf.edit_tags_dialog_flags & ~(flag_tagmap_dlg_attached);

	return FALSE;
}

LRESULT CPreviewTagsDialog::OnEditTagMappings(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (!g_discogs->tag_mappings_dialog) {
		g_discogs->tag_mappings_dialog = fb2k::newDialog<CTagMappingDialog>(core_api::get_main_window(), IDAPPLY);
	}
	else
	{
		if (wID == IDC_EDIT_TAG_MAPPINGS_BUTTON) {
			::SetFocus(g_discogs->tag_mappings_dialog->m_hWnd);
			::uPostMessage(g_discogs->tag_mappings_dialog->m_hWnd, WM_NEXTDLGCTL, (WPARAM)(HWND)GetDlgItem(IDAPPLY), TRUE);		
		}
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
		service_ptr_t<write_tags_task> task = new service_impl_t<write_tags_task>(m_tag_writer);
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
		m_tag_writer = tag_writers[tw_index++];
		if (m_tag_writer->skip || !m_tag_writer->will_modify) {
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
	if (g_discogs->tag_mappings_dialog &&
		(conf.edit_tags_dialog_flags & (flag_tagmap_dlg_attached | flag_tagmap_dlg_is_open)) == (flag_tagmap_dlg_attached | flag_tagmap_dlg_is_open)) {
		g_discogs->tag_mappings_dialog->destroy();
	}
	cfg_window_placement_preview_dlg.on_window_destruction(m_hWnd);
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
	m_tag_writer->skip = true;
	get_next_tag_writer();
	return TRUE;
}

LRESULT CPreviewTagsDialog::OnListClick(LPNMHDR lParam) {
	NMITEMACTIVATE* info = reinterpret_cast<NMITEMACTIVATE*>(lParam);
	if (info->iItem >= 0) {
		if (get_preview_mode() == PREVIEW_NORMAL && TableEdit_IsColumnEditable(info->iSubItem)) {
			//enable edits
			const tag_result_ptr &result = m_tag_writer->tag_results[info->iItem];
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
	erase(m_tag_writer->tag_results, index);
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
	if (m_tag_writer->tag_results[item]->result_approved)
		TableEdit_Start(wParam, lParam);
	return FALSE;
}


#define DISABLED_RGB	RGB(150, 150, 150)
#define CHANGE_NOT_APPROVED_RGB	RGB(150, 100, 100)

void CPreviewTagsDialog::compute_stats(tag_results_list_type tag_results) {
	reset_tag_result_stats();
	compute_stats_track_map(tag_results);
}

void CPreviewTagsDialog::compute_stats_track_map(tag_results_list_type tag_results) {

	int createdtags = 0;
	int updatedtags = 0;

	v_stats.clear();

	const size_t tagcount = m_tag_writer->tag_results.get_count();

	for (unsigned int walk_tag = 0; walk_tag < tagcount; walk_tag++)
	{

		preview_stats stats;

		const tag_result_ptr res = m_tag_writer->tag_results[walk_tag];
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
		ctrl.EnableWindow(write_tag_button_enabled());
		//ctrl.EnableWindow(tag_writer->will_modify);
		ctrl = uGetDlgItem(IDC_REPLACE_ANV_CHECK);
		ctrl.EnableWindow(bresults && tag_writer->release->has_anv());
		ctrl = uGetDlgItem(IDC_CHECK_PREV_DLG_DIFF_TRACKS);
		ctrl.EnableWindow(cfg_preview_dialog_track_map);
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
		return CDRF_DODEFAULT;
	}
	return CDRF_DODEFAULT;
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
		if (g_discogs->track_matching_dialog) {
			CTrackMatchingDialog* dlg = reinterpret_cast<CTrackMatchingDialog*>(g_discogs->track_matching_dialog);
			dlg->destroy_all();
		}
	}
	destroy();
}

void CPreviewTagsDialog::go_back() {
	PFC_ASSERT(!multi_mode);
	destroy();
	if (g_discogs->track_matching_dialog) {
		CTrackMatchingDialog* dlg = reinterpret_cast<CTrackMatchingDialog*>(g_discogs->track_matching_dialog);
		dlg->show();
	}
}

void CPreviewTagsDialog::reset_default_columns(HWND wndlist, bool breset) {

	cfg_prev_ColMap::iterator it;

	float removed_stats_width = 0.0;
	for (int i = 0; i < COL_STAT_COLS; i++) {
		removed_stats_width += ListView_GetColumnWidth(wndlist, COL_STAT_POS);
		ListView_DeleteColumn(wndlist, COL_STAT_POS);
	}

	update_sorted_icol_map(breset);

	for (unsigned int i = 0; i < vec_icol_subitems.size(); i++)
	{
		cfg_prev_col walk_cfg = cfg_lv.colmap.at(vec_icol_subitems[i].first);
		if (breset) {
			if (ListView_GetColumnCount(wndlist) < COL_STAT_POS) {
				if (walk_cfg.icol < COL_STAT_POS && walk_cfg.enabled) {				
					int icol = listview_helper::fr_insert_column(wndlist, walk_cfg.icol,
						walk_cfg.name, (unsigned int)walk_cfg.width, LVCFMT_LEFT);
					cfg_lv.colmap.at(vec_icol_subitems[i].first).enabled = true;
				}
			}
			//add removed stat cols width to the last column
			if (walk_cfg.icol == COL_STAT_POS - 1) {
				float width = ListView_GetColumnWidth(wndlist, walk_cfg.icol);
				width += removed_stats_width;
				ListView_SetColumnWidth(wndlist, COL_STAT_POS - 1, (unsigned int)width);
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
