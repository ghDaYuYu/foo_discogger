#include "stdafx.h"

#include <GdiPlus.h>
#pragma comment(lib, "gdiplus.lib")

#include "string_encoded_array.h"
#include "tasks.h"
#include "tag_mappings_dialog.h"
#include "track_matching_dialog.h"
#include "configuration_dialog.h"
#include "preview_modal_tag_dialog.h"
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
		::GetClientRect(m_results_list, &client_rectangle);
		int width = client_rectangle.Width();

		int c1 = width / 3;
		int	c2 = width / 3 * 2;
		ListView_SetColumnWidth(m_results_list, 0, c1);
		ListView_SetColumnWidth(m_results_list, 1, c2);
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
	
	
	int hi, lo;
	hi = HIWORD(CONF.album_art_skip_default_cust); //<-- user skips artwork
	lo = LOWORD(CONF.album_art_skip_default_cust);

	const int skip_tristate = ::IsDlgButtonChecked(m_hWnd, IDC_CHECK_SKIP_ARTWORK);

	switch (skip_tristate) {
	case (BST_CHECKED):
		hi |= ARTSAVE_SKIP_USER_FLAG;
		break;
	case (BST_INDETERMINATE):
		[[fallthrough]];
	case (BST_UNCHECKED):
		hi &= (int)~ARTSAVE_SKIP_USER_FLAG;
		break;
	default:
		;
	}
	conf.album_art_skip_default_cust = MAKELPARAM(lo, hi);	

	if (CONF.album_art_skip_default_cust != conf.album_art_skip_default_cust) {

		bres |= true;
	}

	size_t attach_flagged = (IsDlgButtonChecked(IDC_EDIT_TAGS_DLG_BIND_FLAG) == BST_CHECKED) ? FLG_TAGMAP_DLG_ATTACHED : 0;	
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

	conf = CConf(CONF);
	conf.SetName("PreviewDlg");
	
	defaultCfgCols(cfg_lv);
	cfg_lv.colmap.at(2).width = static_cast<float>(conf.preview_tags_dialog_w_width);
	cfg_lv.colmap.at(3).width = static_cast<float>(conf.preview_tags_dialog_u_width);
	cfg_lv.colmap.at(4).width = static_cast<float>(conf.preview_tags_dialog_s_width);
	cfg_lv.colmap.at(5).width = static_cast<float>(conf.preview_tags_dialog_e_width);

	m_results_list = GetDlgItem(IDC_PREVIEW_LIST);

	ListView_SetExtendedListViewStyleEx(m_results_list,
		LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_INFOTIP | LVS_EX_LABELTIP, LVS_EX_FULLROWSELECT |
		(conf.list_style != 1 ? LVS_EX_GRIDLINES : 0) | LVS_EX_INFOTIP | LVS_EX_LABELTIP);

	reset_default_columns(m_results_list, true);
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
		uButton_SetCheck(m_hWnd, IDC_EDIT_TAGS_DLG_BIND_FLAG, true);
		
		if (conf.edit_tags_dlg_flags & FLG_TAGMAP_DLG_OPENED) {
			BOOL bdummy = false;
			OnEditTagMappings(0, 0, NULL, bdummy);
		}
	}

	bool managed_artwork = !(CONFARTWORK == uartwork(CONF));
	LPARAM lpskip = conf.album_art_skip_default_cust;

	bool user_wants_skip_artwork = HIWORD(lpskip) & ARTSAVE_SKIP_USER_FLAG;
	bool cust_wants_skip_artwork = HIWORD(lpskip) & ARTSAVE_SKIP_CUST_FLAG;

	if (user_wants_skip_artwork) {

		//Nothing to do
		::CheckDlgButton(m_hWnd, IDC_CHECK_SKIP_ARTWORK, BST_CHECKED);;
	}
	else {
	
		if (cust_wants_skip_artwork) {
			::CheckDlgButton(m_hWnd, IDC_CHECK_SKIP_ARTWORK, BST_INDETERMINATE);;
		}
		else {
			::CheckDlgButton(m_hWnd, IDC_CHECK_SKIP_ARTWORK, BST_UNCHECKED);;
		}
	}

	//..REMOVE TRI-STATE
	SendMessage(GetDlgItem(IDC_CHECK_SKIP_ARTWORK)
		, BM_SETSTYLE
		, (WPARAM)0
		, (LPARAM)0);

	SendMessage(GetDlgItem(IDC_CHECK_SKIP_ARTWORK)
		, BM_SETSTYLE
		, (WPARAM)BS_CHECKBOX | BS_AUTOCHECKBOX
		, (LPARAM)0);
	//..REMOVE TRI-STATE

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

LRESULT CPreviewTagsDialog::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {

	HWND hwndCtrl = (HWND)wParam;
	size_t iItem = ListView_GetFirstSelection(m_results_list);

	context_menu_show(hwndCtrl, iItem, lParam /*Coords*/);

	return FALSE;
}


bool CPreviewTagsDialog::context_menu_show(HWND wnd, size_t isel, LPARAM lParamCoords) {

	bool bvk_apps = lParamCoords == -1;
	bool bselection = isel != ~0;

	POINT point;

	if (bvk_apps) {
		CRect rect;
		CWindow(wnd).GetWindowRect(&rect);
		point = rect.CenterPoint();
	}
	else {
		point.x = GET_X_LPARAM(lParamCoords);
		point.y = GET_Y_LPARAM(lParamCoords);
	}

	try {

		HMENU menu = CreatePopupMenu();

		int csel = 0;
		size_t citems = 0;
		bit_array_bittable selmask(0);

		bool is_results = wnd == uGetDlgItem(IDC_PREVIEW_LIST);
		
		if (is_results) {

			is_results = true;
			csel = ListView_GetSelectedCount(wnd);
			citems = ListView_GetItemCount(wnd);
			selmask.resize(citems);
			size_t n = ListView_GetFirstSelection(wnd) == -1 ? pfc_infinite : ListView_GetFirstSelection(wnd);
			for (size_t i = n; i < citems; i++) {
				selmask.set(i, ListView_IsItemSelected(wnd, i));				
			}
		}

		// menu options

		if (is_results) {

			if (bselection) {

				uAppendMenu(menu, MF_STRING, ID_PREVIEW_CMD_EDIT_RESULT_TAG, "&Edit");
				uAppendMenu(menu, MF_STRING, ID_PREVIEW_CMD_COPY, "&Copy");
				uAppendMenu(menu, MF_SEPARATOR, 0, 0);
			}	
        }

		uAppendMenu(menu, MF_STRING, ID_PREVIEW_CMD_BACK, "&Back");
		uAppendMenu(menu, MF_SEPARATOR, 0, 0);
		bool bskip = IsDlgButtonChecked(IDC_CHECK_SKIP_ARTWORK);		
		bool bshowstats = IsDlgButtonChecked(IDC_CHECK_PREV_DLG_SHOW_STATS);
		bool bnormal = IsDlgButtonChecked(IDC_VIEW_NORMAL);
		bool bdiff = IsDlgButtonChecked(IDC_VIEW_DIFFERENCE);
		bool bori = IsDlgButtonChecked(IDC_VIEW_ORIGINAL);
		uAppendMenu(menu, MF_STRING | (bskip? MF_CHECKED : 0), ID_PREVIEW_CMD_SKIP_ARTWORK, "&skip artwork");
		uAppendMenu(menu, MF_STRING | (bshowstats ? MF_CHECKED : 0), ID_PREVIEW_CMD_SHOW_STATS, "s&how stats");
		uAppendMenu(menu, MF_SEPARATOR, 0, 0);
		uAppendMenu(menu, MF_STRING | (bnormal ? MF_CHECKED : 0), ID_PREVIEW_CMD_RESULTS_VIEW, "&Results view");
		uAppendMenu(menu, MF_STRING | (bdiff ? MF_CHECKED : 0), ID_PREVIEW_CMD_DIFF_VIEW, "&Difference view");
		uAppendMenu(menu, MF_STRING | (bori ? MF_CHECKED : 0), ID_PREVIEW_CMD_ORI_VIEW, "&Original view");
		uAppendMenu(menu, MF_SEPARATOR, 0, 0);
		uAppendMenu(menu, MF_STRING | (false/*todo*/ ? MF_DISABLED | MF_GRAYED : 0), ID_PREVIEW_CMD_WRITE_TAGS, "&Write tags");

		int cmd = TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, point.x, point.y, 0, wnd, 0);
		DestroyMenu(menu);

		context_menu_switch(wnd, point, is_results, cmd, selmask /*, ilist */);
	}
	catch (...) {}
	return false;
}

bool CPreviewTagsDialog::context_menu_switch(HWND wnd, POINT point, bool is_results, int cmd, bit_array_bittable selmask) {

	switch (cmd)
	{
	case ID_PREVIEW_CMD_WRITE_TAGS: {

		BOOL bDummy;
		return OnButtonWriteTags(0, 0, NULL, bDummy);
	}
	case ID_PREVIEW_CMD_BACK: {

		BOOL bDummy;
		return OnButtonBack(0, 0, NULL, bDummy);		
	}
	case ID_PREVIEW_CMD_SKIP_ARTWORK: {

		bool prev = IsDlgButtonChecked(IDC_CHECK_SKIP_ARTWORK);
		uButton_SetCheck(m_hWnd, IDC_CHECK_SKIP_ARTWORK, !prev);
		::SendMessage(m_hWnd, WM_COMMAND, MAKEWPARAM(IDC_CHECK_SKIP_ARTWORK, BN_CLICKED), 0);
		return true;
	}
	case ID_PREVIEW_CMD_SHOW_STATS: {
		bool prev = IsDlgButtonChecked(IDC_CHECK_PREV_DLG_SHOW_STATS);
		uButton_SetCheck(m_hWnd, IDC_CHECK_PREV_DLG_SHOW_STATS, !prev);
		::SendMessage(m_hWnd, WM_COMMAND, MAKEWPARAM(IDC_CHECK_PREV_DLG_SHOW_STATS, BN_CLICKED), 0);
		return true;
	}
	case ID_PREVIEW_CMD_RESULTS_VIEW: {
		uButton_SetCheck(m_hWnd, IDC_VIEW_NORMAL, true);
		uButton_SetCheck(m_hWnd, IDC_VIEW_DIFFERENCE, false);
		uButton_SetCheck(m_hWnd, IDC_VIEW_ORIGINAL, false);
		uButton_SetCheck(m_hWnd, IDC_VIEW_DEBUG, false);

		::SendMessage(m_hWnd, WM_COMMAND, MAKEWPARAM(IDC_VIEW_NORMAL, BN_CLICKED), 0);
		return true;
	}
	case ID_PREVIEW_CMD_DIFF_VIEW: {
		uButton_SetCheck(m_hWnd, IDC_VIEW_NORMAL, false);
		uButton_SetCheck(m_hWnd, IDC_VIEW_DIFFERENCE, true);
		uButton_SetCheck(m_hWnd, IDC_VIEW_ORIGINAL, false);
		uButton_SetCheck(m_hWnd, IDC_VIEW_DEBUG, false);

		::SendMessage(m_hWnd, WM_COMMAND, MAKEWPARAM(IDC_VIEW_DIFFERENCE, BN_CLICKED), 0);
		return true;
	}
	case ID_PREVIEW_CMD_ORI_VIEW: {
		uButton_SetCheck(m_hWnd, IDC_VIEW_NORMAL, false);
		uButton_SetCheck(m_hWnd, IDC_VIEW_DIFFERENCE, false);
		uButton_SetCheck(m_hWnd, IDC_VIEW_ORIGINAL, true);
		uButton_SetCheck(m_hWnd, IDC_VIEW_DEBUG, false);

		::SendMessage(m_hWnd, WM_COMMAND, MAKEWPARAM(IDC_VIEW_ORIGINAL, BN_CLICKED), 0);
		return true;
	}

	// ,,

	case ID_PREVIEW_CMD_EDIT_RESULT_TAG: {

		size_t isel = selmask.find_first(true, 0, selmask.size());

		if (isel != ~0) {

			tag_result_ptr& item_result = m_tag_writer->tag_results[isel];
			file_info_manager_ptr finfo_manager = m_tag_writer->finfo_manager;
			track_mappings_list_type track_mappings = m_tag_writer->track_mappings;

			std::vector<pfc::string8> vtracks_desc;

			for (size_t i = 0; i < track_mappings.get_count(); i++) {
				track_mapping tm = track_mappings[i];
				if (tm.enabled) {
					pfc::string8 track_desc;
					metadb_handle_ptr item = finfo_manager->items[tm.file_index];
					item->format_title(nullptr, track_desc, m_track_desc_script, nullptr);	//TRACK DESCRIPTION
					vtracks_desc.emplace_back(track_desc);
				}
			}

			if (!g_discogs->preview_modal_tag_dialog) {
				fb2k::newDialog <CPreviewModalTagDialog>(core_api::get_main_window(), isel, item_result, vtracks_desc, get_preview_mode());
			}			
			else
			{
				g_discogs->preview_modal_tag_dialog->ReLoad(core_api::get_main_window(), isel, item_result, vtracks_desc, get_preview_mode());
			}
		}

		return true;
	}
	case ID_PREVIEW_CMD_COPY: {

		HWND hwndList = uGetDlgItem(IDC_PREVIEW_LIST);

		size_t isel = ListView_GetFirstSelection(hwndList);
		
		pfc::string8 utf_buffer;
		TCHAR outBuffer[MAX_PATH + 1] = {};

		if (isel != -1) {
			LVITEM lvi;
			TCHAR outBuffer[MAX_PATH + 1] = {};
			lvi.pszText = outBuffer;
			lvi.cchTextMax = MAX_PATH;
			lvi.mask = LVIF_TEXT;
			lvi.stateMask = (UINT)-1;
			lvi.iItem = isel;
			lvi.iSubItem = 1;

			BOOL result = ListView_GetItem(hwndList, &lvi);
			utf_buffer << pfc::stringcvt::string_utf8_from_os(outBuffer).get_ptr();
		}

		if (utf_buffer.get_length()) {

			ClipboardHelper::OpenScope scope; scope.Open(core_api::get_main_window());
			ClipboardHelper::SetString(utf_buffer);

		}

		return true;
	}
	default: {

		return false;
	}
	}
	return false;
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

		std::filesystem::path os_path_small = std::filesystem::u8path(cache_path_small.c_str());

		Bitmap bm(os_path_small.wstring().c_str());

		if (/*HBITMAP hBm;*/  bm.GetHBITMAP(Color::Black, &m_preview_bitmap) == Ok) {
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

void CPreviewTagsDialog::replace_tag_result(size_t item, tag_result_ptr result) {
	
	if (STR_EQUAL(m_tag_writer->tag_results[item]->tag_entry->tag_name, result->tag_entry->tag_name)) {
		m_tag_writer->tag_results[item]->value = result->value;
	}
}

void CPreviewTagsDialog::insert_tag_results(bool computestat) {
	ListView_DeleteAllItems(m_results_list);
	if (computestat) {
		v_stats.clear(); // .reset();
		compute_stats(m_tag_writer->tag_results);
	}

	const size_t count = m_tag_writer->tag_results.get_count();
	for (unsigned int i = 0; i < count; i++) {
		int preview_mode = get_preview_mode();
		insert_tag_result(i, m_tag_writer->tag_results[i], preview_mode);
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
		listview_helper::get_item_text(TableEdit_GetParentWnd(), (int)item, (int)subItem, out);
		bool onelined = out.length() < 50 && out.find_first(';', 0) == pfc_infinite;
		lineCount = onelined ? 1 : LINES_LONGFIELD;
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

// print debug

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

// print normal (original: input oldvalue, results: input value)

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

// print difference

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

// print result with mode param

pfc::string8 print_result_in_mode(const tag_result_ptr& result, int preview_mode) {

	pfc::string8 mode_result;

	if (preview_mode == PREVIEW_NORMAL) {
		mode_result = print_normal(GetPreviewValue(result));
	}
	else if (preview_mode == PREVIEW_DIFFERENCE) {
		mode_result = print_difference(result->value, result->old_value);
	}
	else if (preview_mode == PREVIEW_ORIGINAL) {
		mode_result = print_normal(result->old_value);
	}
	else {
		mode_result = print_stenar(result->value);
	}

	return mode_result;
}

void CPreviewTagsDialog::insert_tag_result(int pos, const tag_result_ptr &result, int preview_mode) {
	
	pfc::string8 mode_result = print_result_in_mode(result, preview_mode).c_str();
	
	listview_helper::insert_item2(m_results_list, pos, result->tag_entry->tag_name, mode_result, 0);
	
	if (cfg_preview_dialog_show_stats) {
		listview_helper::set_item_text(m_results_list, pos, 2, v_stats.size() > 0 ? std::to_string(v_stats.at(pos).totalwrites).c_str() : "n/a");
		listview_helper::set_item_text(m_results_list, pos, 3, v_stats.size() > 0 ? std::to_string(v_stats.at(pos).totalupdates).c_str() : "n/a");
		listview_helper::set_item_text(m_results_list, pos, 4, v_stats.size() > 0 ? std::to_string(v_stats.at(pos).totalskips).c_str() : "n/a");
		listview_helper::set_item_text(m_results_list, pos, 5, v_stats.size() > 0 ? std::to_string(v_stats.at(pos).totalequal).c_str() : "n/a");
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
		SendMessage(g_discogs->configuration_dialog->m_hWnd, WM_CUSTOM_ANV_CHANGED, 0, state);
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

	// remove tristate style

	SendMessage(GetDlgItem(IDC_CHECK_SKIP_ARTWORK)
		, BM_SETSTYLE
		, (WPARAM)0
		, (LPARAM)0);
	SendMessage(GetDlgItem(IDC_CHECK_SKIP_ARTWORK)
		, BM_SETSTYLE
		, (WPARAM)BS_CHECKBOX | BS_AUTOCHECKBOX
		, (LPARAM)0);

	return FALSE;
}

LRESULT CPreviewTagsDialog::OnCheckAttachMappingPanel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	bool state = IsDlgButtonChecked(IDC_EDIT_TAGS_DLG_BIND_FLAG);
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
	int top = ListView_GetTopIndex(m_results_list);
	int sel = ListView_GetSelectionMark(m_results_list);

	SendMessage(m_results_list, WM_SETREDRAW, FALSE, 0);
	insert_tag_results(false);

	SendMessage(m_results_list, LVM_ENSUREVISIBLE,
		SendMessage(m_results_list, LVM_GETITEMCOUNT, 0, 0) - 1, TRUE); //Rool to the end
	SendMessage(m_results_list, LVM_ENSUREVISIBLE, top/* - 1*/, TRUE); //Roll back to top position

	ListView_SetItemState(m_results_list, sel, LVNI_SELECTED | LVNI_FOCUSED, LVNI_SELECTED | LVNI_FOCUSED);
	SendMessage(m_results_list, WM_SETREDRAW, TRUE, 0);
	
	return FALSE;
}

LRESULT CPreviewTagsDialog::OnButtonWriteTags(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	build_current_cfg();
	pushcfg();

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

LRESULT CPreviewTagsDialog::OnListDoubleClick(LPNMHDR lParam) {

	if (lParam->idFrom == IDC_PREVIEW_LIST) {
	
		NMITEMACTIVATE* info = reinterpret_cast<NMITEMACTIVATE*>(lParam);

		if (info->iItem != -1 && info->iSubItem == 0) {

			const int index = ListView_GetSingleSelection(m_results_list);
			const int icount = ListView_GetItemCount(m_results_list);
			bit_array_bittable selmask(icount);
			selmask.set(index, true);
			context_menu_switch(info->hdr.hwndFrom, info->ptAction, true, ID_PREVIEW_CMD_EDIT_RESULT_TAG, selmask);
		}
	}

	return FALSE;
}

bool CPreviewTagsDialog::delete_selection() {
	const int index = ListView_GetSingleSelection(m_results_list);
	if (index < 0) {
		return false;
	}
	
	ListView_DeleteItem(m_results_list, index);
	erase(m_tag_writer->tag_results, index);

	int total = ListView_GetItemCount(m_results_list);
	if (index < total) {
		listview_helper::select_single_item(m_results_list, index);
	}
	else if (total > 0) {
		listview_helper::select_single_item(m_results_list, total - 1);
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
			int index = ListView_GetSingleSelection(m_results_list);
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
		CTrackMatchingDialog* dlg = g_discogs->track_matching_dialog;
		dlg->destroy_all();
	}
	destroy();
}

void CPreviewTagsDialog::go_back() {

	destroy();

	// back to track matching

	if (g_discogs->track_matching_dialog) {

		CTrackMatchingDialog* dlg = g_discogs->track_matching_dialog;
		HWND hwndButton = ::GetDlgItem(dlg->m_hWnd, IDC_PREVIEW_TAGS_BUTTON);
		::PostMessage(dlg->m_hWnd, WM_NEXTDLGCTL, (WPARAM)hwndButton, TRUE);
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
		reset_default_columns(m_results_list, true);
	}
	else {
		
		reset_default_columns(m_results_list, false);

		int def_statswidth = 0;
		for (int cc = COL_STAT_POS; cc < COL_STAT_POS + COL_STAT_COLS; cc++) {
			cfg_lv.colmap.at(cc).enabled = true;
			ListView_SetColumnWidth(m_results_list, cc, cfg_lv.colmap.at(cc).width);
			def_statswidth += (int)cfg_lv.colmap.at(cc).width;
		}

		int def_fieldwidth = ListView_GetColumnWidth(m_results_list, 0);

		CRect reclist;
		::GetWindowRect(m_results_list, reclist);

		int framewidth = reclist.Width();
		int delta = framewidth - def_fieldwidth - def_statswidth - 40;

		if (delta < 0) {
			ListView_SetColumnWidth(m_results_list, 1, delta);
			ListView_SetColumnWidth(m_results_list, 0, def_fieldwidth);
		}
		else {
			ListView_SetColumnWidth(m_results_list, 1, delta);
		}
		for (int cc = COL_STAT_POS; cc < COL_STAT_POS + COL_STAT_COLS; cc++) {
			ListView_SetColumnWidth(m_results_list, cc, cfg_lv.colmap.at(cc).width);
		}

		update_sorted_icol_map(false /*sort without reset*/);
	}

	ListView_DeleteAllItems(m_results_list);
	insert_tag_results(true);

	ListView_RedrawItems(m_results_list, 0, ListView_GetGroupCount(m_results_list));
	return FALSE;
}

void CPreviewTagsDialog::set_image_list() {
    //..
}
