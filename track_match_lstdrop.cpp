#include <stdafx.h>

#include <gdiplus.h>

#include "foo_discogs.h"
#include "track_match_lstdrop.h"

using namespace Gdiplus;

// // // // // // //

// ON CUSTOM DRAW

// // // // // // //

LRESULT MatchListDropHandler::OnCustomDraw(int wParam, LPNMHDR lParam, BOOL bHandled) {

	LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;
	int pos = (int)lplvcd->nmcd.dwItemSpec;

	switch (lplvcd->nmcd.dwDrawStage) {

	case CDDS_PREPAINT: //{
		//request notifications for individual list view items
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT: {

		POINT pt;
		GetCursorPos(&pt);
		HWND wndlist = ListControlHitTest(pt);
		CPoint pi(lplvcd->nmcd.rc.left, lplvcd->nmcd.rc.bottom);
		::ClientToScreen(m_hWnd, &pi);

		int count = ListView_GetItemCount(wndlist);
		if (lplvcd->nmcd.dwItemSpec == count - 1) {
			int h = lplvcd->nmcd.rc.bottom - lplvcd->nmcd.rc.top;
			m_rcPastLastItem = CRect(CPoint(lplvcd->nmcd.rc.left, lplvcd->nmcd.rc.top + h), CPoint(lplvcd->nmcd.rc.right, lplvcd->nmcd.rc.bottom + h));
		}


		if (lplvcd->nmcd.dwItemSpec == m_itemTarget)
		{
			//customize item appearance

			lplvcd->clrTextBk = RGB(200, 200, 200);
			return CDRF_NEWFONT | CDRF_NOTIFYPOSTPAINT;
		}
		else {
			if (m_itemTarget == -1)
				return CDRF_DODEFAULT | CDRF_NOTIFYPOSTPAINT;

			lplvcd->clrTextBk = RGB(255, 255, 255);
			return CDRF_NEWFONT | CDRF_NOTIFYPOSTPAINT;
		}

		return CDRF_NOTIFYPOSTPAINT;

	}
	//Before a subitem is drawn

	case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
		return CDRF_DODEFAULT;
		if (lplvcd->nmcd.dwItemSpec == m_itemTarget)
		{
			if (0 == lplvcd->iSubItem)
			{
				lplvcd->clrTextBk = RGB(0, 0, 0);
				return CDRF_NEWFONT;
			}
			else if (1 == lplvcd->iSubItem) {
				lplvcd->clrTextBk = RGB(0, 0, 0);
				return CDRF_NEWFONT;
			}
			else if (2 == lplvcd->iSubItem) {
				lplvcd->clrTextBk = RGB(0, 0, 0);
				return CDRF_NEWFONT;
			}
			else if (3 == lplvcd->iSubItem) {
				lplvcd->clrTextBk = RGB(0, 0, 0);
				return CDRF_NEWFONT;
			}
			else if (4 == lplvcd->iSubItem) {
				lplvcd->clrTextBk = RGB(0, 0, 0);
				return CDRF_NEWFONT;
			}
			else if (5 == lplvcd->iSubItem) {
				lplvcd->clrTextBk = RGB(0, 0, 0);
				return CDRF_NEWFONT;
			}
			else if (6 == lplvcd->iSubItem) {
				lplvcd->clrTextBk = RGB(0, 0, 0);
				return CDRF_NEWFONT;
			}
		}
		else {
			if (0 == lplvcd->iSubItem)
			{
				lplvcd->clrTextBk = RGB(0, 20, 0);
				return CDRF_NEWFONT;
			}
			else if (1 == lplvcd->iSubItem) {
				lplvcd->clrTextBk = RGB(0, 50, 0);
				return CDRF_NEWFONT;
			}
			else if (2 == lplvcd->iSubItem) {
				lplvcd->clrTextBk = RGB(0, 80, 0);
				return CDRF_NEWFONT;
			}
			else if (3 == lplvcd->iSubItem) {
				lplvcd->clrTextBk = RGB(0, 120, 0);
				return CDRF_NEWFONT;
			}
			else if (4 == lplvcd->iSubItem) {
				lplvcd->clrTextBk = RGB(0, 150, 0);
				return CDRF_NEWFONT;
			}
			else if (5 == lplvcd->iSubItem) {
				lplvcd->clrTextBk = RGB(0, 180, 0);
				return CDRF_NEWFONT;
			}
			else if (6 == lplvcd->iSubItem) {
				lplvcd->clrTextBk = RGB(0, 200, 0);
				return CDRF_NEWFONT;
			}
		}

	case CDDS_ITEMPOSTPAINT: {

		int    nItem = static_cast<int>(lplvcd->nmcd.dwItemSpec);

		// Get the image index and state of this item.  Note that we need to
		// check the selected state manually.  The docs _say_ that the
		// item's state is in pLVCD->nmcd.uItemState, but during my testing
		// it was always equal to 0x0201, which doesn't make sense, since
		// the max CDIS_* constant in commctrl.h is 0x0100.

		LV_ITEM rItem = {};
		rItem.mask = LVIF_IMAGE | LVIF_STATE;
		rItem.iItem = nItem;
		rItem.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
		ListView_GetItem(lParam->hwndFrom, &rItem);

		// If this item is selected, redraw the icon with its normal colors.
		if (rItem.state & LVIS_SELECTED)
		{

			CRect rcIcon;

			// Get the rect that holds the item's icon.
			ListView_GetItemRect(lParam->hwndFrom, lplvcd->nmcd.dwItemSpec, &rcIcon, LVIR_ICON);

			// fix offset on tile view
			if (IsTile(lParam->hwndFrom)) rcIcon.DeflateRect(2, 1);

			//m_list.GetItemRect(nItem, &rcIcon, LVIR_ICON);
			Gdiplus::Graphics* g = Gdiplus::Graphics::FromHDC(lplvcd->nmcd.hdc);

			// Draw the icon.
			CImageList imglist = ListView_GetImageList(lParam->hwndFrom,
				IsTile(lParam->hwndFrom) ? LVSIL_NORMAL : LVSIL_SMALL);

			imglist.Draw(g->GetHDC(), rItem.iImage, rcIcon.TopLeft(), ILD_TRANSPARENT);

			imglist.Detach();
			delete g;

			return CDRF_SKIPDEFAULT;
		}
		if (rItem.state & LVIS_FOCUSED)
		{

			CDC dc;
			CBrush br;

			br.CreateSolidBrush(m_color_blend);
			dc.Attach(lplvcd->nmcd.hdc);

			CRect framerec = lplvcd->nmcd.rc;
			//fix missing bottom border with height offset
			--framerec.bottom;
			dc.FrameRect(&framerec, br);
			dc.Detach();

			br.DeleteObject();
			dc.DeleteDC();

			return CDRF_SKIPDEFAULT;
		}
	}
	}
	return CDRF_DODEFAULT;
}
