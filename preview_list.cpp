#include "stdafx.h"

#include "preview_dialog.h"
#include "preview_list.h"

const size_t TRACER_IMG_NDX = 3;
const size_t LOADED_RELEASES_IMG_NDX = 6;
const size_t SHOWING_IMAGE_NDX = 1;

CPreviewList::CPreviewList(IListControlOwnerDataSource* ilo)
	: CListControlOwnerData(ilo) {

	CPreviewTagsDialog* dlg = dynamic_cast<CPreviewTagsDialog*>(m_host);
}

void CPreviewList::set_image_list() {

	m_hImageList.Create(IDB_BITMAP_TRACER, 16, ILC_COLOR4, RGB(255, 255, 255));
	ListView_SetImageList(this->m_hWnd, m_hImageList, LVSIL_SMALL);
}

//default action

void CPreviewList::Default_Action() {

	size_t isel = GetSingleSel();
	const int icount = GetItemCount();

	bit_array_bittable selmask(icount);
	selmask.set(isel, true);
	CPreviewTagsDialog* dlg = dynamic_cast<CPreviewTagsDialog*>(m_host);

	dlg->context_menu_switch(m_hWnd, { 0 }, CPreviewTagsDialog::ID_PREVIEW_CMD_EDIT_RESULT_TAG, selmask, {});
}

bool CPreviewList::IsSubItemGrayed(size_t item, size_t subItem) {
	return false;
}

void CPreviewList::context_menu(size_t list_index, POINT screen_pos) {

	bool empty_sel = (list_index == -1);
}
