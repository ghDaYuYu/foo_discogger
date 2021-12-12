#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include "stdafx.h"
#include <CommCtrl.h>
#include <commoncontrols.h>
#include <atlctrls.h>
#include <WinUser.h>

#include "libPPUI/win32_utility.h"

#include "resource.h"
#include "find_release_tree.h"
#include "configuration_dialog.h"

#include "tasks.h"
#include "multiformat.h"
#include "sdk_helpers.h"

#include "utils.h"

#include "find_release_dialog.h"

static const GUID guid_cfg_window_placement_find_release_dlg = { 0x7342d2f3, 0x235d, 0x4bed, { 0x86, 0x7e, 0x82, 0x7f, 0xa8, 0x8e, 0xd9, 0x87 } };
static cfg_window_placement cfg_window_placement_find_release_dlg(guid_cfg_window_placement_find_release_dlg);

struct cfgcol {
	int ndx = 0;
	int id = 0;
	int icol = 0;
	pfc::string8 name = "";
	pfc::string8 titleformat = "";
	float width = 0.0f;
	bool default = false;
	bool enabled = true;
};

using cfgColMap = std::map<std::int32_t, cfgcol>;

const int TOTAL_DEF_COLS = 2;

cfgcol column_configs[TOTAL_DEF_COLS] = {
	//#, ndx, icol, name, tf, width, default, enabled
	{1, 1, 0,"Id", "%RELEASE_ID%", cfg_find_release_colummn_showid_width, false, false},
	{100, 10, 0,"Master/Release", "", 200.0f, true, false},
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
		cfgcol tmp_cfgcol{
		column_configs[it].id,
		column_configs[it].ndx,
		column_configs[it].icol,
		column_configs[it].name,
		column_configs[it].titleformat,
		column_configs[it].width,
		column_configs[it].default,
		column_configs[it].enabled,
	};
	cfg_out.colmap.emplace(cfg_out.colmap.size(), tmp_cfgcol);
	}
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
	update_sorted_icol_map(breset);
	for (unsigned int i = 0; i < vec_icol_subitems.size(); i++)
	{
		cfgcol walk_cfg = cfg_lv.colmap.at(vec_icol_subitems[i].first);
		cfg_lv.colmap.at(vec_icol_subitems[i].first).enabled = true;
	}
}

inline void CFindReleaseDialog::load_size() {
	//
}

void CFindReleaseDialog::pushcfg() {
	bool conf_changed = build_current_cfg();
	if (conf_changed) {
		CONF.save(CConf::cfgFilter::FIND, conf);
		CONF.load();
	}
}

inline bool CFindReleaseDialog::build_current_cfg() {
	bool bres = false;

	bool bcheck = IsDlgButtonChecked(IDC_ONLY_EXACT_MATCHES_CHECK);
	if (CONF.display_exact_matches != bcheck) {
		conf.display_exact_matches = bcheck;
		bres |= true;
	}

	if ((CONF.find_release_filter_flag & FilterFlag::Versions) != (conf.find_release_filter_flag & FilterFlag::Versions)) {		
		bres |= true;
	}
	if ((CONF.find_release_filter_flag & FilterFlag::RoleMain) != (conf.find_release_filter_flag & FilterFlag::RoleMain)) {
		bres |= true;
	}
	return bres;
}

LRESULT CFindReleaseDialog::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	cfg_window_placement_find_release_dlg.on_window_destruction(m_hWnd);
	pushcfg();
	KillTimer(KTypeFilterTimerID);
	return 0;
}

LRESULT CFindReleaseDialog::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	pfc::string8 release_id;
	release_id = trim(uGetWindowText(m_release_url_edit).c_str());

	if (!is_number(release_id.get_ptr())) {
	    if (pfc::string_has_prefix(artistname, "https://www.discogs.com/release/")) {
	        release_id = extract_max_number(release_id);
	    }
	    else
	        return TRUE;
    }		
	
	if (release_id.get_length()) 
		on_write_tags(release_id);

	return TRUE;
}

LRESULT CFindReleaseDialog::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	destroy();
	return TRUE;
}

bool OAuthCheck(foo_conf conf) {
	if (!g_discogs->gave_oauth_warning &&
		(!conf.oauth_token.get_length() || !conf.oauth_token_secret.get_length())) {
		g_discogs->gave_oauth_warning = true;
		if (!g_discogs->configuration_dialog) {
			static_api_ptr_t<ui_control>()->show_preferences(guid_pref_page);
		}
		CConfigurationDialog* dlg = reinterpret_cast<CConfigurationDialog*>(g_discogs->configuration_dialog);
		
		if (dlg) {			
			dlg->show_oauth_msg("Please configure OAuth.", true);
			::SetFocus(dlg->m_hWnd);
		}
		return false;
	}
	return true;
}

LRESULT CFindReleaseDialog::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	SetIcon(g_discogs->icon);
	conf = CONF;
	defaultCfgCols(cfg_lv);

	hlcolor = (COLORREF)::GetSysColor(COLOR_HIGHLIGHT);
	hlfrcolor = (COLORREF)::GetSysColor(COLOR_HIGHLIGHTTEXT);
	htcolor = (COLORREF)::GetSysColor(COLOR_HOTLIGHT);

	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(icex);
	icex.dwICC = ICC_STANDARD_CLASSES | ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES;
	InitCommonControlsEx(&icex);

	m_search_edit = GetDlgItem(IDC_SEARCH_EDIT);
	m_filter_edit = GetDlgItem(IDC_FILTER_EDIT);
	m_release_url_edit = GetDlgItem(IDC_RELEASE_URL_TEXT);
	m_artist_list = GetDlgItem(IDC_ARTIST_LIST);
	m_release_tree = GetDlgItem(IDC_RELEASE_TREE);

	ListView_SetExtendedListViewStyle(m_artist_list, LVS_EX_FULLROWSELECT);
	ListView_SetExtendedListViewStyleEx(m_artist_list,
		LVS_EX_COLUMNOVERFLOW | LVS_EX_FLATSB /*| LVS_EX_HEADERDRAGDROP*/ /*| LVS_EX_GRIDLINES*/ | LVS_EX_AUTOSIZECOLUMNS | LVS_EX_COLUMNSNAPPOINTS /*| LVS_EX_BORDERSELECT| LVS_EX_DOUBLEBUFFER*/,
		LVS_EX_COLUMNOVERFLOW | LVS_EX_FLATSB /*| LVS_EX_HEADERDRAGDROP*/ /*| LVS_EX_GRIDLINES*/ | LVS_EX_AUTOSIZECOLUMNS | LVS_EX_COLUMNSNAPPOINTS /*| LVS_EX_BORDERSELECT*/);

	HIMAGELIST hImageList = NULL;
	if (S_OK == SHGetImageList(SHIL_SMALL, IID_IImageList, reinterpret_cast<LPVOID*>(&hImageList))) {
		
		CImageList shimagelist(hImageList);
		SIZE iconsize;
		shimagelist.GetIconSize(iconsize);
		Gdiplus::Bitmap bmBlank(iconsize.cx, iconsize.cy);
		HBITMAP hBmBlank;
		Gdiplus::Graphics gr(&bmBlank); 
		gr.Clear(GetSysColor(COLOR_WINDOW));
		if (bmBlank.GetHBITMAP(RGB(255, 255, 255)/*Color::Black*/, &hBmBlank) != Gdiplus::Ok) {
			log_msg("GdiPlus error (GetHBITMAP 48x48 tmp artwork)");
		}

		shimagelist.Replace(0, hBmBlank, 0);

		ListView_SetImageList(m_artist_list, shimagelist.m_hImageList, LVSIL_SMALL);
		DeleteObject(hBmBlank);
	}

	CRect rc_artists;
	::GetWindowRect(m_artist_list, &rc_artists);
	int icol = listview_helper::fr_insert_column(m_artist_list, 0,
		"Artist", rc_artists.Width(), LVCFMT_LEFT);

	cewb_artist_search.SubclassWindow(m_search_edit);
	cewb_release_filter.SubclassWindow(m_filter_edit);
	cewb_release_url.SubclassWindow(m_release_url_edit);
	SetEnterKeyOverride(conf.release_enter_key_override);

	m_release_ctree.Inititalize(m_release_tree, m_artist_list, m_filter_edit, m_release_url_edit);
	m_release_ctree.SetDiscogsInterface(discogs_interface);
	m_release_ctree.SetOnSelectedNotifier(
		[this](int lparam) -> bool {
			this->on_release_selected(lparam);
			return true;
		});
	m_release_ctree.SetOnOkNotifier(
		[this]() -> bool {
			BOOL bdummy;
			OnOK(0L, 0L, NULL, bdummy);
			return true;
		});

	reset_default_columns(true);

	init_tracker();

	file_info_impl finfo;
	metadb_handle_ptr item = items[0];
	item->get_info(finfo);

	const char* filter = finfo.meta_get("ALBUM", 0);
	m_results_filter = filter ? filter : "";

	const char* artist = finfo.meta_get("ARTIST", 0) ? finfo.meta_get("ARTIST", 0) : finfo.meta_get("ALBUM ARTIST", 0);

	if (artist) {
		uSetWindowText(m_search_edit, artist);
		if (conf.enable_autosearch && (_idtracer.artist || artist)) {
			search_artist(false, artist); //_idtracker.artist_id will be used if available
		}
		else {
			if (_idtracer.release && _idtracer.release_id != -1) {
				uSetWindowText(m_release_url_edit,
					std::to_string(_idtracer.release_id).c_str());
			}
		}
	}


	DlgResize_Init(mygripp.enabled, true);

	if (conf.find_release_filter_flag & FilterFlag::Versions) {
		uButton_SetCheck(m_hWnd, IDC_CHECK_FIND_RELEASE_FILTER_VERSIONS, true);
	}
	if (conf.find_release_filter_flag & FilterFlag::RoleMain) {
		uButton_SetCheck(m_hWnd, IDC_CHECK_FIND_RELEASE_FILTER_ROLEMAIN, true);
	}

	bool db_ready = DBFlags(conf.db_dc_flag).IsReady();
	uButton_SetCheck(m_hWnd, IDC_CHECK_FIND_REL_DB_SEARCH_LIKE, conf.db_dc_flag & DBFlags::DB_SEARCH_LIKE);
	uButton_SetCheck(m_hWnd, IDC_CHECK_FIND_REL_DB_SEARCH_ANV, conf.db_dc_flag & DBFlags::DB_SEARCH_ANV);
	::ShowWindow(uGetDlgItem(IDC_CHECK_FIND_REL_DB_SEARCH_LIKE), db_ready);
	::ShowWindow(uGetDlgItem(IDC_CHECK_FIND_REL_DB_SEARCH_ANV), db_ready);
	
	cfg_window_placement_find_release_dlg.on_window_creation(m_hWnd, true);

	if (OAuthCheck(conf) || DBFlags(conf.db_dc_flag).IsReady() && DBFlags(conf.db_dc_flag).Search()) {
		if ((conf.skip_mng_flag & SkipMng::SKIP_FIND_RELEASE_DLG_IDED) && _idtracer.release) {
			on_write_tags(pfc::toString(_idtracer.release_id).c_str());
		}
		else {
			if (_idtracer.release && _idtracer.release_id != pfc_infinite)
				uSetWindowText(m_release_url_edit, pfc::toString(_idtracer.release_id).c_str());

			show();

			m_DisableFilterBoxEvents = true;

			if (!_idtracer.amr || !conf.enable_autosearch) {
				uSetWindowText(m_filter_edit, filter ? filter : "");
			}

			m_DisableFilterBoxEvents = false;
			UINT default_button = 0;

			if (_idtracer.amr || _idtracer.release) {
				default_button = IDC_PROCESS_RELEASE_BUTTON;
			}
			else if (!artist) {
				default_button = IDC_SEARCH_EDIT;
			}
			else if (artist) {
				// default_button = 0;
			}

			if (default_button) {
				uSendMessage(m_hWnd, WM_NEXTDLGCTL, (WPARAM)(HWND)GetDlgItem(default_button), TRUE);
			}
			else {
				uSendMessage(m_hWnd, WM_NEXTDLGCTL, (WPARAM)(HWND)cewb_release_filter, TRUE);
			}
		}
	}
	else {
		//test failed... show or destroy to avoid grayed out menu item Write tags...
		show();
	}
	//FALSE: prevent the system from setting the default focus
	return FALSE;
}

void CFindReleaseDialog::SetEnterKeyOverride(bool enter_ovr) {
	cewb_release_filter.SetEnterEscHandlers();
	cewb_artist_search.SetEnterEscHandlers();
	cewb_release_url.SetEnterEscHandlers();

	if (enter_ovr) {
		cewb_artist_search.SetEnterOverride(stdf_enteroverride_artist);
		cewb_release_url.SetEnterOverride(stdf_enteroverride_url);
	}
}

//update_releases(m_results_filter, updRelSrc::Undef, false);

void CFindReleaseDialog::fill_artist_list(bool force_exact, updRelSrc updsrc) {
	HWND exactcheck = uGetDlgItem(IDC_ONLY_EXACT_MATCHES_CHECK);
	bool from_search = updsrc != updRelSrc::Undef && updsrc != updRelSrc::ArtistList;
	if (from_search) {
		m_artist_index = pfc_infinite;
		ListView_DeleteAllItems(m_artist_list);
		::EnableWindow(exactcheck, true);
		find_release_artists = m_artist_exact_matches;
		bool exact_matches = IsDlgButtonChecked(IDC_ONLY_EXACT_MATCHES_CHECK) != 0;
		if (!exact_matches) {
			find_release_artists.append(m_artist_other_matches);
		}
		int artists_index = 0, list_index = 0;
		for (size_t i = 0; i < find_release_artists.get_size(); i++) {
			pfc::string8 artist_name = find_release_artists[i]->name;
			bool artist_offline = find_release_artists[i]->loaded_releases_offline;
			_idtracer.artist_check(find_release_artists[i]->id, i);
			bool isdropindex = (list_index == _idtracer.artist_index);
			
			listview_helper::insert_lvItem_tracer(
				m_artist_list, list_index, atoi(find_release_artists[i]->id) == _idtracer.artist_id,
				artist_name.get_ptr(), artists_index, artist_offline? IMAGELIST_OFFLINE_CACHE_NDX : 0);

			artists_index++;
			list_index++;
		}

		if (find_release_artists.get_size()) {
			ListView_SetItemState(m_artist_list, 0, LVIS_SELECTED, LVIS_SELECTED);
			m_artist_index = 0;
		}
	}
	else if (updsrc == updRelSrc::Undef) {
		
		::EnableWindow(exactcheck, false || DBFlags(CONF.db_dc_flag).IsReady());
		pfc::string8 name = find_release_artist->name;
		bool artist_offline = find_release_artist->loaded_releases_offline;
		_idtracer.artist_check(find_release_artist->id, 0);

		listview_helper::insert_lvItem_tracer(m_artist_list, 0,
			_idtracer.artist_id == atoi(find_release_artist->id),
			name.get_ptr(), (LPARAM)0, artist_offline ? IMAGELIST_OFFLINE_CACHE_NDX: 0);

		if (!_idtracer.release) {
			find_release_artists.append_single(std::move(find_release_artist));
			set_artist_item_param(m_artist_list, 0, 0, (LPARAM)0);
			ListView_SetItemState(m_artist_list, 0, LVIS_SELECTED, LVIS_SELECTED);
			m_artist_index = 0;
		}
	}
	else if (updsrc == updRelSrc::ArtistList) {
		bool artist_offline = find_release_artist->loaded_releases_offline;
		LV_ITEM lvitem = {};
		int iPos = ListView_GetNextItem(m_artist_list, -1, LVNI_SELECTED);
		bool bupdate = false;
		lvitem.iItem = iPos;
		lvitem.mask = LVIF_IMAGE;
		ListView_GetItem(m_artist_list, &lvitem);
		if (artist_offline && lvitem.iImage != 8/*IMAGELIST_OFFLINE_CACHE_NDX*/) {
			lvitem.iImage = 8;
			bupdate = true;
		}
		else if (!artist_offline && lvitem.iImage != 0)
		{
			lvitem.iImage = 0;
			bupdate = true;
		}
		if (bupdate)
			ListView_SetItem(m_artist_list, &lvitem);

		::EnableWindow(exactcheck, true);
	}
	m_release_ctree.SetFindReleaseArtists(find_release_artists);
}

void CFindReleaseDialog::on_get_artist_done(updRelSrc updsrc, Artist_ptr& artist) {

	pfc::string8 lastFilter;
	uGetWindowText(m_filter_edit, lastFilter);
	HWND wnd_profile = uGetDlgItem(IDC_PROFILE_EDIT);
	uSetWindowText(wnd_profile, artist->profile);

	if (updsrc == updRelSrc::Undef) {
		pfc::array_t<Artist_ptr> good_matches;
		good_matches.append_single(std::move(artist));
		m_artist_exact_matches.append(good_matches);
	}


	find_release_artist = artist;
	fill_artist_list(true, updsrc);
	hook = std::make_shared<titleformat_hook_impl_multiformat>(&find_release_artist);

	pfc::string8 buffer;
	file_info_impl finfo;	metadb_handle_ptr item = items[0];
	item->get_info(finfo);

	const char* album = finfo.meta_get("ALBUM", 0);
	pfc::string8 filter(album ? album : "");

	//figure out a filter
	if (_idtracer.amr) {
		init_tracker_i("", "", false, true);
		bool albumdone = false;
		pfc::string8 mtitle = _idtracer.master_i != pfc_infinite ? find_release_artist->master_releases[_idtracer.master_i]->title : "";
		pfc::string8 rtitle = _idtracer.release_i == pfc_infinite ? ""
			: _idtracer.master_id == pfc_infinite ? find_release_artist->releases[_idtracer.release_i]->title
			: _idtracer.master_i == pfc_infinite ? "" : find_release_artist->master_releases[_idtracer.master_i]->sub_releases[_idtracer.release_i]->title;

		if (stricmp_utf8(filter, mtitle)) {
			pfc::string8 buffer(filter);
			if (buffer.has_prefix(mtitle)) {
				filter.set_string(mtitle);
			}
			else {
				if (!mtitle.has_prefix(buffer)) {
					filter = "";
				}
			}
		}
		init_tracker_i(mtitle, rtitle, false, true);
	}

	if (!lastFilter.get_length()) {
		if (!m_skip_fill_filter) {
			m_DisableFilterBoxEvents = true;
			uSetWindowText(m_filter_edit, filter);
			m_DisableFilterBoxEvents = false;
		}
	}

	uGetWindowText(m_filter_edit, filter);
	m_results_filter = filter;

	m_release_ctree.SetFindReleases(find_release_artist, _idtracer);

	update_releases(m_results_filter, updsrc, true);

	if (m_vec_items_ptr->size()) {
		int src_lparam = -1;
		mounted_param myparam;
		if (_idtracer.amr) {
			myparam = { _idtracer.master_i, _idtracer.release_i, _idtracer.master,_idtracer.release };
			src_lparam = myparam.lparam();
			//select if release is loaded

			vec_iterator_t vec_lv_it;
			find_vec_by_lparam(*m_vec_items_ptr, src_lparam, vec_lv_it);

			if (vec_lv_it != m_vec_items_ptr->end()) {
				src_lparam = myparam.lparam();
				on_release_selected(src_lparam);
			}
			else {

				//release not loaded, try to select master and expand

				mounted_param not_found;
				myparam = mounted_param(_idtracer.master_i, ~0, _idtracer.master, false);
				src_lparam = myparam.lparam();
				vec_iterator_t vec_lv_it;
				find_vec_by_lparam(*m_vec_items_ptr, src_lparam, vec_lv_it);

				if (vec_lv_it != m_vec_items_ptr->end()) {
					src_lparam = myparam.lparam();
					int ival = 1;
					if (updsrc == updRelSrc::Undef) {
						m_cache_find_release_ptr->SetCacheFlag(vec_lv_it->first, NodeFlag::expanded, &ival);

						m_release_ctree.SetHit(src_lparam);
					}
					on_release_selected(src_lparam);
				}
				else {
					//release not loaded, trying to select master to expand
					mounted_param not_found;
					myparam = mounted_param(~0, _idtracer.release_i, false, _idtracer.release);
					src_lparam = myparam.lparam();
					vec_iterator_t vec_lv_it;
					find_vec_by_lparam(*m_vec_items_ptr, src_lparam, vec_lv_it);

					if (vec_lv_it != m_vec_items_ptr->end()) {
						src_lparam = myparam.lparam();
						int ival = 1;
						
						//note: update_releases() routine updated master info

						m_cache_find_release_ptr->SetCacheFlag(vec_lv_it->first, NodeFlag::expanded, &ival);
						on_release_selected(src_lparam);
					}
				}

				//spawn (reenters update_releases as updSrc::Releases)
				//select_release(0);
			}

		}
		else {
			if (_idtracer.release_i != pfc_infinite && _idtracer.release_index != -1) {
				vec_iterator_t vec_lv_it;
				find_vec_by_id(*m_vec_items_ptr, _idtracer.master_id, vec_lv_it);

				if (vec_lv_it != m_vec_items_ptr->end()) {
					src_lparam = vec_lv_it->first->first;
					on_release_selected(src_lparam);
				}
			}
		}
	}
	else {
		//no releases loaded yet	
	}
}

void CFindReleaseDialog::enable(bool is_enabled) {
	::uEnableWindow(GetDlgItem(IDC_PROCESS_RELEASE_BUTTON), is_enabled);
	::uEnableWindow(GetDlgItem(IDCANCEL), is_enabled);
	::uEnableWindow(GetDlgItem(IDC_SEARCH_BUTTON), is_enabled);
}

void CFindReleaseDialog::on_search_artist_done(pfc::array_t<Artist_ptr>& p_artist_exact_matches, const pfc::array_t<Artist_ptr>& p_artist_other_matches) {

	m_artist_exact_matches = p_artist_exact_matches;
	m_artist_other_matches = p_artist_other_matches;

	if (!m_artist_exact_matches.get_size() && m_artist_other_matches.get_size()) {
		CheckDlgButton(IDC_ONLY_EXACT_MATCHES_CHECK, false);
	}
	
	//spawns by item list selection
	fill_artist_list(_idtracer.amr, updRelSrc::Artist);

}

void CFindReleaseDialog::set_artist_item_param(HWND list, int list_index, int col, LPARAM in_p) {
	LVITEM lvitem = { 0 };
	lvitem.mask = LVIF_PARAM;
	lvitem.lParam = in_p;
	lvitem.iItem = list_index;
	lvitem.iSubItem = col;
	int res = ListView_SetItem(list, &lvitem);
}

pfc::string8 CFindReleaseDialog::run_hook_columns(row_col_data& row_data, int item_data) {

	mounted_param myparam((LPARAM)item_data);
	pfc::string8 search_formatted;
	pfc::string8 id;

	row_data.col_data_list.clear();

	cfgcol walk_cfg = cfg_lv.colmap.at(vec_icol_subitems[0].first);

	if (myparam.is_master()) {
		CONF.search_master_format_string->run_hook(location, &info, hook.get(), search_formatted, nullptr);

		id << find_release_artist->master_releases[myparam.master_ndx]->id;
	}
	else {

        if (myparam.is_release()) {
            CONF.search_master_sub_format_string->run_hook(location, &info, hook.get(), search_formatted, nullptr);
            pfc::string8 main_release_id = find_release_artist->master_releases[myparam.master_ndx]->main_release->id;
            pfc::string8 this_release_id = find_release_artist->master_releases[myparam.master_ndx]->sub_releases[myparam.release_ndx]->id;
            id << this_release_id;
        }
        else
        {
            CONF.search_release_format_string->run_hook(location, &info, hook.get(), search_formatted, nullptr);
            id << find_release_artist->releases[myparam.release_ndx]->id;
        }
    }
    row_data.col_data_list.emplace_back(std::make_pair(walk_cfg.icol, search_formatted));
	row_data.id = atoi(id);

	return search_formatted;
}

bool tokenize_filter(pfc::string8 filter, pfc::array_t<pfc::string>& out_filter_words_lowercase) {
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
		;
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
	t_size ires = 0;
	bool filtered = filter_master.get_length() > 0;
	bool matches_master = true;
	bool matches_release = true;
	_idtracer.master_i = pfc_infinite;
	_idtracer.release_i = pfc_infinite;

	//if (countreserve) fast = false;

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

std::pair<t_size, t_size> CFindReleaseDialog::init_filter_i(pfc::string8 filter_master, pfc::string8 filter_release,
	bool ontitle, bool expanded, bool fast, bool countreserve) {

	bool bsub_release_filter = conf.find_release_filter_flag & FilterFlag::Versions;
	bool brolemain_filter = conf.find_release_filter_flag & FilterFlag::RoleMain;

	std::pair<t_size, t_size> reserve = { 0 , 0 };
	if (countreserve) {
		fast = false; expanded = true;
	}

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
			std::string str_role = std::string(mr_p->search_role);
			if ((!brolemain_filter || (str_role.find("Main") != str_role.npos)) && matches_master /*&& atoi(find_release_artist->master_releases[master_ndx]->id) == _idtracker.master_id*/) {
				pos++;
				if (atoi(find_release_artist->master_releases[master_ndx]->id) == _idtracer.master_id) {
					_idtracer.master_index = pos;
				}
				if (countreserve)
					reserve.first++;
				else
					m_vec_filter.emplace_back(std::pair<int, int>(master_ndx, pfc_infinite));

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
						std::string str_role = std::string(r_p->search_role);
						matches_release = (!brolemain_filter || (str_role.find("Main") != str_role.npos) || r_p->master_id.get_length()) && (!bsub_release_filter || check_match(sub_item, filter_master, lcf_words));
					}
				}
				if (matches_release) {
					pos++;
					if (atoi(find_release_artist->master_releases[master_ndx]->sub_releases[j]->id) == _idtracer.release_id) {
						_idtracer.release_index = pos + j;
					}
					if (countreserve)
						reserve.second++;
					else
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
			std::string str_role = std::string(r_p->search_role);
			if ((!brolemain_filter || (str_role.find("Main") != str_role.npos) || r_p->master_id.get_length()) && matches_release/* && atoi(find_release_artist->releases[release_ndx]->id) == _idtracker.release_id*/) {
				pos++;
				if (atoi(find_release_artist->releases[release_ndx]->id) == _idtracer.release_id) {
					_idtracer.release_index = pos;
					_idtracer.master_index = -1;
				}
				if (countreserve)
					reserve.first++;
				else
					m_vec_filter.emplace_back(std::pair<int, int>(pfc_infinite, release_ndx));

				release_done = true; master_done = true;
			}
		}

		if (bmaster)
			master_ndx++;
		else
			release_ndx++;
	}
	return reserve;
}

bool CFindReleaseDialog::get_cached_find_release_node(int lparam, pfc::string8& item, row_col_data& row_data) {
	bool bres = false;
	cache_t* cached = &m_cache_find_release_ptr->cache;
	auto it = cached->find(lparam);

	if (it != cached->end()) {
		item = it->second.first.col_data_list.begin()->second;
		row_data = it->second.first;
		bres = true;
	}
	return bres;

}

//todo: get rid of list_index, just need it as a flag
void CFindReleaseDialog::update_releases(const pfc::string8& filter, updRelSrc updsrc, bool init_expand) {
	m_release_ctree.EnableDispInfo(false);
	init_tracker();
	t_size the_artist = !find_release_artist ? pfc_infinite : atoi(find_release_artist->id);
	t_size treeview_artist = m_release_ctree.GetTreeViewArtist();

	bool artist_changed = true || treeview_artist != the_artist;
	bool delete_on_enter = updsrc == updRelSrc::Artist || updsrc == updRelSrc::ArtistList || artist_changed;
	bool rebuild_tree_cache = true;

	if (updsrc == updRelSrc::Filter || delete_on_enter) {
		_idtracer.release_reset();
	}

	pfc::array_t<pfc::string> lcf_words;
	bool filtered = tokenize_filter(filter, lcf_words);
	std::pair<t_size, t_size> vec_lv_reserve;

	if (delete_on_enter) {

		vec_lv_reserve = init_filter_i(filter, filter, false, true, false, true);
		m_vec_items_ptr->clear();
		m_vec_items_ptr->reserve(vec_lv_reserve.first + vec_lv_reserve.second);

		if (updsrc != updRelSrc::Filter) {
			if (m_cache_find_release_ptr != nullptr)
				m_cache_find_release_ptr->cache.clear();
			rebuild_tree_cache = true;
		}
		else {
			rebuild_tree_cache = false;
		}

	}

	if (!find_release_artist) {
		return;
	}

	t_size list_index = 0;
	t_size master_index = 0, release_index = 0;

	int item_data;
	std::pair<cache_iterator_t, bool> last_cache_emplaced;
	row_col_data row_data{};

	std::pair<t_size, t_size> reserve;
	if (rebuild_tree_cache) {
		reserve = init_filter_i("", "", false, true, true, true);
		m_cache_find_release_ptr->cache.reserve(find_release_artist->search_order_master.get_count());
	}

	bool brolemain_filter = conf.find_release_filter_flag & FilterFlag::RoleMain;
	std::string str_role;

	try {

		for (t_size walk = 0; walk < find_release_artist->search_order_master.get_size(); walk++) {

			bool is_master = find_release_artist->search_order_master[walk];
			pfc::string8 item; //master or release summary to match
			
			if (is_master) {
				//master
				str_role = std::string(find_release_artist->master_releases[master_index]->search_role);
				mounted_param thisparam(master_index, ~0, true, false);
				item_data = thisparam.lparam();
				if (updsrc != updRelSrc::Filter)
				{
					hook->set_master(&(find_release_artist->master_releases[master_index]));
					item = run_hook_columns(row_data, item_data);
				}
				else
					get_cached_find_release_node(item_data, item, row_data);
			}
			else {
				//no master release
				str_role = std::string(find_release_artist->releases[release_index]->search_role);
				mounted_param thisparam(~0, release_index, false, true);
				item_data = thisparam.lparam();
				if (updsrc != updRelSrc::Filter)
				{
					hook->set_release(&(find_release_artist->releases[release_index]));
					item = run_hook_columns(row_data, item_data);
				}
				else
					get_cached_find_release_node(item_data, item, row_data);
			}

			if (rebuild_tree_cache) {
				//CACHE EMPLACE
				last_cache_emplaced = m_cache_find_release_ptr->cache.emplace(
					item_data, std::pair<row_col_data, flagreg>
						{row_data, { m_cache_find_release_ptr->_ver, 0 }});
			}

			bool inserted = false; bool expanded = false;

			bool matches_role = !brolemain_filter || (str_role.find("Main") != str_role.npos);
			bool matches = matches_role && filtered && check_match(item, filter, lcf_words);

			int prev_expanded = 0;
			if (matches) {

				if (is_master)
					_idtracer.release_check(find_release_artist->master_releases[master_index]->id, list_index,
						is_master, list_index, master_index);
				else
					_idtracer.release_check(find_release_artist->releases[release_index]->id, list_index,
						is_master, list_index, master_index);

				// MASTER OR NO MASTER RELEASES
				
				if (delete_on_enter) {
					mounted_param myparam((LPARAM)item_data);
					if (myparam.is_master()) {
						cache_iterator_t insert_cache_it = m_cache_find_release_ptr->cache.find(myparam.lparam());
						insert_cache_it->second.second.flag = 0;
						insert_cache_it->second.second.ver = m_cache_find_release_ptr->_ver;
						int ival = 1;
						m_cache_find_release_ptr->SetCacheFlag(insert_cache_it, NodeFlag::none, &ival);						
						std::pair<cache_iterator_t, HTREEITEM> vec_row(insert_cache_it, nullptr);
						//VEC PUSH
						m_vec_items_ptr->push_back(std::pair<cache_iterator_t, HTREEITEM>(insert_cache_it, (HTREEITEM)nullptr));
					}
					else {
						if (myparam.is_release()) {
							;
						}
						else {

							cache_iterator_t insert_cache_it = m_cache_find_release_ptr->cache.find(myparam.lparam());
							insert_cache_it->second.second.flag = 0;
							insert_cache_it->second.second.ver = m_cache_find_release_ptr->_ver;
							int ival = 0;
							m_cache_find_release_ptr->SetCacheFlag(insert_cache_it, NodeFlag::added, &ival);
							flagreg& insfr = insert_cache_it->second.second;
							std::pair<cache_iterator_t, HTREEITEM> vecrow(insert_cache_it, (HTREEITEM)nullptr);
							//VEC PUSH
							m_vec_items_ptr->push_back(std::pair<cache_iterator_t, HTREEITEM>(vecrow));
						}
					}
				}

				if (init_expand && is_master) {
					/*int prev_expanded;*/
					int expanded = updsrc == updRelSrc::Filter ? 0 : 1;
					bool bnew = !get_node_expanded(row_data.id, prev_expanded);
				}

				list_index++;
				inserted = true;
			}

			if (is_master) {
				// SUBITEMS
				const MasterRelease_ptr* master_release = &(find_release_artist->master_releases[master_index]);

				for (size_t j = 0; j < master_release->get()->sub_releases.get_size(); j++) {

					mounted_param thisparam = { master_index, j, true, true };
					item_data = thisparam.lparam();
					pfc::string8 sub_item;
					if (updsrc != updRelSrc::Filter)
					{
						hook->set_release(&(find_release_artist->master_releases[master_index]->sub_releases[j]));
						sub_item = run_hook_columns(row_data, item_data);
					}
					else
						get_cached_find_release_node(item_data, sub_item, row_data);

					std::pair<cache_iterator_t, bool> last_cache_subitem_emplaced =
					{ m_cache_find_release_ptr->cache.end(), false };

					if (rebuild_tree_cache) {
						//CACHE EMPLACE
						last_cache_subitem_emplaced =
							m_cache_find_release_ptr->cache.emplace(
								item_data,
								std::pair<row_col_data, flagreg> {row_data, { m_cache_find_release_ptr->_ver, 0 }}
						);
					}
					
					bool matches = matches_role && filtered && check_match(sub_item, filter, lcf_words);
				
					if (matches) {

						int master_id = atoi(find_release_artist->master_releases[master_index]->id);

						vec_iterator_t parent;
						find_vec_by_id(*m_vec_items_ptr, master_id, parent);
						inserted = parent != m_vec_items_ptr->end();

						if (!inserted) {

							//new oct'21 1.1.0
							// INSERT NOT_MATCHING PARENT

							if (delete_on_enter) {
								mounted_param not_matching_master_param(master_index, ~0, true, false);
								
								cache_iterator_t insert_cache_it = m_cache_find_release_ptr->cache.find(not_matching_master_param.lparam());
								insert_cache_it->second.second.flag = 0;
								insert_cache_it->second.second.ver = m_cache_find_release_ptr->_ver;
								int ival = 1;
								m_cache_find_release_ptr->SetCacheFlag(insert_cache_it, NodeFlag::none, &ival);
								std::pair<cache_iterator_t, HTREEITEM> vec_row(insert_cache_it, nullptr);
								//VEC PUSH
								m_vec_items_ptr->push_back(std::pair<cache_iterator_t, HTREEITEM>(insert_cache_it, (HTREEITEM)nullptr));
							
							}

							////TODO 1.0.5: unknown test scenario, test build tree with cache, offline, db & online
							//foo_discogs_exception ex;
							//ex << "Error building release list item [parent not found]";
							//throw ex;
							//return;
							////PFC_ASSERT(false);
						}

						// SUBRELEASE
						if (init_expand && prev_expanded == 1) {

							/*m_vec_build_lv_items.at(m_vec_build_lv_items.size() - 1).second == 1*/

							_idtracer.release_check(master_release->get()->sub_releases[j]->id, list_index,
								false, list_index - j, master_index);

							//insert
							
							if (delete_on_enter) {
								mounted_param myparam((LPARAM)item_data);
								if (myparam.is_master()) {
									cache_iterator_t insit = last_cache_emplaced.first;
									flagreg& insfr = last_cache_emplaced.first->second.second;
									//VEC PUSH
									m_vec_items_ptr->push_back(std::pair<cache_iterator_t, HTREEITEM>
										(last_cache_emplaced.first, nullptr));
								}
								else {
									if (myparam.is_release()) {
										;
									}
									else {
										cache_iterator_t insit = last_cache_emplaced.first;
										flagreg& insfr = last_cache_emplaced.first->second.second;
									}
								}
							}
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

	int list_count = m_vec_items_ptr->size();

	m_release_ctree.EnableDispInfo(true);
	m_release_ctree.SetCache(std::shared_ptr<filter_cache>(m_cache_find_release_ptr));
	m_release_ctree.SetVec(std::shared_ptr<vec_t>(m_vec_items_ptr), _idtracer);
	return;
}

void CFindReleaseDialog::expand_releases(const pfc::string8& filter, updRelSrc updsrc, t_size master_index, t_size master_list_pos) {
	
	bool bsub_release_filter = conf.find_release_filter_flag & FilterFlag::Versions;
		
	pfc::array_t<pfc::string> lcf_words;
	bool filtered = tokenize_filter(filter, lcf_words);

	int list_index = master_list_pos + 1;

	int walk_lparam;
	row_col_data row_data;

	try {
		int skipped = 0;
		mounted_param parent_mp(master_index, ~0, true, false);
		int parent_lparam = parent_mp.lparam();
		cache_t::const_iterator parent_cache = m_cache_find_release_ptr->cache.find(parent_lparam);
		cache_t::const_iterator new_release_cache;
		if (parent_cache == m_cache_find_release_ptr->cache.end()) {
			int debug = 1;
		}
		try {
			hook->set_master(nullptr);
			for (size_t j = 0; j < find_release_artist->master_releases[master_index]->sub_releases.get_size(); j++) {

				row_col_data row_data_subitem;
				hook->set_release(&(find_release_artist->master_releases[master_index]->sub_releases[j]));
				hook->set_master(&(find_release_artist->master_releases[master_index]));
				mounted_param thisparam = { master_index, j, true, true };
				walk_lparam = thisparam.lparam();

				pfc::string8 sub_item;
				//todo: can we avoid this check?
				if (!get_cached_find_release_node(thisparam.lparam(), sub_item, row_data)) {
					sub_item = run_hook_columns(row_data, walk_lparam);
					//CACHE EMPLACE
					m_cache_find_release_ptr->cache.emplace(
						walk_lparam, std::pair<row_col_data, flagreg>
							{row_data, { m_cache_find_release_ptr->_ver, 0 }/*walk_lparam*/});
				}
				
				bool filter_match = !bsub_release_filter || filtered && check_match(sub_item, filter, lcf_words);

				if (filter_match) {
					cache_iterator_t walk_children =
						m_cache_find_release_ptr->cache.find(walk_lparam);

					int ival = 1;
					m_cache_find_release_ptr->SetCacheFlag(walk_children, NodeFlag::spawn, &ival);
					//in filter
					m_cache_find_release_ptr->SetCacheFlag(walk_children, NodeFlag::filterok, &ival);

					int row_pos = list_index + j - skipped;
					_idtracer.release_check(find_release_artist->master_releases[master_index]->sub_releases[j]->id,
						row_pos, false, master_list_pos, master_index);
				}
				else {
					skipped++;
				}
			}
		}
		catch (foo_discogs_exception& e) {
			foo_discogs_exception ex;
			ex << "Error formating release list item [" << e.what() << "]";
			throw ex;
		}
		//set as tree children not done so branches are generated in geteditinfo
		m_cache_find_release_ptr->SetCacheFlag(parent_lparam, NodeFlag::tree_created, false);

		//expand branch
		m_release_ctree.ExpandHit();
		
	}
	catch (foo_discogs_exception& e) {
		foo_discogs_exception ex;
		ex << "Error formating release list subitem [" << e.what() << "]";
		throw ex;
	}
	return;
}

pfc::string8 CFindReleaseDialog::get_param_id(mounted_param myparam) {

	pfc::string8 res_selected_id;
	if (myparam.is_master())
		res_selected_id = find_release_artist->master_releases[myparam.master_ndx]->main_release_id;
	//res_selected_id = find_release_artist->master_releases[myparam.master_ndx]->id;
	else
		if (myparam.is_release()) {
			MasterRelease_ptr dbugmasrel = find_release_artist->master_releases[myparam.master_ndx];
			//Release_ptr dbugrel = find_release_artist->master_releases[myparam.master_ndx]->sub_releases[myparam.release_ndx];
			res_selected_id = find_release_artist->master_releases[myparam.master_ndx]->sub_releases[myparam.release_ndx]->id;
		}
		else
			res_selected_id = find_release_artist->releases[myparam.release_ndx]->id;
	return res_selected_id;
}

int CFindReleaseDialog::get_param_id_master(mounted_param myparam) {
	if (myparam.is_master())
		return atoi(find_release_artist->master_releases[myparam.master_ndx]->id);
	else
		return pfc_infinite;
}


void CFindReleaseDialog::on_release_selected(int src_lparam) {

	vec_iterator_t vecpos;
	find_vec_by_lparam(*m_vec_items_ptr, src_lparam, vecpos);

	if (vecpos == m_vec_items_ptr->end())
		return;

	int selection_index = std::distance(m_vec_items_ptr->begin(), vecpos);
	mounted_param myparam(m_vec_items_ptr->at(selection_index).first->first);

	if (myparam.is_master()) {

		pfc::string8 release_url;
		if (_idtracer.amr && (get_param_id_master(myparam) == _idtracer.master_id))
			release_url.set_string(std::to_string(_idtracer.release_id).c_str());
		else
			release_url.set_string(get_param_id(myparam));

		if (m_cache_find_release_ptr->GetCacheFlag(src_lparam, NodeFlag::expanded)) {

			MasterRelease_ptr& master_release = find_release_artist->master_releases[myparam.master_ndx];

			if (master_release->sub_releases.get_size()) {
				abort_callback_dummy dummy;
				fake_threaded_process_status fstatus;
				on_expand_master_release_done(master_release, selection_index, fstatus, dummy);
			}
			else {
				//spawn
				MasterRelease_ptr master_rel = find_release_artist->master_releases[myparam.master_ndx];
				expand_master_release(master_rel, selection_index);
			}
		}
		else
		{
			//closing master branch
			uSetWindowText(m_release_url_edit, release_url);
		}
	}
	else {
		uSetWindowText(m_release_url_edit, get_param_id(myparam).get_ptr());
	}
}

LRESULT CFindReleaseDialog::ApplyFilter(pfc::string8 strFilter, bool force_redraw, bool force_rebuild) {

	strFilter = trim(strFilter);

	//TODO: mutex?
	if (!find_release_artist) {
		return false;
	}
	m_cache_find_release_ptr->_ver++;
	
	if (!force_rebuild && strFilter.get_length() == 0) {
		m_vec_filter.clear();
		_idtracer.release_reset();
		update_releases(strFilter, updRelSrc::Filter, true);
	}
	else {
		//same filter, exit ...
		if (!force_redraw && !stricmp_utf8(m_results_filter, strFilter)) {
			return FALSE;
		}

		m_vec_items_ptr->clear();
		TreeView_DeleteAllItems(m_release_tree);
		m_cache_find_release_ptr->_ver++;

		_idtracer.release_reset();

		//INIT FILTER
		init_filter_i(strFilter, strFilter, false, true, false, false);

		//reset release position (but keep ids)
		_idtracer.release_reset();

		std::vector<std::pair<int, int>>::iterator it;
		t_size list_index = -1;

		//DISPLAY FILTER RESULTS

		for (it = m_vec_filter.begin(); it != m_vec_filter.end(); it++) {
			list_index++;
			LPARAM lparam = MAKELPARAM(0, 0);
			t_size master_ndx = it->first;
			t_size release_ndx = it->second;
			bool bmaster = it->first != pfc_infinite;
			bool brelease = it->second != pfc_infinite;
			mounted_param myparam = { master_ndx, release_ndx, bmaster, brelease };
			lparam = myparam.lparam();
			pfc::string8 item;
			row_col_data row_data;

			if (myparam.is_master()) {
				hook->set_master(&(find_release_artist->master_releases[myparam.master_ndx]));
			}
			else {
				cache_iterator_t parent_it;
				if (myparam.is_release()) {
					std::vector<std::pair<int, int>>::iterator it_parent;
					int master_id = atoi(find_release_artist->master_releases[myparam.master_ndx]->id);
					mounted_param parent_mp(myparam.master_ndx, ~0, true, false);
					int parent_lparam = parent_mp.lparam();

					vec_iterator_t parent;
					find_vec_by_lparam(*m_vec_items_ptr, parent_lparam, parent);
					if (parent != m_vec_items_ptr->end()) {

						parent_it = parent->first;

						//PARENT FOUND
						
						bool bexp;
						bool bthisver = m_cache_find_release_ptr->GetCacheFlag(parent->first, NodeFlag::expanded, &bexp);
						int ival = bexp ? 1 : 0;
						m_cache_find_release_ptr->SetCacheFlag(parent->first, NodeFlag::expanded, &ival);

					}
					else {
						//INSERT NOT-MATCHING PARENT							
						mounted_param mp_master(master_ndx, ~0, true, false);

						bool bsub_release_filter = conf.find_release_filter_flag & FilterFlag::Versions;
						bool brolemain_filter = conf.find_release_filter_flag & FilterFlag::RoleMain;
						if (!bsub_release_filter) {
							//parent match?
							pfc::array_t<pfc::string> lcf_words;
							bool filtered = tokenize_filter(strFilter, lcf_words);
							MasterRelease_ptr mr_p = find_release_artist->master_releases[master_ndx];
							if (filtered) {
								std::string str_role = std::string(mr_p->search_role);
								bool matches_master = (!brolemain_filter || (str_role.find("Main") != str_role.npos)) && check_match(mr_p->title, strFilter, lcf_words);
								if (!matches_master) {
									int ival = 0;
									m_cache_find_release_ptr->SetCacheFlag(it->first, NodeFlag::filterok, &ival);
									continue;
								}
							}
						}

						get_cached_find_release_node(mp_master.lparam(), item, row_data);
						parent_it = m_cache_find_release_ptr->cache.find(mp_master.lparam());

						bool bexp;
						bool bthisver = m_cache_find_release_ptr->GetCacheFlag(parent_it, NodeFlag::expanded, &bexp);
						int ival = bexp ? 1 : 0;
						m_cache_find_release_ptr->SetCacheFlag(parent_it, NodeFlag::expanded, &ival);

						ival = 1;
						m_cache_find_release_ptr->SetCacheFlag(parent_it, NodeFlag::added, &ival);
						//in filter
						m_cache_find_release_ptr->SetCacheFlag(parent_it, NodeFlag::filterok, &ival);

						flagreg& insfr = parent_it->second.second;
						//VEC PUSH
						m_vec_items_ptr->push_back(std::pair<cache_iterator_t, HTREEITEM>(parent_it, nullptr));

						vec_iterator_t  master_row;
						find_vec_by_id(*m_vec_items_ptr, master_id, master_row);

					}
					hook->set_master(&(find_release_artist->master_releases[myparam.master_ndx]));
					hook->set_release(&(find_release_artist->master_releases[myparam.master_ndx]->sub_releases[myparam.release_ndx]));
				}
				else
					hook->set_release(&(find_release_artist->releases[myparam.release_ndx]));
			}

			//todo: mutex in cache
			if (get_cached_find_release_node(lparam, item, row_data) != true) {
				//impatient...
				return FALSE;
			}
			//

			cache_iterator_t filtered_it = m_cache_find_release_ptr->cache.find(lparam);
			int ival = 1;
			m_cache_find_release_ptr->SetCacheFlag(filtered_it, NodeFlag::filterok, &ival);
			//VEC_PUSH
			m_vec_items_ptr->push_back(std::pair<cache_iterator_t, HTREEITEM>(filtered_it, nullptr));
		}

		int counter = 0;
		std::vector<HTREEITEM> v_hItem_filtered_masters;
		for (auto& walk : *m_vec_items_ptr)
		{
			//todo: recycle previously expanded nodes?
			if (!mounted_param(walk.first->first).is_master() &&
				!mounted_param(walk.first->first).is_nmrelease()) {
				continue;
			}
			
			TVINSERTSTRUCT tvis = { 0 };
			tvis.item.pszText = LPSTR_TEXTCALLBACK;
			tvis.item.iImage = I_IMAGECALLBACK;
			tvis.item.iSelectedImage = I_IMAGECALLBACK;
			tvis.item.cChildren = I_CHILDRENCALLBACK;
			tvis.item.lParam = walk.first->first; /* whatever I need to identify this item and generate its contents later */;
			tvis.hParent = TVI_ROOT; //pNMTreeView->itemNew.hItem;
			tvis.hInsertAfter = TVI_LAST;

			if (walk.first->second.first.id == _idtracer.master_id) {
				tvis.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM | TVIF_STATE;
				tvis.item.state = TVIS_BOLD;
				tvis.item.stateMask = TVIS_BOLD;
			}
			else {
				tvis.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM;
			}

			HTREEITEM newnode = TreeView_InsertItem(m_release_tree, &tvis);
			walk.second = newnode;
			int ival = 0;
			m_cache_find_release_ptr->SetCacheFlag(walk.first, NodeFlag::tree_created, &ival);
			bool val;
			if (m_cache_find_release_ptr->GetCacheFlag(walk.first, NodeFlag::expanded, &val)) {
				v_hItem_filtered_masters.emplace_back(newnode);
			}
		}

		for (auto hnode : v_hItem_filtered_masters) {
			TreeView_Expand(m_release_tree, hnode, TVM_EXPAND);
		}
	}

	m_results_filter.set_string(strFilter);
	return FALSE;
}


LRESULT CFindReleaseDialog::OnEditFilterText(WORD wNotifyCode, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	if (!m_DisableFilterBoxEvents && wNotifyCode == EN_CHANGE) {
		m_skip_fill_filter = true;

		pfc::string8 strFilter;
		uGetWindowText(m_filter_edit, strFilter);

		if (trim(strFilter).length()) {
			KFilterTimerOn(m_hWnd);
		}
		else {
			KillTimer(KTypeFilterTimerID);
			ApplyFilter(strFilter);
		}

	}
	return false;
}

LRESULT CFindReleaseDialog::OnSelectArtist(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	t_size pos = ListView_GetSingleSelection(m_artist_list);
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

LRESULT CFindReleaseDialog::OnArtistListViewItemChanged(int, LPNMHDR hdr, BOOL&) {
	NMLISTVIEW* lpStateChange = reinterpret_cast<NMLISTVIEW*>(hdr);
	if ((lpStateChange->uNewState ^ lpStateChange->uOldState) & LVIS_SELECTED) {
		if (lpStateChange->uNewState & LVIS_SELECTED) {
			int pos = lpStateChange->iItem;
			if (pos != m_artist_index) {
				int fr_artists_count = find_release_artists.get_count();

				if (find_release_artist && (!find_release_artists.get_count() || find_release_artists[pos].get()->id == find_release_artist.get()->id)) {
					//nothing to do, a list with just one artist, from a search based on artist id?
				}
				else {
					get_selected_artist_releases(updRelSrc::ArtistList);
				}
			}
			m_artist_index = pos;
		}
	}
	return 0;
}

LRESULT CFindReleaseDialog::OnCheckOnlyExactMatches(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	if (wID == IDC_ONLY_EXACT_MATCHES_CHECK) {
		conf.display_exact_matches = IsDlgButtonChecked(IDC_ONLY_EXACT_MATCHES_CHECK) != 0;
		fill_artist_list(_idtracer.amr, updRelSrc::Artist);

		get_selected_artist_releases(updRelSrc::Artist);
	}
	return FALSE;
}

LRESULT CFindReleaseDialog::OnCheckFindReleaseFilterFlags(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	
	bool force_refresh, force_rebuild;

	if (wID == IDC_CHECK_FIND_RELEASE_FILTER_ROLEMAIN) {
		bool checked = IsDlgButtonChecked(IDC_CHECK_FIND_RELEASE_FILTER_ROLEMAIN) != 0;
		if (checked)
			conf.find_release_filter_flag |= FilterFlag::RoleMain;
		else
			conf.find_release_filter_flag &= ~FilterFlag::RoleMain;
		force_refresh = force_rebuild = true;
	}
	else if (wID == IDC_CHECK_FIND_RELEASE_FILTER_VERSIONS) {
		bool checked = IsDlgButtonChecked(IDC_CHECK_FIND_RELEASE_FILTER_VERSIONS) != 0;
		if (checked)
			conf.find_release_filter_flag |= FilterFlag::Versions;
		else
			conf.find_release_filter_flag &= ~FilterFlag::Versions;
		force_refresh = true; force_rebuild = false;
	}
	else {
		return FALSE;
	}
	
	pfc::string8 strFilter;
	uGetWindowText(m_filter_edit, strFilter);
	KillTimer(KTypeFilterTimerID);
	ApplyFilter(strFilter, force_refresh, force_rebuild);

	return FALSE;
}

LRESULT CFindReleaseDialog::OnButtonSearch(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	m_artist_index = pfc_infinite;

	pfc::string8 artistname;
	uGetWindowText(m_search_edit, artistname);
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

void CFindReleaseDialog::on_write_tags(const pfc::string8& release_id) {
	pfc::string8 assertArtistId = "";
	pfc::string8 offlineArtistId = "";

	if (find_release_artist)
		assertArtistId = find_release_artist->id;
	else
		if (find_release_artists.get_count() > 0)
			assertArtistId = find_release_artists[0]->id;

	if ((conf.skip_mng_flag & SkipMng::SKIP_FIND_RELEASE_DLG_IDED) && _idtracer.artist_id != pfc_infinite) {
		offlineArtistId = pfc::toString(_idtracer.artist_id).get_ptr();

		if (assertArtistId.get_length() && atoi(assertArtistId) != _idtracer.artist_id) {
			offlineArtistId = assertArtistId;
			_idtracer.artist_reset(); _idtracer.master_reset();  _idtracer.release_reset();
		}
	}
	else {
		offlineArtistId = assertArtistId;
	}

	service_ptr_t<process_release_callback> task = new service_impl_t<process_release_callback>(this, release_id, offlineArtistId, "", items);
	task->start(m_hWnd);
}

void CFindReleaseDialog::extract_id_from_url(pfc::string8& s) {
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

void CFindReleaseDialog::extract_release_from_link(pfc::string8& s) {
	size_t pos = s.find_first("[r");
	if (pos == 0) {
		s = substr(s, 2, s.get_length() - 3);
	}
}

bool CFindReleaseDialog::set_node_expanded(t_size master_id, int& state, bool build) {
	vec_iterator_t master_row;
	find_vec_by_id(*m_vec_items_ptr, master_id, master_row);
	if (master_row != m_vec_items_ptr->end()) {
		int ival = state;
		m_cache_find_release_ptr->SetCacheFlag(master_row->first, NodeFlag::expanded, &ival);
		if (ival) {
			int ival = 1;
			m_cache_find_release_ptr->SetCacheFlag(master_row->first, NodeFlag::added, &ival);
		}
		return true;
	}
	else {
		state = -1;
		return false;
	}
}

bool CFindReleaseDialog::get_node_expanded(t_size master_id, int& out_state) {
	vec_iterator_t  master_row;
	find_vec_by_id(*m_vec_items_ptr, master_id, master_row);
	if (master_row != m_vec_items_ptr->end()) {
		m_cache_find_release_ptr->SetCacheFlag(master_row->first, NodeFlag::expanded, &out_state);
		return true;
	}
	else {
		out_state = -1;
		return false;
	}
}

void CFindReleaseDialog::on_expand_master_release_done(const MasterRelease_ptr& master_release, int list_index, threaded_process_status& p_status, abort_callback& p_abort) {

	int master_i = -1;
	//todo: mutex find_release_artist (artist list -> on item selected)
	for (size_t i = 0; i < find_release_artist->master_releases.get_size(); i++) {
		if (find_release_artist->master_releases[i]->id == master_release->id) {
			master_i = i;
			break;
		}
	}

	if (master_i == -1)
		return; //do not expand artist selection changed

	pfc::string8 filter;
	uGetWindowText(m_filter_edit, filter);

	int expanded = 1;
	bool bnodeset = set_node_expanded(atoi(master_release->id), expanded, false);

	if (bnodeset) {
		expand_releases(filter, updRelSrc::Releases, master_i, list_index);

		int ival = 1;
		mounted_param mparam(master_i, ~0, true, false);
		t_size lparam = mparam.lparam();
		m_cache_find_release_ptr->SetCacheFlag(lparam, NodeFlag::expanded, ival);
		//..

		//set_lvstate_expanded(list_index, expanded, false);
	}

	mounted_param myparam(m_vec_items_ptr->at(list_index).first->first);
	pfc::string8 release_url;

	if (_idtracer.amr && (atoi(master_release->id) == _idtracer.master_id))
		release_url.set_string(std::to_string(_idtracer.release_id).c_str());
	else
		release_url.set_string(get_param_id(myparam));

	uSetWindowText(m_release_url_edit, release_url);
}

void CFindReleaseDialog::on_expand_master_release_complete() {

	m_active_task = NULL;
}

void CFindReleaseDialog::expand_master_release(MasterRelease_ptr& release, int pos) {
	if (release->sub_releases.get_size()) {
		return;
	}

	pfc::string8 offlineArtistId = "";
	if (find_release_artist)
		offlineArtistId = find_release_artist->id;
	else
		if (find_release_artists.get_count() > 0)
			offlineArtistId = find_release_artists[0]->id;

	service_ptr_t<expand_master_release_process_callback> task =
		new service_impl_t<expand_master_release_process_callback>(release, pos, offlineArtistId);

	m_active_task = &task;
	task->start(m_hWnd);
}

void CFindReleaseDialog::get_selected_artist_releases(updRelSrc updsrc) {

	t_size pos = ListView_GetSingleSelection(m_artist_list);

	if (pos != ~0) {
		Artist_ptr& artist = find_release_artists[pos];

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

	pfc::string8 url_artist_id = "";
	if (pfc::string_has_prefix(artistname, "https://www.discogs.com/artist/")) {
		url_artist_id = extract_max_number(artistname);
		if (url_artist_id.get_length()) {
			_idtracer.artist_reset();
			m_artist_index = pfc_infinite;
			find_release_artists.force_reset();
			ListView_DeleteAllItems(m_artist_list);
			::EnableWindow(uGetDlgItem(IDC_ONLY_EXACT_MATCHES_CHECK), false);
		}
		else {
			return; //artist id not found
		}
	}

	bool cfg_skip_search_artist_if_present = true;
	if (url_artist_id.get_length() || (cfg_skip_search_artist_if_present
		&& !skip_tracer && _idtracer.artist && _idtracer.artist_id != pfc_infinite)) {

		pfc::string8 artist_id = url_artist_id.get_length()? url_artist_id : pfc::toString(_idtracer.artist_id).get_ptr();

		if (artist_id.get_length()) {
			service_ptr_t<get_artist_process_callback> task =
				new service_impl_t<get_artist_process_callback>(updRelSrc::Undef, artist_id.get_ptr());
			task->start(m_hWnd);
		}
	}
	else {

		DBFlags dlg_db_dc_flags(conf.db_dc_flag);
		bool db_isready = DBFlags(conf.db_dc_flag).IsReady();		
		dlg_db_dc_flags.SwitchFlag(DBFlags::DB_SEARCH_LIKE, db_isready && IsDlgButtonChecked(IDC_CHECK_FIND_REL_DB_SEARCH_LIKE));
		dlg_db_dc_flags.SwitchFlag(DBFlags::DB_SEARCH_ANV, db_isready && IsDlgButtonChecked(IDC_CHECK_FIND_REL_DB_SEARCH_ANV));

		service_ptr_t<search_artist_process_callback> task =
			new service_impl_t<search_artist_process_callback>(artistname.get_ptr(), dlg_db_dc_flags.GetFlag());
		task->start(m_hWnd);
	}
}
