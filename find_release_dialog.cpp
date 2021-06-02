#include "stdafx.h"
#include <CommCtrl.h>
#include <atlctrls.h>

#include "find_release_dialog.h"
#include "configuration_dialog.h"
#include "tasks.h"
#include "multiformat.h"
#include "sdk_helpers.h"

#include "resource.h"
#include "utils.h"

struct cfgcol {
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

using cfgColMap = std::map<std::int32_t, cfgcol>;

const int TOTAL_DEF_COLS = 10;
 
cfgcol column_configs[TOTAL_DEF_COLS] = {
	//#, ndx, icol, name, tf, width, celltype, factory, default, enabled, defuser
	{1, 1, 0,"Id", "%RELEASE_ID%", cfg_find_release_colummn_showid_width, 0x0000, 0, true, false, false, false}, //min 17.5f
	{2, 2, 0,"Title", "%RELEASE_TITLE%", 120.0f, 0x0000, 0, true, false, false, false},
	{3, 3, 0,"Label", "%RELEASE_SEARCH_LABELS%", 50.0f, 0x0000, 0, true, false, false, false},
	{4, 4, 0,"Major F", "%RELEASE_SEARCH_MAJOR_FORMATS%", 60.0f, 0x0000, 0, true, false, false, false},
	{5, 5, 0,"Format", "%RELEASE_SEARCH_FORMATS%", 100.0f, 0x0000, 0, true, false, false, false},
	{6, 6, 0,"CatNo", "%RELEASE_SEARCH_CATNOS%", 20.0f, 0x0000, 0, true,false, false, false},
	{7, 7, 0,"Year", "%RELEASE_YEAR%", 20.0f, 0x0000, 0, true, false, false, false},
	{8, 8, 0,"---Released", "%length%", 20.0f, 0x0000, 0, false, false, false, false},
	{9, 9, 0,"Country", "%RELEASE_COUNTRY%", 50.0f, 0x0000, 0, true, false, false, false},
	{100, 10, 0,"Master/Release", "", 200.0f, 0x0000, 0, true, true, false, false},
};

struct release_lv_def {
	bool autoscale = false;
	cfgColMap colmap = {};
	uint32_t flags = 0;
} cfg_lv;

static void defaultCfgCols(release_lv_def& cfg_out) {
	cfg_out.autoscale = false;
	cfg_out.colmap.clear();
	for (int it = 0; it < PFC_TABSIZE(column_configs); it++) {
		if (!column_configs[it].name.has_prefix("---")) {
			cfgcol tmp_cfgcol{
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

inline void CFindReleaseDialog::load_size() {

	int width = conf.find_release_dialog_width;
	int height = conf.find_release_dialog_height;
	
	CRect rcCfg(0,0, width, height);
	::CenterWindow(this->m_hWnd, rcCfg, core_api::get_main_window());

	CRect rcCli;
	::GetClientRect(release_list, &rcCli);
	
	ListView_SetColumnWidth(release_list, 0, rcCli.Width());
	ListView_SetColumnWidth(release_list, 1, 0);

}

void CFindReleaseDialog::pushcfg() {
	bool conf_changed = build_current_cfg();
	if (conf_changed) {
		CONF.save(new_conf::ConfFilter::CONF_FILTER_FIND, conf);
		CONF.load();
	}
}

inline bool CFindReleaseDialog::build_current_cfg() {
	bool bres = false;

	//check global settings
	conf.display_exact_matches = IsDlgButtonChecked(IDC_ONLY_EXACT_MATCHES_CHECK) != 0;

	bool local_display_exact_matches = IsDlgButtonChecked(IDC_ONLY_EXACT_MATCHES_CHECK);
	if (CONF.display_exact_matches != conf.display_exact_matches &&
		conf.display_exact_matches != local_display_exact_matches) {

		conf.display_exact_matches = local_display_exact_matches;
		bres = bres || true;
	}

	//get current ui dimensions

	CRect rcDlg;
	GetClientRect(&rcDlg);

	int dlgwidth = rcDlg.Width();
	int dlgheight = rcDlg.Height();

	//dlg size

	if (dlgheight != conf.find_release_dialog_height || dlgwidth != conf.find_release_dialog_width) {
		conf.find_release_dialog_width = dlgwidth;
		conf.find_release_dialog_height = dlgheight;
		bres = bres || true;
	}

	//show id column
	const bool benabled = uButton_GetCheck(m_hWnd, IDC_CHECK_RELEASE_SHOW_ID);
	if (benabled != conf.find_release_dialog_show_id) {
		conf.find_release_dialog_show_id = benabled;
		bres = bres || true;
	}
	
	return bres;
}

LRESULT CFindReleaseDialog::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	pushcfg();
	return 0;
}

LRESULT CFindReleaseDialog::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	pfc::string8 text;
	uGetWindowText(release_url_edit, text);
	pfc::string8 release_id = text.get_ptr();
	release_id = trim(release_id);
	extract_id_from_url(release_id);
	extract_release_from_link(release_id);
	if (release_id.get_length() != 0) {
		on_write_tags(release_id);
	}
	return TRUE;
}

LRESULT CFindReleaseDialog::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	destroy();
	return TRUE;
}

CFindReleaseDialog::~CFindReleaseDialog() {

	g_discogs->find_release_dialog = nullptr;

bool OAuthCheck(foo_discogs_conf conf) {
	if(!g_discogs->gave_oauth_warning && (!conf.oauth_token.get_length() || !conf.oauth_token_secret.get_length())) {

		g_discogs->gave_oauth_warning = true;
		if (!g_discogs->configuration_dialog) {
			static_api_ptr_t<ui_control>()->show_preferences(guid_pref_page);
		}
		g_discogs->configuration_dialog->show_oauth_msg(
			"OAuth is required to use the Discogs API.\n Please configure OAuth.",
			true);

		::SetFocus(g_discogs->configuration_dialog->m_hWnd);

		return false;
	}
	else 
	{
		return true;
	}
}

LRESULT CFindReleaseDialog::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {

	conf = CONF;

	defaultCfgCols(cfg_lv);

	hlcolor = (COLORREF)::GetSysColor(COLOR_HIGHLIGHT);
	hlfrcolor = (COLORREF)::GetSysColor(COLOR_HIGHLIGHTTEXT);
	htcolor = (COLORREF)::GetSysColor(COLOR_HOTLIGHT);

	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(icex);
	icex.dwICC = ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx(&icex);

	search_edit = GetDlgItem(IDC_SEARCH_EDIT);
	filter_edit = GetDlgItem(IDC_FILTER_EDIT);
	artist_list = GetDlgItem(IDC_ARTIST_LIST);
	release_list = GetDlgItem(IDC_RELEASE_LIST);
	release_url_edit = GetDlgItem(IDC_RELEASE_URL_TEXT);
	//uSendMessage(release_list, LB_SETHORIZONTALEXTENT, 600, 0);


	ListView_SetExtendedListViewStyle(artist_list, LVS_EX_FULLROWSELECT);
	ListView_SetExtendedListViewStyleEx(artist_list,
		LVS_EX_COLUMNOVERFLOW | LVS_EX_FLATSB /*| LVS_EX_HEADERDRAGDROP*/ | LVS_EX_GRIDLINES | LVS_EX_AUTOSIZECOLUMNS | LVS_EX_COLUMNSNAPPOINTS /*| LVS_EX_BORDERSELECT| LVS_EX_DOUBLEBUFFER*/,
		LVS_EX_COLUMNOVERFLOW | LVS_EX_FLATSB /*| LVS_EX_HEADERDRAGDROP*/ | LVS_EX_GRIDLINES | LVS_EX_AUTOSIZECOLUMNS | LVS_EX_COLUMNSNAPPOINTS /*| LVS_EX_BORDERSELECT*/);

	ListView_SetExtendedListViewStyle(release_list, LVS_EX_FULLROWSELECT );
	ListView_SetExtendedListViewStyleEx(release_list,
		LVS_EX_COLUMNOVERFLOW | LVS_EX_FLATSB /*| LVS_EX_HEADERDRAGDROP*/ | LVS_EX_GRIDLINES | LVS_EX_AUTOSIZECOLUMNS | LVS_EX_COLUMNSNAPPOINTS /*| LVS_EX_BORDERSELECT| LVS_EX_DOUBLEBUFFER*/,
		LVS_EX_COLUMNOVERFLOW | LVS_EX_FLATSB /*| LVS_EX_HEADERDRAGDROP*/ | LVS_EX_GRIDLINES | LVS_EX_AUTOSIZECOLUMNS | LVS_EX_COLUMNSNAPPOINTS /*| LVS_EX_BORDERSELECT*/);
	CheckDlgButton(IDC_ONLY_EXACT_MATCHES_CHECK, conf.display_exact_matches);

	CRect rcartists;
	::GetWindowRect(artist_list, &rcartists);
	int icol = listview_helper::fr_insert_column(artist_list, 0,
		"Artist", rcartists.Width(), LVCFMT_LEFT);

	ListView_SetOutlineColor(release_list, hlfrcolor);
	
	cewb_artist_search.SubclassWindow(search_edit);
	cewb_artist_search.SetEnterEscHandlers();
	cewb_release_filter.SubclassWindow(filter_edit);
	cewb_release_filter.SetEnterEscHandlers();

	reset_default_columns(true);

	init_tracker();

	file_info_impl finfo;
	metadb_handle_ptr item = items[0];
	item->get_info(finfo);

	const char* filter = finfo.meta_get("ALBUM", 0);

	m_results_filter = filter ? filter : "";

	// TODO: make formatting strings out of these
	const char* artist = finfo.meta_get("ARTIST", 0) ? finfo.meta_get("ARTIST", 0) : finfo.meta_get("ALBUM ARTIST", 0);
	
	if (artist) {
		//artist search edit
		uSetWindowText(search_edit, artist);

		if (conf.enable_autosearch && (_idtracer.artist || artist)) {

			search_artist(false, artist); //_idtracker.artist_id will be used if available

		}
		else {

			if (_idtracer.release && _idtracer.release_id != -1) {
#ifndef DEBUG
				uSetWindowText(release_url_edit, 
					std::to_string(_idtracer.release_id).c_str());
#endif
			}
		}
	}

	DlgResize_Init(mygripp.grip, true);
	load_size();

	if (conf.find_release_dialog_show_id) {
		//todo: revise hacky
		uButton_SetCheck(m_hWnd, IDC_CHECK_RELEASE_SHOW_ID, true);

		BOOL bdummy = false;
		OnCheckReleasesShowId(0, 0, NULL, bdummy);
	}

	CRect rc;
	//::GetClientRect(m_hWnd, rc);
	//DlgResize_UpdateLayout(rc.Width(), rc.Height());
	modeless_dialog_manager::g_add(m_hWnd);
	
	if (OAuthCheck(conf)) {

		if (conf.skip_find_release_dialog_if_ided && _idtracer.release) {

			on_write_tags(pfc::toString(_idtracer.release_id).c_str());

		}
		else {

			if (_idtracer.release && _idtracer.release_id != pfc_infinite)
				uSetWindowText(release_url_edit, pfc::toString(_idtracer.release_id).c_str());
			
			show();

			m_bNoEnChange = true;

			if (!_idtracer.amr) {
				uSetWindowText(filter_edit, filter ? filter : "");
			}

			m_bNoEnChange = false;

			UINT default = 0;

			if (_idtracer.amr || _idtracer.release) {
				default = IDC_PROCESS_RELEASE_BUTTON;
			}
			else if (!artist) {
				default = IDC_SEARCH_EDIT;
			}
			else if (artist) {
				// default = 0;
			}

			if (default) {
				uSendMessage(m_hWnd, WM_NEXTDLGCTL, (WPARAM)(HWND)GetDlgItem(default), TRUE);
			}
			else {
				uSendMessage(m_hWnd, WM_NEXTDLGCTL, (WPARAM)(HWND)cewb_release_filter, TRUE);
			}
		}
	}

	return FALSE;
}

void CFindReleaseDialog::fill_artist_list(bool force_exact, updRelSrc updsrc) {

	HWND exactcheck = uGetDlgItem(IDC_ONLY_EXACT_MATCHES_CHECK);
	
	bool from_search = updsrc != updRelSrc::Undef && updsrc != updRelSrc::ArtistList;
	
	if (from_search) {
		m_artist_index = pfc_infinite;

		ListView_DeleteAllItems(artist_list);
		ListView_DeleteAllItems(release_list);

		::EnableWindow(exactcheck, true);

		find_release_artists = artist_exact_matches;

		bool exact_matches = IsDlgButtonChecked(IDC_ONLY_EXACT_MATCHES_CHECK) != 0;

		if (!exact_matches) {
			find_release_artists.append(artist_other_matches);
		}

		int artists_index = 0, list_index = 0;
		for (size_t i = 0; i < find_release_artists.get_size(); i++) {
			pfc::string8 name;
			name << find_release_artists[i]->name;

			_idtracer.artist_check(find_release_artists[i]->id, i);
			bool isdropindex = (list_index == _idtracer.artist_index);
			listview_helper::fr_insert_item(
				artist_list, list_index, !_idtracer.artist_lv_set && isdropindex, name.get_ptr(), artists_index);
			if (isdropindex) _idtracer.release_lv_set = true;

			artists_index++;
			list_index++;
		}
		if (find_release_artists.get_size()) {
			ListView_SetItemState(artist_list, 0, LVIS_SELECTED, LVIS_SELECTED);
			m_artist_index = 0;
		}
	}
	else if (updsrc == updRelSrc::Undef) {
	
		::EnableWindow(exactcheck, false);
		pfc::string8 name;
		name << find_release_artist->name;

		_idtracer.artist_check(find_release_artist->id, 0);

		listview_helper::insert_item(artist_list, 0, name.get_ptr(), (LPARAM)0);
		if (!_idtracer.release) {

			find_release_artists.append_single(std::move(find_release_artist));
			set_item_param(artist_list, 0, 0, (LPARAM)0);			
			ListView_SetItemState(artist_list, 0, LVIS_SELECTED, LVIS_SELECTED);
			
			m_artist_index = 0;
		}
	}
	else if (updsrc == updRelSrc::ArtistList) {
		::EnableWindow(exactcheck, true);

	}
	
}

void CFindReleaseDialog::on_get_artist_done(updRelSrc updsrc, Artist_ptr &artist) {

	find_release_artist = artist;
	fill_artist_list(true, updsrc);

	hook = std::make_shared<titleformat_hook_impl_multiformat>(&find_release_artist);

	pfc::string8 buffer;
	file_info_impl finfo;	metadb_handle_ptr item = items[0];
	item->get_info(finfo);

	const char* album = finfo.meta_get("ALBUM", 0);
	pfc::string8 filter(album? album : "");

	if (_idtracer.amr) {
		init_tracker_i("", "", false, true);
	
		bool albumdone = false;
		pfc::string8 mtitle = _idtracer.master_i  != pfc_infinite? find_release_artist->master_releases[_idtracer.master_i]->title : "";
		pfc::string8 rtitle = _idtracer.release_i == pfc_infinite ? ""
			:	_idtracer.master_id == pfc_infinite ? find_release_artist->releases[_idtracer.release_i]->title
			: find_release_artist->master_releases[_idtracer.master_i]->sub_releases[_idtracer.release_i]->title;


		if (stricmp_utf8(filter, mtitle)) {
			pfc::string8 buffer(filter);
			if (buffer.has_prefix(mtitle)) {
				filter.set_string(mtitle);
			}
			else {
				filter = "";
			}
		}

		init_tracker_i(mtitle, rtitle, false, true);
	}

	m_results_filter = filter;
	m_vec_lv_items.clear();

	update_releases(m_results_filter, updsrc, true);

	//selection

	m_bNoEnChange = true;
		uSetWindowText(filter_edit, filter);
	m_bNoEnChange = false;

	int list_count = ListView_GetItemCount(release_list);

	if (list_count) {
		if (_idtracer.amr) {
			if (_idtracer.release_index != -1)
				on_release_selected(_idtracer.release_index);
			else if (_idtracer.master_index != -1)
				on_release_selected(_idtracer.master_index);
			else
				//spawn
				on_release_selected(0);
		}
		else {
			if (_idtracer.release_i != pfc_infinite && _idtracer.release_index != -1) {
				on_release_selected(_idtracer.release_index);
			}
			else {
				//
			}
		}
	}
}

void CFindReleaseDialog::enable(bool is_enabled) {
	::uEnableWindow(GetDlgItem(IDC_PROCESS_RELEASE_BUTTON), is_enabled);
	::uEnableWindow(GetDlgItem(IDCANCEL), is_enabled);
	::uEnableWindow(GetDlgItem(IDC_SEARCH_BUTTON), is_enabled);
}

void CFindReleaseDialog::OnFinalMessage(HWND /*hWnd*/) {

	modeless_dialog_manager::g_remove(m_hWnd);
	delete this;
}

void CFindReleaseDialog::on_search_artist_done(pfc::array_t<Artist_ptr> &p_artist_exact_matches, const pfc::array_t<Artist_ptr> &p_artist_other_matches) {
	
	artist_exact_matches = p_artist_exact_matches;
	artist_other_matches = p_artist_other_matches;
	if (!artist_exact_matches.get_size() && artist_other_matches.get_size()) {
		CheckDlgButton(IDC_ONLY_EXACT_MATCHES_CHECK, false);
	}
	//spawn
	fill_artist_list(_idtracer.amr, updRelSrc::Artist);

}

void CFindReleaseDialog::set_item_param(HWND list, int list_index, int col, LPARAM in_p) {
	LVITEM lvitem = { 0 };
	lvitem.mask = LVIF_PARAM;
	lvitem.lParam = in_p;
	lvitem.iItem = list_index;
	lvitem.iSubItem = col;
	int res = ListView_SetItem(list, &lvitem);
}

void CFindReleaseDialog::get_item_param(HWND list, int list_index, int col, LPARAM& out_p) {
	LVITEM lvitem = { 0 };
	lvitem.mask = LVIF_PARAM;
	lvitem.iItem = list_index;
	lvitem.iSubItem = col;
	int res = ListView_GetItem(list, &lvitem);
	if (res)
		out_p = MAKELPARAM(LOWORD(lvitem.lParam), HIWORD(lvitem.lParam));
	else {
		mounted_param not_found = { -1, -1, false, false };
		out_p = get_mounted_lparam(not_found);
	}
}

//void CFindReleaseDialog::set_lvstate_dropId(LVITEM& item, bool val) {
//		item.mask |= LVIF_STATE;
//		item.stateMask = LVIS_OVERLAYMASK;
//		item.state = INDEXTOOVERLAYMASK(val? 8 : 0);
//}

bool CFindReleaseDialog::get_lvstate_tracker(HWND wnd_lv, t_size list_index) {
	UINT img_state = ListView_GetItemState(wnd_lv, list_index, LVIS_OVERLAYMASK) & LVIS_OVERLAYMASK;
	return img_state == INDEXTOOVERLAYMASK(8);
}

void CFindReleaseDialog::set_lvstate_expanded(t_size item_index, bool val) {
	ListView_SetItemState(release_list, item_index,
		INDEXTOSTATEIMAGEMASK(val ? 8 : 0), LVIS_STATEIMAGEMASK);
}

bool CFindReleaseDialog::get_lvstate_expanded(t_size list_index) {
	UINT img_state = ListView_GetItemState(release_list, list_index, LVIS_STATEIMAGEMASK) & LVIS_STATEIMAGEMASK;
	return img_state == INDEXTOSTATEIMAGEMASK(8);
}

void CFindReleaseDialog::insert_item(const pfc::string8 &item, int list_index, int item_data) {
	HWND release_list = uGetDlgItem(IDC_RELEASE_LIST);
	listview_helper::insert_item(release_list, list_index, item.c_str(), (LPARAM)item_data);
}

int CFindReleaseDialog::insert_item_row(row_col_data row_data,
		int list_index, int item_data, bool delete_on_enter) {

	int iret = pfc_infinite;
	//row node
	for (auto iter_column_data = row_data.col_data_list.begin(); iter_column_data != row_data.col_data_list.end(); iter_column_data++) {
		
		if (iter_column_data->first == 0) {

			bool is_lv_tracker = (list_index == _idtracer.release_index);

			iret = listview_helper::fr_insert_item(release_list, list_index,
					!_idtracer.release_lv_set && is_lv_tracker,
					iter_column_data->second,
					item_data);
			
			if (is_lv_tracker)
				_idtracer.release_lv_set = true;
		}
		else {

			listview_helper::fr_insert_item_subitem(release_list, list_index,
					iter_column_data->first,
					iter_column_data->second,
					item_data);
		}
	}

	mounted_param myparam = get_mounted_param((LPARAM)item_data);
	if (delete_on_enter && myparam.is_master()) {
			m_vec_build_lv_items.emplace_back(std::pair<int, int>(row_data.id, 0));
	}
	
	return iret;
}

pfc::string8 CFindReleaseDialog::run_hook_columns(row_col_data& row_data, int item_data) {
	
	mounted_param myparam = get_mounted_param((LPARAM)item_data);

	pfc::string8 search_formatted;
	formatting_script column_script;

	formatting_script id_script = cfg_lv.colmap.at(0).titleformat;
	formatting_script id_script_main = "%MASTER_RELEASE_MAIN_RELEASE_ID%";

	row_data.col_data_list.clear();

	bool bmainrelease = false;
	bool runhook = true;

	for (unsigned int i = 0; i < vec_icol_subitems.size(); i++)
	{
		cfgcol walk_cfg = cfg_lv.colmap.at(vec_icol_subitems[i].first);

		if (myparam.is_master()) {
			CONF.search_master_format_string->run_hook(location, &info, hook.get(), search_formatted, nullptr);

			id_script = "%MASTER_RELEASE_ID%";
			
			if (!stricmp_utf8(walk_cfg.name, "Id")) {
				column_script = "%MASTER_RELEASE_ID%";
			}
			else if (!stricmp_utf8(walk_cfg.name, "Title")) {
				column_script = "'[master] '$join(%MASTER_RELEASE_TITLE%)";
			}
			else if (!stricmp_utf8(walk_cfg.name, "Year")) {
				column_script = "%MASTER_RELEASE_YEAR%";
			}
			else if (!stricmp_utf8(walk_cfg.name, "Id")) {
				column_script = walk_cfg.titleformat;
			}
			else if (!stricmp_utf8(walk_cfg.name, "Master/Release")) {
				column_script = conf.search_master_format_string;
			}
			else
			{
				runhook = false;
			}
		}
		else {

			if (myparam.is_release()) {
				CONF.search_master_sub_format_string->run_hook(location, &info, hook.get(), search_formatted, nullptr);

				//hook->set_release(&(find_release_artist->master_releases[master_index]->sub_releases[subrelease]));
				pfc::string8 main_release_id = find_release_artist->master_releases[myparam.master_ndx]->main_release->id;
				pfc::string8 this_release_id = find_release_artist->master_releases[myparam.master_ndx]->sub_releases[myparam.release_ndx]->id;

				bmainrelease = !stricmp_utf8(main_release_id, this_release_id);

				if (!stricmp_utf8(walk_cfg.name, "Title")) {
					pfc::string8 buffer("'  '");
					buffer << walk_cfg.titleformat;
					column_script = buffer;
				}
				else if (!stricmp_utf8(walk_cfg.name, "Master/Release")) {
					column_script = conf.search_master_sub_format_string;
				}
				else
					column_script = walk_cfg.titleformat;
			}
			else
			{
				CONF.search_release_format_string->run_hook(location, &info, hook.get(), search_formatted, nullptr);

				//hook->set_release(&(find_release_artist->releases[release_index]));
				if (!stricmp_utf8(walk_cfg.name, "Master/Release")) {
					column_script = conf.search_release_format_string;
				}
				else
					column_script = walk_cfg.titleformat;
			}
		}
		
		if (runhook) {
			pfc::string8 column_content;
			column_script->run_hook(location, &info, hook.get(), column_content, nullptr);
			if (bmainrelease) {
				//uReplaceString(buffer, column_content, pfc_infinite, "     ", 5, "  *  ", 5, false);
				column_content.replace_string("     ", "  * ", 0);
			}
			row_data.col_data_list.emplace_back(std::make_pair(walk_cfg.icol, column_content));
		}
	}

	pfc::string8 id;
	if (myparam.is_master()) {
		id_script->run_hook(location, &info, hook.get(), id, nullptr);
		row_data.id = atoi(id);
	}
	else {		
		id_script->run_hook(location, &info, hook.get(), id, nullptr);
		row_data.id = atoi(id);;
	}

	return search_formatted;
}

void CFindReleaseDialog::update_sorted_icol_map(bool reset) {

	cfgColMap& colvals = cfg_lv.colmap;
	cfgColMap::iterator it;

	vec_icol_subitems.clear();
	for (it = colvals.begin(); it != colvals.end(); it++)
	{
		bool bpush = false;
		if ((reset && it->second.default) || (!reset && it->second.enabled)) {
			vec_icol_subitems.push_back(std::make_pair(it->first, it->second.icol));
		}
	}
	std::sort(vec_icol_subitems.begin(), vec_icol_subitems.end(), sortByVal);

}

void CFindReleaseDialog::reset_default_columns(bool breset) {
	
	cfgColMap::iterator it;

	for (int i = 0; i < ListView_GetColumnCount(release_list); i++) {
		ListView_DeleteColumn(release_list, 0);
	}

	update_sorted_icol_map(breset);

	for (unsigned int i = 0; i < vec_icol_subitems.size(); i++)
	{
		cfgcol walk_cfg = cfg_lv.colmap.at(vec_icol_subitems[i].first);

		//insert column
		int icol = listview_helper::fr_insert_column(release_list, walk_cfg.ndx,
			walk_cfg.name, (unsigned int)walk_cfg.width, LVCFMT_LEFT);

		cfg_lv.colmap.at(vec_icol_subitems[i].first).enabled = true;
	}

}

bool tokenize_filter(pfc::string8 filter, pfc::array_t<pfc::string> & out_filter_words_lowercase) {
	pfc::array_t<pfc::string8> filter_words;
	tokenize(filter, " ", filter_words, true);

	for (size_t i = 0; i < filter_words.get_size(); i++) {
		out_filter_words_lowercase.append_single(pfc::string(filter_words[i].get_ptr()).toLower());
	}
	return out_filter_words_lowercase.get_count() > 0;
}

bool check_match(pfc::string8 str, pfc::string8 filter, pfc::array_t<pfc::string> filter_words_lowercase) {
	bool bres = true;
	if (filter.get_length()) {
		pfc::string item_lower = pfc::string(str.get_ptr()).toLower();
		for (size_t j = 0; j < filter_words_lowercase.get_count(); j++) {
			bres = item_lower.contains(filter_words_lowercase[j]);
			if (!bres) {
				break;
			}
		}
	}
	return bres;
}

bool CFindReleaseDialog::init_tracker() {

	file_info_impl finfo;
	metadb_handle_ptr item = items[0];
	item->get_info(finfo);

	pfc::string8 release_id, master_id, artist_id;
	bool release_id_unknown = true;
	bool master_id_unknown = true;
	bool artist_id_unknown = true;

	
	if (g_discogs->file_info_get_tag(item, finfo, TAG_RELEASE_ID, release_id)) {
		_idtracer.release_id = atoi(release_id);
		_idtracer.release = true;
		release_id_unknown = false;
	}
	if (g_discogs->file_info_get_tag(item, finfo, TAG_MASTER_RELEASE_ID, master_id)) {
		_idtracer.master_id = atoi(master_id);
		_idtracer.master = true;
		master_id_unknown = false;
	}
	if (g_discogs->file_info_get_tag(item, finfo, TAG_ARTIST_ID, artist_id)) {
		_idtracer.artist_id = atoi(artist_id);
		_idtracer.artist = true;
		artist_id_unknown = false;
	}

	if (!artist_id_unknown && !master_id_unknown && !release_id_unknown) {
		_idtracer.amr = true;
		_idtracer.release = true;
		_idtracer.master = true;
		_idtracer.artist = true;
	}

	bool drop_check = false;
	if (_idtracer.amr && _idtracer.release) {
		//amr ui
	}
	else {
		if (artist_id_unknown) {
			if (master_id_unknown && !release_id_unknown) drop_check = true;
			else if (!master_id_unknown) drop_check = false;
		}
		else {
			if (master_id_unknown && release_id_unknown) drop_check = true;
			else if (master_id_unknown && !release_id_unknown) drop_check = true;
			else if (!master_id_unknown && release_id_unknown) drop_check = false;
		}

		_idtracer.release = _idtracer.release && !release_id_unknown && drop_check;
		_idtracer.master = _idtracer.master && !master_id_unknown && drop_check;
	}

	return _idtracer.amr;
}

void CFindReleaseDialog::init_tracker_i(pfc::string8 filter_master, pfc::string8 filter_release, bool expanded, bool fast) {

	bool filtered = filter_master.get_length() > 0;
	bool matches_master = true;
	bool matches_release = true;
	_idtracer.master_i = pfc_infinite;
	_idtracer.release_i = pfc_infinite;

	int pos = -1;
	if (_idtracer.amr || _idtracer.release) {
		int master_ndx = 0; int release_ndx = 0; bool master_done = false; bool release_done = false;
		for (size_t walk = 0; walk < find_release_artist->search_order_master.get_size(); walk++) {
			if (master_done && release_done && fast) break;
			bool bmaster = find_release_artist->search_order_master[walk];
			if (bmaster) {
				MasterRelease_ptr mr_p = find_release_artist->master_releases[master_ndx];
				if (filtered) 
					matches_master = !stricmp_utf8(mr_p->title, filter_master);
				
				if (matches_master && atoi(find_release_artist->master_releases[master_ndx]->id) == _idtracer.master_id) {
					pos++;
					_idtracer.master_i = master_ndx;
					_idtracer.master_index = pos;
					
					master_done = true;
				}
				for (size_t j = 0; j < find_release_artist->master_releases[master_ndx]->sub_releases.size(); j++) {
					Release_ptr r_p = find_release_artist->master_releases[master_ndx]->sub_releases[j];
					if (filtered)
						matches_release = !stricmp_utf8(r_p->title, filter_release);
					
					if (matches_release && atoi(find_release_artist->master_releases[master_ndx]->sub_releases[j]->id) == _idtracer.release_id) {
						_idtracer.release_i = j;
						_idtracer.release_index = pos + j;
						if (!_idtracer.master) {
							_idtracer.master_i = master_ndx;
							_idtracer.master_index = pos;
							_idtracer.master = true;
						}
						release_done = true;
					}
				}
			}
			else {
				Release_ptr r_p = find_release_artist->releases[release_ndx];
				if (filtered)
					matches_release = !stricmp_utf8(r_p->title, filter_release);

				if (matches_release && atoi(find_release_artist->releases[release_ndx]->id) == _idtracer.release_id) {
					pos++;
					_idtracer.release_i = release_ndx;
					_idtracer.release_index = pos;
					_idtracer.master_i = pfc_infinite;
					_idtracer.master_index = -1;

					release_done = true; master_done = true;
				}
			}

			if (bmaster)
				master_ndx++;
			else
				release_ndx++;
		}
	}
}

void CFindReleaseDialog::init_filter_i(pfc::string8 filter_master, pfc::string8 filter_release,
		bool ontitle, bool expanded, bool fast) {

	m_vec_filter.clear();

	pfc::array_t<pfc::string> lcf_words;
	bool filtered = tokenize_filter(filter_master, lcf_words);



	int pos = -1;

	int master_ndx = 0; int release_ndx = 0; bool master_done = false; bool release_done = false;
	for (size_t walk = 0; walk < find_release_artist->search_order_master.get_size(); walk++) {

		bool matches_master = false;
		bool matches_release = false;

		if (master_done && release_done && fast) break;
		bool bmaster = find_release_artist->search_order_master[walk];
		if (bmaster) {
			MasterRelease_ptr mr_p = find_release_artist->master_releases[master_ndx];
			if (filtered) {
				if (ontitle)
					matches_master = check_match(mr_p->title, filter_master, lcf_words);
				else {
					row_col_data row_data{};
					hook->set_master(&mr_p);
					pfc::string8 item = run_hook_columns(row_data, (master_ndx << 16) | 9999);

					matches_master = check_match(item, filter_master, lcf_words);
				}
			}
			if (matches_master) {
				pos++;
				if (atoi(find_release_artist->master_releases[master_ndx]->id) == _idtracer.master_id) {
					_idtracer.master_index = pos;
				}

				m_vec_filter.emplace_back(std::pair<int,int>(master_ndx, pfc_infinite));

				master_done = true;
			}
			for (size_t j = 0; j < find_release_artist->master_releases[master_ndx]->sub_releases.size(); j++) {
				Release_ptr r_p = find_release_artist->master_releases[master_ndx]->sub_releases[j];
				if (filtered) {
					if (ontitle)					
						matches_release = check_match(r_p->title, filter_master, lcf_words);
					else {
						row_col_data row_data{};
						hook->set_release(&(find_release_artist->master_releases[master_ndx]->sub_releases[j]));
						pfc::string8 sub_item = run_hook_columns(row_data, (master_ndx << 16) | j);

						matches_release = check_match(sub_item, filter_master, lcf_words);
					}
				}
				if (matches_release) {
					pos++;
					if (atoi(find_release_artist->master_releases[master_ndx]->sub_releases[j]->id) == _idtracer.release_id) {
						_idtracer.release_index = pos + j;
					}

					m_vec_filter.emplace_back(std::pair<int, int>(master_ndx, j));

					release_done = true;
				}
			}
		}
		else {
			Release_ptr r_p = find_release_artist->releases[release_ndx];
			if (filtered) {
				if (ontitle)
					matches_release = check_match(r_p->title, filter_master, lcf_words);
				else {
					row_col_data row_data{};
					hook->set_release(&(find_release_artist->releases[release_ndx]));
					pfc::string8 item = run_hook_columns(row_data, (9999 << 16) | release_ndx);

					matches_release = check_match(item, filter_master, lcf_words);
				}
			}

			if (matches_release) {
				pos++;
				if (atoi(find_release_artist->releases[release_ndx]->id) == _idtracer.release_id) {
					_idtracer.release_index = pos;
					_idtracer.master_index = -1;
				}

				m_vec_filter.emplace_back(std::pair<int, int>(pfc_infinite, release_ndx));
				release_done = true; master_done = true;
			}
		}

		if (bmaster)
			master_ndx++;
		else
			release_ndx++;
	}
}

void CFindReleaseDialog::update_releases(const pfc::string8& filter, updRelSrc updsrc, bool init_expand) {

	init_tracker();

	int first_count = ListView_GetItemCount(release_list);
	t_size the_artist = !find_release_artist ? pfc_infinite : atoi(find_release_artist->id);
	
	bool artist_changed = m_release_list_artist_id != the_artist;
	bool delete_on_enter = updsrc == updRelSrc::Artist || updsrc == updRelSrc::ArtistList || artist_changed;
	
	if (updsrc == updRelSrc::Filter || delete_on_enter) {
		_idtracer.release_reset();
	}

	if (delete_on_enter) {
		ListView_DeleteAllItems(release_list);
		m_vec_build_lv_items.clear();
	}

	if (!find_release_artist) {
		return;
	}

	int list_index = 0;
	int master_index = 0, release_index = 0;

	pfc::array_t<pfc::string> lcf_words;
	bool filtered = tokenize_filter(filter, lcf_words);

	// TODO: cache all these to make filtering fast again
	try {
		for (size_t walk = 0; walk < find_release_artist->search_order_master.get_size(); walk++) {
			int item_data;
			row_col_data row_data{};
			bool is_master = find_release_artist->search_order_master[walk];
			pfc::string8 item; //master or release summary to match

			if (is_master) {

				hook->set_master(&(find_release_artist->master_releases[master_index]));
				mounted_param thisparam = { master_index,  -1, true, false };
				item_data = get_mounted_lparam(thisparam);
				item = run_hook_columns(row_data, item_data);

			}
			else {

				hook->set_release(&(find_release_artist->releases[release_index]));
				mounted_param thisparam = { -1,  release_index,  false, true };
				item_data = get_mounted_lparam(thisparam);
				item = run_hook_columns(row_data, item_data);

			}

			bool inserted = false; bool expanded = false;

			bool matches = filtered && check_match(item, filter, lcf_words);

			if (matches) {

				if (is_master)
					_idtracer.release_check(find_release_artist->master_releases[master_index]->id,	list_index,
							is_master, list_index, master_index);
				else
					_idtracer.release_check(find_release_artist->releases[release_index]->id, list_index,
							is_master, list_index, master_index);

				// MASTER OR NO MASTER RELEASES
				insert_item_row(row_data, list_index, item_data, delete_on_enter);

				if (init_expand && is_master) {
					int prev_expanded; int expanded = updsrc == updRelSrc::Filter ? 0 : 1;
					bool bnew = !get_node_expanded(row_data.id, prev_expanded);
					set_node_expanded(row_data.id, bnew ? expanded : prev_expanded, true);					
					set_lvstate_expanded(list_index, bnew ? expanded : prev_expanded);
				}

				list_index++;
				inserted = true;
			}

			if (is_master) {

				// SUBITEMS
				const MasterRelease_ptr* master_release = &(find_release_artist->master_releases[master_index]);

				for (size_t j = 0; j < master_release->get()->sub_releases.get_size(); j++) {
					
					mounted_param thisparam = { master_index, j, true, true };
					item_data = get_mounted_lparam(thisparam);
					
					hook->set_release(&(find_release_artist->master_releases[master_index]->sub_releases[j]));
					pfc::string8 sub_item = run_hook_columns(row_data, item_data);

					bool matches = filtered && check_match(sub_item, filter, lcf_words);

					if (matches) {

						int master_id = atoi(find_release_artist->master_releases[master_index]->id);
						auto parent = std::find_if(m_vec_build_lv_items.begin(), m_vec_build_lv_items.end(),
							[&](const std::pair<int, int>& e) { return e.first == master_id; });
						inserted = parent != m_vec_build_lv_items.end();
						
						if (!inserted) {
							PFC_ASSERT(false);
						}
						// SUBRELEASE
						if (init_expand && m_vec_build_lv_items.at(m_vec_build_lv_items.size() - 1).second == 1) {

							_idtracer.release_check(master_release->get()->sub_releases[j]->id, list_index,
									false, list_index - j, master_index );

							//insert
							insert_item_row(row_data, list_index, item_data, delete_on_enter);

							list_index++;
						}
					}
				}
			}

			if (is_master) {
				master_index++;
			}
			else {
				release_index++;
			}
		}
	}
	catch (foo_discogs_exception& e) {
		foo_discogs_exception ex;
		ex << "Error formating release list item [" << e.what() << "]";
		throw ex;
	}

	int list_count = ListView_GetItemCount(release_list);

	if (!delete_on_enter) {
		if (first_count < ListView_GetItemCount(release_list)) {

			for (int delwalk = 0; delwalk < first_count; delwalk++) {
				ListView_DeleteItem(release_list, ListView_GetItemCount(release_list) - 1);
			}
		}
	}
	else {
		m_vec_lv_items = m_vec_build_lv_items;
	}

	return;
}

void CFindReleaseDialog::update_releases_columns() {
	
	ListView_DeleteAllItems(release_list);
	m_vec_build_lv_items.clear();

	std::vector<std::pair<int, int>>::iterator it;
	t_size list_index = -1;
	t_size master_list_pos = pfc_infinite;
	t_size parent_master_expanded = pfc_infinite;

	_idtracer.release_reset();

	//DISPLAY FILTER RESULTS
	for (it = m_vec_filter.begin(); it != m_vec_filter.end(); it++) {
		
		list_index++;

		LPARAM lparam = MAKELPARAM(0, 0);
		int master_ndx = it->first;
		int release_ndx = it->second;
		bool bmaster = it->first != pfc_infinite;
		bool brelease = it->second != pfc_infinite;

		mounted_param myparam = { master_ndx, release_ndx, bmaster, brelease };
		lparam = get_mounted_lparam(myparam);

		pfc::string8 item;
		row_col_data row_data;

		int pos = list_index;
		master_list_pos = pfc_infinite;

		if (myparam.is_master()) {
			master_list_pos = pos;
			hook->set_master(&(find_release_artist->master_releases[myparam.master_ndx]));
		}
		else {
			master_list_pos = pfc_infinite;
			if (myparam.is_release()) {

				std::vector<std::pair<int, int>>::iterator it_parent;

				int master_id = atoi(find_release_artist->master_releases[myparam.master_ndx]->id);
				auto parent = std::find_if(m_vec_build_lv_items.begin(), m_vec_build_lv_items.end(),
					[&](const std::pair<int, int>& e) { return e.first == master_id; });

				if (parent != m_vec_build_lv_items.end()) {
					//PARENT FOUND
					parent_master_expanded = std::distance(m_vec_build_lv_items.begin(), parent);
					pos = list_index;
					int expanded;
					bool bfound = get_node_expanded(master_id, expanded);
					if (bfound) {
						set_node_expanded(master_id, expanded, true);
						set_lvstate_expanded(parent_master_expanded, expanded);					
					}
				}
				else {
					//INSERT NOT-MATCHING PARENT							
					hook->set_master(&(find_release_artist->master_releases[myparam.master_ndx]));
					mounted_param mp_master = { master_ndx, -1, true, false };
					item = run_hook_columns(row_data, get_mounted_lparam(mp_master));

					//insert row
					parent_master_expanded = insert_item_row(row_data, pos, get_mounted_lparam(mp_master), true);

					int expanded = 1;
					set_node_expanded(row_data.id, expanded, true);
					set_lvstate_expanded(pos, expanded);

					pos = parent_master_expanded + 1;
					master_list_pos = parent_master_expanded;
				}

				hook->set_master(&(find_release_artist->master_releases[myparam.master_ndx]));

				hook->set_release(&(find_release_artist->master_releases[myparam.master_ndx]->sub_releases[myparam.release_ndx]));
			}
			else
				hook->set_release(&(find_release_artist->releases[myparam.release_ndx]));
		}
		item = run_hook_columns(row_data, lparam);

		if (myparam.is_master())
			_idtracer.release_check(pfc::toString(row_data.id).c_str(), list_index,
				myparam.is_master(), master_list_pos, myparam.master_ndx);
		else
			_idtracer.release_check(pfc::toString(row_data.id).c_str(), pos,
				myparam.is_master(), master_list_pos, myparam.master_ndx);

		//insert row
		list_index = insert_item_row(row_data, pos, lparam, true);
	}
	m_vec_lv_items = m_vec_build_lv_items;
	ListView_RedrawItems(release_list, 0, ListView_GetItemCount(release_list));
	return;
}

void CFindReleaseDialog::expand_releases(const pfc::string8& filter, updRelSrc updsrc, t_size master_index, t_size master_list_pos) {

	pfc::array_t<pfc::string> lcf_words;
	bool filtered = tokenize_filter(filter, lcf_words);

	int list_index = master_list_pos + 1;
	
	int item_data;
	row_col_data row_data;

	try {
		int skipped = 0; 
		for (size_t j = 0; j < find_release_artist->master_releases[master_index]->sub_releases.get_size(); j++) {
			
			row_col_data row_data_subitem;
			hook->set_release(&(find_release_artist->master_releases[master_index]->sub_releases[j]));
			
			mounted_param thisparam = { master_index, j, true, true };
			item_data = get_mounted_lparam(thisparam);

			pfc::string8 sub_item = run_hook_columns(row_data, item_data);

			bool filter_match = filtered && check_match(sub_item, filter, lcf_words);

			if (filter_match) {
				int row_pos = list_index + j - skipped;
				_idtracer.release_check(find_release_artist->master_releases[master_index]->sub_releases[j]->id,
						row_pos, false, master_list_pos, master_index);
			
				//insert
				insert_item_row(row_data, row_pos, item_data, true);

			}
			else {
				skipped++;
			}
		}
	}
	catch (foo_discogs_exception& e) {
		foo_discogs_exception ex;
		ex << "Error formating release list subitem [" << e.what() << "]";
		throw ex;
	}
	return;
}

void CFindReleaseDialog::select_lv_release(int list_index) {

	if (list_index == -1) return;

	mounted_param myparam = get_mounted_param((t_size)list_index);
	if (myparam.is_master()) {

		int master_id = atoi(get_param_id_master(myparam));
		int toggle_state = -1;
		bool bnodeset = set_node_expanded(master_id, toggle_state, false);

		if (bnodeset)
			set_lvstate_expanded(list_index, toggle_state);
		else
			PFC_ASSERT(false);
	}

	on_release_selected(list_index);
}

void CFindReleaseDialog::get_mounted_param(mounted_param& pm, LPARAM lparam) {
	pm.bmaster = (pm.master_ndx != 9999);
	pm.brelease = (pm.release_ndx != 9999);
	pm.master_ndx = HIWORD(lparam);
	pm.release_ndx = LOWORD(lparam);
}

mounted_param CFindReleaseDialog::get_mounted_param(LPARAM lparam) {
	mounted_param mpres;
	if (lparam == 0) {
		lparam = lparam;
	}

	if (lparam == -1) {
		mpres.master_ndx = -1;
		mpres.release_ndx = -1;
		mpres.bmaster = false;
		mpres.brelease = false;
	}
	else {
		mpres.master_ndx = HIWORD(lparam);
		mpres.release_ndx = LOWORD(lparam);
		mpres.bmaster = (mpres.master_ndx != 9999);
		mpres.brelease = (mpres.release_ndx != 9999);
	}
	return mpres;
}

mounted_param CFindReleaseDialog::get_mounted_param(t_size item_index) {
	LPARAM tmp = MAKELPARAM(0, 0);
	get_item_param(release_list, item_index, 0, tmp);
	return get_mounted_param(tmp);
}

int CFindReleaseDialog::get_mounted_lparam(mounted_param myparam) {
	int ires = -1;
	if (myparam.is_release()) {
		ires = myparam.master_ndx << 16;
		ires |= myparam.release_ndx;
	}
	else
	if (myparam.bmaster) {
		ires = myparam.master_ndx << 16 | 9999;
	}
	else
	if (myparam.brelease) {
		ires = 9999 << 16 | myparam.release_ndx;
	}

	return ires;
}

pfc::string8 CFindReleaseDialog::get_param_id(mounted_param myparam) {

	pfc::string8 res_selected_id;
	if (myparam.is_master())
		res_selected_id = find_release_artist->master_releases[myparam.master_ndx]->main_release_id;
	else
		if (myparam.is_release())
			res_selected_id = find_release_artist->master_releases[myparam.master_ndx]->sub_releases[myparam.release_ndx]->id;
		else
			res_selected_id = find_release_artist->releases[myparam.release_ndx]->id;
	return res_selected_id;
}

pfc::string8 CFindReleaseDialog::get_param_id_master(mounted_param myparam) {

	pfc::string8 res_selected_id;
	if (myparam.is_master())
		res_selected_id = find_release_artist->master_releases[myparam.master_ndx]->id;

	return res_selected_id;
}

void CFindReleaseDialog::on_release_selected(t_size selection_index) {

	if (selection_index == -1)
		return;

	mounted_param myparam = get_mounted_param(selection_index);

	if (myparam.brelease)
		m_release_selected_index = selection_index;

	int list_count = ListView_GetItemCount(release_list);

	if (myparam.is_master()) {

		pfc::string8 release_url;
		if (_idtracer.amr && (atoi(get_param_id_master(myparam)) == _idtracer.master_id)) 
			release_url.set_string(std::to_string(_idtracer.release_id).c_str());
		else
			release_url.set_string(get_param_id(myparam));

		if (get_lvstate_expanded(selection_index)) {

			MasterRelease_ptr& master_release = find_release_artist->master_releases[myparam.master_ndx];
			
			if (master_release->sub_releases.get_size()) {

				abort_callback_dummy dummy;
				fake_threaded_process_status fstatus;
				on_expand_master_release_done(master_release, selection_index, fstatus, dummy);
			}
			else {
				//spawn
				expand_master_release(find_release_artist->master_releases[myparam.master_ndx], selection_index);
			}
		}
		else
		{
			//closing master branch

			uSetWindowText(release_url_edit, release_url);

			t_size totalsubs = find_release_artist->master_releases[myparam.master_ndx]->sub_releases.get_count();
			t_size skipped = 0;

			for (t_size walk = 0; walk < totalsubs; walk++) {

				//check tracker reset
				if (get_lvstate_tracker(release_list, selection_index + 1))
					_idtracer.release_reset();

				mounted_param thisparam;
				thisparam = get_mounted_param(selection_index + 1 + skipped);

				//delete rows
				if (thisparam.master_ndx == myparam.master_ndx)
					ListView_DeleteItem(release_list, selection_index + 1 + skipped);
				else
					skipped++;

			}
		}
	}
	else {
		uSetWindowText(release_url_edit, get_param_id(myparam).get_ptr());
	}
}

LRESULT CFindReleaseDialog::OnEditSearchText(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& /*bHandled*/) {
	if (wNotifyCode == EN_SETFOCUS || wNotifyCode == EN_CHANGE) {
		if (wNotifyCode == EN_CHANGE)
		{
			pfc::string8 buffer;
			uGetWindowText(search_edit, buffer);
			//m_artist_search = trim(buffer);
		}
	}
	return FALSE;
}

LRESULT CFindReleaseDialog::OnEditFilterText(WORD wNotifyCode, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	if (!m_bNoEnChange && wNotifyCode == EN_CHANGE) {
		
		pfc::string8 buffer;
		uGetWindowText(filter_edit, buffer);
		buffer = trim(buffer);

		if (!find_release_artist) {
			return false;
		}

		_idtracer.release_reset();

		if (buffer.get_length() == 0) {
			m_vec_filter.clear();
			update_releases(buffer, updRelSrc::Filter, true);
		}
		else {
			//same filter
			if (!stricmp_utf8(m_results_filter, buffer))
				return FALSE;

			ListView_DeleteAllItems(release_list);
			m_vec_build_lv_items.clear();
			
			_idtracer.release_reset();

			//INIT FILTER

			init_filter_i(buffer, buffer, false, true, false);

			//reset pos, keep other info
			_idtracer.release_reset();

			std::vector<std::pair<int, int>>::iterator it;
			t_size list_index = -1;
			t_size master_list_pos = pfc_infinite;
			t_size parent_master_expanded = pfc_infinite;

			//DISPLAY FILTER RESULTS

			for (it = m_vec_filter.begin(); it != m_vec_filter.end(); it++) {
				list_index++;
				LPARAM lparam = MAKELPARAM(0, 0);
				int master_ndx = it->first;
				int release_ndx = it->second;
				bool bmaster = it->first != pfc_infinite;
				bool brelease = it->second != pfc_infinite;
				mounted_param myparam = {master_ndx, release_ndx, bmaster, brelease};
				lparam = get_mounted_lparam(myparam);
				pfc::string8 item;
				row_col_data row_data;

				int pos = list_index;
				master_list_pos = pfc_infinite;

				if (myparam.is_master()) {
					master_list_pos = pos;
					hook->set_master(&(find_release_artist->master_releases[myparam.master_ndx]));
				}
				else {
					master_list_pos = pfc_infinite;
					if (myparam.is_release()) {
						std::vector<std::pair<int, int>>::iterator it_parent;

						int master_id = atoi(find_release_artist->master_releases[myparam.master_ndx]->id);
						auto parent = std::find_if(m_vec_build_lv_items.begin(), m_vec_build_lv_items.end(),
							[&](const std::pair<int, int>& e) { return e.first == master_id; });

						if (parent != m_vec_build_lv_items.end()) {
							//PARENT FOUND
							parent_master_expanded = std::distance(m_vec_build_lv_items.begin(), parent);
							pos = list_index;
							int expanded = 1;
							set_node_expanded(master_id, expanded, true);
							set_lvstate_expanded(parent_master_expanded, expanded);
						}
						else {
							//INSERT NOT-MATCHING PARENT							
							hook->set_master(&(find_release_artist->master_releases[myparam.master_ndx]));
							mounted_param mp_master = { master_ndx, -1, true, false };
							item = run_hook_columns(row_data, get_mounted_lparam(mp_master));
							
							//insert row
							parent_master_expanded = insert_item_row(row_data, pos, get_mounted_lparam(mp_master), true);
							
							int expanded = 1;
							set_node_expanded(row_data.id, expanded, true);
							set_lvstate_expanded(pos, expanded);

							pos = parent_master_expanded + 1;
							master_list_pos = parent_master_expanded;
						}

						hook->set_master(&(find_release_artist->master_releases[myparam.master_ndx]));

						hook->set_release(&(find_release_artist->master_releases[myparam.master_ndx]->sub_releases[myparam.release_ndx]));
					}
					else
						hook->set_release(&(find_release_artist->releases[myparam.release_ndx]));
				}
				item = run_hook_columns(row_data, lparam);
				
				if (myparam.is_master())
					_idtracer.release_check(pfc::toString(row_data.id).c_str(), list_index,
						myparam.is_master(), master_list_pos, myparam.master_ndx);
				else
					_idtracer.release_check(pfc::toString(row_data.id).c_str(), pos,
						myparam.is_master(), master_list_pos, myparam.master_ndx);

				//insert row
				list_index = insert_item_row(row_data, pos, lparam, true);

			}
			m_vec_lv_items = m_vec_build_lv_items;
		}
		m_results_filter.set_string(buffer);
	}
	return FALSE;
}

LRESULT CFindReleaseDialog::OnSelectArtist(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	
	t_size pos = ListView_GetSingleSelection(artist_list);
	if (pos != m_artist_index) {
		if (!find_release_artists.get_count() || find_release_artists[pos].get()->id == find_release_artist.get()->id) {
			//nothing to do, a list with just one artist, from a search based on artist id?
		}
		else
			get_selected_artist_releases(updRelSrc::ArtistList);
	}

	m_artist_index = pos;

	return FALSE;
}

LRESULT CFindReleaseDialog::OnDoubleClickArtist(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	t_size pos = ListView_GetSingleSelection(artist_list);
	if (pos != ~0) {
		pfc::string8 url;
		url << "https://www.discogs.com/artist/" << find_release_artists[pos]->id;
		display_url(url);
	}
	return FALSE;
}

LRESULT CFindReleaseDialog::OnCustomDrawReleaseList(int wParam, LPNMHDR lParam, BOOL bHandled) {

	if (ListView_GetItemCount(release_list) == 0) return CDRF_DODEFAULT;
	
	LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;
	int pos = (int)lplvcd->nmcd.dwItemSpec;

	switch (lplvcd->nmcd.dwDrawStage) {

	case CDDS_PREPAINT: //{
		//request notifications for individual listview items
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT: {
		mounted_param myparam = get_mounted_param(lplvcd->nmcd.lItemlParam);

		bool isdrop = get_lvstate_tracker(release_list, pos);

		if (isdrop && myparam.brelease)
		{
			//customize item appearance
			lplvcd->clrText = /*hlfrcolor;*/ RGB(0, 0, 0);
			lplvcd->clrTextBk = /*hlcolor;*/ RGB(215, 215, 215);
			return CDRF_NEWFONT;
		}
		else {
			if (myparam.brelease) {
				lplvcd->clrText = RGB(0, 0, 0);
				lplvcd->clrTextBk = RGB(255, 255, 255);
			}
			else {
				lplvcd->clrText = htcolor; //RGB(50, 50, 50);
				lplvcd->clrTextBk = RGB(240, 240, 240);
			}
			return CDRF_NEWFONT;
		}
		break;

		//Before a subitem is drawn
	}
	case CDDS_SUBITEM | CDDS_ITEMPREPAINT:

		if (0 == lplvcd->iSubItem)
		{
			//customize subitem appearance for column 0
			lplvcd->clrText = RGB(255, 0, 0);
			lplvcd->clrTextBk = RGB(255, 255, 255);

			//To set a custom font:
			//SelectObject(lplvcd->nmcd.hdc, <your custom HFONT>);

			return CDRF_NEWFONT;
		}
		else if (1 == lplvcd->iSubItem)
		{
			return CDRF_NEWFONT;
		} else if (2 == lplvcd->iSubItem)
		{
			return CDRF_NEWFONT;
		}
	}
	return CDRF_DODEFAULT;
}

LRESULT CFindReleaseDialog::OnCustomDrawArtistList(int wParam, LPNMHDR lParam, BOOL bHandled) {

	if (ListView_GetItemCount(artist_list) == 0) return CDRF_DODEFAULT;

	LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;
	int pos = (int)lplvcd->nmcd.dwItemSpec;

	switch (lplvcd->nmcd.dwDrawStage) {

	case CDDS_PREPAINT:
		//request notifications for individual listview items
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT: {
		//mounted_param myparam = get_mounted_param(lplvcd->nmcd.lItemlParam);

		bool isdrop = get_lvstate_tracker(artist_list, pos);

		if (isdrop && (_idtracker.artist_index == pos))
		{
			//customize item appearance
			lplvcd->clrText = /*hlfrcolor;*/ RGB(0, 0, 0);
			lplvcd->clrTextBk = /*hlcolor;*/ RGB(215, 215, 215);
			return CDRF_NEWFONT;
		}
		break;
	}
	case CDDS_SUBITEM | CDDS_ITEMPREPAINT:

		if (0 == lplvcd->iSubItem)
		{
			return CDRF_NEWFONT;
		}
		else if (1 == lplvcd->iSubItem)
		{
			return CDRF_NEWFONT;
		}
		else if (2 == lplvcd->iSubItem)
		{
			return CDRF_NEWFONT;
		}
	}
	return CDRF_DODEFAULT;
}

LRESULT CFindReleaseDialog::OnArtistListViewItemChanged(int, LPNMHDR hdr, BOOL&) {
	NMLISTVIEW* lpStateChange = reinterpret_cast<NMLISTVIEW*>(hdr);
	if ((lpStateChange->uNewState ^ lpStateChange->uOldState) & LVIS_SELECTED) {
		if (lpStateChange->uNewState & LVIS_SELECTED) {
			int pos = lpStateChange->iItem;
			if (pos != m_artist_index) {
				int frartist = find_release_artists.get_count();

				if (find_release_artist && (!find_release_artists.get_count() || find_release_artists[pos].get()->id == find_release_artist.get()->id)) {
					//nothing to do, a list with just one artist, from a search based on artist id?
				}
				else {
					//if (pos != _idtracker.artist_index) _idtracker.release_reset();
					get_selected_artist_releases(updRelSrc::ArtistList);
				}
			}

			m_artist_index = pos;


			//mounted_param myparam = get_mounted_param((t_size)lpStateChange->iItem);
			//select_release(lpStateChange->iItem);

		}
	}
	return 0;
}

//
//LRESULT CFindReleaseDialog::OnDoubleClickRelease(int, LPNMHDR hdr, BOOL&) {
//	NMLISTVIEW* lpStateChange = reinterpret_cast<NMLISTVIEW*>(hdr);
//
//	int pos = ListView_GetSingleSelection(release_list);
//	if (pos != -1) {
//
//		LV_ITEM lvi;
//		lvi.iItem = pos;
//		lvi.mask = LVIF_PARAM;
//		ListView_GetItem(release_list, &lvi);
//		mounted_param mp = get_mounted_param(lvi.lParam);
//
//		int master_index = lvi.lParam >> 16;
//		int release_index = lvi.lParam & 0xFFFF;
//		master_index = mp.master_ndx;
//		release_index = mp.release_ndx;
//
//		pfc::string8 url;
//		if (master_index != 9999 && release_index == 9999) {
//			url << "https://www.discogs.com/master/" << find_release_artist->master_releases[master_index]->id;
//		}
//		else if (master_index != 9999) {
//			url << "https://www.discogs.com/release/" << find_release_artist->master_releases[master_index]->sub_releases[release_index]->id;
//		}
//		else {
//			url << "https://www.discogs.com/release/" << find_release_artist->releases[release_index]->id;
//		}
//		display_url(url);
//	}
//	return 0;
//}

LRESULT CFindReleaseDialog::OnRClickRelease(int, LPNMHDR hdr, BOOL&) {
	LPNMITEMACTIVATE nmListView = (LPNMITEMACTIVATE)hdr;
	
	bool isArtist = hdr->hwndFrom == artist_list;

	t_size list_index = nmListView->iItem;
	POINT p = nmListView->ptAction;
	::ClientToScreen(hdr->hwndFrom, &p);
	mounted_param myparam = get_mounted_param(list_index);
	
	if (list_index == pfc_infinite)
		return 0;

	pfc::string8 sourcepage = isArtist? "View Artist page" : !myparam.brelease? "View Master Release page" : "View Release page";

	try {
		int coords_x = p.x, coords_y = p.y;
		enum { ID_VIEW_PAGE = 1 };
		HMENU menu = CreatePopupMenu();
		uAppendMenu(menu, MF_STRING, ID_VIEW_PAGE, sourcepage);
		int cmd = TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, coords_x, coords_y, 0, m_hWnd, 0);
		DestroyMenu(menu);
		switch (cmd)
		{
		case ID_VIEW_PAGE:
		{
			pfc::string8 url;
			if (isArtist) {
				if (find_release_artists.get_count() > 0)
					url << "https://www.discogs.com/artist/" << find_release_artists[list_index]->id;
				else
					if (find_release_artist != NULL)
						url << "https://www.discogs.com/artist/" << find_release_artist->id;

			}
			else {
				if (myparam.is_release()) {
					url << "https://www.discogs.com/release/" << find_release_artist->master_releases[myparam.master_ndx]->sub_releases[myparam.release_ndx]->id;
				}
				else
					if (myparam.bmaster) {
						url << "https://www.discogs.com/master/" << find_release_artist->master_releases[myparam.master_ndx]->id;
					}
					else {
						url << "https://www.discogs.com/release/" << find_release_artist->releases[myparam.release_ndx]->id;
					}
			}
			if (url != NULL)
				display_url(url);

			return true;
		}
		}
	}
	catch (...) {}

	return 0;
}

LRESULT CFindReleaseDialog::OnClickRelease(int, LPNMHDR hdr, BOOL&) {
	LPNMITEMACTIVATE nmListView = (LPNMITEMACTIVATE)hdr;

	t_size list_index = nmListView->iItem;
	POINT p = nmListView->ptAction;

	mounted_param myparam = get_mounted_param(list_index);
	select_lv_release(list_index);

	return 0;
}

LRESULT CFindReleaseDialog::OnKeyDownRelease(int, LPNMHDR hdr, BOOL&) {
	LPNMITEMACTIVATE nmListView = (LPNMITEMACTIVATE)hdr;

	t_size list_index = ListView_GetSingleSelection(release_list);

	if (list_index != ~0 && hdr->code == (UINT)LVN_KEYDOWN) {
		UINT keycode = ((LPNMLVKEYDOWN)hdr)->wVKey;
		mounted_param myparam = get_mounted_param(list_index);
		if (myparam.is_master()) {
			bool expanded = get_lvstate_expanded(list_index);
			if (keycode == VK_LEFT)
			{
				if (expanded)
					select_lv_release(list_index);
			}
			else if (keycode == VK_RIGHT)
			{
				if (!expanded)
					select_lv_release(list_index);
			}
		}
	}

	return 0;
}

LRESULT CFindReleaseDialog::OnCheckOnlyExactMatches(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	conf.display_exact_matches = IsDlgButtonChecked(IDC_ONLY_EXACT_MATCHES_CHECK) != 0;
	fill_artist_list(_idtracer.amr, updRelSrc::Artist);
	get_selected_artist_releases(updRelSrc::Artist);
	return FALSE;
}

//TODO: implement proper mng
LRESULT CFindReleaseDialog::OnCheckReleasesShowId(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	cfgcol& cfg_col = cfg_lv.colmap.at(0);
	cfg_col.enabled = !cfg_col.enabled;

	t_size selected = ListView_GetSingleSelection(release_list);

	if (!cfg_col.enabled) {
		//delete column
		if (cfg_col.icol == 0)
			reset_default_columns(false);
		else {
			ListView_DeleteColumn(release_list, cfg_col.icol);
			cfg_col.icol = 0;
			update_sorted_icol_map(false /*sort without reset*/);
		}
	}
	else {
		int newicol = ListView_GetColumnCount(release_list);
		//insert column
		int icol = listview_helper::fr_insert_column(release_list, newicol,
			cfg_col.name, (unsigned int)cfg_col.width, LVCFMT_LEFT);

		cfg_col.enabled = true;
		cfg_col.icol = icol;
		update_sorted_icol_map(false /*sort without reset*/);
	}

	if (m_vec_filter.size()) {

		update_releases_columns();

	}
	else {
		if (find_release_artist) {

			if (g_discogs->find_release_dialog && find_release_artist) {

				//TODO: revision to clearing on enter update_releases... 
				t_size the_artist = !find_release_artist ? pfc_infinite : atoi(find_release_artist->id);
				t_size m_old = m_release_list_artist_id;
				m_release_list_artist_id = the_artist;

				//TODO: sending src artist to delete on enter
				update_releases(m_results_filter, updRelSrc::Artist, true);

				m_release_list_artist_id = the_artist = m_old;
			}
		}

		ListView_RedrawItems(release_list, 0, ListView_GetGroupCount(release_list));

		if (selected != pfc_infinite) {
			ListView_SetItemState(release_list, selected, LVIS_SELECTED, LVIS_SELECTED);
			ListView_EnsureVisible(release_list, selected, FALSE);
		}
	
	}

	return FALSE;
}

LRESULT CFindReleaseDialog::OnButtonSearch(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	
	m_artist_index = pfc_infinite;
	
	pfc::string8 artistname;
	uGetWindowText(search_edit, artistname);
	artistname.set_string(trim(artistname));

	if (artistname.get_length())
		search_artist(true, artistname);

	return FALSE;
}

LRESULT CFindReleaseDialog::OnButtonConfigure(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (!g_discogs->configuration_dialog) {
		static_api_ptr_t<ui_control>()->show_preferences(guid_pref_page);
	}
	else {
		::SetFocus(g_discogs->configuration_dialog->m_hWnd);
	}
	return FALSE;
}

void CFindReleaseDialog::on_write_tags(const pfc::string8 &release_id) {
	service_ptr_t<process_release_callback> task = new service_impl_t<process_release_callback>(this, release_id, items);
	task->start(m_hWnd);
}

void CFindReleaseDialog::extract_id_from_url(pfc::string8 &s) {
	size_t pos = s.find_last('/');
	if (pos != pfc::infinite_size) {
		s = substr(s, pos + 1, s.get_length() - pos - 1);
	}
	pos = s.find_first('#');
	if (pos != pfc::infinite_size) {
		s = substr(s, 0, pos);
	}
	pos = s.find_first('-');
	if (pos != pfc::infinite_size) {
		s = substr(s, 0, pos);
	}
	pos = s.find_first('=');
	if (pos != pfc::infinite_size) {
		s = substr(s, pos + 1, s.get_length() - pos - 1);
	}
}

void CFindReleaseDialog::extract_release_from_link(pfc::string8 &s) {
	size_t pos = s.find_first("[r");
	if (pos == 0) {
		s = substr(s, 2, s.get_length() - 3);
	}
}

bool CFindReleaseDialog::set_node_expanded(t_size master_id, int &state, bool build) {
	
	std::vector<std::pair<int, int>>& vec_lv_items = 
		build ? m_vec_build_lv_items : m_vec_lv_items;

	auto master_row =
		std::find_if(vec_lv_items.begin(), vec_lv_items.end(), [&](const std::pair<int, int>& e) {
		return (e.first == master_id); });

	if (master_row != vec_lv_items.end()) {
		if (state == -1) {
			state = master_row->second == 1 ? 0 : 1;
			master_row->second = state;
		}
		else {
			master_row->second = state;
		}

		return true;
	}
	else {
		state = -1;
		return false;
	}
}

bool CFindReleaseDialog::get_node_expanded(t_size master_id, int &out_state) {

	std::vector<std::pair<int, int>>& vec_lv_items = m_vec_lv_items;

	auto master_row =
		std::find_if(vec_lv_items.begin(), vec_lv_items.end(), [&](const std::pair<int, int>& e) {
		return (e.first == master_id); });

	if (master_row != vec_lv_items.end()) {
		out_state = master_row->second;
		return true;
	}
	else {
		out_state = -1;
		return false;
	}
		
}

void CFindReleaseDialog::on_expand_master_release_done(const MasterRelease_ptr &master_release, int list_index, threaded_process_status &p_status, abort_callback &p_abort) {
	
	int master_i = -1;
	for (size_t i = 0; i < find_release_artist->master_releases.get_size(); i++) {
		if (find_release_artist->master_releases[i]->id == master_release->id) {
			master_i = i;
			break;
		}
	}
	pfc::string8 text;
	uGetWindowText(filter_edit, text);

	PFC_ASSERT(master_i != -1);

	pfc::string8 filter;
	uGetWindowText(filter_edit, filter);

	int expanded = 1;
	bool bnodeset = set_node_expanded(atoi(master_release->id), expanded, false);

	if (bnodeset) {
		expand_releases(filter, updRelSrc::Releases, master_i, list_index);
		set_lvstate_expanded(list_index, expanded);
	}
	
	mounted_param myparam = get_mounted_param((t_size)list_index);

	pfc::string8 release_url;

	if (_idtracer.amr && (atoi(master_release->id) == _idtracer.master_id)) 
		release_url.set_string(std::to_string(_idtracer.release_id).c_str());
	else 
		release_url.set_string(get_param_id(myparam));

	uSetWindowText(release_url_edit, release_url);
	
}

void CFindReleaseDialog::on_expand_master_release_complete() {

	active_task = NULL;
}

void CFindReleaseDialog::expand_master_release(MasterRelease_ptr &release, int pos) {
	if (release->sub_releases.get_size()) {
		return;
	}

	service_ptr_t<expand_master_release_process_callback> task = 
		new service_impl_t<expand_master_release_process_callback>(release, pos);

	active_task = &task;
	task->start(m_hWnd);
}

void CFindReleaseDialog::get_selected_artist_releases(updRelSrc updsrc) {

	t_size pos = ListView_GetSingleSelection(artist_list);

	if (pos != ~0) {
		Artist_ptr &artist = find_release_artists[pos];

		if (find_release_artist.use_count() == 0 || artist.get()->id != find_release_artist.get()->id) {
			service_ptr_t<get_artist_process_callback> task =
				new service_impl_t<get_artist_process_callback>(updsrc, artist->id.get_ptr());
			task->start(m_hWnd);
		}
		else {
			on_get_artist_done(updRelSrc::ArtistList, find_release_artist);
		}
	}
}

void CFindReleaseDialog::search_artist(bool skip_tracer, pfc::string8 artistname) {

	bool cfg_skip_search_artist_if_present = true;
	if (cfg_skip_search_artist_if_present 
		&& !skip_tracer && _idtracer.artist && _idtracer.artist_id != 1) {

		pfc::string8 artist_id;
		artist_id.set_string(std::to_string(_idtracer.artist_id).c_str());

		if (artist_id.get_length()) {
			service_ptr_t<get_artist_process_callback> task =
				new service_impl_t<get_artist_process_callback>(updRelSrc::Undef, artist_id.get_ptr());
			task->start(m_hWnd);
		}
	}
	else {
		service_ptr_t<search_artist_process_callback> task =
			new service_impl_t<search_artist_process_callback>(artistname.get_ptr());
		task->start(m_hWnd);
	}
}
