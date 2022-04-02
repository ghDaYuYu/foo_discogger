#include "stdafx.h"

#include "tag_mappings_dialog.h"
#ifdef CAT_CRED
#include "tag_mappings_credits_dlg.h"
#endif // CAT_CRED

#include "utils.h"
#include "tasks.h"
#include "db_utils.h"
#include "configuration_dialog.h"

static HWND g_hWndTabDialog[NUM_TABS] = {nullptr};
static HWND g_hWndCurrentTab = nullptr;
static t_uint32 g_current_tab;

inline bool toggle_title_format_help() {
	t_uint32 a[] = { CONF_FIND_RELEASE_TAB, CONF_MATCHING_TAB, CONF_ART_TAB };
	return std::find(std::begin(a), std::end(a), g_current_tab) != std::end(a);
}

bool my_threaded_process::run_modal(service_ptr_t<threaded_process_callback> p_callback, unsigned p_flags, HWND p_parent, const char* p_title, t_size p_title_len = ~0) {
	bool bres = false;
	g_run_modal(p_callback, p_flags, p_parent, p_title, p_title_len);
	return bres;
}
bool my_threaded_process::run_modeless(service_ptr_t<threaded_process_callback> p_callback, unsigned p_flags, HWND p_parent, const char* p_title, t_size p_title_len = ~0) {
	bool bres = false;
	g_run_modeless(p_callback, p_flags, p_parent, p_title, p_title_len);
	return bres;
}

//! Decrements reference count; deletes the object if reference count reaches zero. This is normally not called directly but managed by service_ptr_t<> template. \n
//! Implemented by service_impl_* classes.
//! @returns New reference count. For debug purposes only, in certain conditions return values may be unreliable.
int my_threaded_process::service_release() throw() {
	int ires = 0;
	return ires;
};
//! Increments reference count. This is normally not called directly but managed by service_ptr_t<> template. \n
//! Implemented by service_impl_* classes.
//! @returns New reference count. For debug purposes only, in certain conditions return values may be unreliable.
int my_threaded_process::service_add_ref() throw() {
	int ires = 0;
	return ires;
};
//! Queries whether the object supports specific interface and retrieves a pointer to that interface. This is normally not called directly but managed by service_query_t<> function template. \n
//! Checks the parameter against GUIDs of interfaces supported by this object, if the GUID is one of supported interfaces, p_out is set to service_base pointer that can be static_cast<>'ed to queried interface and the method returns true; otherwise the method returns false. \n
//! Implemented by service_impl_* classes. \n
//! Note that service_query() implementation semantics (but not usage semantics) changed in SDK for foobar2000 1.4; they used to be auto-implemented by each service interface (via FB2K_MAKE_SERVICE_INTERFACE macro); they're now implemented in service_impl_* instead. See SDK readme for more details. \n
bool my_threaded_process::service_query(service_ptr& p_out, const GUID& p_guid) {
	bool bres = 0;
	return bres;
};

void CConfigurationDialog::InitTabs() {
	tab_table.append_single(tab_entry("Searching", searching_dialog_proc, IDD_DIALOG_CONF_FIND_RELEASE));
	tab_table.append_single(tab_entry("Matching", matching_dialog_proc, IDD_DIALOG_CONF_MATCHING));
	tab_table.append_single(tab_entry("Tagging", tagging_dialog_proc, IDD_DIALOG_CONF_TAGGING));
	tab_table.append_single(tab_entry("Cache", caching_dialog_proc, IDD_DIALOG_CONF_CACHING));
	tab_table.append_single(tab_entry("Artwork", art_dialog_proc, IDD_DIALOG_CONF_ART));
	tab_table.append_single(tab_entry("UI Options", ui_dialog_proc, IDD_DIALOG_CONF_UI));

#ifdef DB_DC
	tab_table.append_single(tab_entry("Database", db_dialog_proc, IDD_DIALOG_CONF_DATABASE));
#endif

	tab_table.append_single(tab_entry("OAuth", oauth_dialog_proc, IDD_DIALOG_CONF_OAUTH));
}

CConfigurationDialog::~CConfigurationDialog() {

	g_discogs->configuration_dialog = nullptr;

}

//from libPPUI\CDialogResizeHelper.cpp
static BOOL GetChildWindowRect(HWND wnd, UINT id, RECT* child)
{
	RECT temp;
	HWND wndChild = GetDlgItem(wnd, id);
	if (wndChild == NULL) return FALSE;
	if (!GetWindowRect(wndChild, &temp)) return FALSE;
	if (!MapWindowPoints(0, wnd, (POINT*)&temp, 2)) return FALSE;
	*child = temp;
	return TRUE;
}
//

LRESULT CConfigurationDialog::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	conf = CONF;
	conf_dlg_edit = CONF;
	
	InitTabs();
	HWND hWndTab = uGetDlgItem(IDC_TAB);

	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	InitCtrls.dwICC = ICC_STANDARD_CLASSES;
	InitCommonControlsEx(&InitCtrls);
	EnableThemeDialogTexture(hWndTab, ETDT_ENABLETAB);

	// set up tabs and create (invisible) subdialogs
	uTCITEM item = {0};
	item.mask = TCIF_TEXT;
	for (size_t n = 0; n < NUM_TABS; n++) {
		PFC_ASSERT(tab_table[n].m_pszName != nullptr);

		item.pszText = tab_table[n].m_pszName;
		uTabCtrl_InsertItem(hWndTab, n, &item);

		g_hWndTabDialog[n] = tab_table[n].CreateTabDialog(m_hWnd, (LPARAM)this);
	}

	// get the size of the inner part of the tab control
	RECT rcTab;
	GetChildWindowRect(m_hWnd, IDC_TAB, &rcTab);
	uSendMessage(hWndTab, TCM_ADJUSTRECT, FALSE, (LPARAM)&rcTab);

	// Tab Control
	RECT rcTabDialog;
	::GetClientRect(g_hWndTabDialog[0], &rcTabDialog);
	OffsetRect(&rcTabDialog, rcTab.left, rcTab.top);
	rcTabDialog.bottom = (rcTabDialog.bottom > rcTab.bottom)
		? rcTabDialog.bottom : rcTab.bottom;
	rcTabDialog.right = (rcTabDialog.right > rcTab.right)
		? rcTabDialog.right : rcTab.right;

	uSendMessage(hWndTab, TCM_ADJUSTRECT, TRUE, (LPARAM)&rcTabDialog);
	::SetWindowPos(hWndTab, nullptr,
		rcTabDialog.left, rcTabDialog.top,
		rcTabDialog.right - rcTabDialog.left, rcTabDialog.bottom - rcTabDialog.top,
		SWP_NOZORDER | SWP_NOACTIVATE);

	// position the subdialogs in the inner part of the tab control
	uSendMessage(hWndTab, TCM_ADJUSTRECT, FALSE, (LPARAM)&rcTabDialog);
	//fix left white stripe
	InflateRect(&rcTabDialog, 2, 1);
	OffsetRect(&rcTabDialog, -1, 1);
	for (size_t n = 0; n < tabsize(g_hWndTabDialog); n++) {
		if (g_hWndTabDialog[n] != nullptr) {
			::SetWindowPos(g_hWndTabDialog[n], nullptr,
				rcTabDialog.left, rcTabDialog.top,
				rcTabDialog.right - rcTabDialog.left, rcTabDialog.bottom - rcTabDialog.top,
				SWP_NOZORDER | SWP_NOACTIVATE);
		}
	}

	g_current_tab = conf.last_conf_tab;
	uSendMessage(hWndTab, TCM_SETCURSEL, g_current_tab, 0);

	help_link.SubclassWindow(GetDlgItem(IDC_SYNTAX_HELP));
	pfc::string8 url(core_api::get_profile_path());
	url << "\\user-components\\foo_discogger\\foo_discogs_help.html";
	pfc::stringcvt::string_wide_from_utf8 wtext(url.get_ptr());
	help_link.SetHyperLink((LPCTSTR)const_cast<wchar_t*>(wtext.get_ptr()));

	::ShowWindow(help_link, toggle_title_format_help() ? SW_SHOW : SW_HIDE);

	g_hWndCurrentTab = g_hWndTabDialog[g_current_tab];
	if (g_hWndCurrentTab)
		::ShowWindow(g_hWndCurrentTab, SW_SHOW);

	return TRUE;
}

t_uint8 AskApplyConfirmation(HWND wndParent) {
	pfc::string8 title;
	title << "Configuration Changes";
	return uMessageBox(wndParent, "Apply Changes?", title,
		MB_APPLMODAL | MB_YESNOCANCEL | MB_ICONASTERISK);
}

LRESULT CConfigurationDialog::OnChangingTab(WORD /*wNotifyCode*/, LPNMHDR /*lParam*/, BOOL& /*bHandled*/) {

	if (get_state() & preferences_state::changed) {
		switch (AskApplyConfirmation(get_wnd())) {
		case IDYES:
			pushcfg(false);
			OnChanged();
			break;
		case IDNO:
			setting_dlg = true;
			init_current_tab();
			conf_dlg_edit = conf;
			setting_dlg = false;
			OnChanged();
			break;
		case IDCANCEL:
			return TRUE;
		}
	}
	return FALSE;
}

LRESULT CConfigurationDialog::OnChangeTab(WORD /*wNotifyCode*/, LPNMHDR /*lParam*/, BOOL& /*bHandled*/) {
	if (g_hWndCurrentTab != nullptr) {
		::ShowWindow(g_hWndCurrentTab, SW_HIDE);
	}
	g_hWndCurrentTab = nullptr;

	g_current_tab = (t_uint32)::SendDlgItemMessage(m_hWnd, IDC_TAB, TCM_GETCURSEL, 0, 0);
	if (g_current_tab < tabsize(g_hWndTabDialog)) {
		g_hWndCurrentTab = g_hWndTabDialog[g_current_tab];
		::ShowWindow(help_link, toggle_title_format_help() ? SW_SHOW : SW_HIDE);
		::ShowWindow(g_hWndCurrentTab, SW_SHOW);
	}
	return FALSE;
}

bool CConfigurationDialog::build_current_cfg(bool reset, bool bind) {

	bool bres = false;

	if (reset || g_hWndCurrentTab == g_hWndTabDialog[CONF_FIND_RELEASE_TAB]) {
		save_searching_dialog(g_hWndTabDialog[CONF_FIND_RELEASE_TAB], bind);		
	}
	if (reset || g_hWndCurrentTab == g_hWndTabDialog[CONF_MATCHING_TAB]) {
		save_matching_dialog(g_hWndTabDialog[CONF_MATCHING_TAB], bind);
	}
	if (reset || g_hWndCurrentTab == g_hWndTabDialog[CONF_TAGGING_TAB]) {
		save_tagging_dialog(g_hWndTabDialog[CONF_TAGGING_TAB], bind);
	}
	if (reset || g_hWndCurrentTab == g_hWndTabDialog[CONF_CACHING_TAB]) {
		save_caching_dialog(g_hWndTabDialog[CONF_CACHING_TAB], bind);
	}
#ifdef DB_DC
	if (reset || g_hWndCurrentTab == g_hWndTabDialog[CONF_DB_TAB]) {
		save_db_dialog(g_hWndTabDialog[CONF_DB_TAB], bind);
	}
#endif
	if (reset || g_hWndCurrentTab == g_hWndTabDialog[CONF_ART_TAB]) {
		save_art_dialog(g_hWndTabDialog[CONF_ART_TAB], bind);
	}
	if (reset || g_hWndCurrentTab == g_hWndTabDialog[CONF_UI_TAB]) {
		save_ui_dialog(g_hWndTabDialog[CONF_UI_TAB], bind);
	}
	if (reset || g_hWndCurrentTab == g_hWndTabDialog[CONF_OATH_TAB]) {
		save_oauth_dialog(g_hWndTabDialog[CONF_OATH_TAB], bind);
	}

	bres = reset || HasChanged();

	return bres;

}

void CConfigurationDialog::pushcfg(bool reset) {

	bool bind = !reset;

	bool has_changed = build_current_cfg(reset, bind);

	if (has_changed) {
		discogs_interface->fetcher->update_oauth(conf.oauth_token, conf.oauth_token_secret);
		if (bind) conf = conf_dlg_edit;
		conf_dlg_edit = conf;
		CONF.save(CConf::cfgFilter::CONF, conf);
		CONF.load();
	}
}


void CConfigurationDialog::reset() {

	BOOL bDummy;
	OnDefaults(0, 0, NULL, bDummy);
}

LRESULT CConfigurationDialog::OnDefaults(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	if (uMessageBox(m_hWnd, "Reset component settings to default?", "Reset Discogger",
		MB_OKCANCEL | MB_ICONQUESTION | MB_DEFBUTTON2) == IDOK) {

		//todo: pending reset/default history (db sqlite)

		foo_conf temp;
#ifdef DEBUG
		temp.oauth_token = conf.oauth_token;
		temp.oauth_token_secret = conf.oauth_token_secret;
#endif
		conf = temp;
		conf_dlg_edit = temp;
		init_searching_dialog(g_hWndTabDialog[CONF_FIND_RELEASE_TAB]);
		init_caching_dialog(g_hWndTabDialog[CONF_CACHING_TAB]);
#ifdef DB_DC
		init_db_dialog(g_hWndTabDialog[CONF_DB_TAB]);
#endif
		init_matching_dialog(g_hWndTabDialog[CONF_MATCHING_TAB]);
		init_tagging_dialog(g_hWndTabDialog[CONF_TAGGING_TAB]);
		init_art_dialog(g_hWndTabDialog[CONF_ART_TAB]);
		init_ui_dialog(g_hWndTabDialog[CONF_UI_TAB]);
		init_oauth_dialog(g_hWndTabDialog[CONF_OATH_TAB]);

		pushcfg(true);

		conf = CONF;
		conf_dlg_edit = CONF;

		OnChanged();

		//delete db history

		on_delete_history(m_hWnd, 0, true);

	}
	return FALSE;
}

LRESULT CConfigurationDialog::OnCustomAnvChanged(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	conf.replace_ANVs = lParam;
	if (g_hWndTabDialog[CONF_TAGGING_TAB]) {
		uButton_SetCheck(g_hWndTabDialog[CONF_TAGGING_TAB],
			IDC_ENABLE_ANV_CHECK, conf.replace_ANVs);
	}
	return FALSE;
}

LRESULT CConfigurationDialog::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled) {

	if (conf.last_conf_tab != g_current_tab) {
		CONF.save_active_config_tab(g_current_tab);
	}
	return FALSE;
}

void CConfigurationDialog::show_tab(unsigned int itab) {
	if (itab >= 0 && itab < NUM_TABS) {
		if (g_hWndCurrentTab != nullptr) {
			::ShowWindow(g_hWndCurrentTab, SW_HIDE);
		}
		
		//g_hWndCurrentTab = nullptr;
		//::SendDlgItemMessage(m_hWnd, IDC_TAB, TCM_SETCURSEL, itab, 0);

		g_current_tab = (t_uint32)::SendDlgItemMessage(m_hWnd, IDC_TAB, TCM_GETCURSEL, itab, 0);

		g_hWndCurrentTab = g_hWndTabDialog[itab];
		::ShowWindow(help_link, toggle_title_format_help() ? SW_SHOW : SW_HIDE);
		::ShowWindow(g_hWndCurrentTab, SW_SHOW);
	}
}

inline void set_window_text(HWND wnd, int IDC, const pfc::string8 &text) {
	pfc::stringcvt::string_wide_from_ansi wtext(text);
	::SetWindowText(::uGetDlgItem(wnd, IDC), (LPCTSTR)const_cast<wchar_t*>(wtext.get_ptr()));
}

enum {
	rowStyleGrid = 0,
	rowStyleFlat,
	rowStylePlaylist,
	rowStylePlaylistDelimited,
};

void InitComboRowStyle(HWND wnd, UINT id, t_size sel) {
	std::map<int, std::string>rowstylemap;
	rowstylemap.emplace((t_size)rowStyleGrid, "Grid");
	rowstylemap.emplace((t_size)rowStyleFlat, "Flat");
	rowstylemap.emplace((t_size)rowStylePlaylist, "Playlist");
	rowstylemap.emplace((t_size)rowStylePlaylistDelimited, "Playlist Grid");
	HWND ctrlwnd = uGetDlgItem(wnd, id);

	LRESULT lr = uSendDlgItemMessage(wnd, id, CB_RESETCONTENT, 0, 0);
	size_t pos = 0;
	for (auto& [val, text] : rowstylemap) {
		lr = uSendDlgItemMessageText(wnd, id, CB_INSERTSTRING, pos, text.c_str());
		uSendDlgItemMessage(wnd, id, CB_SETITEMDATA, lr, val);
		if (val == sel) {
			lr = uSendDlgItemMessageText(wnd, id, CB_SETCURSEL, lr, 0);
		}
		++pos;
	}
}

void CConfigurationDialog::init_searching_dialog(HWND wnd) {
	uButton_SetCheck(wnd, IDC_ENABLE_AUTO_SEARCH_CHECK, conf.enable_autosearch);
	uButton_SetCheck(wnd, IDC_SKIP_FIND_RELEASE_DIALOG_CHECK, conf.skip_mng_flag & SkipMng::SKIP_FIND_RELEASE_DLG_IDED);
	uButton_SetCheck(wnd, IDC_SKIP_LOAD_RELEASES_IF_IDED_CHECK, conf.skip_mng_flag & SkipMng::SKIP_LOAD_RELEASES_TASK_IDED);
	uSetDlgItemText(wnd, IDC_RELEASE_FORMATTING_EDIT, conf.search_release_format_string);
	uSetDlgItemText(wnd, IDC_MASTER_FORMATTING_EDIT, conf.search_master_format_string);
	uSetDlgItemText(wnd, IDC_MASTER_SUB_FORMATTING_EDIT, conf.search_master_sub_format_string);	
}

void CConfigurationDialog::init_matching_dialog(HWND wnd) {
	uButton_SetCheck(wnd, IDC_MATCH_USING_DURATIONS, conf.match_tracks_using_duration);
	uButton_SetCheck(wnd, IDC_MATCH_USING_NUMBERS, conf.match_tracks_using_number);
	uButton_SetCheck(wnd, IDC_MATCH_ASSUME_SORTED, conf.assume_tracks_sorted);
	uButton_SetCheck(wnd, IDC_SKIP_RELEASE_DIALOG_CHECK, conf.skip_mng_flag & SkipMng::SKIP_RELEASE_DLG_MATCHED);
	uButton_SetCheck(wnd, IDC_SKIP_BRAINZ_MIBS_FETCH, conf.skip_mng_flag & SkipMng::SKIP_BRAINZ_ID_FETCH);
	uSetDlgItemText(wnd, IDC_DISCOGS_FORMATTING_EDIT, conf.release_discogs_format_string);
	uSetDlgItemText(wnd, IDC_FILE_FORMATTING_EDIT, conf.release_file_format_string);
}

void CConfigurationDialog::init_tagging_dialog(HWND wnd) {
	uButton_SetCheck(wnd, IDC_ENABLE_ANV_CHECK, conf.replace_ANVs);
	uButton_SetCheck(wnd, IDC_MOVE_THE_AT_BEGINNING_CHECK, conf.move_the_at_beginning);
	uButton_SetCheck(wnd, IDC_DISCARD_NUMERIC_SUFFIXES_CHECK, conf.discard_numeric_suffix);

	uButton_SetCheck(wnd, IDC_SKIP_PREVIEW_DIALOG, conf.skip_mng_flag & SkipMng::SKIP_PREVIEW_DLG);
	uButton_SetCheck(wnd, IDC_REMOVE_OTHER_TAGS, conf.remove_other_tags);
	uSetDlgItemText(wnd, IDC_REMOVE_EXCLUDING_TAGS, conf.raw_remove_exclude_tags);
}

void CConfigurationDialog::init_caching_dialog(HWND wnd) {
	uButton_SetCheck(wnd, IDC_HIDDEN_AS_REGULAR_CHECK, conf.parse_hidden_as_regular);
	uButton_SetCheck(wnd, IDC_SKIP_VIDEO_TRACKS, conf.skip_video_tracks);
	uButton_SetCheck(wnd, IDC_CFG_CACHE_USE_OFFLINE_CACHE, conf.cache_offline_cache_flag & ol::CacheFlags::OC_READ);
	uButton_SetCheck(wnd, IDC_CFG_CACHE_WRITE_OFFLINE_CACHE, conf.cache_offline_cache_flag & ol::CacheFlags::OC_WRITE);
	original_parsing = conf.parse_hidden_as_regular;
	original_skip_video = conf.skip_video_tracks;

	pfc::string8 num;
	num << conf.cache_max_objects;
	uSetDlgItemText(wnd, IDC_CACHED_OBJECTS_EDIT, num);

	pfc::string8 text;
	text << "Releases (" << discogs_interface->release_cache_size() << ")";
	set_window_text(wnd, IDC_CLEAR_CACHE_RELEASES, text);
	text = "";
	text << "Master Releases (" << discogs_interface->master_release_cache_size() << ")";
	set_window_text(wnd, IDC_CLEAR_CACHE_MASTERS, text);
	text = "";
	text << "Artists (" << discogs_interface->artist_cache_size() << ")";
	set_window_text(wnd, IDC_CLEAR_CACHE_ARTISTS, text);
	text = "";
	text << "Collection (" << discogs_interface->collection_cache_size() << ")";
	set_window_text(wnd, IDC_CLEAR_CACHE_COLLECTION, text);

	::EnableWindow(::uGetDlgItem(wnd, IDC_CLEAR_CACHE_RELEASES), discogs_interface->release_cache_size() ? TRUE : FALSE);
	::EnableWindow(::uGetDlgItem(wnd, IDC_CLEAR_CACHE_MASTERS), discogs_interface->master_release_cache_size() ? TRUE : FALSE);
	::EnableWindow(::uGetDlgItem(wnd, IDC_CLEAR_CACHE_ARTISTS), discogs_interface->artist_cache_size() ? TRUE : FALSE);
	::EnableWindow(::uGetDlgItem(wnd, IDC_CLEAR_CACHE_COLLECTION), discogs_interface->collection_cache_size() ? TRUE : FALSE);
}

void CConfigurationDialog::init_db_dialog(HWND wnd) {
	uButton_SetCheck(wnd, IDC_CFG_DB_DC_FLAG_SEARCH, DBFlags(conf.db_dc_flag).Search());
	uButton_SetCheck(wnd, IDC_CFG_DB_DC_FLAG_SEARCH_LIKE, DBFlags(conf.db_dc_flag).SearchLike());
	uButton_SetCheck(wnd, IDC_CFG_DB_DC_FLAG_SEARCH_ANV, DBFlags(conf.db_dc_flag).SearchAnv());
	uButton_SetCheck(wnd, IDC_CFG_DB_DC_FLAG_ONLINE_RELEASES, DBFlags(conf.db_dc_flag).WantArtwork());
	uSetDlgItemText(wnd, IDC_DB_DC_PATH, conf.db_dc_path);
}

void CConfigurationDialog::init_art_dialog(HWND wnd) {
	uButton_SetCheck(wnd, IDC_SAVE_ALBUM_ART_CHECK, conf.save_album_art);
	uButton_SetCheck(wnd, IDC_ALBUM_ART_FETCH_ALL_CHECK, conf.album_art_fetch_all);
	uButton_SetCheck(wnd, IDC_ALBUM_ART_EMBED, conf.embed_album_art);
	uSetDlgItemText(wnd, IDC_ALBUM_ART_DIR_EDIT, conf.album_art_directory_string);
	uSetDlgItemText(wnd, IDC_ALBUM_ART_PREFIX_EDIT, conf.album_art_filename_string);
	uButton_SetCheck(wnd, IDC_ALBUM_ART_OVERWRITE_CHECK, conf.album_art_overwrite);

	uButton_SetCheck(wnd, IDC_SAVE_ARTIST_ART_CHECK, conf.save_artist_art);
	uButton_SetCheck(wnd, IDC_ARTIST_ART_FETCH_ALL_CHECK, conf.artist_art_fetch_all);
	uSetDlgItemText(wnd, IDC_ARTIST_ART_IDS_EDIT, conf.artist_art_id_format_string);
	uButton_SetCheck(wnd, IDC_ARTIST_ART_EMBED, conf.embed_artist_art);
	uSetDlgItemText(wnd, IDC_ARTIST_ART_DIR_EDIT, conf.artist_art_directory_string);
	uSetDlgItemText(wnd, IDC_ARTIST_ART_PREFIX_EDIT, conf.artist_art_filename_string);
	uButton_SetCheck(wnd, IDC_ARTIST_ART_OVERWRITE_CHECK, conf.artist_art_overwrite);
}

void CConfigurationDialog::init_ui_dialog(HWND wnd) {
	uButton_SetCheck(wnd, IDC_CFG_UI_HISTORY_ENABLED, HIWORD(conf.history_enabled_max));
	size_t imax = LOWORD(conf.history_enabled_max);
	uSetDlgItemText(wnd, IDC_UI_HISTORY_MAX_ITEMS, std::to_string(imax).c_str());
	uButton_SetCheck(wnd, IDC_RELEASE_ENTER_KEY_OVR, conf.release_enter_key_override);
	uButton_SetCheck(wnd, IDC_CHK_FIND_RELEASE_STATS, conf.find_release_dlg_flags & FLG_SHOW_RELEASE_TREE_STATS);
	InitComboRowStyle(wnd, IDC_CMB_CONFIG_LIST_STYLE, conf.list_style);
	HWND cmb = ::uGetDlgItem(wnd, IDC_CMB_CONFIG_LIST_STYLE);
	BOOL res = ::SendMessage(cmb, CB_SETMINVISIBLE, 10, 0L);
}

void CConfigurationDialog::init_oauth_dialog(HWND wnd) {
	token_edit = ::uGetDlgItem(wnd, IDC_OAUTH_TOKEN_EDIT);
	secret_edit = ::uGetDlgItem(wnd, IDC_OAUTH_SECRET_EDIT);
	oauth_msg = ::uGetDlgItem(wnd, IDC_STATIC_CONF_OAUTH_MSG);

	uSetWindowText(token_edit, conf.oauth_token);
	uSetWindowText(secret_edit, conf.oauth_token_secret);

	uSetWindowText(oauth_msg, "Click to test if OAuth is working.");
}

void CConfigurationDialog::show_oauth_msg(pfc::string8 msg, bool iserror) {
	
	if (g_current_tab != CONF_OATH_TAB) {
		HWND hWndTab = uGetDlgItem(IDC_TAB);
		uSendMessage(hWndTab, TCM_SETCURSEL, CONF_OATH_TAB, 0);
		show_tab(CONF_OATH_TAB);
	}

	uSetDlgItemText(g_hWndCurrentTab, IDC_STATIC_CONF_OAUTH_MSG, msg);
}

void CConfigurationDialog::save_searching_dialog(HWND wnd, bool dlgbind) {

	foo_conf* conf_ptr = dlgbind ? &conf_dlg_edit : &conf;

	conf_ptr->enable_autosearch = uButton_GetCheck(wnd, IDC_ENABLE_AUTO_SEARCH_CHECK);
	
	if (uButton_GetCheck(wnd, IDC_SKIP_LOAD_RELEASES_IF_IDED_CHECK))
		conf_ptr->skip_mng_flag |= SkipMng::SKIP_LOAD_RELEASES_TASK_IDED;
	else
		conf_ptr->skip_mng_flag &= ~SkipMng::SKIP_LOAD_RELEASES_TASK_IDED;
	if (uButton_GetCheck(wnd, IDC_SKIP_FIND_RELEASE_DIALOG_CHECK))
		conf_ptr->skip_mng_flag |= SkipMng::SKIP_FIND_RELEASE_DLG_IDED;
	else
		conf_ptr->skip_mng_flag &= ~SkipMng::SKIP_FIND_RELEASE_DLG_IDED;

	pfc::string8 text;
	uGetDlgItemText(wnd, IDC_RELEASE_FORMATTING_EDIT, text);
	conf_ptr->search_release_format_string = text;
	uGetDlgItemText(wnd, IDC_MASTER_FORMATTING_EDIT, text);
	conf_ptr->search_master_format_string = text;
	uGetDlgItemText(wnd, IDC_MASTER_SUB_FORMATTING_EDIT, text);
	conf_ptr->search_master_sub_format_string = text;

	conf_ptr->list_style = ::uSendDlgItemMessage(wnd, IDC_CMB_CONFIG_LIST_STYLE, CB_GETCURSEL, 0, 0);
}

bool CConfigurationDialog::cfg_searching_has_changed() {
	bool bres = false;
	bool bcmp = false;

	bres |= conf.enable_autosearch != conf_dlg_edit.enable_autosearch;
	bres |= conf.skip_mng_flag != conf_dlg_edit.skip_mng_flag;

	bres |= conf.release_enter_key_override != conf_dlg_edit.release_enter_key_override;

	bcmp = stricmp_utf8(conf.search_release_format_string, conf_dlg_edit.search_release_format_string);
	bres |= bcmp;
	bcmp = stricmp_utf8(conf.search_master_format_string, conf_dlg_edit.search_master_format_string);
	bres |= bcmp;
	bcmp = stricmp_utf8(conf.search_master_sub_format_string, conf_dlg_edit.search_master_sub_format_string);
	bres |= bcmp;

	bres |= conf.list_style != conf_dlg_edit.list_style;

	return bres;
}

void CConfigurationDialog::save_matching_dialog(HWND wnd, bool dlgbind) {

	foo_conf* conf_ptr = dlgbind ? &conf_dlg_edit : &conf;

	conf_ptr->match_tracks_using_duration = uButton_GetCheck(wnd, IDC_MATCH_USING_DURATIONS);
	conf_ptr->match_tracks_using_number = uButton_GetCheck(wnd, IDC_MATCH_USING_NUMBERS);
	conf_ptr->assume_tracks_sorted = uButton_GetCheck(wnd, IDC_MATCH_ASSUME_SORTED);

	if (uButton_GetCheck(wnd, IDC_SKIP_RELEASE_DIALOG_CHECK))
		conf_ptr->skip_mng_flag |= SkipMng::SKIP_RELEASE_DLG_MATCHED;
	else
		conf_ptr->skip_mng_flag &= ~SkipMng::SKIP_RELEASE_DLG_MATCHED;

	pfc::string8 text;
	uGetDlgItemText(wnd, IDC_DISCOGS_FORMATTING_EDIT, text);
	conf_ptr->release_discogs_format_string = text;

	uGetDlgItemText(wnd, IDC_FILE_FORMATTING_EDIT, text);
	conf_ptr->release_file_format_string = text;

	if (uButton_GetCheck(wnd, IDC_SKIP_BRAINZ_MIBS_FETCH))
		conf_ptr->skip_mng_flag |= SkipMng::SKIP_BRAINZ_ID_FETCH;
	else
		conf_ptr->skip_mng_flag &= ~SkipMng::SKIP_BRAINZ_ID_FETCH;
}

bool CConfigurationDialog::cfg_matching_has_changed() {
	bool bres = false;
	bool bcmp = false;

	bres |= conf.match_tracks_using_duration != conf_dlg_edit.match_tracks_using_duration;
	bres |= conf.match_tracks_using_number != conf_dlg_edit.match_tracks_using_number;
	bres |= conf.assume_tracks_sorted != conf_dlg_edit.assume_tracks_sorted;
	bres |= conf.skip_mng_flag != conf_dlg_edit.skip_mng_flag;

	bcmp = stricmp_utf8(conf.release_discogs_format_string, conf_dlg_edit.release_discogs_format_string);
	bres |= bcmp;
	bcmp = stricmp_utf8(conf.release_file_format_string, conf_dlg_edit.release_file_format_string);
	bres |= bcmp;
	return bres;
}

void CConfigurationDialog::save_tagging_dialog(HWND wnd, bool dlgbind) {

	foo_conf* conf_ptr = dlgbind ? &conf_dlg_edit : &conf;

	conf_ptr->replace_ANVs = uButton_GetCheck(wnd, IDC_ENABLE_ANV_CHECK);
	conf_ptr->move_the_at_beginning = uButton_GetCheck(wnd, IDC_MOVE_THE_AT_BEGINNING_CHECK);
	conf_ptr->discard_numeric_suffix = uButton_GetCheck(wnd, IDC_DISCARD_NUMERIC_SUFFIXES_CHECK);

	if (uButton_GetCheck(wnd, IDC_SKIP_PREVIEW_DIALOG))
		conf_ptr->skip_mng_flag |= SkipMng::SKIP_PREVIEW_DLG;
	else
		conf_ptr->skip_mng_flag &= ~SkipMng::SKIP_PREVIEW_DLG;
	
	conf_ptr->remove_other_tags = uButton_GetCheck(wnd, IDC_REMOVE_OTHER_TAGS);
	pfc::string8 text;
	uGetDlgItemText(wnd, IDC_REMOVE_EXCLUDING_TAGS, text);
	conf_ptr->set_remove_exclude_tags(text);
}

bool CConfigurationDialog::cfg_tagging_has_changed() {
	bool bres = false;
	bool bcmp = false;
	bres |= conf.replace_ANVs != conf_dlg_edit.replace_ANVs;
	bres |= conf.move_the_at_beginning != conf_dlg_edit.move_the_at_beginning;
	bres |= conf.discard_numeric_suffix != conf_dlg_edit.discard_numeric_suffix;
	bres |= conf.skip_mng_flag != conf_dlg_edit.skip_mng_flag;
	bres |= conf.remove_other_tags != conf_dlg_edit.remove_other_tags;
	bcmp = stricmp_utf8(conf.raw_remove_exclude_tags, conf_dlg_edit.raw_remove_exclude_tags);
	bres |= bcmp;
	return bres;
}

void CConfigurationDialog::save_caching_dialog(HWND wnd, bool dlgbind) {

	foo_conf* conf_ptr = dlgbind ? &conf_dlg_edit : &conf;

	conf_ptr->parse_hidden_as_regular = uButton_GetCheck(wnd, IDC_HIDDEN_AS_REGULAR_CHECK);
	conf_ptr->skip_video_tracks = uButton_GetCheck(wnd, IDC_SKIP_VIDEO_TRACKS);
	
	if (uButton_GetCheck(wnd, IDC_CFG_CACHE_USE_OFFLINE_CACHE)) 
		conf_ptr->cache_offline_cache_flag |= ol::CacheFlags::OC_READ;
	else
		conf_ptr->cache_offline_cache_flag &= ~ol::CacheFlags::OC_READ;
	
	if (uButton_GetCheck(wnd, IDC_CFG_CACHE_WRITE_OFFLINE_CACHE))
		conf_ptr->cache_offline_cache_flag |= ol::CacheFlags::OC_WRITE;
	else
		conf_ptr->cache_offline_cache_flag &= ~ol::CacheFlags::OC_WRITE;
	
	if (original_parsing != conf_ptr->parse_hidden_as_regular || original_skip_video != conf_ptr->skip_video_tracks) {
		discogs_interface->reset_release_cache();
	}

	pfc::string8 text;
	uGetDlgItemText(wnd, IDC_CACHED_OBJECTS_EDIT, text);

	if (!is_number(text.c_str())) return;

	conf_ptr->cache_max_objects = atol(text);
	discogs_interface->set_cache_size(conf_ptr->cache_max_objects);
}

bool CConfigurationDialog::cfg_caching_has_changed() {
	bool bres = false;
	bres |= conf.parse_hidden_as_regular != conf_dlg_edit.parse_hidden_as_regular;
	bres |= conf.skip_video_tracks != conf_dlg_edit.skip_video_tracks;
	bres |= conf.cache_max_objects != conf_dlg_edit.cache_max_objects;
	bres |= conf.cache_offline_cache_flag != conf_dlg_edit.cache_offline_cache_flag;
	return bres;
}

void CConfigurationDialog::save_db_dialog(HWND wnd, bool dlgbind) {
	
	foo_conf* conf_ptr = dlgbind ? &conf_dlg_edit : &conf;
	
	pfc::string8 str;
	uGetDlgItemText(wnd, IDC_DB_DC_PATH, str);
	conf_ptr->db_dc_path = str;

	if (uButton_GetCheck(wnd, IDC_CFG_DB_DC_FLAG_SEARCH))
		conf_ptr->db_dc_flag |= DBFlags::DB_SEARCH;
	else
		conf_ptr->db_dc_flag &= ~DBFlags::DB_SEARCH;

	if (uButton_GetCheck(wnd, IDC_CFG_DB_DC_FLAG_SEARCH_LIKE))
		conf_ptr->db_dc_flag |= DBFlags::DB_SEARCH_LIKE;
	else
		conf_ptr->db_dc_flag &= ~DBFlags::DB_SEARCH_LIKE;

	if (uButton_GetCheck(wnd, IDC_CFG_DB_DC_FLAG_SEARCH_ANV))
		conf_ptr->db_dc_flag |= DBFlags::DB_SEARCH_ANV;
	else
		conf_ptr->db_dc_flag &= ~DBFlags::DB_SEARCH_ANV;

	if (uButton_GetCheck(wnd, IDC_CFG_DB_DC_FLAG_ONLINE_RELEASES))
		conf_ptr->db_dc_flag |= DBFlags::DB_DWN_ARTWORK;
	else
		conf_ptr->db_dc_flag &= ~DBFlags::DB_DWN_ARTWORK;
}

bool CConfigurationDialog::cfg_db_has_changed() {
	bool bres = false;

	bres |= conf.db_dc_flag != conf_dlg_edit.db_dc_flag;
	bres |= stricmp_utf8(conf.db_dc_path, conf_dlg_edit.db_dc_path) != 0;

	return bres;
}

void CConfigurationDialog::save_art_dialog(HWND wnd, bool dlgbind) {
	pfc::string8 temp;
	foo_conf* conf_ptr = dlgbind? &conf_dlg_edit : &conf;

	conf_ptr->save_album_art = uButton_GetCheck(wnd, IDC_SAVE_ALBUM_ART_CHECK);
	conf_ptr->album_art_fetch_all = uButton_GetCheck(wnd, IDC_ALBUM_ART_FETCH_ALL_CHECK);
	conf_ptr->embed_album_art = uButton_GetCheck(wnd, IDC_ALBUM_ART_EMBED);
	uGetDlgItemText(wnd, IDC_ALBUM_ART_DIR_EDIT, temp);
	conf_ptr->album_art_directory_string = temp;
	uGetDlgItemText(wnd, IDC_ALBUM_ART_PREFIX_EDIT, temp);
	conf_ptr->album_art_filename_string = temp;
	conf_ptr->album_art_overwrite = uButton_GetCheck(wnd, IDC_ALBUM_ART_OVERWRITE_CHECK);
	
	conf_ptr->save_artist_art = uButton_GetCheck(wnd, IDC_SAVE_ARTIST_ART_CHECK);
	conf_ptr->artist_art_fetch_all = uButton_GetCheck(wnd, IDC_ARTIST_ART_FETCH_ALL_CHECK);
	uGetDlgItemText(wnd, IDC_ARTIST_ART_IDS_EDIT, temp);
	conf_ptr->artist_art_id_format_string = temp;
	conf_ptr->embed_artist_art = uButton_GetCheck(wnd, IDC_ARTIST_ART_EMBED);
	uGetDlgItemText(wnd, IDC_ARTIST_ART_DIR_EDIT, temp);
	conf_ptr->artist_art_directory_string = temp;
	uGetDlgItemText(wnd, IDC_ARTIST_ART_PREFIX_EDIT, temp);
	conf_ptr->artist_art_filename_string = temp;
	conf_ptr->artist_art_overwrite = uButton_GetCheck(wnd, IDC_ARTIST_ART_OVERWRITE_CHECK);
}

bool CConfigurationDialog::cfg_art_has_changed() {
	bool bres = false;
	bool bcmp = false;

	bres |= conf.save_album_art != conf_dlg_edit.save_album_art;
	bres |= conf.album_art_fetch_all != conf_dlg_edit.album_art_fetch_all;
	bres |= conf.embed_album_art != conf_dlg_edit.embed_album_art;
	
	bcmp = !(stricmp_utf8(conf.album_art_directory_string, conf_dlg_edit.album_art_directory_string) == 0);
	bres |= bcmp;
	bcmp = !(stricmp_utf8(conf.album_art_filename_string, conf_dlg_edit.album_art_filename_string) == 0);
	bres |= bcmp;

	bres |= conf.album_art_overwrite != conf_dlg_edit.album_art_overwrite;
	bres |= conf.save_artist_art != conf_dlg_edit.save_artist_art;
	bres |= conf.artist_art_fetch_all != conf_dlg_edit.artist_art_fetch_all;

	bcmp = !(stricmp_utf8(conf.artist_art_id_format_string, conf_dlg_edit.artist_art_id_format_string) == 0);
	bres |= bcmp;
	
	bres |= conf.embed_artist_art != conf_dlg_edit.embed_artist_art;
	
	bcmp = !(stricmp_utf8(conf.artist_art_directory_string, conf_dlg_edit.artist_art_directory_string) == 0);
	bres |= bcmp;

	bcmp = !(stricmp_utf8(conf.artist_art_filename_string, conf_dlg_edit.artist_art_filename_string) == 0);
	bres |= bcmp;

	bres |= conf.artist_art_overwrite != conf_dlg_edit.artist_art_overwrite;
	return bres;
}

void CConfigurationDialog::save_ui_dialog(HWND wnd, bool dlgbind) {

	foo_conf* conf_ptr = dlgbind ? &conf_dlg_edit : &conf;

	bool history_enabled = uButton_GetCheck(wnd, IDC_CFG_UI_HISTORY_ENABLED);
	size_t max_items = atoi(uGetDlgItemText(wnd, IDC_UI_HISTORY_MAX_ITEMS));

	conf_ptr->history_enabled_max = MAKELPARAM(max_items, history_enabled ? 1 : 0);
	conf_ptr->release_enter_key_override = uButton_GetCheck(wnd, IDC_RELEASE_ENTER_KEY_OVR);
	bool checked = uButton_GetCheck(wnd, IDC_CHK_FIND_RELEASE_STATS);

	//calc & apply stat flag
	conf_ptr->find_release_dlg_flags &= ~FLG_SHOW_RELEASE_TREE_STATS;
	if (checked) {
		conf_ptr->find_release_dlg_flags |= FLG_SHOW_RELEASE_TREE_STATS;
	}

	conf_ptr->list_style = ::uSendDlgItemMessage(wnd, IDC_CMB_CONFIG_LIST_STYLE, CB_GETCURSEL, 0, 0);
}

bool CConfigurationDialog::cfg_ui_has_changed() {


	bool bres = false;
	bres |= conf.history_enabled_max != conf_dlg_edit.history_enabled_max;
	bres |= conf.release_enter_key_override != conf_dlg_edit.release_enter_key_override;
	bres |= conf.list_style != conf_dlg_edit.list_style;
	return bres;
}

void CConfigurationDialog::save_oauth_dialog(HWND wnd, bool dlgbind) {

	foo_conf* conf_ptr = dlgbind ? &conf_dlg_edit : &conf;

	pfc::string8 text;
	uGetDlgItemText(wnd, IDC_OAUTH_TOKEN_EDIT, text);
	conf_ptr->oauth_token = text;

	uGetDlgItemText(wnd, IDC_OAUTH_SECRET_EDIT, text);
	conf_ptr->oauth_token_secret = text;
}

bool CConfigurationDialog::cfg_oauth_has_changed() {
	bool bres = false;
	bool bcmp = false;

	bcmp = !(stricmp_utf8(conf.oauth_token, conf_dlg_edit.oauth_token) == 0);
	bres |= bcmp;
	bcmp = !(stricmp_utf8(conf.oauth_token_secret, conf_dlg_edit.oauth_token_secret) == 0);
	bres |= bcmp;

	return bres;
}

void CConfigurationDialog::init_current_tab() {

	if (g_hWndCurrentTab == g_hWndTabDialog[CONF_FIND_RELEASE_TAB]) {
		init_searching_dialog(g_hWndTabDialog[CONF_FIND_RELEASE_TAB]);
	}
	else if (g_hWndCurrentTab == g_hWndTabDialog[CONF_MATCHING_TAB]) {
		init_matching_dialog(g_hWndTabDialog[CONF_MATCHING_TAB]);
	}
	else if (g_hWndCurrentTab == g_hWndTabDialog[CONF_TAGGING_TAB]) {
		init_tagging_dialog(g_hWndTabDialog[CONF_TAGGING_TAB]);
	}
	else if (g_hWndCurrentTab == g_hWndTabDialog[CONF_CACHING_TAB]) {
		init_caching_dialog(g_hWndTabDialog[CONF_CACHING_TAB]);
	}
#ifdef DB_DC
	else if (g_hWndCurrentTab == g_hWndTabDialog[CONF_DB_TAB]) {
		init_caching_dialog(g_hWndTabDialog[CONF_DB_TAB]);
	}
#endif
	else if (g_hWndCurrentTab == g_hWndTabDialog[CONF_ART_TAB]) {
		init_art_dialog(g_hWndTabDialog[CONF_ART_TAB]);
	}
	else if (g_hWndCurrentTab == g_hWndTabDialog[CONF_UI_TAB]) {
		init_ui_dialog(g_hWndTabDialog[CONF_UI_TAB]);
	}
	else if (g_hWndCurrentTab == g_hWndTabDialog[CONF_OATH_TAB]) {
		init_oauth_dialog(g_hWndTabDialog[CONF_OATH_TAB]);
	}
}

BOOL CConfigurationDialog::on_tagging_dialog_message(HWND wnd, UINT msg, WPARAM wp, LPARAM lp) {
	switch (msg) {
	case WM_INITDIALOG:
		setting_dlg = true;
		init_tagging_dialog(wnd);
		setting_dlg = false;
		return TRUE;
	case WM_COMMAND:
		if (LOWORD(wp) == IDC_EDIT_TAG_MAPPINGS_BUTTON) {
			if (!g_discogs->tag_mappings_dialog) {
				g_discogs->tag_mappings_dialog = fb2k::newDialog<CTagMappingDialog>(core_api::get_main_window());
			}
			else {
				CDialogImpl* tmdlg = pfc::downcast_guarded<CDialogImpl*>(g_discogs->tag_mappings_dialog);
				::SetFocus(tmdlg->m_hWnd);
			}
		}
		else if (LOWORD(wp) == IDC_EDIT_CAT_CREDIT_BTN) {
#ifdef CAT_CRED
			if (!g_discogs->tag_credit_dialog) {
				g_discogs->tag_credit_dialog = fb2k::newDialog<CTagCreditDialog>(core_api::get_main_window()/*, nullptr*/);
			}
			else {
				CDialogImpl* tmdlg = pfc::downcast_guarded<CDialogImpl*>(g_discogs->tag_credit_dialog);
				::SetFocus(tmdlg->m_hWnd);
			}
#endif // CAT_CRED
		}
		else  {
			if ((HIWORD(wp) == BN_CLICKED) || (HIWORD(wp) == EN_UPDATE)) {
				if (!setting_dlg) {
					save_tagging_dialog(wnd, true);
					OnChanged();
				}
			}
		}
		return FALSE;
	}
	return FALSE;
}

INT_PTR WINAPI CConfigurationDialog::tagging_dialog_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	CConfigurationDialog * p_this;

	if (msg == WM_INITDIALOG) {
		p_this = (CConfigurationDialog*)(lParam); //retrieve pointer to class
		::uSetWindowLong(hWnd, GWL_USERDATA, (LPARAM)p_this); //store it for future use
	}
	else {
		// if isnt wm_create, retrieve pointer to class
		p_this = reinterpret_cast<CConfigurationDialog*>(::uGetWindowLong(hWnd, GWL_USERDATA));
	}
	return p_this ? p_this->on_tagging_dialog_message(hWnd, msg, wParam, lParam) : FALSE;
}

INT_PTR WINAPI CConfigurationDialog::caching_dialog_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	CConfigurationDialog * p_this;
	if (msg == WM_INITDIALOG) {
		p_this = (CConfigurationDialog*)(lParam); //retrieve pointer to class
		::uSetWindowLong(hWnd, GWL_USERDATA, (LPARAM)p_this); //store it for future use
	}
	else {
		// if isnt wm_create, retrieve pointer to class
		p_this = reinterpret_cast<CConfigurationDialog*>(::uGetWindowLong(hWnd, GWL_USERDATA));
	}
	return p_this ? p_this->on_caching_dialog_message(hWnd, msg, wParam, lParam) : FALSE;
}

BOOL CConfigurationDialog::on_caching_dialog_message(HWND wnd, UINT msg, WPARAM wp, LPARAM lp) {
	switch (msg) {
		case WM_INITDIALOG:
			setting_dlg = true;
			init_caching_dialog(wnd);
			setting_dlg = false;
			return TRUE;
		case WM_COMMAND:
			if (LOWORD(wp) == IDC_CLEAR_CACHE_RELEASES) {
				discogs_interface->reset_release_cache();
				::EnableWindow(::uGetDlgItem(wnd, IDC_CLEAR_CACHE_RELEASES), FALSE);
			}
			else if (LOWORD(wp) == IDC_CLEAR_CACHE_MASTERS) {
				discogs_interface->reset_master_release_cache();
				::EnableWindow(::uGetDlgItem(wnd, IDC_CLEAR_CACHE_MASTERS), FALSE);
			}
			else if (LOWORD(wp) == IDC_CLEAR_CACHE_ARTISTS) {
				discogs_interface->reset_artist_cache();
				::EnableWindow(::uGetDlgItem(wnd, IDC_CLEAR_CACHE_ARTISTS), FALSE);
			}
			else if (LOWORD(wp) == IDC_CLEAR_CACHE_COLLECTION) {
				discogs_interface->reset_collection_cache();
				::EnableWindow(::uGetDlgItem(wnd, IDC_CLEAR_CACHE_COLLECTION), FALSE);
			}
			else {
				if ((HIWORD(wp) == BN_CLICKED) || (HIWORD(wp) == EN_UPDATE)) {
					if (!setting_dlg) {
						save_caching_dialog(wnd, true);
						OnChanged();
					}
				}
			}
			return FALSE;
		}
	return FALSE;
}

#ifdef DB_DC
bool open_db_path(HWND hwnd, pfc::string8& out) {

	char filename[MAX_PATH];
	OPENFILENAMEA ofn;
	ZeroMemory(&filename, sizeof(filename));
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFilter = "Sqlite Files (*.db)\0*.db\0All Files (*.*)\0*.*\0";
	ofn.lpstrFile = filename;
	ofn.nMaxFile = MAX_PATH - 1;
	ofn.lpstrTitle = "Discogs backup database...";
	ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;

	if (GetOpenFileNameA(&ofn)) {
		log_msg(filename);
	}

	{
		pfc::string8 tmpFullPath(ofn.lpstrFile);
		if (!tmpFullPath.length()) {
			return false;
		}
	}

	pfc::stringcvt::string_utf8_from_codepage cvt_utf8(0, ofn.lpstrFile);

	out = cvt_utf8.get_ptr();
	return out.get_length();
}

INT_PTR WINAPI CConfigurationDialog::db_dialog_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	CConfigurationDialog* p_this;
	if (msg == WM_INITDIALOG) {
		p_this = (CConfigurationDialog*)(lParam); //retrieve pointer to class
		::uSetWindowLong(hWnd, GWL_USERDATA, (LPARAM)p_this); //store it for future use
	}
	else {
		p_this = reinterpret_cast<CConfigurationDialog*>(::uGetWindowLong(hWnd, GWL_USERDATA));
	}
	return p_this ? p_this->on_db_dialog_message(hWnd, msg, wParam, lParam) : FALSE;
}

BOOL CConfigurationDialog::on_db_dialog_message(HWND wnd, UINT msg, WPARAM wp, LPARAM lp) {
	switch (msg) {
	case WM_INITDIALOG:
		setting_dlg = true;
		init_db_dialog(wnd);
		setting_dlg = false;
		return TRUE;
	case WM_COMMAND:
		if (LOWORD(wp) == IDC_CFG_DB_DC_OPEN_PATH) {
			pfc::string8 db_path;
			try {
				if (open_db_path(m_hWnd, db_path)) {
					uSetDlgItemText(wnd, IDC_DB_DC_PATH, db_path);
					on_test_db(m_hWnd, db_path);
				}
			}
			catch (foo_discogs_exception& e) {
				foo_discogs_exception ex;
				ex << "Database test failed." << e.what();
				throw ex;
			}
		}
		else {
			if ((HIWORD(wp) == BN_CLICKED) || (HIWORD(wp) == EN_UPDATE)) {
				if (!setting_dlg) {
					save_db_dialog(wnd, true);
					OnChanged();
				}
			}
		}
		return FALSE;
	}
	return FALSE;
}
#endif // DB_DC

INT_PTR WINAPI CConfigurationDialog::searching_dialog_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	CConfigurationDialog * p_this;
	if (msg == WM_INITDIALOG) {
		p_this = (CConfigurationDialog*)(lParam); //retrieve pointer to class
		::uSetWindowLong(hWnd, GWL_USERDATA, (LPARAM)p_this); //store it for future use
	}
	else {
		// if isnt wm_create, retrieve pointer to class
		p_this = reinterpret_cast<CConfigurationDialog*>(::uGetWindowLong(hWnd, GWL_USERDATA));
	}
	return p_this ? p_this->on_searching_dialog_message(hWnd, msg, wParam, lParam) : FALSE;
}

BOOL CConfigurationDialog::on_searching_dialog_message(HWND wnd, UINT msg, WPARAM wp, LPARAM lp) {
	switch (msg) {
		case WM_INITDIALOG:
			setting_dlg = true;
			init_searching_dialog(wnd);
			setting_dlg = false;
			return TRUE;
		case WM_COMMAND: {
			if ((HIWORD(wp) == BN_CLICKED) || (HIWORD(wp) == EN_UPDATE)) {
				if (!setting_dlg) {
					save_searching_dialog(wnd, true);
					OnChanged();
				}
			}
			else if (HIWORD(wp) == CBN_SELCHANGE) {
				switch (LOWORD(wp)) {
				case IDC_CMB_CONFIG_LIST_STYLE: {
					int s = ::uSendDlgItemMessage(wnd, IDC_CMB_CONFIG_LIST_STYLE, CB_GETCURSEL, 0, 0);
					int data = ::uSendDlgItemMessage(wnd, IDC_CMB_CONFIG_LIST_STYLE, CB_GETITEMDATA, s, 0);
					if (s != CB_ERR) {
						conf_dlg_edit.list_style = s;
					}
					OnChanged();
					break;
				}
				}
			}
			break;
		}
	}
	return FALSE;
}

INT_PTR WINAPI CConfigurationDialog::matching_dialog_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	CConfigurationDialog * p_this;
	if (msg == WM_INITDIALOG) {
		p_this = (CConfigurationDialog*)(lParam); //retrieve pointer to class
		::uSetWindowLong(hWnd, GWL_USERDATA, (LPARAM)p_this); //store it for future use
	}
	else {
		p_this = reinterpret_cast<CConfigurationDialog*>(::uGetWindowLong(hWnd, GWL_USERDATA));
	}
	return p_this ? p_this->on_matching_dialog_message(hWnd, msg, wParam, lParam) : FALSE;
}

BOOL CConfigurationDialog::on_matching_dialog_message(HWND wnd, UINT msg, WPARAM wp, LPARAM lp) {
	switch (msg) {
		case WM_INITDIALOG:
			setting_dlg = true;
			init_matching_dialog(wnd);
			setting_dlg = false;
			return TRUE;
		case WM_COMMAND: {
			if ((HIWORD(wp) == BN_CLICKED) || (HIWORD(wp) == EN_UPDATE)) {
				if (!setting_dlg) {
					save_matching_dialog(wnd, true);
					OnChanged();
				}
			}
			break; 
		}
	}
	return FALSE;
}

INT_PTR WINAPI CConfigurationDialog::art_dialog_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	CConfigurationDialog * p_this;

	if (msg == WM_INITDIALOG) {
		p_this = (CConfigurationDialog*)(lParam); //retrieve pointer to class
		::uSetWindowLong(hWnd, GWL_USERDATA, (LPARAM)p_this); //store it for future use
	}
	else {
		// if isnt wm_create, retrieve pointer to class
		p_this = reinterpret_cast<CConfigurationDialog*>(::uGetWindowLong(hWnd, GWL_USERDATA));
	}
	return p_this ? p_this->on_art_dialog_message(hWnd, msg, wParam, lParam) : FALSE;
}

BOOL CConfigurationDialog::on_art_dialog_message(HWND wnd, UINT msg, WPARAM wp, LPARAM lp) {
	switch (msg) {
		case WM_INITDIALOG:
			setting_dlg = true;
			init_art_dialog(wnd);
			setting_dlg = false;
			return TRUE;
		case WM_COMMAND: {
			if ((HIWORD(wp) == BN_CLICKED)|| (HIWORD(wp) == EN_UPDATE)) {
				if (!setting_dlg) {
				  save_art_dialog(wnd, true);
				  OnChanged();
				}
			}	
			break;
		}
	}
	return FALSE;
}

INT_PTR WINAPI CConfigurationDialog::ui_dialog_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	CConfigurationDialog* p_this;
	if (msg == WM_INITDIALOG) {
		p_this = (CConfigurationDialog*)(lParam); //retrieve pointer to class
		::uSetWindowLong(hWnd, GWL_USERDATA, (LPARAM)p_this); //store it for future use
	}
	else {
		p_this = reinterpret_cast<CConfigurationDialog*>(::uGetWindowLong(hWnd, GWL_USERDATA));
	}
	return p_this ? p_this->on_ui_dialog_message(hWnd, msg, wParam, lParam) : FALSE;
}

BOOL CConfigurationDialog::on_ui_dialog_message(HWND wnd, UINT msg, WPARAM wp, LPARAM lp) {
	switch (msg) {
		case WM_INITDIALOG:
			setting_dlg = true;
			init_ui_dialog(wnd);
			setting_dlg = false;
			return TRUE;
		case WM_COMMAND: {
			switch (wp) {
				case IDC_BTN_UI_CLEAR_HISTORY:
					if (g_discogs->find_release_dialog)
						g_discogs->find_release_dialog->get_oplogger()->zap_vhistory();
					on_delete_history(wnd, 0, true);
					break;
				default:
					if ((HIWORD(wp) == BN_CLICKED) || (HIWORD(wp) == EN_UPDATE)) {
						if (!setting_dlg) {
							save_ui_dialog(wnd, true);
							OnChanged();
						}
					}
					else if (HIWORD(wp) == CBN_SELCHANGE) {
						switch (LOWORD(wp)) {
							case IDC_CMB_CONFIG_LIST_STYLE: {
								int s = ::uSendDlgItemMessage(wnd, IDC_CMB_CONFIG_LIST_STYLE, CB_GETCURSEL, 0, 0);
								int data = ::uSendDlgItemMessage(wnd, IDC_CMB_CONFIG_LIST_STYLE, CB_GETITEMDATA, s, 0);
								if (s != CB_ERR) {
									conf_dlg_edit.list_style = s;
								}
								OnChanged();
								break;
							}
						}
					}
					break;
			}
			break;
		}
	}
	return FALSE;
}

INT_PTR WINAPI CConfigurationDialog::oauth_dialog_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	CConfigurationDialog * p_this;

	if (msg == WM_INITDIALOG) {
		p_this = (CConfigurationDialog*)(lParam); //retrieve pointer to class
		::uSetWindowLong(hWnd, GWL_USERDATA, (LPARAM)p_this); //store it for future use
	}
	else {
		// if isnt wm_create, retrieve pointer to class
		p_this = reinterpret_cast<CConfigurationDialog*>(::uGetWindowLong(hWnd, GWL_USERDATA));
	}
	return p_this ? p_this->on_oauth_dialog_message(hWnd, msg, wParam, lParam) : FALSE;
}

BOOL CConfigurationDialog::on_oauth_dialog_message(HWND wnd, UINT msg, WPARAM wp, LPARAM lp) {
	pfc::string8 text;
	switch (msg) {
		case WM_INITDIALOG:
			setting_dlg = true;
			init_oauth_dialog(wnd);
			setting_dlg = false;
			return TRUE;

		case WM_COMMAND:
			switch (wp) {
				case IDC_BTN_OAUTH_TEST:
					on_test_oauth(g_hWndTabDialog[CONF_OATH_TAB]);
					break;

				case IDC_BTN_OAUTH_AUTHORIZE:
					on_authorize_oauth(g_hWndTabDialog[CONF_OATH_TAB]);
					break;

				case IDC_BTN_OAUTH_GENERATE:
					on_generate_oauth(g_hWndTabDialog[CONF_OATH_TAB]);
					break;
				default:
					if ((HIWORD(wp) == BN_CLICKED) || (HIWORD(wp) == EN_UPDATE)) {
						if (!setting_dlg) {
							save_oauth_dialog(wnd, true);
							OnChanged();
						}
					}
					break;
			}
			break;
	}
	return FALSE;
}

#ifdef DB_DC
void CConfigurationDialog::on_test_db(HWND wnd, pfc::string8 dbpath) {
	service_ptr_t<test_db_process_callback> task = new service_impl_t<test_db_process_callback>(dbpath);

	m_tp.get()->run_modeless(task,
		threaded_process::flag_show_item | threaded_process::flag_show_progress_dual |
		threaded_process::flag_show_abort,
		m_hWnd,
		"Testing local Discogs database support..."
	);
}
#endif // DB_DC

void CConfigurationDialog::on_delete_history(HWND wnd, size_t max, bool zap) {

	if (!g_discogs->find_release_dialog) {
		g_discogs->find_release_dialog->get_oplogger()->zap_vhistory();
	}

	/* todo: service_ptr_t */
	sqldb db;	
	size_t inc = db.recharge_history(kcmdHistoryDeleteAll, ~0, {});
	db.close();
}

void CConfigurationDialog::on_test_oauth(HWND wnd) {
	pfc::string8 text;
	uGetDlgItemText(wnd, IDC_OAUTH_TOKEN_EDIT, text);
	pfc::string8 token = pfc::string8(text);

	uGetDlgItemText(wnd, IDC_OAUTH_SECRET_EDIT, text);
	pfc::string8 token_secret = pfc::string8(text);

	service_ptr_t<test_oauth_process_callback> task = new service_impl_t<test_oauth_process_callback>(token, token_secret);

	m_tp.get()->run_modeless(task,
		threaded_process::flag_show_item | threaded_process::flag_show_progress_dual |
		threaded_process::flag_show_abort,
		m_hWnd,
		"Testing Discogs OAuth support..."
	);
}

void CConfigurationDialog::on_authorize_oauth(HWND wnd) {
	service_ptr_t<authorize_oauth_process_callback> task = new service_impl_t<authorize_oauth_process_callback>();
	task->start(m_hWnd);
}

void CConfigurationDialog::on_generate_oauth(HWND wnd) {
	pfc::string8 code;
	uGetDlgItemText(wnd, IDC_OAUTH_PIN_EDIT, code);

	if (code.get_length() == 0) {
		show_oauth_msg("Enter a valid PIN code.", true);
		return;
	}

	service_ptr_t<generate_oauth_process_callback> task = new service_impl_t<generate_oauth_process_callback>(code);
	task->start(m_hWnd);
}

t_uint32 CConfigurationDialog::get_state() {
	t_uint32 state = preferences_state::resettable;
	if (HasChanged()) state |= preferences_state::changed;
	return state;
}

void CConfigurationDialog::apply() {

	pushcfg(false);

}

bool CConfigurationDialog::HasChanged() {
	bool bchanged = false;

	if (g_hWndCurrentTab == g_hWndTabDialog[CONF_FIND_RELEASE_TAB]) {
		bchanged = cfg_searching_has_changed();
	}
	else if (g_hWndCurrentTab == g_hWndTabDialog[CONF_MATCHING_TAB]) {
		bchanged = cfg_matching_has_changed();
	}
	else if (g_hWndCurrentTab == g_hWndTabDialog[CONF_TAGGING_TAB]) {
		bchanged = cfg_tagging_has_changed();
	}
	else if (g_hWndCurrentTab == g_hWndTabDialog[CONF_CACHING_TAB]) {
		bchanged = cfg_caching_has_changed();
	}
#ifdef DB_DC
	else if (g_hWndCurrentTab == g_hWndTabDialog[CONF_DB_TAB]) {
		bchanged = cfg_db_has_changed();
	}
#endif
	else if (g_hWndCurrentTab == g_hWndTabDialog[CONF_ART_TAB]) {
		bchanged = cfg_art_has_changed();
	}
	else if (g_hWndCurrentTab == g_hWndTabDialog[CONF_UI_TAB]) {
		bchanged = cfg_ui_has_changed();
	}
	else if (g_hWndCurrentTab == g_hWndTabDialog[CONF_OATH_TAB]) {
		bchanged = cfg_oauth_has_changed();
	}
	return bchanged;
}

void CConfigurationDialog::OnChanged() {
	m_callback->on_state_changed();
}

static preferences_page_factory_t<preferences_page_myimpl> g_preferences_page_myimpl_factory;