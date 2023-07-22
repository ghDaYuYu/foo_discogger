#include "stdafx.h"
#include "resource.h"
#include <gdiplus.h>
#include <filesystem> //makeBuffer

#include "SDK\imageViewer.h"

#include "tasks.h"
#include "utils.h"
#include "utils_menu.h"
#include "multiformat.h"
#include "foo_discogs.h"
#include "file_info_manager.h"
#include "tag_writer.h"

#include "track_matching_utils.h"

#include "preview_dialog.h"
#include "track_matching_dialog.h"

#define ENCODE_DISCOGS(d,t)		((d==-1||t==-1) ? -1 : ((d<<16)|t))
#define DECODE_DISCOGS_DISK(i)	((i==-1) ? -1 : (i>>16))
#define DECODE_DISCOGS_TRACK(i)	((i==-1) ? -1 : (i & 0xFFFF))

// {F4939F4B-0B02-4B5C-8BD3-FACB68775A35}
static const GUID guid_cfg_dialog_position_track_matching_dlg = { 0xf4939f4b, 0xb02, 0x4b5c, { 0x8b, 0xd3, 0xfa, 0xcb, 0x68, 0x77, 0x5a, 0x35 } };
static cfgDialogPosition cfg_dialog_position_track_matching_dlg(guid_cfg_dialog_position_track_matching_dlg);

//private
void CTrackMatchingDialog::add_pending_previews(size_t n) {

	m_pending_previews += n;

}
//public
void CTrackMatchingDialog::pending_previews_done(size_t n) {

	{
		std::lock_guard<std::mutex> guard(m_mx_pending_previews_mod);
		m_pending_previews -= n;
	}

	if (!m_pending_previews) {

		ShowWindow(SW_SHOW);
		m_coord.Invalidate();
	}
}

LRESULT CTrackMatchingDialog::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {

	showtitle();
	SetIcon(g_discogs->icon);
	DlgResize_Init(mygripp.enabled, true);

	::ShowWindow(uGetDlgItem(IDC_BTN_WRITE_TAGS), true);
	if (!m_tag_writer->GetRelease()) {
		::EnableWindow(uGetDlgItem(IDC_BTN_WRITE_TAGS), false);
	}
	::ShowWindow(uGetDlgItem(IDC_BTN_PREVIEW_TAGS), true);
	::ShowWindow(uGetDlgItem(IDC_BTN_PREVIOUS), false);
	::ShowWindow(uGetDlgItem(IDC_BTN_NEXT), false);
	::ShowWindow(uGetDlgItem(IDC_BTN_SKIP), false);
	::ShowWindow(uGetDlgItem(IDCANCEL), true);
	::ShowWindow(uGetDlgItem(IDC_BTN_BACK), true);
	::ShowWindow(uGetDlgItem(IDC_SPLIT_BTN_TAG_CAT_CREDIT), false);
	HWND hwndControl = GetDlgItem(IDC_BTN_PREVIEW_TAGS);
	SendMessage(m_hWnd, WM_NEXTDLGCTL, (WPARAM)hwndControl, TRUE);

	HWND discogs_track_list = uGetDlgItem(IDC_UI_DC_ARTWORK_LIST);
	HWND file_list = uGetDlgItem(IDC_UI_FILE_ARTWORK_LIST);
	HWND write_btn = GetDlgItem(IDC_BTN_WRITE_TAGS);
	::SendMessageW(write_btn, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)g_hIcon_rec);
	if (m_tag_writer) {

		CONF_MULTI_ARTWORK = multi_uartwork(m_conf, m_tag_writer->GetRelease());

		match_message_update();

		m_idc_list.CreateInDialog(*this, IDC_UI_LIST_DISCOGS);
		m_idc_list.InitializeHeaderCtrl(HDS_DRAGDROP);
		m_idc_list.SetRowStyle(m_conf.list_style);

		m_ifile_list.CreateInDialog(*this, IDC_UI_LIST_FILES);
		m_ifile_list.InitializeHeaderCtrl(HDS_DRAGDROP);
		m_ifile_list.SetRowStyle(m_conf.list_style);

		m_ida_list.CreateInDialog(*this, IDC_UI_DC_ARTWORK_LIST);
		m_ida_list.InitializeHeaderCtrl(HDS_DRAGDROP);
		m_ida_list.SetRowStyle(m_conf.list_style);

		m_ifa_list.CreateInDialog(*this, IDC_UI_FILE_ARTWORK_LIST);
		m_ifa_list.InitializeHeaderCtrl(HDS_DRAGDROP);
		m_ifa_list.SetRowStyle(m_conf.list_style);

		m_coord.Initialize(m_hWnd, m_conf);
		m_coord.SetTagWriter(m_tag_writer);

		m_coord.InitFormMode(lsmode::tracks_ui, IDC_UI_LIST_DISCOGS, IDC_UI_LIST_FILES);
		m_coord.InitFormMode(lsmode::art, IDC_UI_DC_ARTWORK_LIST, IDC_UI_FILE_ARTWORK_LIST);

		m_coord.SetMode(lsmode::tracks_ui);
		m_coord.Show();

		//default view (tracks, files)
		m_coord.InitUiList(m_idc_list.m_hWnd, lsmode::tracks_ui, true, &m_idc_list);
		m_coord.InitUiList(m_ifile_list.m_hWnd, lsmode::tracks_ui, false, &m_ifile_list);
		m_coord.InitUiList(m_ida_list.m_hWnd, lsmode::art, true, &m_ida_list);
		m_coord.InitUiList(m_ifa_list.m_hWnd, lsmode::art, false, &m_ifa_list);

		cfg_dialog_position_track_matching_dlg.AddWindow(m_hWnd);

		LibUIAsTrackList(true);

		::ShowWindow(uGetDlgItem(IDC_UI_DC_ARTWORK_LIST), SW_HIDE);
		::ShowWindow(uGetDlgItem(IDC_UI_FILE_ARTWORK_LIST), SW_HIDE);

		::ShowWindow(uGetDlgItem(IDC_UI_LIST_DISCOGS), SW_SHOW);
		::ShowWindow(uGetDlgItem(IDC_UI_LIST_FILES), SW_SHOW);

		init_track_matching_dialog();
		//darkmode
		AddDialog(m_hWnd);
		AddControls(m_hWnd);

		CustomFont(m_hWnd, HIWORD(m_conf.custom_font));

		int user_wants_skip = (int)HIWORD(m_conf.album_art_skip_default_cust);

		bool buser_skip_artwork = (user_wants_skip & ARTSAVE_SKIP_USER_FLAG) != 0;
		bool save_artwork = m_conf.save_album_art || m_conf.save_artist_art || m_conf.embed_album_art || m_conf.embed_artist_art;

		m_tristate.Init(buser_skip_artwork ? BST_CHECKED : BST_UNCHECKED, save_artwork);

		if (!buser_skip_artwork && save_artwork) {
			//;
		}
		if (m_tag_writer->m_match_status == MATCH_SUCCESS && (m_conf.skip_mng_flag & SkipMng::RELEASE_DLG_MATCHED)) {
			generate_track_mappings(m_tag_writer->m_track_mappings);
			try {
				service_ptr_t<generate_tags_task> task = new service_impl_t<generate_tags_task>(this, m_tag_writer, false);
				task->start();
			}
			catch (locked_task_exception e)
			{
				log_msg(e.what());
			}
			return FALSE;
		}

		HWND hwndControl = GetDlgItem(IDC_BTN_PREVIEW_TAGS);
		SendMessage(m_hWnd, WM_NEXTDLGCTL, (WPARAM)hwndControl, TRUE);

		size_t cartist_art = 0;

		for (auto wra : m_tag_writer->GetArtists()) {
			if (wra->full_artist) {
				cartist_art += wra->full_artist->images.get_count();
			}
		}

		if (CONF_MULTI_ARTWORK.isEmpty() || m_tag_writer->GetArtCount(art_src::alb) > kMax_Artwork ||
			cartist_art > kMax_Artwork) {

			HWND hwndBtn = uGetDlgItem(IDC_CHK_MNG_ARTWORK);
			::EnableWindow(hwndBtn, FALSE);

			m_tristate.Init(BST_CHECKED, FALSE);
			preview_job pj(false, 0, false, false, false);
			m_vpreview_jobs.emplace_back(pj);
		}
		else {

			bool get_mibs = !(m_conf.skip_mng_flag & SkipMng::BRAINZ_ID_FETCH);

			// * REQUEST 

			for (size_t it = 0; it < m_tag_writer->GetArtCount(art_src::alb); it++) {
				preview_job pj(false, it, false, it != 0, it == 0 && get_mibs);
				m_vpreview_jobs.emplace_back(pj);
			}
			size_t acc_ndx = 0;
			for (auto wra : m_tag_writer->GetArtists()) {
				for (size_t it = 0; it < wra->full_artist->images.get_count(); it++) {
					preview_job pj(false, acc_ndx + it, true, it != 0, false);
					m_vpreview_jobs.emplace_back(pj);
				}
				acc_ndx += wra->full_artist->images.get_count();
			}
			// 0 front, 1 back, 3 disk, 2 artist
			for (size_t i = 0; i < album_art_ids::num_types(); i++) {
				preview_job pj;
				if (i == 2) {
					pj =  preview_job(true, 3, false, true, false);					
				}
				else if (i == 3) {
					pj = preview_job(true, 2, false, true, false);
				}
				else {
					pj = preview_job(true, i, false, true, false);
				}
				m_vpreview_jobs.emplace_back(pj);
			}
		}

		m_pending_previews = m_vpreview_jobs.size();

		if (!m_pending_previews) {
			//no jobs to request, show dialog
			ShowWindow(SW_SHOW);
			return FALSE;
		}

		{
			std::lock_guard<std::mutex> lg(m_mx_pending_previews_mod);

			for (const auto& w : m_vpreview_jobs) {
				if (w.isfile) {
					request_file_preview(w.index_art, w.artist_art);
				}
				else {
					request_preview(w.index_art, w.artist_art, w.only_cache, w.get_mibs);
				}			
			}
		}
	}
	return FALSE;
}

LRESULT CTrackMatchingDialog::OnCheckSkipArtwork(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	m_tristate.SetBistate();
	return FALSE;
}

LRESULT CTrackMatchingDialog::OnCheckArtworkFileMatch(WORD wNotifyCode, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	bool bfile_match = (uButton_GetCheck(m_hWnd, IDC_CHK_ARTWORK_FILEMATCH) == TRUE);
	CONF_MULTI_ARTWORK.file_match = bfile_match;
	::EnableWindow(uGetDlgItem(IDC_UI_FILE_ARTWORK_LIST), bfile_match);
	return FALSE;
}

LRESULT CTrackMatchingDialog::OnCheckManageArtwork(WORD wNotifyCode, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	WPARAM wParam = MAKEWPARAM(0, wNotifyCode);

	if (wID == IDC_CHK_MNG_ARTWORK) {

		if (HIWORD(wParam) == BN_CLICKED) {

			bool bmanaged = (uButton_GetCheck(m_hWnd, IDC_CHK_MNG_ARTWORK) == TRUE);
			bool bfile_match = CONF_MULTI_ARTWORK.file_match;
			if (bmanaged) {

				LibUIAsTrackList(false);

				auto ida_cols = m_ida_list.GetColumnCount();

				::ShowWindow(uGetDlgItem(IDC_UI_DC_ARTWORK_LIST), SW_SHOW);
				::ShowWindow(uGetDlgItem(IDC_UI_FILE_ARTWORK_LIST), SW_SHOW);

				::EnableWindow(uGetDlgItem(IDC_UI_DC_ARTWORK_LIST), true);
				::EnableWindow(uGetDlgItem(IDC_UI_FILE_ARTWORK_LIST), bfile_match);

				::ShowWindow(uGetDlgItem(IDC_BTN_WRITE_ARTWORK), SW_SHOW);
				::ShowWindow(uGetDlgItem(IDC_CHK_ARTWORK_FILEMATCH), SW_SHOW);
				::ShowWindow(uGetDlgItem(IDC_BTN_WRITE_TAGS), SW_HIDE);
				::ShowWindow(uGetDlgItem(IDC_UI_LIST_DISCOGS), SW_HIDE);
				::ShowWindow(uGetDlgItem(IDC_UI_LIST_FILES), SW_HIDE);

				::EnableWindow(uGetDlgItem(IDC_UI_LIST_DISCOGS), false);
				::EnableWindow(uGetDlgItem(IDC_UI_LIST_FILES), false);
			}
			else {

				LibUIAsTrackList(true);
				::ShowWindow(uGetDlgItem(IDC_UI_LIST_DISCOGS), SW_SHOW);
				::ShowWindow(uGetDlgItem(IDC_UI_LIST_FILES), SW_SHOW);
				::EnableWindow(uGetDlgItem(IDC_UI_LIST_DISCOGS), true);
				::EnableWindow(uGetDlgItem(IDC_UI_LIST_FILES), true);

				::ShowWindow(uGetDlgItem(IDC_UI_DC_ARTWORK_LIST), SW_HIDE);
				::ShowWindow(uGetDlgItem(IDC_UI_FILE_ARTWORK_LIST), SW_HIDE);

				::EnableWindow(uGetDlgItem(IDC_UI_DC_ARTWORK_LIST), false);
				::EnableWindow(uGetDlgItem(IDC_UI_FILE_ARTWORK_LIST), false);

				::ShowWindow(uGetDlgItem(IDC_BTN_WRITE_ARTWORK), SW_HIDE);
				::ShowWindow(uGetDlgItem(IDC_CHK_ARTWORK_FILEMATCH), SW_HIDE);
				::ShowWindow(uGetDlgItem(IDC_BTN_WRITE_TAGS), SW_SHOW);
			}

			m_coord.SetMode(bmanaged ? lsmode::art : lsmode::tracks_ui);
			
			bool bfirstrun = m_coord.Show();

			::ShowScrollBar(uGetDlgItem(IDC_UI_DC_ARTWORK_LIST), SB_HORZ, FALSE);
		}
	}
	return FALSE;
}

void CTrackMatchingDialog::match_message_update(pfc::string8 local_msg) {

	HWND ctrl_match_msg = uGetDlgItem(IDC_STATIC_MATCH_TRACKS_MSG);
	pfc::string8 newmessage;

	int local_status;
	const int local_override = -100;
	if (local_msg.length() > 0)
		local_status = local_override;
	else
		local_status = m_tag_writer->m_match_status;

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
		uSetDlgItemText(m_hWnd, IDC_STATIC_MATCH_TRACKS_MSG, newmessage);
		::ShowWindow(ctrl_match_msg, newmessage.length() > 0);
	}
}

void CTrackMatchingDialog::request_preview(size_t img_ndx, bool artist_art, bool onlycache, bool get_mibs) {
	service_ptr_t<process_artwork_preview_callback> task =
		new service_impl_t<process_artwork_preview_callback>(this, m_tag_writer->GetRelease(), img_ndx, artist_art, onlycache, get_mibs);
	task->start(m_hWnd);
}

void CTrackMatchingDialog::request_file_preview(size_t img_ndx, bool artist_art) {
	service_ptr_t<process_file_artwork_preview_callback> file_art_task =
		new service_impl_t<process_file_artwork_preview_callback>(this,
			m_tag_writer->GetRelease(), m_tag_writer->m_finfo_manager->items, img_ndx, artist_art);
	file_art_task->start(m_hWnd);
}

void CTrackMatchingDialog::process_artwork_preview_done(size_t img_ndx, bool artist_art, MemoryBlock callback_small_art, musicbrainz_info musicbrainz_mibs) {
	if (!musicbrainz_mibs.empty()) {
		m_musicbrainz_mibs = musicbrainz_mibs;
	}

	HWND lvlist = uGetDlgItem(IDC_UI_DC_ARTWORK_LIST);
	art_src art_source = artist_art ? art_src::art : art_src::alb;
	bool res = m_coord.show_artwork_preview(img_ndx, art_source, callback_small_art);
	this->pending_previews_done(1);
}

void CTrackMatchingDialog::process_file_artwork_preview_done(size_t img_ndx, bool artist_art, imgpairs callback_pair_hbitmaps, std::pair<pfc::string8, pfc::string8> temp_file_names) {
	HWND lvlist = uGetDlgItem(IDC_UI_FILE_ARTWORK_LIST);
	art_src art_source = artist_art ? art_src::art : art_src::alb;

	bool res = m_coord.add_file_artwork_preview(img_ndx, art_source, callback_pair_hbitmaps, temp_file_names);
	this->pending_previews_done(1);
}

void CTrackMatchingDialog::process_download_art_paths_done(pfc::string8 callback_release_id,
	std::shared_ptr<std::vector<std::pair<pfc::string8, bit_array_bittable>>> vres,
	pfc::array_t<GUID> my_album_art_ids) {
	if (m_tag_writer->GetRelease()->id != callback_release_id || get_mode() != lsmode::art) {
		return;
	}

	bit_array_bittable saved_mask(my_album_art_ids.get_count());
	
	for (const auto& done_walk : *vres) {
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
	size_t cjobs = 0;

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
					cjobs++;
				}
			}
		}
	}

	{
		std::lock_guard<std::mutex> lg(m_mx_pending_previews_mod);
		add_pending_previews(cjobs);
	}

	size_t true_pos = refresh_mask.find_first(true, 0, refresh_mask.size());
	while (true_pos < refresh_mask.size()) {

		//note: dlg owns these processes
		request_file_preview(true_pos, refresh_mask_types.get(true_pos));
		true_pos = refresh_mask.find_first(true, true_pos + 1, refresh_mask.size());
	}
}

LRESULT CTrackMatchingDialog::OnColorStatic(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	
	if ((HWND)lParam == uGetDlgItem(IDC_STATIC_MATCH_TRACKS_MSG)) {

		pfc::string8 match_msg;
		uGetDlgItemText(m_hWnd, IDC_STATIC_MATCH_TRACKS_MSG, match_msg);

		if (!(stricmp_utf8(match_msg, match_failed) 
			&& stricmp_utf8(match_msg, match_assumed)
			&& stricmp_utf8(match_msg, match_manual))) {

			SetBkMode((HDC)wParam, GetSysColor(COLOR_3DFACE));
			SetTextColor((HDC)wParam, RGB(255, 0, 0));
			return (LRESULT)GetSysColorBrush(COLOR_3DFACE);
		}
		else {
			return FALSE;
		}
	}
	else {
		return FALSE;
	}
}
void CTrackMatchingDialog::generate_track_mappings(track_mappings_list_type &track_mappings) {
	
	track_mappings.force_reset();

	int file_index = -1;
	int dc_track_index = -1;
	int dc_disc_index = -1;
	const size_t count_d = m_coord.GetDiscogsTrackUiLvSize();
	const size_t count_f = m_coord.GetFileTrackUiLvSize();

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
			size_t orig_file_index;
			if ((orig_file_index = m_coord.GetFileTrackUiAtLvPos(i, file_match)) != pfc_infinite) {
				file_index = orig_file_index;
			}
			else
				file_index = -1;
		}

		track.enabled = (dc_track_index != -1 && dc_disc_index != -1);
		track.discogs_disc = dc_disc_index; //DECODE_DISCOGS_DISK(dindex);
		track.discogs_track = dc_track_index;  //DECODE_DISCOGS_TRACK(dindex);
		track.file_index = file_index;
		track_mappings.append_single(track);
	}
}

LRESULT CTrackMatchingDialog::OnButtonBack(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	m_idc_list.TooltipRemove();
	m_ifile_list.TooltipRemove();
	m_ida_list.TooltipRemove();
	m_ifa_list.TooltipRemove();
	cfg_dialog_position_track_matching_dlg.RemoveWindow(m_hWnd);

	go_back();

	return TRUE;
}

bool CTrackMatchingDialog::generate_artwork_guids(pfc::array_t<GUID> &my_album_art_ids, bool want_file_match) {

	//conf

	if (m_coord.GetDiscogsArtRowLvSize() && m_coord.GetFileArtRowLvSize()) {

		bool bfile_match = CONF_MULTI_ARTWORK.file_match;
		CONF_MULTI_ARTWORK = *m_coord.GetUartwork();
		CONF_MULTI_ARTWORK.file_match = bfile_match;
	}
	else {

		if (want_file_match) return false;

		bool bfile_match = CONF_MULTI_ARTWORK.file_match;
		CONF_MULTI_ARTWORK = multi_uartwork(CONF, m_tag_writer->GetRelease());
		CONF_MULTI_ARTWORK.file_match = bfile_match;
	}

	//guids

	if (want_file_match) {

		if (m_coord.GetDiscogsArtRowLvSize() && m_coord.GetFileArtRowLvSize()) {

			CONF_ARTWORK_GUIDS = *m_coord.GetUartwork_GUIDS();

			my_album_art_ids = pfc::array_t<GUID>();
			my_album_art_ids.resize(m_tag_writer->GetArtCount());

			GUID last_guid = { 0,0,0,0 };
			pfc::fill_array_t(my_album_art_ids, last_guid);

			getimages_file_it getimgfile_it;
			getimages_it getimg_it;

			size_t walk_file = 0;
			size_t dc_skipped = 0;

			auto csrcfiles = m_coord.GetDiscogsArtRowLvSize();
			auto ctargetfiles = m_coord.GetFileArtRowLvSize();
			for (size_t walk = 0; walk < ctargetfiles && walk < csrcfiles; walk++) {

				var_it_t out;
				size_t ndx_discogs_img = m_coord.Get_V_LvRow(lsmode::art, true, walk, out);

				if (ndx_discogs_img == pfc_infinite) break;

				getimg_it = std::get<2>(*out).second;
				art_src art_source = (art_src)getimg_it->first.first;
				lv_art_param this_lv_param(ndx_discogs_img, art_source);
				bool dc_isalbum = this_lv_param.is_album();
				size_t att_pos = dc_isalbum ? ndx_discogs_img : ndx_discogs_img - m_tag_writer->GetArtCount(art_src::alb);
				af att = dc_isalbum ? af::alb_sd : af::art_sd;
				bool b_woe_flag = CONF_MULTI_ARTWORK.getflag(att, att_pos);
				att = dc_isalbum ? af::alb_ovr : af::art_ovr;
				b_woe_flag |= CONF_MULTI_ARTWORK.getflag(att, att_pos);
				att = dc_isalbum ? af::alb_emb : af::art_emb;
				b_woe_flag |= CONF_MULTI_ARTWORK.getflag(att, att_pos);

				if (!b_woe_flag) {
					++dc_skipped;
					continue;
				}

				size_t ndx_file_img = m_coord.Get_V_LvRow(lsmode::art, false, walk_file + dc_skipped, out);

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

					if (!btype) {
						for (size_t i = 0; i < template_art_ids::num_types(); i++) {
							if (template_art_ids::query_type(i) == imgfileguid) {
								btype = true;
								break;
							}
						}
					}
					if (btype) {
						my_album_art_ids[ndx_discogs_img] = imgfileguid;
						last_guid = imgfileguid;
					}
				}
				else {
					if (last_guid != GUID({ 0,0,0,0 }))
						my_album_art_ids[ndx_discogs_img] = last_guid;
					else
						my_album_art_ids[ndx_discogs_img] = dc_isalbum ? album_art_ids::cover_front : album_art_ids::artist;
				}
				++walk_file;
			}
		}
		else {
			return false;
		}
	}
	else {

		size_t calbum_art = m_tag_writer->GetArtCount(art_src::alb);
		size_t cartist_art = 0;

		for (auto wra : m_tag_writer->GetRelease()->artists) {
			cartist_art += wra->full_artist->images.get_count();
		}
		if ((calbum_art + cartist_art) > 0) {
			my_album_art_ids.resize(m_tag_writer->GetArtCount());
		}

		static const GUID undef_guid = { 0xCDCDCDCD, 0xCDCD, 0xCDCD, { 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD } };

		pfc::fill_array_t(my_album_art_ids, undef_guid);

		if (calbum_art) {
			my_album_art_ids[0] = album_art_ids::cover_front;
		}

		if (cartist_art) {
			my_album_art_ids[calbum_art] = album_art_ids::artist; //may be 0 for no album artwork releases
		}
	}
	return true;
}
LRESULT CTrackMatchingDialog::OnButtonWriteArtwork(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	bool b_user_skip_artwork = IsDlgButtonChecked(IDC_CHK_SKIP_ARTWORK);
	if (CONF_MULTI_ARTWORK.isEmpty()) return TRUE;

	pfc::array_t<GUID> my_album_art_ids;
	bool bfile_match = CONF_MULTI_ARTWORK.file_match;

	if (!generate_artwork_guids(my_album_art_ids, bfile_match)) {
		return false;
	}

	//write art
	service_ptr_t<download_art_paths_task> task_ids =
		new service_impl_t<download_art_paths_task>(this, m_tag_writer->GetRelease()->id,
			m_tag_writer->m_finfo_manager->items,	my_album_art_ids, false, bfile_match);

	task_ids->start();

	init_track_matching_dialog();

	return TRUE;
}

LRESULT CTrackMatchingDialog::OnButtonPreviewTags(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	auto user_wants_skip = HIWORD(m_conf.album_art_skip_default_cust);

	bool b_user_skip_artwork = IsDlgButtonChecked(IDC_CHK_SKIP_ARTWORK);
	if (b_user_skip_artwork)  user_wants_skip |= ARTSAVE_SKIP_USER_FLAG;
	else user_wants_skip &= ~(ARTSAVE_SKIP_USER_FLAG);

	m_conf.album_art_skip_default_cust = MAKELPARAM(LOWORD(m_conf.album_art_skip_default_cust), user_wants_skip);
	bool save_artwork = m_conf.save_album_art || m_conf.save_artist_art || m_conf.embed_album_art || m_conf.embed_artist_art;
	if (!b_user_skip_artwork && save_artwork) {
		bool bfile_match = CONF_MULTI_ARTWORK.file_match;

		if (!generate_artwork_guids(CONF_ARTWORK_GUIDS, bfile_match)) {
			return FALSE;
		}
	}
	pushcfg();
	generate_track_mappings(m_tag_writer->m_track_mappings);

	try {
		service_ptr_t<generate_tags_task> task = new service_impl_t<generate_tags_task>(this, m_tag_writer, true);
		task->start();
	}
	catch (locked_task_exception e)
	{
		log_msg(e.what());
	}
	return TRUE;
}

LRESULT CTrackMatchingDialog::OnButtonWriteTags(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	bool save_artwork = m_conf.save_album_art || m_conf.save_artist_art || m_conf.embed_album_art || m_conf.embed_artist_art;

	auto user_wants_skip = HIWORD(m_conf.album_art_skip_default_cust);

	bool b_user_skip_artwork = IsDlgButtonChecked(IDC_CHK_SKIP_ARTWORK);
	if (b_user_skip_artwork)  user_wants_skip |= ARTSAVE_SKIP_USER_FLAG;
	else user_wants_skip &= ~(ARTSAVE_SKIP_USER_FLAG);

	m_conf.album_art_skip_default_cust = MAKELPARAM(LOWORD(m_conf.album_art_skip_default_cust), user_wants_skip);

	if (!b_user_skip_artwork && save_artwork) {
		bool bfile_match = CONF_MULTI_ARTWORK.file_match;
		if (!generate_artwork_guids(CONF_ARTWORK_GUIDS, bfile_match)) {
			return FALSE;
		}
	}

	generate_track_mappings(m_tag_writer->m_track_mappings);

	try {
		service_ptr_t<generate_tags_task> task = new service_impl_t<generate_tags_task>(this, m_tag_writer, false);
		task->start();
	}
	catch (locked_task_exception e)
	{
		log_msg(e.what());
	}

	return TRUE;
}

void CTrackMatchingDialog::pushcfg() {

	if (build_current_cfg()) {
		CONF.save(CConf::cfgFilter::TRACK, m_conf);
		CONF.save(CConf::cfgFilter::TRACK, m_conf, CFG_ALBUM_ART_SKIP_DEFAULT_CUST);
		CONF.load();
	}
}

inline bool CTrackMatchingDialog::build_current_cfg() {

	bool bres = false;
	const foo_conf * cfg_coord = &CONF;

	//track match columns
	if (cfg_coord->match_tracks_discogs_col1_width != m_conf.match_tracks_discogs_col1_width ||
		cfg_coord->match_tracks_discogs_col2_width != m_conf.match_tracks_discogs_col2_width ||
		//track match columns
		cfg_coord->match_tracks_files_col1_width != m_conf.match_tracks_files_col1_width ||
		cfg_coord->match_tracks_files_col2_width != m_conf.match_tracks_files_col2_width ||
		//styles
		cfg_coord->match_tracks_discogs_style != m_conf.match_tracks_discogs_style ||
		cfg_coord->match_tracks_files_style != m_conf.match_tracks_files_style) {

		bres |= true;
	}

	//artwork image columns
	if (cfg_coord->match_discogs_artwork_ra_width != m_conf.match_discogs_artwork_ra_width ||
		cfg_coord->match_discogs_artwork_type_width != m_conf.match_discogs_artwork_type_width ||
		cfg_coord->match_discogs_artwork_dim_width != m_conf.match_discogs_artwork_dim_width ||
		cfg_coord->match_discogs_artwork_save_width != m_conf.match_discogs_artwork_save_width ||
		cfg_coord->match_discogs_artwork_ovr_width != m_conf.match_discogs_artwork_ovr_width ||
		cfg_coord->match_discogs_artwork_embed_width != m_conf.match_discogs_artwork_embed_width ||
		cfg_coord->match_discogs_artwork_index_width != m_conf.match_discogs_artwork_index_width ||
		//artwork file columns
		cfg_coord->match_file_artwork_name_width != m_conf.match_file_artwork_name_width ||
		cfg_coord->match_file_artwork_dim_width != m_conf.match_file_artwork_dim_width ||
		cfg_coord->match_file_artwork_size_width != m_conf.match_file_artwork_size_width ||
		cfg_coord->match_file_artwork_size_width != m_conf.match_file_artwork_index_width ||
		//styles
		//todo: depri
		cfg_coord->match_discogs_artwork_art_style != m_conf.match_discogs_artwork_art_style ||
		cfg_coord->match_file_artworks_art_style != m_conf.match_file_artworks_art_style) {
		//..
		bres |= true;
	}

	//skip artwork
	int skip_art = 0;

	//IDC_CHK_SKIP_ARTWORK
	auto checkstate = m_tristate.GetState();
	if (checkstate == BST_INDETERMINATE)
	{
		skip_art |= BST_INDETERMINATE;
	}
	else if (checkstate == BST_CHECKED) {

		skip_art |= ARTSAVE_SKIP_USER_FLAG;
	}
	m_conf.album_art_skip_default_cust = (int)MAKELPARAM(LOWORD(m_conf.album_art_skip_default_cust), skip_art);

	if (cfg_coord->album_art_skip_default_cust != m_conf.album_art_skip_default_cust) {
				
		bres |= true;
	}

	return bres;
}

void CTrackMatchingDialog::set_tristate(bool btri, UINT state, bool enabled) {
	if (btri) m_tristate.SetTristate(BST_INDETERMINATE, BST_INDETERMINATE);
	m_tristate.Init(state, enabled);
}

void CTrackMatchingDialog::LibUIAsTrackList(bool toTrackFile) {

	// match control size, libUI control into match owner data control view

	CRect rc_dcList;
	CRect rc_fileList;
	CRect rc_UI_dcList;
	CRect rc_UI_fileList;

	if (toTrackFile) {

		::GetWindowRect(uGetDlgItem(IDC_UI_DC_ARTWORK_LIST), &rc_dcList);
		::GetWindowRect(uGetDlgItem(IDC_UI_FILE_ARTWORK_LIST), &rc_fileList);

		ScreenToClient(&rc_dcList);
		::SetWindowPos(uGetDlgItem(IDC_UI_LIST_DISCOGS), HWND_TOP, rc_dcList.left, rc_dcList.top,
			rc_dcList.Width(), rc_dcList.Height(), SWP_NOACTIVATE | SWP_SHOWWINDOW);
		ScreenToClient(&rc_fileList);
		::SetWindowPos(uGetDlgItem(IDC_UI_LIST_FILES), HWND_TOP, rc_fileList.left, rc_fileList.top,
			rc_fileList.Width(), rc_fileList.Height(), SWP_NOACTIVATE | SWP_SHOWWINDOW);
	}
	else {
		int art_app_flag = LOWORD(m_conf.album_art_skip_default_cust);
		::GetWindowRect(uGetDlgItem(IDC_UI_LIST_DISCOGS), &rc_UI_dcList);
		::GetWindowRect(uGetDlgItem(IDC_UI_LIST_FILES), &rc_UI_fileList);

		CRect rc_dcList;
		::GetWindowRect(uGetDlgItem(IDC_UI_DC_ARTWORK_LIST), &rc_dcList);
		CRect rc_fileList;
		::GetWindowRect(uGetDlgItem(IDC_UI_FILE_ARTWORK_LIST), &rc_fileList);

		ScreenToClient(&rc_UI_dcList);
		::SetWindowPos(uGetDlgItem(IDC_UI_DC_ARTWORK_LIST), HWND_TOP, rc_UI_dcList.left, rc_UI_dcList.top,
			rc_UI_dcList.Width(), rc_UI_dcList.Height(), SWP_NOACTIVATE | SWP_SHOWWINDOW);
		ScreenToClient(&rc_UI_fileList);
		::SetWindowPos(uGetDlgItem(IDC_UI_FILE_ARTWORK_LIST), HWND_TOP, rc_UI_fileList.left, rc_UI_fileList.top,
			rc_UI_fileList.Width(), rc_UI_fileList.Height(), SWP_NOACTIVATE | SWP_SHOWWINDOW);
	}
}

size_t CTrackMatchingDialog::get_art_perm_selection(HWND hwndList, bool flagselected, const size_t max_items, pfc::array_t<t_uint8>& outmask, bit_array_bittable& are_albums) {

	auto p_uilist = GetUIListByWnd(hwndList);
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
		if (p_uilist->IsItemSelected(i)) {
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

void CTrackMatchingDialog::context_menu_art_attrib_switch(HWND wnd, af att_album, af att_art, UINT att_id, bool mixattr) {
	
	multi_uartwork* multi_uart = m_coord.GetUartwork();
	auto uilist = GetUIListByWnd(wnd);
	size_t citems = uilist->GetItemCount();
	pfc::array_t<t_uint8> perm_selection;
	perm_selection.resize(citems);
	bit_array_bittable are_albums(citems);

	const size_t max_items = m_tag_writer->GetArtCount();
	const t_uint8 cAlbumArt = static_cast<t_uint8>(m_tag_writer->GetArtCount(art_src::alb));

	size_t perm_csel = get_art_perm_selection(wnd, true, max_items, perm_selection, are_albums);

	for (size_t i = 0; i < citems && perm_csel; i++) {

		//selected items are flagged with citems value
		bool bselected = perm_selection[i] != max_items; 

		if (bselected) {

			t_uint8 dc_ndx = perm_selection[i];
			
			af att = are_albums.get(i) ? att_album : att_art;
			dc_ndx = are_albums.get(i) ? dc_ndx : dc_ndx - cAlbumArt;
			
			bool val = mixattr ? false : !multi_uart->getflag(att, dc_ndx);
			multi_uart->setflag(att, dc_ndx, val);

			perm_csel--;

			uilist->ReloadItem(i);
		}
	}
	init_track_matching_dialog();
}

bool CTrackMatchingDialog::context_menu_form(HWND wnd, LPARAM lParamPos) {

	bool bvk_apps = lParamPos == -1;

	POINT point;

	if (bvk_apps) {
		CRect rect;
		CWindow(wnd).GetWindowRect(&rect);
		point = rect.CenterPoint();
	}
	else {
		point.x = GET_X_LPARAM(lParamPos);
		point.y = GET_Y_LPARAM(lParamPos);
	}
	file_info_impl finfo;
	metadb_handle_ptr item = m_tag_writer->m_finfo_manager->items[0];
	item->get_info(finfo);
		
	pfc::string8 local_release_id;
	
	const char* ch_local_rel = finfo.meta_get("DISCOGS_RELEASE_ID", 0);
	if (ch_local_rel) {
		local_release_id = ch_local_rel;
	}

	auto tw_release = m_tag_writer->GetRelease();

	pfc::string8 discogs_release_id(tw_release->id);
	pfc::string8 master_release_id(tw_release->master_id);
	pfc::string8 artist_id(tw_release->artists[0]->full_artist->id);
	std::vector<std::pair<std::string, std::string>>vartists;
	bool is_multiartist = discogs_interface->artists_vid(tw_release, vartists);

	bool hasRelease = discogs_release_id.get_length();
	bool hasMasterRelease = master_release_id.get_length();
	bool hasArtist = artist_id.get_length();

	bool diff_ids = local_release_id.get_length() && (!(STR_EQUAL(discogs_release_id, local_release_id)));

	pfc::string8 discogs_release_info (diff_ids ? "Discogs Release Id: " : "Release Id: ");
	pfc::string8 local_release_info;
	discogs_release_info << discogs_release_id;
	if (local_release_id.get_length())
		local_release_info << "Files Release Id: " << local_release_id;

	bit_array_bittable mixedvals;

	try {

		enum { ID_URL_DC_INFO = 1000, ID_URL_LC_INFO, ID_URL_RELEASE, ID_URL_MASTER_RELEASE, ID_URL_ARTIST, ID_URL_ARTISTS,
			ID_URL_MB_REL = 1100, ID_URL_MB_RELGRP, ID_URL_MB_COVERS, ID_URL_MB_ARTIST };

		HMENU menu = CreatePopupMenu();

		uAppendMenu(menu, MF_STRING, ID_PREVIEW_CMD_BACK, "&Back");
		uAppendMenu(menu, MF_STRING, ID_PREVIEW_CMD_PREVIEW, "&Preview");
		uAppendMenu(menu, MF_SEPARATOR, 0, 0);

		uAppendMenu(menu, MF_STRING | (get_mode() == lsmode::default ? MF_UNCHECKED : MF_CHECKED),
			ID_ART_TOOGLE_TRACK_ART_MODES, "&manage artwork");

		uAppendMenu(menu, MF_SEPARATOR, 0, 0);

		uAppendMenu(menu, MF_STRING | MF_DISABLED | MF_GRAYED, ID_URL_DC_INFO , discogs_release_info);

		if (diff_ids)
			uAppendMenu(menu, MF_STRING | MF_DISABLED | MF_GRAYED, ID_URL_LC_INFO, local_release_info);

		uAppendMenu(menu, MF_SEPARATOR, 0, 0);
		uAppendMenu(menu, MF_STRING | (!hasRelease ? MF_DISABLED | MF_GRAYED : 0), ID_URL_RELEASE , "Web &release page");
		uAppendMenu(menu, MF_STRING | (!hasMasterRelease ? MF_DISABLED | MF_GRAYED : 0), ID_URL_MASTER_RELEASE , "Web mas&ter release page");
		uAppendMenu(menu, MF_STRING | (!hasArtist ? MF_DISABLED | MF_GRAYED | MF_GRAYED : 0), ID_URL_ARTIST , "Web &artist page");
		if (is_multiartist) {
			build_sub_menu(menu, ID_URL_ARTISTS, vartists, _T("Web release artist&s page"), 16);
		}
		if (!(CONF.skip_mng_flag & SkipMng::BRAINZ_ID_FETCH)) {
			uAppendMenu(menu, MF_SEPARATOR, 0, 0);
			bool hasMib_Release = m_musicbrainz_mibs.release.get_length();
			bool hasMib_ReleaseGroup = m_musicbrainz_mibs.release_group.get_length();
			bool hasMib_CoverArt = m_musicbrainz_mibs.coverart;
			bool hasMib_Artist = m_musicbrainz_mibs.artist.get_length();
			uAppendMenu(menu, MF_STRING | (!hasMib_Release ? MF_DISABLED | MF_GRAYED : 0), ID_URL_MB_REL, "Web MusicBrainz re&lease page");		
			uAppendMenu(menu, MF_STRING | (!hasMib_ReleaseGroup ? MF_DISABLED | MF_GRAYED : 0), ID_URL_MB_RELGRP, "Web MusicBrainz release-&group page");
			uAppendMenu(menu, MF_STRING | (!hasMib_CoverArt ? MF_DISABLED | MF_GRAYED : 0), ID_URL_MB_COVERS, "Web MusicBrainz co&verart page");
			uAppendMenu(menu, MF_STRING | (!hasMib_Artist ? MF_DISABLED | MF_GRAYED : 0), ID_URL_MB_ARTIST, "Web MusicBrainz &artist page");
		}

		uAppendMenu(menu, MF_SEPARATOR, 0, 0);

		if (get_mode() != lsmode::default) {

			uAppendMenu(menu, MF_STRING, ID_PREVIEW_CMD_WRITE_ARTWORK, "Save artw&ork");
			uAppendMenu(menu, MF_SEPARATOR, 0, 0);
		}

		uAppendMenu(menu, MF_STRING, ID_PREVIEW_CMD_WRITE_TAGS, "&Write tags");

		int cmd = TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, point.x, point.y, 0, core_api::get_main_window(), 0);
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
		case ID_URL_MB_REL: {
			url << "https://musicbrainz.org/release/" << m_musicbrainz_mibs.release;
			break;
		}
		case ID_URL_MB_RELGRP: {
			url << "https://musicbrainz.org/release-group/" << m_musicbrainz_mibs.release_group;
			break;
		}
		case ID_URL_MB_COVERS: {
			url << "https://musicbrainz.org/release/" << m_musicbrainz_mibs.release << "/cover-art";
			break;
		}
		case ID_URL_MB_ARTIST: {
			url << "https://musicbrainz.org/artist/" << m_musicbrainz_mibs.artist;
			break;
		}
		case ID_ART_TOOGLE_TRACK_ART_MODES:
			[[fallthrough]];
		case ID_PREVIEW_CMD_BACK:
			[[fallthrough]];
		case ID_PREVIEW_CMD_PREVIEW:
			[[fallthrough]];
		case ID_PREVIEW_CMD_WRITE_TAGS:
			[[fallthrough]];
		case ID_PREVIEW_CMD_WRITE_ARTWORK:
			context_menu_track_switch(NULL, point, false, cmd, pfc::bit_array_bittable(), mixedvals);
			break;
		default: {
			if (cmd >= ID_URL_ARTISTS && cmd < ID_URL_MB_REL) {
				pfc::string8 artist_id = vartists.at(cmd - ID_URL_ARTISTS).first.c_str();
				url << "https://www.discogs.com/artist/" << artist_id;
			}
			break;
		}
		}

		if (url.get_length()) display_url(url);
	}
	catch (...) {};

	return true;
}

bool CTrackMatchingDialog::context_menu_track_show(HWND wnd, int idFrom, LPARAM lParamPos) {
	CListControlOwnerData* p_uilist = GetUIListByWnd(wnd);

	CPoint point = p_uilist->GetContextMenuPoint(lParamPos);

	bool is_uilist = p_uilist;
	bool is_files = ::GetWindowLong(wnd, GWL_ID) == IDC_UI_FILE_ARTWORK_LIST ||
		::GetWindowLong(wnd, GWL_ID) == IDC_UI_LIST_FILES;
	try {
		HMENU menu = CreatePopupMenu();
		
		HMENU _childmenuAlign = CreatePopupMenu();
		HMENU _childmenuTemplate = CreatePopupMenu();
		MENUITEMINFO mi_align = { 0 };
		MENUITEMINFO mi_template = { 0 };
		
		mi_align.cbSize = sizeof(MENUITEMINFO);
		mi_align.fMask = MIIM_SUBMENU | MIIM_STRING | MIIM_ID;
		mi_align.wID = ID_SUBMENU_SELECTOR_ALIGN;
		mi_align.hSubMenu = _childmenuAlign;
		mi_align.dwTypeData = _T("Column alignment");

		mi_template.cbSize = sizeof(MENUITEMINFO);
		mi_template.fMask = MIIM_SUBMENU | MIIM_STRING | MIIM_ID;
		mi_template.wID = ID_SUBMENU_SELECTOR_TEMPLATES;
		mi_template.hSubMenu = _childmenuTemplate;
		mi_template.dwTypeData = _T("Templates");

		bool bver = core_version_info_v2::get()->test_version(1, 6, 2, 0);

		size_t csel = is_uilist ? p_uilist->GetSelectedCount() : 0;
		size_t citems = is_uilist ? p_uilist->GetItemCount() : 0;
		bit_array_bittable selmask = is_uilist ? p_uilist->GetSelectionMask() : bit_array_bittable();
		bool bsel = selmask.find_first(true, 0, selmask.size()) != selmask.size();

		bit_array_bittable mixedvals;

		uAppendMenu(menu, MF_STRING, ID_PREVIEW_CMD_BACK, "&Back");
		uAppendMenu(menu, MF_STRING, ID_PREVIEW_CMD_PREVIEW, "&Preview");
		if (is_files) {
			uAppendMenu(menu, MF_STRING | (bsel && get_mode() == lsmode::art && bver? 0 : MF_DISABLED | MF_GRAYED), ID_ART_IMAGE_VIEWER, "Image Viewer\tDouble Click");
		}
		uAppendMenu(menu, MF_SEPARATOR, 0, 0);
		uAppendMenu(menu, MF_STRING	| (get_mode() == lsmode::default ? MF_UNCHECKED : MF_CHECKED),
			ID_ART_TOOGLE_TRACK_ART_MODES, "&manage artwork");

		if (get_mode() == lsmode::art) {
			if (!is_files) {
				if (csel) {
					uAppendMenu(menu, MF_SEPARATOR, 0, 0);
					uAppendMenu(menu, MF_STRING, ID_ART_PREVIEW, "Thumbnail preview");
					bool check_template_requirement = m_coord.template_artwork_mode(template_art_ids::inlay_card, 0, p_uilist->GetFirstSelected(), true);
					if (csel && check_template_requirement) {
						uAppendMenu(menu, MF_SEPARATOR, 0, 0);
						// display submenu
						for (size_t walk_ids = 0; walk_ids < template_art_ids::num_types(); walk_ids++) {
							uAppendMenu(_childmenuTemplate, MF_STRING, ID_ART_TEMPLATE1 + walk_ids, template_art_ids::query_capitalized_name(walk_ids));
						}
						InsertMenuItem(menu, ID_SUBMENU_SELECTOR_TEMPLATES, true, &mi_template);
					}
				}
				//attribs
				
				context_menu_art_attrib_show(wnd, &menu, mixedvals);

			}
			else {
				uAppendMenu(menu, MF_SEPARATOR, 0, 0);
				uAppendMenu(menu, MF_STRING, ID_ART_RESET, "Reset");				
			}
		}

		uAppendMenu(menu, MF_SEPARATOR, 0, 0);
		uAppendMenu(menu, MF_STRING | (citems? 0 : MF_DISABLED | MF_GRAYED), ID_SELECT_ALL, "Select all");
		uAppendMenu(menu, MF_STRING | (csel? 0 : MF_DISABLED | MF_GRAYED), ID_INVERT_SELECTION, "Invert selection");
		uAppendMenu(menu, MF_STRING | (csel? 0 : MF_DISABLED | MF_GRAYED), ID_REMOVE, "Remove");
		uAppendMenu(menu, MF_STRING | (csel? 0 : MF_DISABLED | MF_GRAYED), ID_CROP, "Crop");

		if (is_uilist) {
			if ((get_mode() == lsmode::tracks_ui)) {
				uAppendMenu(menu, MF_SEPARATOR, 0, 0);
				uAppendMenu(menu, MF_STRING, ID_ROW_NUMBERS, "Show row number");
			}
			std::pair<size_t, presenter*> icol_hit = m_coord.columnHitTest(point);
			if (icol_hit.first != pfc_infinite) {

				size_t fmt = m_coord.GetUiColumnFormat(icol_hit.first, icol_hit.second);
				// display submenu
				uAppendMenu(_childmenuAlign, MF_STRING | (fmt == HDF_LEFT ? MF_CHECKED | MF_DISABLED | MF_GRAYED : 0), ID_LEFT_ALIGN, "Left");
				uAppendMenu(_childmenuAlign, MF_STRING | (fmt == HDF_CENTER ? MF_CHECKED | MF_DISABLED | MF_GRAYED : 0), ID_CENTER_ALIGN, "Center");
				uAppendMenu(_childmenuAlign, MF_STRING | (fmt == HDF_RIGHT ? MF_CHECKED | MF_DISABLED | MF_GRAYED : 0), ID_RIGHT_ALIGN, "Right");
				InsertMenuItem(menu, ID_SUBMENU_SELECTOR_ALIGN, true, &mi_align);				
			}
		}

		uAppendMenu(menu, MF_SEPARATOR, 0, 0);

		if (get_mode() != lsmode::default) {

			uAppendMenu(menu, MF_STRING | (citems ? 0 : MF_DISABLED | MF_GRAYED), ID_PREVIEW_CMD_WRITE_ARTWORK, "Save &artwork");
			uAppendMenu(menu, MF_SEPARATOR, 0, 0);
		}
		
		uAppendMenu(menu, MF_STRING | (citems ? 0 : MF_DISABLED | MF_GRAYED), ID_PREVIEW_CMD_WRITE_TAGS, "&Write tags");

		int cmd = TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, point.x, point.y, 0, wnd, 0);
		DestroyMenu(menu);

		context_menu_track_switch(wnd, point, is_files, cmd, selmask, mixedvals);
	}
	catch (...) {}
	return FALSE;
}

bool CTrackMatchingDialog::context_menu_track_switch(HWND wnd, POINT point, bool is_files, size_t cmd,
	bit_array_bittable selmask, bit_array_bittable mixedvals) {

	auto p_uilist = GetUIListByWnd(wnd);

	UINT att_id = 0;
	af att_album;
	af att_art;

	lsmode mode = get_mode();

	switch (cmd)
	{
	case ID_ART_TOOGLE_TRACK_ART_MODES: {
		uButton_SetCheck(m_hWnd, IDC_CHK_MNG_ARTWORK, get_mode() == lsmode::default);
		::SendMessage(m_hWnd, WM_COMMAND, MAKEWPARAM(IDC_CHK_MNG_ARTWORK, BN_CLICKED), 0);
		return true;
	}
	case ID_SELECT_ALL:
	{
		if (p_uilist)
			p_uilist->SelectAll();

		return true;
	}
	case ID_INVERT_SELECTION:
	{
		if (p_uilist) {
			for (size_t i = 0; i < selmask.size(); i++) selmask.set(i, !selmask.get(i));
			p_uilist->SetSelection(bit_array_true(), selmask);
		}

		return true;
	}
	case ID_REMOVE:
	{
		//process attribs

		bool bcrop = false;
		const size_t count = selmask.size();
		bit_array_bittable delmask(count);
		for (size_t i = 0; i < count; i++) {
			if (bcrop)
				delmask.set(i, !selmask.get(i));
			else
				delmask.set(i, selmask.get(i));
		}

		//are albums

		pfc::array_t<t_uint8> perm_selection;
		bit_array_bittable are_albums;
		if (get_mode() == lsmode::art && !is_files) {
			perm_selection.resize(count);
			are_albums.resize(count);
			const size_t max_items = m_tag_writer->GetArtCount();
			size_t csel = get_art_perm_selection(wnd, true, max_items, perm_selection, are_albums);		
		}

		size_t nextfocus = ~0;
		if (p_uilist->m_hWnd) {
			//list user command...
			nextfocus = m_coord.ListUserCmd(wnd, get_mode(), ID_REMOVE, delmask, are_albums, {}, false);
			match_message_update(match_manual);
		}
		selmask.resize(0);
		if (nextfocus != 0) {
			selmask.resize(count - perm_selection.size());
			selmask.set(nextfocus, true);
		}
		p_uilist->OnItemsRemoved(selmask, selmask.size());

		if (get_mode() == lsmode::art && !is_files) {
			//refresh tristate
			init_track_matching_dialog();
		}
		return true;
	}
	
	case ID_CROP:
	{
		for (size_t i = 0; i < selmask.size(); i++) {
			selmask.set(i, !selmask.get(i));
		}

		bit_array_bittable are_albums;
		if (get_mode() == lsmode::art && !is_files) {
			//not a valid selection on crop, need are_albums
			pfc::array_t<t_uint8> x_selection;
			x_selection.resize(selmask.size());
			are_albums.resize(selmask.size());
			const size_t max_items = m_tag_writer->GetArtCount();
			size_t csel = get_art_perm_selection(p_uilist->m_hWnd/*wnd*/, true, max_items, x_selection, are_albums);
		}

		if (p_uilist) {
			m_coord.ListUserCmd(p_uilist->m_hWnd, get_mode(), ID_REMOVE, selmask, are_albums, {}, true);
			match_message_update(match_manual);
			p_uilist->OnItemsRemoved(selmask, selmask.size());
		}

		if (get_mode() == lsmode::art && !is_files) {
			//refresh tristate
			init_track_matching_dialog();
			}
		return true; // found;
	}
	case ID_ROW_NUMBERS:
		m_coord.ColumnRowToggle();
		return true;
	case ID_LEFT_ALIGN: {
		std::pair<size_t, presenter*> icol_hit = m_coord.columnHitTest(point);
		m_coord.SetUiColumnFormat(icol_hit.first, icol_hit.second, HDF_LEFT);
		return true;
	}
	case ID_CENTER_ALIGN:	{
		std::pair<size_t, presenter*> icol_hit = m_coord.columnHitTest(point);
		m_coord.SetUiColumnFormat(icol_hit.first, icol_hit.second, HDF_CENTER);
		return true;
	}
	case ID_RIGHT_ALIGN: {
		std::pair<size_t, presenter*> icol_hit = m_coord.columnHitTest(point);
		m_coord.SetUiColumnFormat(icol_hit.first, icol_hit.second, HDF_RIGHT);
		return true;
	}
	case ID_ART_PREVIEW: {
		HWND wndlist = wnd;
		bool is_files = wnd == uGetDlgItem(IDC_UI_FILE_ARTWORK_LIST);
		size_t citems = p_uilist->GetItemCount();

		if (is_files || !citems) return true;
		
		pfc::array_t<t_uint8> perm_selection;
		perm_selection.resize(citems);
		bit_array_bittable are_albums(citems);

		const size_t max_items = m_tag_writer->GetArtCount();
		size_t csel = get_art_perm_selection(wndlist, true, max_items, perm_selection, are_albums);

		size_t chk = perm_selection[0];

		size_t added = 0;

		for (size_t i = 0; i < citems; i++) {

			if (perm_selection[i] != max_items) { 
				size_t album_artist_ndx = perm_selection[i];

				if (!are_albums.get(i)) {
					album_artist_ndx -= m_tag_writer->GetArtCount(art_src::alb);
				}
				preview_job pj(false, album_artist_ndx, !are_albums.get(i), false, false);
				m_vpreview_jobs.emplace_back(pj);
				++added;
			}
		}

		if (added)
		{
			std::lock_guard<std::mutex> lg(m_mx_pending_previews_mod);
			
			add_pending_previews(added);

			for (auto it = m_vpreview_jobs.rbegin(); it - m_vpreview_jobs.rbegin() < added; it++) {				
				request_preview(it->index_art, it->artist_art, false, false);
			}
		}

		return true;
	}
	case ID_ART_IMAGE_VIEWER: {

		bool bsel = selmask.find_first(true, 0, selmask.size()) != selmask.size();

		//image viewer req >= 1.6.2
		bool has_viewer = core_version_info_v2::get()->test_version(1, 6, 2, 0);
		if (has_viewer) {
			size_t first_sel = selmask.find_first(true, 0, selmask.size());
			getimages_file_it out_art_file_it;
			m_coord.GetFileArtAtLvPos(first_sel, out_art_file_it);
			if (bsel) {
				ndx_image_file_t ndx_img = out_art_file_it->first;
				service_ptr_t<fb2k::imageViewer> img_viewer = fb2k::imageViewer::get();
				const GUID aaType = ndx_img.second;
				img_viewer->load_and_show(m_hWnd,
					m_tag_writer->m_finfo_manager->items,
					aaType, 0);
			}
		}
		break;
	}
	// artwork
	case ID_ART_OVR:
		att_album = af::alb_ovr;
		att_art = af::art_ovr;
		att_id = ID_ART_OVR;
		context_menu_art_attrib_switch(wnd, att_album, att_art,
			att_id, mixedvals.get(1));
		break;
	case ID_ART_EMBED:
		att_album = af::alb_emb;
		att_art = af::art_emb;
		att_id = ID_ART_EMBED;
		context_menu_art_attrib_switch(wnd, att_album, att_art,
			att_id, mixedvals.get(2));
		break;
	case ID_ART_WRITE:
		att_album = af::alb_sd;
		att_art = af::art_sd;
		att_id = ID_ART_WRITE;
		context_menu_art_attrib_switch(wnd, att_album, att_art,
			att_id, mixedvals.get(0));
		break;
	case ID_ART_TILE:
		m_coord.SetModeTile(!m_coord.isTile());
		break;
	case ID_ART_DETAIL:
		m_coord.SetModeTile(!m_coord.isTile());
		break;
	case ID_ART_RESET: {
		if (p_uilist && get_mode() == lsmode::art) {
			bool bfile_match = false;
			CONF_MULTI_ARTWORK = multi_uartwork(CONF);
			CONF_MULTI_ARTWORK.file_match = bfile_match;
			uButton_SetCheck(m_hWnd, IDC_CHK_ARTWORK_FILEMATCH, bfile_match);
			::EnableWindow(uGetDlgItem(IDC_UI_FILE_ARTWORK_LIST), bfile_match);
			//..

			m_coord.Reset(wnd, get_mode());
			m_coord.Show(!is_files, is_files);

			if (get_mode() == lsmode::art && !is_files) {
				//refresh tristate
				init_track_matching_dialog();
			}
		}
		return true;
	}
	case ID_PREVIEW_CMD_BACK: {

		HWND hwndButton = uGetDlgItem(IDC_BTN_BACK);
		uSendMessage(m_hWnd, WM_NEXTDLGCTL, (WPARAM)(HWND)hwndButton, TRUE);
		auto retval = ::PostMessage(hwndButton, BM_CLICK, (WPARAM)(HWND)hwndButton, 0);
		break;
	}
	case ID_PREVIEW_CMD_PREVIEW: {
		BOOL bDummy;
		OnButtonPreviewTags(0, 0, NULL, bDummy);
		break;
	}
	case ID_PREVIEW_CMD_WRITE_TAGS: {

		HWND hwndButton = uGetDlgItem(IDC_BTN_WRITE_TAGS);
		uSendMessage(m_hWnd, WM_NEXTDLGCTL, (WPARAM)(HWND)hwndButton, TRUE);
		auto retval = ::PostMessage(hwndButton, BM_CLICK, (WPARAM)(HWND)hwndButton, 0);
		break;
	}
	case ID_PREVIEW_CMD_WRITE_ARTWORK: {
		BOOL bDummy;
		OnButtonWriteArtwork(0, 0, NULL, bDummy);
		break;
	}
	default: {
		if (cmd == 0) return false;
		size_t iTempl = cmd - ID_ART_TEMPLATE1;
		if (iTempl >= 0 && iTempl < template_art_ids::num_types() && p_uilist->GetSelectedCount() > 0) {
			bool check_template_requirement = m_coord.template_artwork_mode(template_art_ids::query_type(iTempl), p_uilist->GetSelectedCount(), p_uilist->GetFirstSelected(), false);
			auto p_dest_uilist = GetUIListById(IDC_UI_FILE_ARTWORK_LIST);
			p_dest_uilist->ReloadItems(bit_array_true());
			return true;
		}
	}
	}
	return false;
}

bool CTrackMatchingDialog::context_menu_art_attrib_show(HWND wndlist, HMENU* menu, bit_array_bittable &mixedvals) {
	
	lsmode mode = get_mode();

	if (mode != lsmode::art)
		return false;

	auto p_uilist = GetUIListByWnd(wndlist);
	size_t citems = p_uilist->GetItemCount();
	bool bselection = p_uilist->GetSelectedCount();
	bool is_files = ::GetWindowLong(wndlist, GWL_ID) == IDC_UI_FILE_ARTWORK_LIST;
	bool is_tile = m_coord.isTile();

	try {
		multi_uartwork* uartw = m_coord.GetUartwork();

		pfc::array_t<t_uint8> perm_selection;
		perm_selection.resize(citems);
		bit_array_bittable are_albums(citems);

		const size_t max_items = citems;
		const size_t cAlbumArt = m_tag_writer->GetArtCount(art_src::alb);

		size_t csel = get_art_perm_selection(wndlist, true, max_items, perm_selection, are_albums);

		bool mixedwrite = false, write = false;

		for (size_t i = 0; !mixedwrite && i < citems; i++) {
			if (perm_selection[i] != max_items) {
				size_t dc_ndx = perm_selection[i];
				af att = are_albums.get(i) ? af::alb_sd : af::art_sd;
				dc_ndx = are_albums.get(i) ? dc_ndx : dc_ndx - cAlbumArt;
				bool val = uartw->getflag(att, dc_ndx);
				if (!i) write = val;
				else if (write != val) mixedwrite = true;			
			}
		}
		bool mixedovr = false, ovr = false;
		for (size_t i = 0; !mixedovr && i < citems; i++) {
			if (perm_selection[i] != citems) {
				size_t dc_ndx = perm_selection[i];
				af att = are_albums.get(i) ? af::alb_ovr : af::art_ovr;
				dc_ndx = are_albums.get(i) ? dc_ndx : dc_ndx - cAlbumArt;
				bool val = uartw->getflag(att, dc_ndx);
				if (!i) ovr = val;
				else if (ovr != val) mixedovr = true;
			}
		}
		bool mixedembed = false, embed = false;
		for (size_t i = 0; !mixedembed && i < citems; i++) {
			if (perm_selection[i] != citems) {
				size_t dc_ndx = perm_selection[i];
				af att = are_albums.get(i) ? af::alb_emb : af::art_emb;
				dc_ndx = are_albums.get(i) ? dc_ndx : dc_ndx - cAlbumArt;
				bool val = uartw->getflag(att, dc_ndx);
				if (!i) embed = val;
				else if (embed != val) mixedembed = true;
			}
		}

		mixedvals.resize(3);
		mixedvals.set(0, mixedwrite);
		mixedvals.set(1, mixedovr);
		mixedvals.set(2, mixedembed);

		bool bitems = p_uilist->GetItemCount();
		uAppendMenu(*menu, MF_SEPARATOR, (bselection ? 0 : MF_DISABLED | MF_GRAYED), 0);
		uAppendMenu(*menu, MF_STRING | (bselection ? 0 : MF_DISABLED | MF_GRAYED) | (write || mixedwrite? MF_CHECKED | MF_UNCHECKED : 0), ID_ART_WRITE, "Write");
		uAppendMenu(*menu, MF_STRING | (bselection ? 0 : MF_DISABLED | MF_GRAYED) | (ovr || mixedovr? MF_CHECKED | MF_UNCHECKED : 0), ID_ART_OVR, "Overwrite");
		uAppendMenu(*menu, MF_STRING | (bselection ? 0 : MF_DISABLED | MF_GRAYED) | (embed || mixedembed? MF_CHECKED | MF_UNCHECKED : 0), ID_ART_EMBED, "Embed");
		uAppendMenu(*menu, MF_STRING, ID_ART_RESET, "Reset");

		uAppendMenu(*menu, MF_SEPARATOR, (bitems ? 0 : MF_DISABLED | MF_GRAYED), 0);
		uAppendMenu(*menu, MF_STRING | (is_tile ? MF_CHECKED : 0) | (bitems && !is_tile ? 0 : MF_DISABLED | MF_GRAYED), ID_ART_TILE, "Tile");
		uAppendMenu(*menu, MF_STRING | (!is_tile ? MF_CHECKED : 0) | (bitems && is_tile ? 0 : MF_DISABLED | MF_GRAYED), ID_ART_DETAIL, "Detail");
	}
	catch (...) {}
	return false;
}

LRESULT CTrackMatchingDialog::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {

	bHandled = TRUE;
	HWND hwnd = (HWND)wParam;
	UINT id = ::GetWindowLong(hwnd, GWL_ID);

	if (m_coord.GetCtrIDMode(id) != lsmode::unknown) {

		context_menu_track_show(hwnd, id, lParam);
	}
	else {

		context_menu_form(m_hWnd, lParam);
	}
	return FALSE;
};

void CTrackMatchingDialog::enable(bool is_enabled) {
	::uEnableWindow(GetDlgItem(IDC_BTN_WRITE_TAGS), is_enabled);
	::uEnableWindow(GetDlgItem(IDCANCEL), is_enabled);
	::uEnableWindow(GetDlgItem(IDC_BTN_BACK), is_enabled);
	::uEnableWindow(GetDlgItem(IDC_BTN_TAG_MAPPINGS), is_enabled);
	::uEnableWindow(GetDlgItem(IDC_CHK_REPLACE_ANV), is_enabled);
	::uEnableWindow(GetDlgItem(IDC_BTN_PREVIEW_TAGS), is_enabled);
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

void CTrackMatchingDialog::init_track_matching_dialog() {

	multi_uartwork multi_current;

	if (m_coord.GetFileArtRowLvSize()) {
		multi_current = *m_coord.GetUartwork();
	}
	else {
		multi_current = CONF_MULTI_ARTWORK;
	}

	multi_uartwork multi_defaults = multi_uartwork(CONF, m_tag_writer->GetRelease());
	
	bool managed_artwork = !(multi_current == multi_defaults);
	bool user_wants_skip = HIWORD(m_conf.album_art_skip_default_cust) & ARTSAVE_SKIP_USER_FLAG;

	if (managed_artwork && !user_wants_skip) {
		m_tristate.SetTristate(BST_INDETERMINATE, BST_INDETERMINATE);
	}
	else {
		m_tristate.RemoveTristate();
		m_tristate.SetBistate(user_wants_skip ? BST_CHECKED : BST_UNCHECKED);
	}
	return;
}

void CTrackMatchingDialog::show(const int skip_tristate) {

	::CheckDlgButton(m_hWnd, IDC_CHK_SKIP_ARTWORK, skip_tristate);
	MyCDialogImpl::show();
}

//override
void CTrackMatchingDialog::show() {
	
	m_conf = CConf(CONF);
	m_conf.SetName("TrackMatch");

	init_track_matching_dialog();
	MyCDialogImpl::show();

}

//override
void CTrackMatchingDialog::hide() {
	MyCDialogImpl::hide();

}

void CTrackMatchingDialog::go_back() {
	pfc::string8 dlg_release_id, dlg_artist_id, dlg_artist_name;
	pfc::string8 item_release_id, item_artist_id, item_artist;
	
	metadb_handle_list items = m_tag_writer->m_finfo_manager->items;

	//item release & artist
	if (m_tag_writer->m_finfo_manager->items.get_size()) {
		file_info_impl finfo;
		metadb_handle_ptr item = items[0];
		item->get_info(finfo);
		g_discogs->file_info_get_tag(item, finfo, TAG_RELEASE_ID, item_release_id);
		g_discogs->file_info_get_tag(item, finfo, TAG_ARTIST_ID, item_artist_id);
		if (!item_artist_id.get_length()) {
			item_artist.set_string(file_info_get_artist_name(finfo, item));
		}
	}

	// prepare find release dlg controls
	auto tw_release = m_tag_writer->GetRelease();
	if (tw_release) {
		dlg_artist_id = tw_release->artists[0]->full_artist->id;
		dlg_artist_name = tw_release->artists[0]->full_artist->name;
		dlg_release_id = tw_release->id;
	}

	CFindReleaseDialog* find_release_dlg = g_discogs->find_release_dialog;
	HWND hwndReleases = g_discogs->find_release_dialog->m_hWnd;

	UINT next_default = IDC_BTN_PROCESS_RELEASE;
	
	if (dlg_release_id.get_length()) {
		
		find_release_dlg->setitems(items);
		HWND release_url_edit = ::GetDlgItem(hwndReleases, IDC_RELEASE_URL_TEXT);
		uSetWindowText(release_url_edit, dlg_release_id.get_ptr());

		if (!dlg_artist_id.get_length()) {
			next_default = IDC_BTN_SEARCH;
			HWND search_edit = ::GetDlgItem(hwndReleases, IDC_SEARCH_TEXT);
			uSetWindowText(search_edit, dlg_artist_name.get_ptr());
		}
	}

	// set default button

	HWND hwnd_next_default = ::GetDlgItem(hwndReleases, next_default);
	uSendMessage(hwndReleases, WM_NEXTDLGCTL, (WPARAM)hwnd_next_default, TRUE);

	find_release_dlg->show();
	find_release_dlg = nullptr;
	DestroyWindow();
}


LRESULT CTrackMatchingDialog::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	destroy_all();
	return TRUE;
}

LRESULT CTrackMatchingDialog::OnUpdateSkipButton(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	int not_user_skip = HIWORD(m_conf.album_art_skip_default_cust);
	not_user_skip &= ~(ARTSAVE_SKIP_USER_FLAG);
	m_conf.album_art_skip_default_cust =
		MAKELPARAM(LOWORD(m_conf.album_art_skip_default_cust), not_user_skip);
	init_track_matching_dialog();
	return TRUE;
}

void CTrackMatchingDialog::destroy_all() {

	if (g_discogs) {
		CFindReleaseDialog* dlg = g_discogs->find_release_dialog;
		if (dlg) {
			dlg->destroy();	
		}
		else {
			log_msg("Tracing exit sequence... find release dialog gone.");
		}
	}
	else {
		log_msg("Tracing exit sequence... g_discogs gone.");
	}

	DestroyWindow();
}

LRESULT CTrackMatchingDialog::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {

	m_coord.PullConf(get_mode(), true, &m_conf);
	m_coord.PullConf(get_mode(), false, &m_conf);

	pushcfg();

	cfg_dialog_position_track_matching_dlg.RemoveWindow(m_hWnd);

	return 0;
}

void CTrackMatchingDialog::showtitle() {
	if (CONF.awt_alt_mode()) { uSetWindowText(m_hWnd, "Track Matching & Artwork +"); }
	else { uSetWindowText(m_hWnd, "Track Matching & Artwork"); }
}