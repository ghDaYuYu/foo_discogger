#include  "stdafx.h"

#include "find_release_tree.h"

LRESULT CFindReleaseTree::OnTreeGetInfo(WORD /*wNotifyCode*/, LPNMHDR hdr, BOOL& /*bHandled*/) {
	if (m_vec_ptr->size() == 0 || !m_dispinfo_enabled)
		return FALSE;

	NMTVDISPINFO* pDispInfo = reinterpret_cast<NMTVDISPINFO*>(hdr);
	TVITEMW* pItem = &(pDispInfo)->item;
	HTREEITEM* hItem = &(pItem->hItem);

	int lparam = pItem->lParam;
	mounted_param myparam(lparam);

	bool db_isready = DBFlags(CONF.db_dc_flag).IsReady();
	pfc::string8 release_id;
	size_t ctracks = 0;

	cache_iterator_t cache_it = m_cache_ptr->cache.find(lparam);

	if (cache_it != m_cache_ptr->cache.end()) {

		row_col_data rcdata = cache_it->second.first;
		release_id << pfc::toString(rcdata.id).c_str();
		Release_ptr release = m_discogs_interface->get_release(release_id, false);

		size_t cdiscs = release->discs.get_count();
		for (size_t walk = 0; walk < cdiscs; walk++) {
			ctracks += release->discs[walk]->tracks.size();
		}
		if (!ctracks && db_isready)
			ctracks = release->db_total_tracks;
	}

	if (pItem->mask & TVIF_CHILDREN)
	{
		if (myparam.is_master()) {
			pItem->cChildren = I_CHILDRENCALLBACK;
		}
		else {
			pItem->cChildren = 0;
		}
	}

	if (pItem->mask & TVIF_TEXT)
	{
        //masters and releases
        TCHAR outBuffer[MAX_PATH + 1] = {};
        auto cit = m_cache_ptr->cache.find(lparam);
        auto row_data = cit->second.first.col_data_list.begin();
        pfc::string8 nodetext(row_data->second);
        if (ctracks) nodetext << " [" << pfc::toString(ctracks).c_str()  << "]";
        pfc::stringcvt::convert_utf8_to_wide(outBuffer, MAX_PATH,
            nodetext.get_ptr(), nodetext.get_length());
        _tcscpy_s(pItem->pszText, pItem->cchTextMax, const_cast<TCHAR*>(outBuffer));
		return FALSE;
	}
	return FALSE;
}

LRESULT CFindReleaseTree::OnReleaseTreeExpanding(int, LPNMHDR hdr, BOOL&) {

	NMTREEVIEW* pNMTreeView = (NMTREEVIEW*)hdr;
	TVITEMW* pItemExpanding = &(pNMTreeView)->itemNew;
	HTREEITEM* hItemExpanding = &(pNMTreeView->itemNew.hItem);

	if ((pNMTreeView->action & TVE_EXPAND) != TVE_EXPAND)
	{
		vec_iterator_t master_it;
		find_vec_by_lparam(*m_vec_ptr, pItemExpanding->lParam, master_it);
		if (master_it != m_vec_ptr->end()) {
			cache_iterator_t cache_it = master_it->first;
			int ival = 0;
			m_cache_ptr->SetCacheFlag(cache_it, NodeFlag::expanded, &ival);
		}

		return FALSE;
	}


	if (pItemExpanding->mask & TVIF_CHILDREN)
	{
		mounted_param myparam(pItemExpanding->lParam);

		//can not rely on cChildren value here, need to register them ourselves
		//(pItemExpanding->cChildren == I_CHILDRENCALLBACK, ...)
		// int tc = TreeView_GetCount(release_tree);

		auto& cache_parent = m_cache_ptr->cache.find(pItemExpanding->lParam);

		/*
		or

		auto node_it = std::find_if(m_vec_lv_items.begin(), m_vec_lv_items.end(),
				[pItemExpanding](const std::pair<cache_iterator_t, flagreg> e) {
					return e.first->first == pItemExpanding->lParam; }
			);
		*/

		bool children_done;
		m_cache_ptr->GetCacheFlag(cache_parent, NodeFlag::added, &children_done);

		bool tree_children_done;
		bool tree_children_done_ver = m_cache_ptr->GetCacheFlag(cache_parent, NodeFlag::tree_created, &tree_children_done);

		if (children_done) {
			if (!tree_children_done_ver) {
				if (myparam.is_master()) {
					t_size try_release_ndx = 0;
					//std::vector<std::pair<cache_iterator_t, TVINSERTSTRUCT>> vchildrenpair = {};
					std::vector<TVINSERTSTRUCT> vchildren = {};
					//seq access
					//auto walk_children = cache_parent++;
					//random access
					int walk_param = mounted_param(myparam.master_ndx, try_release_ndx, true, true).lparam();
					auto walk_children = m_cache_ptr->cache.find(walk_param);
					int newindex = 0;
					while (walk_children != m_cache_ptr->cache.end()) {

						bool ival = false;
						m_cache_ptr->GetCacheFlag(walk_children, NodeFlag::spawn, &ival);
						bool bfilter;
						bool bfilter_ver = m_cache_ptr->GetCacheFlag(walk_children, NodeFlag::filterok, &bfilter);

						pfc::string8 filter;
						uGetWindowText(p_parent_filter_edit, filter);

						//..

						if (m_vec_ptr->size() == 0 || (m_vec_ptr->size() && bfilter_ver)) {

							//if (/*ival == 1 &&*/ !(m_results_filter.get_length() && !bfilter_ver)) {

							vchildren.resize(newindex + 1);
							//vchildrenpair.resize(newindex + 1);
							int lparam = walk_children->first;
							mounted_param mp(lparam);


							////check memory cache children
							//pfc::string8 release_id;
							//size_t ctracks = 0;
							//vec_iterator_t find_it;
							//find_vec_by_lparam(*m_vec_ptr, lparam, find_it);
							//if (find_it != m_vec_ptr->end()) {
							//	cache_iterator_t cache_it = find_it->first;
							//	row_col_data rcdata = cache_it->second.first;
							//	release_id = pfc::toString(rcdata.id).c_str();
							//	Release_ptr release = m_discogs_interface->get_release(release_id, false);
							//	if (release->discs.get_count() > 0) {
							//		int debug = 0;
							//	}
							//	size_t cdiscs = release->discs.get_count();
							//	for (size_t walk = 0; walk < cdiscs; walk++) {
							//		//string_encoded_array disctracks = release->discs[walk]->get_total_tracks();
							//		ctracks += release->discs[walk]->tracks.size();
							//	}
							//}
							//..

							TV_INSERTSTRUCT* tvis = (TV_INSERTSTRUCT*)&vchildren.at(newindex);
							//TV_INSERTSTRUCT* tvis = (TV_INSERTSTRUCT*)&vchildrenpair.at(newindex).second;
							//vchildrenpair.at(newindex).first = walk_children;

							//memmove(&(tvis->item), &tvi, sizeof(TV_ITEM));
							tvis->hParent = (HTREEITEM)pItemExpanding->hItem;
							tvis->hInsertAfter = TVI_LAST;
							TV_ITEM tvi;
							memset(&tvi, 0, sizeof(tvi));
							tvis->item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM | TVIF_STATE; //TVIF_IMAGE | TVIF_TEXT | TVIF_SELECTEDIMAGE;
							tvis->item.pszText = LPSTR_TEXTCALLBACK;
							tvis->item.iImage = I_IMAGECALLBACK;
							tvis->item.iSelectedImage = I_IMAGECALLBACK;
							tvis->item.cChildren = 0; // ctracks; //0;
							tvis->item.lParam = walk_children->first;
							int walk_id = walk_children->second.first.id;

							if (walk_id == m_idtracer.release_id) {
								tvis->item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM | TVIF_STATE; //TVIF_IMAGE | TVIF_TEXT | TVIF_SELECTEDIMAGE;
								tvis->item.state = TVIS_BOLD;
								tvis->item.stateMask = TVIS_BOLD;
							}
							else {
								tvis->item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM; //TVIF_IMAGE | TVIF_TEXT | TVIF_SELECTEDIMAGE;
							}

							newindex++;
						}
						walk_param = mounted_param(myparam.master_ndx, ++try_release_ndx, true, true).lparam();
						walk_children = m_cache_ptr->cache.find(walk_param);
					}

					for (const auto& tvchild : vchildren) {
						HTREEITEM node = TreeView_InsertItem(p_treeview, &tvchild);
					}
					/*for (const auto& tvchild : vchildrenpair) {
						HTREEITEM node = TreeView_InsertItem(p_treeview, &tvchild.second);
						vec_iterator_t find_it;
						find_vec_by_lparam(*m_vec_ptr, tvchild.first->first, find_it);
						find_it->second = node;
					}*/

					int ival = 1; //node added, set flag tree_created 
					m_cache_ptr->SetCacheFlag(pItemExpanding->lParam, NodeFlag::tree_created, ival);

					//bool bval = false;
					//m_cache_ptr->GetCacheFlag(pItemExpanding->lParam, NodeFlag::expanded, bval);
					//int kk = cache_parent->first;

					int ivalex = 1;
					m_cache_ptr->SetCacheFlag(pItemExpanding->lParam, NodeFlag::expanded, &ivalex);
					//children #
					pItemExpanding->cChildren = vchildren.size();
					//pItemExpanding->cChildren = vchildrenpair.size();
				}
				else {
					//could also test it with get_count()? 1 : 0
					//m_find_release_artist_p->releases.get_count() ? I_CHILDRENCALLBACK : 0;
					//pItemExpanding->cChildren = m_find_release_artist_p->releases.get_count() ? I_CHILDRENCALLBACK : 0;





					////check memory cache children
					//pfc::string8 release_id;
					//size_t ctracks = 0;
					//vec_iterator_t find_it;
					//find_vec_by_lparam(*m_vec_ptr, pItemExpanding->lParam, find_it);
					//if (find_it != m_vec_ptr->end()) {
					//	cache_iterator_t cache_it = find_it->first;
					//	row_col_data rcdata = cache_it->second.first;
					//	release_id << rcdata.id;
					//	Release_ptr release = m_discogs_interface->get_release(release_id, false);
					//	if (release->discs.get_count() > 0) {
					//		int debug = 0;
					//	}
					//	size_t cdiscs = release->discs.get_count();
					//	for (size_t walk = 0; walk < cdiscs; walk++) {
					//		//string_encoded_array disctracks = release->discs[walk]->get_total_tracks();
					//		ctracks += release->discs[walk]->tracks.size();
					//	}
					//}

					// EXPANDING NON MASTER (insert tracks)

					////check memory cache children
					//pfc::string8 release_id;
					//size_t ctracks = 0;
					//vec_iterator_t find_it;
					//find_vec_by_lparam(*m_vec_ptr, pItemExpanding->lParam, find_it);
					//if (find_it != m_vec_ptr->end()) {
					//	cache_iterator_t cache_it = find_it->first;
					//	row_col_data rcdata = cache_it->second.first;
					//	release_id = pfc::toString(rcdata.id).c_str();
					//	Release_ptr release = m_discogs_interface->get_release(release_id, false);
					//	if (release->discs.get_count() > 0) {
					//		int debug = 0;
					//	}
					//	size_t cdiscs = release->discs.get_count();
					//	for (size_t walk = 0; walk < cdiscs; walk++) {
					//		//string_encoded_array disctracks = release->discs[walk]->get_total_tracks();
					//		ctracks += release->discs[walk]->tracks.size();
					//	}
					//}



					//t_size try_release_ndx = 0;
					//std::vector<TVINSERTSTRUCT> vchildren = {};

					//vchildren.resize(ctracks);

					//for (size_t walk_tracks = 0; walk_tracks < ctracks; walk_tracks++) {

					//	TV_INSERTSTRUCT* tvis = (TV_INSERTSTRUCT*)&vchildren.at(walk_tracks);
					//	//memmove(&(tvis->item), &tvi, sizeof(TV_ITEM));
					//	tvis->hParent = (HTREEITEM)pItemExpanding->hItem;
					//	tvis->hInsertAfter = TVI_LAST;
					//	TV_ITEM tvi;
					//	memset(&tvi, 0, sizeof(tvi));
					//	tvis->item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM | TVIF_STATE; //TVIF_IMAGE | TVIF_TEXT | TVIF_SELECTEDIMAGE;


					//	//TCHAR outBuffer[MAX_PATH + 1] = {};
					//	////cache_iterator_t cit = m_cache_ptr->cache.find(lparam);
					//	//pfc::string8 pfcbuffer; pfcbuffer << walk_tracks;
					//	//pfc::stringcvt::convert_utf8_to_wide(outBuffer, MAX_PATH,
					//	//	pfcbuffer.get_ptr(), pfcbuffer.get_length());
					//	//_tcscpy_s(tvis->item.pszText, MAX_PATH, const_cast<TCHAR*>(outBuffer));
					//	//std::wstring kk = std::to_wstring((int)walk_tracks);
					//	tvis->item.pszText = LPSTR_TEXTCALLBACK;

					//	tvis->item.iImage = I_IMAGECALLBACK;
					//	tvis->item.iSelectedImage = I_IMAGECALLBACK;
					//	tvis->item.cChildren = 0;
					//	mounted_param tmp_param = mounted_param(pfc_infinite, 900 + walk_tracks, false, true);
					//	tvis->item.lParam = tmp_param.lparam();
					//	tvis->item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM; //TVIF_IMAGE | TVIF_TEXT | TVIF_SELECTEDIMAGE;

					//}

					//for (const auto& tvchild : vchildren) {
					//	TreeView_InsertItem(p_treeview, &tvchild);
					//}

					//vec_iterator_t master_it;
					//find_vec_by_lparam(*m_vec_ptr, pItemExpanding->lParam, master_it);
					//cache_iterator_t cache_it = master_it->first;
					//int ival = 1;
					//m_cache_ptr->SetCacheFlag(cache_it, NodeFlag::added, &ival);

					//int createdval = 1; //node added, set flag tree_created 
					//m_cache_ptr->SetCacheFlag(pItemExpanding->lParam, NodeFlag::tree_created, createdval);




					// END EXPANDING NON MASTER
					size_t ctracks = get_mem_cache_children_count(pItemExpanding->lParam);
					pItemExpanding->cChildren = ctracks; // ? ctracks /*I_CHILDRENCALLBACK*/ : 0;

					//..



				}
			}
			else {
				//children already produced
				int ival = 1;
				m_cache_ptr->SetCacheFlag(cache_parent, NodeFlag::expanded, &ival);

				if (myparam.is_nmrelease()) {
				
					size_t ctracks = get_mem_cache_children_count(pItemExpanding->lParam);
					pItemExpanding->cChildren = ctracks; // ? ctracks /*I_CHILDRENCALLBACK*/ : 0;

				}

				return FALSE;
			}
		}
		else {
			
			if (myparam.is_master()) {

				//children not done -> spawn
				//store clicked hItem to expand after spawn
				m_hhit = pItemExpanding->hItem;
				//temp fix listview compatibility
				vec_iterator_t master_it;
				find_vec_by_lparam(*m_vec_ptr, pItemExpanding->lParam, master_it);

				if (master_it == m_vec_ptr->end())
					return TRUE;

				vec_iterator_t start = m_vec_ptr->begin();

				int selection_index = std::distance(start, master_it);

				if (master_it != m_vec_ptr->end()) {
					int ival = 1;
					m_cache_ptr->SetCacheFlag(master_it->first, NodeFlag::expanded, &ival);

					if (ival) {
						int ival = 1;
						m_cache_ptr->SetCacheFlag(master_it->first, NodeFlag::added, &ival);
					}
				}
				stdf_on_release_selected_notifier(pItemExpanding->lParam);

			
			}
			//else {
			//
			//	if (myparam.is_nmrelease()) {
			//	
			//		// EXPANDING NON MASTER (insert tracks)

			//		//check cache children
			//		size_t ctracks = get_mem_cache_children_count(pItemExpanding->lParam);


			//		t_size try_release_ndx = 0;
			//		std::vector<TVINSERTSTRUCT> vchildren = {};

			//		vchildren.resize(ctracks);

			//		for (size_t walk_tracks = 0; walk_tracks < ctracks; walk_tracks++) {

			//			TV_INSERTSTRUCT* tvis = (TV_INSERTSTRUCT*)&vchildren.at(walk_tracks);
			//			//memmove(&(tvis->item), &tvi, sizeof(TV_ITEM));
			//			tvis->hParent = (HTREEITEM)pItemExpanding->hItem;
			//			tvis->hInsertAfter = TVI_LAST;
			//			TV_ITEM tvi;
			//			memset(&tvi, 0, sizeof(tvi));
			//			tvis->item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM | TVIF_STATE; //TVIF_IMAGE | TVIF_TEXT | TVIF_SELECTEDIMAGE;

			//			tvis->item.pszText = LPSTR_TEXTCALLBACK;

			//			tvis->item.iImage = I_IMAGECALLBACK;
			//			tvis->item.iSelectedImage = I_IMAGECALLBACK;
			//			tvis->item.cChildren = 0;
			//			mounted_param tmp_param = mounted_param(pfc_infinite, 900 + walk_tracks, false, true);
			//			tvis->item.lParam = tmp_param.lparam();
			//			tvis->item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM; //TVIF_IMAGE | TVIF_TEXT | TVIF_SELECTEDIMAGE;

			//		}

			//		for (const auto& tvchild : vchildren) {
			//			TreeView_InsertItem(p_treeview, &tvchild);
			//		}

			//		vec_iterator_t master_it;
			//		find_vec_by_lparam(*m_vec_ptr, pItemExpanding->lParam, master_it);
			//		cache_iterator_t cache_it = master_it->first;
			//		int ival = 1;
			//		m_cache_ptr->SetCacheFlag(cache_it, NodeFlag::added, &ival);

			//		int createdval = 1; //node added, set flag tree_created 
			//		m_cache_ptr->SetCacheFlag(pItemExpanding->lParam, NodeFlag::tree_created, createdval);

			//		// END EXPANDING NON MASTER

			//	}
			//}			
		}
	}
	return FALSE;
}

t_size CFindReleaseTree::get_mem_cache_children_count(LPARAM lParam) {
	
	pfc::string8 release_id;
	size_t ctracks = 0;
	vec_iterator_t find_it;
	find_vec_by_lparam(*m_vec_ptr, lParam, find_it);
	if (find_it != m_vec_ptr->end()) {
		cache_iterator_t cache_it = find_it->first;
		row_col_data rcdata = cache_it->second.first;
		release_id << pfc::toString(rcdata.id).c_str();
		Release_ptr release = m_discogs_interface->get_release(release_id, false);
		if (release->discs.get_count() > 0) {
			int debug = 0;
		}
		size_t cdiscs = release->discs.get_count();
		for (size_t walk = 0; walk < cdiscs; walk++) {
			//string_encoded_array disctracks = release->discs[walk]->get_total_tracks();
			ctracks += release->discs[walk]->tracks.size();
		}
	}
	return ctracks;
}

LRESULT CFindReleaseTree::OnReleaseTreeSelChanging(int, LPNMHDR hdr, BOOL& bHandled) {

	return FALSE;
}

LRESULT CFindReleaseTree::OnReleaseTreeSelChanged(int, LPNMHDR hdr, BOOL& bHandled) {
	LPNMTREEVIEW pnmtv = (LPNMTREEVIEW)hdr;
	//NMTREEVIEW* pNMTreeView = (NMTREEVIEW*)hdr;
	//TVITEMW* pItemExpanding = &(pNMTreeView)->itemNew;
	//HTREEITEM* hItemExpanding = &(pNMTreeView->itemNew.hItem);

	TVITEM pItemMaster = { 0 };
	pItemMaster.mask = TVIF_PARAM;
	pItemMaster.hItem = pnmtv->itemNew.hItem;
	TreeView_GetItem(p_treeview, &pItemMaster);
	mounted_param myparam = mounted_param(pItemMaster.lParam);
	cache_iterator_t it = m_cache_ptr->cache.find(pItemMaster.lParam);
	if (it != m_cache_ptr->cache.end()) {

		pfc::string8 strId;
		if (myparam.is_master()) {
			strId = m_find_release_artist_p->master_releases[myparam.master_ndx]->main_release_id;
		}
		else {
			int id = it->second.first.id;
			strId = pfc::toString(id).c_str();
		}

		pfc::string8 release_url = strId;
		uSetWindowText(p_parent_url_edit, release_url);
	}
	bHandled = FALSE;
	return 0;
}