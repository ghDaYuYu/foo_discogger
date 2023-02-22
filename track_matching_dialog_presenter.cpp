#pragma once
#include "stdafx.h"

#include <gdiplus.h>
#include "CGdiPlusBitmap.h"

#include "track_matching_dialog_presenter.h"

using namespace Gdiplus;

// coord presenters

void coord_presenters::Initialize(HWND hparent, const foo_conf& dlgconf) {

	m_hWnd = hparent;
	m_conf = dlgconf;

	binomials.emplace_back(m_discogs_track_libui_presenter, m_file_track_libui_presenter);
	binomials.emplace_back(m_discogs_art_presenter, m_file_art_presenter);

	binomials_it bin_it = binomials.begin();

	form_mode[lsmode::tracks_ui] = bin_it;
	form_mode[lsmode::art] = ++bin_it;
}

void coord_presenters::InitFormMode(lsmode mode, UINT id_lvleft, UINT id_lvright) {
	form_mode[mode]->first.Init(m_hWnd, m_tag_writer, id_lvleft);
	form_mode[mode]->second.Init(m_hWnd, m_tag_writer, id_lvright);
}
void coord_presenters::SetTagWriter(TagWriter_ptr tag_writer) {
	m_tag_writer = tag_writer;
	m_tag_writer_release = tag_writer->release;
	
	m_discogs_track_libui_presenter.SetTagWriter(tag_writer);
	m_file_track_libui_presenter.SetTagWriter(tag_writer);

	m_discogs_art_presenter.SetTagWriter(tag_writer);
	m_discogs_art_presenter.SetRelease(m_tag_writer_release);

	m_file_art_presenter.SetRelease(m_tag_writer_release);
}

const foo_conf & coord_presenters::Get_Conf() {

	return m_conf;
}

void coord_presenters::Set_Conf(foo_conf cfg) {
	m_conf = std::move(cfg);
}

std::vector<pfc::string8> coord_presenters::Get_Titles(lsmode mode, bool tracks) {
	std::vector<pfc::string8> titles;
	auto bin = form_mode[mode];
	auto& pres = tracks ? bin->first : bin->second;
	return pres.m_vtitles;
}

size_t coord_presenters::GetDiscogsTrackUiAtLvPos(size_t list_position, track_it& out) {
	var_it_t var_it;
	size_t res = m_discogs_track_libui_presenter.GetVRow(list_position, var_it);
	if (res == ~0) return ~0;
	try {
		res = std::get<0>(*var_it).first;
		out = std::get<0>(*var_it).second;
	}
	catch (std::bad_variant_access&) {
		return pfc_infinite;
	}
	return res;
}

size_t coord_presenters::GetFileTrackUiAtLvPos(size_t list_position, file_it& out) {
	var_it_t var_it;
	size_t res = m_file_track_libui_presenter.GetVRow(list_position, var_it);
	if (res == ~0) return ~0;

	try {
		auto tryvar = std::get<1>(*var_it);
		res = tryvar.first;
		out = tryvar.second;
	}
	catch (std::bad_variant_access&) {
		return pfc_infinite;
	}
	return res;
}

size_t coord_presenters::GetTrackArtAtLvPos(size_t list_position, getimages_it& out) {
	var_it_t var_it;
	size_t res = m_discogs_art_presenter.GetVRow(list_position, var_it);
	if (res == ~0) return ~0;

	try {
		res = std::get<2>(*var_it).first;
		out = std::get<2>(*var_it).second;
	}
	catch (std::bad_variant_access&) {
		return pfc_infinite;
	}
	return res;
}

size_t coord_presenters::GetFileArtAtLvPos(size_t list_position, getimages_file_it& out) {
	var_it_t var_it;
	size_t res = m_file_art_presenter.GetVRow(list_position, var_it);
	if (res == ~0) return ~0;

	try {
		res = std::get<3>(*var_it).first;
		out = std::get<3>(*var_it).second;
	}
	catch (std::bad_variant_access&) {
		return pfc_infinite;
	}
	return res;
}

void coord_presenters::InitUiList(HWND hwnd, lsmode mode, bool tracks, CListControlOwnerData* uilist) {
	
	auto bin = form_mode[mode];
	track_presenter* uipres;

	if (tracks) {
		uipres = (track_presenter*)&bin->first;
	}
	else {
		uipres = (track_presenter*)&bin->second;
	}

	uipres->SetUIList(uilist);
}

std::pair<size_t, presenter*> coord_presenters::columnHitTest(CPoint point) {
	
	auto bin = form_mode[lsmode::tracks_ui];

	size_t icol = pfc_infinite;
	
	//testhit tracks

	track_presenter* uipres = dynamic_cast<track_presenter*>(&bin->first);

	try {
		icol = uipres->columnHitTest(point);
	}
	catch (...) { return std::pair(pfc_infinite, nullptr); }

	if (icol != pfc_infinite)
		return std::pair(icol, uipres);

	//testhit files

	uipres = dynamic_cast<track_presenter*>(&bin->second);

	try {
		icol = uipres->columnHitTest(point);
	}
	catch (...) { return std::pair(pfc_infinite, nullptr); }

	if (icol != pfc_infinite)
		return std::pair(icol, uipres);
	
	return std::pair(pfc_infinite, nullptr);
}

void coord_presenters::SetUiColumnFormat(size_t icol, presenter* pres, size_t fmt) {
	
	try {
		track_presenter* uipres = dynamic_cast<track_presenter*>(pres);
		uipres->SetHeaderColumnFormat(icol, fmt);
	}
	catch (...) {}
}

size_t coord_presenters::GetUiColumnFormat(size_t icol, presenter *pres) {
	size_t fmt = pfc_infinite;

	try {	
		track_presenter* uipres = dynamic_cast<track_presenter*>(pres);
		fmt = uipres->GetHeaderColumnFormat(icol);
	}
	catch (...) {}

	return fmt;
}

bool coord_presenters::ColumnRowToggle() {

	auto& bin = form_mode[lsmode::tracks_ui];

	track_presenter* uipres = dynamic_cast<track_presenter*>(&bin->first);
	uipres->SetUIList();

	uipres = dynamic_cast<track_presenter*>(&bin->second);
	uipres->SetUIList();

	return true;
}

size_t coord_presenters::ListUserCmd(HWND hwnd, lsmode mode, int cmd,
	bit_array_bittable cmdmask, bit_array_bittable are_albums, pfc::array_t<size_t> order, bool cmdmod) {
	
	size_t nextfocus = ~0;

	presenter* pres = nullptr;
	auto bin = form_mode[mode];

	if (bin->first.GetListView() == hwnd)
		pres = &bin->first;
	else if (bin->second.GetListView() == hwnd)
		pres = &bin->second;
	else return nextfocus;

	switch (cmd) {

		case ID_REMOVE: {
			
			//context menu or keyboard

			nextfocus = ListUserCmdDELETE(hwnd, mode, cmd, cmdmask, are_albums, cmdmod);
			break;
		}
	}
	return nextfocus;
}

size_t coord_presenters::ListUserCmdDELETE(HWND hwnd, lsmode mode, int cmd, bit_array_bittable cmdmask, bit_array_bittable are_albums, bool cmdmod) {
	t_size nextfocus = pfc_infinite;

	presenter* pres = nullptr;
	auto bin = form_mode[mode];

	if (bin->first.GetListView() == hwnd)
		pres = &bin->first;
	else if (bin->second.GetListView() == hwnd)
		pres = &bin->second;
	else return nextfocus;

	if (mode == lsmode::art) {
		if (hwnd == bin->first.GetListView()) {
			if (!are_albums.size()) {
				((discogs_artwork_presenter*)pres)->GetAreAlbumMask(are_albums);
			}
		}	
	}

	bool bcrop = cmdmod;
	const size_t count = pres->GetDataLvSize();
	const size_t cAlbumArt = m_tag_writer->GetArtCount(art_src::alb);

	t_size del = 0;
	size_t n = cmdmask.find(true, 0, count);

	for (size_t i = n; i < count; i++) {
		if (cmdmask.get(i)) {

			size_t ndx_deleted = pres->DeleteLvRow(i - del);

			if (mode == lsmode::art) {
				if (pres == &bin->first) {
					multi_uartwork* multi_uart = m_discogs_art_presenter.GetUartwork();

					af att_save, att_emb;
					if (are_albums.get(i)) {
						att_save = af::alb_sd;
						att_emb = af::alb_emb;
					}
					else {
						att_save = af::art_sd;
						att_emb = af::art_emb;
					}
					ndx_deleted = are_albums.get(i) ? ndx_deleted : ndx_deleted - cAlbumArt;
					multi_uart->setflag(att_save, ndx_deleted, false);
					multi_uart->setflag(att_emb, ndx_deleted, false);
				}
			}
			if (ndx_deleted != pfc_infinite) nextfocus = i;
			del++;
		}
		else
		{
			if (bcrop && nextfocus == pfc_infinite)
				nextfocus = i;
		}
	}
	return nextfocus;
}

size_t coord_presenters::ListUserCmdMOVE(HWND hwnd, lsmode mode, int cmd, bit_array_bittable cmdmask, bit_array_bittable are_albums, pfc::array_t<size_t> order, bool cmdmod) {

	presenter* pres = nullptr;
	auto bin = form_mode[mode];

	if (bin->first.GetListView() == hwnd) {
		pres = &bin->first;
	}
	else if (bin->second.GetListView() == hwnd) {
		pres = &bin->second;
	}
	else {
		return pfc_infinite;
	}

	bool bcrop = cmdmod;
	const size_t count = pres->GetDataLvSize();
	const size_t cAlbumArt = m_tag_writer->GetArtCount(art_src::alb);
	pres->ReorderMapItem(order.get_ptr(), pres->GetDataLvSize());

	return pfc_infinite;
}
bool coord_presenters::ShowFormMode(lsmode mode, bool showleft, bool showright , bool populate /*= true*/, bool invalidate /*= false*/) {
	PFC_ASSERT(mode == lsmode::tracks_ui || mode == lsmode::art);

	presenter* pres_tracks;
	presenter* pres_files;

	pres_tracks = &form_mode[mode]->first;
	pres_files = &form_mode[mode]->second;

	if (mode == lsmode::tracks_ui) {
		//insert track mapping
		if (populate)	populate_track_ui_mode();
	}
	else {
		//insert artwork mapping
		if (populate)	populate_artwork_mode();
		
		if (invalidate) {

			((track_presenter*)&m_discogs_art_presenter)->ListInvalidate();
			((track_presenter*)&m_file_art_presenter)->ListInvalidate();
		}
	}

	return true/*populate*/;
}

bool coord_presenters::Show(bool showleft, bool showright) {
	return ShowFormMode(m_lsmode, showleft, showright /*, false*/);
}

bool coord_presenters::Invalidate(bool showleft, bool showright) {
	return ShowFormMode(m_lsmode, showleft, showright, false, true);
}

void coord_presenters::SetModeTile(bool enable) {

	discogs_artwork_presenter* dap = (discogs_artwork_presenter*) & form_mode[lsmode::art]->first;
	files_artwork_presenter* fap = (files_artwork_presenter*) & form_mode[lsmode::art]->second;
	dap->SetTile(enable);
	fap->SetTile(enable);
}

bool coord_presenters::isTile() {
	discogs_artwork_presenter* dap = (discogs_artwork_presenter*)&form_mode[lsmode::art]->first;
	files_artwork_presenter* fap = (files_artwork_presenter*)&form_mode[lsmode::art]->second;
	return dap->IsTile() || fap->IsTile();
}

void coord_presenters::SetMode(lsmode mode) {
	PFC_ASSERT(mode == lsmode::tracks_ui || mode == lsmode::art);

	lsmode oldMode = m_lsmode;
	m_lsmode = mode;
}

void coord_presenters::Reset(HWND hlist, lsmode mode) {

	presenter* pres = nullptr;
	auto& fm = form_mode[mode];

	if (fm->first.GetListView() == hlist) {
		pres = &fm->first;
	}

	if (fm->second.GetListView() == hlist) {
		pres = &fm->second;
	}

	if (pres) {
		pres->Reset();
		ShowFormMode(mode, 0, 0, true, true);
	}
}

void coord_presenters::swap_map_elements(HWND hwnd, const size_t key1, const size_t key2, lsmode mode)
{
	presenter* pres = nullptr;

	auto& fm = form_mode[mode];
	HWND debug = fm->first.GetListView();

	if (fm->first.GetListView() == hwnd) {
		pres = &fm->first;
	}
	if (fm->second.GetListView() == hwnd) {
		pres = &fm->second;
	}

	if (pres) {
		pres->SwapMapItem(key1, key2);
	}
}

void coord_presenters::reorder_map_elements(HWND hwnd, size_t const* order, size_t count, lsmode mode) {

	presenter* pres = nullptr;
	auto fm = form_mode[lsmode::tracks_ui];

	if (fm->first.GetListView() == hwnd)
		pres = &fm->first;

	else if (fm->second.GetListView() == hwnd)
		pres = &fm->second;

	fm = form_mode[lsmode::art];

	if (fm->first.GetListView() == hwnd)
		pres = &fm->first;

	else if (fm->second.GetListView() == hwnd)
		pres = &fm->second;
	
	if (pres) pres->ReorderMapItem(order, count);
}

void coord_presenters::PullConf(lsmode mode, bool tracks, foo_conf* out_conf) {

	m_conf = CConf(*out_conf);

	PFC_ASSERT(mode == lsmode::tracks_ui || mode == lsmode::art);

	if (tracks)
		form_mode[mode]->first.build_cfg_columns();
	else
		form_mode[mode]->second.build_cfg_columns();

	*out_conf = CConf(m_conf);
}

void coord_presenters::PushConf(lsmode mode, bool tracks, bool loadwoas) {

	track_presenter* pres;

	if (tracks) {
		if (loadwoas) {			
			form_mode[mode]->first.define_columns();
		}
		pres = (track_presenter*)&form_mode[mode]->first;
		pres->SetUIList();

	}
		
	else {
		if (loadwoas) {
			form_mode[mode]->second.define_columns();;
		}
		pres = (track_presenter*)&form_mode[mode]->second;
		pres->SetUIList();
	}
}

//	PRESENTER

presenter::presenter(coord_presenters* coord) : presenter() {

	mm_hWnd = NULL;
	m_coord = coord;
	mp_conf = coord->get_conf();

	m_listID = 0;
	m_loaded = false;
	//m_dwHeaderStyle = 0L;
	m_row_height = false;

	m_tag_writer = nullptr;
	m_release = nullptr;
	m_conf_col_woa = {};
}

void presenter::set_lv_images_lists() {

	return;
}

void presenter::Init(HWND hDlg, TagWriter_ptr tag_writer, UINT ID) {

	mm_hWnd = hDlg;
	m_tag_writer = tag_writer;
	m_release = tag_writer->release;

	m_listID = ID;

	define_columns();
}

void track_presenter::set_row_height(bool assign)
{
	CImageList* imgList = get_imagelists().first;
	CImageList* imgListSmall = get_imagelists().second;

	if (imgList->m_hImageList) {
		SIZE size;
		int tmpc = imgList->GetImageCount();
		int tmps = imgList->GetIconSize(size);
	}

	if (imgList->m_hImageList == NULL) {
		imgList->Create(150, 150, ILC_COLOR16, 0, 0);
	}
	if (imgListSmall->m_hImageList == NULL) {
		imgListSmall->Create(48, 48, ILC_COLOR16, 0, 0);
	}

	if (assign) {
		set_lv_images_lists();
		m_row_height = true;
		ListView_RedrawItems(GetListView(), 0, ListView_GetItemCount(GetListView()));
	}
}
//	DISCOGS TRACK LIBPPUI PRESENTER

void discogs_track_libui_presenter::AddRow(std::any track) {

	populated = true;

	if (!m_vtracks.size()) {
		m_vtracks.reserve(m_tag_writer->m_track_mappings.get_count());
	}
	
	m_vtracks.push_back(std::any_cast<track_match_t>(track));
	m_lvtracks.emplace(m_lvtracks.end(), std::pair<size_t, track_it>(m_lvtracks.size(), --m_vtracks.end()));
}

size_t discogs_track_libui_presenter::GetVRow(size_t list_position, var_it_t& out) {

	if (list_position >= GetDataLvSize()) {

		return ~0;	
	}

	std::vector<V>::iterator v_it = m_lvtracks.begin();
	std::advance(v_it, list_position);
	out = v_it;
	return list_position;
}

//override
void discogs_track_libui_presenter::define_columns() {

	m_vtitles = { "Discogs Track", "Length" };
	m_conf_col_woa.clear();
	m_conf_col_woa.push_back(mp_conf->match_tracks_discogs_col1_width);
	m_conf_col_woa.push_back(mp_conf->match_tracks_discogs_col2_width);
}

void discogs_track_libui_presenter::build_cfg_columns() {

	CHeaderCtrl header_ctrl = m_ui_list->GetHeaderCtrl();
	if (!header_ctrl) return;
	bool has_rowid = m_ui_list->GetColumnCount() > 2;
	if (has_rowid) return;

	m_conf_col_woa = build_woas_libppui(m_ui_list, /*m_tile ? 150 / 48 :*/ 1);

	if (m_conf_col_woa.size()) {

		mp_conf->match_tracks_discogs_col1_width = m_conf_col_woa.at(0);
		mp_conf->match_tracks_discogs_col2_width = m_conf_col_woa.at(1);

		size_t icomp = get_extended_style(GetListView());
		mp_conf->match_tracks_discogs_style = icomp;
	}
}

//	FILE TRACK libui PRESENTER

void file_track_libui_presenter::AddRow(std::any file) {

	populated = true;

	if (!m_vfiles.size()) {
		m_vfiles.reserve(m_tag_writer->m_finfo_manager->items.get_count());
	}

	m_vfiles.push_back(std::any_cast<file_match_t>(file));

	file_it f_it = --m_vfiles.end();
	m_lvfiles.emplace(m_lvfiles.end(), std::pair<size_t, file_it>(m_lvfiles.size(), f_it));
}

size_t file_track_libui_presenter::GetVRow(size_t list_position, var_it_t& out) {

	if (list_position >= GetDataLvSize()) {

		return ~0;
	}

	std::vector<V>::iterator v_it = m_lvfiles.begin();
	std::advance(v_it, list_position);
	out = v_it;
	return list_position;
}

//override
void file_track_libui_presenter::define_columns() {

	m_vtitles = { "Local File", "Length" };
	m_conf_col_woa.clear();
	m_conf_col_woa.push_back(mp_conf->match_tracks_files_col1_width);
	m_conf_col_woa.push_back(mp_conf->match_tracks_files_col2_width);
}

void file_track_libui_presenter::build_cfg_columns() {

	CHeaderCtrl header_ctrl = m_ui_list->GetHeaderCtrl();
	if (!header_ctrl) return;
	bool has_rowid = m_ui_list->GetColumnCount() > 2;
	if (has_rowid) return;

	m_conf_col_woa = build_woas_libppui(m_ui_list, /*m_tile ? 150 / 48 :*/ 1);

	if (m_conf_col_woa.size()) {

		mp_conf->match_tracks_files_col1_width = m_conf_col_woa.at(0);
		mp_conf->match_tracks_files_col2_width = m_conf_col_woa.at(1);

	}
	size_t icomp = get_extended_style(GetListView());
	mp_conf->match_tracks_files_style = icomp;
}

void presenter::update_imagelist(size_t img_ndx, size_t max_img, std::pair<HBITMAP, HBITMAP> hRES) {

	auto hInst = core_api::get_my_instance();

	CGdiPlusBitmapResource gdip_image;
	HBITMAP hBmDefaultSmall, hBmDefaultMini;

	//default 150x150 and 48x48
	gdip_image.Load(MAKEINTRESOURCE(get_icon_id(LVSIL_NORMAL)), L"PNG", hInst);
	Status res_get = gdip_image.m_pBitmap->GetHBITMAP(Color(255, 255, 255)/*Color::Black*/, &hBmDefaultSmall);
	DeleteObject(gdip_image);
	gdip_image.Load(MAKEINTRESOURCE(get_icon_id(LVSIL_SMALL)), L"PNG", hInst);
	res_get = gdip_image.m_pBitmap->GetHBITMAP(Color(255, 255, 255)/*Color::Black*/, &hBmDefaultMini);
	DeleteObject(gdip_image);

	CImageList* imgList = get_imagelists().first;
	CImageList* imgListSmall = get_imagelists().second;

	bool bnew_list = false;
	int count = 0;

	if (imgList->m_hImageList == NULL || imgListSmall->m_hImageList == NULL) {
		set_row_height(false);
		bnew_list = true;;
	}

	bool bupdate = img_ndx != pfc_infinite /*&& hRES.first != NULL && hRES.second != NULL*/;

	if (!bupdate && !bnew_list) {
		DeleteObject(hBmDefaultSmall);
		DeleteObject(hBmDefaultMini);
		DeleteObject(hRES.first);
		DeleteObject(hRES.second);
		return;
	}

	for (size_t i = 0; i < max_img; i++) {
		if (i == img_ndx) {

			if (bnew_list) {

				//note: image higher than its width will fail, solution: add/replace
				//note: 48x48 version was previously scaled
				int res = imgList->Add(hBmDefaultSmall);
				res = imgListSmall->Add(hBmDefaultMini);
				if (hRES.first)
					res = imgList->Replace(i, hRES.first, 0);
				if (hRES.second)
					res = imgListSmall->Replace(i, hRES.second, 0);				
			}
			else {
				int res = 0;
				if (hRES.first)
					res = imgList->Replace(count, hRES.first, 0);
				if (hRES.second)
					res = imgListSmall->Replace(count, hRES.second, 0);
			}

		}
		else {
			if (bnew_list) {
				if (bnew_list) {

					int sa = imgList->GetImageCount();
					int sb = imgListSmall->GetImageCount();

					imgList->Add(hBmDefaultSmall);
					imgListSmall->Add(hBmDefaultMini);

					sa = imgList->GetImageCount();
					sb = imgListSmall->GetImageCount();
					sa = imgList->GetImageCount();
				}
			}
		}
		count++;
	}

	IMAGEINFO ii;
	auto bRetVal = imgList->GetImageInfo(0, &ii);
	IMAGEINFO iii;
	auto bRetVal2 = imgListSmall->GetImageInfo(0, &iii);


	BOOL bres;

	bres = DeleteObject(hBmDefaultSmall);
	bres = DeleteObject(hBmDefaultMini);

	//non-zero = ok
	
	if (hRES.first)
		bres = DeleteObject(hRES.first);
	if (hRES.second)
		bres = DeleteObject(hRES.second);
}

//	TRACK PRESENTER

void track_presenter::SetUIList(CListControlOwnerData* ui_replace_list) {
	if (ui_replace_list) {
		m_ui_list = ui_replace_list;
	}

	LPARAM woa = 0;
	size_t col_align, col_width;

	std::vector<int> vorder;
	vorder.resize(m_vtitles.size());

	auto cols = m_ui_list->GetColumnCount();
	if (cols == 3) {
		m_ui_list->DeleteColumn(2,true);
		return;
	}
	else if (cols == 2) {
		auto scw = GetSystemMetrics(SM_CXVSCROLL);
		m_ui_list->AddColumnEx("#", scw * 3, HDF_CENTER, true);
		return;
	}
	
	for (size_t walk = 0; walk < m_conf_col_woa.size(); walk++) {

		LPARAM woa = m_conf_col_woa[walk];
		vorder[walk] = LOWORD(woa) % 10;
		col_align = LOWORD(woa) / 10;
		auto dbg = HIWORD(woa);
		col_width = HIWORD(woa);
		//set first col to auto-width
		if (walk == 0) {
			col_width = UINT32_MAX;
		}
		else {
			if (col_width == 0) {
				col_width = DEF_TIME_COL_WIDTH;
			}
		}

		//width pixels at 96dpi
		m_ui_list->AddColumnEx(m_vtitles.at(walk), col_width == UINT32_MAX ? UINT32_MAX : col_width, col_align == 0 ? HDF_LEFT : col_align);
	}

	//default config value is all zeros, fill with ascending values
	if (std::find(vorder.begin(), vorder.end(), 1) == vorder.end()) {
		int c = 0;
		for (int& v : vorder) v = c++;
	}

	CHeaderCtrl header = m_ui_list->GetHeaderCtrl();
	header.SetOrderArray((int)vorder.size(), &vorder[0]);
}

void track_presenter::SetHeaderColumnFormat(size_t icol, size_t fmt) {

	CHeaderCtrl header = m_ui_list->GetHeaderCtrl();
	if (!header) return;

	HDITEM item = {};
	item.mask = HDI_FORMAT;
	header.GetItem(icol, &item);

	item.fmt = item.fmt & ~HDF_JUSTIFYMASK;
	item.fmt = item.fmt | (int)fmt;

	header.SetItem(icol, &item);

	CRect rc;
	m_ui_list->GetClientRect(&rc);
	m_ui_list->RedrawWindow(rc);
}

size_t track_presenter::GetHeaderColumnFormat(size_t icol) {
	
	size_t fmt = pfc_infinite;
	CHeaderCtrl header = m_ui_list->GetHeaderCtrl();
	if (!header) {
		return fmt;
	}

	HDITEM item = {0};
	item.mask = HDI_FORMAT;
	header.GetItem(icol, &item);

	fmt = item.fmt & HDF_JUSTIFYMASK;
	return fmt;
}

size_t track_presenter::columnHitTest(CPoint point) {

	size_t icol = pfc_infinite;
	CHeaderCtrl header = m_ui_list->GetHeaderCtrl();
	if (!header) {
		return icol;
	}

	::ScreenToClient(m_ui_list->m_hWnd, &point);

	HDHITTESTINFO lptest = {0};
	lptest.pt = point;
	header.HitTest(&lptest);

	if (lptest.flags == HHT_ONHEADER) {
		icol = lptest.iItem;
	}	
	return icol;
}

//	DISCOGS ARTWORK PRESENTER

void discogs_artwork_presenter::SetTile(bool enable) {

	build_cfg_columns();
	m_tile = enable;

	this->m_ui_list->DeleteColumns(bit_array_true(), false);
	define_columns();
	SetUIList();
	m_ui_list->Invalidate(true);
}

size_t discogs_artwork_presenter::get_icon_id(size_t iImageList) {

	if (iImageList == LVSIL_SMALL) {
		return IDB_PNG48F;
	}
	else if (iImageList == LVSIL_NORMAL) {
		return IDB_PNG150F;
	}
	else {
		return LVSIL_NORMAL;
	}
}

void discogs_artwork_presenter::AddRow(std::any imagerow) {

	populated = true;
	
	if (!m_vimages.size()) {
		m_vimages.reserve(m_tag_writer->m_track_mappings.get_count());
	}

	ndx_image_info_row_t ndximginfo = std::any_cast<ndx_image_info_row_t>(imagerow);

	if (m_lvimages.size() == 0) {
		//fix no primary album art
		
		pfc::string8 artype = ndximginfo.second.at(0);
		bool fixed = fixPrimary(artype);
	}

	m_vimages.push_back(ndximginfo);
	getimages_it g_it = --m_vimages.end();

	m_lvimages.emplace(m_lvimages.end(), std::pair<size_t, getimages_it>(m_lvimages.size(), g_it) /*pp*/);
}

size_t discogs_artwork_presenter::GetVRow(size_t list_position, var_it_t& out) {

	if (list_position >= GetDataLvSize()) {

		return ~0;	
	}

	std::vector<V>::iterator v_it = m_lvimages.begin();
	std::variant<V> vb(*m_lvimages.begin());

	std::advance(v_it, list_position);
	out = v_it;
	return list_position;
}

void discogs_artwork_presenter::SetUIList(CListControlOwnerData* ui_replace_list) {

	if (ui_replace_list) {
		m_ui_list = ui_replace_list;
	}

	size_t col_align, col_width;

	auto DPI = QueryScreenDPIEx(mm_hWnd);

	std::vector<int> vorder;
	vorder.resize(m_vtitles.size());
	if (m_ui_list->GetColumnCount()) return;

	for (size_t walk = 0; walk < m_conf_col_woa.size(); walk++) {

		LPARAM woa = m_conf_col_woa[walk];

		vorder[walk] = LOWORD(woa) % 10;
		col_align = LOWORD(woa) / 10;

		if (col_align == 0 && walk > 1) col_align = HDF_CENTER;
		
		auto dbg = HIWORD(woa);
		//if tile expand walk 0 and minimize the rest
		col_width = HIWORD(woa);
		if (walk == 0) {
			if (col_width == 0) {
				auto scw = GetSystemMetrics(SM_CXVSCROLL);
				col_width = m_tile ? (150 + scw) : (48 + 2*scw);
			}
		}
		else {
			if (col_width == 0) {
				if (walk < m_conf_col_woa.size() - 1) {
					col_width = DEF_TIME_COL_WIDTH;
				}
				else {
					auto scw = GetSystemMetrics(SM_CXVSCROLL);
					col_width = 2*scw; //index
				}
			}
		}
		m_ui_list->AddColumnEx(m_vtitles.at(walk), col_width == UINT32_MAX ? UINT32_MAX : col_width, col_align == 0 ? HDF_LEFT : col_align);
	}
	if (std::find(vorder.begin(), vorder.end(), 1) == vorder.end()) {
		int c = 0;
		for (int& v : vorder) v = c++;
	}

	CHeaderCtrl header = m_ui_list->GetHeaderCtrl();
	header.SetOrderArray((int)vorder.size(), &vorder[0]);
}

void discogs_artwork_presenter::define_columns() {

	m_vtitles = { "Discogs", "Type", "Dim", "Save", "Overwrite", "Embed", "#"};

	m_conf_col_woa.clear();

	if (!m_tile) {
		m_conf_col_woa.push_back(mp_conf->match_discogs_artwork_ra_width);
		m_conf_col_woa.push_back(mp_conf->match_discogs_artwork_type_width);
		m_conf_col_woa.push_back(mp_conf->match_discogs_artwork_dim_width);
		m_conf_col_woa.push_back(mp_conf->match_discogs_artwork_save_width);
		m_conf_col_woa.push_back(mp_conf->match_discogs_artwork_ovr_width);
		m_conf_col_woa.push_back(mp_conf->match_discogs_artwork_embed_width);
		m_conf_col_woa.push_back(mp_conf->match_discogs_artwork_index_width);
	}
	else {
		m_conf_col_woa.push_back(mp_conf->match_discogs_artwork_tl_ra_width);
		m_conf_col_woa.push_back(mp_conf->match_discogs_artwork_tl_type_width);
		m_conf_col_woa.push_back(mp_conf->match_discogs_artwork_tl_dim_width);
		m_conf_col_woa.push_back(mp_conf->match_discogs_artwork_tl_save_width);
		m_conf_col_woa.push_back(mp_conf->match_discogs_artwork_tl_ovr_width);
		m_conf_col_woa.push_back(mp_conf->match_discogs_artwork_tl_embed_width);
		m_conf_col_woa.push_back(mp_conf->match_discogs_artwork_tl_index_width);
	}
}

void discogs_artwork_presenter::build_cfg_columns() {

	m_conf_col_woa = build_woas_libppui(m_ui_list, 1.0);

	if (m_conf_col_woa.size()) {

		if (!m_tile) {
			mp_conf->match_discogs_artwork_ra_width = m_conf_col_woa.at(0);
			mp_conf->match_discogs_artwork_type_width = m_conf_col_woa.at(1);
			mp_conf->match_discogs_artwork_dim_width = m_conf_col_woa.at(2);
			mp_conf->match_discogs_artwork_save_width = m_conf_col_woa.at(3);
			mp_conf->match_discogs_artwork_ovr_width = m_conf_col_woa.at(4);
			mp_conf->match_discogs_artwork_embed_width = m_conf_col_woa.at(5);
			mp_conf->match_discogs_artwork_index_width = m_conf_col_woa.at(6);
		}
		else {
			mp_conf->match_discogs_artwork_tl_ra_width = m_conf_col_woa.at(0);
			mp_conf->match_discogs_artwork_tl_type_width = m_conf_col_woa.at(1);
			mp_conf->match_discogs_artwork_tl_dim_width = m_conf_col_woa.at(2);
			mp_conf->match_discogs_artwork_tl_save_width = m_conf_col_woa.at(3);
			mp_conf->match_discogs_artwork_tl_ovr_width = m_conf_col_woa.at(4);
			mp_conf->match_discogs_artwork_tl_embed_width = m_conf_col_woa.at(5);
			mp_conf->match_discogs_artwork_tl_index_width = m_conf_col_woa.at(6);
		}
	}
	size_t icomp = get_extended_style(GetListView());
	mp_conf->match_discogs_artwork_art_style = icomp;
}


// local files artwork presenter

size_t files_artwork_presenter::get_icon_id(size_t iImageList) {

	if (iImageList == LVSIL_SMALL) {
		return IDB_PNG48F;
	}
	else if (iImageList == LVSIL_NORMAL) {
		return IDB_PNG150F;
	}
	else {
		return LVSIL_NORMAL;
	}
}

void files_artwork_presenter::AddRow(std::any imagefilerow) {

	populated = true;

	ndx_image_file_info_row_t ndximginfo = std::any_cast<ndx_image_file_info_row_t>(imagefilerow);
	const char* ccheck_art_ids = album_art_ids::name_of(ndximginfo.first.second);
	bool bextend = false;
	if (!m_vimage_files.size()) {
		m_vimage_files.reserve(kMax_Artwork * 2);
	}
	m_vimage_files.push_back(ndximginfo);
	getimages_file_it g_it = --m_vimage_files.end();

	m_lvimage_files.emplace(m_lvimage_files.end(), std::pair<size_t, getimages_file_it>(m_vimage_files.size()-1/*m_lvimage_files.size()*/, g_it) /*pp*/);
}

size_t files_artwork_presenter::GetVRow(size_t list_position, var_it_t& out) {

	if (list_position >= GetDataLvSize()) {
		return ~0;
	}

	std::vector<V>::iterator v_it = m_lvimage_files.begin();
	std::advance(v_it, list_position);

	out = v_it;
	return list_position;
}

void files_artwork_presenter::define_columns() {

	m_vtitles = { "File name", "Dim", "Size", "#"};
	m_conf_col_woa.clear();
	m_conf_col_woa.push_back(mp_conf->match_file_artwork_name_width);
	m_conf_col_woa.push_back(mp_conf->match_file_artwork_dim_width);
	m_conf_col_woa.push_back(mp_conf->match_file_artwork_size_width);
	m_conf_col_woa.push_back(mp_conf->match_file_artwork_index_width);
}

void files_artwork_presenter::build_cfg_columns() {

	build_woas_libppui(m_ui_list,/* m_tile ? 150 / 48 :*/ 1);

	if (m_conf_col_woa.size()) {

		mp_conf->match_file_artwork_name_width = m_conf_col_woa.at(0);
		mp_conf->match_file_artwork_dim_width = m_conf_col_woa.at(1);
		mp_conf->match_file_artwork_size_width = m_conf_col_woa.at(2);
		mp_conf->match_file_artwork_index_width = m_conf_col_woa.at(3);
	}
	size_t icomp = get_extended_style(GetListView());
	mp_conf->match_file_artworks_art_style = icomp;
}

void files_artwork_presenter::update_list_width(bool initcontrols = false) {

	CRect reclist;
	::GetWindowRect(GetListView(), reclist);

	int framewidth = reclist.Width();
	int slot = (framewidth - 20 - 40) / 6;

	size_t icol = 0;
	size_t col_width = slot;

	for (pfc::string8 htitle : m_vtitles) {

		pfc::stringcvt::string_os_from_utf8 labelOS(htitle.c_str());
		DWORD fmtFlags = icol > 0 ? HDF_CENTER : HDF_LEFT;

		if (icol == 0) col_width = slot * 4; //filename
		else col_width = slot;

		if (htitle.equals("#")) col_width = slot / 2;

		LVCOLUMN lvcol = {};
		lvcol.iSubItem = icol;
		lvcol.cx = col_width;
		lvcol.fmt = fmtFlags /*| HDF_BITMAP | LVCFMT_IMAGE*/ /*HDF_STRING*/;
		lvcol.mask = LVCF_WIDTH | LVCF_FMT;

		ListView_SetColumn(GetListView(), icol, &lvcol);

		icol++;
	}
}

void files_artwork_presenter::GetExistingArtwork() {

	metadb_handle_ptr item = m_tag_writer->m_finfo_manager->get_item_handle(0);

	std::vector<std::pair<size_t, pfc::string8>> album_art_prefs;
	album_art_prefs.reserve(album_art_ids::num_types());
	
	std::vector<std::pair<size_t, pfc::string8>>::iterator artist_it;
	std::vector<std::pair<size_t, pfc::string8>>::iterator disc_it;

	size_t artist_num = ~0, disc_num = ~0;
	for (size_t i = 0; i < album_art_ids::num_types(); i++) {
		pfc::string8 file_name = album_art_ids::query_name(i);
		pfc::chain_list_v2_t<pfc::string8> splitfile;
		pfc::splitStringByChar(splitfile, file_name.get_ptr(), ' ');
		if (splitfile.get_count()) file_name = *splitfile.first();
		file_name << ".jpg";

		if (album_art_ids::query_type(i) == album_art_ids::artist) {
			artist_num = i;
			artist_it = album_art_prefs.emplace(album_art_prefs.end(), std::pair(i, file_name));
		} else if (album_art_ids::query_type(i) == album_art_ids::disc) {
			disc_num = i;
			disc_it = album_art_prefs.emplace(album_art_prefs.end(), std::pair(i, file_name));
		}
		else {
			album_art_prefs.push_back(std::pair(i, file_name));
		}
	}
	
	std::swap(album_art_prefs[artist_num], album_art_prefs[disc_num]);

	for (auto fb_art_pref : album_art_prefs) {
		std::pair<pfc::string8, pfc::string8> result;

		pfc::string8 directory;
		file_info_impl info;
		threaded_process_status p_status;
		
		titleformat_hook_impl_multiformat hook(p_status, &m_release);
		CONF.album_art_directory_string->run_hook(item->get_location(), &info, &hook, directory, nullptr);

		result = ReadDimSizeFromFile(directory, fb_art_pref.second);

		pfc::string8 full_path(directory); full_path << "\\" << fb_art_pref.second;

		image_info_t image_info = {full_path, result.first, result.second };

		ndx_image_file_t ndx_image_file = std::pair<size_t, GUID>(fb_art_pref.first, album_art_ids::query_type(fb_art_pref.first));
		ndx_image_file_info_row_t ndximgfileinfo = std::pair(ndx_image_file, image_info);
			
		AddRow(ndximgfileinfo);
	}
}

void files_artwork_presenter::Populate() {
	
	PFC_ASSERT(m_release->id.get_length());

	Reset();

	GetExistingArtwork();
	set_row_height(true);

	set_lv_images_lists();
	populated = true;
}

void files_artwork_presenter::Add_template(GUID template_guid, size_t template_size) {
	
	metadb_handle_ptr item = m_tag_writer->m_finfo_manager->get_item_handle(0);

	for (size_t walk_append = 0; walk_append < template_size; walk_append++) {
		std::pair<pfc::string8, pfc::string8> result;

		pfc::string8 directory;
		file_info_impl info;
		fake_threaded_process_status f_status;

		threaded_process_status p_status;
		titleformat_hook_impl_multiformat hook(p_status, &m_release);

		CONF.album_art_directory_string->run_hook(item->get_location(), &info, &hook, directory, nullptr);
		
		pfc::string8 tmpl_file_name = template_art_ids::name_of(template_guid);

		if (walk_append)
			tmpl_file_name << "_" << walk_append;
		tmpl_file_name << ".jpg";

		result = ReadDimSizeFromFile(directory, tmpl_file_name);

		pfc::string8 full_path(directory); full_path << "\\" << tmpl_file_name;

		image_info_t image_info = { full_path, result.first, result.second };
		size_t tmpl_ndx = ~0;
		for (size_t walk_tmpl = 0; walk_tmpl < template_art_ids::num_types(); walk_tmpl++) {
			if (template_art_ids::query_type(walk_tmpl) == template_guid) {
				tmpl_ndx = walk_tmpl;
				break;
			}
		}

		ndx_image_file_t ndx_image_file = std::pair<size_t, GUID>(tmpl_ndx, template_guid);
		ndx_image_file_info_row_t ndximgfileinfo = std::pair(ndx_image_file, image_info);

		AddRow(ndximgfileinfo);
	}
}
size_t files_artwork_presenter::AddFileArtwork(size_t img_ndx, art_src art_source,
	imgpairs callback_pair_memblock, std::pair<pfc::string8, pfc::string8> temp_file_names) {

	size_t sz_res = pfc_infinite;
	size_t list_param_ndx = img_ndx;
	//reorder disc and artist
	if (img_ndx == 2)
		list_param_ndx = 3;
	else if (img_ndx == 3)
		list_param_ndx = 2;
	else list_param_ndx = img_ndx;
	size_t list_pos = ~0;

	size_t citems = m_lvimage_files.size();
	if (citems) {
		auto find_it = std::find_if(m_lvimage_files.begin(), m_lvimage_files.end(), [&](V const& elem) {

			size_t discogs_img_pos = std::get<3>(elem).first;
			getimages_file_it img_it = std::get<3>(elem).second;
			ndx_image_file_t ndx_image = img_it->first;

			sz_res = ndx_image.first;			
			return (discogs_img_pos == list_param_ndx);
			});

		if (find_it != m_lvimage_files.end()) {
			list_pos = std::distance(m_lvimage_files.begin(), find_it);
		}
	}
	else {
		list_pos = img_ndx;		
	}
	auto param = std::pair(callback_pair_memblock.first.second, callback_pair_memblock.second.second);
	if (list_pos != ~0) {
		update_imagelist(list_param_ndx, album_art_ids::num_types(), param);

		if (m_vicons.size() <= list_param_ndx) m_vicons.resize(list_param_ndx + 1);
		m_vicons.at(list_param_ndx) = std::pair(callback_pair_memblock.first.first, callback_pair_memblock.second.first);

		std::pair<pfc::string8, pfc::string8> prev_temp_file_names = m_vtemp_files[list_param_ndx];
		if (prev_temp_file_names.first.get_length()) {
			uDeleteFile(prev_temp_file_names.first);
			uDeleteFile(prev_temp_file_names.second);
		}

		m_vtemp_files[list_param_ndx] = std::pair(temp_file_names.first, temp_file_names.second);
		update_img_defs(list_param_ndx, sz_res);
	}

	return sz_res;
}
void files_artwork_presenter::update_img_defs(size_t img_ndx, size_t album_art_id) {

	if (album_art_id == pfc_infinite) return; //nothing else to do
	pfc::string8 file_name = album_art_ids::query_name(album_art_id);
	pfc::chain_list_v2_t<pfc::string8> splitfile;
	pfc::splitStringByChar(splitfile, file_name.get_ptr(), ' ');
	if (splitfile.get_count()) file_name = *splitfile.first();
	file_name << ".jpg";

	Release_ptr release = m_tag_writer->release;
	metadb_handle_ptr item = m_tag_writer->m_finfo_manager->get_item_handle(0);

	pfc::string8 directory;
	file_info_impl info;
	fake_threaded_process_status f_status;
	threaded_process_status kk;

	threaded_process_status p_status;
	titleformat_hook_impl_multiformat hook(p_status, &release);
	CONF.album_art_directory_string->run_hook(item->get_location(), &info, &hook, directory, nullptr);

	std::pair<pfc::string8, pfc::string8> result;
	result = ReadDimSizeFromFile(directory, file_name);

	pfc::string8 full_path(directory); full_path << "\\" << file_name;

	//get lv position for this album art id
	auto find_it = std::find_if(m_lvimage_files.begin(), m_lvimage_files.end(), [&](V const& elem) {
		size_t discogs_img_pos = std::get<3>(elem).first;
		getimages_file_it img_it = std::get<3>(elem).second;
		ndx_image_file_t ndx_image = img_it->first;
		size_t art_id = ndx_image.first;

		return (art_id == album_art_id);
		});

	//replace dims and file size
	if (find_it != m_lvimage_files.end()) {
		image_info_t& image_info = std::get<3>(*find_it).second->second;
		image_info = { full_path, result.first, result.second };
	}
}

void files_artwork_presenter::ImageListReset(pfc::array_t<GUID> album_art_ids) {
	
	for (int i = 0; i < album_art_ids.get_count(); i++) {
		
		auto find_it = std::find_if(m_vimage_files.begin(), m_vimage_files.end(), [&](ndx_image_file_info_row_t const& elem) {

			ndx_image_file_t ndx_image = elem.first;
			size_t image_src = ndx_image.first;
			GUID image_guid = ndx_image.second;
			return (image_guid == album_art_ids[i]);
			});

		size_t pos_v, pos_lv;
		if (find_it != m_vimage_files.end()) {
			pos_v = std::distance(m_vimage_files.begin(), find_it);
			auto find_lvit = std::find_if(m_lvimage_files.begin(), m_lvimage_files.end(), [&](V const& elem) {
				size_t discogs_img_pos = std::get<3>(elem).first;
				getimages_file_it img_it = std::get<3>(elem).second;
				ndx_image_file_t ndx_image = img_it->first;
				size_t image_src = ndx_image.first;				
				return (image_src == pos_v);
				});
			if (find_lvit != m_lvimage_files.end()) {
				pos_lv = std::distance(m_lvimage_files.begin(), find_lvit);				
			}

			update_imagelist(pos_v, album_art_ids.get_count(), std::pair<HBITMAP, HBITMAP>(NULL, NULL));
			uDeleteFile(m_vtemp_files[pos_v].first);
			uDeleteFile(m_vtemp_files[pos_v].second);
		}
	}

	BOOL bres = m_lstimg.RemoveAll();
	bres = m_lstimg_small.RemoveAll();
	PFC_ASSERT(m_lstimg.GetImageCount() == 0);

	m_lstimg.Destroy();
	m_lstimg_small.Destroy();

	for (auto tmp_pair : m_vtemp_files) {
		uDeleteFile(tmp_pair.first);
		uDeleteFile(tmp_pair.second);
	}
}

//	DISCOGS ARTWORK PRESENTER

void discogs_artwork_presenter::update_list_width(bool initcontrols = false) {

	CRect reclist;
	::GetWindowRect(GetListView(), reclist);
	int framewidth = reclist.Width();
	int slot = (framewidth - 20 - 40) / 6;

	size_t icol = 0;
	size_t col_width;

	for (pfc::string8 htitle : m_vtitles) {

		pfc::stringcvt::string_os_from_utf8 labelOS(htitle.c_str());

		DWORD fmtFlags = icol > 1 ? HDF_CENTER : HDF_LEFT;
				
		if (htitle.equals("#")) col_width = slot / 2;
		else col_width = slot;
		
		LVCOLUMN lvcol = {};
		lvcol.iSubItem = col_width;
		lvcol.cx = col_width;
		lvcol.fmt = fmtFlags;
		lvcol.mask = LVCF_WIDTH | LVCF_FMT;

		ListView_SetColumn(GetListView(), icol, &lvcol);

		icol++;
	}
}

// Get TagWriter Images

void discogs_artwork_presenter::Populate() {

	Reset();

	pfc::array_t<Discogs::ReleaseArtist_ptr> release_artists = m_release->artists;

	size_t cartist_img = 0;
	for (auto wra : release_artists) {
		cartist_img += wra->full_artist->images.get_count();
	}

	size_t count_release_img = m_release->images.get_count();
	size_t count_artist_img = cartist_img;

	m_vimages.reserve(count_release_img + count_artist_img);

	//album artwork
	for (size_t i = 0; i < count_release_img; i++) {
		image_info_t imageinfo;
		Image_ptr pi = m_release->images[i];
		if (i == 0) {
			pfc::string8 fixedprimary = pi->get_type().print().toString();
			fixPrimary(fixedprimary);
			imageinfo.emplace_back(fixedprimary);
		}
		else {
			imageinfo.emplace_back(pi->get_type().print().toString());
		}
		imageinfo.emplace_back(pi->get_width().print().toString());
		imageinfo.emplace_back(pi->get_height().print().toString());
		imageinfo.emplace_back(pi->get_url().print().toString());
		imageinfo.emplace_back(pi->get_thumbnail_url().print().toString());
		ndx_image_t ndx_image((int)art_src::alb, pi);
		ndx_image_info_row_t image_info_row(ndx_image, imageinfo);
		m_vimages.push_back(image_info_row);
		std::pair<size_t, getimages_it> vrow = std::pair(m_lvimages.size(), --m_vimages.end());
		m_lvimages.emplace(m_lvimages.end(), vrow);

	}

	for (auto wra : release_artists) {
		//artist artwork
		pfc::array_t<Discogs::Image_ptr> full_artist_images = wra->full_artist->images;
		for (size_t i = 0; i < full_artist_images.get_count(); i++) {
			image_info_t imageinfo;
			Image_ptr pi = full_artist_images[i];
			if (!i) {
				pfc::string8 fixedprimary = pi->get_type().print().toString();
				fixPrimary(fixedprimary);
				imageinfo.emplace_back(fixedprimary);
			}
			else {
				imageinfo.emplace_back(pi->get_type().print().toString());
			}
			imageinfo.emplace_back(pi->get_width().print().toString());
			imageinfo.emplace_back(pi->get_height().print().toString());
			imageinfo.emplace_back(pi->get_url().print().toString());
			imageinfo.emplace_back(pi->get_thumbnail_url().print().toString());
			ndx_image_t ndx_image((int)art_src::art, pi);
			ndx_image_info_row_t image_info_row(ndx_image, imageinfo);
			m_vimages.push_back(image_info_row);
			m_lvimages.emplace(m_lvimages.end(), std::pair<size_t, getimages_it>(m_lvimages.size(), --m_vimages.end()));
		}
	}

	populated = true;
	set_row_height(true);
	PopulateConfArtWork();
}

void discogs_artwork_presenter::GetAreAlbumMask(bit_array_bittable &mask) {
	mask.resize(m_vimages.size());
	for (getimages_t::iterator wit = m_vimages.begin(); wit < m_vimages.end(); wit++ ) {
		bool balbum = wit->first.first == (int)art_src::alb;
		mask.set(std::distance(m_vimages.begin(), wit), balbum);
	}
	return;
}

void discogs_artwork_presenter::PopulateConfArtWork() {

	bool bfirstRun = (m_multi_uart == multi_uartwork()); 

	if (bfirstRun) {

		bool bfilematch = CONF_MULTI_ARTWORK.file_match;
		CONF_MULTI_ARTWORK = multi_uartwork(CONF, m_tag_writer->release);
		CONF_MULTI_ARTWORK.file_match = bfilematch;
		m_multi_uart = CONF_MULTI_ARTWORK;
	}

	const size_t count = m_vimages.size();
	size_t album_ndx = ~0;
	size_t ndx = 0;

	for (size_t i = 0; i < count; i++) {

		bool balbum = m_vimages.at(i).first.first == (int)art_src::alb;
		if (balbum) album_ndx = ndx = i;
		else ndx = album_ndx != ~0 ? i - (album_ndx + 1) : i;

		af a_sd = balbum ? af::alb_sd : af::art_sd;
		af a_ovr = balbum ? af::alb_ovr : af::art_ovr;
		af a_emb = balbum ? af::alb_emb : af::art_emb;

		std::vector<pfc::string8> rows = m_vimages.at(i).second;

		pfc::string8 type(rows.at(0));

		pfc::string8 dim(rows.at(1));
		dim << "x" << rows.at(2);

		bool bprimary, bsave, bover, bembed;
		bprimary = isPrimary(type);

		bsave = m_multi_uart.getflag(a_sd, ndx);
		bover = m_multi_uart.getflag(a_ovr, ndx);
		bembed = m_multi_uart.getflag(a_emb, ndx);

		m_multi_uart.setflag(a_sd, ndx, bsave);
		m_multi_uart.setflag(a_ovr, ndx, bover);
		m_multi_uart.setflag(a_emb, ndx, bembed);

		LPARAM param = MAKELPARAM(i, m_vimages.at(i).first.first);
	}
}

bool discogs_artwork_presenter::AddArtwork(size_t img_ndx, art_src artSrc, MemoryBlock small_art) {

	pfc::string8 id;
	size_t thumb_index= ~0;;

	if (artSrc == art_src::alb) {
		id = m_release->id;
		thumb_index = img_ndx;
	}
	else {
		Artist_ptr artist;
		thumb_index = img_ndx;
		if (discogs_interface->img_artists_ndx_to_artist(m_release, img_ndx, artist, thumb_index)) {
			id = artist->id;
		}
	}

	pfc::string8 n8_cache_path_small = Offline::get_thumbnail_cache_path_filenames(
		id, artSrc, LVSIL_NORMAL, true, pfc_infinite)[0];
	
	pfc::string8 n8_cache_path_mini = Offline::get_thumbnail_cache_path_filenames(
		id, artSrc, LVSIL_SMALL, true, pfc_infinite)[0];

	n8_cache_path_small << thumb_index << THUMB_EXTENSION;
	n8_cache_path_mini << thumb_index << THUMB_EXTENSION;

	bool bfolder_ready = Offline::create_offline_subpage_folder(id, artSrc, pfc_infinite, Offline::GetFrom::ArtistReleases, "", true);
	if (!bfolder_ready) {

		return false;

	}

	// prepare bitmaps

	imgpairs hRES = MemoryBlockToTmpBitmap(std::pair(n8_cache_path_small,	n8_cache_path_mini), small_art);
	size_t list_pos = ~0;
	size_t list_param_ndx = artSrc == art_src::alb ? img_ndx : img_ndx + m_tag_writer->release->images.get_count();

	size_t citems = m_lvimages.size();
	if (citems) {
		auto find_it = std::find_if(m_lvimages.begin(), m_lvimages.end(), [&](V const& elem) {
			size_t discogs_img_pos = std::get<2>(elem).first;
			getimages_it img_it = std::get<2>(elem).second;
			ndx_image_t ndx_image = img_it->first;
			size_t image_src = ndx_image.first;
			return (discogs_img_pos == list_param_ndx && image_src == (size_t)artSrc);
			});	
			
		if (find_it != m_lvimages.end()) {
			list_pos = std::distance(m_lvimages.begin(), find_it);
		}
	}
	else {
		list_pos = img_ndx;
	}
	if (list_pos != ~0) {
		auto param = std::pair(hRES.first.second, hRES.second.second);
		update_imagelist(list_param_ndx, m_tag_writer->GetArtCount(), param);

		if (m_vicons.size() < m_tag_writer->GetArtCount()) {
			m_vicons.resize(m_tag_writer->GetArtCount());
		}

		m_vicons.at(list_param_ndx) = std::pair(hRES.first.first, hRES.second.first);
		m_ui_list->ReloadItem(list_param_ndx);
	}
	return true;
}

art_src discogs_artwork_presenter::get_vimages_src_type_at_pos(size_t list_position) {

	if (list_position >= m_vimages.size()) {
		return art_src::unknown;
	}
	else {
		std::variant vv_it = m_lvimages[list_position];
		std::pair<size_t, getimages_it> rowpair = std::get<2>(vv_it);

		getimages_it src_img = rowpair.second;
		ndx_image_t ndx_image = src_img->first;
		return (ndx_image.first ? (art_src)ndx_image.first : art_src::unknown);
	}
}

size_t discogs_artwork_presenter::get_ndx_at_pos(size_t list_position) {
	if (list_position >= m_vimages.size()) {
		return pfc_infinite;
	}	
	else {
		std::variant vv_it = m_lvimages[list_position];
		std::pair<size_t, getimages_it> rowpair = std::get<2>(vv_it);

		return rowpair.first;
	}
}

void discogs_artwork_presenter::display_columns() {
	PopulateConfArtWork();
}

void files_artwork_presenter::display_columns() {
	//..
}

//	COORD PRESENTER

bool coord_presenters::show_artwork_preview(size_t image_ndx, art_src art_source, MemoryBlock small_art) {

	if (small_art.get_count())
		return m_discogs_art_presenter.AddArtwork(image_ndx, art_source, small_art);
	else
		return false;
}

bool coord_presenters::add_file_artwork_preview(size_t image_ndx, art_src art_source,
	imgpairs callback_pair_hbitmaps, std::pair<pfc::string8, pfc::string8> temp_file_names) {

	size_t album_art_id = m_file_art_presenter.AddFileArtwork(image_ndx, art_source, callback_pair_hbitmaps, temp_file_names);
	return true;
}

void coord_presenters::populate_track_ui_mode() {

	lsmode my_mode = lsmode::tracks_ui;

	binomials_it binomial = form_mode[my_mode];
	presenter& pres_tracks = binomial->first;
	presenter& pres_files = binomial->second;
	if (m_discogs_track_libui_presenter.GetPopulated() &&
		m_file_track_libui_presenter.GetPopulated()) {
		return;
	}

	m_discogs_track_libui_presenter.Reset();
	m_file_track_libui_presenter.Reset();

	m_hook.set_custom("DISPLAY_ANVS", m_conf.display_ANVs);
	m_hook.set_custom("REPLACE_ANVS", m_conf.replace_ANVs);

	const size_t ctracks = m_tag_writer->m_track_mappings.get_count();
	const size_t citems = m_tag_writer->m_finfo_manager->items.get_count();

	const size_t cmax = (std::max)(ctracks, citems);

	titleformat_object::ptr lenght_script;
	static_api_ptr_t<titleformat_compiler>()->compile_safe_ex(lenght_script, "%length%");

	int debugrejected = 0;

	for (size_t i = 0; i < cmax; i++) {
		
		if (i >= citems) {

			debugrejected++;

		}
		else {

			file_info& finfo = m_tag_writer->m_finfo_manager->get_item(i);
			auto item = m_tag_writer->m_finfo_manager->items.get_item(i);

			pfc::string8 formatted_name;
			CONF.release_file_format_string->run_simple(item->get_location(), &finfo, formatted_name);

			pfc::string8 formatted_length;
			const char* length_titleformat = "%length%";
			item->format_title(nullptr, formatted_length, titleformat_object_wrapper(length_titleformat), nullptr);

			pfc::string8 display = pfc::string_filename_ext(formatted_name);
			match_info_t file_match_info(display, formatted_length);

			size_t file_index = -1;
			if (i < ctracks) {
				const track_mapping& mapping = m_tag_writer->m_track_mappings[i];
				file_index = mapping.file_index;
			}
			file_match_t file_match(file_match_info, file_index);

			//add track row
			m_file_track_libui_presenter.AddRow(file_match);
		}

		if (i >= ctracks) {

			debugrejected++;

		}
		else {

			ReleaseDisc_ptr disc;
			ReleaseTrack_ptr track;

			const track_mapping& mapping = m_tag_writer->m_track_mappings[i];

			if (mapping.discogs_disc == -1 || mapping.discogs_track == -1) {
				int d, t;
				if (!m_tag_writer->release->find_disc_track(i, &d, &t)) {
					continue;
				}

				disc = m_tag_writer->release->discs[d];
				track = disc->tracks[t];
			}
			else {
				disc = m_tag_writer->release->discs[mapping.discogs_disc];
				track = disc->tracks[mapping.discogs_track];
			}
			
			m_hook.set_release(&m_tag_writer->release);
			m_hook.set_disc(&disc);
			m_hook.set_track(&track);

			if (i == 0) {
				file_info_impl finfo;
				m_tag_writer->m_finfo_manager->items[0]->get_info(finfo);
				
				pfc::string8 local_release_id;

				const char* ch_local_rel = finfo.meta_get("DISCOGS_RELEASE_ID", 0);
				if (ch_local_rel) {
					local_release_id = ch_local_rel;
				}

				bool bdiffid = (local_release_id.get_length() && !(STR_EQUAL(m_tag_writer->release->id, local_release_id)));

				pfc::string8 compact_release;
				CONF.search_master_sub_format_string->run_hook(m_location, &m_info, &m_hook, compact_release, nullptr);

				pfc::string8 release_desc = bdiffid ? "!! " : "";
				release_desc << ltrim(compact_release);
				
				uSetDlgItemText(m_hWnd, IDC_STATIC_MATCH_TRACKING_REL_NAME, release_desc);
			}			
			
			pfc::string8 dc_track_desc, time;
			CONF.release_discogs_format_string->run_hook(m_location, &m_info, &m_hook, dc_track_desc, nullptr);

			if (track->discogs_hidden_duration_seconds) {
				int duration_seconds = track->discogs_duration_seconds + track->discogs_hidden_duration_seconds;
				time = duration_to_str(duration_seconds);
			}
			else {
				time = track->discogs_duration_raw;
			}

			match_info_t track_match_info(dc_track_desc, time);
			track_match_t track_file_match(track_match_info, file_match_nfo{ mapping.discogs_track, mapping.discogs_disc });

			m_discogs_track_libui_presenter.AddRow(track_file_match);
		}
	}
}

void coord_presenters::populate_artwork_mode(size_t select) {

	binomials_it binomial = form_mode[lsmode::art];
	presenter& pres_tracks = binomial->first;
	presenter& pres_files = binomial->second;

	if (m_discogs_art_presenter.GetPopulated() &&
		m_file_art_presenter.GetPopulated()) {
		return;
	}

	if (select == 0 || select == 1)
	{
		m_discogs_art_presenter.Populate();
	}
	if (select == 0 || select == 2)
	{
		m_file_art_presenter.Populate();
	}
}

void coord_presenters::FileArtDeleteImageList(pfc::array_t<GUID> album_art_ids) {
	
	m_file_art_presenter.ImageListReset(album_art_ids);
}

bool coord_presenters::template_artwork_mode(GUID template_guid, size_t template_size, size_t lv_pos, bool check_reqs) {

	if (check_reqs) {
		size_t lv_file_art_size = m_file_art_presenter.GetDataLvSize();
		return lv_file_art_size <= lv_pos;
	}
	else {
		m_file_art_presenter.Add_template(template_guid, template_size);
		return false;
	}
}
