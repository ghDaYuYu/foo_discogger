#include "stdafx.h"

#include <GdiPlus.h>
#pragma comment(lib, "gdiplus.lib")

#include "utils_menu.h"
#include "string_encoded_array.h"
#include "tasks.h"
#include "tag_mappings_dialog.h"
#include "track_matching_dialog.h"
#include "configuration_dialog.h"
#include "preview_leading_tag_dialog.h"
#include "preview_dialog.h"

#define DISABLED_RGB	RGB(150, 150, 150)
#define CHANGE_NOT_APPROVED_RGB	RGB(150, 100, 100)
#define CHANGE_USR_APPROVED_RGB	RGB(50, 180, 50)

static const GUID guid_cfg_window_placement_preview_dlg = { 0xcb1debbd, 0x1b0a, 0x4046, { 0x8c, 0x6f, 0x74, 0xee, 0x9d, 0x74, 0x7f, 0x83 } };
static cfg_window_placement cfg_window_placement_preview_dlg(guid_cfg_window_placement_preview_dlg);

using namespace Gdiplus;

struct cfg_preview_column_type {
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

using cfg_preview_column_map = std::map<std::int32_t, cfg_preview_column_type>;

const int COL_DEF_NCOLS = 6;
const int COL_STAT_NCOLS = 4;
const int COL_STATS_FIRST_COL = 2;

cfg_preview_column_type column_configs[COL_DEF_NCOLS] = {
	//#, ndx, icol, name, tf, width, celltype, factory, default, enabled, defuser
	{1, 1, 0,"Tag Name", "",	20.0f, 0x0000, 0, true, true, false, false},
	{2, 2, 1,"Value(s)", "",	40.0f, 0x0000, 0, true, true, false, false},
	{3, 3, 2,"Write", "",		40.0f, 0x0000, 0, true, false, false, false},
	{4, 4, 3,"Update", "",		40.0f, 0x0000, 0, true, false, false, false},
	{5, 5, 4,"Skip W/U", "",	40.0f, 0x0000, 0, true,false, false, false},
	{6, 6, 5,"Equal", "",		40.0f, 0x0000, 0, true, false, false, false},
};

struct cfg_preview_listview_type {
	bool autoscale = false;
	cfg_preview_column_map colmap = {};
	uint32_t flags = 0;

	void init() {
		autoscale = false;
		colmap.clear();
		for (int it = 0; it < PFC_TABSIZE(column_configs); it++) {
			if (!column_configs[it].name.has_prefix("---")) {
				cfg_preview_column_type tmp_cfgcol{
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
				colmap.emplace((int32_t)colmap.size(), tmp_cfgcol);
			}
		}
	}
} cfg_listview;

void CPreviewTagsDialog::load_column_layout() {

	bool bshowstats = conf.edit_tags_dlg_show_tm_stats;
	int min_cols = COL_STATS_FIRST_COL + (bshowstats ? COL_STAT_NCOLS : 0);
	bool wrong_layout = !conf.preview_tags_dialog_col1_width;

	reset_default_columns(wrong_layout, bshowstats);
}

bool CPreviewTagsDialog::build_current_cfg() {

	bool bres = false;
	bool local_replace_ANV = IsDlgButtonChecked(IDC_CHK_REPLACE_ANV);
	if (CONF.replace_ANVs != conf.replace_ANVs &&
		conf.replace_ANVs != local_replace_ANV) {
		bres |= true;
	}

	//preview mode
	int current_preview_mode = (int)get_preview_mode();
	if (current_preview_mode != conf.preview_mode) {
		conf.preview_mode = current_preview_mode;
		bres |= true;
	}

	int colwidth1 = m_uilist.GetColumnWidthF(0);
	int colwidth2 = m_uilist.GetColumnWidthF(1);
	
	//columns
	if (colwidth1 != conf.preview_tags_dialog_col1_width || colwidth2 != conf.preview_tags_dialog_col2_width) {
		conf.preview_tags_dialog_col1_width = colwidth1;
		conf.preview_tags_dialog_col2_width = colwidth2;
		bres |= true;
	}

	//stats
	const bool bstats_checked = uButton_GetCheck(m_hWnd, IDC_CHK_PREV_DLG_SHOW_STATS);

	if (bstats_checked) {

		int colw = m_uilist.GetColumnWidthF(2);
		int colu = m_uilist.GetColumnWidthF(3);
		int cols = m_uilist.GetColumnWidthF(4);
		int cole = m_uilist.GetColumnWidthF(5);

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
	hi = HIWORD(CONF.album_art_skip_default_cust);
	lo = LOWORD(CONF.album_art_skip_default_cust);

	const int skip_tristate = ::IsDlgButtonChecked(m_hWnd, IDC_CHK_SKIP_ARTWORK);

#pragma warning( push )
#pragma warning( disable : 26819 )
	switch (skip_tristate) {
	case (BST_CHECKED):
		hi |= ARTSAVE_SKIP_USER_FLAG;
		break;
	case (BST_INDETERMINATE):
		[[fallthrough]];
	case (BST_UNCHECKED):
		hi &= ~(ARTSAVE_SKIP_USER_FLAG);
		break;
	default:
		;
	}
#pragma warning( pop )

	conf.album_art_skip_default_cust = MAKELPARAM(lo, hi);	

	if (CONF.album_art_skip_default_cust != conf.album_art_skip_default_cust) {

		bres |= true;
	}

	//bind tags panel
	size_t attach_flagged = (IsDlgButtonChecked(IDC_CHK_BIND_TAGS_DLG) == BST_CHECKED) ? FLG_TAGMAP_DLG_ATTACHED : 0;
	size_t open_flagged = g_discogs->tag_mappings_dialog ? FLG_TAGMAP_DLG_OPENED : 0;

	conf.edit_tags_dlg_flags &= ~(FLG_TAGMAP_DLG_ATTACHED | FLG_TAGMAP_DLG_OPENED);
	conf.edit_tags_dlg_flags |= (attach_flagged | open_flagged);

	bres |= (CONF.edit_tags_dlg_flags != conf.edit_tags_dlg_flags);

	return bres;
}
inline bool get_diff_release_name(TagWriter_ptr tag_writer, pfc::string8& rel_desc, pfc::string8& diff_rel_id) {

	bool bdiffid = false;
	file_info_impl finfo;
	metadb_handle_ptr item = tag_writer->m_finfo_manager->items[0];
	item->get_info(finfo);

	pfc::string8 local_release_id;

	const char* ch_local_rel = finfo.meta_get("DISCOGS_RELEASE_ID", 0);
	if (ch_local_rel) {
		local_release_id = ch_local_rel;
	}
	bdiffid = (local_release_id.get_length() && !STR_EQUAL(tag_writer->release->id, local_release_id));

	pfc::string8 compact_release;
	playable_location_impl location;
	file_info_impl info;
	titleformat_hook_impl_multiformat hook;
	hook.set_release(&tag_writer->release);
	CONF.search_master_sub_format_string->run_hook(location, &info, &hook, compact_release, nullptr);

	diff_rel_id = bdiffid ? "" : tag_writer->release->id;
	rel_desc = bdiffid ? "!! " : "";
	rel_desc << ltrim(compact_release);

	return bdiffid;

}

CPreviewTagsDialog::~CPreviewTagsDialog() {


	awt_save_normal_mode();

	if (m_uilist.TableEdit_IsActive()) {
		m_uilist.TableEdit_Abort(true);
	}

	if (g_discogs->preview_modal_tag_dialog) {
		CPreviewLeadingTagDialog* dlg = g_discogs->preview_modal_tag_dialog;
		dlg->destroy();
	}
	DeleteObject(m_preview_bitmap);
	g_discogs->preview_tags_dialog = nullptr;
}

LRESULT CPreviewTagsDialog::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {

	showtitle();
	SetIcon(g_discogs->icon);

	conf = CConf(CONF);
	conf.SetName("PreviewDlg");

	cfg_listview.init();

	cfg_listview.colmap.at(0).width = static_cast<float>(conf.preview_tags_dialog_col1_width);
	cfg_listview.colmap.at(1).width = static_cast<float>(conf.preview_tags_dialog_col2_width);

	cfg_listview.colmap.at(2).width = static_cast<float>(conf.preview_tags_dialog_w_width);
	cfg_listview.colmap.at(3).width = static_cast<float>(conf.preview_tags_dialog_u_width);
	cfg_listview.colmap.at(4).width = static_cast<float>(conf.preview_tags_dialog_s_width);
	cfg_listview.colmap.at(5).width = static_cast<float>(conf.preview_tags_dialog_e_width);

	if (conf.edit_tags_dlg_show_tm_stats) {
		cfg_listview.colmap.at(2).enabled =
			cfg_listview.colmap.at(3).enabled =
			cfg_listview.colmap.at(4).enabled =
			cfg_listview.colmap.at(5).enabled = true;
	}
	if (!cfg_listview.colmap.at(2).width) {
			auto dpiX = QueryScreenDPIEx(m_hWnd).cx;	
			auto sbwitch = GetSystemMetrics(SM_CXVSCROLL);
			int fw = MulDiv(sbwitch, dpiX, USER_DEFAULT_SCREEN_DPI);

			cfg_listview.colmap.at(2).width =
				cfg_listview.colmap.at(3).width =
				cfg_listview.colmap.at(4).width =
				cfg_listview.colmap.at(5).width = 2 * fw;
			if (conf.edit_tags_dlg_show_tm_stats) {
				cfg_listview.colmap.at(1).width -= (2 * fw * 4);
			}
	}

	m_results_list = GetDlgItem(IDC_PREVIEW_LIST);

	m_uilist.CreateInDialog(m_hWnd, IDC_PREVIEW_LIST, m_results_list);
	m_results_list = m_uilist.m_hWnd;
	
	m_uilist.InitializeHeaderCtrl(HDS_FULLDRAG);
	m_uilist.SetRowStyle(conf.list_style);

	HWND hwndHeader = ListView_GetHeader(m_results_list);
	hwndHeader = ListView_GetHeader(m_results_list);

	::ShowWindow(uGetDlgItem(IDC_BTN_TAG_MAPPINGS), true);
	::ShowWindow(uGetDlgItem(IDC_BTN_BACK), true);
	::ShowWindow(uGetDlgItem(IDC_BTN_SKIP), false);
	::ShowWindow(uGetDlgItem(IDCANCEL), true);

	CustomFont(m_hWnd, HIWORD(conf.custom_font));

	// dlg resize and dimensions

	DlgResize_Init(mygripp.enabled, true);
	cfg_window_placement_preview_dlg.on_window_creation(m_hWnd, true);

	load_column_layout();
	set_preview_mode((PreView)conf.preview_mode);

	init_other_controls_and_results();

	//add rec icon to write tags button
	HWND write_btn = GetDlgItem(IDC_BTN_WRITE_TAGS);
	::SendMessageW(write_btn, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)g_hIcon_rec);
	HWND hwndSmallPreview = GetDlgItem(IDC_ALBUM_ART);

	LPARAM stl = ::uGetWindowLong(hwndSmallPreview, GWL_STYLE);
	stl |= SS_GRAYFRAME;
	::uSetWindowLong(hwndSmallPreview, GWL_STYLE, stl);

	pfc::string8 rel_desc, rel_diff_id;
	get_diff_release_name(m_tag_writer, rel_desc, rel_diff_id);
	uSetDlgItemText(m_hWnd, IDC_STATIC_MATCH_TRACKING_REL_NAME, rel_desc);

	if (conf.edit_tags_dlg_show_tm_stats) {

		uButton_SetCheck(m_hWnd, IDC_CHK_PREV_DLG_SHOW_STATS, true);
	}

	if (conf.edit_tags_dlg_flags & FLG_TAGMAP_DLG_ATTACHED) {
		uButton_SetCheck(m_hWnd, IDC_CHK_BIND_TAGS_DLG, true);
		
		if (conf.edit_tags_dlg_flags & FLG_TAGMAP_DLG_OPENED) {
			BOOL bdummy = false;
			OnButtonEditTagMappings(0, 0, NULL, bdummy);
		}
	}

	bool user_wants_skip = (HIWORD(conf.album_art_skip_default_cust) & ARTSAVE_SKIP_USER_FLAG) != 0;
	bool save_artwork = conf.save_album_art || conf.save_album_art || conf.embed_album_art || conf.embed_artist_art;

	if (!save_artwork) {
		m_tristate.Init(BST_CHECKED, FALSE);
	}
	else
	{
		//skip start
		bool managed_artwork = !(CONF_MULTI_ARTWORK == multi_uartwork(CONF, m_tag_writer->release));
		LPARAM lpskip = conf.album_art_skip_default_cust;

		bool user_skip_artwork = HIWORD(lpskip) & ARTSAVE_SKIP_USER_FLAG;
		bool ind_skip_artwork = HIWORD(lpskip) & BST_INDETERMINATE;

		if (user_skip_artwork) {

			::CheckDlgButton(m_hWnd, IDC_CHK_SKIP_ARTWORK, BST_CHECKED);
		}
		else {

			if (ind_skip_artwork) {
				::CheckDlgButton(m_hWnd, IDC_CHK_SKIP_ARTWORK, BST_INDETERMINATE);;
			}
			else {
				::CheckDlgButton(m_hWnd, IDC_CHK_SKIP_ARTWORK, BST_UNCHECKED);;
			}
		}

		m_tristate.SetBistate();
	}

	bool write_button_enabled = check_write_tags_status();
	::EnableWindow(GetDlgItem(IDC_BTN_WRITE_TAGS), write_button_enabled);

	HWND hwndDefaultControl;
	if (write_button_enabled)
		hwndDefaultControl = GetDlgItem(IDC_BTN_WRITE_TAGS);
	else
		hwndDefaultControl = GetDlgItem(IDCANCEL);

	SendMessage(m_hWnd, WM_NEXTDLGCTL, (WPARAM)hwndDefaultControl, TRUE);

	//darkmode
	AddDialog(m_hWnd);
	AddControls(m_hWnd);

	show();
	return FALSE;
}

LRESULT CPreviewTagsDialog::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {

	HWND hwndCtrl = (HWND)wParam;
	size_t iItem = m_uilist.GetFirstSelected();
		
	m_uilist.GetContextMenuPoint(lParam);

	context_menu_show(hwndCtrl, iItem, lParam);
	return FALSE;
}

bool CPreviewTagsDialog::context_menu_show(HWND wnd, size_t isel, LPARAM lParamPos) {

	bool bvk_apps = lParamPos == -1;
	bit_array_bittable selmask = m_uilist.GetSelectionMask();
	isel = m_uilist.GetSingleSel();
	bool bselection = isel != ~0;
	size_t csel = m_uilist.GetSelectedCount();

	POINT point;

	if (bvk_apps) {
		CRect rect;
		CWindow(wnd).GetWindowRect(&rect);
		point = rect.CenterPoint();
	}
	else {
		point.x = GET_X_LPARAM(lParamPos);
		point.y = GET_Y_LPARAM(lParamPos);
	}
	pfc::string8 discogs_release_id(m_tag_writer->release->id);
	pfc::string8 master_release_id(m_tag_writer->release->master_id);
	pfc::string8 artist_id(m_tag_writer->release->artists[0]->full_artist->id);

	bool hasRelease = discogs_release_id.get_length();
	bool hasMasterRelease = master_release_id.get_length();
	bool hasArtist = artist_id.get_length();
	std::vector<std::pair<std::string, std::string>>vartists;
	bool is_multiartist = discogs_interface->artists_vid(m_tag_writer->release, vartists);
	//..

	try {

		HMENU menu = CreatePopupMenu();
		HMENU _childmenuAlign = CreatePopupMenu();

		bool b_result_list = wnd == uGetDlgItem(IDC_PREVIEW_LIST);
		// add menu options

		uAppendMenu(menu, MF_STRING, ID_PREVIEW_CMD_BACK, "&Back");
		uAppendMenu(menu, MF_SEPARATOR, 0, 0);

		if (b_result_list) {
			if (bselection) {

				bool binplace = is_result_editable(isel);
				uAppendMenu(menu, MF_STRING, ID_PREVIEW_CMD_EDIT_RESULT_TAG, "&Edit");
				uAppendMenu(menu, MF_STRING | (binplace ? 0 : MF_DISABLED | MF_GRAYED), ID_PREVIEW_CMD_EDIT_RESULT_TAGINP, "E&dit (in-place)");
				uAppendMenu(menu, MF_STRING, ID_PREVIEW_CMD_COPY, "&Copy");

				uAppendMenu(menu, MF_SEPARATOR, 0, 0);
			}
		}

		bool bskip = IsDlgButtonChecked(IDC_CHK_SKIP_ARTWORK);		
		bool bshowstats = IsDlgButtonChecked(IDC_CHK_PREV_DLG_SHOW_STATS);

		bool bnormal = IsDlgButtonChecked(IDC_VIEW_NORMAL);
		bool bdiff = IsDlgButtonChecked(IDC_VIEW_DIFFERENCE);
		bool bori = IsDlgButtonChecked(IDC_VIEW_ORIGINAL);

		uAppendMenu(menu, MF_STRING | (bskip? MF_CHECKED : 0), ID_PREVIEW_CMD_SKIP_ARTWORK, "&skip artwork");
		uAppendMenu(menu, MF_STRING | (bshowstats ? MF_CHECKED : 0), ID_PREVIEW_CMD_SHOW_STATS, "s&how stats");

		uAppendMenu(menu, MF_SEPARATOR, 0, 0);
		uAppendMenu(menu, MF_STRING | (bnormal ? MF_CHECKED : 0), ID_PREVIEW_CMD_RESULTS_VIEW, "&Results view");
		uAppendMenu(menu, MF_STRING | (bdiff ? MF_CHECKED : 0), ID_PREVIEW_CMD_DIFF_VIEW, "Di&fference view");
		uAppendMenu(menu, MF_STRING | (bori ? MF_CHECKED : 0), ID_PREVIEW_CMD_ORI_VIEW, "&Original view");
		if (!b_result_list) {
			uAppendMenu(menu, MF_SEPARATOR, 0, 0);
			uAppendMenu(menu, MF_STRING | (!hasRelease ? MF_DISABLED | MF_GRAYED : 0), ID_URL_RELEASE, "Web &release page");
			uAppendMenu(menu, MF_STRING | (!hasMasterRelease ? MF_DISABLED | MF_GRAYED : 0), ID_URL_MASTER_RELEASE, "Web mas&ter release page");
			uAppendMenu(menu, MF_STRING | (!hasArtist ? MF_DISABLED | MF_GRAYED | MF_GRAYED : 0), ID_URL_ARTIST, "Web &artist page");
			if (is_multiartist) {
				build_sub_menu(menu, ID_URL_ARTISTS, vartists, _T("Web release artist&s page"), 16);
			}
		}

		if (bselection && csel == 1) {
			PreView pv = get_preview_mode();
			bool bshow_write_mask = false;
			if (pv == PreView::Diff) {
				auto tmpdif = ((ILOD_preview*)this)->GetListRow(isel, pv);
				bshow_write_mask = tmpdif.get_length();
			}
			else if (pv == PreView::Normal) {
				auto tmpdif = ((ILOD_preview*)this)->GetListRow(isel, pv);
				bshow_write_mask = tmpdif.get_length();
			}
			pfc::string8 str;
			str << "Write se&lected tag (" << (preview_to_label(pv).toLower()) << ")";

			uAppendMenu(menu, MF_SEPARATOR, 0, 0);
			uAppendMenu(menu, MF_STRING | bshow_write_mask? 0 : MF_DISABLED | MF_GRAYED, ID_PREVIEW_CMD_WRITE_TAGS_MASK, str/*"Write se&lected tag"*/);
		}
		uAppendMenu(menu, MF_SEPARATOR, 0, 0);
		uAppendMenu(menu, MF_STRING | !bselection || csel == 1 ? 0 : MF_DISABLED | MF_GRAYED, ID_PREVIEW_CMD_WRITE_TAGS, !bselection || csel == 1 ? "&Write all tags" : "&Write tags");

		int cmd = TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, point.x, point.y, 0, wnd, 0);
		DestroyMenu(menu);

		context_menu_switch(wnd, point, cmd, selmask, vartists);
	}
	catch (...) {}
	return false;
}

bool CPreviewTagsDialog::context_menu_switch(HWND wnd, POINT point, int cmd, bit_array_bittable selmask, std::vector<std::pair<std::string, std::string>>vartists) {

	UINT att_id = 0;
	pfc::string8 url;

	switch (cmd)
	{
	case ID_PREVIEW_CMD_WRITE_TAGS:
	case ID_PREVIEW_CMD_WRITE_TAGS_MASK: {
		m_write_only_selected = selmask.size();
		BOOL bDummy;
		return OnButtonWriteTags(0, 0, NULL, bDummy);
	}
	case ID_PREVIEW_CMD_BACK: {

		BOOL bDummy;
		return OnButtonBack(0, 0, NULL, bDummy);		
	}
	case ID_PREVIEW_CMD_SKIP_ARTWORK: {

		bool prev = IsDlgButtonChecked(IDC_CHK_SKIP_ARTWORK);
		uButton_SetCheck(m_hWnd, IDC_CHK_SKIP_ARTWORK, !prev);
		::SendMessage(m_hWnd, WM_COMMAND, MAKEWPARAM(IDC_CHK_SKIP_ARTWORK, BN_CLICKED), 0);
		return true;
	}
	case ID_PREVIEW_CMD_SHOW_STATS: {
		bool prev = IsDlgButtonChecked(IDC_CHK_PREV_DLG_SHOW_STATS);
		uButton_SetCheck(m_hWnd, IDC_CHK_PREV_DLG_SHOW_STATS, !prev);
		::SendMessage(m_hWnd, WM_COMMAND, MAKEWPARAM(IDC_CHK_PREV_DLG_SHOW_STATS, BN_CLICKED), 0);
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
	case ID_PREVIEW_CMD_EDIT_RESULT_TAGINP: {

		size_t isel = selmask.find_first(true, 0, selmask.size());
		if (isel < selmask.size()) {
			if (is_result_editable(isel)) {
				m_uilist.TableEdit_Start(isel, 1);
			}
		}
		break;
	}
	case ID_PREVIEW_CMD_EDIT_RESULT_TAG: {

		size_t isel = selmask.find_first(true, 0, selmask.size());
		//open result item dlg

		if (isel < selmask.size()) {
			if (!g_discogs->preview_modal_tag_dialog) {
				fb2k::newDialog <CPreviewLeadingTagDialog>(core_api::get_main_window(), m_tag_writer, isel, get_preview_mode(), HIWORD(conf.custom_font));
			}
			else {
				::SetFocus(g_discogs->preview_modal_tag_dialog->m_hWnd);
				g_discogs->preview_modal_tag_dialog->ReloadItem(core_api::get_main_window(), isel, get_preview_mode());
			}
		}
		return true;
	}
	case ID_PREVIEW_CMD_COPY: {

		HWND hwndList = uGetDlgItem(IDC_PREVIEW_LIST);
		auto isel = m_uilist.GetSingleSel();
		if (isel != ~0) {
			pfc::string8 out;
			m_uilist.GetSubItemText(isel, 1, out);
			ClipboardHelper::OpenScope scope; scope.Open(core_api::get_main_window(), true);
			ClipboardHelper::SetString(out);
		}
		return true;
	}
	case ID_URL_RELEASE:
	{
		url << "https://www.discogs.com/release/" << m_tag_writer->release->id;
		break;
	}
	case ID_URL_MASTER_RELEASE: {
		url << "https://www.discogs.com/master/" << m_tag_writer->release->master_id;
		break;
	}
	case ID_URL_ARTIST: {
		url << "https://www.discogs.com/artist/" << m_tag_writer->release->artists[0]->full_artist->id;
		break;
	}
	default: {
		if (cmd >= ID_URL_ARTISTS && cmd < ID_URL_ARTISTS + 100) {
			pfc::string8 artist_id = vartists.at(cmd - ID_URL_ARTISTS).first.c_str();
			url << "https://www.discogs.com/artist/" << artist_id;
		}
		else {
			return false;
		}

	}
	} //end switch

	if (url.get_length()) display_url(url);

	return false;
}

bool CPreviewTagsDialog::check_write_tags_status() {
	
	auto checkstate = m_tristate.GetState();
	bool skip_art = checkstate == BST_CHECKED;

	bool bres = m_tag_writer->will_modify;
	bres |= !skip_art && (CONF.save_album_art || CONF.save_artist_art || CONF.embed_album_art || CONF.embed_artist_art);
	return bres;
}

bool CPreviewTagsDialog::init_other_controls_and_results() {
	if (!m_tag_writer) {
		return false;
	}

	bool release_has_anv = m_tag_writer->release->has_anv();

	CheckDlgButton(IDC_CHK_REPLACE_ANV, release_has_anv && conf.replace_ANVs);
	auto n8_thumb_paths = Offline::get_thumbnail_cache_path_filenames(m_tag_writer->release->id, art_src::alb, LVSIL_NORMAL, true, 0);
	// Album art
	if (n8_thumb_paths.get_count()) {
		pfc::string8 n8_cache_path_small = n8_thumb_paths[0];
		std::filesystem::path os_path_small = std::filesystem::u8path(n8_cache_path_small.c_str());

		Bitmap bm(os_path_small.wstring().c_str());

		if (bm.GetHBITMAP(Color::Black, &m_preview_bitmap) == Ok) {
			uSendDlgItemMessage(IDC_ALBUM_ART, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)m_preview_bitmap);
		}
		else {
			log_msg("GdiPlus error (GetHBITMAP small album preview)");
		}
	}
	generate_tag_results(true);

	enable(true, true);
	return true;
}
void CPreviewTagsDialog::reset_tag_result_stats() {
	m_totalwrites = 0;
	m_totalupdates = 0;
}

void CPreviewTagsDialog::replace_tag_result(size_t item, tag_result_ptr result) {
	
	if (STR_EQUAL(m_tag_writer->tag_results[item]->tag_entry->tag_name, result->tag_entry->tag_name)) {
		m_tag_writer->tag_results[item]->value = result->value;
	}
}

void CPreviewTagsDialog::generate_tag_results(bool computestat) {
	ListView_DeleteAllItems(m_results_list);
	if (computestat) {
		m_vstats.clear();
		compute_stats();
	}
	SetResults(m_tag_writer, get_preview_mode(), m_vstats);
}

void CPreviewTagsDialog::tag_mappings_updated() {
	m_generating_tags = true;
	try {
		service_ptr_t<generate_tags_task> task = new service_impl_t<generate_tags_task>(this, m_tag_writer);
		task->start();
	}
	catch (locked_task_exception e)
	{
		log_msg(e.what());
	}
}

void CPreviewTagsDialog::GlobalReplace_ANV(bool state) {
	CONF.replace_ANVs = state;
	if (g_discogs->configuration_dialog) {
		SendMessage(g_discogs->configuration_dialog->m_hWnd, WM_CUSTOM_ANV_CHANGED, 0, state);
	}
}

LRESULT CPreviewTagsDialog::OnCheckReplaceANVs(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	
	bool local_replace_ANV = IsDlgButtonChecked(IDC_CHK_REPLACE_ANV);
	if (CONF.replace_ANVs != local_replace_ANV) {
		conf.replace_ANVs = local_replace_ANV;
		GlobalReplace_ANV(local_replace_ANV);
		tag_mappings_updated();
	} 
	return FALSE;
}

LRESULT CPreviewTagsDialog::OnCheckSkipArtwork(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	m_tristate.SetBistate();
	return FALSE;
}

LRESULT CPreviewTagsDialog::OnCheckAttachMappingPanel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	bool state = IsDlgButtonChecked(IDC_CHK_BIND_TAGS_DLG);
	conf.edit_tags_dlg_flags = state ?
		conf.edit_tags_dlg_flags | (FLG_TAGMAP_DLG_ATTACHED)
		: conf.edit_tags_dlg_flags & ~(FLG_TAGMAP_DLG_ATTACHED);

	return FALSE;
}

LRESULT CPreviewTagsDialog::OnButtonEditTagMappings(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (!g_discogs->tag_mappings_dialog) {
		g_discogs->tag_mappings_dialog = fb2k::newDialog<CTagMappingDialog>(core_api::get_main_window(), IDC_APPLY);
	}
	else
	{
		if (wID == IDC_BTN_TAG_MAPPINGS) {
			::SetFocus(g_discogs->tag_mappings_dialog->m_hWnd);
			::uPostMessage(g_discogs->tag_mappings_dialog->m_hWnd, WM_NEXTDLGCTL, (WPARAM)(HWND)GetDlgItem(IDC_APPLY), TRUE);
		}
	}
	return FALSE;
}

void CPreviewTagsDialog::set_preview_mode(PreView mode) {
	CheckRadioButton(IDC_VIEW_NORMAL, IDC_VIEW_DEBUG, mode == PreView::Normal ? IDC_VIEW_NORMAL :
		mode == PreView::Diff ? IDC_VIEW_DIFFERENCE :
		mode == PreView::Original ? IDC_VIEW_ORIGINAL :
		IDC_VIEW_DEBUG);
}

PreView CPreviewTagsDialog::get_preview_mode() {
	return IsDlgButtonChecked(IDC_VIEW_NORMAL) ? PreView::Normal :
		IsDlgButtonChecked(IDC_VIEW_DIFFERENCE) ? PreView::Diff :
		IsDlgButtonChecked(IDC_VIEW_ORIGINAL) ? PreView::Original :
		PreView::Dbg;
}

LRESULT CPreviewTagsDialog::OnChangePreviewMode(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int top = ListView_GetTopIndex(m_results_list);
	int sel = ListView_GetSelectionMark(m_results_list);

	SendMessage(m_results_list, WM_SETREDRAW, FALSE, 0);
	generate_tag_results(false);

	SendMessage(m_results_list, LVM_ENSUREVISIBLE,
		SendMessage(m_results_list, LVM_GETITEMCOUNT, 0, 0) - 1, TRUE);	//move to end
	SendMessage(m_results_list, LVM_ENSUREVISIBLE, top, TRUE);			//move to top

	ListView_SetItemState(m_results_list, sel, LVNI_SELECTED | LVNI_FOCUSED, LVNI_SELECTED | LVNI_FOCUSED);
	SendMessage(m_results_list, WM_SETREDRAW, TRUE, 0);

	m_uilist.Invalidate(1);
	
	return FALSE;
}

LRESULT CPreviewTagsDialog::OnButtonWriteTags(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	
	if (m_uilist.TableEdit_IsActive()) {
		m_uilist.TableEdit_Abort(true);
		m_uilist.UpdateItem(m_uilist.GetSingleSel());
	}

	pushcfg();

	if (m_write_only_selected) {
		m_write_only_selected = false;
		m_tag_writer->tag_results_mask = m_uilist.GetSelectionMask();
		m_tag_writer->tag_results_mask_mode = get_preview_mode();
	}
	else {
		m_tag_writer->tag_results_mask = {};
		m_tag_writer->tag_results_mask_mode = PreView::Undef;
	}

	try {
		service_ptr_t<write_tags_task> task = new service_impl_t<write_tags_task>(m_tag_writer);
		task->start();
	}
	catch (locked_task_exception e)
	{
		log_msg(e.what());
	}

	if (!(CONF.tag_save_flags & TAGSAVE_PREVIEW_STICKY_FLAG)) {
		destroy_all();
	}
	return TRUE;
}

void CPreviewTagsDialog::cb_generate_tags() {
	m_generating_tags = false;
	generate_tag_results(true);
	enable(true, true);
}

LRESULT CPreviewTagsDialog::OnButtonBack(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	go_back();
	return TRUE;
}

void CPreviewTagsDialog::pushcfg() {
	if (build_current_cfg()) {
		CONF.save(CConf::cfgFilter::PREVIEW, conf);
		CONF.save(CConf::cfgFilter::TRACK, conf, CFG_ALBUM_ART_SKIP_DEFAULT_CUST);
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
	if (m_uilist.TableEdit_IsActive()) {
		m_uilist.TableEdit_Abort(false);
	}
	destroy_all();
	return TRUE;
}

LRESULT CPreviewTagsDialog::OnListDoubleClick(LPNMHDR lParam) {

	if (lParam->idFrom == IDC_PREVIEW_LIST) {
	
		NMITEMACTIVATE* info = reinterpret_cast<NMITEMACTIVATE*>(lParam);

		if (info->iItem != -1 && info->iSubItem != 1) {
			const int isel = m_uilist.GetSingleSel();
			const int icount = m_uilist.GetItemCount();

			bit_array_bittable selmask(icount);
			selmask.set(isel, true);

			context_menu_switch(info->hdr.hwndFrom, info->ptAction, ID_PREVIEW_CMD_EDIT_RESULT_TAG, selmask, {});
		}
	}
	return FALSE;
}

bool CPreviewTagsDialog::delete_selection() {

	const int isel = m_uilist.GetSingleSel();
	if (!is_result_editable(isel)) {
		return false;
	}

	ListView_DeleteItem(m_results_list, isel);
	erase(m_tag_writer->tag_results, isel);

	int total = ListView_GetItemCount(m_results_list);
	if (isel < total) {
		m_uilist.SelectSingle(isel);
	}
	else if (total > 0) {
		m_uilist.SelectSingle(total-1);
	}
	return true;
}

LRESULT CPreviewTagsDialog::OnListKeyDown(LPNMHDR lParam) {
	
	NMLVKEYDOWN * info = reinterpret_cast<NMLVKEYDOWN*>(lParam);

	switch (info->wVKey) {
	case VK_DELETE:
		delete_selection();
		break;
	case VK_F2:	{
		int isel = m_uilist.GetSingleSel();

		if (this->is_result_editable(isel)) {
			m_uilist.TableEdit_Start(isel, 1);
		}
		break;
	}
	}
	return FALSE;
}

void CPreviewTagsDialog::compute_stats() {
	reset_tag_result_stats();
	compute_stats_track_map();
}

void CPreviewTagsDialog::compute_stats_track_map() {

	int createdtags = 0;
	int updatedtags = 0;

	m_vstats.clear();

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
				if (!r_approved) {
					bool usr_approved = false;
					if (old_value_cv_length == 0 && entry->enable_write) {
						usr_approved = true;
						subwrites++; subskips > 0 ? subskips-- : 0;
					} else if (entry->enable_update && old_value_cv_length != 0) {
						usr_approved = true;
						subupdates++; subskips > 0 ? subskips-- : 0;
					}
					if (usr_approved) {
						m_tag_writer->will_modify = true;
						tag_result_ptr usr_res = m_tag_writer->tag_results[walk_tag];
						usr_res->r_usr_approved.resize(walk_tk + 1); usr_res->r_usr_approved.set(walk_tk, usr_approved);
						usr_res->r_usr_modded.resize(walk_tk + 1); usr_res->r_usr_modded.set(walk_tk, usr_approved);
					}
				}
				else {
					tag_result_ptr usr_res = m_tag_writer->tag_results[walk_tag];
					usr_res->r_usr_modded.resize(walk_tk + 1); usr_res->r_usr_modded.set(walk_tk, false);
				}

			}
			else {

				if (r_approved) {
					tag_result_ptr usr_res = m_tag_writer->tag_results[walk_tag];
					usr_res->r_usr_approved.resize(walk_tk + 1); usr_res->r_usr_approved.set(walk_tk, false);
					usr_res->r_usr_modded.resize(walk_tk + 1); usr_res->r_usr_modded.set(walk_tk, true);

				
				
				}
				else {
					tag_result_ptr usr_res = m_tag_writer->tag_results[walk_tag];
					usr_res->r_usr_modded.resize(walk_tk + 1); usr_res->r_usr_modded.set(walk_tk, false);
				}
				

				//no need to update old val = new val for this tag/track
				stats.totalequal++;
			}

			if (subwrites) {
				stats.totalsubwrites += subwrites;
				stats.m_totalwrites++;
				subwrites = 0;
			}
			if (subupdates) {
				stats.totalsubupdates += subupdates;
				stats.m_totalupdates++;
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

		if (before_tag_loop_stats.m_totalwrites != stats.m_totalwrites)
			createdtags++;
		if (before_tag_loop_stats.m_totalupdates != stats.m_totalupdates)
			updatedtags++;

		m_vstats.emplace_back(stats);

	} // tag loop
}

LRESULT CPreviewTagsDialog::OnCustomDraw(int idCtrl, LPNMHDR lParam, BOOL& bHandled) {

	if (m_generating_tags) {
		return CDRF_DODEFAULT;
	}
	LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;
	int pos = (int)lplvcd->nmcd.dwItemSpec;
	int sub_item;
	bool bresults = m_tag_writer->tag_results.size() > 0;
	const tag_mapping_entry* entry = bresults ?
		m_tag_writer->tag_results[pos]->tag_entry: nullptr;

	switch (lplvcd->nmcd.dwDrawStage) {

	case CDDS_PREPAINT: {
		auto ctrl = uGetDlgItem(IDC_BTN_WRITE_TAGS);
		ctrl.EnableWindow(check_write_tags_status());
		ctrl = uGetDlgItem(IDC_CHK_REPLACE_ANV);
		ctrl.EnableWindow(bresults && m_tag_writer->release->has_anv());
		ctrl = uGetDlgItem(IDC_VIEW_NORMAL);
		ctrl.EnableWindow(bresults);
		ctrl = uGetDlgItem(IDC_VIEW_DIFFERENCE);
		ctrl.EnableWindow(bresults);
		ctrl = uGetDlgItem(IDC_VIEW_ORIGINAL);
		ctrl.EnableWindow(bresults);
		ctrl = uGetDlgItem(IDC_VIEW_DEBUG);
		ctrl.EnableWindow(bresults);
		return CDRF_NOTIFYITEMDRAW;
	}

	case CDDS_ITEMPREPAINT:
		return CDRF_NOTIFYSUBITEMDRAW;

	case CDDS_SUBITEM | CDDS_ITEMPREPAINT: {
		sub_item = lplvcd->iSubItem;
		if (entry && (sub_item == 0 || sub_item == 1)) {
			bool bresview = get_preview_mode() == PreView::Normal;
			bool bchgdiscarded = false && !m_tag_writer->tag_results[pos]->result_approved &&
				m_tag_writer->tag_results[pos]->changed;

			size_t c_usr_modded = m_tag_writer->tag_results[pos]->r_usr_modded.size();

			size_t c_usr_approved = m_tag_writer->tag_results[pos]->r_usr_approved.size();
			size_t first_usr_modded = m_tag_writer->tag_results[pos]->r_usr_modded.find_first(true, 0, c_usr_modded);
			size_t first_usr_approved = m_tag_writer->tag_results[pos]->r_usr_approved.find_first(true, 0, c_usr_approved);
			//bool this_usr_modded = m_tag_writer->tag_results[pos]->r_usr_approved[];

			if (m_tag_writer->tag_results[pos]->r_usr_modded.find_first(true, 0, c_usr_modded) < c_usr_modded) {
				lplvcd->clrText = CHANGE_USR_APPROVED_RGB;
				return CDRF_NEWFONT;
			}

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
		if (entry && m_cfg_bshow_stats &&
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

	HWND ctrlWriteTags = GetDlgItem(IDC_BTN_WRITE_TAGS);
	::uEnableWindow(ctrlWriteTags, !is_enabled ? FALSE : m_tag_writer->will_modify/*is_enabled*/);

	for (HWND walk = ::GetWindow(m_hWnd, GW_CHILD); walk != NULL; ) {
		HWND next = ::GetWindow(walk, GW_HWNDNEXT);
		if (next != ctrlWriteTags) 
			::uEnableWindow(next, is_enabled);
		walk = next;
	}

	if (g_discogs->preview_modal_tag_dialog) {
		CPreviewLeadingTagDialog* dlg = g_discogs->preview_modal_tag_dialog;
		for (HWND walk = ::GetWindow(dlg->m_hWnd, GW_CHILD); walk != NULL; ) {
			HWND next = ::GetWindow(walk, GW_HWNDNEXT);
			::uEnableWindow(next, is_enabled);
			walk = next;
		}
	}
}

void CPreviewTagsDialog::destroy_all() {

	if (g_discogs->track_matching_dialog) {
		CTrackMatchingDialog* dlg = g_discogs->track_matching_dialog;

		HWND hwndTri = ::GetDlgItem(dlg->m_hWnd, IDC_CHK_SKIP_ARTWORK);
		bool btri = m_tristate.IsTri();
		bool benabled = ::IsWindowEnabled(hwndTri);
		auto state = m_tristate.GetState();
		dlg->set_tristate(btri, state, benabled);

		dlg->destroy_all();
	}
	destroy();
}

void CPreviewTagsDialog::go_back() {
	if (m_uilist.TableEdit_IsActive()) {
		m_uilist.TableEdit_Abort(true);
	}
	destroy();

	// back to track matching

	if (g_discogs->track_matching_dialog) {

		CTrackMatchingDialog* dlg = g_discogs->track_matching_dialog;
		HWND hwndButton = ::GetDlgItem(dlg->m_hWnd, IDC_BTN_PREVIEW_TAGS);
		::PostMessage(dlg->m_hWnd, WM_NEXTDLGCTL, (WPARAM)hwndButton, TRUE);

		dlg->show();
	}
}

void CPreviewTagsDialog::reset_default_columns(bool breset, bool bshowstats) {

	cfg_preview_column_map::iterator it;

	auto ccols = m_uilist.GetColumnCount();

	if (breset && !bshowstats && ccols > COL_STATS_FIRST_COL && conf.preview_tags_dialog_w_width != 0) {
	
		int acc_stats_width = 0;

		for (int walk_stat_ndx = COL_STATS_FIRST_COL; walk_stat_ndx < COL_STATS_FIRST_COL + COL_STAT_NCOLS; walk_stat_ndx++) {

			if (walk_stat_ndx < ccols) {

				cfg_listview.colmap.at(walk_stat_ndx).enabled = false;
				cfg_listview.colmap.at(walk_stat_ndx).width = (int)m_uilist.GetColumnWidthF(walk_stat_ndx);
				acc_stats_width += cfg_listview.colmap.at(walk_stat_ndx).width;
			}
		}
		cfg_listview.colmap.at(1).width += acc_stats_width;
	}

	fix_sorted_icol_map(breset, bshowstats);
	m_uilist.DeleteColumns(pfc::bit_array_true());

	for (auto walk_subitem :  m_vcol_data_subitems) {

		auto & walk_cfg = cfg_listview.colmap.at(walk_subitem.first);

		if (breset) {

			auto dpiX = QueryScreenDPIEx(m_hWnd).cx;
			CRect rc;	m_uilist.GetClientRect(&rc);
			auto rcwidth = rc.Width();
			rcwidth -= GetSystemMetrics(SM_CXVSCROLL);

			int fw = MulDiv(rcwidth, dpiX, USER_DEFAULT_SCREEN_DPI);
			int c0 =  fw / 3; int c1 = fw / 3 * 2;

			if (walk_cfg.enabled) {
				
				if (walk_cfg.icol == 0) walk_cfg.width = c0;
				if (walk_cfg.icol == 1) walk_cfg.width = c1;

				//stat cols:
				if (walk_cfg.icol >= COL_STATS_FIRST_COL) {

					int def_fieldwidth = cfg_listview.colmap.at(0).width;

					CRect rc;
					m_uilist.GetClientRect(&rc);
					int sbwidth = GetSystemMetrics(SM_CXVSCROLL);

					int delta = rc.Width() - (2 * sbwidth * COL_STAT_NCOLS) - sbwidth;
					delta = MulDiv(delta, dpiX, USER_DEFAULT_SCREEN_DPI);
					delta -= def_fieldwidth;

					if (delta < 0) {

						m_uilist.ResizeColumn(1, delta, 0);
						m_uilist.ResizeColumn(0, def_fieldwidth, 0);
					}
					else {
						auto kk = m_uilist.GetColumnCount();
						m_uilist.ResizeColumn(1, delta, 0);
					}
					walk_cfg.width = MulDiv(2 * sbwidth, dpiX, USER_DEFAULT_SCREEN_DPI);
				}
				auto kk = m_uilist.GetColumnCount();
				m_uilist.AddColumnEx(walk_cfg.name, walk_cfg.width, LVCFMT_LEFT);
			}
		}
		else {
			if (walk_cfg.default || (walk_cfg.enabled && bshowstats && walk_cfg.icol >= COL_STATS_FIRST_COL)) {
				//insert column
				m_uilist.AddColumnEx(walk_cfg.name, walk_cfg.width, LVCFMT_LEFT);
			}
		}
	}
}

void CPreviewTagsDialog::fix_sorted_icol_map(bool reset, bool bshowstats) {

	m_vcol_data_subitems.clear();

	for (auto & walk_col : cfg_listview.colmap)	{

		if (reset) {
			if (walk_col.second.default || (walk_col.second.icol >= COL_STATS_FIRST_COL && bshowstats)) {
				walk_col.second.enabled = true;
				m_vcol_data_subitems.push_back(std::make_pair(walk_col.first, walk_col.second.icol));
			}
			else {
				walk_col.second.enabled = false;
			}
		}
		else {
			//restore
			if (walk_col.second.default || walk_col.second.enabled) {
				m_vcol_data_subitems.push_back(std::make_pair(walk_col.first, walk_col.second.icol));
			}
			else {
				//..
			}
		}
	}

	std::sort(m_vcol_data_subitems.begin(), m_vcol_data_subitems.end(), sortByVal);
}

LRESULT CPreviewTagsDialog::OnCheckPreviewShowStats(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	
	bool old_bshow_stats = m_cfg_bshow_stats;
	m_cfg_bshow_stats = ::IsDlgButtonChecked(m_hWnd, IDC_CHK_PREV_DLG_SHOW_STATS);
	bool bshowstats_changed = old_bshow_stats != m_cfg_bshow_stats;

	// update layout
	reset_default_columns(bshowstats_changed, m_cfg_bshow_stats);

	generate_tag_results(/*compute stats*/true);

	return FALSE;
}

void CPreviewTagsDialog::showtitle() {
	if (CONF.awt_alt_mode()) { uSetWindowText(m_hWnd, "Preview Tags +"); }
	else { uSetWindowText(m_hWnd, "Preview Tags"); }
}
