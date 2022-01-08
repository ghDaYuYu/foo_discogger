﻿#pragma once

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

	std::function<bool(HWND)>stdf_change_notifier = nullptr;

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

                        size_t c = 0;
                        for (auto & walk : vorder) {
                        	walk = c; c++;
                        }
                        
                        // Rearrange the items
                        int iPos = ListView_GetNextItem(wndlist, -1, LVNI_SELECTED);
                        int cSel = ListView_GetSelectedCount(wndlist);
                        TCHAR szbuf[1024];
                        size_t lastPos = ~0; size_t moveForwards = 0; size_t deletions = 0;
                        bool fwd = iPos < m_itemTarget;
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
                            lvi.iItem = bPastLast ? lcount : lvhti.iItem;
                            // Insert the main item
                            int iRet = ListView_InsertItem(wndlist, &lvi);

                            vorder.insert(vorder.begin() + (iRet), vorder[iPos]); // .insert(vorder.begin() + bPastLast ? lcount : lvhti.iItem, 3);
    
                            if (lvi.iItem < iPos)
                                lvhti.iItem++;
                            if (iRet <= iPos)
                                iPos++;

                            // Set subitem text
                            int JOB_WIN_COLUMN_NUM = 2;
                            
                            /*for (int i = 1; i < JOB_WIN_COLUMN_NUM; i++) {
                                ListView_GetItemText(wndlist, iPos,
                                    i, szbuf, 1024);
                                ListView_SetItemText(wndlist, iRet, i, szbuf);
                            }*/
                            
                            // Delete from original position
                            ListView_DeleteItem(wndlist, iPos);

                            vorder.erase(vorder.begin() + iPos);
                            deletions++;
                            iPos = ListView_GetNextItem(wndlist, -1, LVNI_SELECTED);
                        }

                        pfc::array_t<size_t> order;
                        order.resize(vorder.size());

                        for (size_t i = 0; i < order.size(); i++) {
                            order[i] = vorder[i];
                        }

                        t_size preorderfocus = ListView_GetFocusItem(wndlist);

                        m_coord->reorder_map_elements(wndlist, &order[0], order.size(), lsmode::art);

                        //bit_array_bittable done; done.resize(order.size());
                        //size_t ifocus = ~0;

                        for (size_t walk = 0; walk < order.size(); walk++) {
                        //	//LV_ITEM lvitem = {};
                        //	if (order[walk] != walk && !done[order[walk]]) {
                        //		done.set(walk, true);
                        //		//lvitem.iItem = walk; // order[walk];
                        //		m_coord->swap_map_elements(wndlist, walk, order[walk], lsmode::art);
                        //		//lvitem.state |= (LVIS_SELECTED | (ifocus == ~0 ? LVIS_FOCUSED : 0));
                        //		//ifocus = walk;
                        //	}
                        //	//else {
                        //	//	lvitem.iItem = walk;
                        //	//	lvitem.state &= ~(LVIS_SELECTED | LVIS_FOCUSED);
                        //	//}
                        //	//lvitem.mask = LVIF_STATE;
                        //	//ListView_SetItem(wndlist, &lvitem);

                            ListView_SetItemState(wndlist, walk,
                                ~(LVNI_SELECTED | LVNI_FOCUSED),
                                LVIS_SELECTED | LVIS_FOCUSED);
                        }

                        //ListView_DeleteAllItems(wndlist);
                        ListView_RedrawItems(wndlist, 0, ListView_GetItemCount(wndlist));
                        
                        //b_drag_res = TRUE;

                        //Reselect and focus
                        if (m_itemTarget != -1) {

                            size_t postorderfocus = order[preorderfocus];

                            for (int walk = 0; walk < cSel; walk++) {
                                ListView_SetItemState(wndlist, m_itemTarget + walk,
                                    //LVNI_SELECTED | (walk == postorderfocus ? LVNI_FOCUSED : 0),									
                                    //LVIS_SELECTED | (walk == postorderfocus ? LVIS_FOCUSED : 0));
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
