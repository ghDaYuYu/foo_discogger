#include "stdafx.h"

#include "find_release_dialog.h"
#include "find_artist_list.h"

const size_t TRACER_IMG_NDX = 3;
const size_t SHOWING_IMAGE_NDX = 1;
const size_t LIMIT_ARTISTS = 63;

size_t CArtistList::get_next_role_pos() {

	if (m_last_role_pos > LIMIT_ARTISTS) m_last_role_pos = 0;
	return ++m_last_role_pos;

}

void CArtistList::on_get_artist_done(updRelSrc updsrc, Artist_ptr& artist) {

	if (updsrc == updRelSrc::Undef || updsrc == updRelSrc::UndefFast) {

		//from on init

		m_find_release_artist = artist;
		m_artist_exact_matches.force_reset();
		m_artist_other_matches.force_reset();
		set_artists(true, artist, m_artist_exact_matches, m_artist_other_matches);
		
		return;

	}
	else if (updsrc == updRelSrc::ArtistProfile) {

		//update profile

		{
			std::lock_guard<std::mutex> guard(m_dlg->m_loading_selection_rw_mutex);

			if (m_dlg->m_loading_selection_id == atoi(artist->id)) {

				if (g_discogs->find_release_artist_dialog)
					g_discogs->find_release_artist_dialog->UpdateProfile(artist);


				m_dlg->m_loading_selection_id = pfc_infinite;
			}
		}

		return;
	}
	else if (updsrc == updRelSrc::ArtistList) {

		Artist_ptr selected_artist = get_selected_artist();
		size_t selected_id = selected_artist ? atoi(selected_artist->id) : pfc_infinite;

		if (selected_id == pfc_infinite || atoi(artist->id) == selected_id) {
	
			{
				std::lock_guard<std::mutex> guard(m_dlg->m_loading_selection_rw_mutex);

				if (m_dlg->m_loading_selection_id == atoi(artist->id) ||
					m_dlg->m_loading_selection_id == pfc_infinite) {

					fill_artist_list(false /*not from dlg*/, true, updsrc);

					set_find_release_artist(artist);

					m_dlg->m_loading_selection_id = pfc_infinite;
				}
			}
		}
	}
	else {
		int debugme = 1;
	}
}

void CArtistList::fill_artist_list(bool dlgexact, bool force_exact, updRelSrc updsrc) {

	if (updsrc == updRelSrc::ArtistSearch) {

		// search button callback

		// both search artist name and by id

		// set_artist(exact, artist, exactmatch, othermatch) already called

		_idtracer_p->artist_reset();
	}
	else if (updsrc == updRelSrc::ArtistListCheckExact) {

		// EXACT CHECKBOX

		switch_exact_matches(dlgexact);
	}

	else if (updsrc == updRelSrc::Undef || updsrc == updRelSrc::UndefFast) {

		//  ON INIT DIALOG

		if (!get_size()) {

			set_artists(true, m_find_release_artist, m_artist_exact_matches, m_artist_other_matches);
		}
	}
	else if (updsrc == updRelSrc::ArtistList || updsrc != updRelSrc::ArtistProfile) {

		// get artist callback:
		// on change list selection with profile unavailable 
		// on artist list doubled clicked on enter key

		size_t prev_role_pos = m_find_release_artist ? m_find_release_artist->search_role_list_pos : ~0;
		size_t prev_list_pos = ~0;
		for (size_t w = 0; w < m_find_release_artists.get_count(); w++) {
			if (m_find_release_artists[w]->search_role_list_pos == prev_role_pos) {
				prev_list_pos = w;
				break;
			}
		}

		t_size pos = ListView_GetFirstSelection(m_hwndArtists);
		
		//..

		m_find_release_artist = Get_Artists()[pos];

		//..

		//todo: misplaced
		//refresh lv images

		ListView_RedrawItems(m_hwndArtists, pos, pos);
		if (prev_list_pos != ~0 && prev_list_pos != pos) {
			//prev item
			ListView_RedrawItems(m_hwndArtists, prev_list_pos, prev_list_pos);
		}

		_idtracer_p->artist_check(m_find_release_artist->id, 0);
	}
}

void CArtistList::set_artists(bool bexact_matches, const Artist_ptr p_get_artist, const pfc::array_t<Artist_ptr>& p_artist_exact_matches,
	const pfc::array_t<Artist_ptr>& p_artist_other_matches) {

	//0 exact, 1 single, 3 exact+other

	if (p_get_artist) {
		switch_find_releases(1);
	}
	else {

		m_artist_exact_matches = p_artist_exact_matches;
		m_artist_other_matches = p_artist_other_matches;
		switch_exact_matches(bexact_matches);
	}
}

void CArtistList::switch_exact_matches(bool exact_matches) {

	if (exact_matches) {
		switch_find_releases(0);
	}
	else {
		switch_find_releases(3);
	}
}

void CArtistList::switch_find_releases(size_t op) {

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
		m_find_release_artists = m_artist_exact_matches;
		m_find_release_artists.append(m_artist_other_matches);
		break;
	}

	//reset ndx artist id (ref. artist search role)

	for (size_t i = 0; i < m_find_release_artists.get_count(); i++) {
		m_find_release_artists[i]->search_role_list_pos = get_next_role_pos();
	}
	ListView_SetItemCountEx(m_hwndArtists, m_find_release_artists.get_count(), 0);
	if (get_size())
	{
		ListView_SetItemState(m_hwndArtists, 0, 0, 0x000F);
		ListView_SetItemState(m_hwndArtists, 0, LVIS_FOCUSED | LVIS_SELECTED, 0x000F);
	}

	m_lost_selection_id = pfc_infinite;
}

void CArtistList::set_image_list() {

	m_hImageList.Create(IDB_BITMAP_TRACER, 16, ILC_COLOR4, RGB(255, 255, 255));
	ListView_SetImageList(m_hwndArtists, m_hImageList, LVSIL_SMALL);
}

bool CArtistList::OnDisplayCellImage(int item, int subitem, int& result) {

	int img_ndx = I_IMAGENONE;
	bool is_traced = !_idtracer_p->is_multi_artist() && atoi(m_find_release_artists[item]->id) == _idtracer_p->get_artist_id();
	if (is_traced) {
		img_ndx = TRACER_IMG_NDX;
	}
	else {
		size_t artist_pos_showing = m_dlg->get_tree_artist_list_pos();
		size_t item_rol_pos = m_find_release_artists[item]->search_role_list_pos;
		if (item_rol_pos == artist_pos_showing) {
			img_ndx = SHOWING_IMAGE_NDX;
		}
	}

	result = img_ndx;
	return true;
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

	if (m_find_release_artists.get_count() > 0)
		return m_find_release_artists[list_index];

	return nullptr;
}

Artist_ptr CArtistList::get_selected_artist() {

	size_t pos = ListView_GetSingleSelection(m_hwndArtists);
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


//

// on get info, list changing, list changed

//

LRESULT CArtistList::OnGetInfo(WORD /*wNotifyCode*/, LPNMHDR hdr, BOOL& /*bHandled*/) {

	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)hdr;
	LV_DISPINFO* plvdi = (LV_DISPINFO*)hdr;

	bool req = m_find_release_artists.get_count() > plvdi->item.iItem;

	if (!req) {

		return FALSE;
	}

	LV_ITEM* pItem = &(plvdi)->item;

	if (pItem->mask & LVIF_TEXT)
	{
		if (pItem->iSubItem == 0) {

			pfc::string8 nodetext(m_find_release_artists[pItem->iItem]->name);

			if (OnDisplayCellString(pItem->iItem, 0, nodetext)) {

				TCHAR outBuffer[MAX_PATH + 1] = {};

				pfc::stringcvt::convert_utf8_to_wide(outBuffer, MAX_PATH,
					nodetext.get_ptr(), nodetext.get_length());

				_tcscpy_s(pItem->pszText, pItem->cchTextMax, const_cast<TCHAR*>(outBuffer));
			}
			else
				pItem->pszText = LPSTR_TEXTCALLBACK;
		}
	}

	if (pItem->mask & LVIF_IMAGE)
	{
		if (pItem->iSubItem == 0) {

			int img_ndx;

			// calc tracer image

			if (OnDisplayCellImage(pItem->iItem, 0, img_ndx)) {

				pItem->iImage = img_ndx;				
			}
			else {

				pItem->iImage = I_IMAGECALLBACK;
			}
		}
	}

	//

	// lost selection

	//

	if ((pNMListView->uChanged & LVIF_STATE) && (pNMListView->uOldState & LVNI_SELECTED) && !(pNMListView->uNewState & LVNI_SELECTED)) {

		// lost selection

		int pos = pNMListView->iItem;

		if (pos == -1 || pos >= m_find_release_artists.get_count()) {

			m_lost_selection_id = pfc_infinite;
			return 0;
		}

		//todo: mutex unselect
		if (m_find_release_artists.get_count()) {

			m_lost_selection_id = atoi(m_find_release_artists[pNMListView->iItem]->id);
		}
		else {

			m_lost_selection_id = atoi(m_find_release_artist->id);
		}
	}


	//

	// focus: convey profile

	//

	if ((pNMListView->uChanged & LVIF_STATE) && (!(pNMListView->uOldState & LVNI_FOCUSED)) && (pNMListView->uNewState & LVNI_FOCUSED)) {

		// get focus, for example pressing search button an not selecting any items

		if (pNMListView->iItem == -1) return 0;

		// convey artist profile

		m_dlg->convey_artist_list_selection(updRelSrc::ArtistProfile);
	}

	return TRUE;
}

//

// state changed

//

LRESULT CArtistList::OnListSelChanged(int i, LPNMHDR hdr, BOOL& bHandled) {

	bHandled = true;

	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)hdr;

	if ((pNMListView->uChanged & LVIF_STATE) && (pNMListView->uNewState & LVNI_SELECTED) && !((pNMListView->uOldState & LVNI_SELECTED) == LVNI_SELECTED)) {

		// becomes selected

		if (pNMListView->iItem == -1) return 0;

		size_t pos = pNMListView->iItem;

		// 

		// ** artist selected, collect and display

		// convey

		m_dlg->convey_artist_list_selection(updRelSrc::ArtistProfile);

	}

	return FALSE;
}

LRESULT CArtistList::OnListSelChanging(int i, LPNMHDR hdr, BOOL& bHandled) {

	//bHandled = true;
	//NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)hdr;
	//debug

	return FALSE;
}

//

// double click

//

LRESULT CArtistList::OnListDoubleClick(int i, LPNMHDR hdr, BOOL& bHandled) {

	bHandled = TRUE;

	if (hdr->code == NM_DBLCLK) {

		NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)hdr;

		if (pNMListView->iItem == -1) return 0;

		// artist default action (ArtistList)

		Default_Artist_Action();

		//
	}
	return 0;
}

//

// on context menu

//

LRESULT CArtistList::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {

	bHandled = FALSE;

	HWND hwndCtrl = (HWND)wParam;

	if (hwndCtrl != m_hwndArtists) return FALSE;

	CPoint screen_position, p;
	size_t list_index = ListView_GetFirstSelection(m_hwndArtists);

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

LRESULT CArtistList::OnRClickRelease(int, LPNMHDR hdr, BOOL&) {

    //debug
	return 0;
}

//

//default action for artist list item

//

void CArtistList::Default_Artist_Action() {

	size_t pos = ListView_GetFirstSelection(m_hwndArtists);

	if (pos == ~0) return;
	
	Artist_ptr artist = m_find_release_artists[pos];

	if (atoi(artist->id) != _idtracer_p->get_artist_id()) {

		set_artist_tracer_ovr(atoi(artist->id));
	}
	
	// convery
	
	m_dlg->convey_artist_list_selection(updRelSrc::ArtistList);

	//
}

//

// open artist profile panel

//

void CArtistList::ShowArtistProfile() {
	
	//serves dlg bind panel checkbox

	size_t pos = ListView_GetFirstSelection(m_hwndArtists);
	open_artist_profile(pos);
}


void CArtistList::open_artist_profile(size_t list_index) {

	if (!g_discogs->find_release_artist_dialog) {
		g_discogs->find_release_artist_dialog = fb2k::newDialog<CFindReleaseArtistDialog>(core_api::get_main_window(), SW_SHOW);
	}

	if (list_index != pfc_infinite) {

		Artist_ptr artist = get_artist_inlist(list_index);

		if (artist) {
			g_discogs->find_release_artist_dialog->UpdateProfile(artist);
		}		
	}

	::SetFocus(g_discogs->find_release_artist_dialog->m_hWnd);
}


void CArtistList::context_menu(size_t list_index, POINT screen_pos) {
	
	bool empty_sel = (list_index == -1);

	Artist_ptr artist;
	bool isArtistOffline = false;

	if (!empty_sel) {

		artist = get_selected_artist();
		isArtistOffline = artist->loaded_releases_offline;
	}

	pfc::string8 sourcepage = "View artist page";
	pfc::string8 copyrow = "Copy to clipboard";

	try {

		enum { ID_VIEW_PAGE = 1, ID_CLP_COPY_ROW, ID_ARTIST_DEL_CACHE, ID_ARTIST_PROFILE };
		HMENU menu = CreatePopupMenu();

		if (!empty_sel) {
		
			uAppendMenu(menu, MF_STRING, ID_VIEW_PAGE, sourcepage);
			uAppendMenu(menu, MF_STRING, ID_ARTIST_PROFILE, "Open profile panel");
			uAppendMenu(menu, MF_SEPARATOR, 0, 0);
			uAppendMenu(menu, MF_STRING, ID_CLP_COPY_ROW, copyrow);
			if (isArtistOffline) {
				uAppendMenu(menu, MF_SEPARATOR, 0, 0);
				uAppendMenu(menu, MF_STRING, ID_ARTIST_DEL_CACHE, "Clear artist cache");
			}
		}
		
		int cmd = TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, screen_pos.x, screen_pos.y, 0, m_hwndArtists, 0);
		DestroyMenu(menu);

		switch (cmd)
		{
		case ID_VIEW_PAGE:
		{
			pfc::string8 url;
			url << "https://www.discogs.com/artist/" << artist->id;			
			display_url(url);
			break;
		}
		case ID_CLP_COPY_ROW:
		{
			pfc::string8 utf_buffer;
			TCHAR outBuffer[MAX_PATH + 1] = {};

			if (list_index != -1) {
				LVITEM lvi;
				TCHAR outBuffer[MAX_PATH + 1] = {};
				lvi.pszText = outBuffer;
				lvi.cchTextMax = MAX_PATH;
				lvi.mask = LVIF_TEXT;
				lvi.stateMask = (UINT)-1;
				lvi.iItem = list_index;
				lvi.iSubItem = 0;

				BOOL result = ListView_GetItem(m_hwndArtists, &lvi);
				utf_buffer << pfc::stringcvt::string_utf8_from_os(outBuffer).get_ptr();
			}

			if (utf_buffer.get_length()) {

				ClipboardHelper::OpenScope scope; scope.Open(core_api::get_main_window());
				ClipboardHelper::SetString(utf_buffer);

			}
			break;
		}
		case ID_ARTIST_DEL_CACHE:
		{
			pfc::string8 utf_buffer;
			TCHAR outBuffer[MAX_PATH + 1] = {};

			size_t cItems = ListView_GetItemCount(m_hwndArtists);
			for (size_t walk_item = 0; walk_item < cItems; walk_item++) {
				if (LVIS_SELECTED == ListView_GetItemState(m_hwndArtists, walk_item, LVIS_SELECTED)) {
					//delete cache
					discogs_interface->delete_artist_cache(artist->id);
					artist->loaded_releases_offline = false;
				}
			}

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
