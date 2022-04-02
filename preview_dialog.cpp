#include "stdafx.h"

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

#define PREVIEW_NORMAL		1
#define PREVIEW_DIFFERENCE	2
#define PREVIEW_ORIGINAL	3
#define PREVIEW_DEBUG		4

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
	{1, 1, 0,"Tag Name", "",	20.0f, 0x0000, 0, true, true, false, false},
	{2, 2, 1,"Value(s)", "",	40.0f, 0x0000, 0, true, true, false, false},
	{3, 3, 2,"Write", "",		40.0f, 0x0000, 0, true, false, false, false},
	{4, 4, 3,"Update", "",		40.0f, 0x0000, 0, true, false, false, false},
	{5, 5, 4,"Skip W/U", "",	40.0f, 0x0000, 0, true,false, false, false},
	{6, 6, 5,"Equal", "",		40.0f, 0x0000, 0, true, false, false, false},
	{7, 7, 6,"---x", "",		20.0f, 0x0000, 0, false, false, false, false},
	{8, 8, 7,"---y", "",		50.0f, 0x0000, 0, false, false, false, false},
	{100, 10, 0,"---z", "",		200.0f, 0x0000, 0, false, false, false, false},
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

	//check globals
	//replace_ANV was synced on click
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
	int current_preview_mode = get_preview_mode();
	if (current_preview_mode != conf.preview_mode) {
		conf.preview_mode = current_preview_mode;
		bres |= true;
	}
	
	//columns
	if (colwidth1 != conf.preview_tags_dialog_col1_width || colwidth2 != conf.preview_tags_dialog_col2_width) {
		conf.preview_tags_dialog_col1_width = colwidth1;
		conf.preview_tags_dialog_col2_width = colwidth2;
		bres |= true;
	}

	//show stats
	const bool bstats_checked = uButton_GetCheck(m_hWnd, IDC_CHECK_PREV_DLG_SHOW_STATS);

	if (bstats_checked) {
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

	if (conf.edit_tags_dlg_show_tm_stats != bstats_checked) {
		conf.edit_tags_dlg_show_tm_stats = bstats_checked;
		bres |= true;
	}

	//skip artwork
	if (CONF.album_art_skip_default_cust != conf.album_art_skip_default_cust) {

		bres |= true;
	}

	size_t attach_flagged = (IsDlgButtonChecked(IDC_EDIT_TAGS_DIALOG_ATTACH_FLAG) == BST_CHECKED) ? FLG_TAGMAP_DLG_ATTACHED : 0;	
	size_t open_flagged = g_discogs->tag_mappings_dialog ? FLG_TAGMAP_DLG_OPENED : 0;

	conf.edit_tags_dlg_flags &= ~(FLG_TAGMAP_DLG_ATTACHED | FLG_TAGMAP_DLG_OPENED);
	conf.edit_tags_dlg_flags |= (attach_flagged | open_flagged);
	bres |= (CONF.edit_tags_dlg_flags != conf.edit_tags_dlg_flags);

	return bres;
}

CPreviewTagsDialog::~CPreviewTagsDialog() {

	DeleteObject(m_rec_icon);
	DeleteObject(m_preview_bitmap);
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

	::ShowWindow(uGetDlgItem(IDC_EDIT_TAG_MAPPINGS_BUTTON), true);
	::ShowWindow(uGetDlgItem(IDC_BACK_BUTTON), true);
	::ShowWindow(uGetDlgItem(IDC_SKIP_BUTTON), false);
	::ShowWindow(uGetDlgItem(IDCANCEL), true);

	initialize();

	//add rec icon to write tags button
	HWND write_btn = GetDlgItem(IDC_WRITE_TAGS_BUTTON);
	::SendMessageW(write_btn, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)m_rec_icon);

	HWND hwndSmallPreview = GetDlgItem(IDC_ALBUM_ART);
	LPARAM stl = ::uGetWindowLong(hwndSmallPreview, GWL_STYLE);
	stl |= SS_GRAYFRAME;
	::uSetWindowLong(hwndSmallPreview, GWL_STYLE, stl);

	DlgResize_Init(mygripp.enabled, true);
	cfg_window_placement_preview_dlg.on_window_creation(m_hWnd, true);

	load_size();

	if (conf.edit_tags_dlg_show_tm_stats) {
		//todo: stats init revision
		uButton_SetCheck(m_hWnd, IDC_CHECK_PREV_DLG_SHOW_STATS, true);
		BOOL bdummy = false;
		OnCheckPreviewShowStats(0, 0, NULL, bdummy);
	}

	if (conf.edit_tags_dlg_flags & FLG_TAGMAP_DLG_ATTACHED) {
		uButton_SetCheck(m_hWnd, IDC_EDIT_TAGS_DIALOG_ATTACH_FLAG, true);
		if (conf.edit_tags_dlg_flags & FLG_TAGMAP_DLG_OPENED) {
			BOOL bdummy = false;
			OnEditTagMappings(0, 0, NULL, bdummy);
		}
	}

	// todo: unify with enable(bool, bool)
	// note: might be called from generate_tags_task::on_success(HWND p_wnd)

	bool managed_artwork = !(CONFARTWORK == uartwork(CONF));
	LPARAM lpskip = conf.album_art_skip_default_cust;

	bool lo_skip_global_defs = (LOWORD(lpskip) & ART_CUST_SKIP_DEF_FLAG) == ART_CUST_SKIP_DEF_FLAG;
	bool hi_skip_global_defs = (HIWORD(lpskip) & ART_CUST_SKIP_DEF_FLAG) == ART_CUST_SKIP_DEF_FLAG;
	
	bool skip_artwork = lo_skip_global_defs | hi_skip_global_defs;

	uButton_SetCheck(m_hWnd, IDC_CHECK_PREV_DLG_SKIP_ARTWORK, managed_artwork || skip_artwork);

	bool write_button_enabled = check_write_tags_status();
	::EnableWindow(GetDlgItem(IDC_WRITE_TAGS_BUTTON), write_button_enabled);

	HWND hwndDefaultControl;
	if (write_button_enabled)
		hwndDefaultControl = GetDlgItem(IDC_WRITE_TAGS_BUTTON);
	else
		hwndDefaultControl = GetDlgItem(IDCANCEL);
	
	SendMessage(m_hWnd, WM_NEXTDLGCTL, (WPARAM)hwndDefaultControl, TRUE);

	show();
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
		return false;
	}

	bool release_has_anv = m_tag_writer->release->has_anv();

	CheckDlgButton(IDC_REPLACE_ANV_CHECK, release_has_anv && conf.replace_ANVs);
	CheckDlgButton(IDC_CHECK_PREV_DLG_DIFF_TRACKS, cfg_preview_dialog_track_map);

	//Done on CDDS_PREPAINT
	//::EnableWindow(uGetDlgItem(IDC_REPLACE_ANV_CHECK), release_has_anv);

	// Album art
	if (m_tag_writer->release->small_art.get_size() > 0 && (conf.save_album_art || conf.embed_album_art)) {

		pfc::string8 cache_path_small = Offline::get_thumbnail_cache_path_filenames(
			m_tag_writer->release->id, art_src::alb, LVSIL_NORMAL, 0)[0];

		extract_native_path(cache_path_small, cache_path_small);
		pfc::stringcvt::string_wide_from_ansi wtemp(cache_path_small);
		
		Bitmap bm(wtemp);

		if (bm.GetHBITMAP(Color::Black, &m_preview_bitmap) == Ok) {
			uSendDlgItemMessage(IDC_ALBUM_ART, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)m_preview_bitmap);
		}
		else {
			log_msg("GdiPlus error (GetHBITMAP small album preview)");
		}
	}
	insert_tag_results(true);

	enable(true, true);
	//::EnableWindow(GetDlgItem(IDC_WRITE_TAGS_BUTTON), tag_writer->changed);

	return true;
}
void CPreviewTagsDialog::reset_tag_result_stats() {
	totalwrites = 0;
	totalupdates = 0;
}

void CPreviewTagsDialog::insert_tag_results(bool computestat) {
	ListView_DeleteAllItems(tag_results_list);
	if (computestat) {
		v_stats.clear(); // .reset();
		compute_stats(m_tag_writer->tag_results);
	}

	const size_t count = m_tag_writer->tag_results.get_count();
	for (unsigned int i = 0; i < count; i++) {
		insert_tag_result(i, m_tag_writer->tag_results[i]);
	}
}

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
		listview_helper::set_item_text(tag_results_list, pos, 2, v_stats.size() > 0 ? std::to_string(v_stats.at(pos).totalwrites).c_str() : "n/a");
		listview_helper::set_item_text(tag_results_list, pos, 3, v_stats.size() > 0 ? std::to_string(v_stats.at(pos).totalupdates).c_str() : "n/a");
		listview_helper::set_item_text(tag_results_list, pos, 4, v_stats.size() > 0 ? std::to_string(v_stats.at(pos).totalskips).c_str() : "n/a");
		listview_helper::set_item_text(tag_results_list, pos, 5, v_stats.size() > 0 ? std::to_string(v_stats.at(pos).totalequal).c_str() : "n/a");
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

LRESULT CPreviewTagsDialog::OnCheckSkipArtwork(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	LPARAM param = conf.album_art_skip_default_cust;
	unsigned lo = LOWORD(param);
	unsigned hi = HIWORD(param);

	if (IsDlgButtonChecked(IDC_CHECK_PREV_DLG_SKIP_ARTWORK))
		lo |= ART_CUST_SKIP_DEF_FLAG;
	else
		lo &= ~ART_CUST_SKIP_DEF_FLAG;
	
	conf.album_art_skip_default_cust = MAKELPARAM(lo, hi);

	return FALSE;
}

LRESULT CPreviewTagsDialog::OnCheckAttachMappingPanel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	bool state = IsDlgButtonChecked(IDC_EDIT_TAGS_DIALOG_ATTACH_FLAG);
	conf.edit_tags_dlg_flags = state ?
		conf.edit_tags_dlg_flags | (FLG_TAGMAP_DLG_ATTACHED)
		: conf.edit_tags_dlg_flags & ~(FLG_TAGMAP_DLG_ATTACHED);

	return FALSE;
}

LRESULT CPreviewTagsDialog::OnEditTagMappings(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (!g_discogs->tag_mappings_dialog) {
		g_discogs->tag_mappings_dialog = fb2k::newDialog<CTagMappingDialog>(core_api::get_main_window(), IDC_APPLY);
	}
	else
	{
		if (wID == IDC_EDIT_TAG_MAPPINGS_BUTTON) {
			::SetFocus(g_discogs->tag_mappings_dialog->m_hWnd);
			::uPostMessage(g_discogs->tag_mappings_dialog->m_hWnd, WM_NEXTDLGCTL, (WPARAM)(HWND)GetDlgItem(IDC_APPLY), TRUE);
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

		service_ptr_t<write_tags_task> task = new service_impl_t<write_tags_task>(m_tag_writer);
		task->start();
		destroy_all();

	return TRUE;
}

void CPreviewTagsDialog::cb_generate_tags() {
	generating_tags = false;
	insert_tag_results(true);
	enable(true, true);
}

LRESULT CPreviewTagsDialog::OnButtonBack(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
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

void CPreviewTagsDialog::pushcfg() {
	if (build_current_cfg()) {
		CONF.save(CConf::cfgFilter::PREVIEW, conf);
		CONF.load();
	}
}

LRESULT CPreviewTagsDialog::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	pushcfg();
	if (g_discogs->tag_mappings_dialog &&
		(conf.edit_tags_dlg_flags & (FLG_TAGMAP_DLG_ATTACHED | FLG_TAGMAP_DLG_OPENED)) == (FLG_TAGMAP_DLG_ATTACHED | FLG_TAGMAP_DLG_OPENED)) {
		g_discogs->tag_mappings_dialog->destroy();
	}
	cfg_window_placement_preview_dlg.on_window_destruction(m_hWnd);
	return 0;
}

LRESULT CPreviewTagsDialog::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	destroy_all();
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

		//TODO: remove for short statistics
		//if (!res->result_approved) {
		//	v_stats.emplace_back(stats);
		//	continue;
		//}

		const tag_mapping_entry* entry = res->tag_entry;

		int subwrites = 0, subupdates = 0, subskips = 0, wereequal = 0;

		const size_t tk_count = res->r_approved.get_count();

//#ifdef DEBUG
//		if (stricmp_utf8(entry->tag_name, "DISCOGS_ARTISTS_MEMBERS") == 0) {
//			int dbug = 0;
//		}
//#endif	

		preview_stats before_tag_loop_stats = stats;

		for (size_t walk_tk = 0; walk_tk < tk_count; walk_tk++) {

			const bool r_approved = res->r_approved[walk_tk];
			string_encoded_array* oldvalue;
			string_encoded_array* value;

//#ifdef DEBUG
//			int oldvalue_count = res->old_value.get_count();
//			int value_count = res->m_value.get_count();
//			int oldvalue_size = res->old_value.get_size();
//			int value_size = res->m_value.get_size();
//#endif

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
		//revision: race condition on tag_writer->tag_results generating tags?
		return CDRF_DODEFAULT;
	}
	//revision: race condition on tag_writer->tag_results if user changed tag mapping ?!
	LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;
	int pos = (int)lplvcd->nmcd.dwItemSpec;
	int sub_item;

	//int dbug = tag_writer->tag_results.size();

	bool bresults = m_tag_writer->tag_results.size() > 0;
	const tag_mapping_entry* entry = bresults ?
		m_tag_writer->tag_results[pos]->tag_entry: nullptr;

	switch (lplvcd->nmcd.dwDrawStage) {

	case CDDS_PREPAINT: {
		//TODO: create EnableEx(true | false | noresult)
		//TODO: remove all EnableWindow calls from PREPAINT
		auto ctrl = uGetDlgItem(IDC_WRITE_TAGS_BUTTON);
		ctrl.EnableWindow(check_write_tags_status());
		//ctrl.EnableWindow(tag_writer->will_modify);
		ctrl = uGetDlgItem(IDC_REPLACE_ANV_CHECK);
		ctrl.EnableWindow(bresults && m_tag_writer->release->has_anv());
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
		//display_tag_result_stats();
		return CDRF_NOTIFYITEMDRAW;
	}

	case CDDS_ITEMPREPAINT:
		return CDRF_NOTIFYSUBITEMDRAW;

	case CDDS_SUBITEM | CDDS_ITEMPREPAINT: {
		sub_item = lplvcd->iSubItem;
		if (entry && (sub_item == 0 || sub_item == 1)) {
			bool bresview = get_preview_mode() == PREVIEW_NORMAL;
			//todo: quick fix, disable colored lines for now...
			bool bchgdiscarded = false && !m_tag_writer->tag_results[pos]->result_approved &&
				m_tag_writer->tag_results[pos]->changed;

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
		//return CDRF_DODEFAULT;
	}
	case CDDS_POSTPAINT:

		return CDRF_DODEFAULT;
	}
	return CDRF_DODEFAULT;
}

void CPreviewTagsDialog::enable(bool is_enabled, bool change_focus) {
	::uEnableWindow(GetDlgItem(IDC_WRITE_TAGS_BUTTON), !is_enabled ? FALSE : m_tag_writer->will_modify/*is_enabled*/);
	::uEnableWindow(GetDlgItem(IDCANCEL), is_enabled);
	::uEnableWindow(GetDlgItem(IDC_BACK_BUTTON), is_enabled);
	::uEnableWindow(GetDlgItem(IDC_EDIT_TAG_MAPPINGS_BUTTON), is_enabled);
	::uEnableWindow(GetDlgItem(IDC_REPLACE_ANV_CHECK), is_enabled);
	::uEnableWindow(GetDlgItem(IDC_CHECK_PREV_DLG_DIFF_TRACKS), is_enabled);
	::uEnableWindow(GetDlgItem(IDC_PREVIEW_LIST), is_enabled);
	
	/*
	if (change_focus) {
        if (tag_writer->will_modify)
    		::SetFocus(GetDlgItem(IDC_WRITE_TAGS_BUTTON));
    	else
    		::SetFocus(GetDlgItem(IDCANCEL));
    }
    */
}

void CPreviewTagsDialog::destroy_all() {

	if (g_discogs->track_matching_dialog) {
		CTrackMatchingDialog* dlg = reinterpret_cast<CTrackMatchingDialog*>(g_discogs->track_matching_dialog);
		dlg->destroy_all();
	}
	destroy();
}

void CPreviewTagsDialog::go_back() {

	destroy();
	if (g_discogs->track_matching_dialog) {
		CTrackMatchingDialog* dlg = reinterpret_cast<CTrackMatchingDialog*>(g_discogs->track_matching_dialog);
		dlg->show();
	}
}

void CPreviewTagsDialog::reset_default_columns(HWND wndlist, bool breset) {

	cfg_prev_ColMap::iterator it;

	//delete stat rows
	size_t removed_stats_width = 0;
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
					//insert column					
					int icol = listview_helper::fr_insert_column(wndlist, walk_cfg.icol,
						walk_cfg.name, (unsigned int)walk_cfg.width, LVCFMT_LEFT);
					cfg_lv.colmap.at(vec_icol_subitems[i].first).enabled = true;
				}
			}
			//add removed stat cols total width to the last column
			if (walk_cfg.icol == COL_STAT_POS - 1) {
				size_t width = ListView_GetColumnWidth(wndlist, walk_cfg.icol);
				width += removed_stats_width;
				ListView_SetColumnWidth(wndlist, COL_STAT_POS - 1, width);
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

		int def_fieldwidth = ListView_GetColumnWidth(tag_results_list, 0);

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

void CPreviewTagsDialog::set_image_list() {
    //..
}
