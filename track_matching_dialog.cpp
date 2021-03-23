#include "stdafx.h"
#include "resource.h"
#include "track_matching_dialog.h"
#include "preview_dialog.h"
#include "tasks.h"
#include "utils.h"
#include "multiformat.h"

#include "track_matching_lv_helpers.h"

#define ENCODE_DISCOGS(d,t)		((d==-1||t==-1) ? -1 : ((d<<16)|t))
#define DECODE_DISCOGS_DISK(i)	((i==-1) ? -1 : (i>>16))
#define DECODE_DISCOGS_TRACK(i)	((i==-1) ? -1 : (i & 0xFFFF))
#define DISABLED_RGB	RGB(150, 150, 150)
#define CHANGE_NOT_APPROVED_RGB	RGB(150, 100, 100)

namespace listview_helper {

	unsigned insert_column(HWND p_listview, unsigned p_index, const char * p_name, unsigned p_width_dlu, int fmt)
	{
		pfc::stringcvt::string_os_from_utf8 os_string_temp(p_name);
		RECT rect = { 0, 0, static_cast<LONG>(p_width_dlu), 0 };
		MapDialogRect(GetParent(p_listview), &rect);
		LVCOLUMN data = {};
		data.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
		data.fmt = fmt;
		data.cx = rect.right;
		data.pszText = const_cast<TCHAR*>(os_string_temp.get_ptr());

		LRESULT ret = uSendMessage(p_listview, LVM_INSERTCOLUMN, p_index, (LPARAM)&data);
		if (ret < 0) return ~0;
		else return (unsigned)ret;
	}
}

LRESULT CTrackMatchingDialog::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	conf = CONF;

	pfc::string8 match_msg;

	if (multi_mode) {
		::ShowWindow(uGetDlgItem(IDC_WRITE_TAGS_BUTTON), false);
		::ShowWindow(uGetDlgItem(IDC_PREVIEW_TAGS_BUTTON), false);
		::ShowWindow(uGetDlgItem(IDC_PREVIOUS_BUTTON), true);
		::ShowWindow(uGetDlgItem(IDC_NEXT_BUTTON), true);
		::ShowWindow(uGetDlgItem(IDC_SKIP_BUTTON), true);
		::ShowWindow(uGetDlgItem(IDCANCEL), false);
		::ShowWindow(uGetDlgItem(IDC_BACK_BUTTON), false);
	}
	else {
		::ShowWindow(uGetDlgItem(IDC_WRITE_TAGS_BUTTON), true);
		::ShowWindow(uGetDlgItem(IDC_PREVIEW_TAGS_BUTTON), true);
		::ShowWindow(uGetDlgItem(IDC_PREVIOUS_BUTTON), false);
		::ShowWindow(uGetDlgItem(IDC_NEXT_BUTTON), false);
		::ShowWindow(uGetDlgItem(IDC_SKIP_BUTTON), false);
		::ShowWindow(uGetDlgItem(IDCANCEL), true);
		::ShowWindow(uGetDlgItem(IDC_BACK_BUTTON), true);

		//default button
		UINT default = conf.skip_preview_dialog ? IDC_WRITE_TAGS_BUTTON
			: IDC_PREVIEW_TAGS_BUTTON;
		uSendMessage(m_hWnd, WM_NEXTDLGCTL, (WPARAM)(HWND)GetDlgItem(default), TRUE);
	}

	discogs_track_list = uGetDlgItem(IDC_DISCOGS_TRACK_LIST);
	file_list = uGetDlgItem(IDC_FILE_LIST);

	listview_helper::insert_column(discogs_track_list, 0, "Discogs", 0);
	listview_helper::insert_column(discogs_track_list, 1, "Length", 45, LVCFMT_RIGHT);
	listview_helper::insert_column(file_list, 0, "Files", 0);
	listview_helper::insert_column(file_list, 1, "Length", 45, LVCFMT_RIGHT);

	update_list_width(discogs_track_list, true);
	update_list_width(file_list, true);

	ListView_SetExtendedListViewStyle(discogs_track_list, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER/*| LVS_SHOWSELALWAYS*/);
	ListView_SetExtendedListViewStyle(file_list, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

	list_drop_handler.Initialize(m_hWnd, discogs_track_list, file_list);
	list_drop_handler.SetNotifier(stdf_change_notifier);

	DlgResize_Init(mygripp.grip, true);
	load_size();
	modeless_dialog_manager::g_add(m_hWnd);
	show();

	if (tag_writer) {
		initialize();
	}
	else {
		get_next_tag_writer();
	}
	return TRUE;
}

void CTrackMatchingDialog::match_message_update(pfc::string8 local_msg) {

	HWND ctrl_match_msg = uGetDlgItem(IDC_MATCH_TRACKS_MSG);
	pfc::string8 newmessage;

	int local_status;
	const int local_override = -100;
	if (local_msg.length() > 0)
		local_status = local_override;
	else
		local_status = tag_writer->match_status;

	switch (local_status) {
	case local_override:
		newmessage.set_string(match_manual);
		goto showwindow;
	case MATCH_FAIL:
		newmessage.set_string(match_failed);
		goto showwindow;
	case MATCH_ASSUME:
		newmessage.set_string(match_assumed);
		goto showwindow;
	case MATCH_SUCCESS:
		newmessage.set_string(match_success);
		goto showwindow;
	case MATCH_NA:
		newmessage.set_string("NA");
		goto showwindow;

	default:
		newmessage.set_string("UNKNOWN");
	showwindow:
		uSetDlgItemText(m_hWnd, IDC_MATCH_TRACKS_MSG, newmessage);
		::ShowWindow(ctrl_match_msg, newmessage.length() > 0);
	}
}

bool CTrackMatchingDialog::initialize() {

	match_message_update();

	insert_track_mappings();

	// TODO: assess this! (return value not used? does it work if skip release dialog checked?)
	if (!multi_mode && tag_writer->match_status == MATCH_SUCCESS && conf.skip_release_dialog_if_matched) {
		generate_track_mappings(tag_writer->track_mappings);
		service_ptr_t<generate_tags_task> task = new service_impl_t<generate_tags_task>(this, tag_writer, false, use_update_tags);
		task->start();
		return FALSE;
	}
	return TRUE;
}

void CTrackMatchingDialog::insert_track_mappings() {
	ListView_DeleteAllItems(discogs_track_list);
	ListView_DeleteAllItems(file_list);

	hook.set_custom("DISPLAY_ANVS", conf.display_ANVs);
	hook.set_custom("REPLACE_ANVS", conf.replace_ANVs);

	const size_t count = tag_writer->track_mappings.get_count();
	titleformat_object::ptr lenght_script;
	static_api_ptr_t<titleformat_compiler>()->compile_safe_ex(lenght_script, "%length%");
	for (size_t i = 0; i < count; i++) {
		const track_mapping & mapping = tag_writer->track_mappings[i];
    	if (i < tag_writer->finfo_manager->items.get_count()) {
			file_info &finfo = tag_writer->finfo_manager->get_item(mapping.file_index);
			auto item = tag_writer->finfo_manager->items.get_item(mapping.file_index);
			pfc::string8 formatted_name;
			conf.release_file_format_string->run_simple(item->get_location(), &finfo, formatted_name);
			pfc::string8 display = pfc::string_filename_ext(formatted_name);
			pfc::string8 formatted_length;
			item->format_title(nullptr, formatted_length, lenght_script, nullptr);
			listview_helper::insert_item2(file_list, i, display, formatted_length,(LPARAM)mapping.file_index);
		}
		if (mapping.discogs_disc == -1 && mapping.discogs_track == -1) {
			//TODO: further testing on more files than discogs tracks
		}
		else {
			const ReleaseDisc_ptr disc = tag_writer->release->discs[mapping.discogs_disc];
			const ReleaseTrack_ptr track = disc->tracks[mapping.discogs_track];
			hook.set_release(&tag_writer->release);
			hook.set_disc(&disc);
			hook.set_track(&track);

			if (i == 0) {
				pfc::string8 compact_release;
				CONF.search_master_sub_format_string->run_hook(location, &info, &hook, compact_release, nullptr);
				int l = compact_release.get_length();
				uSetDlgItemText(m_hWnd, IDC_STATIC_MATCH_TRACKING_REL_NAME, ltrim(compact_release));
			}

			pfc::string8 text;
			conf.release_discogs_format_string->run_hook(location, &info, &hook, text, nullptr);
			pfc::string8 time;
			if (track->discogs_hidden_duration_seconds) {
				int duration_seconds = track->discogs_duration_seconds + track->discogs_hidden_duration_seconds;
				time = duration_to_str(duration_seconds);
			}
			else {
				time = track->discogs_duration_raw;
			}
			listview_helper::insert_item2(discogs_track_list, i, text, time, ENCODE_DISCOGS(mapping.discogs_disc, mapping.discogs_track));
		}
	}
	update_list_width(discogs_track_list);
	update_list_width(file_list);
}

LRESULT CTrackMatchingDialog::OnColorStatic(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	if ((HWND)lParam == uGetDlgItem(IDC_MATCH_TRACKS_MSG)) {

		pfc::string8 match_msg;
		uGetDlgItemText(m_hWnd, IDC_MATCH_TRACKS_MSG, match_msg);

		if (!stricmp_utf8(match_msg, match_failed) 
			|| !stricmp_utf8(match_msg, match_assumed)
			|| !stricmp_utf8(match_msg, match_manual)) {

			SetBkMode((HDC)wParam, TRANSPARENT);
			SetTextColor((HDC)wParam, RGB(255, 0, 0));
		}
	}
	/*else if ((HWND)lParam == match_success) {
		SetBkMode((HDC)wParam, TRANSPARENT);
		SetTextColor((HDC)wParam, RGB(0, 200, 0));
	}*/
	else {
		return FALSE;
	}
	return (BOOL)GetSysColorBrush(COLOR_MENU);
}

CTrackMatchingDialog::~CTrackMatchingDialog() {
	g_discogs->track_matching_dialog = nullptr;
}


LRESULT CTrackMatchingDialog::OnMoveTrackUp(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	move_selected_items_up(discogs_track_list);
	match_message_update(match_manual);
	return FALSE;
}

LRESULT CTrackMatchingDialog::OnMoveTrackDown(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	match_message_update(match_manual);
	move_selected_items_down(discogs_track_list);
	return FALSE;
}

LRESULT CTrackMatchingDialog::OnRemoveTrackButton(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	match_message_update(match_manual);
	if (wID == IDC_REMOVE_DISCOGS_TRACK_BUTTON)
		remove_items(discogs_track_list, false);
	else
		remove_items(file_list, false);
	return FALSE;
}

void CTrackMatchingDialog::generate_track_mappings(track_mappings_list_type &track_mappings) {
	track_mappings.force_reset();
	const size_t count_d = ListView_GetItemCount(discogs_track_list);
	const size_t count_f = ListView_GetItemCount(file_list);
	int dindex; int findex;
	for (size_t i = 0; i < count_d; i++) {
		findex = -1;
		track_mapping track;
		LVITEM lvi_d;
		ZeroMemory(&lvi_d, sizeof(lvi_d));
		lvi_d.mask = LVIF_PARAM;
		lvi_d.iItem = i;
		lvi_d.iSubItem = 0;
		ListView_GetItem(discogs_track_list, &lvi_d);
		dindex = lvi_d.lParam;
		if (i < count_f) {
			LVITEM lvi_f;
			ZeroMemory(&lvi_f, sizeof(lvi_f));
			lvi_f.mask = LVIF_PARAM;
			lvi_f.iItem = i;
			lvi_f.iSubItem = 0;
			ListView_GetItem(file_list, &lvi_f);
			findex = lvi_f.lParam;
		}
		track.enabled = (findex != -1 && dindex != -1);
		track.discogs_disc = DECODE_DISCOGS_DISK(dindex);
		track.discogs_track = DECODE_DISCOGS_TRACK(dindex);
		track.file_index = findex;
		track_mappings.append_single(track);
	}
}

bool CTrackMatchingDialog::init_count() {
	const bool die = !conf.update_tags_manually_match;
	for (size_t i = 0; i < tag_writers.get_count(); i++) {
		if (tag_writers[i]->skip || tag_writers[i]->match_status == MATCH_SUCCESS || tag_writers[i]->match_status == MATCH_ASSUME) {
			continue;
		}
		if (die) {
			tag_writers[i]->skip = true;
		}
		else {
			multi_count++;
		}
	}
	return multi_count != 0;
}

bool CTrackMatchingDialog::get_next_tag_writer() {
	while (tw_index < tag_writers.get_count()) {
		tag_writer = tag_writers[tw_index++];
		if (tag_writer->force_skip || tag_writer->match_status == MATCH_SUCCESS || tag_writer->match_status == MATCH_ASSUME) {
			tw_skip++;
			continue;
		}
		update_window_title();
		initialize();
		return true;
	}
	finished_tag_writers();
	destroy();
	return false;
}

bool CTrackMatchingDialog::get_previous_tag_writer() {
	size_t previous_index = tw_index-1;
	while (previous_index > 0) {
		previous_index--;
		auto tw = tag_writers[previous_index];
		if (!(tw->force_skip || tw->match_status == MATCH_SUCCESS || tw->match_status == MATCH_ASSUME)) {
			tw_index = previous_index;
			tag_writer = tag_writers[tw_index++];
			update_window_title();
			initialize();
			return true;
		}
		tw_skip--;
	}
	return false;
}

void CTrackMatchingDialog::finished_tag_writers() {
	service_ptr_t<generate_tags_task_multi> task = new service_impl_t<generate_tags_task_multi>(tag_writers, conf.update_tags_preview_changes, use_update_tags);
	task->start();
}

LRESULT CTrackMatchingDialog::OnBack(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	go_back();
	return TRUE;
}

LRESULT CTrackMatchingDialog::OnPreviewTags(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	PFC_ASSERT(!multi_mode);
	generate_track_mappings(tag_writer->track_mappings);
	service_ptr_t<generate_tags_task> task = new service_impl_t<generate_tags_task>(this, tag_writer, true, use_update_tags);
	task->start();
	return TRUE;
}

LRESULT CTrackMatchingDialog::OnWriteTags(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	PFC_ASSERT(!multi_mode);
	generate_track_mappings(tag_writer->track_mappings);
	service_ptr_t<generate_tags_task> task = new service_impl_t<generate_tags_task>(this, tag_writer, false, use_update_tags);
	task->start();
	return TRUE;
}

LRESULT CTrackMatchingDialog::OnMultiNext(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	PFC_ASSERT(multi_mode);
	generate_track_mappings(tag_writer->track_mappings);
	get_next_tag_writer();
	return TRUE;
}

LRESULT CTrackMatchingDialog::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (multi_mode) {
		destroy();
	}
	else {
		destroy_all();
	}
	return TRUE;
}


void CTrackMatchingDialog::pushcfg() {
	bool conf_changed = build_current_cfg();
	if (conf_changed) {
		CONF.save(new_conf::ConfFilter::CONF_FILTER_TRACK, conf);
		CONF.load();
	}
}

inline bool CTrackMatchingDialog::build_current_cfg() {
	bool bres = false;

	//check global settings

	//get current ui dimensions

	CRect rcDlg;
	GetClientRect(&rcDlg);

	int dlgwidth = rcDlg.Width();
	int dlgheight = rcDlg.Height();

	//dlg size

	if (dlgwidth != conf.release_dialog_width || dlgheight != conf.release_dialog_height) {
		conf.release_dialog_width = dlgwidth;
		conf.release_dialog_height = dlgheight;
		bres = bres || true;
	}

	return bres;
}

inline void CTrackMatchingDialog::load_size() {

	int width = conf.release_dialog_width;
	int height = conf.release_dialog_height;
	CRect rcCfg(0, 0, width, height);
	::CenterWindow(this->m_hWnd, rcCfg, core_api::get_main_window());
	
}

LRESULT CTrackMatchingDialog::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	pushcfg();
	return 0;
}

LRESULT CTrackMatchingDialog::OnMultiSkip(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	PFC_ASSERT(multi_mode);
	tag_writer->skip = true;
	get_next_tag_writer();
	return TRUE;
}

LRESULT CTrackMatchingDialog::OnMultiPrev(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	PFC_ASSERT(multi_mode);
	get_previous_tag_writer();
	return TRUE;
}

void CTrackMatchingDialog::OnFinalMessage(HWND /*hWnd*/) {
	modeless_dialog_manager::g_remove(m_hWnd);
	delete this;
}

void CTrackMatchingDialog::update_list_width(HWND list, bool initialize) {
	CRect client_rectangle;
	::GetClientRect(list, &client_rectangle);
	int width = client_rectangle.Width();

	int c1, c2;
	if (initialize) {
		c2 = 45;
	}
	else {
		c2 = ListView_GetColumnWidth(list, 1);
	}
	c1 = width - c2;
	ListView_SetColumnWidth(list, 0, c1);
	ListView_SetColumnWidth(list, 1, c2);
}

LRESULT CTrackMatchingDialog::list_key_down(HWND wnd, LPNMHDR lParam) {
	HWND wndList = wnd;
	NMLVKEYDOWN* info = reinterpret_cast<NMLVKEYDOWN*>(lParam);

	bool shift = (GetKeyState(VK_SHIFT) & 0x8000) == 0x8000;
	bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000);

	switch (info->wVKey) {
	case VK_DELETE:

		remove_items(wndList, false);
		match_message_update(match_manual);
		return TRUE;

	case VK_DOWN:
		switch (GetHotkeyModifierFlags()) {
		case MOD_CONTROL:

			move_selected_items_down(wndList);
			match_message_update(match_manual);
			return TRUE;
		default:
			return FALSE;
		}

	case VK_UP:
		switch (GetHotkeyModifierFlags()) {
		case MOD_CONTROL:

			move_selected_items_up(wndList);
			match_message_update(match_manual);
			return TRUE;
		default:
			return FALSE;
		}

	case 0x41: //'a' select all - invert selection
		if (shift && ctrl) {
			invert_selected_items(wndList);
			return TRUE;
		}
		else if (ctrl) {
			select_all_items(wndList);
			return TRUE;
		}
		else
			return FALSE;

	case 0x43: //'c' crop
		if (shift && ctrl) {
			;
			return TRUE;
		}
		else if (ctrl) {

			remove_items(wndList, true);
			match_message_update(match_manual);
			return TRUE;
		}
		else
			return FALSE;
	}
	return FALSE;
}
LRESULT CTrackMatchingDialog::OnListKeyDown(LPNMHDR lParam) {
	HWND wnd = ((LPNMHDR)lParam)->hwndFrom;
	list_key_down(wnd, lParam);
	return 0;
}

bool CTrackMatchingDialog::track_context_menu(HWND wnd, LPARAM coords) {
	POINT* pf = (POINT*)coords;
	if ((LPARAM)pf == (LPARAM)(-1)) {
		CRect rect;
		CWindow(wnd).GetWindowRect(&rect);
		pf = &rect.CenterPoint();
	}
	
	try {
		int coords_x = pf->x, coords_y = pf->y;
		enum { ID_SELECT_ALL = 1, ID_INVERT_SELECTION, ID_REMOVE, ID_CROP };
		HMENU menu = CreatePopupMenu();
		bool bitems = (ListView_GetItemCount(wnd) > 0);
		bool bselection = (ListView_GetSelectedCount(wnd) > 0);
		uAppendMenu(menu, MF_STRING | (bitems? 0 : MF_DISABLED), ID_SELECT_ALL, "Select all\tCtrl+A");
		uAppendMenu(menu, MF_STRING | (bselection? 0 : MF_DISABLED), ID_INVERT_SELECTION, "Invert selection\tCtrl+Shift+A");
		uAppendMenu(menu, MF_STRING | (bselection? 0 : MF_DISABLED), ID_REMOVE, "Remove\tDel");
		uAppendMenu(menu, MF_STRING | (bselection? 0 : MF_DISABLED), ID_CROP, "Crop\tCtrl+C");
		
		int cmd = TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, coords_x, coords_y, 0, wnd, 0);
		DestroyMenu(menu);
		switch (cmd)
		{
			case ID_SELECT_ALL:
			{
				select_all_items(wnd);
				return true;
			}
			case ID_INVERT_SELECTION:
			{
				invert_selected_items(wnd);
				return true;
			}
			case ID_REMOVE:
			{

				remove_items(wnd, false);
				match_message_update(match_manual);

				return true;
			}
			case ID_CROP:
			{

				remove_items(wnd, true);
				match_message_update(match_manual);
				return true; // found;
			}
		}
	}
	catch (...) {}
	return false;
}

LRESULT CTrackMatchingDialog::OnListRClick(LPNMHDR lParam) {
	HWND wnd = ((LPNMHDR)lParam)->hwndFrom;
	NMITEMACTIVATE* info = reinterpret_cast<NMITEMACTIVATE*>(lParam);
	POINT coords; GetCursorPos(&coords);
	track_context_menu(wnd, (LPARAM)&coords);
	return FALSE;
}

void CTrackMatchingDialog::enable(bool is_enabled) {
	::uEnableWindow(GetDlgItem(IDC_WRITE_TAGS_BUTTON), is_enabled);
	::uEnableWindow(GetDlgItem(IDCANCEL), is_enabled);
	::uEnableWindow(GetDlgItem(IDC_BACK_BUTTON), is_enabled);
	::uEnableWindow(GetDlgItem(IDC_EDIT_TAG_MAPPINGS_BUTTON), is_enabled);
	::uEnableWindow(GetDlgItem(IDC_REPLACE_ANV_CHECK), is_enabled);
	::uEnableWindow(GetDlgItem(IDC_REMOVE_DISCOGS_TRACK_BUTTON), is_enabled);
	::uEnableWindow(GetDlgItem(IDC_REMOVE_FILE_TRACK_BUTTON), is_enabled);
	::uEnableWindow(GetDlgItem(IDC_MOVE_UP_BUTTON), is_enabled);
	::uEnableWindow(GetDlgItem(IDC_MOVE_DOWN_BUTTON), is_enabled);
	::uEnableWindow(GetDlgItem(IDC_PREVIEW_TAGS_BUTTON), is_enabled);
}

void CTrackMatchingDialog::destroy_all() {
	if (!multi_mode) {
		g_discogs->find_release_dialog->destroy();
	}
	MyCDialogImpl<CTrackMatchingDialog>::destroy();
}

pfc::string8 file_info_get_artist_name(file_info_impl finfo, metadb_handle_ptr item) {
	pfc::string8 artist;
	g_discogs->file_info_get_tag(item, finfo, "Artist", artist);
	if (!artist.get_length())
		g_discogs->file_info_get_tag(item, finfo, "Album Artist", artist);
	else if (!artist.get_length())
		g_discogs->file_info_get_tag(item, finfo, "DISCOGS_ARTISTS", artist);
	else if (!artist.get_length())
		g_discogs->file_info_get_tag(item, finfo, "DISCOGS_ALBUM_ARTISTS", artist);
	return artist;
}

void CTrackMatchingDialog::go_back() {
	PFC_ASSERT(!multi_mode);
	
	pfc::string8 release_id, artist_id,	artist;
	metadb_handle_list items = tag_writer->finfo_manager->items;

	if (tag_writer->finfo_manager->items.get_size()) {
		file_info_impl finfo;
		metadb_handle_ptr item = items[0];
		item->get_info(finfo);
		g_discogs->file_info_get_tag(item, finfo, TAG_RELEASE_ID, release_id);
		g_discogs->file_info_get_tag(item, finfo, TAG_ARTIST_ID, artist_id);
		if (!artist_id.get_length()) {
			artist.set_string(file_info_get_artist_name(finfo, item));
		}
	}

	destroy();
	g_discogs->find_release_dialog->show();

	HWND wndFindRelease = g_discogs->find_release_dialog->m_hWnd;
	UINT default = IDC_PROCESS_RELEASE_BUTTON;
	
	if (release_id.get_length()) {
		
		g_discogs->find_release_dialog->setitems(items);
		HWND release_url_edit = ::GetDlgItem(wndFindRelease, IDC_RELEASE_URL_TEXT);
		uSetWindowText(release_url_edit,release_id.get_ptr());

		if (!artist_id.get_length()) {
			default = IDC_SEARCH_BUTTON;
			HWND search_edit = ::GetDlgItem(wndFindRelease, IDC_SEARCH_TEXT);
			uSetWindowText(search_edit, artist.get_ptr());
		}
	}

	uSendMessage(wndFindRelease, WM_NEXTDLGCTL, (WPARAM)(HWND)::GetDlgItem(wndFindRelease, default), TRUE);
}
