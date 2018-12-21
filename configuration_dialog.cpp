#include "stdafx.h"

#include "configuration_dialog.h"
#include "utils.h"
#include "liboauthcpp/liboauthcpp.h"
#include "tasks.h"


static HWND g_hWndTabDialog[NUM_TABS] = {nullptr};
static HWND g_hWndCurrentTab = nullptr;
static t_uint32 g_current_tab;


CConfigurationDialog::CConfigurationDialog(HWND p_parent) {
	conf = CONF;
	tab_table.append_single(tab_entry("Searching", searching_dialog_proc, IDD_DIALOG_CONF_FIND_RELEASE_DIALOG));
	tab_table.append_single(tab_entry("Matching", matching_dialog_proc, IDD_DIALOG_CONF_MATCHING_DIALOG));
	tab_table.append_single(tab_entry("Tagging", tagging_dialog_proc, IDD_DIALOG_CONF_TAGGING));
	tab_table.append_single(tab_entry("Caching", caching_dialog_proc, IDD_DIALOG_CONF_CACHING_DIALOG));
	tab_table.append_single(tab_entry("Artwork", art_dialog_proc, IDD_DIALOG_CONF_ART));
	tab_table.append_single(tab_entry("OAuth", oauth_dialog_proc, IDD_DIALOG_CONF_OAUTH));
	Create(p_parent);
}

CConfigurationDialog::~CConfigurationDialog() {
	g_discogs->configuration_dialog = nullptr;
}

LRESULT CConfigurationDialog::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
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

	// now resize tab control and entire prefs window to fit all controls
	// (it is necessary to manually move the buttons and resize the dialog
	//  to ensure correct display on large font (120dpi) screens)

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

	// now resize the entire window
	RECT rcCtrl;
	::SetRect(&rcCtrl, 0, 0, 6, 26);
	::MapDialogRect(m_hWnd, &rcCtrl);
	rcCtrl.right += rcTabDialog.right;
	rcCtrl.bottom += rcTabDialog.bottom + GetSystemMetrics(SM_CYCAPTION);
	::SetWindowPos(m_hWnd, nullptr,
		rcCtrl.left, rcCtrl.top, rcCtrl.right - rcCtrl.left, rcCtrl.bottom - rcCtrl.top,
		SWP_NOZORDER | SWP_NOMOVE);

	// position the subdialogs in the inner part of the tab control
	uSendMessage(hWndTab, TCM_ADJUSTRECT, FALSE, (LPARAM)&rcTabDialog);

	for (size_t n = 0; n < tabsize(g_hWndTabDialog); n++) {
		if (g_hWndTabDialog[n] != nullptr) {
			::SetWindowPos(g_hWndTabDialog[n], nullptr,
				rcTabDialog.left, rcTabDialog.top,
				rcTabDialog.right - rcTabDialog.left, rcTabDialog.bottom - rcTabDialog.top,
				SWP_NOZORDER | SWP_NOACTIVATE);
		}
	}

	g_current_tab = CONF.last_conf_tab;
	uSendMessage(hWndTab, TCM_SETCURSEL, g_current_tab, 0);

	help_link.SubclassWindow(GetDlgItem(IDC_SYNTAX_HELP));
	pfc::string8 url(core_api::get_profile_path());
	url << "\\user-components\\foo_discogs\\foo_discogs_help.html";
	pfc::stringcvt::string_wide_from_utf8 wtext(url.get_ptr());
	help_link.SetHyperLink((LPCTSTR)const_cast<wchar_t*>(wtext.get_ptr()));

	modeless_dialog_manager::g_add(m_hWnd);
	g_hWndCurrentTab = g_hWndTabDialog[g_current_tab];	
	::ShowWindow(g_hWndCurrentTab, SW_SHOW);
	show();
	return TRUE;
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
	save_searching_dialog(g_hWndTabDialog[CONF_FIND_RELEASE_TAB]);
	save_caching_dialog(g_hWndTabDialog[CONF_CACHING_TAB]);
	save_matching_dialog(g_hWndTabDialog[CONF_MATCHING_TAB]);
	save_tagging_dialog(g_hWndTabDialog[CONF_TAGGING_TAB]);
	save_art_dialog(g_hWndTabDialog[CONF_ART_TAB]);
	save_oauth_dialog(g_hWndTabDialog[CONF_OATH_TAB]);
	CONF = conf;
	CONF.save();
	discogs_interface->fetcher->update_oauth(conf.oauth_token, conf.oauth_token_secret);
	destroy();
	return TRUE;
}

LRESULT CConfigurationDialog::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	destroy();
	return TRUE;
}

LRESULT CConfigurationDialog::OnDefaults(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (uMessageBox(m_hWnd, "Reset all settings to default?", "Reset",
		MB_OKCANCEL | MB_ICONQUESTION | MB_DEFBUTTON2) == IDOK) {
		foo_discogs_conf temp;
		temp.oauth_token = conf.oauth_token;
		temp.oauth_token_secret = conf.oauth_token_secret;
		conf = temp;
		init_searching_dialog(g_hWndTabDialog[CONF_FIND_RELEASE_TAB]);
		init_caching_dialog(g_hWndTabDialog[CONF_CACHING_TAB]);
		init_matching_dialog(g_hWndTabDialog[CONF_MATCHING_TAB]);
		init_tagging_dialog(g_hWndTabDialog[CONF_TAGGING_TAB]);
		init_art_dialog(g_hWndTabDialog[CONF_ART_TAB]);
		init_oauth_dialog(g_hWndTabDialog[CONF_OATH_TAB]);
	}
	return FALSE;
}

LRESULT CConfigurationDialog::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled) {
	CONF.last_conf_tab = g_current_tab;
	CONF.save();
	pfc::fill_array_t(g_hWndTabDialog, (HWND)nullptr);
	g_hWndCurrentTab = nullptr;
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
	original_parsing = conf.parse_hidden_as_regular;

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
	uButton_SetCheck(wnd, IDC_DISPLAY_ART_CHECK, conf.display_art);
	uButton_SetCheck(wnd, IDC_ALBUM_ART_OVERWRITE_CHECK, conf.album_art_overwrite);
	uButton_SetCheck(wnd, IDC_ALBUM_ART_FETCH_ALL_CHECK, conf.album_art_fetch_all);
	uSetDlgItemText(wnd, IDC_ALBUM_ART_DIR_EDIT, conf.album_art_directory_string);
	uSetDlgItemText(wnd, IDC_ALBUM_ART_PREFIX_EDIT, conf.album_art_filename_string);

	uButton_SetCheck(wnd, IDC_SAVE_ARTIST_ART_CHECK, conf.save_artist_art);
	uButton_SetCheck(wnd, IDC_ARTIST_ART_OVERWRITE_CHECK, conf.artist_art_overwrite);
	uButton_SetCheck(wnd, IDC_ARTIST_ART_FETCH_ALL_CHECK, conf.artist_art_fetch_all);
	uSetDlgItemText(wnd, IDC_ARTIST_ART_DIR_EDIT, conf.artist_art_directory_string);
	uSetDlgItemText(wnd, IDC_ARTIST_ART_PREFIX_EDIT, conf.artist_art_filename_string);
	uSetDlgItemText(wnd, IDC_ARTIST_ART_IDS_EDIT, conf.artist_art_id_format_string);
}

void CConfigurationDialog::init_oauth_dialog(HWND wnd) {
	token_edit = ::uGetDlgItem(wnd, IDC_OAUTH_TOKEN_EDIT);
	secret_edit = ::uGetDlgItem(wnd, IDC_OAUTH_SECRET_EDIT);

	uSetWindowText(token_edit, conf.oauth_token);
	uSetWindowText(secret_edit, conf.oauth_token_secret);
}

void CConfigurationDialog::save_tagging_dialog(HWND wnd) {
	conf.replace_ANVs = uButton_GetCheck(wnd, IDC_ENABLE_ANV_CHECK);
	conf.move_the_at_beginning = uButton_GetCheck(wnd, IDC_MOVE_THE_AT_BEGINNING_CHECK);
	conf.discard_numeric_suffix = uButton_GetCheck(wnd, IDC_DISCARD_NUMERIC_SUFFIXES_CHECK);

	conf.skip_preview_dialog = uButton_GetCheck(wnd, IDC_SKIP_PREVIEW_DIALOG);
	conf.remove_other_tags = uButton_GetCheck(wnd, IDC_REMOVE_OTHER_TAGS);
	pfc::string8 text;
	uGetDlgItemText(wnd, IDC_REMOVE_EXCLUDING_TAGS, text);
	conf.set_remove_exclude_tags(text);
}

void CConfigurationDialog::save_caching_dialog(HWND wnd) {
	conf.parse_hidden_as_regular = uButton_GetCheck(wnd, IDC_HIDDEN_AS_REGULAR_CHECK);
	if (original_parsing != conf.parse_hidden_as_regular) {
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
	conf.cache_max_objects = atol(str);
	discogs_interface->set_cache_size(conf.cache_max_objects);
}

void CConfigurationDialog::save_searching_dialog(HWND wnd) {
	conf.enable_autosearch = uButton_GetCheck(wnd, IDC_ENABLE_AUTO_SEARCH_CHECK);
	conf.skip_find_release_dialog_if_ided = uButton_GetCheck(wnd, IDC_SKIP_FIND_RELEASE_DIALOG_CHECK);
	pfc::string8 text;
	uGetDlgItemText(wnd, IDC_RELEASE_FORMATTING_EDIT, text);
	conf.search_release_format_string = text;
	uGetDlgItemText(wnd, IDC_MASTER_FORMATTING_EDIT, text);
	conf.search_master_format_string = text;
	uGetDlgItemText(wnd, IDC_MASTER_SUB_FORMATTING_EDIT, text);
	conf.search_master_sub_format_string = text;
}

void CConfigurationDialog::save_matching_dialog(HWND wnd) {
	conf.match_tracks_using_duration = uButton_GetCheck(wnd, IDC_MATCH_USING_DURATIONS);
	conf.match_tracks_using_number = uButton_GetCheck(wnd, IDC_MATCH_USING_NUMBERS);
	conf.assume_tracks_sorted = uButton_GetCheck(wnd, IDC_MATCH_ASSUME_SORTED);
	conf.skip_release_dialog_if_matched = uButton_GetCheck(wnd, IDC_SKIP_RELEASE_DIALOG_CHECK);
	
	pfc::string8 text;
	uGetDlgItemText(wnd, IDC_DISCOGS_FORMATTING_EDIT, text);
	conf.release_discogs_format_string = text;

	uGetDlgItemText(wnd, IDC_FILE_FORMATTING_EDIT, text);
	conf.release_file_format_string = text;
}

void CConfigurationDialog::save_art_dialog(HWND wnd) {
	pfc::string8 temp;

	conf.save_album_art = uButton_GetCheck(wnd, IDC_SAVE_ALBUM_ART_CHECK);
	conf.display_art = uButton_GetCheck(wnd, IDC_DISPLAY_ART_CHECK);
	uGetDlgItemText(wnd, IDC_ALBUM_ART_DIR_EDIT, temp);
	conf.album_art_directory_string = temp;
	uGetDlgItemText(wnd, IDC_ALBUM_ART_PREFIX_EDIT, temp);
	conf.album_art_filename_string = temp;
	conf.album_art_overwrite = uButton_GetCheck(wnd, IDC_ALBUM_ART_OVERWRITE_CHECK);
	conf.album_art_fetch_all = uButton_GetCheck(wnd, IDC_ALBUM_ART_FETCH_ALL_CHECK);

	conf.save_artist_art = uButton_GetCheck(wnd, IDC_SAVE_ARTIST_ART_CHECK);
	uGetDlgItemText(wnd, IDC_ARTIST_ART_DIR_EDIT, temp);
	conf.artist_art_directory_string = temp;
	uGetDlgItemText(wnd, IDC_ARTIST_ART_PREFIX_EDIT, temp);
	conf.artist_art_filename_string = temp;
	uGetDlgItemText(wnd, IDC_ARTIST_ART_IDS_EDIT, temp);
	conf.artist_art_id_format_string = temp;
	conf.artist_art_overwrite = uButton_GetCheck(wnd, IDC_ARTIST_ART_OVERWRITE_CHECK);
	conf.artist_art_fetch_all = uButton_GetCheck(wnd, IDC_ARTIST_ART_FETCH_ALL_CHECK);
}

void CConfigurationDialog::save_oauth_dialog(HWND wnd) {
	pfc::string8 text;
	uGetDlgItemText(wnd, IDC_OAUTH_TOKEN_EDIT, text);
	conf.oauth_token = text;

	uGetDlgItemText(wnd, IDC_OAUTH_SECRET_EDIT, text);
	conf.oauth_token_secret = text;
}

BOOL CConfigurationDialog::on_tagging_dialog_message(HWND wnd, UINT msg, WPARAM wp, LPARAM lp) {
	switch (msg) {
	case WM_INITDIALOG:
		init_tagging_dialog(wnd);
		return TRUE;
	case WM_COMMAND:
		if (LOWORD(wp) == IDC_EDIT_TAG_MAPPINGS_BUTTON) {
			if (!g_discogs->tag_mappings_dialog) {
				g_discogs->tag_mappings_dialog = new CNewTagMappingsDialog(core_api::get_main_window());
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
			init_caching_dialog(wnd);
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
			init_searching_dialog(wnd);
			return TRUE;
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
			init_matching_dialog(wnd);
			return TRUE;
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
			init_art_dialog(wnd);
			return TRUE;
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
			init_oauth_dialog(wnd);
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
			}
	}
	return FALSE;
}

void CConfigurationDialog::OnFinalMessage(HWND /*hWnd*/) {
	modeless_dialog_manager::g_remove(m_hWnd);
	delete this;
}

void CConfigurationDialog::on_test_oauth(HWND wnd) {
	pfc::string8 text;
	uGetDlgItemText(wnd, IDC_OAUTH_TOKEN_EDIT, text);
	pfc::string8 token = pfc::string8(text);

	uGetDlgItemText(wnd, IDC_OAUTH_SECRET_EDIT, text);
	pfc::string8 token_secret = pfc::string8(text);

	service_ptr_t<test_oauth_process_callback> task = new service_impl_t<test_oauth_process_callback>(token, token_secret);
	task->start(m_hWnd);
}

void CConfigurationDialog::on_authorize_oauth(HWND wnd) {
	service_ptr_t<authorize_oauth_process_callback> task = new service_impl_t<authorize_oauth_process_callback>();
	task->start(m_hWnd);
}

void CConfigurationDialog::on_generate_oauth(HWND wnd) {
	pfc::string8 code;
	uGetDlgItemText(wnd, IDC_OAUTH_PIN_EDIT, code);

	if (code.get_length() == 0) {
		uMessageBox(nullptr, "You must enter a valid PIN code.", "Error.", MB_OK);
		return;
	}

	service_ptr_t<generate_oauth_process_callback> task = new service_impl_t<generate_oauth_process_callback>(code);
	task->start(m_hWnd);
}
