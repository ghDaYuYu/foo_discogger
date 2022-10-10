#include "stdafx.h"

#include "find_release_dialog.h"
#include "find_artist_list.h"

const size_t TRACER_IMG_NDX = 3;
const size_t LOADED_RELEASES_IMG_NDX = 6;
const size_t SHOWING_IMAGE_NDX = 1;

CArtistList::CArtistList(IListControlOwnerDataSource* ilo, id_tracer* idtracer_p)
	: CListControlOwnerData(ilo), m_idtracer_p(idtracer_p) {
	//..
}

size_t CArtistList::get_next_role_pos() {

	if (m_last_role_pos > MAX_ARTISTS) m_last_role_pos = 0;
	return ++m_last_role_pos;
}

void CArtistList::on_get_artist_done(cupdRelSrc updsrc, Artist_ptr& artist) {

	if (updsrc == updRelSrc::Undef || updsrc == updRelSrc::UndefFast) {

		//from on init

		m_find_release_artist = artist;
		m_artist_exact_matches.force_reset();
		m_artist_other_matches.force_reset();
		set_artists(true, false, artist, m_artist_exact_matches, m_artist_other_matches);
		return;
	}
	else if (updsrc == updRelSrc::ArtistProfile) {

		//update profile

		CFindReleaseDialog* dlg = static_cast<CFindReleaseDialog*>(m_host);

		{
			std::lock_guard<std::mutex> guard(dlg->m_loading_selection_rw_mutex);

			if (dlg->m_loading_selection_id == atoi(artist->id)) {

				g_discogs->find_release_dialog->UpdateArtistProfile(artist);

				dlg->m_loading_selection_id = pfc_infinite;
			}
		}
		return;
	}
	else if (updsrc == updRelSrc::ArtistList) {
		Artist_ptr selected_artist = get_selected_artist();
		size_t selected_id = selected_artist ? atoi(selected_artist->id) : pfc_infinite;
		if (selected_id == pfc_infinite || atoi(artist->id) == selected_id) {
	
			CFindReleaseDialog* dlg = static_cast<CFindReleaseDialog*>(m_host);
			{
				std::lock_guard<std::mutex> guard(dlg->m_loading_selection_rw_mutex);

				if (dlg->m_loading_selection_id == atoi(artist->id) ||
					dlg->m_loading_selection_id == pfc_infinite) {

					fill_artist_list(false /*not from dlg*/, true, updsrc);

					set_find_release_artist(artist);

					dlg->m_loading_selection_id = pfc_infinite;
				}
			}
		}
	}
}

void CArtistList::fill_artist_list(bool dlgexact, bool force_exact, updRelSrc updsrc) {

	if (updsrc == updRelSrc::ArtistSearch) {


		// ARTIST-SEARCH

		// search button callback with text

		// both search artist name and by id

		// set_artist(exact, artist, exactmatch, othermatch) already called


		m_idtracer_p->artist_reset();
	}

	else if (updsrc == updRelSrc::ArtistListCheckExact) {

		// EXACT CHECKBOX

		switch_exact_matches(dlgexact, false);
	}

	else if (updsrc == updRelSrc::Undef || updsrc == updRelSrc::UndefFast) {

		//  ON INIT DIALOG

		if (!get_size()) {

			set_artists(true, false, m_find_release_artist, m_artist_exact_matches, m_artist_other_matches);
		}
	}
	else if (updsrc == updRelSrc::ArtistList || updsrc != updRelSrc::ArtistProfile) {

		// ARTIST LIST AND .ANY NON-ARTIST PROFILE. ?

		// get artist callback:
		// on change list selection with profile unavailable 
		// on artist list doubled clicked on enter key

		size_t prev_role_pos = m_find_release_artist ? m_find_release_artist->search_role_list_pos : ~0;
		size_t prev_list_pos = ~0;
		for (size_t w = 0; w < m_find_release_artists.get_count(); w++) {
			if (m_find_release_artists[w]->search_role_list_pos == prev_role_pos) {
				prev_list_pos = w + 1; //1 base
				break;
			}
		}

		t_size pos = GetFirstSelected();

		// * CHANGE CURRENT ARTIST

		m_find_release_artist = Get_Artists()[pos];

		//..
		//refresh lv images

		ListView_RedrawItems(this->m_hWnd, pos, pos);
		
		if (prev_list_pos != ~0 && prev_list_pos != pos) {
			
			//refresh prev item
			ListView_RedrawItems(this->m_hWnd, prev_list_pos, prev_list_pos);
		}
		m_idtracer_p->artist_check(m_find_release_artist->id, 0);
	}
}

void CArtistList::set_artists(bool bexact_matches, bool append, const Artist_ptr p_get_artist, const pfc::array_t<Artist_ptr>& p_artist_exact_matches,
	const pfc::array_t<Artist_ptr>& p_artist_other_matches) {

	//0 exact, 1 single, 3 exact+other

	if (p_get_artist) {
		switch_find_releases(1, append);
	}
	else {
		if (!append) {
			m_artist_exact_matches = p_artist_exact_matches;
			m_artist_other_matches = p_artist_other_matches;
		}
		else {
			m_artist_exact_matches.append(p_artist_exact_matches);
			m_artist_other_matches.append(p_artist_other_matches);
		}
		switch_exact_matches(bexact_matches, append);
	}
}

void CArtistList::switch_exact_matches(bool exact_matches, bool append) {

	if (exact_matches) {
		switch_find_releases(0, append);
	}
	else {
		switch_find_releases(3, append);
	}
}

void CArtistList::switch_find_releases(size_t op, bool append) {

	switch (op) {
	case 0:
		m_find_release_artists = m_artist_exact_matches;
		break;
	case 1:
		m_artist_exact_matches.add_item(m_find_release_artist);
		m_find_release_artists = m_artist_exact_matches;
		break;
	case 2:
		break;
	case 3:
		if (!append) {
			m_find_release_artists = m_artist_exact_matches;
		}
		else {
			int base = m_find_release_artists.get_count();
			int cadd = m_artist_exact_matches.get_count() - base;
			m_find_release_artists.resize(base + cadd);
			int c = 0;
			for (auto w = base; w < base + cadd; w++) {
				//copy last cadd elements
				m_find_release_artists[w] = m_artist_exact_matches[w];
				m_find_release_artists[w]->search_role_list_pos = get_next_role_pos();
			}
		}
		m_find_release_artists.append(m_artist_other_matches);
		break;
	}

	//reset ndx artist id (ref. artist search role)

	for (size_t i = 0; !append && i < m_find_release_artists.get_count(); i++) {
		m_find_release_artists[i]->search_role_list_pos = get_next_role_pos();
	}
	CFindReleaseDialog* dlg = static_cast<CFindReleaseDialog*>(m_host);

	Invalidate();

	auto citems = get_size();
	bool bskip_idded_release_dlg = CONF.skip_mng_flag & SkipMng::RELEASE_DLG_IDED;
	if (!bskip_idded_release_dlg && !append == true && ((CONF.enable_autosearch && op == 3) ||
		(CONF.auto_rel_load_on_open && op == 1 && citems == 1) ||
		(CONF.auto_rel_load_on_open && op == 3) ||
		(op == 0 && citems == 1 && append == false)))
	{
		SetSelectionAt(0, true);
	}
}

void CArtistList::set_image_list() {

	m_hImageList.Create(IDB_BITMAP_TRACER, 16, ILC_COLOR4, RGB(255, 255, 255));
	ListView_SetImageList(m_hWnd, m_hImageList, LVSIL_SMALL);
}

bool CArtistList::OnDisplayCellImage(int item, int subitem, int& result) const {

	int img_ndx = I_IMAGENONE;
	bool is_traced;

	if (m_idtracer_p->is_multi_artist()) {

		auto v = m_idtracer_p->get_vartist_ids();
		auto fit = std::find(v.begin(), v.end(), atoi(m_find_release_artists[item]->id));
		is_traced = fit != v.end();
	}
	else {
		is_traced = atoi(m_find_release_artists[item]->id) == m_idtracer_p->get_artist_id();
	}

	if (is_traced) {
		img_ndx = TRACER_IMG_NDX;
	}
	else {

		CFindReleaseDialog* dlg = static_cast<CFindReleaseDialog*>(m_host);
		size_t artist_pos_showing = dlg->get_tree_artist_list_pos();
		size_t item_rol_pos = m_find_release_artists[item]->search_role_list_pos;
		if (item_rol_pos == artist_pos_showing) {
			img_ndx = SHOWING_IMAGE_NDX;
		}
	}
	result = img_ndx;
	return img_ndx != I_IMAGENONE;
}

bool CArtistList::OnDisplayCellString(int item, int subitem, pfc::string8& result) {

	result = m_find_release_artists[item]->name;
	return true;
}

size_t CArtistList::get_artist_id(size_t pos) {

	if (get_size() > pos)
		return atoi(m_find_release_artists[pos]->id);

	if (m_find_release_artist)
		return atoi(m_find_release_artist->id);

	return pfc_infinite;
}

size_t CArtistList::get_size() {
	return m_find_release_artists.get_count() ?
		m_find_release_artists.get_count() : 0;
}

Artist_ptr CArtistList::get_artist_inlist(size_t list_index) {

	if (list_index != pfc::infinite_size && m_find_release_artists.get_count() > list_index) {
		return m_find_release_artists[list_index];
	}
	return nullptr;
}

Artist_ptr CArtistList::get_selected_artist() {

	const size_t pos = GetSingleSel();
	return get_artist_inlist(pos);
}

size_t CArtistList::get_artist_role_pos(size_t szartist_id) {

	if (!get_size()) return ~0;

	if (m_find_release_artist) {

		if (atoi(m_find_release_artist->id) == szartist_id) {
			return m_find_release_artist->search_role_list_pos;
		}
	}
	else if (m_find_release_artists.get_count()) {

		Artist_ptr artist;
		for (size_t walk = 0; walk < m_find_release_artists.get_count(); walk++) {

			if (atoi(m_find_release_artists[walk]->id) == szartist_id) {

				return m_find_release_artists[walk]->search_role_list_pos;
			}
		}
	}
	return ~0;
}

// on context menu

LRESULT CArtistList::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {

	CPoint p = GetContextMenuPoint(lParam);

	bHandled = FALSE; // tree handle needs event too. todo: rev own message loops

	HWND hwndCtrl = (HWND)wParam;
	CFindReleaseDialog* dlg = static_cast<CFindReleaseDialog*>(m_host);
	if (hwndCtrl != this->m_hWnd) return FALSE;

	CPoint screen_position;

	size_t list_index = GetSingleSel();

	if (lParam != -1) {

		screen_position = CPoint(LOWORD(lParam), HIWORD(lParam));
	}
	else {
		::GetCursorPos(&screen_position);
	}

	context_menu(list_index, screen_position);
	bHandled = TRUE;
	return FALSE;
}

//default action for artist list item

void CArtistList::Default_Action() {

	size_t pos = GetFirstSelected();

	if (pos == ~0) return;
	
	Artist_ptr artist = m_find_release_artists[pos];

	if (atoi(artist->id) != m_idtracer_p->get_artist_id()) {

		set_artist_tracer_ovr(atoi(artist->id));
	}

	// * CONVEY ARTIST-LIST

	CFindReleaseDialog* dlg = static_cast<CFindReleaseDialog*>(m_host);
	dlg->convey_artist_list_selection(updRelSrc::ArtistList);
}

// CListControlOwnerData overrides

bool CArtistList::RenderCellImageTest(size_t item, size_t subItem) const {
	int dummy;
	OnDisplayCellImage(item, subItem, dummy);
	return dummy == TRACER_IMG_NDX;
}

void CArtistList::RenderCellImage(size_t item, size_t subItem, CDCHandle dc, const CRect& rc) const  {

	ICONINFO ii;
	auto res = GetIconInfo(g_hIcon_quian, &ii);
	BITMAP bm;
	res = GetObject(ii.hbmMask, sizeof(bm), &bm) == sizeof(bm);
		dc.DrawIconEx(CPoint(rc.TopLeft().x, rc.TopLeft().y), g_hIcon_quian, CSize(bm.bmWidth, bm.bmWidth));
}

// open artist profile panel

void CArtistList::ShowArtistProfile() {

	//serves dlg bind panel checkbox

	size_t pos = GetFirstSelected();
	
	open_artist_profile(pos);
}


void CArtistList::open_artist_profile(size_t list_index) {

	if (!g_discogs->find_release_artist_dialog) {
		g_discogs->find_release_artist_dialog = fb2k::newDialog<CFindReleaseArtistDialog>(core_api::get_main_window(), SW_SHOW, HIWORD(CONF.custom_font));
	}

	if (list_index != pfc_infinite) {

		Artist_ptr artist = get_artist_inlist(list_index);

		if (artist) {
			
			g_discogs->find_release_dialog->UpdateArtistProfile(artist);
		}		
	}

	::SetFocus(g_discogs->find_release_artist_dialog->m_hWnd);
}


void CArtistList::context_menu(size_t list_index, POINT screen_pos) {

	bool empty_sel = (list_index == ~0);

	CFindReleaseDialog* dlg = static_cast<CFindReleaseDialog*>(m_host);

	Artist_ptr artist;
	bool isArtistOffline = false;

	if (!empty_sel) {

		if (artist = get_selected_artist()) {
			isArtistOffline = artist->loaded_releases_offline;
		}

	}

	pfc::string8 sourcepage = "Web &artist page";
	pfc::string8 copyrow = "&Copy";

	try {

		enum { ID_CMD_LOAD_RELEASES = 1, ID_VIEW_PAGE, ID_CLP_COPY_ROW, ID_ARTIST_DEL_CACHE, ID_ARTIST_EXACT_MATCHES, ID_ARTIST_PROFILE };
		HMENU menu = CreatePopupMenu();

		if (!empty_sel) {

			bool bexact_matches = ::IsDlgButtonChecked(dlg->m_hWnd, IDC_CHK_ONLY_EXACT_MATCHES);

			uAppendMenu(menu, MF_STRING | (!artist ? MF_DISABLED | MF_GRAYED : 0), ID_CMD_LOAD_RELEASES, "&Load artist releases");
			uAppendMenu(menu, MF_SEPARATOR, 0, 0);
			uAppendMenu(menu, MF_STRING | (!artist ? MF_DISABLED | MF_GRAYED : 0), ID_CLP_COPY_ROW, copyrow);
			uAppendMenu(menu, MF_SEPARATOR, 0, 0);
			uAppendMenu(menu, MF_STRING | (bexact_matches? MF_CHECKED : MF_UNCHECKED), ID_ARTIST_EXACT_MATCHES, "&exact matches");
			uAppendMenu(menu, MF_STRING, ID_ARTIST_PROFILE, "&profile panel");
			uAppendMenu(menu, MF_SEPARATOR, 0, 0);
			if (isArtistOffline) {
				uAppendMenu(menu, MF_STRING | (!artist ? MF_DISABLED | MF_GRAYED : 0), ID_ARTIST_DEL_CACHE, "C&lear artist cache");
				uAppendMenu(menu, MF_SEPARATOR, 0, 0);
			}
			uAppendMenu(menu, MF_STRING | (!artist ? MF_DISABLED | MF_GRAYED : 0), ID_VIEW_PAGE, sourcepage);
		}
		
		int cmd = TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, screen_pos.x, screen_pos.y, 0, dlg->m_hWnd, 0);
		DestroyMenu(menu);

		if (!cmd) return;

		switch (cmd)
		{
		case ID_CMD_LOAD_RELEASES:
		{
			Default_Action();
			break;
		}
		case ID_VIEW_PAGE:
		{
			pfc::string8 url;
			url << "https://www.discogs.com/artist/" << artist->id;			
			display_url(url);
			break;
		}
		case ID_CLP_COPY_ROW:
		{
			auto isel = GetSingleSel();
			if (isel != ~0) {
				pfc::string8 out;
				GetSubItemText(isel, 1, out);
				ClipboardHelper::OpenScope scope;
				scope.Open(core_api::get_main_window(), true);
				ClipboardHelper::SetString(out);
			}
			break;
		}
		case ID_ARTIST_DEL_CACHE:
		{
			discogs_interface->delete_artist_cache(artist->id);
			break;
		}	
		case ID_ARTIST_EXACT_MATCHES: {

			bool prev = ::IsDlgButtonChecked(dlg->m_hWnd, IDC_CHK_ONLY_EXACT_MATCHES);
			uButton_SetCheck(dlg->m_hWnd, IDC_CHK_ONLY_EXACT_MATCHES, !prev);
			::SendMessage(dlg->m_hWnd, WM_COMMAND, MAKEWPARAM(IDC_CHK_ONLY_EXACT_MATCHES, BN_CLICKED), 0);

			break;
		}
		case ID_ARTIST_PROFILE: {

			open_artist_profile(list_index);
			break;
		}
		}
	}
	catch (...) {}
}
