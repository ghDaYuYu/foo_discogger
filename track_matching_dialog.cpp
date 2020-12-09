#include "stdafx.h"

#include "track_matching_dialog.h"
#include "preview_dialog.h"
#include "tasks.h"
#include "utils.h"
#include "multiformat.h"

#define ENCODE_DISCOGS(d,t)		((d==-1||t==-1) ? -1 : ((d<<16)|t))
#define DECODE_DISCOGS_DISK(i)	((i==-1) ? -1 : (i>>16))
#define DECODE_DISCOGS_TRACK(i)	((i==-1) ? -1 : (i & 0xFFFF))


namespace listview_helper {
	
	unsigned insert_column(HWND p_listview, unsigned p_index, const char * p_name, unsigned p_width_dlu, int fmt)
	{
		pfc::stringcvt::string_os_from_utf8 os_string_temp(p_name);

		RECT rect = { 0, 0, p_width_dlu, 0 };
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


inline void CTrackMatchingDialog::load_size() {
	int x = conf.release_dialog_width;
	int y = conf.release_dialog_height;
	if (x != 0 && y != 0) {
		SetWindowPos(nullptr, 0, 0, x, y, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

		CRect rectClient;
		GetClientRect(&rectClient);
		DlgResize_UpdateLayout(rectClient.Width(), rectClient.Height());
	}
}

inline void CTrackMatchingDialog::save_size(int x, int y) {
	conf.release_dialog_width = x;
	conf.release_dialog_height = y;
	conf_changed = true;
}

LRESULT CTrackMatchingDialog::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	conf = CONF;
	
	match_failed = uGetDlgItem(IDC_FAILED_TO_MATCH_TRACKS);
	match_assumed = uGetDlgItem(IDC_ASSUMED_MATCH_TRACKS);
	match_success = uGetDlgItem(IDC_MATCHED_TRACKS);

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
		SendMessage(m_hWnd, DM_SETDEFID, (WPARAM)conf.skip_preview_dialog ? IDC_WRITE_TAGS_BUTTON : IDC_PREVIEW_TAGS_BUTTON, 0);
	}

	discogs_track_list = uGetDlgItem(IDC_DISCOGS_TRACK_LIST);
	file_list = uGetDlgItem(IDC_FILE_LIST);

	listview_helper::insert_column(discogs_track_list, 0, "Discogs", 0);
	listview_helper::insert_column(discogs_track_list, 1, "Length", 45, LVCFMT_RIGHT);
	listview_helper::insert_column(file_list, 0, "Files", 0);
	listview_helper::insert_column(file_list, 1, "Length", 45, LVCFMT_RIGHT);

	update_list_width(discogs_track_list, true);
	update_list_width(file_list, true);

	ListView_SetExtendedListViewStyleEx(discogs_track_list, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
	ListView_SetExtendedListViewStyleEx(file_list, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

	DlgResize_Init(true, true);
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

bool CTrackMatchingDialog::initialize() {
	::ShowWindow(match_failed, false);
	::ShowWindow(match_assumed, false);
	::ShowWindow(match_success, false);

	if (tag_writer->match_status == MATCH_FAIL) {
		::ShowWindow(match_failed, true);
	}
	else if (tag_writer->match_status == MATCH_ASSUME) {
		::ShowWindow(match_assumed, true);
	}
	else {
		::ShowWindow(match_success, true);
	}
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

pfc::string8 duration_to_str(int seconds) {
	int hours = seconds / 3600;
	seconds %= 3600;
	int minutes = seconds / 60;
	seconds = seconds % 60;
	pfc::string8 result = "";
	if (hours) {
		result << hours << ":";
	}
	if (hours && minutes < 10) {
		result << "0";
	}
	result << minutes << ":";
	if (seconds < 10) {
		result << "0";
	}
	result << seconds;
	return result;
}

void CTrackMatchingDialog::insert_track_mappings() {
	ListView_DeleteAllItems(discogs_track_list);
	ListView_DeleteAllItems(file_list);

	hook.set_custom("DISPLAY_ANVS", conf.display_ANVs);
	hook.set_custom("REPLACE_ANVS", conf.replace_ANVs);

	const size_t count = tag_writer->track_mappings.get_count();
	for (size_t i = 0; i < count; i++) {
		const track_mapping & mapping = tag_writer->track_mappings[i];
		if (mapping.file_index == -1) {
			listview_helper::insert_item(file_list, i, "", -1);
		}
		else {
			file_info &finfo = tag_writer->finfo_manager->get_item(mapping.file_index);
			auto item = tag_writer->finfo_manager->items.get_item(mapping.file_index);
			pfc::string8 formatted_name;
			conf.release_file_format_string->run_simple(item->get_location(), &finfo, formatted_name);
			pfc::string8 display = pfc::string_filename_ext(formatted_name);
			const char *length_titleformat = "%length%";
			pfc::string8 formatted_length;
			item->format_title(nullptr, formatted_length, titleformat_object_wrapper(length_titleformat), nullptr);
			listview_helper::insert_item2(file_list, i, display, formatted_length, mapping.file_index);
		}
		if (mapping.discogs_disc == -1 && mapping.discogs_track == -1) {
			listview_helper::insert_item(discogs_track_list, i, "", -1);
		}
		else {
			const ReleaseDisc_ptr disc = tag_writer->release->discs[mapping.discogs_disc];
			const ReleaseTrack_ptr track = disc->tracks[mapping.discogs_track];
			hook.set_release(&tag_writer->release);
			hook.set_disc(&disc);
			hook.set_track(&track);
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
	if ((HWND)lParam == match_failed || (HWND)lParam == match_assumed) {
		SetBkMode((HDC)wParam, TRANSPARENT);
		SetTextColor((HDC)wParam, RGB(255, 0, 0));
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
	if (conf_changed) {
		CONF.save(new_conf::ConfFilter::CONF_FILTER_TRACK, conf);
		CONF.load();
	}
	g_discogs->track_matching_dialog = nullptr;
}

void CTrackMatchingDialog::list_swap_items(HWND list, unsigned int pos1, unsigned int pos2) {
	const unsigned LOCAL_BUFFER_SIZE = 4096;
	LVITEM lvi1, lvi2;
	UINT uMask = LVIF_TEXT | LVIF_IMAGE | LVIF_INDENT | LVIF_PARAM | LVIF_STATE;
	char buffer1[LOCAL_BUFFER_SIZE + 1];
	char buffer2[LOCAL_BUFFER_SIZE + 1];
	lvi1.pszText = (LPWSTR)buffer1;
	lvi2.pszText = (LPWSTR)buffer2;
	lvi1.cchTextMax = sizeof(buffer1);
	lvi2.cchTextMax = sizeof(buffer2);

	lvi1.mask = uMask;
	lvi1.stateMask = (UINT)-1;
	lvi1.iItem = pos1;
	lvi1.iSubItem = 0;
	BOOL result1 = ListView_GetItem(list, &lvi1);

	lvi2.mask = uMask;
	lvi2.stateMask = (UINT)-1;
	lvi2.iItem = pos2;
	lvi2.iSubItem = 0;
	BOOL result2 = ListView_GetItem(list, &lvi2);

	if (result1 && result2) {
		lvi1.iItem = pos2;
		lvi2.iItem = pos1;
		lvi1.mask = uMask;
		lvi2.mask = uMask;
		lvi1.stateMask = (UINT)-1;
		lvi2.stateMask = (UINT)-1;
		//swap the items
		ListView_SetItem(list, &lvi1);
		ListView_SetItem(list, &lvi2);

		int total_columns = Header_GetItemCount(ListView_GetHeader(list));
		// Loop for swapping each column in the items.
		for (int i = 1; i < total_columns; i++) {
			buffer1[0] = '\0';
			buffer2[0] = '\0';
			ListView_GetItemText(list, pos1, i, (LPWSTR)buffer1, LOCAL_BUFFER_SIZE);
			ListView_GetItemText(list, pos2, i, (LPWSTR)buffer2, LOCAL_BUFFER_SIZE);
			ListView_SetItemText(list, pos2, i, (LPWSTR)buffer1);
			ListView_SetItemText(list, pos1, i, (LPWSTR)buffer2);
		}
	}
}

LRESULT CTrackMatchingDialog::OnMoveTrackUp(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int direction;
	size_t limit;
	//size_t max_selected = max(release->get_total_track_count(), items.get_count());
	direction = -1;
	limit = 0;
	// TODO: support moving more than 1 at a time, arbitrarily
	int nselected = ListView_GetSelectedCount(discogs_track_list);
	if (nselected == 1) {
		int index = ListView_GetNextItem(discogs_track_list, -1, LVNI_SELECTED);
		if (index != -1 && index != limit) {
			list_swap_items(discogs_track_list, index, index + direction);
			ListView_SetItemState(discogs_track_list, index + direction, LVIS_SELECTED, LVIS_SELECTED);
		}
	}
	return FALSE;
}

LRESULT CTrackMatchingDialog::OnMoveTrackDown(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int direction;
	size_t limit;
	size_t max_selected = max(tag_writer->release->get_total_track_count(), tag_writer->finfo_manager->items.get_count());
	direction = 1;
	limit = max_selected - 1;
	// TODO: support moving more than 1 at a time, arbitrarily
	int nselected = ListView_GetSelectedCount(discogs_track_list);
	if (nselected == 1) {
		int index = ListView_GetNextItem(discogs_track_list, -1, LVNI_SELECTED);
		if (index != -1 && index != limit) {
			list_swap_items(discogs_track_list, index, index + direction);
			ListView_SetItemState(discogs_track_list, index + direction, LVIS_SELECTED, LVIS_SELECTED);
		}
	}
	return FALSE;
}

LRESULT CTrackMatchingDialog::OnRemoveDiscogsTrack(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	remove_selected_items(discogs_track_list);
	return FALSE;
}

LRESULT CTrackMatchingDialog::OnRemoveFileTrack(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	remove_selected_items(file_list);
	return FALSE;
}

void CTrackMatchingDialog::remove_selected_items(HWND list) {
	int index;
	int end = ListView_GetItemCount(list);
	while ((index = ListView_GetNextItem(list, -1, LVNI_SELECTED)) != -1) {
		ListView_DeleteItem(list, index);
		listview_helper::insert_item(list, end, "", -1);
	}
}

void CTrackMatchingDialog::move_selected_items_down(HWND list) {
	pfc::array_t<int> swap;
	int index = -1;
	while ((index = ListView_GetNextItem(list, index, LVNI_SELECTED)) != -1) {
		swap.append_single(index);
	}
	int end = ListView_GetItemCount(list);
	for (size_t i = swap.get_size() - 1; i >= 0; i--) {
		if (swap[i] == end) {
			end--;
		}
		else if (swap[i] < end) {
			list_swap_items(list, swap[i], swap[i] + 1);
		}
	}
}

void CTrackMatchingDialog::move_selected_items_up(HWND list) {
	pfc::array_t<int> swap;
	int index = -1;
	while ((index = ListView_GetNextItem(list, index, LVNI_SELECTED)) != -1) {
		swap.append_single(index);
	}
	int start = 0;
	for (size_t i = 0; i < swap.get_size(); i++) {
		if (swap[i] == start) {
			start++;
		}
		else if (swap[i] > start) {
			list_swap_items(list, swap[i], swap[i] - 1);
		}
	}
}

void CTrackMatchingDialog::generate_track_mappings(track_mappings_list_type &track_mappings) {
	track_mappings.force_reset();
	const size_t count = ListView_GetItemCount(discogs_track_list);
	for (size_t i = 0; i < count; i++) {
		track_mapping track;
		LVITEM lvi;
		lvi.mask = LVIF_PARAM;
		lvi.iItem = i;
		lvi.iSubItem = 0;
		ListView_GetItem(discogs_track_list, &lvi);
		int dindex = (int)lvi.lParam;
		ListView_GetItem(file_list, &lvi);
		int findex = (int)lvi.lParam;
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

LRESULT CTrackMatchingDialog::OnFileListKeyDown(LPNMHDR lParam) {
	NMLVKEYDOWN * info = reinterpret_cast<NMLVKEYDOWN*>(lParam);
	switch (info->wVKey) {
		case VK_DELETE:
			remove_selected_items(file_list);
			return TRUE;

		case VK_DOWN:
			switch (GetHotkeyModifierFlags()) {
				case MOD_CONTROL:
					move_selected_items_down(file_list);
					return TRUE;
				default:
					return FALSE;
			}

		case VK_UP:
			switch (GetHotkeyModifierFlags()) {
				case MOD_CONTROL:
					move_selected_items_up(file_list);
					return TRUE;
				default:
					return FALSE;
			}
	}
	return FALSE;
}

LRESULT CTrackMatchingDialog::OnDiscogsListKeyDown(LPNMHDR lParam) {
	NMLVKEYDOWN * info = reinterpret_cast<NMLVKEYDOWN*>(lParam);
	switch (info->wVKey) {
		case VK_DELETE:
			remove_selected_items(discogs_track_list);
			return TRUE;

		case VK_DOWN:
			switch (GetHotkeyModifierFlags()) {
				case MOD_CONTROL:
					move_selected_items_down(discogs_track_list);
					return TRUE;
				default:
					return FALSE;
			}

		case VK_UP:
			switch (GetHotkeyModifierFlags()) {
				case MOD_CONTROL:
					move_selected_items_up(discogs_track_list);
					return TRUE;
				default:
					return FALSE;
			}
	}
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

void CTrackMatchingDialog::go_back() {
	PFC_ASSERT(!multi_mode);
	destroy();
	g_discogs->find_release_dialog->show();
}
