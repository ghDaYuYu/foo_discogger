#include "stdafx.h"

#include <libPPUI/CDialogResizeHelper.h>

#include "configuration_dialog.h"
#include "utils.h"
#include "tasks.h"

static HWND g_hWndTabDialog[NUM_TABS] = {nullptr};
static HWND g_hWndCurrentTab = nullptr;
static t_uint32 g_current_tab;

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
	tab_table.append_single(tab_entry("Searching", searching_dialog_proc, IDD_DIALOG_CONF_FIND_RELEASE_DIALOG));
	tab_table.append_single(tab_entry("Matching", matching_dialog_proc, IDD_DIALOG_CONF_MATCHING_DIALOG));
	tab_table.append_single(tab_entry("Tagging", tagging_dialog_proc, IDD_DIALOG_CONF_TAGGING));
	tab_table.append_single(tab_entry("Caching", caching_dialog_proc, IDD_DIALOG_CONF_CACHING_DIALOG));
	tab_table.append_single(tab_entry("Artwork", art_dialog_proc, IDD_DIALOG_CONF_ART));
	tab_table.append_single(tab_entry("OAuth && Network ", oauth_dialog_proc, IDD_DIALOG_CONF_OAUTH));
	//Create(p_parent);
}

CConfigurationDialog::~CConfigurationDialog() {
	g_discogs->configuration_dialog = nullptr;
}

//copy from libPPUI\CDialogResizeHelper.cpp
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
	// get handle of tab control
	HWND hWndTab = uGetDlgItem(IDC_TAB);

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
			apply();
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
		::ShowWindow(g_hWndCurrentTab, SW_SHOW);
	}
	return FALSE;
}

LRESULT CConfigurationDialog::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	save_searching_dialog(g_hWndTabDialog[CONF_FIND_RELEASE_TAB], true);
	save_caching_dialog(g_hWndTabDialog[CONF_CACHING_TAB], true);
	save_matching_dialog(g_hWndTabDialog[CONF_MATCHING_TAB], true);
	save_tagging_dialog(g_hWndTabDialog[CONF_TAGGING_TAB], true);
	save_art_dialog(g_hWndTabDialog[CONF_ART_TAB], true);
	save_oauth_dialog(g_hWndTabDialog[CONF_OATH_TAB], true);
	conf = conf_dlg_edit;
	CONF.save(new_conf::ConfFilter::CONF_FILTER_CONF, conf);
	CONF.load();

	discogs_interface->fetcher->update_oauth(conf.oauth_token, conf.oauth_token_secret);
	
	//(preferences_page_instance)
	//The host ensures that our dialog is destroyed
	//destroy();
	return TRUE;
}

LRESULT CConfigurationDialog::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	destroy();
	return TRUE;
}

LRESULT CConfigurationDialog::OnDefaults(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (uMessageBox(m_hWnd, "Reset component settings to default?", "Reset",
		MB_OKCANCEL | MB_ICONQUESTION | MB_DEFBUTTON2) == IDOK) {
		foo_discogs_conf temp;
		temp.oauth_token = conf.oauth_token;
		temp.oauth_token_secret = conf.oauth_token_secret;
		conf = temp;
		conf_dlg_edit = temp;
		init_searching_dialog(g_hWndTabDialog[CONF_FIND_RELEASE_TAB]);
		init_caching_dialog(g_hWndTabDialog[CONF_CACHING_TAB]);
		init_matching_dialog(g_hWndTabDialog[CONF_MATCHING_TAB]);
		init_tagging_dialog(g_hWndTabDialog[CONF_TAGGING_TAB]);
		init_art_dialog(g_hWndTabDialog[CONF_ART_TAB]);
		init_oauth_dialog(g_hWndTabDialog[CONF_OATH_TAB]);
	}

	conf.find_release_dialog_size = 0;
	conf.release_dialog_size = 0;
	conf.edit_tags_dialog_size = 0;
	conf.preview_tags_dialog_size = 0;

	conf.find_release_dialog_position = 0;
	conf.release_dialog_position = 0;
	conf.edit_tags_dialog_position = 0;
	conf.preview_tags_dialog_position = 0;

	conf.match_discogs_artwork_ra_width = conf.match_discogs_artwork_ra_width;
	conf.match_discogs_artwork_type_width = conf.match_discogs_artwork_type_width;
	conf.match_discogs_artwork_dim_width = conf.match_discogs_artwork_dim_width;
	conf.match_discogs_artwork_save_width = conf.match_discogs_artwork_save_width;
	conf.match_discogs_artwork_ovr_width = conf.match_discogs_artwork_ovr_width;
	conf.match_discogs_artwork_embed_width = conf.match_discogs_artwork_embed_width;
	conf.match_file_artwork_name_width = conf.match_file_artwork_name_width;
	conf.match_file_artwork_dim_width = conf.match_file_artwork_dim_width;
	conf.match_file_artwork_size_width = conf.match_file_artwork_size_width;

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


void CConfigurationDialog::show_tab(unsigned int num) {
	if (num >= 0 && num < NUM_TABS) {
		if (g_hWndCurrentTab != nullptr) {
			::ShowWindow(g_hWndCurrentTab, SW_HIDE);
		}
		g_hWndCurrentTab = nullptr;

		::SendDlgItemMessage(m_hWnd, IDC_TAB, TCM_SETCURSEL, num, 0);

		g_hWndCurrentTab = g_hWndTabDialog[num];
		::ShowWindow(g_hWndCurrentTab, SW_SHOW);
	}
}

void CConfigurationDialog::init_tagging_dialog(HWND wnd) {
	uButton_SetCheck(wnd, IDC_ENABLE_ANV_CHECK, conf.replace_ANVs);
	uButton_SetCheck(wnd, IDC_MOVE_THE_AT_BEGINNING_CHECK, conf.move_the_at_beginning);
	uButton_SetCheck(wnd, IDC_DISCARD_NUMERIC_SUFFIXES_CHECK, conf.discard_numeric_suffix);

	uButton_SetCheck(wnd, IDC_SKIP_PREVIEW_DIALOG, conf.skip_preview_dialog);
	uButton_SetCheck(wnd, IDC_REMOVE_OTHER_TAGS, conf.remove_other_tags);
	uSetDlgItemText(wnd, IDC_REMOVE_EXCLUDING_TAGS, conf.raw_remove_exclude_tags);
}

inline void set_window_text(HWND wnd, int IDC, const pfc::string8 &text) {
	pfc::stringcvt::string_wide_from_ansi wtext(text);
	::SetWindowText(::uGetDlgItem(wnd, IDC), (LPCTSTR)const_cast<wchar_t*>(wtext.get_ptr()));
}

void CConfigurationDialog::init_caching_dialog(HWND wnd) {
	uButton_SetCheck(wnd, IDC_HIDDEN_AS_REGULAR_CHECK, conf.parse_hidden_as_regular);
	uButton_SetCheck(wnd, IDC_SKIP_VIDEO_TRACKS, conf.skip_video_tracks);
	uButton_SetCheck(wnd, IDC_CFG_CACHE_USE_OFFLINE_CACHE, conf.cache_use_offline_cache & ol::CacheFlags::OC_READ);
	uButton_SetCheck(wnd, IDC_CFG_CACHE_WRITE_OFFLINE_CACHE, conf.cache_use_offline_cache & ol::CacheFlags::OC_WRITE);
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

void CConfigurationDialog::init_searching_dialog(HWND wnd) {
	uButton_SetCheck(wnd, IDC_ENABLE_AUTO_SEARCH_CHECK, conf.enable_autosearch);
	uButton_SetCheck(wnd, IDC_SKIP_FIND_RELEASE_DIALOG_CHECK, conf.skip_find_release_dialog_if_ided);
	uButton_SetCheck(wnd, IDC_RELEASE_ENTER_KEY_OVR, conf.release_enter_key_override);
	uSetDlgItemText(wnd, IDC_RELEASE_FORMATTING_EDIT, conf.search_release_format_string);
	uSetDlgItemText(wnd, IDC_MASTER_FORMATTING_EDIT, conf.search_master_format_string);
	uSetDlgItemText(wnd, IDC_MASTER_SUB_FORMATTING_EDIT, conf.search_master_sub_format_string);
}

void CConfigurationDialog::init_matching_dialog(HWND wnd) {
	uButton_SetCheck(wnd, IDC_MATCH_USING_DURATIONS, conf.match_tracks_using_duration);
	uButton_SetCheck(wnd, IDC_MATCH_USING_NUMBERS, conf.match_tracks_using_number);
	uButton_SetCheck(wnd, IDC_MATCH_ASSUME_SORTED, conf.assume_tracks_sorted);
	uButton_SetCheck(wnd, IDC_SKIP_RELEASE_DIALOG_CHECK, conf.skip_release_dialog_if_matched);
	uSetDlgItemText(wnd, IDC_DISCOGS_FORMATTING_EDIT, conf.release_discogs_format_string);
	uSetDlgItemText(wnd, IDC_FILE_FORMATTING_EDIT, conf.release_file_format_string);
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

void CConfigurationDialog::init_oauth_dialog(HWND wnd) {
	token_edit = ::uGetDlgItem(wnd, IDC_OAUTH_TOKEN_EDIT);
	secret_edit = ::uGetDlgItem(wnd, IDC_OAUTH_SECRET_EDIT);
	oauth_msg = ::uGetDlgItem(wnd, IDC_STATIC_CONF_OAUTH_MSG);

	uSetWindowText(token_edit, conf.oauth_token);
	uSetWindowText(secret_edit, conf.oauth_token_secret);

	HWND wndIconOk = ::uGetDlgItem(g_hWndCurrentTab, IDC_STATIC_OAUTH_ICO_OK);
	HWND wndIconErr = ::uGetDlgItem(g_hWndCurrentTab, IDC_STATIC_OAUTH_ICO_ERROR);
	::ShowWindow(wndIconOk, SW_HIDE);
	::ShowWindow(wndIconErr, SW_HIDE);
	uSetWindowText(oauth_msg, "Click to test if OAuth is working.");
}

void CConfigurationDialog::show_oauth_msg(pfc::string8 msg, bool iserror) {
	if (g_current_tab != CONF_OATH_TAB)
		show_tab(CONF_OATH_TAB);

	HWND wndIconOk = ::uGetDlgItem(g_hWndCurrentTab, IDC_STATIC_OAUTH_ICO_OK);
	HWND wndIconErr = ::uGetDlgItem(g_hWndCurrentTab, IDC_STATIC_OAUTH_ICO_ERROR);

	if (msg.length() > 0) {
		::ShowWindow(wndIconOk, iserror? SW_HIDE : SW_SHOW);
		::ShowWindow(wndIconErr, iserror? SW_SHOW : SW_HIDE);
	}
	else {
		::ShowWindow(wndIconOk, SW_HIDE);
		::ShowWindow(wndIconOk, SW_HIDE);
	}
	uSetDlgItemText(g_hWndCurrentTab, IDC_STATIC_CONF_OAUTH_MSG, msg);
}

void CConfigurationDialog::save_tagging_dialog(HWND wnd, bool dlgbind) {

	foo_discogs_conf* conf_ptr = dlgbind ? &conf_dlg_edit : &conf;

	conf_ptr->replace_ANVs = uButton_GetCheck(wnd, IDC_ENABLE_ANV_CHECK);
	conf_ptr->move_the_at_beginning = uButton_GetCheck(wnd, IDC_MOVE_THE_AT_BEGINNING_CHECK);
	conf_ptr->discard_numeric_suffix = uButton_GetCheck(wnd, IDC_DISCARD_NUMERIC_SUFFIXES_CHECK);

	conf_ptr->skip_preview_dialog = uButton_GetCheck(wnd, IDC_SKIP_PREVIEW_DIALOG);
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
	bres |= conf.skip_preview_dialog != conf_dlg_edit.skip_preview_dialog;
	bres |= conf.remove_other_tags != conf_dlg_edit.remove_other_tags;
	bcmp = !(stricmp_utf8(conf.raw_remove_exclude_tags, conf_dlg_edit.raw_remove_exclude_tags) == 0);
	bres |= bcmp;
	return bres;
}

void CConfigurationDialog::save_caching_dialog(HWND wnd, bool dlgbind) {

	foo_discogs_conf* conf_ptr = dlgbind ? &conf_dlg_edit : &conf;

	conf_ptr->parse_hidden_as_regular = uButton_GetCheck(wnd, IDC_HIDDEN_AS_REGULAR_CHECK);
	conf_ptr->skip_video_tracks = uButton_GetCheck(wnd, IDC_SKIP_VIDEO_TRACKS);
	
	if (uButton_GetCheck(wnd, IDC_CFG_CACHE_USE_OFFLINE_CACHE)) 
		conf_ptr->cache_use_offline_cache |= ol::CacheFlags::OC_READ;
	else
		conf_ptr->cache_use_offline_cache &= ~ol::CacheFlags::OC_READ;
	
	if (uButton_GetCheck(wnd, IDC_CFG_CACHE_WRITE_OFFLINE_CACHE))
		conf_ptr->cache_use_offline_cache |= ol::CacheFlags::OC_WRITE;
	else
		conf_ptr->cache_use_offline_cache &= ~ol::CacheFlags::OC_WRITE;
	
	if (original_parsing != conf_ptr->parse_hidden_as_regular || original_skip_video != conf_ptr->skip_video_tracks) {
		discogs_interface->reset_release_cache();
	}

	pfc::string8 text;
	uGetDlgItemText(wnd, IDC_CACHED_OBJECTS_EDIT, text);
	const char *str = text.get_ptr();
	for (size_t i = 0; i < strlen(str); i++) {
		if (!isdigit(str[i])) {
			return;
		}
	}
	conf_ptr->cache_max_objects = atol(str);
	discogs_interface->set_cache_size(conf_ptr->cache_max_objects);
}

bool CConfigurationDialog::cfg_caching_has_changed() {
	bool bres = false;
	bres |= conf.parse_hidden_as_regular != conf_dlg_edit.parse_hidden_as_regular;
	bres |= conf.skip_video_tracks != conf_dlg_edit.skip_video_tracks;
	bres |= conf.cache_max_objects != conf_dlg_edit.cache_max_objects;
	bres |= conf.cache_use_offline_cache != conf_dlg_edit.cache_use_offline_cache;
	return bres;
}

void CConfigurationDialog::save_searching_dialog(HWND wnd, bool dlgbind) {

	foo_discogs_conf* conf_ptr = dlgbind ? &conf_dlg_edit : &conf;

	conf_ptr->enable_autosearch = uButton_GetCheck(wnd, IDC_ENABLE_AUTO_SEARCH_CHECK);
	conf_ptr->skip_find_release_dialog_if_ided = uButton_GetCheck(wnd, IDC_SKIP_FIND_RELEASE_DIALOG_CHECK);
	conf_ptr->release_enter_key_override = uButton_GetCheck(wnd, IDC_RELEASE_ENTER_KEY_OVR);
	pfc::string8 text;
	uGetDlgItemText(wnd, IDC_RELEASE_FORMATTING_EDIT, text);
	conf_ptr->search_release_format_string = text;
	uGetDlgItemText(wnd, IDC_MASTER_FORMATTING_EDIT, text);
	conf_ptr->search_master_format_string = text;
	uGetDlgItemText(wnd, IDC_MASTER_SUB_FORMATTING_EDIT, text);
	conf_ptr->search_master_sub_format_string = text;
}

bool CConfigurationDialog::cfg_searching_has_changed() {
	bool bres = false;
	bool bcmp = false;

	bres |= conf.enable_autosearch != conf_dlg_edit.enable_autosearch;
	bres |= conf.skip_find_release_dialog_if_ided != conf_dlg_edit.skip_find_release_dialog_if_ided;
	bres |= conf.release_enter_key_override != conf_dlg_edit.release_enter_key_override;
	
	bcmp = !(stricmp_utf8(conf.search_release_format_string, conf_dlg_edit.search_release_format_string) == 0);
	bres |= bcmp;
	bcmp = !(stricmp_utf8(conf.search_master_format_string, conf_dlg_edit.search_master_format_string) == 0);
	bres |= bcmp;
	bcmp = !(stricmp_utf8(conf.search_master_sub_format_string, conf_dlg_edit.search_master_sub_format_string) == 0);
	bres |= bcmp;
	return bres;
}

void CConfigurationDialog::save_matching_dialog(HWND wnd, bool dlgbind) {

	foo_discogs_conf* conf_ptr = dlgbind ? &conf_dlg_edit : &conf;

	conf_ptr->match_tracks_using_duration = uButton_GetCheck(wnd, IDC_MATCH_USING_DURATIONS);
	conf_ptr->match_tracks_using_number = uButton_GetCheck(wnd, IDC_MATCH_USING_NUMBERS);
	conf_ptr->assume_tracks_sorted = uButton_GetCheck(wnd, IDC_MATCH_ASSUME_SORTED);
	conf_ptr->skip_release_dialog_if_matched = uButton_GetCheck(wnd, IDC_SKIP_RELEASE_DIALOG_CHECK);
	
	pfc::string8 text;
	uGetDlgItemText(wnd, IDC_DISCOGS_FORMATTING_EDIT, text);
	conf_ptr->release_discogs_format_string = text;

	uGetDlgItemText(wnd, IDC_FILE_FORMATTING_EDIT, text);
	conf_ptr->release_file_format_string = text;
}

bool CConfigurationDialog::cfg_matching_has_changed() {
	bool bres = false;
	bool bcmp = false;

	bres |= conf.match_tracks_using_duration != conf_dlg_edit.match_tracks_using_duration;
	bres |= conf.match_tracks_using_number != conf_dlg_edit.match_tracks_using_number;
	bres |= conf.assume_tracks_sorted != conf_dlg_edit.assume_tracks_sorted;
	bres |= conf.skip_release_dialog_if_matched != conf_dlg_edit.skip_release_dialog_if_matched;

	bcmp = !(stricmp_utf8(conf.release_discogs_format_string, conf_dlg_edit.release_discogs_format_string) == 0);
	bres |= bcmp;
	bcmp = !(stricmp_utf8(conf.release_file_format_string, conf_dlg_edit.release_file_format_string) == 0);
	bres |= bcmp;
	return bres;
}

void CConfigurationDialog::save_art_dialog(HWND wnd, bool dlgbind) {
	pfc::string8 temp;
	foo_discogs_conf* conf_ptr = dlgbind? &conf_dlg_edit : &conf;

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

void CConfigurationDialog::save_oauth_dialog(HWND wnd, bool dlgbind) {

	foo_discogs_conf* conf_ptr = dlgbind ? &conf_dlg_edit : &conf;

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
	else if (g_hWndCurrentTab == g_hWndTabDialog[CONF_ART_TAB]) {
		init_art_dialog(g_hWndTabDialog[CONF_ART_TAB]);
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
				g_discogs->tag_mappings_dialog = fb2k::newDialog<CNewTagMappingsDialog>(core_api::get_main_window());
			}
			else {
				CDialogImpl* tmdlg = pfc::downcast_guarded<CDialogImpl*>(g_discogs->tag_mappings_dialog);
				::SetFocus(tmdlg->m_hWnd);
			}
		}
		else {
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
		// if isnt wm_create, retrieve pointer to class
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
				case IDOAUTHTEST:
					on_test_oauth(g_hWndTabDialog[CONF_OATH_TAB]);
					break;

				case IDOAUTHAUTHORIZE:
					on_authorize_oauth(g_hWndTabDialog[CONF_OATH_TAB]);
					break;

				case IDOAUTHGENERATE:
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
		show_oauth_msg("You must enter a valid PIN code.", true);
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

void CConfigurationDialog::reset() {
	BOOL bdummy = FALSE;
	OnDefaults(0, 0, NULL, bdummy);
	apply();
	
	conf = CONF;
	conf_dlg_edit = CONF;

	OnChanged();
}

void CConfigurationDialog::apply() {
	BOOL bdummy = FALSE;
	OnOK(0, 0, NULL, bdummy);

	//OnChanged();
}

bool CConfigurationDialog::HasChanged() {
	//returns whether our dialog content is different from the current configuration (whether the apply button should be enabled or not)
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
	else if (g_hWndCurrentTab == g_hWndTabDialog[CONF_ART_TAB]) {
		bchanged = cfg_art_has_changed();
	}
	else if (g_hWndCurrentTab == g_hWndTabDialog[CONF_OATH_TAB]) {
		bchanged = cfg_oauth_has_changed();
	}
	return bchanged;
}

void CConfigurationDialog::OnChanged() {
	//tell the host that our state has changed to enable/disable the apply button appropriately.
	m_callback->on_state_changed();
}

static preferences_page_factory_t<preferences_page_myimpl> g_preferences_page_myimpl_factory;