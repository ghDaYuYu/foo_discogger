#pragma once
#include <atltypes.h>
#include <windef.h>
#include "../SDK/foobar2000.h"
#include "libPPUI\clipboard.h"

#include "discogs.h"
#include "discogs_interface.h"
#include "find_release_utils.h"

using namespace Discogs;

class CFindReleaseTree : public CMessageMap {
private:
	HWND p_treeview;
	HWND p_artistlist;
	HWND p_parent_filter_edit;
	HWND p_parent_url_edit;

	HTREEITEM m_selected;
	size_t m_tree_artist;
	bool m_dispinfo_enabled;

	DiscogsInterface* m_discogs_interface;
	Artist_ptr m_find_release_artist_p;
	pfc::array_t<Artist_ptr> m_find_release_artists_p;

	std::shared_ptr<filter_cache> m_cache_ptr;
	std::shared_ptr<vec_t> m_vec_ptr;

	id_tracer m_idtracer;

	HTREEITEM m_hhit;

	std::function<bool(int lparam)>stdf_on_release_selected_notifier;
	std::function<bool()>stdf_on_ok_notifier;

public:

	const unsigned m_ID;

	CFindReleaseTree(HWND p_parent, unsigned ID) : m_ID(ID), 	
		m_cache_ptr(std::make_shared<filter_cache>()),
		m_vec_ptr(std::make_shared<vec_t>()), m_selected(NULL) {

		m_dispinfo_enabled = true;
		m_hhit = nullptr;

		p_treeview = NULL;
		p_parent_filter_edit = NULL;
		p_parent_url_edit = NULL;
		p_artistlist = NULL;

		m_discogs_interface = nullptr;
		m_find_release_artist_p = nullptr;
		m_find_release_artists_p = {};

		m_tree_artist = pfc_infinite;
	}

	void SetOnSelectedNotifier(std::function<bool(int)>stdf_notifier) {
		stdf_on_release_selected_notifier = stdf_notifier;
	}

	void SetOnOkNotifier(std::function<bool()>stdf_notifier) {
		stdf_on_ok_notifier = stdf_notifier;
	}

	void Inititalize(HWND treeview, HWND artistlist, HWND filter_edit, HWND url_edit) {
		p_treeview = treeview;
		p_parent_filter_edit = filter_edit;
		p_parent_url_edit = url_edit;
		p_artistlist = artistlist;
	}

	void SetFindReleases(Artist_ptr find_release_artist_p, id_tracer idtracer) {
		m_find_release_artist_p = find_release_artist_p;
		m_idtracer = idtracer;
	}

	size_t GetTreeViewArtist() {
		return m_idtracer.artist_id;
	}

	void SetFindReleaseArtists(pfc::array_t<Artist_ptr> find_release_artists_p) {
		m_find_release_artists_p = find_release_artists_p;
	}

	void SetDiscogsInterface(DiscogsInterface* discogs_interface) {
		m_discogs_interface = discogs_interface;
	}

	void SetCache(std::shared_ptr<filter_cache>&cached) {
		m_cache_ptr = cached;
	}
	void SetVec(std::shared_ptr<vec_t>& vec_items, const id_tracer _idtracer) {
		
		m_dispinfo_enabled = false;
		TreeView_DeleteAllItems(p_treeview);
		m_dispinfo_enabled = true;

		m_vec_ptr = vec_items;
		m_idtracer = _idtracer;

		int counter = 0;
		for (auto& walk : *m_vec_ptr)
		{
			TVINSERTSTRUCT tvis = { 0 };

			tvis.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM;
			tvis.item.pszText = LPSTR_TEXTCALLBACK;
			tvis.item.iImage = I_IMAGECALLBACK;
			tvis.item.iSelectedImage = I_IMAGECALLBACK;
			tvis.item.cChildren = I_CHILDRENCALLBACK;
			tvis.item.lParam = walk.first->first;
			tvis.hParent = TVI_ROOT; //pNMTreeView->itemNew.hItem;
			tvis.hInsertAfter = TVI_LAST;
			if (walk.first->second.first.id == _idtracer.master_id) {
				tvis.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM | TVIF_STATE;
				tvis.item.state = TVIS_BOLD;
				tvis.item.stateMask = TVIS_BOLD;
			}
			else {
				tvis.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM;
			}
			HTREEITEM newnode = TreeView_InsertItem(p_treeview, &tvis);
			walk.second = newnode;
		}

	}

	void SetHit(int lparam) {
		//prepare to m_hHit node
		CTreeViewCtrl tree(p_treeview);
		HTREEITEM first = tree.GetRootItem();
		for (HTREEITEM walk = first; walk != NULL; walk = tree.GetNextVisibleItem(walk)) {
			TVITEM pItemMaster = { 0 };
			pItemMaster.mask = TVIF_PARAM;
			pItemMaster.hItem = walk;
			TreeView_GetItem(p_treeview, &pItemMaster);
			if (pItemMaster.lParam == lparam) {
				m_hhit = walk;
				break;
			}
		}
	}

	void ExpandHit() {
		if (m_hhit != NULL) {
			TreeView_Expand(p_treeview, m_hhit, TVM_EXPAND);
			m_hhit = nullptr;
		}
	}

	void EnableDispInfo(bool enable) {
		m_dispinfo_enabled = enable;
	}

	//message handlers are chained, notify handler should not mapped in owner

	BEGIN_MSG_MAP(CFindReleaseTree)
		NOTIFY_HANDLER(IDC_RELEASE_TREE, TVN_GETDISPINFO, OnTreeGetInfo)
		NOTIFY_HANDLER(IDC_RELEASE_TREE, TVN_ITEMEXPANDING, OnReleaseTreeExpanding)
		NOTIFY_HANDLER(IDC_RELEASE_TREE, TVN_SELCHANGING, OnReleaseTreeSelChanging)
		NOTIFY_HANDLER(IDC_RELEASE_TREE, TVN_SELCHANGED, OnReleaseTreeSelChanged)
		NOTIFY_HANDLER(IDC_RELEASE_TREE, NM_DBLCLK, OnDoubleClickRelease)
		NOTIFY_HANDLER(IDC_RELEASE_TREE, NM_RCLICK, OnRClickRelease)
		NOTIFY_HANDLER(IDC_ARTIST_LIST, NM_RCLICK, OnRClickRelease)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	END_MSG_MAP()
private:

	t_size get_mem_cache_children_count(LPARAM lparam);

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		return FALSE;
	}

	LRESULT OnReleaseTreeSelChanged(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		CTreeViewCtrl ctrl(p_treeview);
		HTREEITEM hsel = ctrl.GetSelectedItem();

		TVITEM pItemMaster = { 0 };
		pItemMaster.mask = TVIF_PARAM;
		pItemMaster.hItem = hsel;
		TreeView_GetItem(p_treeview, &pItemMaster);

		m_selected = hsel;
		return 0;
	}

	LRESULT OnDoubleClickRelease(int, LPNMHDR hdr, BOOL&) {
		LPNMITEMACTIVATE nmListView = (LPNMITEMACTIVATE)hdr;
		NMTVDISPINFO* pDispInfo = reinterpret_cast<NMTVDISPINFO*>(hdr);
		TVITEMW* pItem = &(pDispInfo)->item;
		HTREEITEM* hItem = &(pItem->hItem);
		HTREEITEM hhit;

		int lparam = pItem->lParam;
		mounted_param myparam(lparam);

		HWND treev = GetDlgItem(core_api::get_main_window(), IDC_RELEASE_TREE);

		POINT p;
		GetCursorPos(&p);
		::ScreenToClient(p_treeview, &p);

		TVHITTESTINFO hitinfo = { 0 };
		hitinfo.pt = p;

		if (hhit = (HTREEITEM)SendMessage(p_treeview,
			TVM_HITTEST, NULL, (LPARAM)&hitinfo)) {
			TVITEM phit = { 0 };
			phit.mask = TVIF_PARAM;
			phit.hItem = hhit;
			TreeView_GetItem(p_treeview, &phit);
			lparam = phit.lParam;
			myparam = mounted_param(lparam);

			if (hitinfo.flags & TVHT_ONITEM) {
				//use right click also to select items
				TreeView_SelectItem(p_treeview, hhit);
			}
		}
		else {
			return FALSE;
		}
		t_size list_index = nmListView->iItem;

		if (!myparam.is_master()) {
			stdf_on_ok_notifier();
		}
		return FALSE;
	}

	int fr_get_selected_item_param() {

		TVITEM pItemMaster = { 0 };
		pItemMaster.mask = TVIF_PARAM;
		pItemMaster.hItem = m_selected;
		TreeView_GetItem(p_treeview, &pItemMaster);
		return pItemMaster.lParam;
	}

	LRESULT OnRClickRelease(int, LPNMHDR hdr, BOOL&) {
		//list
		LPNMITEMACTIVATE nmView = (LPNMITEMACTIVATE)hdr;
		//tree
		NMTVDISPINFO* pDispInfo = reinterpret_cast<NMTVDISPINFO*>(hdr);
		TVITEMW* pItem = &(pDispInfo)->item;
		HTREEITEM* hItem = &(pItem->hItem);
		HTREEITEM hhit = NULL;
		//..

		bool isArtist = hdr->hwndFrom == p_artistlist;
		bool isReleaseTree = hdr->hwndFrom == p_treeview;

		t_size list_index = -1;
		int lparam = -1;
		mounted_param myparam(lparam);

		POINT p;
		if (isArtist) {
			list_index = nmView->iItem;
			if (list_index == pfc_infinite)
				return FALSE;

			if (m_find_release_artists_p.get_count() > 0)
				isArtistOffline = m_find_release_artists_p[list_index]->loaded_releases_offline;
			else
				if (m_find_release_artist_p != NULL)
					isArtistOffline = m_find_release_artist_p->loaded_releases_offline;

			p = nmView->ptAction;
			::ClientToScreen(hdr->hwndFrom, &p);
		}
		else if (isReleaseTree) {
			//get client point to hit test
			GetCursorPos(&p);
			::ScreenToClient(p_treeview, &p);
			TVHITTESTINFO hitinfo = { 0 };
			hitinfo.pt = p;
			if (hhit = (HTREEITEM)SendMessage(p_treeview,
				TVM_HITTEST, NULL, (LPARAM)&hitinfo)) {
				TVITEM phit = { 0 };
				phit.mask = TVIF_PARAM;
				phit.hItem = hhit;
				TreeView_GetItem(p_treeview, &phit);
				lparam = phit.lParam;
				myparam = mounted_param(lparam);
				if (hitinfo.flags & TVHT_ONITEM) {
					//use right click also for selection
					TreeView_SelectItem(p_treeview, hhit);
				}
			}
			else {
				return FALSE;
			}
			//screen position for context panel
			GetCursorPos(&p);
		}
		lparam = myparam.lparam();

		pfc::string8 sourcepage = isArtist ? "View Artist page" : !myparam.brelease ? "View Master Release page" : "View Release page";
		pfc::string8 copytitle = "Copy Title to Clipboard";
		pfc::string8 copyrow = "Copy to Clipboard";

		try {
			int coords_x = p.x, coords_y = p.y;
			enum { ID_VIEW_PAGE = 1, ID_CLP_COPY_TITLE, ID_CLP_COPY_ROW, ID_ARTIST_DEL_CACHE };
			HMENU menu = CreatePopupMenu();
			uAppendMenu(menu, MF_STRING, ID_VIEW_PAGE, sourcepage);
			uAppendMenu(menu, MF_SEPARATOR, 0, 0);
			if (isArtist) {
				uAppendMenu(menu, MF_STRING, ID_CLP_COPY_ROW, copyrow);
				if (isArtistOffline) {
					uAppendMenu(menu, MF_SEPARATOR, 0, 0);
					uAppendMenu(menu, MF_STRING, ID_ARTIST_DEL_CACHE, "Clear artist cache");
				}
			}
			else {
				uAppendMenu(menu, MF_STRING, ID_CLP_COPY_TITLE, copytitle);
				uAppendMenu(menu, MF_STRING, ID_CLP_COPY_ROW, copyrow);
			}
			int cmd = TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, coords_x, coords_y, 0, core_api::get_main_window(), 0);
			DestroyMenu(menu);
			switch (cmd)
			{
			case ID_VIEW_PAGE:
			{
				pfc::string8 url;
				if (isArtist) {
					if (m_find_release_artists_p.get_count() > 0)
						url << "https://www.discogs.com/artist/" << m_find_release_artists_p[list_index]->id;
					else
						if (m_find_release_artist_p != NULL)
							url << "https://www.discogs.com/artist/" << m_find_release_artist_p->id;
				}
				else {
					if (myparam.is_release()) {
						url << "https://www.discogs.com/release/" << m_find_release_artist_p->master_releases[myparam.master_ndx]->sub_releases[myparam.release_ndx]->id;
					}
					else
						if (myparam.bmaster) {
							url << "https://www.discogs.com/master/" << m_find_release_artist_p->master_releases[myparam.master_ndx]->id;
						}
						else {
							url << "https://www.discogs.com/release/" << m_find_release_artist_p->releases[myparam.release_ndx]->id;
						}
				}
				if (url != NULL)
					display_url(url);

				return true;
			}
			case ID_CLP_COPY_TITLE:
			{
				pfc::string8 buffer;
				if (isArtist) {
					;
				}
				else {
					if (myparam.is_release()) {

						buffer << m_find_release_artist_p->master_releases[myparam.master_ndx]->sub_releases[myparam.release_ndx]->title;
					}
					else
						if (myparam.bmaster) {
							buffer << m_find_release_artist_p->master_releases[myparam.master_ndx]->title;
						}
						else {
							buffer << m_find_release_artist_p->releases[myparam.release_ndx]->title;
						}
				}
				if (buffer != NULL) {
					ClipboardHelper::OpenScope scope; scope.Open(core_api::get_main_window());
					ClipboardHelper::SetString(buffer);
				}

				return true;
			}
			case ID_CLP_COPY_ROW:
			{
				pfc::string8 utf_buffer;
				TCHAR outBuffer[MAX_PATH + 1] = {};

				if (isArtist) {
					if (nmView->iItem != -1) {
						LVITEM lvi;
						TCHAR outBuffer[MAX_PATH + 1] = {};
						lvi.pszText = outBuffer;
						lvi.cchTextMax = MAX_PATH;
						lvi.mask = LVIF_TEXT;
						lvi.stateMask = (UINT)-1;
						lvi.iItem = nmView->iItem;
						lvi.iSubItem = 0;
						BOOL result = ListView_GetItem(p_artistlist, &lvi);
						utf_buffer << pfc::stringcvt::string_utf8_from_os(outBuffer).get_ptr();
					}
				}
				else {
					if (hhit != NULL) {
						TVITEM phit = { 0 };
						phit.mask = TVIF_PARAM | TVIF_TEXT;
						phit.pszText = outBuffer;
						phit.cchTextMax = MAX_PATH;
						phit.hItem = hhit;
						TreeView_GetItem(p_treeview, &phit);
						utf_buffer << pfc::stringcvt::string_utf8_from_os(outBuffer).get_ptr();
					}
				}
				if (utf_buffer != NULL) {
					ClipboardHelper::OpenScope scope; scope.Open(core_api::get_main_window());
					ClipboardHelper::SetString(utf_buffer);
				}
				return true;
			}
			case ID_ARTIST_DEL_CACHE:
			{
				if (isArtist) {

					Artist_ptr artist;
					
					if (m_find_release_artists_p.get_count() > 0) 
						artist = m_find_release_artists_p[list_index];
					else
						if (m_find_release_artist_p != NULL)
							artist = m_find_release_artist_p;

					if (artist) {
						m_discogs_interface->delete_artist_cache(artist->id);
						artist->loaded_releases_offline = false;
					}
				}
				return true;
			}
			}
		}
		catch (...) {}

		return 0;
	}

	LRESULT OnClick(WORD /*wNotifyCode*/, LPNMHDR /*lParam*/, BOOL& /*bHandled*/) {
		CPoint pt(GetMessagePos());
		return FALSE;
	}
	LRESULT OnTreeGetInfo(WORD /*wNotifyCode*/, LPNMHDR hdr, BOOL& /*bHandled*/);
	LRESULT CFindReleaseTree::OnReleaseTreeExpanding(int, LPNMHDR hdr, BOOL&);
	LRESULT OnReleaseTreeSelChanging(int, LPNMHDR hdr, BOOL& bHandled);
	LRESULT OnReleaseTreeSelChanged(int, LPNMHDR hdr, BOOL& bHandled);

};