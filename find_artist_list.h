#pragma once

#include "resource.h"
#include "libPPUI/CListControlOwnerData.h"
#include "find_release_utils.h"

class CArtistList : public CListControlOwnerData {

public:

	CArtistList(IListControlOwnerDataSource* ilo, id_tracer* idtracer_p);

	~CArtistList() {

		DeleteObject(m_hImageList);
	}

	// serves dlg->expand_master_release, convey
	const Artist_ptr Get_Artist() { return m_find_release_artist; }
	pfc::array_t<Artist_ptr> Get_Artists() { return m_find_release_artists;	}
	const pfc::string8 Get_Selected_Id () { auto p = get_selected_artist(); return p ? p->id : ""; }

	// serves dlg
	void ShowArtistProfile();

	// updrelsrc
	void on_get_artist_done(cupdRelSrc updsrc, Artist_ptr& artist);
	void fill_artist_list(bool dlgexcact, bool force_exact, updRelSrc updsrc);

	size_t get_artist_id(size_t pos);
	size_t get_artist_role_pos(size_t szartist_id);
	void switch_exact_matches(bool exact_matches, bool append);

	void set_artists(bool bexact_matches, bool append,
		const Artist_ptr p_get_artist,
		const pfc::array_t<Artist_ptr>& p_artist_exact_matches,
		const pfc::array_t<Artist_ptr>& p_artist_other_matches);

	void Inititalize() {

		set_image_list();

		SetSelectionModeSingle();
		SetRowStyle(rowStyleFlat);
		auto dpiX = QueryScreenDPIEx(m_hWnd).cx;
		CRect rc; GetClientRect(&rc);
		auto rcwidth = rc.Width() - GetSystemMetrics(SM_CXVSCROLL);
		int colw = MulDiv(rcwidth, dpiX, USER_DEFAULT_SCREEN_DPI);

		AddColumnEx("Artist", colw, LVCFMT_LEFT, true);

		if (CONF.mode_write_alt)
			::EnableWindow(m_hWnd, FALSE);
	}

	// (nVKReturn)
	void Default_Action();

	// CListControlOwnerData overrides

	bool RenderCellImageTest(size_t item, size_t subItem) const override;
	void RenderCellImage(size_t item, size_t subItem, CDCHandle, const CRect&) const override;

#pragma warning( push )
#pragma warning( disable : 26454 )
	typedef CListControlOwnerData TParent;
	BEGIN_MSG_MAP(CArtistList)
		MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
		CHAIN_MSG_MAP(TParent)
	END_MSG_MAP()

#pragma warning(pop)

	LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	bool OnDisplayCellImage(int item, int subitem, int& result) const;

private:

	size_t get_size();
	size_t get_next_role_pos();
	Artist_ptr get_artist_inlist(size_t list_index);
	Artist_ptr get_selected_artist();

	void set_artist_tracer_ovr(size_t artist_fr_id) { m_idtracer_p->set_artist_ovr_id(artist_fr_id); }
	void set_find_release_artist(Artist_ptr find_release_artist_p) { m_find_release_artist = find_release_artist_p; }
	void set_find_release_artists(pfc::array_t<Artist_ptr> find_release_artists_p) { m_find_release_artists = find_release_artists_p;	}

	void switch_find_releases(size_t op, bool append);
	void open_artist_profile(size_t list_index);
	void context_menu(size_t list_index, POINT screen_pos);

	void set_image_list();

	bool OnDisplayCellString(int item, int subitem, pfc::string8& result);

	id_tracer* m_idtracer_p;

	Artist_ptr m_find_release_artist;
	pfc::array_t<Artist_ptr> m_find_release_artists;

	pfc::array_t<Artist_ptr> m_artist_exact_matches;
	pfc::array_t<Artist_ptr> m_artist_other_matches;

	std::function<bool(int lparam)>stdf_on_artist_selected_notifier;
	std::function<bool()>stdf_on_ok_notifier;

	size_t m_last_role_pos = 0;
	CImageList m_hImageList;
};
