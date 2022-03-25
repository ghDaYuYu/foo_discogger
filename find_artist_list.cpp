#include "stdafx.h"

#include "find_release_dialog.h"
#include "find_artist_list.h"


void CFindArtistList::on_get_artist_done(updRelSrc updsrc, Artist_ptr& artist) {

	if (updsrc == updRelSrc::Undef) {

		//..

	}
	else if (updsrc == updRelSrc::ArtistProfile) {

		//update profile

		{
			std::lock_guard<std::mutex> guard(m_dlg->m_loading_selection_readwrite_mutex);

			if (m_dlg->m_loading_selection_id == atoi(artist->id)) {

				if (g_discogs->find_release_artist_dialog)
					g_discogs->find_release_artist_dialog->UpdateProfile(artist);


				m_dlg->m_loading_selection_id = pfc_infinite;
			}
		}

		return;
	}

	fill_artist_list(false /*not from dlg*/, true, updsrc);

	set_find_release_artist(artist);
}

void CFindArtistList::fill_artist_list(bool dlgexcact, bool force_exact, updRelSrc updsrc) {

	if (updsrc == updRelSrc::ArtistSearch) {

		// (search button callback)

		pfc::array_t<Artist_ptr> artist_other_matches = m_artist_other_matches;
		_idtracer_p->artist_reset();

		// select 

		size_t  artist_pos = _idtracer_p->artist_pos != ~0 ? _idtracer_p->artist_pos : 0;

		if (get_size()) {

			ListView_SetItemState(m_hwndArtists, artist_pos, LVIS_SELECTED, LVIS_SELECTED);
		}
	}

	else if (updsrc == updRelSrc::ArtistListCheckExact) {

		// (exact matches checkbox))
		
		set_exact_matches(dlgexcact);
	}

	else if (updsrc == updRelSrc::Undef) {

		//  on init dialog

		if (!get_size()) {

			set_artists(true, m_find_release_artist, m_artist_exact_matches, m_artist_other_matches);
		}
	}
	else if (updsrc == updRelSrc::ArtistList || updsrc != updRelSrc::ArtistProfile) {

		// get artist callback:
		// on change list selection with profile unavailable 
		// on doubled click, on enter (ArtistList)

		t_size pos = ListView_GetFirstSelection(m_hwndArtists);		
		m_find_release_artist = Get_Artists()[pos];

		_idtracer_p->artist_check(m_find_release_artist->id, 0);
	}
}

bool CFindArtistList::OnDisplayCellImage(int item, int subitem, int & result) {

	bool is_traced = (atoi(m_find_release_artists[item]->id) == _idtracer_p->get_artist_id());
	result = is_traced ? 0 : -1;
	return true;
}


bool CFindArtistList::OnDisplayCellString(int item, int subitem, pfc::string8 & result) {

	result = m_find_release_artists[item]->name;
	return true;
}

pfc::string8 CFindArtistList::get_artist_id(size_t pos) {

	if (get_size() > pos)
		return m_find_release_artists[pos]->id;
	
	if (m_find_release_artist)
		return m_find_release_artist->id;

	return "";
}

size_t CFindArtistList::get_size() {
	return m_find_release_artists.get_count() ?
		m_find_release_artists.get_count() : 0;
}

size_t CFindArtistList::get_artist_pos(size_t szartist_id) {

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

void CFindArtistList::set_artists(bool bexact_matches, const Artist_ptr p_get_artist, const pfc::array_t<Artist_ptr>& p_artist_exact_matches,
	const pfc::array_t<Artist_ptr>& p_artist_other_matches) {
    
    //0 exact, 1 single, 3 exact+other

	if (p_get_artist) {
		switch_find_releases(1);
	}
	else {

		m_artist_exact_matches = p_artist_exact_matches;
		m_artist_other_matches = p_artist_other_matches;
		switch_find_releases(bexact_matches ? 0 : 3);
	}
}

void CFindArtistList::set_exact_matches(bool exact_matches) {

	if (exact_matches) {
		switch_find_releases(0);
	}
	else {
		switch_find_releases(3); 
	}
}

void CFindArtistList::switch_find_releases(size_t op) {

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
		m_find_release_artists[i]->search_role_list_pos = i;
	}
	ListView_SetItemCountEx(m_hwndArtists, m_find_release_artists.get_count(), 0);
	
	m_lost_selection_id = pfc_infinite;
}

//

// on get info, list changing, list changed

//

LRESULT CFindArtistList::OnGetInfo(WORD /*wNotifyCode*/, LPNMHDR hdr, BOOL& /*bHandled*/) {

	LV_DISPINFO* plvdi = (LV_DISPINFO*)hdr;

	bool valid = m_find_release_artists.get_count() > plvdi->item.iItem;

	if (!valid) { return FALSE;	}
	
	LV_ITEM* pItem = &(plvdi)->item;

	if (pItem->mask & LVIF_TEXT)
	{
		if (pItem->iSubItem == 0) {

			pfc::string8 nodetext(m_find_release_artists[i]->name);

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
		
			// Request-Image
			int img_ndx = -1;
			if (OnDisplayCellImage(pItem->iItem, 0, img_ndx)) {

				pItem->iImage = img_ndx;
			}
			else
				pItem->iImage = I_IMAGECALLBACK;
		}
	}
	return FALSE;
}

//

// artist item status changed

//

LRESULT CFindArtistList::OnListSelChanged(int i, LPNMHDR hdr, BOOL& bHandled) {

	bHandled = true;

	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)hdr;

	if ((pNMListView->uNewState & LVNI_SELECTED) && !((pNMListView->uOldState & LVNI_SELECTED) == LVNI_SELECTED)) {

		// becomes selected

		if (pNMListView->iItem == -1) return 0;

		size_t pos = pNMListView->iItem;

		// 

		// ** artist SELECTED, may sp�wm. collects and displays

		//

		m_dlg->convey_artist_list_selection(updRelSrc::ArtistProfile);

		return 0;
	}

	if ((pNMListView->uChanged & LVIF_STATE) && (pNMListView->uOldState & LVNI_SELECTED) && !(pNMListView->uNewState & LVNI_SELECTED)) {

		// lost selection

		int pos = pNMListView->iItem;

		if (pos == -1 || pos >= m_find_release_artists.get_count()) {
			m_lost_selection_id = pfc_infinite;
			return 0;
		}

		//todo: mutex unselect
		if (m_find_release_artists.get_count())
			m_lost_selection_id = atoi(m_find_release_artists[pNMListView->iItem]->id);
		else
			m_lost_selection_id = atoi(m_find_release_artist->id);
	}
	return 0;
}

//

// double click

//

LRESULT CFindArtistList::OnListDoubleClick(int i, LPNMHDR hdr, BOOL& bHandled) {

	bHandled = TRUE;

	if (hdr->code == NM_DBLCLK) {

		NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)hdr;

		if (pNMListView->iItem == -1) return 0;

		size_t pos = pNMListView->iItem;

		Artist_ptr artist;

		if (m_find_release_artists.get_count() > 0) {
			artist = m_find_release_artists[pos];
		}
		else if (m_find_release_artist != NULL) {
			artist = m_find_release_artist;
		}

		updRelSrc updsrc = updRelSrc::ArtistList;

		//convey_artist_list_selection(updsrc);
		m_dlg->convey_artist_list_selection(updsrc);

	    return 0;
	}
	return 0;
}

//

// on context menu

//

LRESULT CFindArtistList::OnRClickRelease(int, LPNMHDR hdr, BOOL&) {

	LVITEM* hhit = NULL;

	bool isArtistOffline = false;

	t_size list_index = -1;
	int lparam = -1;
	mounted_param myparam(lparam);

	POINT screen_cursor_position;
	GetCursorPos(&screen_cursor_position);

	LPNMITEMACTIVATE nmView = (LPNMITEMACTIVATE)hdr;
	list_index = nmView->iItem;

	if (list_index == pfc_infinite)
		return FALSE;

	if (m_find_release_artists.get_count() > 0)
		isArtistOffline = m_find_release_artists[list_index]->loaded_releases_offline;
	else
		if (m_find_release_artist != NULL)
			isArtistOffline = m_find_release_artist->loaded_releases_offline;


	lparam = myparam.lparam();

	pfc::string8 sourcepage = "View artist page";
	pfc::string8 copytitle = "Copy title to clipboard";
	pfc::string8 copyrow = "Copy to clipboard";

	try {

		enum { ID_VIEW_PAGE = 1, ID_CLP_COPY_TITLE, ID_CLP_COPY_ROW, ID_ARTIST_DEL_CACHE, ID_ARTIST_PROFILE };
		HMENU menu = CreatePopupMenu();

		if (isArtist) {
			uAppendMenu(menu, MF_STRING, ID_VIEW_PAGE, sourcepage);
			uAppendMenu(menu, MF_STRING, ID_ARTIST_PROFILE, "Open profile panel");
			uAppendMenu(menu, MF_SEPARATOR, 0, 0);
			uAppendMenu(menu, MF_STRING, ID_CLP_COPY_ROW, copyrow);
			if (isArtistOffline) {
				uAppendMenu(menu, MF_SEPARATOR, 0, 0);
				uAppendMenu(menu, MF_STRING, ID_ARTIST_DEL_CACHE, "Clear artist cache");
			}
		}
		else {
			uAppendMenu(menu, MF_STRING, ID_VIEW_PAGE, sourcepage);
			uAppendMenu(menu, MF_SEPARATOR, 0, 0);
			uAppendMenu(menu, MF_STRING, ID_CLP_COPY_TITLE, copytitle);
			uAppendMenu(menu, MF_STRING, ID_CLP_COPY_ROW, copyrow);
		}

		int cmd = TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, screen_cursor_position.x, screen_cursor_position.y, 0, core_api::get_main_window(), 0);
		DestroyMenu(menu);
		switch (cmd)
		{
		case ID_VIEW_PAGE:
		{
			pfc::string8 url;

			if (m_find_release_artists.get_count() > 0)
				url << "https://www.discogs.com/artist/" << m_find_release_artists[list_index]->id;
			else
				if (m_find_release_artist != NULL)
					url << "https://www.discogs.com/artist/" << m_find_release_artist->id;

			if (url.get_length())
				display_url(url);

			return true;
		}
		case ID_CLP_COPY_TITLE:
		{
			pfc::string8 buffer;
			ClipboardHelper::OpenScope scope;
			scope.Open(core_api::get_main_window());
			ClipboardHelper::SetString(buffer);

			return true;
		}
		case ID_CLP_COPY_ROW:
		{
			pfc::string8 utf_buffer;
			TCHAR outBuffer[MAX_PATH + 1] = {};

			LPNMITEMACTIVATE nmView = (LPNMITEMACTIVATE)hdr;
			if (nmView->iItem != -1) {
				LVITEM lvi;
				TCHAR outBuffer[MAX_PATH + 1] = {};
				lvi.pszText = outBuffer;
				lvi.cchTextMax = MAX_PATH;
				lvi.mask = LVIF_TEXT;
				lvi.stateMask = (UINT)-1;
				lvi.iItem = nmView->iItem;
				lvi.iSubItem = 0;

				BOOL result = ListView_GetItem(m_hwndArtists, &lvi);
				utf_buffer << pfc::stringcvt::string_utf8_from_os(outBuffer).get_ptr();
			}

			if (utf_buffer.get_length()) {

				ClipboardHelper::OpenScope scope; scope.Open(core_api::get_main_window());
				ClipboardHelper::SetString(utf_buffer);

			}
			return true;
		}
		case ID_ARTIST_DEL_CACHE:
		{
			pfc::string8 utf_buffer;
			TCHAR outBuffer[MAX_PATH + 1] = {};

			//todo: rev getfirstselected and getnextselected

			size_t cItems = ListView_GetItemCount(m_hwndArtists);
			for (size_t walk_item = 0; walk_item < cItems; walk_item++) {
				if (LVIS_SELECTED == ListView_GetItemState(m_hwndArtists, walk_item, LVIS_SELECTED)) {
					Artist_ptr artist;
					if (m_find_release_artists.get_count() > 0)
					artist = m_find_release_artists[list_index];
					else
						if (m_find_release_artist != NULL)
							artist = m_find_release_artist;
					if (artist) {
						m_discogs_interface->delete_artist_cache(artist->id);
						artist->loaded_releases_offline = false;
					}
				}
			}

			return true;
		}
		case ID_ARTIST_PROFILE: {

			open_artist_profile(list_index);

			return true;
		}
		}
	}
	catch (...) {}

	return 0;
}

//

// misc

//

size_t CFindArtistList::test_hit_artist() {

	t_size pos = ListView_GetSingleSelection(m_hwndArtists);
	pos = ListView_GetFirstSelection(m_hwndArtists);

	return pos;

}

//default action for artist list item

void CFindArtistList::nVKReturn_Artists() {

	size_t pos = test_hit_artist();

	if (pos != ~0) {

		Artist_ptr artist;

		bool load_artist = false;
		pfc::string8 name, profile;

		if (m_find_release_artists.get_count() > 0) {
			artist = m_find_release_artists[pos];
		}
		else if (m_find_release_artist != NULL) {
			artist = m_find_release_artist;
		}

		if (artist) {
			name = artist->name;
			profile = artist->profile;
			load_artist = !artist->loaded;
		}

		set_artist_tracer_fr(atoi(artist->id));

		updRelSrc updsrc = updRelSrc::ArtistList;

		m_dlg->convey_artist_list_selection(updsrc);
	}
}

//

// open artist profile panel

//

void CFindArtistList::open_artist_profile(size_t list_index) {

	if (!g_discogs->find_release_artist_dialog) {
		g_discogs->find_release_artist_dialog = fb2k::newDialog<CFindReleaseArtistDialog>(core_api::get_main_window()/*, nullptr*/);
	}

	if (g_discogs->find_release_artist_dialog->m_hWnd != NULL) {

		Artist_ptr artist;

		if (m_find_release_artists.get_count() > 0) {
			artist = m_find_release_artists[list_index];
		}
		else if (m_find_release_artist != NULL) {
			artist = m_find_release_artist;
		}

		if (artist) {
			g_discogs->find_release_artist_dialog->UpdateProfile(artist);
		}
		else {
			int debug = 1;
		}

		::SetFocus(g_discogs->find_release_artist_dialog->m_hWnd);
	}
}