#include "stdafx.h"

#include "find_release_dialog.h"
#include "configuration_dialog.h"
#include "tasks.h"
#include "multiformat.h"
#include "sdk_helpers.h"

#include "resource.h"
#include "utils.h"


inline void CFindReleaseDialog::load_size() {
	int x = conf.find_release_dialog_width;
	int y = conf.find_release_dialog_height;
	if (x != 0 && y != 0) {
		SetWindowPos(nullptr, 0, 0, x, y, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

		CRect rectClient;
		GetClientRect(&rectClient);
		DlgResize_UpdateLayout(rectClient.Width(), rectClient.Height());
	}
}

inline void CFindReleaseDialog::save_size(int x, int y) {
	conf.find_release_dialog_width = x;
	conf.find_release_dialog_height = y;
	conf_changed = true;
}

CFindReleaseDialog::~CFindReleaseDialog() {
	if (conf_changed) {
		CONF.save(new_conf::ConfFilter::CONF_FILTER_FIND, conf);
		CONF.load();
	}
	g_discogs->find_release_dialog = nullptr;
}

LRESULT CFindReleaseDialog::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	
	conf = CONF;

	filter_edit = GetDlgItem(IDC_FILTER_EDIT);
	search_edit = GetDlgItem(IDC_SEARCH_TEXT);
	artist_list = GetDlgItem(IDC_ARTIST_LIST);
	release_list = GetDlgItem(IDC_RELEASE_LIST);
	//uSendMessage(release_list, LB_SETHORIZONTALEXTENT, 600, 0);
	release_url_edit = GetDlgItem(IDC_RELEASE_URL_TEXT);
	
	::SetFocus(search_edit);

	CheckDlgButton(IDC_ONLY_EXACT_MATCHES_CHECK, conf.display_exact_matches);

	pfc::string8 release_id;

	file_info_impl finfo;
	metadb_handle_ptr item = items[0];
	item->get_info(finfo);

	bool release_id_unknown = true;
	if (g_discogs->file_info_get_tag(item, finfo, TAG_RELEASE_ID, release_id)) {
		uSetWindowText(release_url_edit, release_id.get_ptr());
		release_id_unknown = false;
	}

	// TODO: make formatting strings out of these
	const char *artist = finfo.meta_get("ARTIST", 0);
	if (!artist) {
		artist = finfo.meta_get("ALBUM ARTIST", 0);
	}
	if (artist) {
		uSetWindowText(search_edit, artist);
		if (release_id_unknown && conf.enable_autosearch && artist[0] != '\0') {
			search_artist();
		}
	}
	else {
		uSetWindowText(search_edit, "");
	}
	uSendMessage(m_hWnd, DM_SETDEFID, (WPARAM)release_id_unknown ? IDC_SEARCH_BUTTON : IDC_PROCESS_RELEASE_BUTTON, 0);

	const char *album = finfo.meta_get("ALBUM", 0);
	if (album && strcmp(album, "") != 0) {
		uSetWindowText(filter_edit, album);
	}

	DlgResize_Init(true, true);

	load_size();
	modeless_dialog_manager::g_add(m_hWnd);
	show();

	if (!g_discogs->gave_oauth_warning && (!conf.oauth_token.get_length() || !conf.oauth_token_secret.get_length())) {
		g_discogs->gave_oauth_warning = true;
		if (!g_discogs->configuration_dialog) {	
			static_api_ptr_t<ui_control>()->show_preferences(guid_pref_page);
		}
		else {
			::SetFocus(g_discogs->configuration_dialog->m_hWnd);
		}
		g_discogs->configuration_dialog->show_tab(CONF_OATH_TAB);

		uMessageBox(m_hWnd, "OAuth is required to use the\nDiscogs API. Please configure OAuth.", "Error.", MB_OK);
	}
	else if (!release_id_unknown && conf.skip_find_release_dialog_if_ided) {
		on_write_tags(release_id);
	}
	return FALSE;
}

void CFindReleaseDialog::fill_artist_list() {
	uSendMessage(artist_list, LB_RESETCONTENT, 0, 0);
	uSendMessage(release_list, LB_RESETCONTENT, 0, 0);

	find_release_artists = artist_exact_matches;

	bool exact_matches = IsDlgButtonChecked(IDC_ONLY_EXACT_MATCHES_CHECK) != 0;

	if (!exact_matches) {
		find_release_artists.append(artist_other_matches);
	}

	int artists_index = 0, list_index = 0;
	for (size_t i = 0; i < find_release_artists.get_size(); i++) {
		pfc::string8 name;
		name << find_release_artists[i]->name;
		uSendMessageText(artist_list, LB_ADDSTRING, 0, name.get_ptr());
		uSendMessage(artist_list, LB_SETITEMDATA, list_index, (LPARAM)artists_index);
		artists_index++;
		list_index++;
	}
	if (find_release_artists.get_size()) {
		uSendMessage(artist_list, LB_SETCURSEL, 0, 0);
		get_selected_artist_releases();
	}
}

void CFindReleaseDialog::on_get_artist_done(Artist_ptr &artist) {
	find_release_artist = artist;
	hook = std::make_shared<titleformat_hook_impl_multiformat>(&find_release_artist);
	pfc::string8 text;
	uGetWindowText(filter_edit, text);
	filter_releases(text);
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
	fill_artist_list();
}

void CFindReleaseDialog::insert_item(const pfc::string8 &item, int list_index, int item_data) {
	uSendMessageText(release_list, LB_ADDSTRING, 0, item.get_ptr());
	uSendMessage(release_list, LB_SETITEMDATA, list_index, (LPARAM)item_data);
}

void CFindReleaseDialog::filter_releases(const pfc::string8 &filter) {
	uSendMessage(release_list, LB_RESETCONTENT, 0, 0);

	if (!find_release_artist) {
		return;
	}
	
	int list_index = 0;
	int master_index = 0, release_index = 0;
	int item_data;
	pfc::array_t<pfc::string8> filter_words;
	tokenize(filter, " ", filter_words, true);
	pfc::array_t<pfc::string> filter_words_lowercase;
	for (size_t i = 0; i < filter_words.get_size(); i++) {
		filter_words_lowercase.append_single(pfc::string(filter_words[i].get_ptr()).toLower());
	}

	// TODO: cache all these to make filtering fast again
	try {
		for (size_t i = 0; i < find_release_artist->search_order_master.get_size(); i++) {
			bool is_master = find_release_artist->search_order_master[i];
			pfc::string8 item;

			if (is_master) {
				hook->set_master(&(find_release_artist->master_releases[master_index]));
				conf.search_master_format_string->run_hook(location, &info, hook.get(), item, nullptr);
				item_data = (master_index << 16) | 9999;
			}
			else {
				hook->set_release(&(find_release_artist->releases[release_index]));
				conf.search_release_format_string->run_hook(location, &info, hook.get(), item, nullptr);
				item_data = (9999 << 16) | release_index;
			}

			bool inserted = false;

			bool matches = true;
			if (filter.get_length()) {
				pfc::string item_lower = pfc::string(item.get_ptr()).toLower();
				for (size_t j = 0; j < filter_words_lowercase.get_count(); j++) {
					matches = item_lower.contains(filter_words_lowercase[j]);
					if (!matches) {
						break;
					}
				}
			}
			if (matches) {
				insert_item(item, list_index, item_data);
				list_index++;
				inserted = true;
			}
			if (is_master) {
				pfc::string8 sub_item;
				for (size_t j = 0; j < find_release_artist->master_releases[master_index]->sub_releases.get_size(); j++) {
					hook->set_release(&(find_release_artist->master_releases[master_index]->sub_releases[j]));
					conf.search_master_sub_format_string->run_hook(location, &info, hook.get(), sub_item, nullptr);
					
					bool matches = true;
					if (filter.get_length()) {
						pfc::string sub_item_lower = pfc::string(sub_item.get_ptr()).toLower();
						for (size_t k = 0; k < filter_words_lowercase.get_count(); k++) {
							matches = sub_item_lower.contains(filter_words_lowercase[k]);
							if (!matches) {
								break;
							}
						}
					}
					if (matches) {
						if (!inserted) {
							insert_item(item, list_index, item_data);
							list_index++;
							inserted = true;
						}
						item_data = (master_index << 16) | j;
						insert_item(sub_item, list_index, item_data);
						list_index++;
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
	catch (foo_discogs_exception &e) {
		foo_discogs_exception ex;
		ex << "Error formating release list item [" << e.what() << "]";
		throw ex;
	}
	select_first_release();
}

void CFindReleaseDialog::select_first_release() {
	if (uSendMessage(release_list, LB_GETCOUNT, 0, 0) == 1) {
		uSendMessage(release_list, LB_SETCURSEL, 0, 0);
		on_release_selected(0);
	}
}

void CFindReleaseDialog::on_release_selected(int list_index) {
	if (this->active_task) {
		return;
	}
	int item_data = (int)uSendMessage(release_list, LB_GETITEMDATA, list_index, 0);
	int master_index = item_data >> 16;
	int release_index = item_data & 0xFFFF;
	pfc::string8 selected_id;
	if (master_index != 9999 && release_index == 9999) {
		expand_master_release(find_release_artist->master_releases[master_index], list_index);
		selected_id = find_release_artist->master_releases[master_index]->main_release_id;
	}
	else if (master_index != 9999) {
		selected_id = find_release_artist->master_releases[master_index]->sub_releases[release_index]->id;
	}
	else {
		selected_id = find_release_artist->releases[release_index]->id;
	}
	uSetWindowText(release_url_edit, selected_id.get_ptr());
	uSendMessage(m_hWnd, DM_SETDEFID, IDC_PROCESS_RELEASE_BUTTON, 0);
}

LRESULT CFindReleaseDialog::OnEditReleaseIdText(WORD wNotifyCode, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (wNotifyCode == EN_SETFOCUS || wNotifyCode == EN_CHANGE) {
		uSendMessage(m_hWnd, DM_SETDEFID, IDC_PROCESS_RELEASE_BUTTON, 0);
	}
	return FALSE;
}

LRESULT CFindReleaseDialog::OnEditSearchText(WORD wNotifyCode, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (wNotifyCode == EN_SETFOCUS || wNotifyCode == EN_CHANGE) {
		uSendMessage(m_hWnd, DM_SETDEFID, IDC_SEARCH_BUTTON, 0);
	}
	return FALSE;
}

LRESULT CFindReleaseDialog::OnEditFilterText(WORD wNotifyCode, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	pfc::string8 text;
	if (wNotifyCode == EN_CHANGE) {
		uGetWindowText(filter_edit, text);
		filter_releases(text);
	}
	return FALSE;
}

LRESULT CFindReleaseDialog::OnSelectArtist(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	get_selected_artist_releases();
	return FALSE;
}

LRESULT CFindReleaseDialog::OnDoubleClickArtist(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int pos = (int)uSendMessage(artist_list, LB_GETCURSEL, 0, 0);
	if (pos != -1) {
		pfc::string8 url;
		url << "https://www.discogs.com/artist/" << find_release_artists[pos]->id;
		display_url(url);
	}
	return FALSE;
}

LRESULT CFindReleaseDialog::OnSelectRelease(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int pos = (int)uSendMessage(release_list, LB_GETCURSEL, 0, 0);
	if (pos != -1) {
		on_release_selected(pos);
	}
	return FALSE;
}

LRESULT CFindReleaseDialog::OnDoubleClickRelease(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int pos = (int)uSendMessage(release_list, LB_GETCURSEL, 0, 0);
	if (pos != -1) {
		int item_data = (int)uSendMessage(release_list, LB_GETITEMDATA, pos, 0);
		int master_index = item_data >> 16;
		int release_index = item_data & 0xFFFF;
		pfc::string8 url; 
		if (master_index != 9999 && release_index == 9999) {
			url << "https://www.discogs.com/master/" << find_release_artist->master_releases[master_index]->id;
		}
		else if (master_index != 9999) {
			url << "https://www.discogs.com/release/" << find_release_artist->master_releases[master_index]->sub_releases[release_index]->id;
		}
		else {
			url << "https://www.discogs.com/release/" << find_release_artist->releases[release_index]->id;
		}
		display_url(url);
	}
	return FALSE;
}

LRESULT CFindReleaseDialog::OnCheckOnlyExactMatches(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	conf.display_exact_matches = IsDlgButtonChecked(IDC_ONLY_EXACT_MATCHES_CHECK) != 0;
	conf_changed = true; 
	fill_artist_list();
	return FALSE;
}

LRESULT CFindReleaseDialog::OnSearch(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	search_artist();
	return FALSE;
}

LRESULT CFindReleaseDialog::OnClearFilter(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	uSetWindowText(filter_edit, "");
	filter_releases("");
	return FALSE;
}

LRESULT CFindReleaseDialog::OnConfigure(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
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

void CFindReleaseDialog::on_expand_master_release_done(const MasterRelease_ptr &master_release, int list_index, threaded_process_status &p_status, abort_callback &p_abort) {
	int master_index = 0;
	for (size_t i = 0; i < find_release_artist->master_releases.get_size(); i++) {
		if (find_release_artist->master_releases[i]->id == master_release->id) {
			master_index = i;
		}
	}
	pfc::string8 text;
	uGetWindowText(filter_edit, text);
	filter_releases(text);
	/*
	list_index++;
	for (size_t i = 0; i < master_release->sub_releases.get_size(); i++) {
		pfc::string8 item;
		titleformat_hook_impl_multiformat hook(p_status, &master_release, &(master_release->sub_releases[i]));
		try {
			CONF.search_master_sub_format_string->run_hook(location, &info, &hook, item, nullptr);
		}
		catch (foo_discogs_exception &e) {
			foo_discogs_exception ex;
			ex << "Error formating release list item [" << e.what() << "]";
			throw ex;
		}
		
		uSendMessageText(release_list, LB_INSERTSTRING, list_index, item.get_ptr());
		int item_data = (master_index << 16) | i;
		uSendMessage(release_list, LB_SETITEMDATA, list_index, (LPARAM)item_data);
	}
	*/
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

void CFindReleaseDialog::get_selected_artist_releases() {
	int pos = (int)uSendMessage(artist_list, LB_GETCURSEL, 0, 0);
	if (pos != -1) {
		Artist_ptr &artist = find_release_artists[pos];
		service_ptr_t<get_artist_process_callback> task =
			new service_impl_t<get_artist_process_callback>(artist->id.get_ptr());
		task->start(m_hWnd);
	}
}

void CFindReleaseDialog::search_artist() {
	pfc::string8 search;
	uGetWindowText(search_edit, search);

	service_ptr_t<search_artist_process_callback> task =
		new service_impl_t<search_artist_process_callback>(trim(search).get_ptr());
	task->start(m_hWnd);
}
