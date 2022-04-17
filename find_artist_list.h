#pragma once
#include <atltypes.h>
#include <windef.h>
#include <gdiplus.h>

#include "resource.h"
#include "libPPUI\clipboard.h"
#include "icon_map.h"
#include "CGdiPlusBitmap.h"

#include "discogs.h"
#include "discogs_interface.h"
#include "discogs_db_interface.h"

#include "history_oplog.h"
#include "find_release_utils.h"
#include "find_release_artist_dlg.h"

using namespace Gdiplus;
using namespace Discogs;

//#define DEBUG_TREE

class CFindReleaseDialog;
class CFindReleaseTree;

class CArtistList : public CMessageMap {

private:

	CFindReleaseDialog* m_dlg = nullptr;

	HWND m_hwndArtists;
	HWND m_edit_artist;

	CImageList m_hImageList;

	LV_ITEM* m_lv_selected = NULL;
	LV_ITEM* m_lv_hit = NULL;

	//app

	id_tracer* _idtracer_p;

	size_t m_lost_selection_id = pfc_infinite;
	bool m_dispinfo_enabled;

	//artist and matches

	Artist_ptr m_find_release_artist;
	pfc::array_t<Artist_ptr> m_find_release_artists;

	pfc::array_t<Artist_ptr> m_artist_exact_matches;
	pfc::array_t<Artist_ptr> m_artist_other_matches;

	//notifiers

	std::function<bool(int lparam)>stdf_on_artist_selected_notifier;
	std::function<bool()>stdf_on_ok_notifier;

	size_t m_last_role_pos = 0;
	size_t get_next_role_pos();

public:

	CArtistList(CFindReleaseDialog* dlg)
		: m_dlg(dlg) {

		m_dispinfo_enabled = true;

		m_hwndArtists = NULL;
		m_edit_artist = NULL;

		m_find_release_artist;

		m_lv_selected = NULL;
		m_lv_hit = NULL;
	}

	~CArtistList() {

		DeleteObject(m_hImageList);

	}

	//serves dlg->expand_master_release, convery

	const Artist_ptr Get_Artist() { return m_find_release_artist; }
	pfc::array_t<Artist_ptr> Get_Artists() { return m_find_release_artists; }

	//serves dlg
	void ShowArtistProfile();

	//

	void on_get_artist_done(updRelSrc updsrc, Artist_ptr& artist);
	void fill_artist_list(bool dlgexcact, bool force_exact, updRelSrc updsrc);

	//

	size_t get_artist_id(size_t pos);
	size_t get_artist_role_pos(size_t szartist_id);

	void switch_exact_matches(bool exact_matches);

	void set_artists(bool bexact_matches,
		const Artist_ptr p_get_artist,
		const pfc::array_t<Artist_ptr>& p_artist_exact_matches,
		const pfc::array_t<Artist_ptr>& p_artist_other_matches);

	void Inititalize(HWND artistlist, HWND search_edit, id_tracer* tracer_p) {

		m_hwndArtists = artistlist;
		m_edit_artist = search_edit;

		_idtracer_p = tracer_p;

		set_image_list();

		ListView_SetExtendedListViewStyleEx(m_hwndArtists,
			LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_LABELTIP,
			LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_LABELTIP);

		CRect rc;
		::GetClientRect(m_hwndArtists, &rc);

		//add column
		listview_helper::fr_insert_column(m_hwndArtists, 0, "Artist", rc.Width(), LVCFMT_LEFT);
	}

	void Default_Artist_Action();


#pragma warning( push )
#pragma warning( disable : 26454 )

	BEGIN_MSG_MAP(CArtistList)
		NOTIFY_HANDLER(IDC_ARTIST_LIST, LVN_GETDISPINFO, OnGetInfo)
		NOTIFY_HANDLER(IDC_ARTIST_LIST, LVN_ITEMCHANGED, OnListSelChanged)
		NOTIFY_HANDLER(IDC_ARTIST_LIST, LVN_ITEMCHANGING, OnListSelChanging)
		MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
		NOTIFY_HANDLER(IDC_ARTIST_LIST, NM_RCLICK, OnRClickRelease)
		//NOTIFY_HANDLER(IDC_ARTIST_LIST, NM_RETURN, OnListReturn) (parent hook)
		NOTIFY_HANDLER(IDC_ARTIST_LIST, NM_DBLCLK, OnListDoubleClick)
	END_MSG_MAP()

#pragma warning(pop)

private:

	size_t get_size();
	Artist_ptr get_artist_inlist(size_t list_index);
	Artist_ptr get_selected_artist();

	pfc::string8 get_search_string() { return uGetWindowText(m_edit_artist); }

	void set_artist_tracer_ovr(size_t artist_ovr_id) { _idtracer_p->set_artist_ovr_id(artist_ovr_id); }

	void set_find_release_artist(Artist_ptr find_release_artist_p) {
		m_find_release_artist = find_release_artist_p;
	}
	void set_find_release_artists(pfc::array_t<Artist_ptr> find_release_artists_p) {
		m_find_release_artists = find_release_artists_p;
	}

	void switch_find_releases(size_t op);

	void open_artist_profile(size_t list_index);
	void context_menu(size_t list_index, POINT screen_pos);

	LRESULT OnGetInfo(WORD /*wNotifyCode*/, LPNMHDR hdr, BOOL& /*bHandled*/);
	LRESULT OnListSelChanged(int, LPNMHDR hdr, BOOL& bHandled);
	LRESULT OnListSelChanging(int, LPNMHDR hdr, BOOL& bHandled);
	LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnRClickRelease(int, LPNMHDR hdr, BOOL&);
	LRESULT OnListDoubleClick(int, LPNMHDR hdr, BOOL& bHandled);

	void set_image_list();

	//pass data to getdispinfo
	bool OnDisplayCellImage(int item, int subitem, int& result);
	bool OnDisplayCellString(int item, int subitem, pfc::string8& result);
};