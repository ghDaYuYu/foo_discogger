#pragma once
#include <atltypes.h>
#include <windef.h>
#include "resource.h"
#include "libPPUI\clipboard.h"

#include "icon_map.h"
#include <gdiplus.h>
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

class CFindArtistList : public CMessageMap {

private:

	CFindReleaseDialog* m_dlg = nullptr;

	HWND m_hwndParent;
	HWND m_hwndArtists;
	HWND m_search_edit;

	HICON hiconItem;
	CImageList m_lstimg_small;
	HIMAGELIST hImageList = NULL;

	LV_ITEM* m_lv_selected = NULL;
	LV_ITEM* m_lv_hit = NULL;

	//app

	foo_conf* conf_p = nullptr;
	history_oplog* m_oplogger_p;
	id_tracer* _idtracer_p;

	size_t m_lost_selection_id = pfc_infinite;
	bool m_dispinfo_enabled;

	//dc

	DiscogsInterface* m_discogs_interface;

	Artist_ptr m_find_release_artist;
	pfc::array_t<Artist_ptr> m_find_release_artists;
	pfc::array_t<Artist_ptr> m_artist_exact_matches;
	pfc::array_t<Artist_ptr> m_artist_other_matches;

	//notifiers

	std::function<bool(int lparam)>stdf_on_artist_selected_notifier;
	std::function<bool()>stdf_on_ok_notifier;

public:

	const unsigned m_ID;

	CFindArtistList(HWND hwndParent, unsigned ID) :
		m_ID(ID), m_lv_selected(NULL) {

		m_dispinfo_enabled = true;
		m_lv_hit = nullptr;

		m_hwndParent = hwndParent;
		m_hwndArtists = NULL;

		m_search_edit = NULL;

		m_discogs_interface = nullptr;
		m_find_release_artist = nullptr;
		m_find_release_artists = {};
	}

	~CFindArtistList() {
		DeleteObject(hImageList);

		m_dlg = nullptr;
	}

	//serves dlg->expand_master_release, convery

	const Artist_ptr Get_Artist() { return m_find_release_artist; }
	pfc::array_t<Artist_ptr> Get_Artists() { return m_find_release_artists; }
	
	//..

	void on_get_artist_done(updRelSrc updsrc, Artist_ptr& artist);
	void fill_artist_list(bool dlgexcact, bool force_exact, updRelSrc updsrc);


	pfc::string8 get_artist_id(size_t pos);
	size_t get_artist_pos(size_t szartist_id);

	void set_exact_matches(bool exact_matches);
	void set_artists(bool bexact_matches, const Artist_ptr p_get_artist, const pfc::array_t<Artist_ptr>& p_artist_exact_matches,
		const pfc::array_t<Artist_ptr>& p_artist_other_matches);

	void Inititalize(CFindReleaseDialog* dlg,
		HWND artistlist, HWND search_edit, id_tracer* tracer_p) {

		m_dlg = dlg;		
		m_search_edit = search_edit;
		m_hwndArtists = artistlist;

		_idtracer_p = tracer_p;

		//image list
		m_lstimg_small.Create(IDB_BITMAP1, 16, ILC_COLOR4, RGB(255, 255, 255));
		ListView_SetImageList(m_hwndArtists, m_lstimg_small, LVSIL_SMALL);

		ListView_SetExtendedListViewStyleEx(m_hwndArtists,
			LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_LABELTIP, 
			LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_LABELTIP);
		
		CRect rc;
		::GetClientRect(m_hwndArtists, &rc);

        //add column
        listview_helper::fr_insert_column(m_hwndArtists, 0, "Artist", rc.Width(), LVCFMT_LEFT);
	}

	// (nVKReturn)

	void nVKReturn_Artists();

	//..

#pragma warning( push )
#pragma warning( disable : 26454 )

	BEGIN_MSG_MAP(CFindArtistList)
		NOTIFY_HANDLER(IDC_ARTIST_LIST, NM_RCLICK, OnRClickRelease)
		NOTIFY_HANDLER(IDC_ARTIST_LIST, NM_DBLCLK, OnListDoubleClick)
		NOTIFY_HANDLER(IDC_ARTIST_LIST, NM_RCLICK, OnRClickRelease)
	END_MSG_MAP()

#pragma warning(pop)

private:

	size_t test_hit_artist();
	size_t get_size();

	pfc::string8 get_search_string() { return uGetWindowText(m_search_edit); }

	void set_artist_tracer_fr(size_t artist_fr_id) { _idtracer_p->set_artist_fr_id(artist_fr_id); }

	void set_find_release_artist(Artist_ptr find_release_artist_p) {
		m_find_release_artist = find_release_artist_p;
	}
	void set_find_release_artists(pfc::array_t<Artist_ptr> find_release_artists_p) {
		m_find_release_artists = find_release_artists_p;
	}

	void switch_find_releases(size_t op);
	void RebuildListView() {};
	void open_artist_profile(size_t list_index);

	LRESULT OnClick(WORD /*wNotifyCode*/, LPNMHDR /*lParam*/, BOOL& /*bHandled*/) {
		CPoint pt(GetMessagePos());
		//..
		return FALSE;
	}

	LRESULT OnListSelChanged(int, LPNMHDR hdr, BOOL& bHandled);
	LRESULT OnListDoubleClick(int, LPNMHDR hdr, BOOL& bHandled);
	LRESULT OnRClickRelease(int, LPNMHDR hdr, BOOL&);

	//pass data to getdispinfo
	bool OnDisplayCellImage(int item, int subitem, int& result);
	bool OnDisplayCellString(int item, int subitem, pfc::string8& result);
};