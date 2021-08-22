#include "stdafx.h"
#include "resource.h"
#include <gdiplus.h>

#include "SDK\imageViewer.h"

#include "track_matching_dialog.h"
#include "preview_dialog.h"
#include "tasks.h"
#include "utils.h"
#include "multiformat.h"

#include <filesystem>
#include "track_matching_lv_helpers.h"
#include "track_matching_utils.h"

#define ENCODE_DISCOGS(d,t)		((d==-1||t==-1) ? -1 : ((d<<16)|t))
#define DECODE_DISCOGS_DISK(i)	((i==-1) ? -1 : (i>>16))
#define DECODE_DISCOGS_TRACK(i)	((i==-1) ? -1 : (i & 0xFFFF))
#define DISABLED_RGB	RGB(150, 150, 150)
#define CHANGE_NOT_APPROVED_RGB	RGB(150, 100, 100)

LRESULT CTrackMatchingDialog::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {

	DlgResize_Init(mygripp.grip, true);
	load_size();

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
		UINT default = m_conf.skip_preview_dialog ? IDC_WRITE_TAGS_BUTTON
			: IDC_PREVIEW_TAGS_BUTTON;
		uSendMessage(m_hWnd, WM_NEXTDLGCTL, (WPARAM)(HWND)GetDlgItem(default), TRUE);
	}

	HWND discogs_track_list = uGetDlgItem(IDC_DISCOGS_TRACK_LIST);
	HWND file_list = uGetDlgItem(IDC_FILE_LIST);

	list_drop_handler.Initialize(m_hWnd, discogs_track_list, file_list, &m_coord);
	list_drop_handler.SetNotifier(stdf_change_notifier);

	if (m_tag_writer) {
		CONFARTWORK = uartwork(m_conf);
		match_message_update();
		//libPPUI owner data list control
		m_idc_list.CreateInDialog(*this, IDC_UI_LIST_DISCOGS);
		m_idc_list.InitializeHeaderCtrl(HDS_DRAGDROP);

		m_ifile_list.CreateInDialog(*this, IDC_UI_LIST_FILES);
		m_ifile_list.InitializeHeaderCtrl(HDS_DRAGDROP);

		m_coord.Initialize(m_hWnd, &m_conf);
		m_coord.SetTagWriter(m_tag_writer);

		m_coord.InitFormMode(lsmode::tracks_ui, IDC_UI_LIST_DISCOGS, IDC_UI_LIST_FILES);
		m_coord.InitFormMode(lsmode::art, IDC_DISCOGS_TRACK_LIST, IDC_FILE_LIST);

		m_coord.SetMode(lsmode::tracks_ui);
		m_coord.Show();

		m_coord.InitUiList(m_idc_list.m_hWnd, lsmode::tracks_ui, true, &m_idc_list);
		m_coord.InitUiList(m_ifile_list.m_hWnd, lsmode::tracks_ui, false, &m_ifile_list);

		show();

		LibUIAsOwnerData(true);

		::ShowWindow(uGetDlgItem(IDC_DISCOGS_TRACK_LIST), SW_HIDE);
		::ShowWindow(uGetDlgItem(IDC_FILE_LIST), SW_HIDE);

		::ShowWindow(uGetDlgItem(IDC_UI_LIST_DISCOGS), SW_SHOW);
		::ShowWindow(uGetDlgItem(IDC_UI_LIST_FILES), SW_SHOW);

		ShowWindow(SW_SHOW);

		// TODO: assess this (return value not used? does it work if skip release dialog checked?)
		if (!multi_mode && m_tag_writer->match_status == MATCH_SUCCESS && m_conf.skip_release_dialog_if_matched) {
			generate_track_mappings(m_tag_writer->track_mappings);
			service_ptr_t<generate_tags_task> task = new service_impl_t<generate_tags_task>(this, m_tag_writer, false, use_update_tags);
			task->start();
			return FALSE;
		}

		//todo: expand limit of 30 images

		if (m_tag_writer->release->images.get_count() > 30 || 
			m_tag_writer->release->artists[0]->full_artist->images.get_count() > 30) {

			HWND hwndManageArt = uGetDlgItem(IDC_TRACK_MATCH_ALBUM_ART);
			::EnableWindow(hwndManageArt, FALSE);
			request_preview(0, false, false);
		}
		else {
			for (int it = 0; it < m_tag_writer->release->images.get_count(); it++) {
				request_preview(it, false, it != 0);		
			}
			for (int it = 0; it < m_tag_writer->release->artists[0]->full_artist->images.get_count(); it++) {
				request_preview(it, true, it != 0);
			}
			for (size_t i = 0; i < album_art_ids::num_types(); i++) {
				if (i == 2)
					request_file_preview(3, false);
				else if (i == 3)
					request_file_preview(2, false);
				else request_file_preview(i, false);
			}		
		}
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
		local_status = m_tag_writer->match_status;

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

void CTrackMatchingDialog::request_preview(size_t img_ndx, bool artist_art, bool onlycache) {
	service_ptr_t<process_artwork_preview_callback> task =
		new service_impl_t<process_artwork_preview_callback>(this, m_tag_writer->release, img_ndx, artist_art, onlycache);
	task->start(m_hWnd);
}

void CTrackMatchingDialog::request_file_preview(size_t img_ndx, bool artist_art) {
	service_ptr_t<process_file_artwork_preview_callback> file_art_task =
		new service_impl_t<process_file_artwork_preview_callback>(this,
			m_tag_writer->release, m_tag_writer->finfo_manager->items, img_ndx, artist_art);
	file_art_task->start(m_hWnd);
}

void CTrackMatchingDialog::process_artwork_preview_done(size_t img_ndx, bool artist_art, MemoryBlock callback_small_art) {
	HWND lvlist = uGetDlgItem(IDC_DISCOGS_TRACK_LIST);
	art_src art_source = artist_art ? art_src::art : art_src::alb;
	bool res = m_coord.show_artwork_preview(img_ndx, art_source, callback_small_art);
	ListView_RedrawItems(lvlist, img_ndx, img_ndx);
}

void CTrackMatchingDialog::process_file_artwork_preview_done(size_t img_ndx, bool artist_art, std::pair<HBITMAP, HBITMAP> callback_pair_hbitmaps, std::pair<pfc::string8, pfc::string8> temp_file_names) {
	HWND lvlist = uGetDlgItem(IDC_FILE_LIST);
	art_src art_source = artist_art ? art_src::art : art_src::alb;
	bool res = m_coord.show_file_artwork_preview(img_ndx, art_source, callback_pair_hbitmaps, temp_file_names);
	ListView_RedrawItems(lvlist, img_ndx, img_ndx);
}

void CTrackMatchingDialog::process_download_art_paths_done(pfc::string8 callback_release_id,
	std::shared_ptr<std::vector<std::pair<pfc::string8, bit_array_bittable>>> vres,
	pfc::array_t<GUID> my_album_art_ids) {
	
	if (m_tag_writer->release->id != callback_release_id || GetMode() != lsmode::art) {
		return;
	}

	bit_array_bittable saved_mask(my_album_art_ids.get_count());
	
	for (auto done_walk : *vres) {
		bit_array_bittable saved_mask_walk = done_walk.second;
		size_t true_pos = saved_mask_walk.find_first(true, 0, saved_mask_walk.size());
		while (true_pos < saved_mask_walk.size()) {
			saved_mask.set(true_pos, true);
			true_pos = saved_mask_walk.find_first(true, true_pos + 1, saved_mask_walk.size());
		}
	}

	bit_array_bittable refresh_mask(album_art_ids::num_types());
	bit_array_bittable refresh_mask_types(album_art_ids::num_types());

	size_t img_lv_ndx;
	size_t img_type;
	for (size_t i = 0; i < saved_mask.size(); i++) {

		if (!saved_mask.get(i))
			continue;

		size_t dc_art_ids_size = m_coord.GetDiscogsArtRowLvSize();

		for (size_t walk = 0; walk < dc_art_ids_size; walk++) {
			var_it_t out;
			size_t ndx_v = m_coord.Get_V_LvRow(lsmode::art, true, walk, out);
			getimages_it img_it = std::get<2>(*out).second;

			ndx_image_t ndx_image_file = img_it->first;
			img_type = ndx_image_file.first;
			if (ndx_v == i) {
				
				img_lv_ndx = walk;

				getimages_file_it file_it;
				size_t ndx_vfile = m_coord.GetLvFileArtRow(img_lv_ndx, file_it);
				if (ndx_vfile != pfc_infinite) {
					ndx_image_file_t ndx_image_file = file_it->first;
					size_t img_ndx = ndx_image_file.first;				
					refresh_mask.set(img_ndx, true);
					refresh_mask_types.set(img_ndx, img_type == 2);
				}
			}
		}
	}

	size_t true_pos = refresh_mask.find_first(true, 0, refresh_mask.size());
	while (true_pos < refresh_mask.size()) {
		request_file_preview(true_pos, refresh_mask_types.get(true_pos));
		true_pos = refresh_mask.find_first(true, true_pos + 1, refresh_mask.size());
	}

	m_coord.SetMode(lsmode::art);
	m_coord.Show();
}

bool CTrackMatchingDialog::initialize_next_tag_writer() {
	
	CONFARTWORK = uartwork(m_conf);
	
	match_message_update();

	insert_track_mappings();

	// TODO: assess this! (return value not used? does it work if skip release dialog checked?)
	if (!multi_mode && m_tag_writer->match_status == MATCH_SUCCESS && m_conf.skip_release_dialog_if_matched) {
		generate_track_mappings(m_tag_writer->track_mappings);
		service_ptr_t<generate_tags_task> task = new service_impl_t<generate_tags_task>(this, m_tag_writer, false, use_update_tags);
		task->start();
		return FALSE;
	}
	return TRUE;
}

void CTrackMatchingDialog::insert_track_mappings() {

	HWND discogs_track_list = uGetDlgItem(IDC_DISCOGS_TRACK_LIST);
	HWND file_list = uGetDlgItem(IDC_FILE_LIST);

	ListView_DeleteAllItems(discogs_track_list);
	ListView_DeleteAllItems(file_list);

	hook.set_custom("DISPLAY_ANVS", m_conf.display_ANVs);
	hook.set_custom("REPLACE_ANVS", m_conf.replace_ANVs);

	const size_t count = m_tag_writer->track_mappings.get_count();
	const size_t count_finfo = m_tag_writer->finfo_manager->items.get_count();
	
	titleformat_object::ptr lenght_script;
	static_api_ptr_t<titleformat_compiler>()->compile_safe_ex(lenght_script, "%length%");

	for (size_t i = 0; i < count; i++) {
		const track_mapping & mapping = m_tag_writer->track_mappings[i];   
		if (i < m_tag_writer->finfo_manager->items.get_count()) {			
			file_info &finfo = m_tag_writer->finfo_manager->get_item(mapping.file_index);
			auto item = m_tag_writer->finfo_manager->items.get_item(mapping.file_index);
			pfc::string8 formatted_name;
			m_conf.release_file_format_string->run_simple(item->get_location(), &finfo, formatted_name);
			pfc::string8 display = pfc::string_filename_ext(formatted_name);
			pfc::string8 formatted_length;
			item->format_title(nullptr, formatted_length, lenght_script, nullptr);
			listview_helper::insert_item2(file_list, i, display, formatted_length,(LPARAM)mapping.file_index);
		}
		
		if (mapping.discogs_disc == -1 && mapping.discogs_track == -1) {
			//more files than discogs tracks
		}
		else {
			const ReleaseDisc_ptr disc = m_tag_writer->release->discs[mapping.discogs_disc];
			const ReleaseTrack_ptr track = disc->tracks[mapping.discogs_track];
			hook.set_release(&m_tag_writer->release);
			hook.set_disc(&disc);
			hook.set_track(&track);

			if (i == 0) {
				//warn differnt release id
				bool b_diff_id = false;
				file_info_impl finfo;
				metadb_handle_ptr item = m_tag_writer->finfo_manager->items[0];
				item->get_info(finfo);

				pfc::string8 local_release_id;

				const char* ch_local_rel = finfo.meta_get("DISCOGS_RELEASE_ID", 0);
				if (ch_local_rel) {
					local_release_id = ch_local_rel;
				}
				b_diff_id = (local_release_id.get_length() && !STR_EQUAL(m_tag_writer->release->id, local_release_id));
				//..

				pfc::string8 compact_release;
				CONF.search_master_sub_format_string->run_hook(location, &info, &hook/*titlehook.get()*/, compact_release, nullptr);

				pfc::string8 rel_desc = b_diff_id ? "!! " : "";
				rel_desc  << ltrim(compact_release);
				uSetDlgItemText(m_hWnd, IDC_STATIC_MATCH_TRACKING_REL_NAME, rel_desc);
			}

			pfc::string8 text;
			m_conf.release_discogs_format_string->run_hook(location, &info, &hook, text, nullptr);
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

LRESULT CTrackMatchingDialog::OnMoveTrackUp(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	HWND discogs_track_list = uGetDlgItem(IDC_DISCOGS_TRACK_LIST);
	move_selected_items_up(discogs_track_list);
	match_message_update(match_manual);
	return FALSE;
}

LRESULT CTrackMatchingDialog::OnMoveTrackDown(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	HWND discogs_track_list = uGetDlgItem(IDC_DISCOGS_TRACK_LIST);
	match_message_update(match_manual);
	move_selected_items_down(discogs_track_list);
	return FALSE;
}

LRESULT CTrackMatchingDialog::OnRemoveTrackButton(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	HWND discogs_track_list = uGetDlgItem(IDC_DISCOGS_TRACK_LIST);
	HWND file_list = uGetDlgItem(IDC_FILE_LIST);
	match_message_update(match_manual);
	if (wID == IDC_REMOVE_DISCOGS_TRACK_BUTTON)
		remove_items(discogs_track_list, false);
	else
		remove_items(file_list, false);
	return FALSE;
}

void CTrackMatchingDialog::generate_track_mappings(track_mappings_list_type &track_mappings) {
	
	track_mappings.force_reset();

	int file_index = -1;
	int dc_track_index = -1;
	int dc_disc_index = -1;

	//or check tag_writer->finfo_manager->items
	const size_t count_d = m_coord.GetDiscogsTrackUiLvSize(); // vrow_discogs.size(); // ListView_GetItemCount(discogs_track_list);
	const size_t count_f = m_coord.GetFileTrackUiLvSize();		// ListView_GetItemCount(file_list);

	for (size_t i = 0; i < count_d; i++) {
		dc_track_index = -1;
		dc_disc_index = -1;
		track_mapping track;
		track_it track_match;
		size_t res = m_coord.GetDiscogsTrackUiAtLvPos(i, track_match);
		file_match_nfo match_nfo = track_match->second;
		dc_track_index = match_nfo.track;
		dc_disc_index = match_nfo.disc;

		file_it file_match;
		if (i >= count_f)
		{
			dc_track_index = -1;
			dc_disc_index = -1;
		}
		else {
			size_t res = m_coord.GetFileTrackUiAtLvPos(i, file_match);
			file_index = file_match->second;
		}

		track.enabled = (dc_track_index != -1 && dc_disc_index != -1);
		track.discogs_disc = dc_disc_index; //DECODE_DISCOGS_DISK(dindex);
		track.discogs_track = dc_track_index;  //DECODE_DISCOGS_TRACK(dindex);
		track.file_index = file_index;
		track_mappings.append_single(track);
	}
}

bool CTrackMatchingDialog::init_count() {
	const bool die = !m_conf.update_tags_manually_match;
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
		m_tag_writer = tag_writers[tw_index++];
		if (m_tag_writer->force_skip || m_tag_writer->match_status == MATCH_SUCCESS || m_tag_writer->match_status == MATCH_ASSUME) {
			tw_skip++;
			continue;
		}
		update_window_title();
		initialize_next_tag_writer();
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
			m_tag_writer = tag_writers[tw_index++];
			update_window_title();
			initialize_next_tag_writer();
			return true;
		}
		tw_skip--;
	}
	return false;
}

void CTrackMatchingDialog::finished_tag_writers() {
	service_ptr_t<generate_tags_task_multi> task = new service_impl_t<generate_tags_task_multi>(tag_writers, m_conf.update_tags_preview_changes, use_update_tags);
	task->start();
}

LRESULT CTrackMatchingDialog::OnBack(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	m_idc_list.TooltipRemove();
	m_ifile_list.TooltipRemove();
	go_back();
	return TRUE;
}

//write all discogs art selected, matching available album art ids
LRESULT CTrackMatchingDialog::OnWriteArtwork(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	CONFARTWORK = *m_coord.GetUartwork();
	CONFARTGUIDS = *m_coord.GetUartwork_GUIDS();

	pfc::array_t<GUID> my_album_art_ids; my_album_art_ids.resize(m_tag_writer->get_art_count());
	
	getimages_file_it getimgfile_it;
	getimages_it getimg_it;
	
	for (size_t walk = 0; walk < m_coord.GetDiscogsArtRowLvSize(); walk++) {

		var_it_t out;
		size_t ndx_discogs_img = m_coord.Get_V_LvRow(lsmode::art, true, walk, out);
		
		if (ndx_discogs_img == pfc_infinite) break;

		getimg_it = std::get<2>(*out).second;

		size_t ndx_file_img = m_coord.Get_V_LvRow(lsmode::art, false, walk, out);

		if (ndx_file_img != pfc_infinite) {

			getimgfile_it = std::get<3>(*out).second;

			GUID imgfileguid = getimgfile_it->first.second;
			bool btype = false;
			for (size_t i = 0; i < album_art_ids::num_types(); i++) {
				if (album_art_ids::query_type(i) == imgfileguid) {
					btype = true;
					break;
				}
			}
			if (btype)
				my_album_art_ids[ndx_discogs_img] = imgfileguid;
		}
		else {
			int debug = 1;
		}
	}

	service_ptr_t<download_art_paths_task> task_ids =
		new service_impl_t<download_art_paths_task>(this, m_tag_writer->release->id,
			m_tag_writer->finfo_manager->items,	my_album_art_ids, false);

	task_ids->start();

	return TRUE;
}
//write only matching album art ids
LRESULT CTrackMatchingDialog::OnWriteArtworkKnownIds(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	CONFARTWORK = *m_coord.GetUartwork();
	CONFARTGUIDS = *m_coord.GetUartwork_GUIDS();

	pfc::array_t<GUID> my_album_art_ids; my_album_art_ids.resize(m_tag_writer->get_art_count());

	getimages_file_it getimgfile_it;
	getimages_it getimg_it;

	for (size_t walk = 0; walk < m_coord.GetFileArtRowLvSize(); walk++) {

		var_it_t out;
		size_t ndx_file_img = m_coord.Get_V_LvRow(lsmode::art, false, walk, out);

		if (ndx_file_img == pfc_infinite) break;

		getimgfile_it = std::get<3>(*out).second;

		size_t ndx_discogs_img = m_coord.Get_V_LvRow(lsmode::art, true, walk, out);

		if (ndx_discogs_img != pfc_infinite) {

			getimg_it = std::get<2>(*out).second;

			GUID imgfileguid = getimgfile_it->first.second;
			bool btype = false;
			for (size_t i = 0; i < album_art_ids::num_types(); i++) {
				if (album_art_ids::query_type(i) == imgfileguid) {
					btype = true;
					break;
				}
			}
			if (btype)
				my_album_art_ids[ndx_discogs_img] = imgfileguid;
		}
		else {
			int debug = 1;
		}
	}

	//write only art
	service_ptr_t<download_art_paths_task> task =
		new service_impl_t<download_art_paths_task>(this, m_tag_writer->release->id,
			m_tag_writer->finfo_manager->items, my_album_art_ids, false);

	task->start();

	return TRUE;
}

LRESULT CTrackMatchingDialog::OnPreviewTags(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	PFC_ASSERT(!multi_mode);
	
	CONFARTWORK = *m_coord.GetUartwork();
	CONFARTGUIDS = *m_coord.GetUartwork_GUIDS();
	
	generate_track_mappings(m_tag_writer->track_mappings);
	service_ptr_t<generate_tags_task> task = new service_impl_t<generate_tags_task>(this, m_tag_writer, true, use_update_tags);
	task->start();
	return TRUE;
}

LRESULT CTrackMatchingDialog::OnWriteTags(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	PFC_ASSERT(!multi_mode);
	generate_track_mappings(m_tag_writer->track_mappings);
	service_ptr_t<generate_tags_task> task = new service_impl_t<generate_tags_task>(this, m_tag_writer, false, use_update_tags);
	task->start();
	return TRUE;
}

LRESULT CTrackMatchingDialog::OnMultiNext(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	PFC_ASSERT(multi_mode);
	generate_track_mappings(m_tag_writer->track_mappings);
	get_next_tag_writer();
	return TRUE;
}

void CTrackMatchingDialog::pushcfg() {
	bool conf_changed = build_current_cfg();
	if (conf_changed) {
		CONF.save(new_conf::ConfFilter::CONF_FILTER_TRACK, m_conf);
		CONF.load();
	}
}

inline bool CTrackMatchingDialog::build_current_cfg() {
	bool bres = false;
	//dlg dimensions
	CRect rcDlg;
	GetWindowRect(&rcDlg);

	int dlgwidth = rcDlg.Width();
	int dlgheight = rcDlg.Height();
	//dlg position
	if (rcDlg.left != LOWORD(m_conf.release_dialog_position) || rcDlg.top != HIWORD(m_conf.release_dialog_position)) {
		m_conf.release_dialog_position = MAKELPARAM(rcDlg.left, rcDlg.top);
		bres = bres || true;
	}
	//dlg size
	if (dlgwidth != LOWORD(m_conf.release_dialog_size) || dlgheight != HIWORD(m_conf.release_dialog_size)) {
		m_conf.release_dialog_size = MAKELPARAM(dlgwidth, dlgheight);
		bres = bres || true;
	}

	const foo_discogs_conf cfg_coord = CONF; //*coord.cfgRef();
	//track match columns
	if (cfg_coord.match_tracks_discogs_col1_width != m_conf.match_tracks_discogs_col1_width ||
		cfg_coord.match_tracks_discogs_col2_width != m_conf.match_tracks_discogs_col2_width ||
		//track match columns
		cfg_coord.match_tracks_files_col1_width != m_conf.match_tracks_files_col1_width ||
		cfg_coord.match_tracks_files_col2_width != m_conf.match_tracks_files_col2_width ||
		//styles
		cfg_coord.match_tracks_discogs_style != m_conf.match_tracks_discogs_style ||
		cfg_coord.match_tracks_files_style != m_conf.match_tracks_files_style) {

		bres = bres || true;
	}
	//artwork image columns
	if (cfg_coord.match_discogs_artwork_ra_width != m_conf.match_discogs_artwork_ra_width ||
		cfg_coord.match_discogs_artwork_type_width != m_conf.match_discogs_artwork_type_width ||
		cfg_coord.match_discogs_artwork_dim_width != m_conf.match_discogs_artwork_dim_width ||
		cfg_coord.match_discogs_artwork_save_width != m_conf.match_discogs_artwork_save_width ||
		cfg_coord.match_discogs_artwork_ovr_width != m_conf.match_discogs_artwork_ovr_width ||
		cfg_coord.match_discogs_artwork_embed_width != m_conf.match_discogs_artwork_embed_width ||
		//artwork file columns
		cfg_coord.match_file_artwork_name_width != m_conf.match_file_artwork_name_width ||
		cfg_coord.match_file_artwork_dim_width != m_conf.match_file_artwork_dim_width ||
		cfg_coord.match_file_artwork_size_width != m_conf.match_file_artwork_size_width ||
		//styles
		cfg_coord.match_discogs_artwork_art_style != m_conf.match_discogs_artwork_art_style ||
		cfg_coord.match_file_artworks_art_style != m_conf.match_file_artworks_art_style) {
		bres = bres || true;
	}
	return bres;
}

inline void CTrackMatchingDialog::load_size() {
	int width = LOWORD(m_conf.release_dialog_size);
	int height = HIWORD(m_conf.release_dialog_size);
	CRect rcCfg(0, 0, width, height);
	LPARAM lparamLeftTop = m_conf.release_dialog_position;
	::CenterWindow(this->m_hWnd, rcCfg, core_api::get_main_window(), lparamLeftTop);
}

void CTrackMatchingDialog::LibUIAsOwnerData(bool OwnerToLibUi) {

	CRect rc_dcList;
	CRect rc_fileList;
	CRect rc_UI_dcList;
	CRect rc_UI_fileList;

	if (OwnerToLibUi) {

		::GetWindowRect(uGetDlgItem(IDC_DISCOGS_TRACK_LIST), &rc_dcList);
		::GetWindowRect(uGetDlgItem(IDC_FILE_LIST), &rc_fileList);

		ScreenToClient(&rc_dcList);
		::SetWindowPos(uGetDlgItem(IDC_UI_LIST_DISCOGS), HWND_TOP, rc_dcList.left, rc_dcList.top,
			rc_dcList.Width(), rc_dcList.Height(), SWP_NOACTIVATE | SWP_SHOWWINDOW);
		ScreenToClient(&rc_fileList);
		::SetWindowPos(uGetDlgItem(IDC_UI_LIST_FILES), HWND_TOP, rc_fileList.left, rc_fileList.top,
			rc_fileList.Width(), rc_fileList.Height(), SWP_NOACTIVATE | SWP_SHOWWINDOW);
	}
	else {

		foo_discogs_conf* cfg_coord = m_coord.cfgRef();

		size_t icol = 0;
			
		LPARAM woa = cfg_coord->match_tracks_discogs_col1_width;
		size_t debug = HIWORD(woa);
		LPARAM tmpwoa = MAKELPARAM(LOWORD(woa), m_idc_list.GetColumnWidthF(icol));

		cfg_coord->match_tracks_discogs_col1_width = tmpwoa;

		icol++;

		woa = cfg_coord->match_tracks_discogs_col2_width;
		tmpwoa = MAKELPARAM(LOWORD(woa), m_idc_list.GetColumnWidthF(icol));
			
		cfg_coord->match_tracks_discogs_col2_width = tmpwoa;

		//second list
		icol = 0;

		woa = cfg_coord->match_tracks_files_col1_width;
		debug = HIWORD(woa);
		tmpwoa = MAKELPARAM(LOWORD(woa), m_ifile_list.GetColumnWidthF(icol));

		cfg_coord->match_tracks_files_col1_width = tmpwoa;

		icol++;

		woa = cfg_coord->match_tracks_files_col2_width;
		tmpwoa = MAKELPARAM(LOWORD(woa), m_ifile_list.GetColumnWidthF(icol));
			
		cfg_coord->match_tracks_files_col2_width = tmpwoa;

		::GetWindowRect(uGetDlgItem(IDC_UI_LIST_DISCOGS), &rc_UI_dcList);
		::GetWindowRect(uGetDlgItem(IDC_UI_LIST_FILES), &rc_UI_fileList);

		CRect rc_dcList;
		::GetWindowRect(uGetDlgItem(IDC_DISCOGS_TRACK_LIST), &rc_dcList);
		CRect rc_fileList;
		::GetWindowRect(uGetDlgItem(IDC_FILE_LIST), &rc_fileList);

		ScreenToClient(&rc_UI_dcList);
		::SetWindowPos(uGetDlgItem(IDC_DISCOGS_TRACK_LIST), HWND_TOP, rc_UI_dcList.left, rc_UI_dcList.top,
			rc_UI_dcList.Width(), rc_UI_dcList.Height(), SWP_NOACTIVATE | SWP_SHOWWINDOW);
		ScreenToClient(&rc_UI_fileList);
		::SetWindowPos(uGetDlgItem(IDC_FILE_LIST), HWND_TOP, rc_UI_fileList.left, rc_UI_fileList.top,
			rc_UI_fileList.Width(), rc_UI_fileList.Height(), SWP_NOACTIVATE | SWP_SHOWWINDOW);
	}
}

LRESULT CTrackMatchingDialog::OnCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	
	if (LOWORD(wParam) == IDC_TRACK_MATCH_ALBUM_ART) {
		
		if (HIWORD(wParam) == BN_CLICKED) {
			
			bool state = (uButton_GetCheck(m_hWnd, IDC_TRACK_MATCH_ALBUM_ART) == TRUE);

			SetDlgItemText(IDC_TRACKLIST_GROUP, state ? _T("Manage Artwork") : _T("Match Tracks"));

			//backup column defs (mode exit)
			m_coord.PullConf(state ? lsmode::tracks_ui : lsmode::art, true, &m_conf);
			m_coord.PullConf(state ? lsmode::tracks_ui : lsmode::art, false, &m_conf);

			if (state) {

				LibUIAsOwnerData(false);

				::ShowWindow(uGetDlgItem(IDC_DISCOGS_TRACK_LIST), SW_SHOW);
				::ShowWindow(uGetDlgItem(IDC_FILE_LIST), SW_SHOW);
				::ShowWindow(uGetDlgItem(IDC_BTN_TRACK_MATCH_WRITE_ARTWORK), SW_SHOW);

				m_idc_list.DeleteColumns(bit_array_true(), false);
				m_ifile_list.DeleteColumns(bit_array_true(), false);

				::ShowWindow(uGetDlgItem(IDC_UI_LIST_DISCOGS), SW_HIDE);
				::ShowWindow(uGetDlgItem(IDC_UI_LIST_FILES), SW_HIDE);

			}
			else {

				LibUIAsOwnerData(true);

				if (!m_idc_list.GetColumnCount()) {
					m_coord.InitUiList(m_idc_list.m_hWnd, lsmode::tracks_ui, true, &m_idc_list);
					m_coord.InitUiList(m_ifile_list.m_hWnd, lsmode::tracks_ui, false, &m_ifile_list);
				}

				::ShowWindow(uGetDlgItem(IDC_UI_LIST_DISCOGS), SW_SHOW);
				::ShowWindow(uGetDlgItem(IDC_UI_LIST_FILES), SW_SHOW);

				::ShowWindow(uGetDlgItem(IDC_DISCOGS_TRACK_LIST), SW_HIDE);
				::ShowWindow(uGetDlgItem(IDC_FILE_LIST), SW_HIDE);
				::ShowWindow(uGetDlgItem(IDC_BTN_TRACK_MATCH_WRITE_ARTWORK), SW_HIDE);
			}

			HWND wnd = uGetDlgItem(IDC_DISCOGS_TRACK_LIST);

			m_coord.SetMode(state ? lsmode::art : lsmode::tracks_ui);
			m_coord.Show();
			::ShowScrollBar(wnd, SB_HORZ, FALSE);
		}
	}
	return 0;
}

LRESULT CTrackMatchingDialog::OnMultiSkip(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	PFC_ASSERT(multi_mode);
	m_tag_writer->skip = true;
	get_next_tag_writer();
	return TRUE;
}

LRESULT CTrackMatchingDialog::OnMultiPrev(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	PFC_ASSERT(multi_mode);
	get_previous_tag_writer();
	return TRUE;
}

void CTrackMatchingDialog::update_list_width(HWND list, bool initialize) {
	CRect client_rectangle;
	::GetClientRect(list, &client_rectangle);
	int width = client_rectangle.Width();

	int c1, c2;
	if (initialize) {
		c2 = DEF_TIME_COL_WIDTH;
	}
	else {
		c2 = ListView_GetColumnWidth(list, 1);
	}
	c1 = width - c2;
	const int count = ListView_GetColumnCount(list);

	ListView_SetColumnWidth(list, count-2, c1);
	ListView_SetColumnWidth(list, count-1, c2);
}

LRESULT CTrackMatchingDialog::list_key_down(HWND wnd, LPNMHDR lParam) {
	
	NMLVKEYDOWN* info = reinterpret_cast<NMLVKEYDOWN*>(lParam);

	bool bshift = (GetKeyState(VK_SHIFT) & 0x8000) == 0x8000;
	bool bctrl = (GetKeyState(VK_CONTROL) & 0x8000);

	HWND wndFiles = uGetDlgItem(IDC_FILE_LIST);
	HWND wndDiscogs = uGetDlgItem(IDC_DISCOGS_TRACK_LIST);
	HWND focuswnd = ::GetFocus();

	bool is_files = wnd == wndFiles || wnd == uGetDlgItem(IDC_UI_LIST_FILES);
	bool is_discogs = wnd == uGetDlgItem(IDC_DISCOGS_TRACK_LIST) ||
		wnd == uGetDlgItem(IDC_UI_LIST_DISCOGS);
	
	CListControlOwnerData* ilist = nullptr; bool is_ilist = false;
	if (wnd == uGetDlgItem(IDC_UI_LIST_DISCOGS) || wnd == uGetDlgItem(IDC_UI_LIST_FILES)) {
		is_ilist = true;
		ilist = (wnd == uGetDlgItem(IDC_UI_LIST_DISCOGS)) ? &m_idc_list : &m_ifile_list;
	}

	bool is_valid = is_files || is_discogs || is_ilist;

	int csel = is_ilist ? ilist->GetSelectedCount()
		: ListView_GetSelectedCount(wnd);

	int citems = is_ilist ? ilist->GetItemCount()
		: ListView_GetItemCount(wnd);

	bit_array_bittable selmask(0);

	if (is_ilist) selmask = ilist->GetSelectionMask();
	else {
		selmask.resize(citems);
		size_t n = ListView_GetFirstSelection(wnd) == -1 ? pfc_infinite : ListView_GetFirstSelection(wnd);
		for (size_t i = n; i < citems; i++) {
			selmask.set(i, ListView_IsItemSelected(wnd, i));
		}
	}

	CPoint key_no_point(-1, -1);

	switch (info->wVKey) {
	case VK_DELETE:
		bool bres;
		try {
			bres = switch_context_menu(wnd, key_no_point, is_files, ID_REMOVE, selmask, ilist);
		}
		catch (...) {}
		return bres;

	case VK_DOWN:
		switch (GetHotkeyModifierFlags()) {
		case MOD_CONTROL:

			move_selected_items_down(wnd);
			match_message_update(match_manual);
			return TRUE;
		default:
			return FALSE;
		}

	case VK_UP:
		switch (GetHotkeyModifierFlags()) {
		case MOD_CONTROL:

			move_selected_items_up(wnd);
			match_message_update(match_manual);
			return TRUE;
		default:
			return FALSE;
		}

	case 0x41: //'a' select all - invert selection
		if (bshift && bctrl) {
			bool bres;
			try {
				bres = switch_context_menu(wnd, key_no_point, is_files, ID_INVERT_SELECTION, selmask, ilist);
			}
			catch (...) {}
			return bres;
		}
		else if (bctrl) {
			bool bres;
			try {
				bres = switch_context_menu(wnd, key_no_point, is_files, ID_SELECT_ALL, selmask, ilist);
			}
			catch (...) {}
			return bres;
		}
		else
			return FALSE;

	case 0x43: //'c' crop
		if (bshift && bctrl) {
			;
			return TRUE;
		}
		else if (bctrl) {
			bool bres;
			try {
				bres = switch_context_menu(wnd, key_no_point, is_files, ID_CROP, selmask, ilist);
			}
			catch (...) {}
			return bres;
		}
		else
			return FALSE;
	case 0x50: //'p' preview
		if (GetMode() != lsmode::art) return TRUE;
		if (bshift && bctrl) {
			;
			return TRUE;
		}
		else if (bctrl) {
			bool bres;
			try {
				bres = switch_context_menu(wnd, key_no_point, is_files, ID_ART_PREVIEW, selmask, ilist);
			}
			catch (...) {}
			return bres;
		}
		else
			return FALSE;
	case 0x57: //'w' write
		if (GetMode() != lsmode::art) return TRUE;
		if (bshift && bctrl) {
			;
			return TRUE;
		}
		else if (bctrl) {
			bool bres;
			try {
				bres = switch_context_menu(wnd, key_no_point, is_files, ID_ART_WRITE, selmask, ilist);
			}
			catch (...) {}
			return bres;
		}
		else
			return FALSE;
	case 0x4F: //'o' overwrite
		if (GetMode() != lsmode::art) return TRUE;
		if (bshift && bctrl) {
			;
			return TRUE;
		}
		else if (bctrl) {
			bool bres;
			try {
				bres = switch_context_menu(wnd, key_no_point, is_files, ID_ART_OVR, selmask, ilist);
			}
			catch (...) {}
			return bres;
		}
		else
			return FALSE;
	case 0x45: //'e' embed
		if (GetMode() != lsmode::art) return TRUE;
		if (bshift && bctrl) {
			;
			return TRUE;
		}
		else if (bctrl) {
			bool bres;
			try {
				bres = switch_context_menu(wnd, key_no_point, is_files, ID_ART_EMBED, selmask, ilist);
			}
			catch (...) {}
			return bres;
		}
		else
			return FALSE;
	//case 0x78: //'n' next list view (tile, details, list...)
	//	if (!GetMode() == lsmode::art) return TRUE;
	//	if (shift && ctrl) {
	//		;
	//		return TRUE;
	//	}
	//	else if (ctrl) {
	//		bool bres;
	//		try {
	//			bres = switch_context_menu(wndList, NEXT ARt view);
	//		}
	//		catch (...) {}
	//		return bres;
	//	}
	//	else
	//		return FALSE;
	case 0x54: //'t' Artwork view <-> Track view
		if (bshift && bctrl) {
			;
			return TRUE;
		}
		else if (bctrl) {
			bool bres;
			try {
				bres = switch_context_menu(wnd, CPoint(-1, -1), is_files, ID_ART_TOOGLE_TRACK_ART_MODES, selmask, ilist);
			}
			catch (...) {}
			return bres;
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

bool IsTile(HWND hwnd) {
	DWORD dwView = ListView_GetView(hwnd);
	return (dwView == LV_VIEW_TILE);
}

//TODO: tidy up GetDispInfo

LRESULT CTrackMatchingDialog::OnDiscogListGetDispInfo(LPNMHDR lParam) {
	if (GetMode() == lsmode::tracks_ui) {
		return DiscogTrackGetDispInfo(lParam);
	}
	else {
		return DiscogArtGetDispInfo(lParam);
	}
}

LRESULT CTrackMatchingDialog::DiscogTrackGetDispInfo(LPNMHDR lParam) {
	LV_DISPINFO* pLvdi = (LV_DISPINFO*)lParam;
	HWND wnd = pLvdi->hdr.hwndFrom;
	UINT id = pLvdi->hdr.idFrom;
	LV_ITEM* lvitem = &(pLvdi->item);

	if ((GetMode() == lsmode::tracks_ui) && (lvitem->mask & LVIF_TEXT)) {

		size_t pos = lvitem->iItem;

		if (id == IDC_DISCOGS_TRACK_LIST) {
			
			track_it track_match;
			size_t res = m_coord.GetDiscogsTrackUiAtLvPos(pos, track_match);

			switch (lvitem->iSubItem) {
                case 0: {
                    TCHAR outBuffer[MAX_PATH + 1] = {};
                    pfc::stringcvt::convert_utf8_to_wide(outBuffer, MAX_PATH,
                        track_match->first.first, track_match->first.first.get_length());
                    _tcscpy_s(pLvdi->item.pszText, MAX_PATH/*pLvdi->item.cchTextMax*/, const_cast<TCHAR*>(outBuffer));
                    pLvdi->item.cchTextMax = MAX_PATH;
                    break;
                }
                // primary or secundary
                case 1: {
                    TCHAR outBuffer[MAX_PATH + 1] = {};
                    pfc::stringcvt::convert_utf8_to_wide(outBuffer, MAX_PATH,
                        track_match->first.second, track_match->first.second.get_length());
                    _tcscpy_s(pLvdi->item.pszText, MAX_PATH/*pLvdi->item.cchTextMax*/, const_cast<TCHAR*>(outBuffer));
                    break;
                }
    
                default: {
                    int debug = 1;
                }
			}
		}
		else {
			file_it file_match;
			size_t res = m_coord.GetFileTrackUiAtLvPos(pos, file_match);

			switch (lvitem->iSubItem) {
                case 0: {
                    TCHAR outBuffer[MAX_PATH + 1] = {};
                    pfc::stringcvt::convert_utf8_to_wide(outBuffer, MAX_PATH,
                        file_match->first.first, file_match->first.first.get_length());
                    _tcscpy_s(pLvdi->item.pszText, MAX_PATH/*pLvdi->item.cchTextMax*/, const_cast<TCHAR*>(outBuffer));
                    pLvdi->item.cchTextMax = MAX_PATH;
                    break;
                }
                // primary or secundary
                case 1: {
                    TCHAR outBuffer[MAX_PATH + 1] = {};
                    pfc::stringcvt::convert_utf8_to_wide(outBuffer, MAX_PATH,
                        file_match->first.second, file_match->first.second.get_length());
                    _tcscpy_s(pLvdi->item.pszText, MAX_PATH/*pLvdi->item.cchTextMax*/, const_cast<TCHAR*>(outBuffer));
                    pLvdi->item.cchTextMax = MAX_PATH;
                    break;
                }
    
                default: {
                    int debug = 1;
                }
			}
		}
	}

	if ((GetMode() == lsmode::tracks_ui) && (lvitem->mask & LVIF_STATE)) {

		size_t pos = lvitem->iItem;

		if (id == IDC_DISCOGS_TRACK_LIST) {

			track_it track_match;
			size_t res = m_coord.GetDiscogsTrackUiAtLvPos(pos, track_match);
		}
		else {}
	}

	return 0;
}

LRESULT CTrackMatchingDialog::DiscogArtGetDispInfo(LPNMHDR lParam) {
	LV_DISPINFO* pLvdi = (LV_DISPINFO*)lParam;
	HWND wnd = pLvdi->hdr.hwndFrom;
	UINT id = pLvdi->hdr.idFrom;

	//todo: test tile view requests for out of bound items...
	LV_ITEM* lvitem = &(pLvdi->item);

	if (GetMode() != lsmode::art)
		return 0;

	if (id == IDC_DISCOGS_TRACK_LIST) {
		if (lvitem->mask & LVIF_COLUMNS) {
			UINT arrColumns[5] = { 1, 2, 3, 4, 5 };
			lvitem->cColumns = 5;
			lvitem->puColumns[0] = 1;
			lvitem->puColumns[1] = 2;
			lvitem->puColumns[2] = 3;
			lvitem->puColumns[3] = 4;
			lvitem->puColumns[4] = 5;
		}

		if (lvitem->mask & LVIF_TEXT) {
			uartwork uart = *m_coord.GetUartwork();
			DWORD dwView = ListView_GetView(wnd);
			bool bTile = dwView == LV_VIEW_TILE;

			getimages_it imginfo;
			size_t pos = lvitem->iItem;

			var_it_t out;
			size_t ndxpos = m_coord.Get_V_LvRow(lsmode::art, true, lvitem->iItem, out);
			imginfo = std::get<2>(*out).second;

			//todo: tile view requests for out of bound items?
			if (ndxpos == pfc_infinite) {
				return 0;
			}

			art_src art_source = (art_src)imginfo->first.first;

			if (art_source == art_src::unknown) {
				//initiating form, list is still empty?
				return 0;
			}

			lv_art_param this_lv_param(lvitem->iItem, art_source);

			bool dc_isalbum = this_lv_param.is_album();

			switch (lvitem->iSubItem) {
                case 0: {
                    pLvdi->item.pszText = !bTile ? (dc_isalbum ? _T("Rel") : _T("Art")) : dc_isalbum ? m_szAlbum : m_szArtist;
                    break;
                }
                // primary or secundary
                case 1: {
                    if (pLvdi->item.cchTextMax > 0) {
                        TCHAR outBuffer[MAX_PATH + 1] = {};
                        pfc::stringcvt::convert_utf8_to_wide(outBuffer, MAX_PATH,
                            imginfo->second.at(0).get_ptr(), imginfo->second.at(0).get_length());
                        _tcscpy_s(pLvdi->item.pszText, MAX_PATH/*pLvdi->item.cchTextMax*/, const_cast<TCHAR*>(outBuffer));
                        pLvdi->item.cchTextMax = MAX_PATH;
                    }
                    break;
                }
                //dimensions
                case 2: {
                    if (pLvdi->item.cchTextMax > 0) {
                        pfc::string8 dim(imginfo->second.at(1));
                        dim << "x" << imginfo->second.at(2);
                        TCHAR outBuffer[MAX_PATH + 1] = {};
                        pfc::stringcvt::convert_utf8_to_wide(outBuffer, MAX_PATH,
                            dim.get_ptr(), dim.get_length());
                        _tcscpy_s(pLvdi->item.pszText, MAX_PATH/*pLvdi->item.cchTextMax*/, const_cast<TCHAR*>(outBuffer));
                        pLvdi->item.cchTextMax = MAX_PATH;
                    }
                    break;
                }
                // write
                case 3: {
                    if (pLvdi->item.cchTextMax > 0) {
                        af att = dc_isalbum ? af::alb_sd : af::art_sd;
                        bool bflag = uart.getflag(att, ndxpos);
                        pfc::string8 str(bflag ? bTile ? "Write" : "x" : "");
                        TCHAR outBuffer[MAX_PATH + 1] = {};
                        pfc::stringcvt::convert_utf8_to_wide(outBuffer, MAX_PATH,
                            str.get_ptr(), str.get_length());
                        _tcscpy_s(pLvdi->item.pszText, MAX_PATH, const_cast<TCHAR*>(outBuffer));
                        pLvdi->item.cchTextMax = MAX_PATH;
                    }
                    break;
                }
                // overwrite
                case 4: {
                    if (pLvdi->item.cchTextMax > 0) {
                        af att = dc_isalbum ? af::alb_ovr : af::art_ovr;
                        bool bflag = uart.getflag(att, ndxpos);
                        pfc::string8 str(bflag ? bTile ? "Overwrite" : "x" : "");
                        TCHAR outBuffer[MAX_PATH + 1] = {};
                        pfc::stringcvt::convert_utf8_to_wide(outBuffer, MAX_PATH,
                            str.get_ptr(), str.get_length());
                        _tcscpy_s(pLvdi->item.pszText, MAX_PATH, const_cast<TCHAR*>(outBuffer));
                        pLvdi->item.cchTextMax = MAX_PATH;
                    }
                    break;
                }
                // embed
                case 5: {
                    if (pLvdi->item.cchTextMax > 0) {
                        af att = dc_isalbum ? af::alb_emb : af::art_emb;
                        bool bflag = uart.getflag(att, ndxpos);
                        pfc::string8 str(bflag ? bTile ? "Embed" : "x" : "");
                        TCHAR outBuffer[MAX_PATH + 1] = {};
                        pfc::stringcvt::convert_utf8_to_wide(outBuffer, MAX_PATH,
                            str.get_ptr(), str.get_length());
                        _tcscpy_s(pLvdi->item.pszText, MAX_PATH, const_cast<TCHAR*>(outBuffer));
                        pLvdi->item.cchTextMax = MAX_PATH;
                    }
                    break;
                }
                default: {
                    int debug = 1;
                }
			}
		}

		if (lvitem->mask & LVIF_IMAGE) {
			if (pLvdi->item.iSubItem == 0) {
				HWND discogs_track_list = uGetDlgItem(IDC_DISCOGS_TRACK_LIST);

				getimages_it imginfo;

				var_it_t out;
				size_t ndxpos = m_coord.Get_V_LvRow(lsmode::art, true, lvitem->iItem, out);
				imginfo = std::get<2>(*out).second;

				if (ndxpos == pfc_infinite) return 0;

				CImageList testlist;
				testlist = ListView_GetImageList(discogs_track_list, LVSIL_NORMAL);

				int c = !testlist ? -1 : testlist.GetImageCount();

				CImageList testlist_small;
				testlist_small = ListView_GetImageList(discogs_track_list, LVSIL_SMALL);
				int csmall = !testlist_small ? -1 : testlist_small.GetImageCount();

				DWORD dwView = ListView_GetView(wnd);
				bool bTile = dwView == LV_VIEW_TILE;

				if (bTile) {
					if (testlist && testlist.GetImageCount() /*&& ndxpos <= testlist.GetImageCount()*/) {
						pLvdi->item.iImage = ndxpos;
					}
				}
				else {
					if (testlist_small && testlist_small.GetImageCount() /*&& ndxpos <= testlist_small.GetImageCount()*/) {
						pLvdi->item.iImage = ndxpos;
					}
				}
			}
		}
	}
	else {
		//IDC_FILE_LIST
		if (lvitem->mask & LVIF_TEXT) {
			size_t pos = lvitem->iItem;

			getimages_file_it imagefilerow;
			size_t ndx_pos = m_coord.GetLvFileArtRow(pLvdi->item.iItem, imagefilerow);

			if (ndx_pos == pfc_infinite) return 0;

			switch (lvitem->iSubItem) {
			//file name
			case 0: {
				pfc::string8 str(imagefilerow->second.at(0));
				pfc::string8 filename = pfc::string_filename_ext(str);
				 
				TCHAR outBuffer[MAX_PATH + 1] = {};
				pfc::stringcvt::convert_utf8_to_wide(outBuffer, MAX_PATH,
					filename.get_ptr(), filename.get_length());
				_tcscpy_s(pLvdi->item.pszText, MAX_PATH, const_cast<TCHAR*>(outBuffer));
				pLvdi->item.cchTextMax = MAX_PATH;

				break;
			}
			//dims
			case 1: {
				pfc::string8 str(imagefilerow->second.at(1));
				TCHAR outBuffer[MAX_PATH + 1] = {};
				pfc::stringcvt::convert_utf8_to_wide(outBuffer, MAX_PATH,
					str.get_ptr(), str.get_length());
				_tcscpy_s(pLvdi->item.pszText, MAX_PATH/*pLvdi->item.cchTextMax*/, const_cast<TCHAR*>(outBuffer));
				pLvdi->item.cchTextMax = MAX_PATH;

				break;
			}
			//file size
			case 2: {
				pfc::string8 str(imagefilerow->second.at(2));
				TCHAR outBuffer[MAX_PATH + 1] = {};
				pfc::stringcvt::convert_utf8_to_wide(outBuffer, MAX_PATH,
					str.get_ptr(), str.get_length());
				_tcscpy_s(pLvdi->item.pszText, MAX_PATH/*pLvdi->item.cchTextMax*/, const_cast<TCHAR*>(outBuffer));
				pLvdi->item.cchTextMax = MAX_PATH;

				break;
			}
			default: {
				int debug = 1;
			}
			}
		}

		if (lvitem->mask & LVIF_IMAGE) {
			if (pLvdi->item.iSubItem == 0) {
				HWND file_art_list = wnd;

				getimages_file_it imgfileinfo;

				size_t pos = lvitem->iItem;
				size_t ndx_pos = m_coord.GetLvFileArtRow(lvitem->iItem, imgfileinfo);
				
				if (ndx_pos == pfc_infinite) return 0;

				CImageList testlist;
				testlist = ListView_GetImageList(file_art_list, LVSIL_NORMAL);

				int c = !testlist ? -1 : testlist.GetImageCount();

				CImageList testlist_small;
				testlist_small = ListView_GetImageList(file_art_list, LVSIL_SMALL);
				int csmall = !testlist_small ? -1 : testlist_small.GetImageCount();

				DWORD dwView = ListView_GetView(wnd);
				bool bTile = dwView == LV_VIEW_TILE;

				if (bTile) {
					if (testlist && testlist.GetImageCount() /*&& ndxpos <= testlist.GetImageCount()*/) {
						pLvdi->item.iImage = ndx_pos;
					}
				}
				else {
					if (testlist_small && testlist_small.GetImageCount() /*&& ndxpos <= testlist_small.GetImageCount()*/) {
						pLvdi->item.iImage = ndx_pos;
					}
				}
			}
		}
	}
	return 0;
}

size_t CTrackMatchingDialog::get_art_perm_selection(HWND list, bool flagselected, const size_t max_items, pfc::array_t<t_uint8>& outmask, bit_array_bittable& are_albums) {

	//PFC_ASSERT(vRows() <= max_items);

	size_t selcount = 0;
	pfc::array_t<t_uint8> perm_selection;
	perm_selection.resize(outmask.size());

	for (size_t i = 0; i < outmask.size(); i++) {

		lv_art_param lv_param(i, m_coord.GetSourceTypeAtPos(i));
		are_albums.set(i, lv_param.is_album());

		getimages_it imginfo;

		var_it_t out;
		size_t ndxpos = m_coord.Get_V_LvRow(lsmode::art, true, i, out);
		imginfo = std::get<2>(*out).second;

		if (ListView_IsItemSelected(list, i)) {
			perm_selection[i] = static_cast<t_uint8>(ndxpos);
			selcount++;
		}
		else {
			if (flagselected) perm_selection[i] = static_cast<t_uint8>(max_items);
		}
	}
	for (size_t i = 0; i < outmask.size(); i++) {
		outmask[i] = perm_selection[i];
	}
	return selcount;
}

void CTrackMatchingDialog::attrib_menu_command(HWND wnd, af att_album, af att_art,
	UINT att_id, lsmode mode) {

	HWND wndlist = wnd;

	uartwork* uart = m_coord.GetUartwork();

	size_t citems = ListView_GetItemCount(wndlist);
	bool bselection = (ListView_GetSelectedCount(wndlist) > 0);
	bool is_files = wndlist == uGetDlgItem(IDC_FILE_LIST);

	pfc::array_t<t_uint8> perm_selection;
	perm_selection.resize(citems);
	bit_array_bittable are_albums(citems);

	const size_t max_items = m_tag_writer->get_art_count();
	const size_t cAlbumArt = m_tag_writer->release->images.get_count();

	size_t csel = get_art_perm_selection(wndlist, true, max_items, perm_selection, are_albums);

	bool mixedvals = false, valchange = false;
	size_t firstval = ~0;
	for (size_t i = 0; i < citems; i++) {
		bool bselected = perm_selection[i] != max_items; //selected flagged with citems value
		if (bselected) {
			t_uint8 dc_ndx = perm_selection[i];
			af att = are_albums.get(i) ? att_album : att_art;
			dc_ndx = are_albums.get(i) ? dc_ndx : dc_ndx - cAlbumArt;
			bool val = uart->getflag(att, dc_ndx);
			if (firstval == ~0) firstval = !val;
			uart->setflag(att, dc_ndx, firstval);
			
			ListView_RedrawItems(wnd, i, i);

			valchange |= val;
			if (valchange != val) mixedvals = true;
		}
	}

	CONFARTWORK = uartwork(*uart);
}

bool CTrackMatchingDialog::track_url_context_menu(HWND wnd, LPARAM lParamPos) {

	POINT point;
	point.x = MAKEPOINTS(lParamPos).x;
	point.y = MAKEPOINTS(lParamPos).y;

	file_info_impl finfo;
	metadb_handle_ptr item = m_tag_writer->finfo_manager->items[0];
	item->get_info(finfo);
		
	pfc::string8 local_release_id;
	
	const char* ch_local_rel = finfo.meta_get("DISCOGS_RELEASE_ID", 0);
	if (ch_local_rel) {
		local_release_id = ch_local_rel;
	}

	pfc::string8 discogs_release_id(m_tag_writer->release->id);
	pfc::string8 master_release_id(m_tag_writer->release->master_id);
	pfc::string8 artist_id(m_tag_writer->release->artists[0]->full_artist->id);

	bool hasRelease = discogs_release_id.get_length();
	bool hasMasterRelease = master_release_id.get_length();
	bool hasArtist = artist_id.get_length();

	bool diff_ids = local_release_id.get_length() && (!STR_EQUAL(discogs_release_id, local_release_id));

	pfc::string8 discogs_release_info (diff_ids ? "Discogs Release Id: " : "Release Id: ");
	pfc::string8 local_release_info;
	discogs_release_info << discogs_release_id;
	if (local_release_id.get_length())
		local_release_info << "Files Release Id: " << local_release_id;

	try {

		enum { ID_URL_DC_INFO = 1, ID_URL_LC_INFO, ID_URL_RELEASE, ID_URL_MASTER_RELEASE, ID_URL_ARTIST };
		HMENU menu = CreatePopupMenu();
		uAppendMenu(menu, MF_STRING | MF_DISABLED, ID_URL_DC_INFO , discogs_release_info);

		if (diff_ids)
			uAppendMenu(menu, MF_STRING | MF_DISABLED, ID_URL_LC_INFO, local_release_info);

		uAppendMenu(menu, MF_SEPARATOR, 0, 0);
		uAppendMenu(menu, MF_STRING | (!hasRelease ? MF_DISABLED : 0), ID_URL_RELEASE , "View Release Page");
		uAppendMenu(menu, MF_STRING | (!hasMasterRelease ? MF_DISABLED : 0), ID_URL_MASTER_RELEASE , "View Master Release Page");
		uAppendMenu(menu, MF_STRING | (!hasArtist ? MF_DISABLED : 0), ID_URL_ARTIST , "View Artist Page");

		POINT scr_point = point;
		::ClientToScreen(m_hWnd, &scr_point);

		int cmd = TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, scr_point.x, scr_point.y, 0, core_api::get_main_window(), 0);
		DestroyMenu(menu);
		pfc::string8 url;

		switch (cmd)
		{
		case ID_URL_RELEASE:
		{
			url << "https://www.discogs.com/release/" << discogs_release_id;
			break;
		}
		case ID_URL_MASTER_RELEASE: {
			url << "https://www.discogs.com/master/" << master_release_id;
			break;
		}
		case ID_URL_ARTIST: {
			url << "https://www.discogs.com/artist/" << artist_id;
			break;
		}
		}
		if (url.get_length())
			display_url(url);
	}
	catch (...) {};

	return true;
}

bool CTrackMatchingDialog::track_context_menu(HWND wnd, LPARAM lParamCoords) {
	
	POINT point;

	point.x = GET_X_LPARAM(lParamCoords);
	point.y = GET_Y_LPARAM(lParamCoords);
	
	if (point.x == -1 || point.y == -1) {
		CRect rect;
		CWindow(wnd).GetWindowRect(&rect);
		point = rect.CenterPoint();
	}
	
	try {
		HMENU menu = CreatePopupMenu();
		HMENU _childmenuDisplay = CreatePopupMenu();
		MENUITEMINFO mi1 = { 0 };
		mi1.cbSize = sizeof(MENUITEMINFO);
		mi1.fMask = MIIM_SUBMENU | MIIM_STRING | MIIM_ID;
		mi1.wID = ID_SUBMENU_SELECTOR;
		mi1.hSubMenu = _childmenuDisplay;
		mi1.dwTypeData = _T("Column Alignment");

		CListControlOwnerData* ilist = nullptr; bool is_ilist = false;
		if (wnd == uGetDlgItem(IDC_UI_LIST_DISCOGS) || wnd == uGetDlgItem(IDC_UI_LIST_FILES)) {
			is_ilist = true;
			ilist = (wnd == uGetDlgItem(IDC_UI_LIST_DISCOGS)) ? &m_idc_list : &m_ifile_list;
		}
		
		int csel = is_ilist ? ilist->GetSelectedCount()
			: ListView_GetSelectedCount(wnd);

		int citems = is_ilist ? ilist->GetItemCount()
			: ListView_GetItemCount(wnd);

		bit_array_bittable selmask(0);
		
		if (is_ilist) selmask = ilist->GetSelectionMask();
		else {
			selmask.resize(citems);
			size_t n = ListView_GetFirstSelection(wnd) == -1 ? pfc_infinite : ListView_GetFirstSelection(wnd);
			for (size_t i = n; i < citems; i++) {
				selmask.set(i, ListView_IsItemSelected(wnd, i));
			}
		}

		bool is_files = wnd == uGetDlgItem(IDC_FILE_LIST) || wnd == uGetDlgItem(IDC_UI_LIST_FILES);
		
		uAppendMenu(menu, MF_STRING
			| (citems? 0 : MF_DISABLED), ID_ART_TOOGLE_TRACK_ART_MODES,
			GetMode() == lsmode::default? "View Artwork\tCtrl+T" : "View Tracks\tCtrl+T");
		uAppendMenu(menu, MF_SEPARATOR, 0, 0);
		uAppendMenu(menu, MF_STRING | (citems? 0 : MF_DISABLED), ID_SELECT_ALL, "Select all\tCtrl+A");
		uAppendMenu(menu, MF_STRING | (csel? 0 : MF_DISABLED), ID_INVERT_SELECTION, "Invert selection\tCtrl+Shift+A");
		uAppendMenu(menu, MF_STRING | (csel? 0 : MF_DISABLED), ID_REMOVE, "Remove\tDel");
		uAppendMenu(menu, MF_STRING | (csel? 0 : MF_DISABLED), ID_CROP, "Crop\tCtrl+C");

		presenter* pres;
		std::pair<size_t, presenter*> icol_hit = m_coord.HitUiTest(point);
		if (icol_hit.first != pfc_infinite) {

			size_t fmt = m_coord.GetUiColumnFormat(icol_hit.first, icol_hit.second);

			uAppendMenu(menu, MF_SEPARATOR, 0, 0);
			// display submenu
			DWORD dwStyle = ::SendMessage(wnd, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
			uAppendMenu(_childmenuDisplay, MF_STRING | (fmt == HDF_LEFT ? MF_CHECKED | MF_DISABLED : 0), ID_LEFT_ALIGN, "Left");
			uAppendMenu(_childmenuDisplay, MF_STRING | (fmt == HDF_CENTER ? MF_CHECKED | MF_DISABLED : 0), ID_CENTER_ALIGN, "Center");
			uAppendMenu(_childmenuDisplay, MF_STRING | (fmt == HDF_RIGHT ? MF_CHECKED | MF_DISABLED : 0), ID_RIGHT_ALIGN, "Right");
			InsertMenuItem(menu, ID_SUBMENU_SELECTOR, true, &mi1);
		}

		//uAppendMenu(menu, MF_SEPARATOR, 0, 0);
		
		// display submenu
		//DWORD dwStyle = ::SendMessage(wnd, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
		//uAppendMenu(_childmenuDisplay, MF_STRING, ID_RESET_COLUMNS, "Reset Columns");
		//uAppendMenu(_childmenuDisplay, MF_STRING| (dwStyle & LVS_EX_GRIDLINES ? MF_CHECKED : 0), ID_SHOW_GRID, "Show Grid");
		//if (is_files)
		//	uAppendMenu(_childmenuDisplay, MF_STRING | (citems ? 0 : MF_DISABLED), ID_LAYOUT_CENTER, "Center Align");

		//InsertMenuItem(menu, ID_SUBMENU_SELECTOR, true, &mi1);		
		
		if (GetMode() == lsmode::art) {
			if (!is_files) {	
				//attribs...
				append_art_context_menu(wnd, &menu);
				uAppendMenu(menu, MF_SEPARATOR, 0, 0);
				uAppendMenu(menu, MF_STRING | (/*count_sel == 1 &&*/ GetMode() == lsmode::art ? 0 : MF_DISABLED), ID_ART_PREVIEW, "Preview\tCtrl+P");
			}
			else {				
				//image viewer req >= 1.6.2
				bool b_ver_ok = core_version_info_v2::get()->test_version(1, 6, 2, 0);
				if (b_ver_ok) {
					uAppendMenu(menu, MF_SEPARATOR, 0, 0);
					uAppendMenu(menu, MF_STRING | (GetMode() == lsmode::art ? 0 : MF_DISABLED), ID_ART_IMAGE_VIEWER, "Image Viewer\tDouble Click");
				}
			}
		}

		//uAppendMenu(menu, MF_STRING | (citems ? 0 : MF_DISABLED), ID_ART_RESET, "Reset");

		int cmd = TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, point.x, point.y, 0, wnd, 0);
		DestroyMenu(menu);

		switch_context_menu(wnd, point, is_files, cmd, selmask, ilist);

	}
	catch (...) {}
	return false;
}

bool CTrackMatchingDialog::switch_context_menu(HWND wnd, POINT point, bool is_files, int cmd, bit_array_bittable selmask, CListControlOwnerData* ilist) {

	UINT att_id = 0;
	af att_album;
	af att_art;

	lsmode mode = GetMode();

	switch (cmd)
	{
	case ID_ART_TOOGLE_TRACK_ART_MODES: {
		uButton_SetCheck(m_hWnd, IDC_TRACK_MATCH_ALBUM_ART, GetMode() == lsmode::default);
		::SendMessage(m_hWnd, WM_COMMAND, MAKEWPARAM(IDC_TRACK_MATCH_ALBUM_ART, BN_CLICKED), 0);
		return true;
	}
	case ID_SELECT_ALL:
	{
		if (ilist)
			ilist->SelectAll();
		else
			select_all_items(wnd);

		return true;
	}
	case ID_INVERT_SELECTION:
	{
		if (ilist) {
			for (size_t i = 0; i < selmask.size(); i++) selmask.set(i, !selmask.get(i));
			ilist->SetSelection(bit_array_true(), selmask);
		}
		else
			invert_selected_items(wnd);
		return true;
	}
	case ID_REMOVE:
	{
		bool bcrop = false;
		const size_t count = selmask.size();
		bit_array_bittable delmask(count);
		for (size_t i = 0; i < count; i++) {
			if (bcrop)
				delmask.set(i, !selmask.get(i));
			else
				delmask.set(i, selmask.get(i));
		}

		pfc::array_t<t_uint8> perm_selection;
		bit_array_bittable are_albums;
		if (GetMode() == lsmode::art && !is_files) {
			perm_selection.resize(count);
			are_albums.resize(count);
			const size_t max_items = m_tag_writer->get_art_count();
			size_t csel = get_art_perm_selection(wnd, true, max_items, perm_selection, are_albums);		
		}

		if (ilist) wnd = ilist == &m_idc_list ? uGetDlgItem(IDC_UI_LIST_DISCOGS) : uGetDlgItem(IDC_UI_LIST_FILES);

		//forward command...
		m_coord.ListUserCmd(wnd, GetMode(), ID_REMOVE, delmask, are_albums, false);

		match_message_update(match_manual);
		if (ilist) ilist->OnItemsRemoved(selmask, selmask.size());
		return true;
	}
	case ID_CROP:
	{
		for (size_t i = 0; i < selmask.size(); i++) {
				selmask.set(i, !selmask.get(i));
		}
		if (ilist) wnd = ilist == &m_idc_list ? uGetDlgItem(IDC_UI_LIST_DISCOGS) : uGetDlgItem(IDC_UI_LIST_FILES);

		bit_array_bittable are_albums;
		if (GetMode() == lsmode::art && !is_files) {	
            //todo: revision of perm_selections
			pfc::array_t<t_uint8> x_selection;
			x_selection.resize(selmask.size());
			are_albums.resize(selmask.size());
			const size_t max_items = m_tag_writer->get_art_count();
			size_t csel = get_art_perm_selection(wnd, true, max_items, x_selection, are_albums);
		}

		m_coord.ListUserCmd(wnd, GetMode(), ID_REMOVE, selmask, are_albums, true);

		match_message_update(match_manual);
		if (ilist) ilist->OnItemsRemoved(selmask, selmask.size());
		return true;
	}
	case ID_LEFT_ALIGN: {
		presenter* pres;
		std::pair<size_t, presenter*> icol_hit = m_coord.HitUiTest(point);
		m_coord.SetUiColumnFormat(icol_hit.first, icol_hit.second, HDF_LEFT);
		return true;
	}
	case ID_CENTER_ALIGN:	{
		presenter* pres;
		std::pair<size_t, presenter*> icol_hit = m_coord.HitUiTest(point);
		m_coord.SetUiColumnFormat(icol_hit.first, icol_hit.second, HDF_CENTER);
		return true;
	}
	case ID_RIGHT_ALIGN: {
		presenter* pres;
		std::pair<size_t, presenter*> icol_hit = m_coord.HitUiTest(point);
		m_coord.SetUiColumnFormat(icol_hit.first, icol_hit.second, HDF_RIGHT);
		return true;
	}
	case ID_ART_PREVIEW: {
		HWND wndlist = wnd;
		bool is_files = wnd == uGetDlgItem(IDC_FILE_LIST);
		size_t citems = ListView_GetItemCount(wndlist);

		if (is_files || !citems) return true;
		
		pfc::array_t<t_uint8> perm_selection;
		perm_selection.resize(citems);
		bit_array_bittable are_albums(citems);

		const size_t max_items = m_tag_writer->get_art_count();
		size_t csel = get_art_perm_selection(wndlist, true, max_items, perm_selection, are_albums);

		size_t chk = perm_selection[0];

		for (size_t i = 0; i < citems; i++) {
			if (perm_selection[i] != max_items) {		//selected?
				size_t album_artist_ndx = perm_selection[i];
				if (!are_albums.get(i)) {			//artist art?
					album_artist_ndx -= m_tag_writer->release->images.get_count();
				}

				request_preview(album_artist_ndx, !are_albums.get(i), false);

			}
		}
		return true;
	}
	case ID_ART_IMAGE_VIEWER: {
		//image viewer req >= 1.6.2
		bool b_ver_ok = core_version_info_v2::get()->test_version(1, 6, 2, 0);
		if (b_ver_ok) {
			size_t first_sel = selmask.find_first(true, 0, selmask.size());
			getimages_file_it out_art_file_it;
			m_coord.GetFileArtAtLvPos(first_sel, out_art_file_it);
			ndx_image_file_t ndx_img = out_art_file_it->first;
			service_ptr_t<fb2k::imageViewer> img_viewer = fb2k::imageViewer::get();
			img_viewer->load_and_show(m_hWnd,
				m_tag_writer->finfo_manager->items,
				ndx_img.second, 0);
		}
		break;
	}
	// artwork
	case ID_ART_OVR:
		att_album = af::alb_ovr;
		att_art = af::art_ovr;
		att_id = ID_ART_OVR;
		attrib_menu_command(wnd, att_album, att_art,
			att_id, mode);
		break;
	case ID_ART_EMBED:
		att_album = af::alb_emb;
		att_art = af::art_emb;
		att_id = ID_ART_EMBED;
		attrib_menu_command(wnd, att_album, att_art,
			att_id, mode);
		break;
	case ID_ART_WRITE:
		att_album = af::alb_sd;
		att_art = af::art_sd;
		att_id = ID_ART_WRITE;
		attrib_menu_command(wnd, att_album, att_art,
			att_id, mode);
		break;
	case ID_ART_TILE:
		ListView_SetView(wnd, LV_VIEW_TILE);
		break;
	case ID_ART_DETAIL:
		ListView_SetView(wnd, LV_VIEW_DETAILS);
		break;
	case ID_ART_RESET: {
		if (ilist) {
			int oldcountdc = m_coord.GetDiscogsTrackUiLvSize();
			int oldcountfiles = m_coord.GetFileTrackUiLvSize();
			m_coord.Reset(uGetDlgItem(IDC_UI_LIST_DISCOGS), GetMode());
			m_coord.Reset(uGetDlgItem(IDC_UI_LIST_FILES), GetMode());
			
			m_idc_list.OnItemsRemoved(bit_array_true(), oldcountdc);
			m_ifile_list.OnItemsRemoved(bit_array_true(), oldcountfiles);

			m_coord.Show(true, true);

			m_ifile_list.ReloadItems(bit_array_true());
			m_ifile_list.ReloadItems(bit_array_true());
		}
		else {
			m_coord.Reset(wnd, GetMode());
			m_coord.Show(!is_files, is_files);	
		}
		return true;
	}
	}
	return false;
}

bool CTrackMatchingDialog::append_art_context_menu(HWND wndlist, HMENU* menu) {
	
	lsmode mode = GetMode();

	if (mode != lsmode::art)
		return false;

	size_t citems = ListView_GetItemCount(wndlist);
	bool bselection = (ListView_GetSelectedCount(wndlist) > 0);
	bool is_files = wndlist == uGetDlgItem(IDC_FILE_LIST);
	bool is_tile = (DWORD)ListView_GetView(wndlist) == LV_VIEW_TILE;

	try {
		
		uartwork* uartw = m_coord.GetUartwork();  //get_listview_uartwork(wndlist, coord.lstate_backup[lsmode::art]);

		pfc::array_t<t_uint8> perm_selection;
		perm_selection.resize(citems);
		bit_array_bittable are_albums(citems);

		const size_t max_items = citems;

		size_t csel = get_art_perm_selection(wndlist, true, max_items, perm_selection, are_albums);

		bool mixedwrite = false, write = false;

		for (size_t i = 0; i < citems; i++) {
			if (perm_selection[i] != max_items) {
				size_t dc_ndx = perm_selection[i];
				af att = are_albums.get(i) ? af::alb_sd : af::art_sd;
				dc_ndx = are_albums.get(i) ? dc_ndx : dc_ndx - cAlbumArt;
				bool val = uartw->getflag(att, dc_ndx);
				write |= val;
				if (write != val) mixedwrite = true;			
			}
		}
		bool mixedovr = false, ovr = false;
		for (size_t i = 0; i < citems; i++) {
			if (perm_selection[i] != citems) {
				size_t dc_ndx = perm_selection[i];
				af att = are_albums.get(i) ? af::alb_ovr : af::art_ovr;
				dc_ndx = are_albums.get(i) ? dc_ndx : dc_ndx - cAlbumArt;
				bool val = uartw->getflag(att, dc_ndx);
				ovr |= val;
				if (ovr != val) mixedovr = true;
			}
		}
		bool mixedembed = false, embed = false;
		for (size_t i = 0; i < citems; i++) {
			if (perm_selection[i] != citems) {
				size_t dc_ndx = perm_selection[i];
				af att = are_albums.get(i) ? af::alb_emb : af::art_emb;
				dc_ndx = are_albums.get(i) ? dc_ndx : dc_ndx - cAlbumArt;
				bool val = uartw->getflag(att, dc_ndx);
				embed |= val;
				if (write != val) mixedembed = true;
			}
		}
		bool bitems = (ListView_GetItemCount(wndlist) > 0);
		uAppendMenu(*menu, MF_SEPARATOR, (bselection ? 0 : MF_DISABLED), 0);
		uAppendMenu(*menu, MF_STRING | (bselection ? 0 : MF_DISABLED) | (write ? mixedwrite? MF_CHECKED | MF_UNCHECKED : MF_CHECKED : 0), ID_ART_WRITE, "Write\tCtrl+W");
		uAppendMenu(*menu, MF_STRING | (bselection ? 0 : MF_DISABLED) | (ovr ? mixedovr? MF_CHECKED | MF_UNCHECKED : MF_CHECKED : 0), ID_ART_OVR, "Overwrite\tCtrl+O");
		uAppendMenu(*menu, MF_STRING | (bselection ? 0 : MF_DISABLED) | (embed ? mixedembed? MF_CHECKED | MF_UNCHECKED : MF_CHECKED : 0), ID_ART_EMBED, "Embed\tCtrl+E");
		//uAppendMenu(*menu, MF_SEPARATOR, (bitems ? 0 : MF_DISABLED), 0);
		//uAppendMenu(*menu, MF_STRING | (bitems ? 0 : write && !mixedwrite ? MF_CHECKED | MF_DISABLED : MF_DISABLED), ID_ART_WRITE_ALL, "Write All\tShift+Ctrl+W");
		//uAppendMenu(*menu, MF_STRING | (bitems ? 0 : ovr && !mixedovr ? MF_CHECKED | MF_DISABLED : MF_DISABLED), ID_ART_OVR_ALL, "Overwrite All\tShift+Ctrl+O");
		//uAppendMenu(*menu, MF_STRING | (bitems ? 0 : embed && !mixedembed ? MF_CHECKED | MF_DISABLED : MF_DISABLED), ID_ART_EMBED_ALL, "Embed All\tShift+Ctrl+E");
		uAppendMenu(*menu, MF_STRING | (bitems ? 0 : MF_DISABLED), ID_ART_RESET, "Reset\tCtrl+R");
		uAppendMenu(*menu, MF_SEPARATOR, (bitems ? 0 : MF_DISABLED), 0);
		//uAppendMenu(*menu, MF_STRING | (bitems ? 0 : MF_DISABLED), ID_ART_LIST, "List");
		uAppendMenu(*menu, MF_STRING | (is_tile ? MF_CHECKED : 0) | (bitems && !is_tile ? 0 : MF_DISABLED), ID_ART_TILE, "Tile");
		uAppendMenu(*menu, MF_STRING | (!is_tile ? MF_CHECKED : 0) | (bitems && is_tile ? 0 : MF_DISABLED), ID_ART_DETAIL, "Detail");
	}
	catch (...) {}
	return false;
}

LRESULT CTrackMatchingDialog::OnListRClick(LPNMHDR lParam) {
	HWND wnd = ((LPNMHDR)lParam)->hwndFrom;
	NMITEMACTIVATE* info = reinterpret_cast<NMITEMACTIVATE*>(lParam);
	POINT coords; GetCursorPos(&coords);
	track_context_menu(wnd, MAKELPARAM(coords.x, coords.y));
	return FALSE;
}

LRESULT CTrackMatchingDialog::OnListDBLClick(LPNMHDR lParam) {
	HWND wnd = ((LPNMHDR)lParam)->hwndFrom;
	NMITEMACTIVATE* info = reinterpret_cast<NMITEMACTIVATE*>(lParam);
	POINT coords; GetCursorPos(&coords);

	int citems = ListView_GetItemCount(wnd);
	bit_array_bittable selmask(citems);
	size_t n = ListView_GetFirstSelection(wnd) == -1 ? pfc_infinite : ListView_GetFirstSelection(wnd);
	selmask.set(info->iItem, true);

	bool is_art_files = wnd == uGetDlgItem(IDC_FILE_LIST);
	if (is_art_files) {
		switch_context_menu(m_hWnd, coords, true, ID_ART_IMAGE_VIEWER, selmask, nullptr);
	}
	return FALSE;
}

LRESULT CTrackMatchingDialog::OnRButtonUp(UINT ctrl_id, WPARAM p_wp, LPARAM lParam, BOOL& bHandled) {

	UINT uid = 0;
	POINT point;

	point.x = MAKEPOINTS(lParam).x;
	point.y = MAKEPOINTS(lParam).y;

	
	bool ok_hit = false;

	UINT uid_release_description = IDC_STATIC_MATCH_TRACKING_REL_NAME;
	UINT uid_hit = uid_release_description;
	HWND wnd_ctrl = uGetDlgItem(uid_hit);
	CRect rc;

	if (!::GetWindowRect(wnd_ctrl, &rc))
		ok_hit = false;
	else {
		ScreenToClient(&rc);
		if (PtInRect(rc, point))
			ok_hit = true;
	}

	if (ok_hit && uid_hit == uid_release_description) {

		track_url_context_menu(m_hWnd, lParam);

		bHandled = TRUE;
		return 0;
	}

	bHandled = FALSE;
	return 0;
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
	
	pfc::string8 dlg_release_id, dlg_artist_id, dlg_artist;
	pfc::string8 item_release_id, item_artist_id, item_artist;
	
	metadb_handle_list items = m_tag_writer->finfo_manager->items;
    //item release & artist
	if (m_tag_writer->finfo_manager->items.get_size()) {
		file_info_impl finfo;
		metadb_handle_ptr item = items[0];
		item->get_info(finfo);
		g_discogs->file_info_get_tag(item, finfo, TAG_RELEASE_ID, item_release_id);
		g_discogs->file_info_get_tag(item, finfo, TAG_ARTIST_ID, item_artist_id);
		if (!item_artist_id.get_length()) {
			item_artist.set_string(file_info_get_artist_name(finfo, item));
		}
	}
	//dlg release & artist
	dlg_release_id = m_tag_writer->release->id;
	dlg_artist_id = m_tag_writer->release->artists[0]->full_artist->id;
	dlg_artist = m_tag_writer->release->artists[0]->full_artist->name;

	CFindReleaseDialog* find_dlg = reinterpret_cast<CFindReleaseDialog*>(g_discogs->find_release_dialog);
	find_dlg->show();

	HWND wndFindRelease = g_discogs->find_release_dialog->m_hWnd;
	UINT default = IDC_PROCESS_RELEASE_BUTTON;	//IDC_PROCESS_RELEASE_BUTTON	//IDC_FILTER_EDIT	//IDC_SEARCH_BUTTON
	
	if (dlg_release_id.get_length()) {
		
		find_dlg->setitems(items);

		HWND release_url_edit = ::GetDlgItem(wndFindRelease, IDC_RELEASE_URL_TEXT);

		uSetWindowText(release_url_edit, dlg_release_id.get_ptr());

		if (!dlg_artist_id.get_length()) {
			default = IDC_SEARCH_BUTTON;
			HWND search_edit = ::GetDlgItem(wndFindRelease, IDC_SEARCH_TEXT);
			uSetWindowText(search_edit, dlg_artist.get_ptr());
		}
	}

	uSendMessage(wndFindRelease, WM_NEXTDLGCTL, (WPARAM)(HWND)::GetDlgItem(wndFindRelease, default), TRUE);

	DestroyWindow();

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

void CTrackMatchingDialog::destroy_all() {
	if (!multi_mode) {
		CFindReleaseDialog* dlg = reinterpret_cast<CFindReleaseDialog*>(g_discogs->find_release_dialog);
		dlg->destroy();
	}
	destroy();
}

LRESULT CTrackMatchingDialog::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	m_coord.PullConf(GetMode(), true, &m_conf);
	m_coord.PullConf(GetMode(), false, &m_conf);
	pushcfg();

	return 0;
}
