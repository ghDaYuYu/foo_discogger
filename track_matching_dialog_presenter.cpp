#pragma once
#include "stdafx.h"

#include <gdiplus.h>
#include "CGdiPlusBitmap.h"

#include "track_matching_dialog_presenter.h"

using namespace Gdiplus;

///////////////////////////////////////////////
//
//	C O O R D  PRESENTER
//
///////////////////////////////////////////////

void coord_presenters::Initialize(HWND hparent, const foo_discogs_conf* dlgconf) {

	m_hWnd = hparent;
	m_conf = *dlgconf;

	binomials.emplace_back(m_discogs_track_libui_presenter, m_file_track_libui_presenter);
	binomials.emplace_back(m_discogs_art_presenter, m_file_art_presenter);

	binomials_it bin_it = binomials.begin();

	form_mode[lsmode::tracks_ui] = bin_it;
	form_mode[lsmode::art] = ++bin_it;

}

void coord_presenters::InitFormMode(lsmode mode, UINT id_lvleft, UINT id_lvright) {
	form_mode[mode]->first.Init(m_hWnd, m_tag_writer, m_conf, id_lvleft);
	form_mode[mode]->second.Init(m_hWnd, m_tag_writer, m_conf, id_lvright);
}

//todo: tidy up
void coord_presenters::SetTagWriter(TagWriter_ptr tag_writer) {
	m_tag_writer = tag_writer;
	m_tag_writer_release = tag_writer->release;
	
	m_discogs_track_libui_presenter.SetTagWriter(tag_writer);
	m_file_track_libui_presenter.SetTagWriter(tag_writer);

	m_discogs_art_presenter.SetTagWriter(tag_writer);
	m_discogs_art_presenter.SetRelease(m_tag_writer_release);

	m_file_art_presenter.SetRelease(m_tag_writer_release);
}

foo_discogs_conf* coord_presenters::cfgRef() {

	return &m_conf;
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

	if (tracks) {
		discogs_track_libui_presenter* uipres = dynamic_cast<discogs_track_libui_presenter*>(&bin->first);
		uipres->SetUIList(uilist);
	}
	else {
		file_track_libui_presenter* uipres = dynamic_cast<file_track_libui_presenter*>(&bin->second);
		uipres->SetUIList(uilist);
	}
}

std::pair<size_t, presenter*> coord_presenters::HitUiTest(CPoint point) {
	
	auto bin = form_mode[lsmode::tracks_ui];

	size_t icol = pfc_infinite;
	
	//testhit tracks

	track_presenter* uipres = dynamic_cast<track_presenter*>(&bin->first);

	try {
		icol = uipres->HitTest(point);
	}
	catch (...) { return std::pair(pfc_infinite, nullptr); }

	if (icol != pfc_infinite)
		return std::pair(icol, uipres);

	//testhit files

	uipres = dynamic_cast<track_presenter*>(&bin->second);

	try {
		icol = uipres->HitTest(point);
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

void coord_presenters::ListUserCmd(HWND hwnd, lsmode mode, int cmd,
	bit_array_bittable cmdmask, bit_array_bittable are_albums, bool cmdmod) {
	
	presenter* pres = nullptr;
	auto bin = form_mode[mode];
	//for (auto fm : form_mode) {
		if (bin->first.GetListView() == hwnd)
			pres = &bin->first;
		if (bin->second.GetListView() == hwnd)
			pres = &bin->second;
		if (!pres) return;
	//}

	switch (cmd) {
		case ID_REMOVE: {
			
			bool bcrop = cmdmod;
			const size_t count = pres->GetDataLvSize();
			const size_t cAlbumArt = m_tag_writer->release->images.get_count();

			t_size del = 0;
			t_size nextfocus = pfc_infinite;
			size_t n = cmdmask.find(true, 0, count);

			for (size_t i = n; i < count; i++) {
				if (cmdmask.get(i)) {
				
					size_t ndx_deleted = pres->DeleteLvRow(i - del);

					if (mode == lsmode::art) {
						if (pres == &bin->first) {
							//deleting discogs artwork rows
							//change save/embed attribs
							uartwork* uart = m_discogs_art_presenter.GetUartwork();

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
							uart->setflag(att_save, ndx_deleted, false);
							uart->setflag(att_emb, ndx_deleted, false);
						}
					}
					if (nextfocus != pfc_infinite) nextfocus = i;
					del++;
				}
				else
				{
					if (bcrop && nextfocus == pfc_infinite)
						nextfocus = i;
				}
			}
			
			ListView_SetItemCount(hwnd, pres->GetDataLvSize());

			if (!bcrop && ListView_GetItemCount(hwnd) > 0) {
				ListView_SetItemState(hwnd, nextfocus > 0 ? nextfocus - 1 : 0, LVIS_FOCUSED | LVIS_SELECTED, 0x000F);
			}
		} //end case remove
	}
}

void coord_presenters::ShowFormMode(lsmode mode, bool showleft, bool showright /*, bool populate*/) {

	PFC_ASSERT(/*mode == lsmode::tracks ||*/ mode == lsmode::tracks_ui || mode == lsmode::art);

	bool populate = false;
	if (showleft && !form_mode[mode]->first.IsLoaded()) {
		form_mode[mode]->first.Load();
		populate = true;
	}
	if (showright && !form_mode[mode]->second.IsLoaded()) {
		form_mode[mode]->second.Load();
		populate = true;
	}

	presenter* pres_tracks;
	presenter* pres_files;

	pres_tracks = &form_mode[mode]->first;
	pres_files = &form_mode[mode]->second;
		
	HWND hwndpres = pres_tracks->GetListView();

	if (mode == lsmode::tracks_ui) {
		//insert track mapping
		if (populate)	populate_track_ui_mode();
	}
	else {
		//insert artwork mapping
		if (populate)	populate_artwork_mode(); 
	}		

#ifdef DEBUG
	int debugtracks = pres_tracks->GetDataLvSize();
	int debugfiles = pres_files->GetDataLvSize();
#endif

	ListView_SetItemCount(pres_tracks->GetListView(), pres_tracks->GetDataLvSize());
	ListView_SetItemCount(pres_files->GetListView(), pres_files->GetDataLvSize());

}

void coord_presenters::Show(bool showleft, bool showright) {
	ShowFormMode(m_lsmode, showleft, showright /*, false*/);
}

void coord_presenters::SetMode(lsmode mode) {
	
	PFC_ASSERT(/*mode == lsmode::tracks ||*/ mode == lsmode::tracks_ui || mode == lsmode::art);
	
	lsmode oldMode = m_lsmode;
	
	presenter* pres;
	
	pres = &form_mode[oldMode]->first;
	if (pres->IsLoaded()) ListView_DeleteAllItems(pres->GetListView());

	pres = &form_mode[oldMode]->second;
	if (pres->IsLoaded()) ListView_DeleteAllItems(pres->GetListView());

	m_lsmode = mode;

}

void coord_presenters::Reset(HWND hlist, lsmode mode) {
	presenter* pres = nullptr;
	auto fm = form_mode[mode];
	if (fm->first.GetListView() == hlist)
		pres = &fm->first;
	if (fm->second.GetListView() == hlist)
		pres = &fm->second;
	if (!pres) return;

	pres->Reset();
	pres->Unload();

}

void coord_presenters::swap_map_elements(HWND hwnd, const size_t key1, const size_t key2, lsmode mode)
{
	presenter* pres = nullptr;
	auto fm = form_mode[mode];
	HWND debug = fm->first.GetListView();
	if (fm->first.GetListView() == hwnd)
		pres = &fm->first;
	debug = fm->second.GetListView();
	if (fm->second.GetListView() == hwnd)
		pres = &fm->second;
	if (!pres) return;

	pres->SwapMapItem(key1, key2);
}

void coord_presenters::reorder_map_elements(HWND hwnd, size_t const* order, size_t count, lsmode mode) {
	presenter* pres = nullptr;
	auto fm = form_mode[mode];
	HWND debug = fm->first.GetListView();
	if (fm->first.GetListView() == hwnd)
		pres = &fm->first;
	debug = fm->second.GetListView();
	if (fm->second.GetListView() == hwnd)
		pres = &fm->second;
	
	if (pres) pres->ReorderMapItem(order, count);
}

void coord_presenters::PullConf(lsmode mode, bool tracks, foo_discogs_conf* out_conf) {

	PFC_ASSERT(/*mode == lsmode::tracks || */mode == lsmode::tracks_ui || mode == lsmode::art);

	presenter* pres;
	
	if (tracks)
		form_mode[mode]->first.build_cfg_columns(out_conf);
	else
		form_mode[mode]->second.build_cfg_columns(out_conf);

}

///////////////////////////////////////////////
//
//	PRESENTER
//
///////////////////////////////////////////////

void presenter::Load() {
	if (!m_loaded) {
		m_loaded = true;
		display();
	}
}

void presenter::Unload() {
	build_cfg_columns();
	m_loaded = false;
	ListView_DeleteAllItems(GetListView());
}

bool presenter::IsTile() {
	DWORD dwView = ListView_GetView(GetListView());
	return (dwView == ID_ART_TILE);
}

void presenter::set_lv_images_lists() {

	if (!m_lstimg.m_hImageList) return;
	ListView_SetImageList(GetListView(), m_lstimg, LVSIL_NORMAL);
	ListView_SetImageList(GetListView(), m_lstimg_small, LVSIL_SMALL);

}

void presenter::Init(HWND hDlg, TagWriter_ptr tag_writer, foo_discogs_conf& conf, UINT ID) {

	mm_hWnd = hDlg;
	m_tag_writer = tag_writer;
	m_release = tag_writer->release;
	m_conf = conf;
	m_listID = ID;
	//presenter derived override
	define_columns();
}

///////////////////////////////////////////////
//
//	TRACK PRESENTER
//
///////////////////////////////////////////////

void track_presenter::display() {
	//for now we delegate coord presenters InitUiList()
	//insert_columns();
	//set_row_height(true);
}

void track_presenter::set_row_height(bool assign)
{
	int height = 1;

	CImageList* imgList = get_imagelists().first;
	CImageList* imgListSmall = get_imagelists().second;
	
	if (imgList->m_hImageList == NULL) {
		imgList->Create(1, height, ILC_COLOR16, 0, 0);
	}
	if (imgListSmall->m_hImageList == NULL) {
		imgListSmall->Create(1, height, ILC_COLOR16, 0, 0);
	}

	if (assign) {

		set_lv_images_lists();

		//hack our way back to normal row heigth in detail view
		ListView_SetView(GetListView(), LV_VIEW_LIST);
		ListView_SetView(GetListView(), LV_VIEW_DETAILS);
	}
}

void track_presenter::update_list_width(bool initialize) {
	CRect client_rectangle;
	::GetClientRect(GetListView(), &client_rectangle);
	int width = client_rectangle.Width() - 40;

	int c1, c2;
	if (initialize) {
		c2 = DEF_TIME_COL_WIDTH;
	}
	else {
		c2 = ListView_GetColumnWidth(GetListView(), 1);
	}
	c1 = width - c2;
	const int count = ListView_GetColumnCount(GetListView());
	ListView_SetColumnWidth(GetListView(), count - 2, c1);
	ListView_SetColumnWidth(GetListView(), count - 1, c2);
}

///////////////////////////////////////////////
//
//	DISCOGS TRACK LIBPPUI PRESENTER
//
///////////////////////////////////////////////

void discogs_track_libui_presenter::AddRow(std::any track) {

	if (!m_vtracks.size())
		m_vtracks.reserve(m_tag_writer->track_mappings.get_count());
	
	m_vtracks.push_back(std::any_cast<track_match_t>(track));
	m_lvtracks.emplace(m_lvtracks.end(), std::pair<size_t, track_it>(m_lvtracks.size(), --m_vtracks.end()));
}

size_t discogs_track_libui_presenter::GetVRow(size_t list_position, var_it_t& out) {
	if (list_position >= GetDataLvSize()) return ~0;
	std::vector<V>::iterator v_it = m_lvtracks.begin();
	std::advance(v_it, list_position);
	out = v_it;
	return list_position;
}

//override
void discogs_track_libui_presenter::define_columns() {

	m_vtitles = { "Discogs Track", "Length" };
	m_conf_col_woa.clear();
	m_conf_col_woa.push_back(m_conf.match_tracks_discogs_col1_width);
	m_conf_col_woa.push_back(m_conf.match_tracks_discogs_col2_width);

	//m_dwHeaderStyle = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_HEADERDRAGDROP;
	//if (m_conf.match_tracks_discogs_style != 0)
	//	m_dwHeaderStyle |= static_cast<DWORD>(m_conf.match_tracks_discogs_style);
	//ListView_SetExtendedListViewStyleEx(GetListView(), m_dwHeaderStyle, m_dwHeaderStyle);
	//ListView_SetView(GetListView(), LVS_REPORT);
	//track_presenter::define_columns();
}

void discogs_track_libui_presenter::build_cfg_columns(foo_discogs_conf* out_conf) {

	foo_discogs_conf* config = out_conf == nullptr ? &m_conf : out_conf;

	if (IsLoaded())	m_conf_col_woa = build_woas_libppui(m_ui_list);

	if (m_conf_col_woa.size()) {

		config->match_tracks_discogs_col1_width = m_conf_col_woa.at(0);
		config->match_tracks_discogs_col2_width = m_conf_col_woa.at(1);
		size_t icomp = get_extended_style(GetListView());
		config->match_tracks_discogs_style = icomp;
	}
}

///////////////////////////////////////////////
//
//	FILE TRACK libui PRESENTER
//
///////////////////////////////////////////////

void file_track_libui_presenter::AddRow(std::any file) {

	if (!m_vfiles.size())
		m_vfiles.reserve(m_tag_writer->finfo_manager->items.get_count());

	m_vfiles.push_back(std::any_cast<file_match_t>(file));

	file_it f_it = --m_vfiles.end();

	m_lvfiles.emplace(m_lvfiles.end(), std::pair<size_t, file_it>(m_lvfiles.size(), f_it));
}

size_t file_track_libui_presenter::GetVRow(size_t list_position, var_it_t& out) {
	if (list_position >= GetDataLvSize()) return ~0;
	std::vector<V>::iterator v_it = m_lvfiles.begin();
	std::advance(v_it, list_position);
	out = v_it;
	return list_position;
}

//override
void file_track_libui_presenter::define_columns() {

	m_vtitles = { "Local File", "Length" };
	m_conf_col_woa.clear();
	m_conf_col_woa.push_back(m_conf.match_tracks_files_col1_width);
	m_conf_col_woa.push_back(m_conf.match_tracks_files_col2_width);

	//m_dwHeaderStyle = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_HEADERDRAGDROP;
	//if (m_conf.match_tracks_files_style != 0)
	//	m_dwHeaderStyle |= static_cast<DWORD>(m_conf.match_tracks_files_style);
	//ListView_SetExtendedListViewStyleEx(GetListView(), m_dwHeaderStyle, m_dwHeaderStyle);
	//ListView_SetView(GetListView(), LVS_REPORT);
	//track_presenter::define_columns();
}

void file_track_libui_presenter::build_cfg_columns(foo_discogs_conf* out_conf) {

	foo_discogs_conf* config = out_conf == nullptr ? &m_conf : out_conf;

	if (IsLoaded())	m_conf_col_woa = build_woas_libppui(m_ui_list);

	if (m_conf_col_woa.size()) {

		config->match_tracks_files_col1_width = m_conf_col_woa.at(0);
		config->match_tracks_files_col2_width = m_conf_col_woa.at(1);

	}
	size_t icomp = get_extended_style(GetListView());
	config->match_tracks_files_style = icomp;

}

///////////////////////////////////////////////
//
//	ARTWORK PRESENTER
//
///////////////////////////////////////////////

void artwork_presenter::display() {
	insert_columns();
	HWND wnddlg = ::GetParent(GetListView());
	uButton_SetCheck(wnddlg, IDC_TRACK_MATCH_ALBUM_ART, true);
}

void artwork_presenter::update_imagelist(size_t img_ndx, size_t max_img, std::pair<HBITMAP, HBITMAP> hRES) {

	auto hInst = core_api::get_my_instance();

	CGdiPlusBitmapResource m_image;
	HBITMAP hBmDefaultSmall, hBmDefaultMini;

	//default 150x150 and 48x48
	m_image.Load(MAKEINTRESOURCE(get_icon_id(LVSIL_NORMAL)), L"PNG", hInst);
	Status res_get = m_image.m_pBitmap->GetHBITMAP(Color(255, 255, 255)/*Color::Black*/, &hBmDefaultSmall);
	DeleteObject(m_image);
	m_image.Load(MAKEINTRESOURCE(get_icon_id(LVSIL_SMALL)), L"PNG", hInst);
	res_get = m_image.m_pBitmap->GetHBITMAP(Color(255, 255, 255)/*Color::Black*/, &hBmDefaultMini);
	DeleteObject(m_image);

	CImageList* imgList = get_imagelists().first;
	CImageList* imgListSmall = get_imagelists().second;

	bool bnew_list = false;
	int count = 0;

	if (imgList->m_hImageList == NULL || imgListSmall->m_hImageList == NULL) {
		set_row_height(false);
		bnew_list = true;;
	}

	bool bupdate = img_ndx != pfc_infinite;

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

				//int cadebug = imgList->GetImageCount();
				//int cbdebug = imgListSmall->GetImageCount();

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

	BOOL bres;

	bres = DeleteObject(hBmDefaultSmall);
	bres = DeleteObject(hBmDefaultMini);

	//non-zero = ok
	
	if (hRES.first)
		bres = DeleteObject(hRES.first);
	if (hRES.second)
		bres = DeleteObject(hRES.second);
}

void artwork_presenter::set_row_height(bool assign)
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

// <type, dim>
std::pair<pfc::string8, pfc::string8> artwork_presenter::get_tag_writer_img_finfo(art_src src, size_t list_position) {

	Release_ptr tag_writer_release = m_tag_writer->release;

	pfc::array_t<Image_ptr> images;

	size_t ndx = ~0;;

	if (src == art_src::alb) {
		images = m_tag_writer->release->images;
		ndx = list_position;
	}
	else {
		images = m_tag_writer->release->artists[0]->full_artist->images;
		ndx = list_position - m_tag_writer->release->images.get_count();
	}

	pfc::string8 dim(images[ndx]->width);
	dim << "x" << images[ndx]->height;
	return std::pair(images[ndx]->type.get_ptr(), dim.get_ptr());

}

///////////////////////////////////////////////
//
//	TRACK PRESENTER
//
///////////////////////////////////////////////

void track_presenter::SetUIList(CListControlOwnerData* ui_list) {

	m_ui_list = ui_list;

	LPARAM woa;
	size_t col_align, col_width;
	
	auto DPI = QueryScreenDPIEx(mm_hWnd);

	std::vector<int> vorder;
	vorder.resize(m_vtitles.size());

	for (size_t walk = 0; walk < m_conf_col_woa.size(); walk++) {

		LPARAM woa = m_conf_col_woa[walk];
		vorder[walk] = LOWORD(woa) % 10;
		col_align = LOWORD(woa) / 10;
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
		ui_list->AddColumnEx(m_vtitles.at(walk), col_width == UINT32_MAX ? UINT32_MAX : col_width, col_align == 0 ? HDF_LEFT : col_align);
	}

	//default config value is all zeros, fill with ascending values
	if (std::find(vorder.begin(), vorder.end(), 1) == vorder.end()) {
		int c = 0;
		for (int& v : vorder) v = c++;
	}

	CHeaderCtrl header = ui_list->GetHeaderCtrl();
	header.SetOrderArray((int)vorder.size(), &vorder[0]);
}

void track_presenter::insert_columns() {

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
	if (!header) return fmt;

	HDITEM item = {0};
	item.mask = HDI_FORMAT;
	header.GetItem(icol, &item);

	fmt = item.fmt & HDF_JUSTIFYMASK;
	return fmt;
}

size_t track_presenter::HitTest(CPoint point) {
	size_t icol = pfc_infinite;

	CHeaderCtrl header = m_ui_list->GetHeaderCtrl();
	::ScreenToClient(m_ui_list->m_hWnd, &point);
	HDHITTESTINFO lptest = {0};
	lptest.pt = point;
	header.HitTest(&lptest);
	if (lptest.flags == HHT_ONHEADER)
		icol = lptest.iItem;
		
	return icol;
}

///////////////////////////////////////////////
//
//	ARTWORK PRESENTER
//
///////////////////////////////////////////////

void artwork_presenter::insert_columns() {
	CHeaderCtrl hc = ListView_GetHeader(GetListView());
	int count = hc ? ListView_GetColumnCount(GetListView()) : 0;
	for (int colIndex = 0; colIndex < count; ++colIndex) {
		ListView_DeleteColumn(GetListView(), 0);
	}

	set_row_height(m_row_height);

	std::vector<int> col_order;
	std::vector<int> col_align;
	std::vector<int> col_width;

	size_t icol = 0;
	for (LPARAM param : m_conf_col_woa) {

		int lo = LOWORD(param);

		col_order.push_back(lo % 10);
		col_align.push_back(lo / 10);
		col_width.push_back(HIWORD(param));

		LVCOLUMN Column;
		listview_helper::insert_column(GetListView(), icol, m_vtitles.at(icol),
			col_width.at(icol), col_align.at(icol));

		ListView_SetColumnWidth(GetListView(), icol, col_width.at(icol));

		Column.mask = LVCF_FMT;
		Column.fmt = col_align.at(icol);

		ListView_SetColumn(GetListView(), icol, &Column);

		icol++;
	}

	CHeaderCtrl header = ListView_GetHeader(GetListView());

	ListView_SetExtendedListViewStyle(GetListView(), m_dwHeaderStyle);

	if (col_width.at(0) == 0)
		update_list_width(true);
}

pfc::string8 artwork_presenter::get_header_label(pfc::string8 header) {

	DWORD style = ListView_GetExtendedListViewStyle(GetListView());
	bool bDetails = ((style & LV_VIEW_DETAILS) == LV_VIEW_DETAILS);
	bool bTile = ((style & LV_VIEW_TILE) == LV_VIEW_TILE);

	if (bTile) {
		return (STR_EQUAL(header, "R") ? "Release" : "Artist");
	}
	else {
		return header;
	}
}

pfc::string8 artwork_presenter::get_tickit_label(af att, bool val) {

	DWORD style = ListView_GetExtendedListViewStyle(GetListView());
	bool bDetails = ((style & LV_VIEW_DETAILS) == LV_VIEW_DETAILS);
	bool bTile = ((style & LV_VIEW_TILE) == LV_VIEW_TILE);

	if (bTile) {
		if (att == af::alb_sd || att == af::art_sd) {
			return val ? "write" : "";
		}
		else if (att == af::alb_ovr || att == af::art_ovr) {
			return val ? "ovr" : "";
		}
		else if (att == af::alb_emb || att == af::art_emb) {
			return val ? "embed" : "";
		}
		else return "?";
	}
	else /*if (bDetails)*/ {
		if (att == af::alb_sd || att == af::alb_ovr || att == af::alb_emb ||
			att == af::art_sd || att == af::art_ovr || att == af::art_emb) {
			return val ? "x" : "";
		}
		else
			return "?";
	}
}


///////////////////////////////////////////////
//
//	DISCOGS ARTWORK PRESENTER
//
///////////////////////////////////////////////

size_t discogs_artwork_presenter::get_icon_id(size_t iImageList) {
	if (iImageList == LVSIL_SMALL) return IDB_PNG48F;
	else if (iImageList == LVSIL_NORMAL) return IDB_PNG150F;
	else return LVSIL_NORMAL;
}

void discogs_artwork_presenter::AddRow(std::any imagerow) {
	
	if (!m_vimages.size()) {
		m_vimages.reserve(m_tag_writer->track_mappings.get_count());
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
	if (list_position >= GetDataLvSize()) return ~0;
	std::vector<V>::iterator v_it = m_lvimages.begin();
	std::variant<V> vb(*m_lvimages.begin());
	std::advance(v_it, list_position);
	out = v_it;
	return list_position;
}

void discogs_artwork_presenter::define_columns() {

	m_vtitles = { "", "Type", "Dim", "Save", "Overwrite", "Embed" };

	m_conf_col_woa.clear();
	m_conf_col_woa.push_back(m_conf.match_discogs_artwork_ra_width);
	m_conf_col_woa.push_back(m_conf.match_discogs_artwork_type_width);
	m_conf_col_woa.push_back(m_conf.match_discogs_artwork_dim_width);
	m_conf_col_woa.push_back(m_conf.match_discogs_artwork_save_width);
	m_conf_col_woa.push_back(m_conf.match_discogs_artwork_ovr_width);
	m_conf_col_woa.push_back(m_conf.match_discogs_artwork_embed_width);

	m_dwHeaderStyle = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | /*LVS_EX_SUBITEMIMAGES |*/ LVS_REPORT | LVS_OWNERDATA | LVS_ICON;
	
	if (m_conf.match_discogs_artwork_art_style != 0)
		m_dwHeaderStyle |= static_cast<DWORD>(m_conf.match_discogs_artwork_art_style);

	ListView_SetExtendedListViewStyleEx(GetListView(), m_dwHeaderStyle, m_dwHeaderStyle);
	ListView_SetView(GetListView(), LVS_REPORT);

	artwork_presenter::define_columns();

}

void discogs_artwork_presenter::build_cfg_columns(foo_discogs_conf* out_conf) {

	foo_discogs_conf* config = out_conf == nullptr ? &m_conf : out_conf;

	if (IsLoaded())	m_conf_col_woa = build_woas(GetListView());

	if (m_conf_col_woa.size()) {

		config->match_discogs_artwork_ra_width = m_conf_col_woa.at(0);
		config->match_discogs_artwork_type_width = m_conf_col_woa.at(1);
		config->match_discogs_artwork_dim_width = m_conf_col_woa.at(2);
		config->match_discogs_artwork_save_width = m_conf_col_woa.at(3);
		config->match_discogs_artwork_ovr_width = m_conf_col_woa.at(4);
		config->match_discogs_artwork_embed_width = m_conf_col_woa.at(5);

	}
	size_t icomp = get_extended_style(GetListView());
	config->match_discogs_artwork_art_style = icomp;

}

//////////////////////////////////
//
// LOCAL FILES ARTWORK PRESENTER
//
//////////////////////////////////

size_t files_artwork_presenter::get_icon_id(size_t iImageList) {
	if (iImageList == LVSIL_SMALL) return IDB_PNG48F;
	else if (iImageList == LVSIL_NORMAL) return IDB_PNG150F;
	else return LVSIL_NORMAL;
}

void files_artwork_presenter::AddRow(std::any imagefilerow) {
	if (!m_vimage_files.size()) {
		m_vimage_files.reserve(album_art_ids::num_types() /*   PFC_TABSIZE(ARTWORK_FILENAMES)*/);
	}
	ndx_image_file_info_row_t ndximginfo = std::any_cast<ndx_image_file_info_row_t>(imagefilerow);

	m_vimage_files.push_back(ndximginfo);
	getimages_file_it g_it = --m_vimage_files.end();

	m_lvimage_files.emplace(m_lvimage_files.end(), std::pair<size_t, getimages_file_it>(m_lvimage_files.size(), g_it) /*pp*/);
}

size_t files_artwork_presenter::GetVRow(size_t list_position, var_it_t& out) {
	if (list_position >= GetDataLvSize()) return ~0;
	std::vector<V>::iterator v_it = m_lvimage_files.begin();
	std::advance(v_it, list_position);
	out = v_it;
	return list_position;
}

void files_artwork_presenter::define_columns() {

	m_vtitles = { "Cover Name", "Dim", "Size" };
	m_conf_col_woa.clear();
	m_conf_col_woa.push_back(m_conf.match_file_artwork_name_width);
	m_conf_col_woa.push_back(m_conf.match_file_artwork_dim_width);
	m_conf_col_woa.push_back(m_conf.match_file_artwork_size_width);

	m_dwHeaderStyle = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | /*LVS_EX_SUBITEMIMAGES |*/ LVS_REPORT | LVS_OWNERDATA | LVS_ICON;

	if (m_conf.match_file_artworks_art_style != 0)
		m_dwHeaderStyle |= static_cast<DWORD>(m_conf.match_file_artworks_art_style);

	ListView_SetExtendedListViewStyleEx(GetListView(), m_dwHeaderStyle, m_dwHeaderStyle);
	ListView_SetView(GetListView(), LVS_REPORT);

	artwork_presenter::define_columns();
}

void files_artwork_presenter::build_cfg_columns(foo_discogs_conf* out_conf) {

	foo_discogs_conf* config = out_conf == nullptr ? &m_conf : out_conf;

	if (IsLoaded())	m_conf_col_woa = build_woas(GetListView());

	if (m_conf_col_woa.size()) {

		config->match_file_artwork_name_width = m_conf_col_woa.at(0);
		config->match_file_artwork_dim_width = m_conf_col_woa.at(1);
		config->match_file_artwork_size_width = m_conf_col_woa.at(2);

	}
	size_t icomp = get_extended_style(GetListView());
	config->match_file_artworks_art_style = icomp;

}

void files_artwork_presenter::update_list_width(bool initialize = false) {

	CRect reclist;
	::GetWindowRect(GetListView(), reclist);
	int framewidth = reclist.Width();
	int slot = (framewidth - 20 - 40) / 6;

	size_t icol = 0;
	for (pfc::string8 htitle : m_vtitles) {
		pfc::stringcvt::string_os_from_utf8 labelOS(htitle.c_str());

		DWORD fmtFlags = icol > 0 ? HDF_CENTER : HDF_LEFT;

		LVCOLUMN lvcol;
		lvcol.iSubItem = icol;
		lvcol.cx = slot;
		lvcol.fmt = fmtFlags /*| HDF_BITMAP | LVCFMT_IMAGE*/ /*HDF_STRING*/;
		lvcol.mask = LVCF_WIDTH | LVCF_FMT;

		ListView_SetColumn(GetListView(), icol, &lvcol);

		icol++;

	}

	size_t wfilename = ListView_GetColumnWidth(GetListView(), 0) + slot * 3 + 20;
	ListView_SetColumnWidth(GetListView(), 0, wfilename);
}

void files_artwork_presenter::GetExistingArtwork() {

	metadb_handle_ptr item = m_tag_writer->finfo_manager->get_item_handle(0);

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
		fake_threaded_process_status f_status;
		threaded_process_status kk;

		threaded_process_status p_status;
		titleformat_hook_impl_multiformat hook(p_status, &m_release);
		CONF.album_art_directory_string->run_hook(item->get_location(), &info, &hook, directory, nullptr);

		result = ReadDimSizeFromFile(directory, fb_art_pref.second);

		pfc::string8 full_path(directory); full_path << "\\" << fb_art_pref.second;
		char converted[MAX_PATH - 1];
		pfc::stringcvt::string_os_from_utf8 cvt(full_path);
		wcstombs(converted, cvt.get_ptr(), MAX_PATH - 1);

		//imagefile info...
		image_info_t image_info = { pfc::string8(converted), result.first, result.second };

		ndx_image_file_t ndx_image_file = std::pair<size_t, GUID>(fb_art_pref.first, album_art_ids::query_type(fb_art_pref.first));
		ndx_image_file_info_row_t ndximgfileinfo = std::pair(ndx_image_file, image_info);
			
		AddRow(ndximgfileinfo);
	}
}

void files_artwork_presenter::Populate(/*Release_ptr release*/) {
	
	PFC_ASSERT(m_release->id.get_length());

	ListView_DeleteAllItems(GetListView());
	Reset();

	GetExistingArtwork();
	set_row_height(true);

	set_lv_images_lists();
}

//returns album_art_id of artwork file, or pfc_infinite on indetermined
size_t files_artwork_presenter::AddFileArtwork(size_t img_ndx, art_src art_source,
	std::pair<HBITMAP, HBITMAP> callback_pair_memblock, std::pair<pfc::string8, pfc::string8> temp_file_names) {

	size_t sz_res = pfc_infinite;

	size_t list_param_ndx = img_ndx;
	//reorder disc and artist
	if (img_ndx == 2)
		list_param_ndx = 3;
	else if (img_ndx == 3)
		list_param_ndx = 2;
	else list_param_ndx = img_ndx;

	//Find lv item by inner ndx
	//create list to parse ndx lists in next releases

	size_t citems = m_lvimage_files.size();
	pfc::array_t<t_uint8> perm_selection;
	perm_selection.resize(citems);
	bit_array_bittable are_albums(citems);

	size_t list_pos = ~0;

	if (citems == 0) {
		//are we on the dialog initialization sequence - OnInit() ?
		//listviews not ready yet...
		list_pos = img_ndx;		
	}
	else {
            
		//todo: check visitor pattern or tidy up
		//check for availability of this (album or artist src type) art list item
		auto find_it = std::find_if(m_lvimage_files.begin(), m_lvimage_files.end(), [&](V const& elem) {
			size_t discogs_img_pos = std::get<3>(elem).first;
			getimages_file_it img_it = std::get<3>(elem).second;
			ndx_image_file_t ndx_image = img_it->first;
			
			// return value: album_art_ids
			sz_res = ndx_image.first;
			
			return (discogs_img_pos == list_param_ndx /*&& image_src == (size_t)art_source*/);
			});

		if (find_it != m_lvimage_files.end()) {
			list_pos = std::distance(m_lvimage_files.begin(), find_it);
		}
	}

	if (list_pos != ~0) {
		update_imagelist(list_param_ndx, album_art_ids::num_types(), callback_pair_memblock);

		std::pair<pfc::string8, pfc::string8> prev_temp_file_names = m_vtemp_files[list_param_ndx];
		if (prev_temp_file_names.first.get_length()) {
			uDeleteFile(prev_temp_file_names.first);
			uDeleteFile(prev_temp_file_names.second);
		}
		m_vtemp_files[list_param_ndx] = std::pair(temp_file_names.first, temp_file_names.second);

		if (loaded()) {
			set_row_height(true);
			ListView_RedrawItems(GetListView(), list_pos, list_pos);
		}
		else {
			set_row_height(false);
		}

		update_img_defs(list_param_ndx, sz_res);
	}

	return sz_res;
}

//todo: unify with GetExistingArtwork/CheckExisting

void files_artwork_presenter::update_img_defs(size_t img_ndx, size_t album_art_id) {

	if (album_art_id == pfc_infinite) return; //nothing else to do

	// update listview file size and dimensions

	pfc::string8 file_name = album_art_ids::query_name(album_art_id);
	pfc::chain_list_v2_t<pfc::string8> splitfile;
	pfc::splitStringByChar(splitfile, file_name.get_ptr(), ' ');
	if (splitfile.get_count()) file_name = *splitfile.first();
	file_name << ".jpg";

	Release_ptr release = m_tag_writer->release;
	metadb_handle_ptr item = m_tag_writer->finfo_manager->get_item_handle(0);

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
	char converted[MAX_PATH - 1];
	pfc::stringcvt::string_os_from_utf8 cvt(full_path);
	wcstombs(converted, cvt.get_ptr(), MAX_PATH - 1);

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
		image_info = { pfc::string8(converted), result.first, result.second };
	}

	//ndx_image_file_t ndx_image_file = std::pair<size_t, GUID>(fb_art_pref.first, album_art_ids::query_type(fb_art_pref.first));
	//ndx_image_file_info_row_t ndximgfileinfo = std::pair(ndx_image_file, image_info);
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

///////////////////////////////////////////////
//
//	DISCOGS ARTWORK PRESENTER
//
///////////////////////////////////////////////

void discogs_artwork_presenter::update_list_width(/*HWND list,*/ bool initialize = false) {

	CRect reclist;
	::GetWindowRect(GetListView(), reclist);
	int framewidth = reclist.Width();
	int slot = (framewidth - 20 - 40) / 6;

	size_t icol = 0;
	for (pfc::string8 htitle : m_vtitles) {

		pfc::stringcvt::string_os_from_utf8 labelOS(htitle.c_str());

		DWORD fmtFlags = icol > 1 ? HDF_CENTER : HDF_LEFT;

		LVCOLUMN lvcol;
		lvcol.iSubItem = icol;
		lvcol.cx = slot;
		lvcol.fmt = fmtFlags /*| HDF_BITMAP | LVCFMT_IMAGE*/ /*HDF_STRING*/;
		lvcol.mask = LVCF_WIDTH | LVCF_FMT;

		ListView_SetColumn(GetListView(), icol, &lvcol);

		icol++;
	}
}

// Get TagWriter Images

void discogs_artwork_presenter::Populate() {

	ListView_DeleteAllItems(GetListView());
	Reset();

	pfc::array_t<Discogs::ReleaseArtist_ptr> release_artists = m_release->artists;
	pfc::array_t<Discogs::Image_ptr> main_artist_images = release_artists[0]->full_artist->images;

	size_t count_release_img = m_release->images.get_count();
	size_t count_artist_img = main_artist_images.get_count();

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
		
		//std::variant<V> varitem = std::pair<size_t, getimages_it>(m_lvimages.size(), --m_vimages.end());

		std::pair<size_t, getimages_it> vrow = std::pair(m_lvimages.size(), --m_vimages.end());
		m_lvimages.emplace(m_lvimages.end(), vrow);

	}

	//artist artwork
	for (size_t i = count_release_img; i < count_release_img + count_artist_img; i++) {
		image_info_t imageinfo;
		Image_ptr pi = main_artist_images[i - count_release_img];
		if (i == count_release_img) {
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

	set_row_height(true);

	PopulateConfArtWork();
}

void discogs_artwork_presenter::PopulateConfArtWork(/*uartwork uartt*/) {

	namespace tl = tileview_helper;

	bool bfirstRun = (m_uart == uartwork(m_conf)); 

	if (bfirstRun) {
		CONFARTWORK = uartwork(
			CONF.embed_album_art,
			CONF.save_album_art,
			CONF.album_art_fetch_all,
			CONF.album_art_overwrite,
			CONF.embed_artist_art,
			CONF.save_artist_art,
			CONF.artist_art_fetch_all,
			CONF.artist_art_overwrite
		);
		m_uart = CONFARTWORK;
	}

	const size_t count = m_vimages.size();
	size_t album_ndx = 0;
	size_t ndx = 0;
	for (size_t i = 0; i < count; i++) {

		bool balbum = m_vimages.at(i).first.first == (int)art_src::alb;
		if (balbum) album_ndx = ndx = i;
		else ndx = i - (album_ndx + 1);

		af a_sa = balbum ? af::alb_sa : af::art_sa;
		af a_sd = balbum ? af::alb_sd : af::art_sd;
		af a_ovr = balbum ? af::alb_ovr : af::art_ovr;
		af a_emb = balbum ? af::alb_emb : af::art_emb;

		std::vector<pfc::string8> rows = m_vimages.at(i).second;

		pfc::string8 type(rows.at(0));

		pfc::string8 dim(rows.at(1));
		dim << "x" << rows.at(2);

		bool bprimary, bsaveall, bsave, bover, bembed;
		bprimary = isPrimary(type);
		bsaveall = m_uart.getflag(a_sa, ndx);
		bsave = m_uart.getflag(a_sd, ndx);
		bover = m_uart.getflag(a_ovr, ndx);
		bembed = m_uart.getflag(a_emb, ndx);

		if (bfirstRun) {
			if (bsaveall) {
				bsave = true;
			}
			else {
				if (!bprimary) {
					bsave &= m_uart.getflag(a_sa, ndx);
					bembed = false;
				}
			}
			bover &= (bsave || bembed);
		}

		m_uart.setflag(a_sd, ndx, bsave);
		m_uart.setflag(a_ovr, ndx, bover);
		m_uart.setflag(a_emb, ndx, bembed);

		//CHANGE TO NON CALLBACK IF NO TILE VIEW IS NEEDED ( 'x' vs 'write' etc)

		//store the original discogs position param (loword) and isAlbum (hiword)
		LPARAM param = MAKELPARAM(i, m_vimages.at(i).first.first/*balbum ? art_src::alb : art_src::art*/);

	}

	CONFARTWORK = m_uart;
	//set lines in details count, set with image + details
	tl::tv_tileview_line_count(GetListView(), 5);
	tl::tv_set_tileview_tile_fixed_width(GetListView(), 250);

}

bool discogs_artwork_presenter::AddArtwork(size_t img_ndx, art_src artSrc, MemoryBlock small_art) {

	pfc::string8 id;
	if (artSrc == art_src::alb) {
		id = m_release->id;
	}
	else {
		//list_param_ndx = img_ndx + m_tag_writer->release->images.get_count();
		id = m_release->artists[0]->full_artist->id;
	}

	pfc::string8 cache_path_small = Offline::get_thumbnail_cache_path_filenames(
		id, artSrc, LVSIL_NORMAL, pfc_infinite)[0];
	pfc::string8 cache_path_mini = Offline::get_thumbnail_cache_path_filenames(
		id, artSrc, LVSIL_SMALL, pfc_infinite)[0];

	cache_path_small << img_ndx << THUMB_EXTENSION;
	cache_path_mini << img_ndx << THUMB_EXTENSION;

	bool bcreated = Offline::CreateOfflinePath(id, artSrc, pfc_infinite, true);
	std::pair<HBITMAP, HBITMAP> hRES = MemoryBlockToTmpBitmap(std::pair(cache_path_small,	cache_path_mini),
		img_ndx, small_art);

	//Find lv item with real ndx ej. 4, artist -> look for art & ndx 4
	//create list to parse ndx lists in next releases

	size_t citems = m_lvimages.size(); //ListView_GetItemCount(GetListView());
	pfc::array_t<t_uint8> perm_selection;
	perm_selection.resize(citems);
	bit_array_bittable are_albums(citems);

	size_t list_pos = ~0;
	size_t list_param_ndx = artSrc == art_src::alb ? img_ndx : img_ndx + m_tag_writer->release->images.get_count();

	if (citems == 0) {
		//an image arrived from dialog OnInit
		list_pos = img_ndx;
	}
	else {
		//check for availability of this (album or artist src type) art list item
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

	if (list_pos != ~0) {
		//note: update_imagelist also deletes HBITMAP pair after copy to imagelist
		update_imagelist(list_param_ndx, m_tag_writer->get_art_count(), hRES);
		if (loaded()) {
			set_row_height(true);
			ListView_RedrawItems(GetListView(), list_pos, list_pos);
		}
		else {
			set_row_height(false);
		}
	}
	return true;
}

art_src discogs_artwork_presenter::get_vimages_src_type_at_pos(size_t list_position) {
	if (list_position >= m_vimages.size())
		return art_src::unknown;
	else {
		std::variant vv_it = m_lvimages[list_position];
		std::pair<size_t, getimages_it> rowpair = std::get<2>(vv_it);
		getimages_it src_img = rowpair.second;
		ndx_image_t ndx_image = src_img->first;
		size_t isource = ndx_image.first;

		return (isource == 0 ? art_src::unknown : (art_src)isource);
	}
}

size_t discogs_artwork_presenter::get_ndx_at_pos(size_t list_position) {
	if (list_position >= m_vimages.size())
		return pfc_infinite;
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
	//todo
}

///////////////////////////////////////////////
//
//	COORD PRESENTER
//
///////////////////////////////////////////////

bool coord_presenters::show_artwork_preview(size_t image_ndx, art_src art_source, MemoryBlock small_art) {

	if (small_art.get_count())
		return m_discogs_art_presenter.AddArtwork(image_ndx, art_source, small_art);
	else
		return false;
}
bool coord_presenters::show_file_artwork_preview(size_t image_ndx, art_src art_source,
	std::pair<HBITMAP, HBITMAP> callback_pair_hbitmaps, std::pair<pfc::string8, pfc::string8> temp_file_names) {

	size_t album_art_id = m_file_art_presenter.AddFileArtwork(image_ndx, art_source, callback_pair_hbitmaps, temp_file_names);

	return true;
}

void coord_presenters::populate_track_ui_mode() {

	lsmode my_mode = lsmode::tracks_ui;

	binomials_it binomial = form_mode[my_mode];
	presenter& pres_tracks = binomial->first;
	presenter& pres_files = binomial->second;

	HWND discogs_track_lst = form_mode[my_mode]->first.GetListView();
	HWND file_lst = form_mode[my_mode]->second.GetListView();

	ListView_DeleteAllItems(discogs_track_lst);
	ListView_DeleteAllItems(file_lst);

	m_discogs_track_libui_presenter.Reset();
	m_file_track_libui_presenter.Reset();

	m_hook.set_custom("DISPLAY_ANVS", m_conf.display_ANVs);
	m_hook.set_custom("REPLACE_ANVS", m_conf.replace_ANVs);

	const size_t count = m_tag_writer->track_mappings.get_count();
	const size_t count_finfo = m_tag_writer->finfo_manager->items.get_count();

	titleformat_object::ptr lenght_script;
	static_api_ptr_t<titleformat_compiler>()->compile_safe_ex(lenght_script, "%length%");

	int debugrejected = 0;

	for (size_t i = 0; i < count; i++) {
		const track_mapping& mapping = m_tag_writer->track_mappings[i];
		if (mapping.file_index == -1) {
			//
			//listview_helper::insert_item(file_list, i, "", -1);
			//
			debugrejected++;
		}
		else {
			file_info& finfo = m_tag_writer->finfo_manager->get_item(mapping.file_index);
			auto item = m_tag_writer->finfo_manager->items.get_item(mapping.file_index);
			pfc::string8 formatted_name;
			CONF.release_file_format_string->run_simple(item->get_location(), &finfo, formatted_name);
			pfc::string8 display = pfc::string_filename_ext(formatted_name);
			const char* length_titleformat = "%length%";
			pfc::string8 formatted_length;
			item->format_title(nullptr, formatted_length, titleformat_object_wrapper(length_titleformat), nullptr);

			//listview_helper::insert_item2(file_list, i, display, formatted_length, mapping.file_index);

			match_info_t match_info(display, formatted_length);
			file_match_t file_match(match_info, mapping.file_index);
			//ADD TRACK ROW
			m_file_track_libui_presenter.AddRow(file_match);
		}
		if (mapping.discogs_disc == -1 && mapping.discogs_track == -1) {
			//
			//listview_helper::insert_item(discogs_track_list, i, "", -1);
			//
			debugrejected++;
		}
		else {
			const ReleaseDisc_ptr disc = m_tag_writer->release->discs[mapping.discogs_disc];
			const ReleaseTrack_ptr track = disc->tracks[mapping.discogs_track];
			
			m_hook.set_release(&m_tag_writer->release);
			m_hook.set_disc(&disc);
			m_hook.set_track(&track);

			if (i == 0) {
				//warn differnt release id
				bool bdiffid = false;
				file_info_impl finfo;
				metadb_handle_ptr item = m_tag_writer->finfo_manager->items[0];
				item->get_info(finfo);

				pfc::string8 local_release_id;

				const char* ch_local_rel = finfo.meta_get("DISCOGS_RELEASE_ID", 0);
				if (ch_local_rel) {
					local_release_id = ch_local_rel;
				}
				bdiffid = (local_release_id.get_length() && !STR_EQUAL(m_tag_writer->release->id, local_release_id));

				pfc::string8 compact_release;
				CONF.search_master_sub_format_string->run_hook(m_location, &m_info, &m_hook, compact_release, nullptr);

				pfc::string8 rel_desc = bdiffid ? "!! " : "";
				rel_desc << ltrim(compact_release);
				
				uSetDlgItemText(m_hWnd, IDC_STATIC_MATCH_TRACKING_REL_NAME, rel_desc);
			}			
			
			pfc::string8 text;
			CONF.release_discogs_format_string->run_hook(m_location, &m_info, &m_hook, text, nullptr);
			pfc::string8 time;
			if (track->discogs_hidden_duration_seconds) {
				int duration_seconds = track->discogs_duration_seconds + track->discogs_hidden_duration_seconds;
				time = duration_to_str(duration_seconds);
			}
			else {
				time = track->discogs_duration_raw;
			}

			//listview_helper::insert_item2(discogs_track_list, i, text, time, ENCODE_DISCOGS(mapping.discogs_disc, mapping.discogs_track));

			match_info_t match_info(text, time);
			track_match_t file_match(match_info, file_match_nfo{ mapping.discogs_track, mapping.discogs_disc });

			m_discogs_track_libui_presenter.AddRow(file_match);
		}
	}
}

void coord_presenters::populate_artwork_mode(size_t select) {

	binomials_it binomial = form_mode[lsmode::art];
	presenter& pres_tracks = binomial->first;
	presenter& pres_files = binomial->second;

	if (select == 0 || select == 1)
		m_discogs_art_presenter.Populate();
	if (select == 0 || select == 2)
		m_file_art_presenter.Populate();

}

void coord_presenters::FileArtDeleteImageList(pfc::array_t<GUID> album_art_ids) {
	
	m_file_art_presenter.ImageListReset(album_art_ids);

}