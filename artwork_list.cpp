﻿#include "stdafx.h"

#include "track_matching_dialog.h"
#include "artwork_list.h"

const size_t TRACER_IMG_NDX = 3;
const size_t LOADED_RELEASES_IMG_NDX = 6;
const size_t SHOWING_IMAGE_NDX = 1;

CArtworkList::CArtworkList(IListControlOwnerDataSource* ilo)
	: CListControlOwnerData(ilo) {
	//..
}

// CListControlOwnerData overrides

int CArtworkList::GetItemHeight() const {

	CTrackMatchingDialog* dlg = (CTrackMatchingDialog*)(m_host);
	auto uilist = dlg->get_ctx_lvlist(IDC_UI_DC_ARTWORK_LIST);

	if (m_hWnd == uilist && dlg->m_coord.isTile()) {
			return 150;	
	}
	return 50;
}

bool CArtworkList::IsSubItemGrayed(size_t item, size_t subItem) {

	CTrackMatchingDialog* dlg = (CTrackMatchingDialog*)(m_host);
	auto uilist = dlg->get_ctx_lvlist(IDC_UI_FILE_ARTWORK_LIST);
	
	if (m_hWnd == uilist) {

		return !CONF_MULTI_ARTWORK.file_match;
	}
	return false;
}

bool CArtworkList::RenderCellImageTest(size_t item, size_t subItem) const {

	return subItem == 0;
}

void CArtworkList::RenderCellImage(size_t item, size_t subItem, CDCHandle dc, const CRect& rc) const {

	CTrackMatchingDialog* dlg = (CTrackMatchingDialog*)(m_host);

	bool isTile = dlg->m_coord.isTile();
	auto ida_list = dlg->get_ctx_lvlist(IDC_UI_DC_ARTWORK_LIST);
		
	HICON hIcon;
	
	if (m_hWnd == ida_list) {
	
        var_it_t out;
		getimages_it imginfo;
		
		size_t ndxpos = dlg->m_coord.Get_V_LvRow(lsmode::art, true, item, out);
		imginfo = std::get<2>(*out).second;
		
		if (ndxpos == pfc_infinite) return;
		
		hIcon = dlg->m_coord.GetDiscogsArtVIcons(ndxpos);		
	}
	else {
	
        var_it_t out;
		getimages_file_it imginfo;

		size_t ndxpos = dlg->m_coord.Get_V_LvRow(lsmode::art, false, item, out);
		imginfo = std::get<3>(*out).second;

		if (ndxpos == pfc_infinite) return;

		hIcon = dlg->m_coord.GetFileArtVIcons(ndxpos);
	}

	if (hIcon) {

		dc.DrawIconEx(CPoint(rc.TopLeft().x, rc.TopLeft().y)/*(rc.TopLeft()*/, hIcon/*vicons.at(ndxpos)*//*hIcon*/ /*g_hIcon_quian*/, CSize(rc.Width() - 1, rc.Height() - 1));
	}
}