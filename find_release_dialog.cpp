#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include "stdafx.h"
#include <CommCtrl.h>
#include <atlctrls.h>

#include "libPPUI\clipboard.h"

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
	int width = conf.find_release_dialog_width;
	int height = conf.find_release_dialog_height;
	CRect rcCfg(0, 0, width, height);
	::CenterWindow(this->m_hWnd, rcCfg, core_api::get_main_window());
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

	conf.display_exact_matches = IsDlgButtonChecked(IDC_ONLY_EXACT_MATCHES_CHECK) != 0;

	bool local_display_exact_matches = IsDlgButtonChecked(IDC_ONLY_EXACT_MATCHES_CHECK);
	if (CONF.display_exact_matches != conf.display_exact_matches &&
		conf.display_exact_matches != local_display_exact_matches) {

		conf.display_exact_matches = local_display_exact_matches;
		bres = bres || true;
	}

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
	
	const bool benabled = uButton_GetCheck(m_hWnd, IDC_CHECK_RELEASE_SHOW_ID);
	if (benabled != conf.find_release_dialog_show_id) {
		conf.find_release_dialog_show_id = benabled;
		bres = bres || true;
	}
	return bres;
}

LRESULT CFindReleaseDialog::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	pushcfg();
	KillTimer(KTypeFilterTimerID);
	return 0;
}

LRESULT CFindReleaseDialog::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	pfc::string8 release_id;
	uGetWindowText(m_release_url_edit, release_id);
	release_id.set_string(trim(release_id));
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
}

bool OAuthCheck(foo_discogs_conf conf) {
	if (!g_discogs->gave_oauth_warning &&
		(!conf.oauth_token.get_length() || !conf.oauth_token_secret.get_length())) {
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
	return true;
}

LRESULT CFindReleaseDialog::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {

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

	CRect rc_artists;
	::GetWindowRect(m_artist_list, &rc_artists);
	int icol = listview_helper::fr_insert_column(m_artist_list, 0,
		"Artist", rc_artists.Width(), LVCFMT_LEFT);

	cewb_artist_search.SubclassWindow(m_search_edit);
	cewb_release_filter.SubclassWindow(m_filter_edit);
	cewb_release_url.SubclassWindow(m_release_url_edit);
	SetEnterKeyOverride(conf.release_enter_key_override);

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

	DlgResize_Init(mygripp.grip, true);
	load_size();

	if (conf.find_release_dialog_show_id) {
		BOOL bdummy = false;
		conf.find_release_dialog_show_id = false;
		//OnCheckReleasesShowId(0, 0, NULL, bdummy);
	}

	modeless_dialog_manager::g_add(m_hWnd);

	if (OAuthCheck(conf)) {
		if (conf.skip_find_release_dialog_if_ided && _idtracer.release) {
			on_write_tags(pfc::toString(_idtracer.release_id).c_str());

		}
		else {
			if (_idtracer.release && _idtracer.release_id != pfc_infinite)
				uSetWindowText(m_release_url_edit, pfc::toString(_idtracer.release_id).c_str());

			show();

			m_DisableFilterBoxEvents = true;
    		if (!_idtracer.amr) {
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
		find_release_artists = artist_exact_matches;
		bool exact_matches = IsDlgButtonChecked(IDC_ONLY_EXACT_MATCHES_CHECK) != 0;
		if (!exact_matches) {
			find_release_artists.append(artist_other_matches);
		}
		int artists_index = 0, list_index = 0;
		for (size_t i = 0; i < find_release_artists.get_size(); i++) {
			pfc::string8 artist_name = find_release_artists[i]->name;
			_idtracer.artist_check(find_release_artists[i]->id, i);
			bool isdropindex = (list_index == _idtracer.artist_index);
			listview_helper::insert_lvItem_tracer(
				m_artist_list, list_index, atoi(find_release_artists[i]->id) == _idtracer.artist_id,
				artist_name.get_ptr(), artists_index);

			artists_index++;
			list_index++;
		}

		if (find_release_artists.get_size()) {
			ListView_SetItemState(m_artist_list, 0, LVIS_SELECTED, LVIS_SELECTED);
			m_artist_index = 0;
		}
	}
	else if (updsrc == updRelSrc::Undef) {
		::EnableWindow(exactcheck, false);
		pfc::string8 name = find_release_artist->name;
		_idtracer.artist_check(find_release_artist->id, 0);

		listview_helper::insert_lvItem_tracer(m_artist_list, 0,
			_idtracer.artist_id == atoi(find_release_artist->id),
			name.get_ptr(), (LPARAM)0);

		if (!_idtracer.release) {
			find_release_artists.append_single(std::move(find_release_artist));
			set_artist_item_param(m_artist_list, 0, 0, (LPARAM)0);
			ListView_SetItemState(m_artist_list, 0, LVIS_SELECTED, LVIS_SELECTED);
			m_artist_index = 0;
		}
	}
	else if (updsrc == updRelSrc::ArtistList) {
		::EnableWindow(exactcheck, true);
	}
}

auto CFindReleaseDialog::find_vec_by_lparam(int lparam) {
	return std::find_if(m_vec_items.begin(), m_vec_items.end(),
		[lparam](const std::pair<cache_iterator_t, HTREEITEM*> e) {
			return e.first->first == lparam; });
}
auto CFindReleaseDialog::find_vec_by_id(int id) {
	return std::find_if(m_vec_items.begin(), m_vec_items.end(),
		[&](const std::pair<cache_iterator_t, HTREEITEM*> e) {
			return e.first->second.first.id == id; });
}

void CFindReleaseDialog::on_get_artist_done(updRelSrc updsrc, Artist_ptr& artist) {

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

	m_results_filter = filter;

	update_releases(m_results_filter, updsrc, true);

	m_DisableFilterBoxEvents = true;
	uSetWindowText(m_filter_edit, filter);
	m_DisableFilterBoxEvents = false;
	if (m_vec_items.size()) {
		int src_lparam = -1;
		mounted_param myparam;
		if (_idtracer.amr) {
			myparam = { _idtracer.master_i, _idtracer.release_i, _idtracer.master,_idtracer.release };
			src_lparam = get_mounted_lparam(myparam);
			//select if release is loaded
			auto vec_lv_it = find_vec_by_lparam(src_lparam);

			if (vec_lv_it != m_vec_items.end()) {
				src_lparam = get_mounted_lparam(myparam);
				on_release_selected(src_lparam);
			}
			else {
				//release not loaded, try to select master and expand
				mounted_param not_found;
				myparam = mounted_param(_idtracer.master_i, ~0, _idtracer.master, false);
				src_lparam = get_mounted_lparam(myparam);
				auto vec_lv_it = find_vec_by_lparam(src_lparam);

				if (vec_lv_it != m_vec_items.end()) {
					src_lparam = get_mounted_lparam(myparam);
					int ival = 1;
					if (updsrc == updRelSrc::Undef) {
						SetCacheFlag(vec_lv_it->first, NodeFlag::expanded, &ival);

						//prepare to m_hHit node
						CTreeViewCtrl tree(m_release_tree);
						HTREEITEM first = tree.GetRootItem();
						for (HTREEITEM walk = first; walk != NULL; walk = tree.GetNextVisibleItem(walk)) {
							TVITEM pItemMaster = { 0 };
							pItemMaster.mask = TVIF_PARAM;
							pItemMaster.hItem = walk;
							TreeView_GetItem(m_release_tree, &pItemMaster);
							if (pItemMaster.lParam == src_lparam) {
								m_hHit = walk;
								break;
							}
						}
					}
					on_release_selected(src_lparam);
				}
				else {
					//release not loaded, trying to select master to expand
					mounted_param not_found;
					myparam = mounted_param(~0, _idtracer.release_i, false, _idtracer.release);
					src_lparam = get_mounted_lparam(myparam);
					auto vec_lv_it = find_vec_by_lparam(src_lparam);

					if (vec_lv_it != m_vec_items.end()) {
						src_lparam = get_mounted_lparam(myparam);
						int ival = 1;
						
						//note: update_releases() updated master info

						SetCacheFlag(vec_lv_it->first, NodeFlag::expanded, &ival);
						on_release_selected(src_lparam);
					}
				}

				//spawn (reenters update_releases as updSrc::Releases)
	            //select_release(0);
			}

		}
		else {
			if (_idtracer.release_i != pfc_infinite && _idtracer.release_index != -1) {
				auto vec_lv_it = find_vec_by_id(_idtracer.master_id);

				if (vec_lv_it != m_vec_items.end()) {
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

void CFindReleaseDialog::OnFinalMessage(HWND /*hWnd*/) {
	modeless_dialog_manager::g_remove(m_hWnd);
	delete this;
}

void CFindReleaseDialog::on_search_artist_done(pfc::array_t<Artist_ptr>& p_artist_exact_matches, const pfc::array_t<Artist_ptr>& p_artist_other_matches) {

	artist_exact_matches = p_artist_exact_matches;
	artist_other_matches = p_artist_other_matches;

	if (!artist_exact_matches.get_size() && artist_other_matches.get_size()) {
		CheckDlgButton(IDC_ONLY_EXACT_MATCHES_CHECK, false);
	}

	//spawn

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

	mounted_param myparam = mount_param((LPARAM)item_data);
	pfc::string8 search_formatted;
	pfc::string8 id;

	row_data.col_data_list.clear();

	for (unsigned int i = 0; i < vec_icol_subitems.size(); i++)
	{
		cfgcol walk_cfg = cfg_lv.colmap.at(vec_icol_subitems[i].first);

		if (myparam.is_master()) {
			CONF.search_master_format_string->run_hook(location, &info, hook.get(), search_formatted, nullptr);

			id << find_release_artist->master_releases[myparam.master_ndx]->id;
		}
		else {

			if (myparam.is_release()) {
				CONF.search_master_sub_format_string->run_hook(location, &info, hook.get(), search_formatted, nullptr);
				//hook need also master set to properly run its string format
				//hook->set_release(&(find_release_artist->master_releases[master_index]->sub_releases[subrelease]));
				pfc::string8 main_release_id = find_release_artist->master_releases[myparam.master_ndx]->main_release->id;
				pfc::string8 this_release_id = find_release_artist->master_releases[myparam.master_ndx]->sub_releases[myparam.release_ndx]->id;
				id << this_release_id;
			}
			else
			{
				CONF.search_release_format_string->run_hook(location, &info, hook.get(), search_formatted, nullptr);
				//hook->set_release(&(find_release_artist->releases[release_index]));
				id << find_release_artist->releases[myparam.release_ndx]->id;
			}
		}
		row_data.col_data_list.emplace_back(std::make_pair(walk_cfg.icol, search_formatted));
	}
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

//todo:
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
			if (matches_master /*&& atoi(find_release_artist->master_releases[master_ndx]->id) == _idtracker.master_id*/) {
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

						matches_release = check_match(sub_item, filter_master, lcf_words);
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

			if (matches_release) {
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
	std::unordered_map<int, std::pair<row_col_data, flagreg>>* cached = &m_cache_find_releases;
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
	m_updating_releases = true; 
	init_tracker();

	t_size the_artist = !find_release_artist ? pfc_infinite : atoi(find_release_artist->id);

	bool artist_changed = m_release_list_artist_id != the_artist;
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
		m_vec_items.clear();
		m_vec_items.reserve(vec_lv_reserve.first + vec_lv_reserve.second);

		if (updsrc != updRelSrc::Filter) {
			m_cache_find_releases.clear();
			rebuild_tree_cache = true;
		}
		else {
			rebuild_tree_cache = false;
		}

		TreeView_DeleteAllItems(m_release_tree);
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
		m_cache_find_releases.reserve(find_release_artist->search_order_master.get_count());
	}
	try {

		for (t_size walk = 0; walk < find_release_artist->search_order_master.get_size(); walk++) {

			bool is_master = find_release_artist->search_order_master[walk];
			pfc::string8 item; //master or release summary to match

			if (is_master) {
				mounted_param thisparam(master_index, ~0, true, false);
				item_data = get_mounted_lparam(thisparam);
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
				mounted_param thisparam(~0, release_index, false, true);
				item_data = get_mounted_lparam(thisparam);
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
				last_cache_emplaced = m_cache_find_releases.emplace(
					item_data, std::pair<row_col_data, flagreg> {row_data, { _ver, 0 }});
			}

			bool inserted = false; bool expanded = false;

			bool matches = filtered && check_match(item, filter, lcf_words);
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
					mounted_param myparam = mount_param((LPARAM)item_data);
					if (myparam.is_master()) {
						cache_iterator_t insit = m_cache_find_releases.find(get_mounted_lparam(myparam));
						insit->second.second.flag = 0;  insit->second.second.ver = _ver;
						int ival = 1;
						SetCacheFlag(insit, NodeFlag::none, &ival);
						//VEC PUSH
						m_vec_items.push_back(std::pair<cache_iterator_t, HTREEITEM*>(insit, nullptr));
					}
					else {
						if (myparam.is_release()) {
							;
						}
						else {

							cache_iterator_t insit = m_cache_find_releases.find(get_mounted_lparam(myparam));
							//cache_iterator_t insit = last_cache_emplaced.first;

							insit->second.second.flag = 0;
							insit->second.second.ver = _ver;
							int ival = 0;
							SetCacheFlag(insit, NodeFlag::added, &ival);

							flagreg& insfr = insit->second.second;
							std::pair<cache_iterator_t, HTREEITEM*> inspair = { insit, nullptr };
							//m_vec_lv_items.reserve(m_vec_lv_items.size() + 1);
							//VEC PUSH
							m_vec_items.push_back(std::pair<cache_iterator_t, HTREEITEM*>(insit, nullptr));
						}
					}
					int lp = m_vec_items.at(m_vec_items.size() - 1).first->first;
				}

				int lp = m_vec_items.at(m_vec_items.size() - 1).first->first;

				if (init_expand && is_master) {
					/*int prev_expanded;*/
					int expanded = updsrc == updRelSrc::Filter ? 0 : 1;
					bool bnew = !get_node_expanded(row_data.id, prev_expanded);

					//set_node_expanded(row_data.id, /*bnew ? expanded : */prev_expanded, true);
					//m_vec_build_lv_items.emplace_back(std::pair<int, int>(row_data.id, bnew ? 0 : bexpandstate));
					//set_lvstate_expanded(list_index-release_index, /*bnew ? expanded :*/ prev_expanded, true);
					//set_lvstate_expanded(list_index - release_index, /*bnew ? expanded :*/ prev_expanded == 1, true);

				}

				list_index++;
				inserted = true;
			}
			if (m_vec_items.size() > 0)
				int lp = m_vec_items.at(0).first->first;
			if (is_master) {
				// SUBITEMS
				const MasterRelease_ptr* master_release = &(find_release_artist->master_releases[master_index]);

				for (size_t j = 0; j < master_release->get()->sub_releases.get_size(); j++) {

					mounted_param thisparam = { master_index, j, true, true };
					item_data = get_mounted_lparam(thisparam);
					pfc::string8 sub_item;
					if (updsrc != updRelSrc::Filter)
					{
						hook->set_release(&(find_release_artist->master_releases[master_index]->sub_releases[j]));
						sub_item = run_hook_columns(row_data, item_data);
					}
					else
						get_cached_find_release_node(item_data, sub_item, row_data);

					std::pair<cache_iterator_t, bool> last_cache_subitem_emplaced =
					{ m_cache_find_releases.end(), false };

					if (rebuild_tree_cache) {

						//m_cache_find_releases_tree.push_back({ row_data, item_data });
						//flagreg fr = { 0 , 0 };

						//CACHE EMPLACE
						last_cache_subitem_emplaced = m_cache_find_releases.emplace(item_data, std::pair<row_col_data, flagreg> {row_data, { _ver, 0 }});
					}

					bool matches = filtered && check_match(sub_item, filter, lcf_words);
				
					if (matches) {

						int master_id = atoi(find_release_artist->master_releases[master_index]->id);

						auto parent = find_vec_by_id(master_id);
						inserted = parent != m_vec_items.end();

						if (!inserted) {
							PFC_ASSERT(false);
						}

						// SUBRELEASE
						if (init_expand && prev_expanded == 1) {

							/*m_vec_build_lv_items.at(m_vec_build_lv_items.size() - 1).second == 1*/

							_idtracer.release_check(master_release->get()->sub_releases[j]->id, list_index,
								false, list_index - j, master_index);

							//insert
							
							if (delete_on_enter) {
								mounted_param myparam = mount_param((LPARAM)item_data);
								if (myparam.is_master()) {
									cache_iterator_t insit = last_cache_emplaced.first;
									flagreg& insfr = last_cache_emplaced.first->second.second;
									//VEC PUSH
									m_vec_items.push_back(std::pair<cache_iterator_t, HTREEITEM*>
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

	int list_count = m_vec_items.size();

	//almost there

	if (!delete_on_enter) {
		;
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

			tvis.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM;
			tvis.item.pszText = LPSTR_TEXTCALLBACK;
			tvis.item.iImage = I_IMAGECALLBACK;
			tvis.item.iSelectedImage = I_IMAGECALLBACK;
			tvis.item.cChildren = I_CHILDRENCALLBACK;
			tvis.item.lParam = walk.first->first;
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
			TreeView_InsertItem(m_release_tree, &tvis);
		}
	}
	m_updating_releases = false;
	return;
}

void CFindReleaseDialog::expand_releases(const pfc::string8& filter, updRelSrc updsrc, t_size master_index, t_size master_list_pos) {
	pfc::array_t<pfc::string> lcf_words;
	bool filtered = tokenize_filter(filter, lcf_words);

	int list_index = master_list_pos + 1;

	int walk_lparam;
	row_col_data row_data;

	try {
		int skipped = 0;
		mounted_param parent_mp(master_index, ~0, true, false);
		int parent_lparam = get_mounted_lparam(parent_mp);
		std::unordered_map<int, std::pair<row_col_data, flagreg>>::const_iterator parent_cache = m_cache_find_releases.find(parent_lparam);
		std::unordered_map<int, std::pair<row_col_data, flagreg>>::const_iterator new_release_cache;
		if (parent_cache == m_cache_find_releases.end()) {
			int debug = 1;
		}
		try {
			hook->set_master(nullptr);
			for (size_t j = 0; j < find_release_artist->master_releases[master_index]->sub_releases.get_size(); j++) {

				row_col_data row_data_subitem;
				hook->set_release(&(find_release_artist->master_releases[master_index]->sub_releases[j]));
				hook->set_master(&(find_release_artist->master_releases[master_index]));
				mounted_param thisparam = { master_index, j, true, true };
				walk_lparam = get_mounted_lparam(thisparam);

				pfc::string8 sub_item;
				//todo: can we avoid this check?
				if (!get_cached_find_release_node(get_mounted_lparam(thisparam), sub_item, row_data)) {
					sub_item = run_hook_columns(row_data, walk_lparam);
					//CACHE EMPLACE
					m_cache_find_releases.emplace(
						walk_lparam, std::pair<row_col_data, flagreg> {row_data, { _ver, 0 }/*walk_lparam*/});
				}

				bool filter_match = filtered && check_match(sub_item, filter, lcf_words);

				if (filter_match) {
					cache_iterator_t walk_children =
						m_cache_find_releases.find(walk_lparam);

					int ival = 1;
					SetCacheFlag(walk_children, NodeFlag::spawn, &ival);
					//in filter
					SetCacheFlag(walk_children, NodeFlag::filterok, &ival);

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
		SetCacheFlag(parent_lparam, NodeFlag::tree_created, false);

		//expand branch

		if (m_hHit != NULL) {
			TreeView_Expand(m_release_tree, m_hHit, TVM_EXPAND);
			m_hHit = NULL;
		}
	}
	catch (foo_discogs_exception& e) {
		foo_discogs_exception ex;
		ex << "Error formating release list subitem [" << e.what() << "]";
		throw ex;
	}
	return;
}

void CFindReleaseDialog::get_mounted_param(mounted_param& pm, LPARAM lparam) {
	pm.bmaster = (pm.master_ndx != 9999);
	pm.brelease = (pm.release_ndx != 9999);
	pm.master_ndx = HIWORD(lparam);
	pm.release_ndx = LOWORD(lparam);
}

mounted_param CFindReleaseDialog::mount_param(LPARAM lparam) {
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

t_size CFindReleaseDialog::get_mounted_lparam(mounted_param myparam) {
	t_size ires = ~0;
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

	auto vecpos = find_vec_by_lparam(src_lparam);

	if (vecpos == m_vec_items.end())
		return;

	int selection_index = std::distance(m_vec_items.begin(), vecpos);
	mounted_param myparam = mount_param(m_vec_items.at(selection_index).first->first);

	if (myparam.is_master()) {

		pfc::string8 release_url;
		if (_idtracer.amr && (get_param_id_master(myparam) == _idtracer.master_id))
			release_url.set_string(std::to_string(_idtracer.release_id).c_str());
		else
			release_url.set_string(get_param_id(myparam));

		if (GetCacheFlag(src_lparam, NodeFlag::expanded)) {

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

LRESULT CFindReleaseDialog::ApplyFilter(pfc::string8 strFilter) {
	strFilter = trim(strFilter);

	//TODO: mutex?
	if (!find_release_artist) {
		return false;
	}
	_ver++;
	if (strFilter.get_length() == 0) {
		m_vec_filter.clear();
		_idtracer.release_reset();
		update_releases(strFilter, updRelSrc::Filter, true);
	}
	else {
		//same filter (paste)
		if (!stricmp_utf8(m_results_filter, strFilter))
			return FALSE;

		m_vec_items.clear();
		TreeView_DeleteAllItems(m_release_tree);
		_ver++;

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
			lparam = get_mounted_lparam(myparam);
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
					int parent_lparam = get_mounted_lparam(parent_mp);

					auto& parent = find_vec_by_lparam(parent_lparam);
					if (parent != m_vec_items.end()) {

						parent_it = parent->first;

						//PARENT FOUND
						
						bool bexp;
						bool bthisver = GetCacheFlag(parent->first, NodeFlag::expanded, &bexp);
						int ival = bexp ? 1 : 0;
						SetCacheFlag(parent->first, NodeFlag::expanded, &ival);

					}
					else {
						//INSERT NOT-MATCHING PARENT							
						mounted_param mp_master(master_ndx, ~0, true, false);

						get_cached_find_release_node(get_mounted_lparam(mp_master), item, row_data);
						parent_it = m_cache_find_releases.find(get_mounted_lparam(mp_master));

						bool bexp;
						bool bthisver = GetCacheFlag(parent_it, NodeFlag::expanded, &bexp);
						int ival = bexp ? 1 : 0;
						SetCacheFlag(parent_it, NodeFlag::expanded, &ival);

						ival = 1;
						SetCacheFlag(parent_it, NodeFlag::added, &ival);
						//in filter
						SetCacheFlag(parent_it, NodeFlag::filterok, &ival);

						flagreg& insfr = parent_it->second.second;
						//VEC PUSH
						m_vec_items.push_back(std::pair<cache_iterator_t, HTREEITEM*>(parent_it, nullptr));

						int expanded = 1;

						auto master_row = find_vec_by_id(master_id);

					}

					hook->set_master(&(find_release_artist->master_releases[myparam.master_ndx]));

					hook->set_release(&(find_release_artist->master_releases[myparam.master_ndx]->sub_releases[myparam.release_ndx]));
				}
				else
					hook->set_release(&(find_release_artist->releases[myparam.release_ndx]));
			}

			PFC_ASSERT(get_cached_find_release_node(lparam, item, row_data) == true);

			cache_iterator_t filtered_it = m_cache_find_releases.find(lparam);
			int ival = 1;
			SetCacheFlag(filtered_it, NodeFlag::filterok, &ival);
			//VEC_PUSH
			m_vec_items.push_back(std::pair<cache_iterator_t, HTREEITEM*>(filtered_it, nullptr));
		}

		int counter = 0;
		std::vector<HTREEITEM> v_hItem_filtered_masters;
		for (auto walk : m_vec_items)
		{
			//todo: maybe recycle previously expanded nodes?
			if (!mount_param(walk.first->first).is_master() &&
				!mount_param(walk.first->first).is_nmrelease()) {
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
			int ival = 0;
			SetCacheFlag(walk.first, NodeFlag::tree_created, &ival);
			bool val;
			if (GetCacheFlag(walk.first, NodeFlag::expanded, &val)) {
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

LRESULT CFindReleaseDialog::OnDoubleClickRelease(int, LPNMHDR hdr, BOOL&) {

	LPNMITEMACTIVATE nmListView = (LPNMITEMACTIVATE)hdr;
	NMTVDISPINFO* pDispInfo = reinterpret_cast<NMTVDISPINFO*>(hdr);
	TVITEMW* pItem = &(pDispInfo)->item;
	HTREEITEM* hItem = &(pItem->hItem);
	HTREEITEM hhit;

	int lparam = pItem->lParam;
	mounted_param myparam = mount_param(lparam);

	//get client point to hit test
	POINT p;
	GetCursorPos(&p);
	::ScreenToClient(m_release_tree, &p);

	TVHITTESTINFO hitinfo = { 0 };
	hitinfo.pt = p;

	if (hhit = (HTREEITEM)SendMessage(m_release_tree,
		TVM_HITTEST, NULL, (LPARAM)&hitinfo)) {
		TVITEM phit = { 0 };
		phit.mask = TVIF_PARAM;
		phit.hItem = hhit;
		TreeView_GetItem(m_release_tree, &phit);
		lparam = phit.lParam;
		myparam = mount_param(lparam);

		if (hitinfo.flags & TVHT_ONITEM) {
			//use right click also to select items
			TreeView_SelectItem(m_release_tree, hhit);
		}
	}
	else {
		return FALSE;
	}

	t_size list_index = nmListView->iItem;

	if (!myparam.is_master()) {
		BOOL bdummy;
		OnOK(0L, 0L, NULL, bdummy);
	}

	return FALSE;
}

LRESULT CFindReleaseDialog::OnRClickRelease(int, LPNMHDR hdr, BOOL&) {
	//list
	LPNMITEMACTIVATE nmView = (LPNMITEMACTIVATE)hdr;
	//tree
	NMTVDISPINFO* pDispInfo = reinterpret_cast<NMTVDISPINFO*>(hdr);
	TVITEMW* pItem = &(pDispInfo)->item;
	HTREEITEM* hItem = &(pItem->hItem);
	HTREEITEM hhit = NULL;
	//..

	bool isArtist = hdr->hwndFrom == m_artist_list;
	bool isReleaseList = false;/*hdr->hwndFrom == release_ownerlist;*/
	bool isReleaseTree = hdr->hwndFrom == m_release_tree;

	t_size list_index = -1;
	int lparam = -1;
	mounted_param myparam = mount_param(lparam);

	POINT p;
	if (isArtist || isReleaseList) {
		list_index = nmView->iItem;
		if (list_index == pfc_infinite)
			return FALSE;
		p = nmView->ptAction;
		::ClientToScreen(hdr->hwndFrom, &p);

		if (isReleaseList) {
			myparam = mount_param(m_vec_items.at(list_index).first->first);
		}
	}
	else if (isReleaseTree) {

		//get client point to hit test
		GetCursorPos(&p);
		::ScreenToClient(m_release_tree, &p);

		TVHITTESTINFO hitinfo = { 0 };
		hitinfo.pt = p;

		if (hhit = (HTREEITEM)SendMessage(m_release_tree,
			TVM_HITTEST, NULL, (LPARAM)&hitinfo)) {
			TVITEM phit = { 0 };
			phit.mask = TVIF_PARAM;
			phit.hItem = hhit;
			TreeView_GetItem(m_release_tree, &phit);
			lparam = phit.lParam;
			myparam = mount_param(lparam);

			if (hitinfo.flags & TVHT_ONITEM) {
				//use right click also for selection
				TreeView_SelectItem(m_release_tree, hhit);
			}
		}
		else {
			return FALSE;
		}

		//screen position for context panel
		GetCursorPos(&p);

	}

	lparam = get_mounted_lparam(myparam);

	pfc::string8 sourcepage = isArtist ? "View Artist page" : !myparam.brelease ? "View Master Release page" : "View Release page";
	pfc::string8 copytitle = "Copy Title to Clipboard";
	pfc::string8 copyrow = "Copy to Clipboard";

	try {
		int coords_x = p.x, coords_y = p.y;
		enum { ID_VIEW_PAGE = 1, ID_CLP_COPY_TITLE, ID_CLP_COPY_ROW };
		HMENU menu = CreatePopupMenu();
		uAppendMenu(menu, MF_STRING, ID_VIEW_PAGE, sourcepage);
		uAppendMenu(menu, MF_SEPARATOR, 0, 0);
		if (isArtist) {
			uAppendMenu(menu, MF_STRING, ID_CLP_COPY_ROW, copyrow);
		}
		else {
			uAppendMenu(menu, MF_STRING, ID_CLP_COPY_TITLE, copytitle);
			uAppendMenu(menu, MF_STRING, ID_CLP_COPY_ROW, copyrow);
		}
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
		case ID_CLP_COPY_TITLE:
		{
			pfc::string8 buffer;
			if (isArtist) {
				;
			}
			else {
				if (myparam.is_release()) {

					buffer << find_release_artist->master_releases[myparam.master_ndx]->sub_releases[myparam.release_ndx]->title;
				}
				else
					if (myparam.bmaster) {
						buffer << find_release_artist->master_releases[myparam.master_ndx]->title;
					}
					else {
						buffer << find_release_artist->releases[myparam.release_ndx]->title;
					}
			}
			if (buffer != NULL) {
				ClipboardHelper::OpenScope scope; scope.Open(m_hWnd);
				ClipboardHelper::SetString(buffer);
			}


			return true;
		}
		case ID_CLP_COPY_ROW:
		{
			pfc::string8 utf_buffer;
			TCHAR outBuffer[MAX_PATH + 1] = {};

			if (isArtist) {
				if (nmView->iItem != -1) {
					LVITEM lvi;
					TCHAR outBuffer[MAX_PATH + 1] = {};
					lvi.pszText = outBuffer;
					lvi.cchTextMax = MAX_PATH;
					lvi.mask = LVIF_TEXT;
					lvi.stateMask = (UINT)-1;
					lvi.iItem = nmView->iItem;
					lvi.iSubItem = 0;
					BOOL result = ListView_GetItem(m_artist_list, &lvi);
					utf_buffer << pfc::stringcvt::string_utf8_from_os(outBuffer).get_ptr();
				}
			}
			else {
				if (hhit != NULL) {
					TVITEM phit = { 0 };
					phit.mask = TVIF_PARAM | TVIF_TEXT;
					phit.pszText = outBuffer;
					phit.cchTextMax = MAX_PATH;
					phit.hItem = hhit;
					TreeView_GetItem(m_release_tree, &phit);
					utf_buffer << pfc::stringcvt::string_utf8_from_os(outBuffer).get_ptr();
				}
			}
			if (utf_buffer != NULL) {
				ClipboardHelper::OpenScope scope; scope.Open(m_hWnd);
				ClipboardHelper::SetString(utf_buffer);
			}
			return true;
		}
		}
	}
	catch (...) {}

	return 0;
}

LRESULT CFindReleaseDialog::OnReleaseTreeGetDispInfo(int, LPNMHDR hdr, BOOL&) {

	if (m_vec_items.size() == 0 || m_updating_releases)
		return FALSE;

	NMTVDISPINFO* pDispInfo = reinterpret_cast<NMTVDISPINFO*>(hdr);
	TVITEMW* pItem = &(pDispInfo)->item;
	HTREEITEM* hItem = &(pItem->hItem);

	int lparam = pItem->lParam;
	mounted_param myparam = mount_param(lparam);


	if (pItem->mask & TVIF_CHILDREN)
	{
		if (myparam.is_master()) {
			pItem->cChildren = I_CHILDRENCALLBACK;
		}
		else {
			pItem->cChildren = 0; //getchildres(x) > 0 ? 1 : 0; //I_CHILDRENCALLBACK;
		}
	}

	if (pItem->mask & TVIF_TEXT)
	{
		TCHAR outBuffer[MAX_PATH + 1] = {};
		cache_iterator_t cit = m_cache_find_releases.find(lparam);
		auto row_data = cit->second.first.col_data_list.begin();
		pfc::stringcvt::convert_utf8_to_wide(outBuffer, MAX_PATH,
			row_data->second.get_ptr(), row_data->second.get_length());
		_tcscpy_s(pItem->pszText, pItem->cchTextMax, const_cast<TCHAR*>(outBuffer));
		return FALSE;
	}

	/* TVIS_USERMASK, etc
	if (pItem->mask & LVIF_IMAGE)
	{
		// ...set an image list index
	}
	*/

	return TRUE;
}

LRESULT CFindReleaseDialog::OnReleaseTreeExpanding(int, LPNMHDR hdr, BOOL&) {

	NMTREEVIEW* pNMTreeView = (NMTREEVIEW*)hdr;
	TVITEMW* pItemExpanding = &(pNMTreeView)->itemNew;
	HTREEITEM* hItemExpanding = &(pNMTreeView->itemNew.hItem);

	if ((pNMTreeView->action & TVE_EXPAND) != TVE_EXPAND)
	{
		auto node_it = find_vec_by_lparam(pItemExpanding->lParam);
		int ival = 0;
		SetCacheFlag(node_it->first, NodeFlag::expanded, &ival);

		return FALSE;
	}


	if (pItemExpanding->mask & TVIF_CHILDREN)
	{
		mounted_param myparam = mount_param(pItemExpanding->lParam);

		//can not rely on cChildren value here, need to register them ourselves
		//(pItemExpanding->cChildren == I_CHILDRENCALLBACK, ...)
		// int tc = TreeView_GetCount(release_tree);

		auto& cache_parent = m_cache_find_releases.find(pItemExpanding->lParam);

		/*
		or

		auto node_it = std::find_if(m_vec_lv_items.begin(), m_vec_lv_items.end(),
				[pItemExpanding](const std::pair<cache_iterator_t, flagreg> e) {
					return e.first->first == pItemExpanding->lParam; }
			);
		*/

		bool children_done;
		GetCacheFlag(cache_parent, NodeFlag::added, &children_done);

		bool tree_children_done;
		bool tree_children_done_ver = GetCacheFlag(cache_parent, NodeFlag::tree_created, &tree_children_done);

		if (children_done) {
			if (!tree_children_done_ver) {
				if (myparam.is_master()) {
					t_size try_release_ndx = 0;
					std::vector<TVINSERTSTRUCT> vchildren = {};
					//seq access
					//auto walk_children = cache_parent++;
					//random access
					int walk_param = get_mounted_lparam({ myparam.master_ndx, try_release_ndx, true, true });
					cache_iterator_t walk_children = m_cache_find_releases.find(walk_param);
					int newindex = 0;
					while (walk_children != m_cache_find_releases.end()) {

						//pfc::string8 randaccess = m_cache_find_releases.find(walk_children->first)->second.first.col_data_list.begin()->second;

						bool ival = false;
						GetCacheFlag(walk_children, NodeFlag::spawn, &ival);

						bool bfilter;
						bool bfilter_ver = GetCacheFlag(walk_children, NodeFlag::filterok, &bfilter);

						pfc::string8 filter;
						uGetWindowText(m_filter_edit, filter);
						
						//..

						if (m_vec_filter.size() == 0 || (m_vec_filter.size() && bfilter_ver)) {

						//if (/*ival == 1 &&*/ !(m_results_filter.get_length() && !bfilter_ver)) {

							vchildren.resize(newindex + 1);
							int lparam = walk_children->first;
							mounted_param mp = mount_param(lparam);
							//todo: move treeview node creation to another class
							TV_INSERTSTRUCT* tvis = (TV_INSERTSTRUCT*)&vchildren.at(newindex);
							//memmove(&(tvis->item), &tvi, sizeof(TV_ITEM));
							tvis->hParent = (HTREEITEM)pItemExpanding->hItem;
							tvis->hInsertAfter = TVI_LAST;
							TV_ITEM tvi;
							memset(&tvi, 0, sizeof(tvi));
							tvis->item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM | TVIF_STATE; //TVIF_IMAGE | TVIF_TEXT | TVIF_SELECTEDIMAGE;
							tvis->item.pszText = LPSTR_TEXTCALLBACK;
							tvis->item.iImage = I_IMAGECALLBACK;
							tvis->item.iSelectedImage = I_IMAGECALLBACK;
							tvis->item.cChildren = 0;
							tvis->item.lParam = walk_children->first;
							int walk_id = walk_children->second.first.id;
							//set bold font to current release_id
							if (walk_id == _idtracer.release_id) {
								tvis->item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM | TVIF_STATE; //TVIF_IMAGE | TVIF_TEXT | TVIF_SELECTEDIMAGE;
								tvis->item.state = TVIS_BOLD;
								tvis->item.stateMask = TVIS_BOLD;
							}
							else {
								tvis->item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM; //TVIF_IMAGE | TVIF_TEXT | TVIF_SELECTEDIMAGE;
							}

							newindex++;

						}

						//vchildren.push_back(tvis);

						walk_param = get_mounted_lparam({ myparam.master_ndx, ++try_release_ndx, true, true });
						walk_children = m_cache_find_releases.find(walk_param);

					}

					for (const auto& tvchild : vchildren) {
						//::EnableWindow(release_tree, false);
						TreeView_InsertItem(m_release_tree, &tvchild);
					}

					int ival = 1; //finished with this node, set it as tree children done 
					SetCacheFlag(pItemExpanding->lParam, NodeFlag::tree_created, ival);

					bool bval = false;
					GetCacheFlag(pItemExpanding->lParam, NodeFlag::expanded, bval);
					int ivalex = 1;
					SetCacheFlag(pItemExpanding->lParam, NodeFlag::expanded, &ivalex);
					int kk = cache_parent->first;

					//children #
					pItemExpanding->cChildren = vchildren.size();

				}
				else {

					//note:could also test with get_count()? 1 : 0

					find_release_artist->releases.get_count() ? I_CHILDRENCALLBACK : 0;
					pItemExpanding->cChildren = find_release_artist->releases.get_count() ? I_CHILDRENCALLBACK : 0;
				}
			}
			else {
				//children already produced

				int ival = 1;
				SetCacheFlag(cache_parent, NodeFlag::expanded, &ival);

				return FALSE;
			}
		}
		else {
			m_hHit = pItemExpanding->hItem;

			//temporal fix to make this compatible with the listview
			auto master_it = find_vec_by_lparam(pItemExpanding->lParam);

			if (master_it == m_vec_items.end())
				return TRUE;
			int selection_index = std::distance(m_vec_items.begin(), master_it);


			if (master_it != m_vec_items.end()) {
				int ival = 1;
				SetCacheFlag(master_it->first, NodeFlag::expanded, &ival);

				if (ival) {
					int ival = 1;
					SetCacheFlag(master_it->first, NodeFlag::added, &ival);
				}
			}
			on_release_selected(pItemExpanding->lParam);

		}
	}
	return FALSE;
}

LRESULT CFindReleaseDialog::OnReleaseTreeSelChanged(int, LPNMHDR hdr, BOOL&) {
	LPNMTREEVIEW pnmtv = (LPNMTREEVIEW)hdr;
	NMTREEVIEW* pNMTreeView = (NMTREEVIEW*)hdr;

	TVITEM pItemMaster = { 0 };
	pItemMaster.mask = TVIF_PARAM;
	pItemMaster.hItem = pnmtv->itemNew.hItem;
	TreeView_GetItem(m_release_tree, &pItemMaster);

	cache_iterator_t it = m_cache_find_releases.find(pItemMaster.lParam);
	if (it != m_cache_find_releases.end()) {
		int id = it->second.first.id;
		pfc::string8 release_url = pfc::toString(id).c_str();

		uSetWindowText(m_release_url_edit, release_url);
	}
	return 0;
}

LRESULT CFindReleaseDialog::OnCheckOnlyExactMatches(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	conf.display_exact_matches = IsDlgButtonChecked(IDC_ONLY_EXACT_MATCHES_CHECK) != 0;
	fill_artist_list(_idtracer.amr, updRelSrc::Artist);

	get_selected_artist_releases(updRelSrc::Artist);
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
	service_ptr_t<process_release_callback> task = new service_impl_t<process_release_callback>(this, release_id, items);
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

bool CFindReleaseDialog::SetCacheFlag(t_size lparam, NodeFlag flagtype, int& val) {
	bool bres = false;
	auto& it = m_cache_find_releases.find(lparam);

	if (it != m_cache_find_releases.end()) {

		std::pair<row_col_data, flagreg >& my_rel_cache = it->second;
		flagreg& fr = my_rel_cache.second;

		switch ((int)flagtype) {
		case 0:
			break;
		case 1:
			break;
		case 2:
			break;
		case (int)NodeFlag::spawn: //4 (downloaded?)
			break;
		case (int)NodeFlag::expanded: //8
			break;
		case 16:
			break;
		case /*32*/(int)NodeFlag::filterok:
			fr.ver = _ver;
			break;
		default:
			break;
		}

		if (val == -1) {
			fr.flag ^= INDEXTOSTATEIMAGEMASK((int)flagtype);
			val = (fr.flag & INDEXTOSTATEIMAGEMASK((int)flagtype)) == INDEXTOSTATEIMAGEMASK((int)flagtype);
		}
		else if (val) {
			fr.flag |= INDEXTOSTATEIMAGEMASK((int)flagtype);
		}
		else {
			fr.flag &= ~INDEXTOSTATEIMAGEMASK((int)flagtype);
		}
		/*fr.ver = ver;*/
		bres = true;
	}
	else {
		bres = false;
	}
	return bres;
}

bool CFindReleaseDialog::SetCacheFlag(t_size lparam, NodeFlag flagtype, bool val) {
	bool bres = false;
	auto& it = m_cache_find_releases.find(lparam);
	if (it != m_cache_find_releases.end()) {

		std::pair<row_col_data, flagreg >& my_rel_cache = it->second;
		flagreg& fr = my_rel_cache.second;

		switch ((int)flagtype) {
		case 0:
			break;
		case 1:
			break;
		case 2:
			break;
		case (int)NodeFlag::spawn: //4 (downloaded?)
			break;
		case (int)NodeFlag::expanded: //8
			break;
		case 16:
			break;
		case /*32*/(int)NodeFlag::filterok:
			fr.ver = _ver;
			break;
		default:
			break;
		}

		/*if (val == -1) {
			fr.flag ^= INDEXTOSTATEIMAGEMASK((int)flagtype);
			val = (fr.flag & INDEXTOSTATEIMAGEMASK((int)flagtype)) == INDEXTOSTATEIMAGEMASK((int)flagtype);
		}
		else*/ if (val) {
			fr.flag |= INDEXTOSTATEIMAGEMASK((int)flagtype);
		}
		else {
			fr.flag &= ~INDEXTOSTATEIMAGEMASK((int)flagtype);
		}
		/*fr.ver = ver;*/
		bres = true;
	}
	else {
		bres = false;
	}
	return bres;
}

bool CFindReleaseDialog::SetCacheFlag(cache_iterator_t& it_cached_item,
	NodeFlag flagtype, int* const& val) {

	bool bres = false;

	if (it_cached_item->first > -1) {

		flagreg& fr = it_cached_item->second.second;

		switch ((int)flagtype) {
		case 0:
			break;
		case 1:
			break;
		case 2:
			break;
		case (int)NodeFlag::spawn: //4 (downloaded?)
			break;
		case (int)NodeFlag::expanded: //8
			break;
		case 16:
			break;
		case /*32*/(int)NodeFlag::filterok:
			fr.ver = _ver;
			break;
		default:
			break;
		}
		
		if (*val == -1) {
			fr.flag ^= INDEXTOSTATEIMAGEMASK((int)flagtype);
			*val = (fr.flag & INDEXTOSTATEIMAGEMASK((int)flagtype)) == INDEXTOSTATEIMAGEMASK((int)flagtype);
		}
		else if (*val) {
			fr.flag |= INDEXTOSTATEIMAGEMASK((int)flagtype);
		}
		else {
			fr.flag &= ~INDEXTOSTATEIMAGEMASK((int)flagtype);
		}
		//fr.ver = ver;
		bres = true;
	}
	else {
		bres = false;
	}
	return bres;
}

bool CFindReleaseDialog::GetCacheFlag(t_size lparam, NodeFlag flagtype, bool& val) {
	bool bres = false;
	auto& it = m_cache_find_releases.find(lparam);
	if (it != m_cache_find_releases.end()) {
		std::pair<row_col_data, flagreg >& my_rel_cache = it->second;
		flagreg& fr = my_rel_cache.second;

		if ((fr.flag & INDEXTOSTATEIMAGEMASK((int)flagtype)) == INDEXTOSTATEIMAGEMASK((int)flagtype)) {
			val = true;
		}
		else {
			//fr.flag &= ~INDEXTOSTATEIMAGEMASK(flag);
			val = false;
		}

		bres = val && fr.ver == _ver;
	}
	else {
		bres = false;
	}
	return bres;
}

bool CFindReleaseDialog::GetCacheFlag(t_size lparam, NodeFlag flagtype) {
	bool bres = false;
	auto& it = m_cache_find_releases.find(lparam);
	if (it != m_cache_find_releases.end()) {

		std::pair<row_col_data, flagreg >& my_rel_cache = it->second;
		flagreg& fr = my_rel_cache.second;

		if ((fr.flag & INDEXTOSTATEIMAGEMASK((int)flagtype)) == INDEXTOSTATEIMAGEMASK((int)flagtype)) {
			bres = true;
		}
		else {
			//fr.flag &= ~INDEXTOSTATEIMAGEMASK(flag);
			bres = false;
		}

		bres = bres && fr.ver == _ver;
	}
	else {
		bres = false;
	}
	return bres;
}

bool CFindReleaseDialog::GetCacheFlag(
	cache_iterator_t it_cached_item,
	NodeFlag flagtype, bool* const& val) {

	bool bres = false;
	bool bval = false;

	if (it_cached_item->first > -1) {

		flagreg& fr = it_cached_item->second.second;

		if ((fr.flag & INDEXTOSTATEIMAGEMASK((int)flagtype)) == INDEXTOSTATEIMAGEMASK((int)flagtype)) {
			bval = true;
		}
		else {
			//fr.flag &= ~INDEXTOSTATEIMAGEMASK(flag);
			bval = false;
		}

		bres = bval && fr.ver == _ver;
		if (val != NULL)
			*val = bval;
	}
	else {
		bres = false;
	}

	return bres;
}

bool CFindReleaseDialog::set_node_expanded(t_size master_id, int& state, bool build) {
	auto& master_row = find_vec_by_id(master_id);
	if (master_row != m_vec_items.end()) {
		int ival = state;
		SetCacheFlag(master_row->first, NodeFlag::expanded, &ival);
		if (ival) {
			int ival = 1;
			SetCacheFlag(master_row->first, NodeFlag::added, &ival);
		}
		return true;
	}
	else {
		state = -1;
		return false;
	}
}

bool CFindReleaseDialog::get_node_expanded(t_size master_id, int& out_state) {
	auto master_row = find_vec_by_id(master_id);
	if (master_row != m_vec_items.end()) {
		SetCacheFlag(master_row->first, NodeFlag::expanded, &out_state);
		return true;
	}
	else {
		out_state = -1;
		return false;
	}
}

void CFindReleaseDialog::on_expand_master_release_done(const MasterRelease_ptr& master_release, int list_index, threaded_process_status& p_status, abort_callback& p_abort) {

	int master_i = -1;
	for (size_t i = 0; i < find_release_artist->master_releases.get_size(); i++) {
		if (find_release_artist->master_releases[i]->id == master_release->id) {
			master_i = i;
			break;
		}
	}

	PFC_ASSERT(master_i != -1);

	pfc::string8 filter;
	uGetWindowText(m_filter_edit, filter);

	int expanded = 1;
	bool bnodeset = set_node_expanded(atoi(master_release->id), expanded, false);

	if (bnodeset) {
		expand_releases(filter, updRelSrc::Releases, master_i, list_index);

		//todo:
		int ival = 1;
		mounted_param mparam(0, ~0, true, false);
		t_size lparam = get_mounted_lparam(mparam);
		SetCacheFlag(lparam, NodeFlag::expanded, ival);
		//..

		//set_lvstate_expanded(list_index, expanded, false);
	}

	mounted_param myparam = mount_param(m_vec_items.at(list_index).first->first);
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

	service_ptr_t<expand_master_release_process_callback> task =
		new service_impl_t<expand_master_release_process_callback>(release, pos);

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
