#pragma once

#include <atltypes.h>
#include <atlapp.h>
#include <windef.h>

#include "../SDK/foobar2000.h"
#include "LibPPUI\PaintUtils.h"

#include "foo_discogs.h"
#include "resource.h"

using namespace Discogs;

class MatchListDropHandler : public CWindowImpl<MatchListDropHandler> {

private:

	HWND p_parent; HWND track_list; HWND file_list;

public:

	MatchListDropHandler() {}
	MatchListDropHandler(HWND p_parent, HWND track_list, HWND file_list) :
		p_parent(p_parent), track_list(track_list), file_list(file_list) {

		stdf_change_notifier = nullptr;

	}

	MatchListDropHandler::~MatchListDropHandler() {
		m_hWnd = NULL;
		if (m_bDragging) ZapCapture();
	}

	std::function<bool(HWND)>stdf_change_notifier;

	// Initialization on dialog parent OnInitDialog()

	void Initialize(HWND wnd_parent, HWND wnd_discogs, HWND wnd_files) {
		m_hWnd = wnd_parent;
		m_discogs_track_list = wnd_discogs;
		m_file_list = wnd_files;
	}

	void SetNotifier(std::function<bool(HWND)>update_notifier) {
		stdf_change_notifier = update_notifier;
	}

	BEGIN_MSG_MAP(CMatchListMdrvImpl)
		
		NOTIFY_HANDLER(IDC_DISCOGS_TRACK_LIST, LVN_BEGINDRAG, OnListDragBegin)
		NOTIFY_HANDLER(IDC_FILE_LIST, LVN_BEGINDRAG, OnListDragBegin)
		NOTIFY_HANDLER(IDC_DISCOGS_TRACK_LIST, WM_LBUTTONUP, OnListLButtonUp)
		NOTIFY_HANDLER(IDC_FILE_LIST, WM_LBUTTONUP, OnListLButtonUp)
		NOTIFY_HANDLER(IDC_DISCOGS_TRACK_LIST, WM_MOUSEMOVE, OnListMouseMove)
		NOTIFY_HANDLER(IDC_FILE_LIST, WM_MOUSEMOVE, OnListMouseMove)
		NOTIFY_HANDLER(IDC_DISCOGS_TRACK_LIST, NM_CUSTOMDRAW, OnCustomDraw)
		NOTIFY_HANDLER(IDC_FILE_LIST, NM_CUSTOMDRAW, OnCustomDraw)
		END_MSG_MAP()

protected:

	HWND m_discogs_track_list;
	HWND m_file_list;
	bool m_bDragging;
	HWND m_listDragging;
	int m_itemDragging;
	int m_itemTarget;
	POINT m_itemDragginPoint;

	HWND ListControlHitTest(const POINT scrpoint) {
		HWND wndres = NULL;
		CRect rc;
		if (!::GetWindowRect(m_discogs_track_list, rc))
			rc.SetRectEmpty();
		else
			if (PtInRect(rc, scrpoint))
				wndres = m_discogs_track_list;
			else
				if (!::GetWindowRect(m_file_list, rc))
					rc.SetRectEmpty();
				else
					if (PtInRect(rc, scrpoint))
						wndres = m_file_list;
		rc.SetRectEmpty();
		return wndres;
	}

	LRESULT OnCustomDraw(int wParam, LPNMHDR lParam, BOOL bHandled) {

		LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;
		int pos = (int)lplvcd->nmcd.dwItemSpec;

		switch (lplvcd->nmcd.dwDrawStage) {

		case CDDS_PREPAINT: //{
			//request notifications for individual listview items
			return CDRF_NOTIFYITEMDRAW;

		case CDDS_ITEMPREPAINT:

			if (lplvcd->nmcd.dwItemSpec == m_itemTarget)
			{
				//customize item appearance
				PaintUtils::FillRectSimple(lplvcd->nmcd.hdc, lplvcd->nmcd.rc, RGB(100, 100, 100));
				//lplvcd->clrText = RGB(255, 0, 0);
				lplvcd->clrTextBk = RGB(200, 200, 200);
				return CDRF_NEWFONT;
			}
			else {
				PaintUtils::FillRectSimple(lplvcd->nmcd.hdc, lplvcd->nmcd.rc, RGB(255, 255, 255));
				//lplvcd->clrText = RGB(0, 0, 255);
				lplvcd->clrTextBk = RGB(255, 255, 255);
				return CDRF_NEWFONT;
			}
			break;

			//Before a subitem is drawn

		case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
			if (m_itemDragging == (int)lplvcd->nmcd.dwItemSpec)
			{
				if (0 == lplvcd->iSubItem)
				{
					//customize subitem appearance for column 0
					//lplvcd->clrText = RGB(255, 0, 0);
					lplvcd->clrTextBk = RGB(255, 255, 255);

					//To set a custom font:
					//SelectObject(lplvcd->nmcd.hdc, 
					//    <your custom HFONT>);

					return CDRF_NEWFONT;
				}
				else if (1 == lplvcd->iSubItem)
				{
					//customize subitem appearance for columns 1..n
					//Note: setting for column i 
					//carries over to columnn i+1 unless
					//      it is explicitly reset
					//lplvcd->clrTextBk = RGB(255, 0, 0);
					lplvcd->clrTextBk = RGB(255, 255, 255);

					return CDRF_NEWFONT;
				}
			}
		}
		return CDRF_DODEFAULT;
	}

	LRESULT OnListLButtonUp(int wParam, LPNMHDR lParam, BOOL bHandled) {

		bool bres = false;

		if (m_bDragging) {

			BOOL bres = false;

			//GetClipCursor / GetClientRect / ClipCursor
			POINT pt;
			GetCursorPos(&pt);
			HWND wndlist = ListControlHitTest(pt);

			if (wndlist != NULL)
			{

				if (wndlist != m_listDragging) {
					//dropping to another list not allowed

					LVITEM lvi;
					ZeroMemory(&lvi, sizeof(lvi));
					lvi.mask = LVIF_STATE;
					lvi.stateMask = LVIS_SELECTED;
					::SendMessage(m_discogs_track_list, LVM_SETITEMSTATE, (WPARAM)-1, (LPARAM)&lvi);
					ZeroMemory(&lvi, sizeof(lvi));
					lvi.mask = LVIF_STATE;
					lvi.stateMask = LVIS_SELECTED;
					::SendMessage(m_file_list, LVM_SETITEMSTATE, (WPARAM)-1, (LPARAM)&lvi);

					ZapCapture();
				}

				NMITEMACTIVATE* info = reinterpret_cast<NMITEMACTIVATE*>(lParam);

				LVHITTESTINFO lvhti;
				// Determine the dropped item
				lvhti.pt.x = pt.x;
				lvhti.pt.y = pt.y;

				::ScreenToClient(wndlist, &lvhti.pt);
				ListView_HitTest(wndlist, &lvhti);

				// Out of the ListView? // Not in an item?
				if ((lvhti.iItem == -1) || (((lvhti.flags & LVHT_ONITEMLABEL) == 0) &&
					((lvhti.flags & LVHT_ONITEMSTATEICON) == 0))) {

					bres = FALSE;
				}
				else
				{
					// Dropping into a selected item?
					LVITEM lvi;
					lvi.iItem = lvhti.iItem;
					lvi.iSubItem = 0;
					lvi.mask = LVIF_STATE;
					lvi.stateMask = LVIS_SELECTED;
					ListView_GetItem(wndlist, &lvi);

					if (lvi.state & LVIS_SELECTED) {

						bres = FALSE;
					}
					else
					{ // OK
						// Rearrange the items
						int iPos = ListView_GetNextItem(wndlist, -1, LVNI_SELECTED);
						int cSel = ListView_GetSelectedCount(wndlist);
						TCHAR szbuf[1024];
						while (iPos != -1) {
							// First, copy one item
							lvi.iItem = iPos;
							lvi.iSubItem = 0;
							lvi.cchTextMax = 1024;
							lvi.pszText = szbuf;
							lvi.stateMask = ~LVIS_SELECTED;
							lvi.mask = LVIF_STATE | LVIF_IMAGE
								| LVIF_INDENT | LVIF_PARAM | LVIF_TEXT;
							ListView_GetItem(wndlist, &lvi);
							lvi.iItem = lvhti.iItem;
							// Insert the main item
							int iRet = ListView_InsertItem(wndlist, &lvi);
							if (lvi.iItem < iPos)
								lvhti.iItem++;
							if (iRet <= iPos)
								iPos++;
							// Set the subitem text
							int JOB_WIN_COLUMN_NUM = 2;
							for (int i = 1; i < JOB_WIN_COLUMN_NUM; i++) {
								ListView_GetItemText(wndlist, iPos,
									i, szbuf, 1024);
								ListView_SetItemText(wndlist, iRet, i, szbuf);
							}
							// Delete from original position
							ListView_DeleteItem(wndlist, iPos);
							iPos = ListView_GetNextItem(wndlist, -1, LVNI_SELECTED);
						}

						bres = TRUE;

						//Reselect and focus
						if (m_itemTarget != -1) {
							for (int walk = 0; walk < cSel; walk++) {
								ListView_SetItemState(wndlist, m_itemTarget + walk,
									LVNI_SELECTED | (!walk ? LVNI_FOCUSED : 0),
									LVIS_SELECTED | (!walk ? LVIS_FOCUSED : 0));
							}

							//update_match_message_display(match_manual);
							stdf_change_notifier(wndlist);

						}
					}
				}
			}
			else {
				// no reason to be here
				// trying to drag on other controls?
			}
            //,,
		}
		else {
			// no reason be here if Dragging was inactive
		}
		
        // RELEASE CAPTURE
		ZapCapture();
		return bres;
	}

	LRESULT OnListMouseMove(int wParam, LPNMHDR lParam, BOOL bHandled) {

		if (!m_bDragging)
			return FALSE;

		POINT pt;
		GetCursorPos(&pt);

		HWND wndlist = ListControlHitTest(pt);

		if (wndlist != NULL)
		{

			LVHITTESTINFO lvhti;

			// Determine the target item
			lvhti.pt.x = pt.x;
			lvhti.pt.y = pt.y;

			::ScreenToClient(wndlist, &lvhti.pt);
			ListView_HitTest(wndlist, &lvhti);

			// Out of the ListView?
			if (lvhti.iItem == -1) {
				m_itemTarget = -1;
				return FALSE;
			}

			// Not in an item?
			if (((lvhti.flags & LVHT_ONITEMLABEL) == 0) &&
				((lvhti.flags & LVHT_ONITEMSTATEICON) == 0))
			{
				m_itemTarget = -1;
				return FALSE;
			}

			if (m_itemTarget != lvhti.iItem) {
				m_itemTarget = lvhti.iItem;
				CRect lrc;
				CRgn updateRgn; updateRgn.CreateRectRgn(0, 0, 0, 0);
				::GetClientRect(wndlist, &lrc);
				::RedrawWindow(wndlist, lrc, updateRgn, RDW_UPDATENOW);
			}

			m_itemTarget = lvhti.iItem;
			m_bDragging = true;

			ListView_SetItemState(wndlist, m_itemTarget, /*LVIS_SELECTED |*/ LVIS_FOCUSED, LVIS_FOCUSED);

			/*CRect arc;
			::GetWindowRect(wndlist, &arc);
			::InvalidateRect(wndlist, arc, true);*/

			CRect lrc;
			::GetClientRect(wndlist, &lrc);
			CRgn updateRgn; updateRgn.CreateRectRgn(0, 0, lrc.right, lrc.bottom);
			::RedrawWindow(wndlist, lrc, updateRgn, RDW_UPDATENOW);

		}

		return TRUE;
	}

	LRESULT OnListDragBegin(int wParam, LPNMHDR lParam, BOOL bHandled) {

		//HWND wnd = ((LPNMHDR)lParam)->hwndFrom;
		//((NM_LISTVIEW*)((LPNMHDR)lParam))->ptAction;

		NMITEMACTIVATE* info = reinterpret_cast<NMITEMACTIVATE*>(lParam);

		POINT coords; GetCursorPos(&coords);

		HWND wndlist = ListControlHitTest(coords);
		g_dragDropInstance = this;
		g_dragDropInstance->m_listDragging = wndlist;
		g_hook = SetWindowsHookEx(WH_MOUSE, MouseHookProc, NULL, GetCurrentThreadId());

		/* INIT CAPTURE */

		::SetCapture(wndlist);

		POINT hitpoint;
		::GetCursorPos(&hitpoint);
		LVHITTESTINFO lvhti;
		// Determine the dropped item
		lvhti.pt.x = hitpoint.x;
		lvhti.pt.y = hitpoint.y;
		::ScreenToClient(wndlist, &lvhti.pt);
		ListView_HitTest(wndlist, &lvhti);

		m_itemDragging = lvhti.iItem;
		m_itemTarget = -1;
		m_bDragging = true;

		return TRUE;
	}

	void  ZapCapture() {

		m_listDragging = nullptr;
		m_itemDragging = -1;
		m_itemTarget = -1;
		m_bDragging = false;
		ReleaseCapture();

		/*res =*/ UnhookWindowsHookEx((HHOOK)MouseHookProc);
		g_hook = NULL;
		g_dragDropInstance = nullptr;

	}

	//LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam);

	// ========================================================================================
	// Mouse wheel vs drag&drop hacks
	// Install MouseHookProc for the duration of DoDragDrop and handle the input from there
	// ========================================================================================
	inline static HHOOK g_hook = NULL;
	inline	static MatchListDropHandler* g_dragDropInstance = nullptr;

	static LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
		//MOUSEHOOKSTRUCT* pmhs = (MOUSEHOOKSTRUCT*)lParam;
		BOOL handled = FALSE;
		if (nCode == HC_ACTION && g_dragDropInstance != nullptr) {
			switch (wParam) {
			case WM_MOUSEMOVE:
				g_dragDropInstance->OnListMouseMove(wParam, (LPNMHDR)lParam, TRUE);
				break;
			case WM_LBUTTONUP:
				g_dragDropInstance->OnListLButtonUp(wParam, (LPNMHDR)lParam, TRUE);
				break;
			case WM_MOUSEWHEEL:
			case WM_MOUSEHWHEEL:
				break;
			}
		}
		return CallNextHookEx(g_hook, nCode, wParam, lParam);
	}

};
