#pragma once

#include <atltypes.h>
#include <atlapp.h>
#include <windef.h>

#include "LibPPUI\PaintUtils.h"

#include "track_matching_dialog_presenter.h"
#include "resource.h"

using namespace Discogs;

class MatchListDropHandler : public CWindowImpl<MatchListDropHandler> {

private:

	HWND p_parent;

public:

	MatchListDropHandler() : p_parent(NULL), m_itemTarget(-1) {}
	MatchListDropHandler(HWND p_parent) :
		p_parent(p_parent) {

		m_itemTarget = -1;
		stdf_change_notifier = nullptr;
	}

	MatchListDropHandler::~MatchListDropHandler() {
		m_hWnd = NULL;
		if (m_bDragging) ZapCapture();
	}

	std::function<bool(HWND, size_t item)>stdf_change_notifier = nullptr;

	// Initialization on dialog parent OnInitDialog()

	void Initialize(HWND wnd_parent, HWND wnd_discogs, HWND wnd_files,
		coord_presenters* coord_pres) {
		m_hWnd = wnd_parent;
		m_discogs_track_list = wnd_discogs;
		m_file_list = wnd_files;
		m_coord = coord_pres;
		m_rcPastLastItemSet = false;

		m_color_highlight = GetSysColor(COLOR_HIGHLIGHT); //(0,120,215)
		m_color_background = GetSysColor(COLOR_WINDOW); //255,255,255
		m_color_text = GetSysColor(COLOR_WINDOWTEXT);
		m_color_blend = PaintUtils::BlendColor(m_color_background, m_color_text, 33);
		m_colors[0] = m_color_blend;
		m_colors[1] = m_color_text;
		m_colors[2] = m_color_highlight;
	}

	void SetNotifier(std::function<bool(HWND, size_t)>update_notifier) {
		stdf_change_notifier = update_notifier;
	}

#pragma warning( push )
#pragma warning( disable : 26454 )

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

#pragma warning( pop )

protected:

	HWND m_discogs_track_list;
	HWND m_file_list;
	coord_presenters* m_coord;
	bool m_bDragging;
	HWND m_listDragging;
	int m_itemDragging;
	int m_itemTarget;
	POINT m_itemDragginPoint;
	RECT m_rcPastLastItem;
	bool m_rcPastLastItemSet;

	COLORREF m_color_highlight;
	COLORREF m_color_background;
	COLORREF m_color_text;
	COLORREF m_color_blend;
	COLORREF m_color_target = 0xC8C8C8;
	t_uint32 m_colors[3];
	
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

	bool IsTile(HWND hwnd) {
		DWORD dwView = ListView_GetView(hwnd);
		bool dbug = (dwView == LV_VIEW_TILE);
		return (dwView == LV_VIEW_TILE);
	}


	LRESULT OnCustomDraw(int wParam, LPNMHDR lParam, BOOL bHandled);

    LRESULT OnListLButtonUp(int wParam, LPNMHDR lParam, BOOL bHandled) {
        //do not return from this function without ZapCapture()
        bool bres = false;

        if (m_bDragging) {

            //BOOL b_drag_res = false;

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

                //... not our lists (HWND wnd = ((LPNMHDR)lParam)->hwndFrom;)
                //... not our x (LOWORD(lParam))
                //... not our y (HIWORD(lParam))

                NMITEMACTIVATE* info = reinterpret_cast<NMITEMACTIVATE*>(lParam);

                LVHITTESTINFO lvhti;
                // Determine the dropped item
                lvhti.pt.x = pt.x;
                lvhti.pt.y = pt.y;

                ::ScreenToClient(wndlist, &lvhti.pt);
                ListView_HitTest(wndlist, &lvhti);

                CPoint lvhtipt(lvhti.pt);
                ::PtInRect(CRect(m_rcPastLastItem), lvhti.pt);
                t_size lcount = ListView_GetItemCount(wndlist);
                bool bPastLast = ::PtInRect(CRect(m_rcPastLastItem), lvhti.pt);
                bPastLast &= lcount > 0;
                if (bPastLast) m_itemTarget = lcount - 1;

                // Out of the ListView? // Not in an item?
                if (!bPastLast && ((lvhti.iItem == -1) || (((lvhti.flags & LVHT_ONITEMLABEL) == 0) &&
                    ((lvhti.flags & LVHT_ONITEMSTATEICON) == 0)))) {

                    // b_drag_res = FALSE;
                }
                else
                {
                    // Dropping into a selected item?
                    LVITEM lvi;
                    lvi.iItem = bPastLast? lcount - 1 : lvhti.iItem;
                    lvi.iSubItem = 0;
                    lvi.mask = LVIF_STATE;
                    lvi.stateMask = LVIS_SELECTED;
                    ListView_GetItem(wndlist, &lvi);

                    if (lvi.state & LVIS_SELECTED) {

                        //b_drag_res = FALSE;
                    }
                    else
                    {	// OK
                        
                        std::vector<size_t> vorder(ListView_GetItemCount(wndlist));
						stdf_change_notifier(wndlist, lvhti.iItem);

                    }
                }
            }
            else {
                // no reason to be here
                // trying to drag on other control?
            }
        }
        else {
            // no reason be here if Dragging was inactive
        }

        ZapCapture();
        return bres;
    }

	LRESULT OnListMouseMove(int wParam, LPNMHDR lParam, BOOL bHandled) {

		if (!m_bDragging)
			return FALSE;

		POINT pt;
		GetCursorPos(&pt);

		HWND wndlist = ListControlHitTest(pt);
		HWND wl = uGetDlgItem(IDC_DISCOGS_TRACK_LIST);

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

				/*::RedrawWindow(wndlist, lrc, updateRgn, RDW_UPDATENOW); */
				ListView_RedrawItems(wndlist, 0, ListView_GetItemCount(wndlist));
			}
			m_bDragging = true;
			ListView_SetItemState(wndlist, m_itemTarget, /*LVIS_SELECTED |*/ LVIS_FOCUSED, LVIS_FOCUSED);
		}
		return TRUE;
	}

	LRESULT OnListDragBegin(int wParam, LPNMHDR lParam, BOOL bHandled) {
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
	inline	static /*CTrackMatchingDialog*/MatchListDropHandler* g_dragDropInstance = nullptr;

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
