#include "stdafx.h"
#include "foo_discogs.h"
#include "multiformat.h"
#include "db_fetcher_component.h"

#include "find_release_utils.h"
#include "find_release_dialog.h"
#include "find_release_tree.h"

const size_t WINE_CHILDREN = 99999; //patch expanding virtual tree

const size_t TRACER_IMG_NDX = 3;
const size_t CHILDREN_DONE_IMG_NDX = 6;
const size_t TRACER_CHILDREN_DONE_IMG_NDX = 7;

//

pfc::string8 CFindReleaseTree::run_hook_columns(row_col_data& row_data, int item_data) {

	mounted_param myparam((LPARAM)item_data);
	pfc::string8 search_formatted;
	pfc::string8 id;

	row_data.col_data_list.clear();

	if (myparam.is_master()) {
		CONF.search_master_format_string->run_hook(location, m_info_p, m_hook.get(), search_formatted, nullptr);

		id << m_find_release_artist->master_releases[myparam.master_ndx]->id;
	}
	else {

		if (myparam.is_release()) {
			CONF.search_master_sub_format_string->run_hook(location, m_info_p, m_hook.get(), search_formatted, nullptr);
			pfc::string8 main_release_id = m_find_release_artist->master_releases[myparam.master_ndx]->main_release->id;
			pfc::string8 this_release_id = m_find_release_artist->master_releases[myparam.master_ndx]->sub_releases[myparam.release_ndx]->id;
			id << this_release_id;
		}
		else
		{
			CONF.search_release_format_string->run_hook(location, m_info_p, m_hook.get(), search_formatted, nullptr);
			id << m_find_release_artist->releases[myparam.release_ndx]->id;
		}
	}
	row_data.col_data_list.emplace_back(std::make_pair(0/*walk_cfg.icol*/, search_formatted));
	row_data.id = atoi(id);

	return search_formatted;
}

void CFindReleaseTree::rebuild_treeview() {

	TreeView_DeleteAllItems(m_hwndTreeView);

	std::shared_ptr<vec_t>& priv_vec = m_rt_cache.get_vec_ref();

	for (auto& walk : *priv_vec)
	{
		TVINSERTSTRUCT tvis = { 0 };

		tvis.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM | TVIF_STATE;
		tvis.item.pszText = LPSTR_TEXTCALLBACK;
		tvis.item.iSelectedImage = I_IMAGECALLBACK;

		tvis.item.cChildren = I_CHILDRENCALLBACK;
		tvis.item.lParam = walk.first->first;
		tvis.hParent = TVI_ROOT;
		tvis.hInsertAfter = TVI_LAST;

		mounted_param myparam(walk.first->first);
		bool bbold = (walk.first->second.first.id == _idtracer_p->master_id) && myparam.is_master(); // && (walk.first->second.first.id == _idtracer_p->master_id);
		bbold |= (walk.first->second.first.id == _idtracer_p->release_id) && myparam.is_nmrelease(); // && (walk.first->second.first.id == _idtracer_p->release_id);

		int img_ndx;
		OnTreeDisplayCellImage(myparam.master_ndx, myparam.release_ndx, walk.first->second.first.id/*cit->second.first.id*/, walk.first, img_ndx);


		if (bbold) {

			tvis.item.iImage = TRACER_IMG_NDX;
			tvis.item.iSelectedImage = TRACER_IMG_NDX;

			tvis.item.state = TVIS_BOLD;
			tvis.item.stateMask = TVIS_BOLD;
		}
		else {
			tvis.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM | TVIF_STATE;
			tvis.item.iImage = -1;
			tvis.item.iSelectedImage = -1;

		}
		HTREEITEM newnode = TreeView_InsertItem(m_hwndTreeView, &tvis);

		walk.second = newnode;
	}
}

void release_tree_cache::vec_PushBack(std::pair<cache_iterator_t, HTREEITEM> item) {
	//todo: logs & stats
	m_vec_ptr->push_back(item);
}

//note: called often
bool release_tree_cache::get_cached_find_release_node(int lparam, pfc::string8& item, row_col_data& row_data) {

	cache_t* cached = &m_cache_ptr->cache;
	auto it = cached->find(lparam);

	if (it != cached->end()) {
		item = it->second.first.col_data_list.begin()->second;
		row_data = it->second.first;
		return true;
	}
	return false;
}

bool check_match(pfc::string8 str, pfc::string8 filter, pfc::array_t<pfc::string> filter_words_lowercase) {
	bool bres = true;
	if (filter.get_length()) {
		pfc::string item_lower = pfc::string(str.get_ptr()).toLower();
		for (size_t j = 0; j < filter_words_lowercase.get_count(); j++) {
			bres = item_lower.contains(filter_words_lowercase[j]);
			if (!bres) {
				break;
			}
		}
	}
	return bres;
}

//

// release_tree_cache :: update releases

//

std::pair<rppair_t, rppair_t> release_tree_cache::update_releases(const pfc::string8& filter, updRelSrc updsrc, bool init_expand, id_tracer* tracer_p, bool brolemain_filter) {

	std::pair<size_t, size_t> vec_lv_reserve_slow;
	std::pair<size_t, size_t> vec_lv_reserve_fast;
	std::pair<size_t, size_t> master_rel_stat_slow;
	std::pair<size_t, size_t> master_rel_stat_fast;

	titleformat_hook_impl_multiformat_ptr hook = m_rt_manager->get_hook();

	bool delete_on_enter = true;

	bool rebuild_tree_cache = true;

	if (updsrc == updRelSrc::Filter || delete_on_enter) {
		tracer_p->release_reset();
	}

	pfc::array_t<pfc::string> lcf_words;
	bool filtered = filter.get_length() && tokenize_filter(filter, lcf_words);

	Artist_ptr find_release_artist = m_rt_manager->get_find_release_artist();

	if (delete_on_enter) {

		//CALC CACHE - FILTERED
		rppair_t r = init_filter(find_release_artist, filter, true, false, true,tracer_p);
		vec_lv_reserve_slow = r.first;
		master_rel_stat_slow = r.second;

		//CLEAR AND RESERVE NEW REF CACHE
		m_vec_ptr->clear();
		m_vec_ptr->reserve(vec_lv_reserve_slow.first + vec_lv_reserve_slow.second);

		if (updsrc != updRelSrc::Filter) {
			//DESTROY BULK CACHE
			if (m_cache_ptr != nullptr)
				m_cache_ptr->cache.clear();

			rebuild_tree_cache = true;
		}
		else {
			rebuild_tree_cache = false;
		}
	}
	else {
		bool debug_me = true;
	}


	//exit on empty source
	if (!find_release_artist) {

		return std::pair(rppair_t(), rppair_t());
	}

	t_size list_index = 0;
	t_size master_index = 0, release_index = 0;

	int item_data;
	std::pair<cache_iterator_t, bool> last_cache_emplaced;
	row_col_data row_data{};

	rppair_t  reserve;
	if (rebuild_tree_cache) {

		//CALC CACHE - NOT FILTERED
		reserve = init_filter(find_release_artist, "", true, true, true, tracer_p);
		vec_lv_reserve_fast = reserve.first;
		master_rel_stat_fast = reserve.second;
		m_cache_ptr->cache.reserve(find_release_artist->search_order_master.get_count());
	}

	std::string str_role;

	try {

		Artist_ptr find_release_artist = m_rt_manager->get_find_release_artist();

		for (t_size walk = 0; walk < find_release_artist->search_order_master.get_size(); walk++) {

			bool is_master = find_release_artist->search_order_master[walk];
			pfc::string8 item; //master or release summary

			if (is_master) {

				//master

				str_role = std::string(find_release_artist->master_releases[master_index]->search_role);
				mounted_param thisparam(master_index, ~0, true, false);
				item_data = thisparam.lparam();
				if (updsrc != updRelSrc::Filter)
				{
					hook->set_master(&(find_release_artist->master_releases[master_index]));
					item = m_rt_manager->run_hook_columns(row_data, item_data);
				}
				else
					get_cached_find_release_node(item_data, item, row_data);
			}
			else {

				//no master release

				str_role = std::string(find_release_artist->releases[release_index]->search_role);
				mounted_param thisparam(~0, release_index, false, true);
				item_data = thisparam.lparam();
				if (updsrc != updRelSrc::Filter)
				{
					hook->set_release(&(find_release_artist->releases[release_index]));
					item = m_rt_manager->run_hook_columns(row_data, item_data);
				}
				else
					get_cached_find_release_node(item_data, item, row_data);
			}

			if (rebuild_tree_cache) {

				// *** BULK CACHE EMPLACE (master or no master release) ***

				last_cache_emplaced = m_cache_ptr->cache.emplace(
					item_data, std::pair<row_col_data, flagreg>	{row_data, { m_cache_ptr->_ver, 0 }});
			}

			bool inserted = false; bool expanded = false;

			//>> check
			bool matches = !filtered || check_match(item, filter, lcf_words);
			bool matches_role = !brolemain_filter || (str_role.find("Main") != str_role.npos);
			matches &= matches_role;
			//<< check

			int prev_expanded = 0;
			if (matches) {

				if (is_master)
					tracer_p->release_check(find_release_artist->master_releases[master_index]->id, list_index,
						is_master, list_index, master_index);
				else
					tracer_p->release_check(find_release_artist->releases[release_index]->id, list_index,
						mounted_param(~0, release_index, false, true), list_index, master_index);

				// MASTER OR NO MASTER RELEASES

				if (delete_on_enter) {
					mounted_param myparam((LPARAM)item_data);

					if (myparam.is_master()) {

						cache_iterator_t insert_cache_it = m_cache_ptr->cache.find(myparam.lparam());
						insert_cache_it->second.second.flag = 0;
						insert_cache_it->second.second.ver = m_cache_ptr->_ver;
						int ival = 1;
						m_cache_ptr->SetCacheFlag(insert_cache_it, NodeFlag::none, &ival);
						std::pair<cache_iterator_t, HTREEITEM> vec_row(insert_cache_it, nullptr);

						// *** VEC PUSH (MASTER) ***

						m_vec_ptr->push_back(std::pair<cache_iterator_t, HTREEITEM>(insert_cache_it, (HTREEITEM)nullptr));
					}
					else {

						if (myparam.is_release()) {

							//  no push here for releases (see below)

						}
						else {

							tracer_p->release_check(find_release_artist->releases[release_index]->id, list_index,
								myparam, ~0, ~0);

							cache_iterator_t insert_cache_it = m_cache_ptr->cache.find(myparam.lparam());
							insert_cache_it->second.second.flag = 0;
							insert_cache_it->second.second.ver = m_cache_ptr->_ver;
							int ival = 0;
							m_cache_ptr->SetCacheFlag(insert_cache_it, NodeFlag::added, &ival);
							flagreg& insfr = insert_cache_it->second.second;
							std::pair<cache_iterator_t, HTREEITEM> vecrow(insert_cache_it, (HTREEITEM)nullptr);

							// *** VEC PUSH (non master release) ***

							m_vec_ptr->push_back(std::pair<cache_iterator_t, HTREEITEM>(vecrow));
						}
					}
				}

				if (init_expand && is_master) {

					//todo: removed moving to new class...
					//int expanded = updsrc == updRelSrc::Filter ? 0 : 1;
					//bool bnew = !get_node_expanded(row_data.id, prev_expanded);
				}

				list_index++;
				inserted = true;
			}

			if (is_master) {

				// SUB-RELEASES LOOP

				const MasterRelease_ptr* master_release = &(find_release_artist->master_releases[master_index]);

				for (size_t j = 0; j < master_release->get()->sub_releases.get_size(); j++) {

					mounted_param thisparam = { master_index, j, true, true };

					item_data = thisparam.lparam();

					pfc::string8 sub_item;
					if (updsrc != updRelSrc::Filter)
					{
						//set hook release
						hook->set_release(&(find_release_artist->master_releases[master_index]->sub_releases[j]));
						sub_item = m_rt_manager->run_hook_columns(row_data, item_data);
					}
					else
						get_cached_find_release_node(item_data, sub_item, row_data);

					std::pair<cache_iterator_t, bool> last_cache_subitem_emplaced =
					{ m_cache_ptr->cache.end(), false };

					if (rebuild_tree_cache) {

						// *** BULK CACHE EMPLACE (sub-release) ***

						last_cache_subitem_emplaced =
							m_cache_ptr->cache.emplace(
								item_data, std::pair<row_col_data, flagreg> {row_data, { m_cache_ptr->_ver, 0 }}
						);
					}

					bool matches = !filtered || check_match(sub_item, filter, lcf_words);
					Release_ptr release = find_release_artist->master_releases[master_index]->sub_releases[j];
					str_role = std::string(release->search_role);

					//todo
					bool release_roles_todo = !str_role.size();
					matches_role = release_roles_todo || !brolemain_filter || (str_role.find("Main") != str_role.npos);
					matches &= matches_role;

					if (matches) {

						int master_id = atoi(find_release_artist->master_releases[master_index]->id);

						vec_iterator_t parent;
						find_vec_by_id(*m_vec_ptr, master_id, parent);
						inserted = parent != m_vec_ptr->end();

						bool master_match_role = inserted;

						if (!inserted) {

							// ADD NOT INSERTED PARENT
							// todo: check match criteria/results

							if (delete_on_enter) {

								std::string master_str_rol = find_release_artist->master_releases[master_index]->search_role;
								master_match_role = !brolemain_filter || (master_str_rol.find("Main") != master_str_rol.npos);

								if (/*matches &&*/ master_match_role) {

									mounted_param not_matching_master_param(master_index, ~0, true, false);

									cache_iterator_t insert_cache_it = m_cache_ptr->cache.find(not_matching_master_param.lparam());
									insert_cache_it->second.second.flag = 0;
									insert_cache_it->second.second.ver = m_cache_ptr->_ver;
									int ival = 1;
									m_cache_ptr->SetCacheFlag(insert_cache_it, NodeFlag::none, &ival);

									// *** VEC PUSH (not matching master) ***

									std::pair<cache_iterator_t, HTREEITEM> vec_row(insert_cache_it, nullptr);
									m_vec_ptr->push_back(vec_row);

								}

							}
						}

						// SUBRELEASE

						if (master_match_role && init_expand && prev_expanded == 1) {

							tracer_p->release_check(master_release->get()->sub_releases[j]->id, j, (mounted_param((LPARAM)item_data)), ~0, master_index);

							// insert

							if (delete_on_enter) {
								mounted_param myparam((LPARAM)item_data);
								if (myparam.is_master()) {
									cache_iterator_t insit = last_cache_emplaced.first;
									flagreg& insfr = last_cache_emplaced.first->second.second;

									// *** VEC PUSH (release) ***

									m_vec_ptr->push_back(std::pair<cache_iterator_t, HTREEITEM>
										(last_cache_emplaced.first, nullptr));
								}
								else {
									if (myparam.is_release()) {
										;
									}
									else {
										cache_iterator_t insit = last_cache_emplaced.first;
										flagreg& insfr = last_cache_emplaced.first->second.second;
									}
								}
							}
							list_index++;
						}
					}
				}
			}

			if (is_master) {
				master_index++;
			}
			else {
				release_index++;
			}
		}
	}
	catch (foo_discogs_exception& e) {
		
		throw e;
	}

	int list_count = m_vec_ptr->size();

	rppair_t full_master_rel_stat = { master_rel_stat_slow, master_rel_stat_fast };
	rppair_t full_lv_reserve = rppair_t(vec_lv_reserve_slow, vec_lv_reserve_fast);

	return std::pair(full_lv_reserve, full_master_rel_stat);
}

// update_releases
// from get_artist_done() and apply_filter()

std::pair<rppair_t, rppair_t> CFindReleaseTree::update_releases(const pfc::string8& filter, updRelSrc updsrc, bool init_expand, bool brolemain_filter) {
	
	EnableDispInfo(false);

	std::pair<rppair_t, rppair_t> res;

	try {

		// forward update releases

		res = m_rt_cache.update_releases(filter, updsrc, init_expand, _idtracer_p, brolemain_filter);

		Artist_ptr artist = get_artist();

		rppair rp = rppair(std::pair(std::to_string(res.second.first.first).c_str(), std::to_string(res.second.first.second).c_str()),
			std::pair(artist->name, ""));

		//root stats
		if (artist->loaded_releases)
			m_dlg->print_root_stats(rp);

	}
	catch (foo_discogs_exception& e) {
		EnableDispInfo(true);
		foo_discogs_exception ex;
		ex << "Error formating release list item [" << e.what() << "]";
		throw ex;
	}

	rebuild_treeview();
	EnableDispInfo(true);

	return res;
}

void CFindReleaseTree::init_tracker_i(Artist_ptr artist, pfc::string8 filter_master, pfc::string8 filter_release, bool expanded, bool fast) {
	
	t_size ires = 0;
	bool filtered = filter_master.get_length() > 0;
	bool matches_master = true;
	bool matches_release = true;
	_idtracer_p->master_i = pfc_infinite;
	_idtracer_p->release_i = pfc_infinite;

	int pos = -1;
	if (_idtracer_p->has_amr() || _idtracer_p->release_tag) {
		int master_ndx = 0; int release_ndx = 0; bool master_done = false; bool release_done = false;
		for (size_t walk = 0; walk < artist->search_order_master.get_size(); walk++) {
			if (master_done && release_done && fast) break;
			bool bmaster = artist->search_order_master[walk];
			if (bmaster) {
				MasterRelease_ptr mr_p = artist->master_releases[master_ndx];
				if (filtered)
					matches_master = !stricmp_utf8(mr_p->title, filter_master);

				if (matches_master && atoi(artist->master_releases[master_ndx]->id) == _idtracer_p->master_id) {
					pos++;
					_idtracer_p->master_i = master_ndx;
					_idtracer_p->master_pos = pos;

					master_done = true;
				}
				for (size_t j = 0; j < artist->master_releases[master_ndx]->sub_releases.size(); j++) {
					Release_ptr r_p = artist->master_releases[master_ndx]->sub_releases[j];
					if (filtered)
						matches_release = !stricmp_utf8(r_p->title, filter_release);

					if (matches_release && atoi(artist->master_releases[master_ndx]->sub_releases[j]->id) == _idtracer_p->release_id) {
						_idtracer_p->release_i = j;
						_idtracer_p->release_pos = pos + j;
						if (!_idtracer_p->master_tag) {
							_idtracer_p->master_i = master_ndx;
							_idtracer_p->master_pos = pos;
							_idtracer_p->master_tag = true;
						}
						release_done = true;
					}
				}
			}
			else {
				Release_ptr r_p = artist->releases[release_ndx];
				if (filtered)
					matches_release = !stricmp_utf8(r_p->title, filter_release);

				if (matches_release && atoi(artist->releases[release_ndx]->id) == _idtracer_p->release_id) {
					pos++;
					_idtracer_p->release_i = release_ndx;
					_idtracer_p->release_pos = pos;
					_idtracer_p->master_i = pfc_infinite;
					_idtracer_p->master_pos = -1;

					release_done = true; master_done = true;
				}
			}
			if (bmaster)
				master_ndx++;
			else
				release_ndx++;
		}
	}
}

void CFindReleaseTree::init_titles(Artist_ptr artist, pfc::string8 & filter_hint) {

	m_hook = std::make_shared<titleformat_hook_impl_multiformat>(&artist);

	pfc::string8 frm_album;
	service_ptr_t<titleformat_object> m_album_name_script;
	static_api_ptr_t<titleformat_compiler>()->compile_force(m_album_name_script, "[%album%]");
	metadb_handle_ptr item = m_items[0];
	item->format_title(nullptr, frm_album, m_album_name_script, nullptr);	//ALBUM

	pfc::string8 filter(frm_album.get_length() ? frm_album : "");

	if (_idtracer_p->has_amr() && artist->loaded_releases) {

		init_tracker_i(artist, "", "", false, true);
		init_tracker_i(artist, m_init_master_title, m_init_release_title, false, true);

		pfc::string8 mtitle;
		pfc::string8 rtitle;

		if (_idtracer_p->master_i != ~0) {
			MasterRelease_ptr master_p = artist->master_releases[_idtracer_p->master_i];
			mtitle = master_p->title;
			if (_idtracer_p->release_i.lparam() != -1) {
				size_t rel_ndx = _idtracer_p->release_i.release_ndx;

				Release_ptr release_p = artist->releases[rel_ndx];
				rtitle = master_p->sub_releases[rel_ndx]->title;
			}
			else {
				rtitle = "";
			}
		}
		else {
			if (_idtracer_p->release_i.lparam() != -1) {
				size_t rel_ndx = _idtracer_p->release_i.release_ndx;

				Release_ptr release_p = artist->releases[rel_ndx];
				rtitle = release_p->title;
			}
		}

		if (stricmp_utf8(filter, mtitle))
		{
			pfc::string8 buffer(filter);
			if (buffer.has_prefix(mtitle))
			{
				filter.set_string(mtitle);
			}
			else {
				if (!mtitle.has_prefix(buffer))
				{
					filter = "";
				}
			}
		}

		m_init_master_title = mtitle;
		m_init_release_title = rtitle;
	}

	filter_hint = filter;
}

//todo: unfinished
void CFindReleaseTree::on_get_artist_done(updRelSrc updsrc, Artist_ptr& artist) {

	if (updsrc == updRelSrc::ArtistProfile) {

		if (artist.get() && g_discogs->find_release_artist_dialog) {
			g_discogs->find_release_artist_dialog->UpdateProfile(artist);
		}

		return;
	}

	m_rt_cache.vec_Clear();
	TreeView_DeleteAllItems(m_hwndTreeView);
	m_rt_cache.bulk_Clear();
	m_rt_cache.new_Version();
	
	_idtracer_p->set_artist_fr_id(atoi(artist->id));

	if (g_discogs->find_release_artist_dialog)
		g_discogs->find_release_artist_dialog->UpdateProfile(artist);

	// todo: tidy up init_titles
	// init source album filter

	pfc::string8 hint;
	init_titles(artist, hint);

	if (m_dlg->is_filter_autofill_enabled() && get_artist()) {
		//todo: rev! also does tracer init

		m_results_filter.set_string(hint);

		if (hint/*lastFilter*/.get_length()) {

			CFindReleaseDialog* dlg = (CFindReleaseDialog*)m_dlg;

			if (dlg->is_filter_autofill_enabled()) {

				dlg->block_filter_box_events(true);

				uSetWindowText(m_edit_filter, hint/*filter*/);

				dlg->block_filter_box_events(false);
			}
		}

	}
	else {
		if (updsrc == updRelSrc::Undef || updsrc == updRelSrc::UndefFast) {
			m_results_filter.set_string(hint);
			uSetWindowText(m_edit_filter, hint/*filter*/);
		}
	}
	
	set_find_release_artist(artist);
	
	//filter string...
	if (_idtracer_p->has_amr()) {

		init_tracker_i(artist, m_results_filter, m_results_filter, false, true);
	}

	bool brolemain_filter = m_dlg->config().find_release_filter_flag & FilterFlag::RoleMain;

	//

	// update releases

	//

	std::pair<rppair_t, rppair_t>  res = update_releases(m_results_filter, updsrc, true, brolemain_filter);


	//todo: rev scroll down to release tree item
	//int src_lparam = m_rt_cache.get_src_param(updsrc, _idtracer_p);
	//
	//if (src_lparam != -1) {
	//
	//	on_release_selected(src_lparam);
	//
	//}
}

rppair_t release_tree_cache::init_filter(const Artist_ptr artist, pfc::string8 filter, bool expanded, bool fast, bool no_alloc, id_tracer* tracer_p) {

	titleformat_hook_impl_multiformat_ptr hook = m_rt_manager->get_hook();

	foo_conf cfg = m_rt_manager->m_dlg->config();
	bool bversions_filter = cfg.find_release_filter_flag & FilterFlag::Versions;
	bool brolemain_filter = cfg.find_release_filter_flag & FilterFlag::RoleMain;

	std::pair<t_size, t_size> reserve = std::pair(0, 0);
	if (no_alloc) {
		fast = false; expanded = true;
	}

	m_vec_filter.clear();

	pfc::array_t<pfc::string> lcf_words;
	bool filtered = filter.get_length() && tokenize_filter(filter, lcf_words);

	int pos = -1;
	int master_ndx = 0;
	int release_ndx = 0;
	bool master_done = false;
	bool release_done = false;

	for (size_t walk = 0; walk < artist->search_order_master.get_size(); walk++) {

		bool matches_master = false;
		bool matches_release = false;

		if (master_done && release_done && fast)
			break;

		bool bmaster = artist->search_order_master[walk];

		if (bmaster) {

			// ---- MASTERS ----

			MasterRelease_ptr mr_p = artist->master_releases[master_ndx];

			//>check
			matches_master = true;
			if (filtered) {

				row_col_data row_data{};
				hook->set_master(&mr_p);
				pfc::string8 item = m_rt_manager->run_hook_columns(row_data, (master_ndx << 16) | 9999);

				matches_master = check_match(item, filter, lcf_words);
			}

			if (brolemain_filter) {
				std::string str_role = std::string(mr_p->search_role);
				matches_master &= (str_role.find("Main") != str_role.npos);
			}
			//<check

			if (matches_master) {

				pos++;

				if (atoi(artist->master_releases[master_ndx]->id) == tracer_p->master_id) {
					tracer_p->master_pos = pos;
				}

				// ** EMPLACE MASTER FILTER

				if (!no_alloc)
					m_vec_filter.emplace_back(std::pair<int, int>(master_ndx, pfc_infinite));

				reserve.first++;

				master_done = true;
			}

			for (size_t j = 0; j < artist->master_releases[master_ndx]->sub_releases.size(); j++) {

				// ---- SUB-RELEASES ----

				Release_ptr r_p = artist->master_releases[master_ndx]->sub_releases[j];

				matches_release = true;

				//>check sub-release
				if (bversions_filter && filtered) {

					row_col_data row_data{};
					hook->set_release(&(artist->master_releases[master_ndx]->sub_releases[j]));
					pfc::string8 sub_item = m_rt_manager->run_hook_columns(row_data, (master_ndx << 16) | j);
					matches_release = check_match(sub_item, filter, lcf_words);
				}

				//...
				matches_release = matches_release;
				//...

				if (matches_release) {

					pos++;

					if (atoi(artist->master_releases[master_ndx]->sub_releases[j]->id) == tracer_p->release_id) {
						tracer_p->release_pos = pos + j;
					}

					// ** EMPLACE SUB-RELEASE FILTER

					if (!no_alloc)
						m_vec_filter.emplace_back(std::pair<int, int>(master_ndx, j));

					reserve.second++;

					release_done = true;
				}
			}
		}
		else {

			// ---- NON-MASTER RELEASES

			Release_ptr non_master = artist->releases[release_ndx];

			//>check non-master release
			matches_release = true;
			if (filtered) {

				row_col_data row_data{};
				hook->set_release(&(artist->releases[release_ndx]));
				pfc::string8 item = m_rt_manager->run_hook_columns(row_data, (9999 << 16) | release_ndx);
				matches_release = check_match(item, filter, lcf_words);
			}

			if (brolemain_filter) {
				std::string str_role = std::string(non_master->search_role);
				pfc::array_t<pfc::string8> search_roles = non_master->search_roles;
				for (auto it : search_roles) {
					if (it.equals("Main"))
						break;
				}

				matches_release &= (str_role.find("Main") != str_role.npos);
			}
			//<end check

			if (matches_release) {

				pos++;

				if (tracer_p->need_release()) {
					tracer_p->release_check(non_master->id, pos, mounted_param(pfc_infinite, release_ndx, false, true), pfc_infinite, pfc_infinite);
				}

				// ** EMPLACE NON-MASTER RELEASE FILTER

				if (!no_alloc)
					m_vec_filter.emplace_back(std::pair<int, int>(pfc_infinite, release_ndx));

				reserve.second++;

				release_done = true; master_done = true;
			}
		}

		if (bmaster)
			master_ndx++;
		else
			release_ndx++;
	}
	return rppair_t(reserve, { master_ndx, release_ndx });
}

pfc::string8 CFindReleaseTree::get_param_id(mounted_param myparam) {

	pfc::string8 res_selected_id;
	if (myparam.is_master())
		res_selected_id = m_find_release_artist->master_releases[myparam.master_ndx]->main_release_id;
	else
		if (myparam.is_release()) {
			MasterRelease_ptr dbugmasrel = m_find_release_artist->master_releases[myparam.master_ndx];
			res_selected_id = m_find_release_artist->master_releases[myparam.master_ndx]->sub_releases[myparam.release_ndx]->id;
		}
		else
			res_selected_id = m_find_release_artist->releases[myparam.release_ndx]->id;
	return res_selected_id;
}

int release_tree_cache::get_src_param(updRelSrc updsrc, id_tracer* tracer_p) {

	int src_lparam = -1;

	if (m_vec_ptr->size()) {

		mounted_param myparam;
		if (tracer_p->has_amr()) {

			myparam = tracer_p->release_i.lparam();

			vec_iterator_t vec_lv_it;
			find_vec_by_lparam(*m_vec_ptr, src_lparam, vec_lv_it);

			if (vec_lv_it != m_vec_ptr->end()) {
				src_lparam = myparam.lparam();
				//todo: do we ever get here?
				return src_lparam;
			}
			else {

				//release not loaded, try to select master and expand

				mounted_param not_found;
				myparam = mounted_param(tracer_p->master_i, ~0, tracer_p->master_tag, false);
				src_lparam = myparam.lparam();
				vec_iterator_t vec_lv_it;
				find_vec_by_lparam(*m_vec_ptr, src_lparam, vec_lv_it);

				if (vec_lv_it != m_vec_ptr->end()) {

					src_lparam = myparam.lparam();

					int ival = 1;
					if (updsrc == updRelSrc::Undef) {

						m_cache_ptr->SetCacheFlag(vec_lv_it->first, NodeFlag::expanded, &ival);

						m_rt_manager->SetHit(src_lparam);
					}

					return src_lparam;
				}
				else {

					src_lparam = tracer_p->release_i.lparam();
					vec_iterator_t vec_lv_it;
					find_vec_by_lparam(*m_vec_ptr, src_lparam, vec_lv_it);

					if (vec_lv_it != m_vec_ptr->end()) {
						src_lparam = myparam.lparam();
						int ival = 1;
						m_cache_ptr->SetCacheFlag(vec_lv_it->first, NodeFlag::expanded, &ival);
						return src_lparam;
					}
				}
			}
		}
		else {
			vec_iterator_t vec_lv_it;
			find_vec_by_id(*m_vec_ptr, tracer_p->master_id, vec_lv_it);

			if (vec_lv_it != m_vec_ptr->end()) {
				src_lparam = vec_lv_it->first->first;

				return src_lparam;

			}
		}
	}
	else {
		//no releases loaded yet	
	}

	return src_lparam;
}

bool CFindReleaseTree::OnTreeDisplayCellImage(size_t item, size_t subitem, size_t id, cache_iterator_t cache_it, int& result) {
	
	bool children_done;
	m_rt_cache.get_bulk()->GetCacheFlag(cache_it, NodeFlag::added, &children_done);

	bool tree_children_done;
	bool tree_children_done_ver = m_rt_cache.get_bulk()->GetCacheFlag(cache_it, NodeFlag::tree_created, &tree_children_done);
	
	bool is_traced;
	if (item != pfc_infinite) {
		if (subitem != pfc_infinite)
			is_traced = (id == _idtracer_p->release_id);
		else
			is_traced = (id == _idtracer_p->master_id);
	}
	else {
		is_traced = (id == _idtracer_p->release_id);
	}
	result = is_traced ? TRACER_IMG_NDX + (tree_children_done ? 1: 0) : tree_children_done ? CHILDREN_DONE_IMG_NDX : I_IMAGENONE/*-1*/;
	return true;
}

void CFindReleaseTree::set_image_list() {

	m_hImageList.Create(IDB_BITMAP_TRACER_EXT, 16, ILC_COLOR4, RGB(255, 255, 255));
	SendMessage(m_hwndTreeView, TVM_SETIMAGELIST, 0, (LPARAM)(HIMAGELIST)m_hImageList);
}

void CFindReleaseTree::Inititalize(HWND treeview, HWND filter_edit, HWND url_edit,
	id_tracer* idtracer_p, metadb_handle_list items, pfc::string8 filter_hint) {

	_idtracer_p = idtracer_p;
	_idtracer_p->enabled = true;

	m_edit_filter = filter_edit;
	m_edit_release = url_edit;

	m_hwndTreeView = treeview;

	m_items = items;
	m_results_filter.set_string(filter_hint);

	set_image_list();
}

int CFindReleaseTree::get_param_id_master(mounted_param myparam) {
	if (myparam.is_master())
		return atoi(m_find_release_artist->master_releases[myparam.master_ndx]->id);
	else
		return pfc_infinite;
}

// on release selected

void CFindReleaseTree::on_release_selected(int src_lparam) {

	const std::shared_ptr<vec_t> vec_items = m_rt_cache.get_vec();

	vec_iterator_t vecpos;
	find_vec_by_lparam(*vec_items, src_lparam, vecpos);

	if (vecpos == vec_items->end())
		return;

	int selection_index = std::distance(vec_items->begin(), vecpos);
	mounted_param myparam(vec_items->at(selection_index).first->first);

	if (myparam.is_master()) {

		pfc::string8 release_url;
		if (_idtracer_p->has_amr() && (get_param_id_master(myparam) == _idtracer_p->master_id))
			release_url.set_string(std::to_string(_idtracer_p->release_id).c_str());
		else
			release_url.set_string(get_param_id(myparam));

		if (m_rt_cache.get_bulk()->GetCacheFlag(src_lparam, NodeFlag::expanded)) {

			MasterRelease_ptr& master_release = m_find_release_artist->master_releases[myparam.master_ndx];

			if (master_release->sub_releases.get_size()) {
				abort_callback_dummy dummy;
				fake_threaded_process_status fstatus;

				on_expand_master_release_done(master_release, selection_index, fstatus, dummy);
			}
			else {

				//SPAWN

				MasterRelease_ptr master_rel = m_find_release_artist->master_releases[myparam.master_ndx];

				//EXPAND

				m_dlg->call_expand_master_service(master_rel, selection_index);

				// NOTHING MORE TO DO
			}
		}
		else
		{
			//closing master branch
			uSetWindowText(m_edit_release, release_url);
		}
	}
	else {
		uSetWindowText(m_edit_release, get_param_id(myparam).get_ptr());
	}
}

LRESULT CFindReleaseTree::apply_filter(pfc::string8 filter, bool force_redraw, bool force_rebuild) {

	std::pair<rppair_t, rppair_t> resource_stat;

	size_t cc_filtered_root = 0;
	size_t cc_filtered_expanded = 0;
	size_t cc_non_master = 0;
	size_t cc_filtered_masters = 0;

	filter = trim(filter);

	//pre-condition: artist and filter changed

	bool bfilter_changed = stricmp_utf8(m_results_filter, filter);


	if (!m_find_release_artist || !m_find_release_artist->loaded_releases || (!bfilter_changed && !force_redraw)) {

		m_results_filter.set_string(filter);

		//nothing more to do

		return FALSE;
	}

	// VER ++

	m_rt_cache.get_bulk()->_ver++;

	if (!force_rebuild && filter.get_length() == 0) {

		m_rt_cache.vec_Clear();

		_idtracer_p->release_reset();

		bool brolemain_filter = m_dlg->config().find_release_filter_flag & FilterFlag::RoleMain;

		// * UPDATE RELELASES *

		resource_stat = update_releases(filter, updRelSrc::Filter, true, brolemain_filter);

		cc_filtered_root = resource_stat.first.first.first;
		cc_filtered_expanded = resource_stat.first.first.second;
	}
	else {

		// * FILTER RELEASES *

		//delete cache references, delete tree, increase _ver
		EnableDispInfo(false);
		
		m_rt_cache.vec_Clear();
		TreeView_DeleteAllItems(m_hwndTreeView);
		m_rt_cache.new_Version();

		EnableDispInfo(true);

		//reset tracer
		_idtracer_p->release_reset();

		//init vfilter
		rppair_t res = m_rt_cache.init_filter(m_find_release_artist, filter, true, false, false, _idtracer_p);

		//reset release position (keep ids)
		_idtracer_p->release_reset();

		pfc::array_t<pfc::string> lcf_words;
		bool filtered = tokenize_filter(filter, lcf_words);

		std::vector<std::pair<int, int>>::iterator it;

		//apply filter
		
		const std::shared_ptr<vec_t> vec_items = m_rt_cache.get_vec();
		std::vector<std::pair<int, int>> vec_filter = m_rt_cache.get_filter_vec();

		for (it = vec_filter.begin(); it != vec_filter.end(); it++) {

			t_size master_ndx = it->first;
			t_size release_ndx = it->second;

			const mounted_param myparam = { master_ndx, release_ndx, master_ndx != -1, release_ndx != -1 };

			pfc::string8 item;
			row_col_data row_data;

			if (myparam.is_master()) {
				//..
			}
			else {
				cache_iterator_t parent_it;
				if (myparam.is_release()) {
					std::vector<std::pair<int, int>>::iterator it_parent;
					int master_id = atoi(m_find_release_artist->master_releases[myparam.master_ndx]->id);
					mounted_param parent_mp(myparam.master_ndx, ~0, true, false);
					int parent_lparam = parent_mp.lparam();

					vec_iterator_t parent;
					find_vec_by_lparam(*vec_items, parent_lparam, parent);
					if (parent != vec_items->end()) {

						parent_it = parent->first;

						//PARENT FOUND

						bool bexp;
						bool bthisver = m_rt_cache.get_bulk()->GetCacheFlag(parent->first, NodeFlag::expanded, &bexp);
						int ival = bexp ? 1 : 0;
						m_rt_cache.get_bulk()->SetCacheFlag(parent->first, NodeFlag::expanded, &ival);

					}
					else {
						//INSERT NOT-MATCHING PARENT							
						mounted_param mp_master(master_ndx, ~0, true, false);

						bool bversions_filter = m_dlg->config().find_release_filter_flag & FilterFlag::Versions;
						bool brolemain_filter = m_dlg->config().find_release_filter_flag & FilterFlag::RoleMain;
						if (!bversions_filter) {
 							MasterRelease_ptr mr_p = m_find_release_artist->master_releases[master_ndx];
							if (filtered) {
								std::string str_role = std::string(mr_p->search_role);
								bool matches_master = (!brolemain_filter || (str_role.find("Main") != str_role.npos)) && check_match(mr_p->title, filter, lcf_words);
								if (!matches_master) {
									int ival = 0;
									m_rt_cache.get_bulk()->SetCacheFlag(it->first, NodeFlag::filterok, &ival);
									continue;
								}
							}
						}

						m_rt_cache.get_cached_find_release_node(mp_master.lparam(), item, row_data);
						parent_it = m_rt_cache.get_bulk()->cache.find(mp_master.lparam());

						bool bexp;
						bool bthisver = m_rt_cache.get_bulk()->GetCacheFlag(parent_it, NodeFlag::expanded, &bexp);
						int ival = bexp ? 1 : 0;
						m_rt_cache.get_bulk()->SetCacheFlag(parent_it, NodeFlag::expanded, &ival);

						ival = 1;
						m_rt_cache.get_bulk()->SetCacheFlag(parent_it, NodeFlag::added, &ival);
						//in filter
						m_rt_cache.get_bulk()->SetCacheFlag(parent_it, NodeFlag::filterok, &ival);

						flagreg& insfr = parent_it->second.second;

						// ** VEC PUSH **

						m_rt_cache.vec_PushBack(std::pair<cache_iterator_t, HTREEITEM>(parent_it, nullptr));

						vec_iterator_t  master_row;
						find_vec_by_id(*vec_items, master_id, master_row);
					}
				}
			}

			//todo: mutex in cache
			if (m_rt_cache.get_cached_find_release_node(myparam.lparam(), item, row_data) != true) {
				//..
				return FALSE;
			}
			//

			cache_iterator_t filtered_it = m_rt_cache.get_bulk()->cache.find(myparam.lparam());
			int ival = 1;
			m_rt_cache.get_bulk()->SetCacheFlag(filtered_it, NodeFlag::filterok, &ival);

			// ** VEC_PUSH **
			m_rt_cache.vec_PushBack(std::pair<cache_iterator_t, HTREEITEM>(filtered_it, nullptr));

			cc_filtered_masters = m_rt_cache.vec_Size();
			cc_non_master = m_rt_cache.bulk_Size() - cc_filtered_masters;
		}


		// BUILD TREE ITEMS

		int counter = 0;

		std::vector<HTREEITEM> v_hItem_expanded_masters;
		std::shared_ptr<vec_t>& nu_vec_items = m_rt_cache.get_vec_ref();

		for (auto& walk : *nu_vec_items)
		{
			mounted_param wmyparam(walk.first->first);
			//todo: recycle previously expanded nodes?
			if (!wmyparam.is_master() &&
				!wmyparam.is_nmrelease()) {
				continue;
			}

			TVINSERTSTRUCT tvis = { 0 };
			tvis.item.pszText = LPSTR_TEXTCALLBACK;
			tvis.item.iSelectedImage = I_IMAGECALLBACK;
			tvis.item.cChildren = I_CHILDRENCALLBACK;
			tvis.item.lParam = walk.first->first; /* whatever I need to identify this item and generate its contents later */;

			tvis.hParent = TVI_ROOT;
			tvis.hInsertAfter = TVI_LAST;

			bool bbold = wmyparam.is_master() && (walk.first->second.first.id == _idtracer_p->master_id);
			bbold |= wmyparam.is_nmrelease() && (walk.first->second.first.id == _idtracer_p->release_id);

			if (bbold) {
				tvis.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM | TVIF_STATE;
				//int img_ndx;
				//OnTreeDisplayCellImage(wmyparam.master_ndx, wmyparam.release_ndx, walk.first->second.first.id, walk.first, img_ndx);

				tvis.item.iImage = TRACER_IMG_NDX/*img_ndx*/;
				tvis.item.iSelectedImage = TRACER_IMG_NDX/*img_ndx*/;
				tvis.item.state = TVIS_BOLD;
				tvis.item.stateMask = TVIS_BOLD;
			}
			else {
				tvis.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM | TVIF_STATE;
				tvis.item.iImage = -1;
				tvis.item.iSelectedImage = -1;
			}

			HTREEITEM newnode = TreeView_InsertItem(m_hwndTreeView, &tvis);
			walk.second = newnode;
			int ival = 0;
			std::shared_ptr<filter_cache>& cache_ptr = m_rt_cache.get_bulk();
			cache_ptr->SetCacheFlag(walk.first, NodeFlag::tree_created, &ival);
			bool val;
			if (cache_ptr->GetCacheFlag(walk.first, NodeFlag::expanded, &val)) {
				v_hItem_expanded_masters.emplace_back(newnode);
			}
		}

		for (auto hnode : v_hItem_expanded_masters) {
			TreeView_Expand(m_hwndTreeView, hnode, TVM_EXPAND);
		}
	}

	m_results_filter.set_string(filter);

	return FALSE;
}

void CFindReleaseTree::expand_releases(const pfc::string8& filter, t_size master_index, t_size master_list_pos) {
	try {
		//forward expand
		m_rt_cache.expand_releases(filter, master_index, master_list_pos, _idtracer_p);
		//signal branch hit node to TVM_EXPAND
		ExpandHit();
	}
	catch (foo_discogs_exception& e) {
		foo_discogs_exception ex;
		ex << "Error formating release list subitem [" << e.what() << "]";
		throw ex;
	}
}


void release_tree_cache::expand_releases(const pfc::string8& filter, t_size master_index, t_size master_list_pos, id_tracer* tracer_p) {

	titleformat_hook_impl_multiformat_ptr hook = m_rt_manager->get_hook();

	pfc::array_t<Artist_ptr> m_find_release_artists = m_rt_manager->get_find_release_artists();
	Artist_ptr m_find_release_artist = m_rt_manager->get_find_release_artist();

	foo_conf cfg = m_rt_manager->m_dlg->config();
	bool bversions_filter = cfg.find_release_filter_flag & FilterFlag::Versions;

	pfc::array_t<pfc::string> lcf_words;
	bool filtered = tokenize_filter(filter, lcf_words);

	int list_index = master_list_pos + 1;

	int walk_lparam;
	row_col_data row_data;

	try {
		int skipped = 0;
		mounted_param parent_mp(master_index, ~0, true, false);
		int parent_lparam = parent_mp.lparam();
		cache_t::const_iterator parent_cache = m_cache_ptr->cache.find(parent_lparam);
		cache_t::const_iterator new_release_cache;
		if (parent_cache == m_cache_ptr->cache.end()) {
			int debug = 1;
		}
		try {

			m_rt_manager->get_hook()->set_master(nullptr);
			for (size_t j = 0; j < m_find_release_artist->master_releases[master_index]->sub_releases.get_size(); j++) {

				row_col_data row_data_subitem;
				hook->set_release(&(m_find_release_artist->master_releases[master_index]->sub_releases[j]));
				hook->set_master(&(m_find_release_artist->master_releases[master_index]));

				mounted_param thisparam = { master_index, j, true, true };
				walk_lparam = thisparam.lparam();

				pfc::string8 sub_item;
				if (!get_cached_find_release_node(thisparam.lparam(), sub_item, row_data)) {

					// CACHE MAP EMPLACE
					sub_item = m_rt_manager->run_hook_columns(row_data, walk_lparam);
					cache_t& cache_map = m_cache_ptr->cache;
					cache_map.emplace(walk_lparam,
						std::pair<row_col_data, flagreg> { row_data, { get_Version(), 0 } });
					//
				}

				bool filter_match = !bversions_filter || !filtered || (filtered && check_match(sub_item, filter, lcf_words));

				if (filter_match) {

					cache_t& cache_map = m_cache_ptr->cache;
					cache_iterator_t walk_children = cache_map.find(walk_lparam);

					int ival = 1;
					//leaf spawned
					m_cache_ptr->SetCacheFlag(walk_children, NodeFlag::spawn, &ival);
					//leaf matched filter
					m_cache_ptr->SetCacheFlag(walk_children, NodeFlag::filterok, &ival);

					//tracer
					int row_pos = list_index + j - skipped;
					Release_ptr release = m_find_release_artist->master_releases[master_index]->sub_releases[j];
					tracer_p->release_check(release->id, row_pos, walk_lparam, master_list_pos, master_index);
				}
				else {

					skipped++;
				}
			}
		}
		catch (foo_discogs_exception& e) {
			foo_discogs_exception ex;
			ex << "Error formating release list item [" << e.what() << "]";
			throw ex;
		}

		//mark as pending (children not done) so branches are generated by OnTreeGetInfo->I_CHILDRENCALLBACK->OnTreeExpanding
		m_cache_ptr->SetCacheFlag(parent_lparam, NodeFlag::tree_created, false);
	}
	catch (foo_discogs_exception& e) {
		foo_discogs_exception ex;
		ex << "Error formating release list subitem [" << e.what() << "]";
		throw ex;
	}
	return;
}

void CFindReleaseTree::on_expand_master_release_done(const MasterRelease_ptr& master_release, int list_index, threaded_process_status& p_status, abort_callback& p_abort) {
	int master_i = -1;
    //todo: mutex on item selected)
	for (size_t i = 0; i < m_find_release_artist->master_releases.get_size(); i++) {
		if (m_find_release_artist->master_releases[i]->id == master_release->id) {
			master_i = i;
			break;
		}
	}

	if (master_i == -1)
		return; //do not expand artist selection changed

	pfc::string8 filter = get_edit_filter_string();

	int state_expanded = 1;
	bool bnodeset = set_node_expanded(atoi(master_release->id), state_expanded, false);

	if (bnodeset) {

		m_rt_cache.expand_releases(filter.c_str(), master_i, list_index, _idtracer_p);

		//signal branch node to TVM_EXPAND
		ExpandHit();

		mounted_param mparam(master_i, ~0, true, false);
		t_size lparam = mparam.lparam();

		std::shared_ptr<filter_cache>& cache_ptr = m_rt_cache.get_bulk();

		
		int ival = 1; // * NODE EXPANDED
		cache_ptr->SetCacheFlag(lparam, NodeFlag::expanded, ival);
	}

	const std::shared_ptr<vec_t> vec_items = m_rt_cache.get_vec();

	mounted_param myparam(vec_items->at(list_index).first->first);
	
	pfc::string8 release_url;

	//autofill release_id textbox ...
	size_t autofill_id = get_autofill_release_id(myparam, atoi(master_release->id));

	if (autofill_id != ~0) {
		uSetWindowText(m_edit_release, std::to_string(autofill_id).c_str());
	}
}

bool CFindReleaseTree::set_node_expanded(t_size master_id, int& state, bool build) {

	vec_iterator_t master_row;

	std::shared_ptr<vec_t>& vec_items = m_rt_cache.get_vec_ref();

	find_vec_by_id(*vec_items, master_id, master_row);

	if (master_row != vec_items->end()) {

		std::shared_ptr<filter_cache>& cache_ptr = m_rt_cache.get_bulk();

		int ival_expanded = state;
		cache_ptr->SetCacheFlag(master_row->first, NodeFlag::expanded, &ival_expanded);

		if (ival_expanded) {
			int ival_added = 1;
			cache_ptr->SetCacheFlag(master_row->first, NodeFlag::added, &ival_added);
		}
		return true;
	}
	else {
		state = -1;
	}
	return false;
}

LRESULT CFindReleaseTree::OnReleaseTreeExpanding(int, LPNMHDR hdr, BOOL&) {

	const std::shared_ptr<vec_t> vec_items = m_rt_cache.get_vec();
	std::shared_ptr<filter_cache> cache_ptr = m_rt_cache.get_bulk();

	NMTREEVIEW* pNMTreeView = (NMTREEVIEW*)hdr;
	TVITEMW* pItemExpanding = &(pNMTreeView)->itemNew;
	HTREEITEM* hItemExpanding = &(pNMTreeView->itemNew.hItem);

	// early leaving

	if ((pNMTreeView->action & TVE_EXPAND) != TVE_EXPAND)
	{
		vec_iterator_t master_it;
		find_vec_by_lparam(*vec_items, pItemExpanding->lParam, master_it);
		if (master_it != vec_items->end()) {
			cache_iterator_t cache_it = master_it->first;
			int ival = 0;  // * NOT EXPANDED
			cache_ptr->SetCacheFlag(cache_it, NodeFlag::expanded, &ival);
		}
		return FALSE;
	}

	bool fix_wine = g_os_is_wine;

	if (pItemExpanding->mask & TVIF_CHILDREN || fix_wine)
	{
		mounted_param myparam(pItemExpanding->lParam);
		auto& cache_parent = cache_ptr->cache.find(pItemExpanding->lParam);

		bool children_done;
		cache_ptr->GetCacheFlag(cache_parent, NodeFlag::added, &children_done);

		bool tree_children_done;
		bool tree_children_done_ver = cache_ptr->GetCacheFlag(cache_parent, NodeFlag::tree_created, &tree_children_done);

		if (children_done) {

			if (!tree_children_done_ver) {
				if (myparam.is_master()) {
					t_size try_release_ndx = 0;
					std::vector<TVINSERTSTRUCT> vchildren = {};

					int walk_param = mounted_param(myparam.master_ndx, try_release_ndx, true, true).lparam();
					auto walk_children = cache_ptr->cache.find(walk_param);
					int newindex = 0;
					while (walk_children != cache_ptr->cache.end()) {

						bool ival = false;
						cache_ptr->GetCacheFlag(walk_children, NodeFlag::spawn, &ival);
						bool bfilter;
						bool bfilter_ver = cache_ptr->GetCacheFlag(walk_children, NodeFlag::filterok, &bfilter);

						//..

						if (vec_items->size() == 0 || (vec_items->size() && bfilter_ver)) {

							vchildren.resize(newindex + 1);
							int lparam = walk_children->first;
							mounted_param mp(lparam);

							TV_INSERTSTRUCT* tvis = (TV_INSERTSTRUCT*)&vchildren.at(newindex);
							tvis->hParent = (HTREEITEM)pItemExpanding->hItem;
							tvis->hInsertAfter = TVI_LAST;
							TV_ITEM tvi;
							memset(&tvi, 0, sizeof(tvi));

							tvis->item.pszText = LPSTR_TEXTCALLBACK;
							tvis->item.iSelectedImage = /*newindex*/I_IMAGECALLBACK;
							tvis->item.cChildren = 0; // ctracks; //0;
							tvis->item.lParam = walk_children->first;

							int walk_id = walk_children->second.first.id;

							if (walk_id == _idtracer_p->release_id) {
								tvis->item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM | TVIF_STATE; //TVIF_IMAGE | TVIF_TEXT | TVIF_SELECTEDIMAGE;
								tvis->item.iImage = TRACER_IMG_NDX;
								tvis->item.iSelectedImage = TRACER_IMG_NDX;
								tvis->item.state = TVIS_BOLD;
								tvis->item.stateMask = TVIS_BOLD;
							}
							else {
								tvis->item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM | TVIF_STATE; //TVIF_IMAGE | TVIF_TEXT | TVIF_SELECTEDIMAGE;
								tvis->item.iImage = -1;
								tvis->item.iSelectedImage = -1;
							}

							newindex++;
						}
						walk_param = mounted_param(myparam.master_ndx, ++try_release_ndx, true, true).lparam();
						walk_children = cache_ptr->cache.find(walk_param);
					}

					for (const auto& tvchild : vchildren) {
						HTREEITEM node = TreeView_InsertItem(m_hwndTreeView, &tvchild);
					}

					int ival = 1; //node added, set flag tree_created 
					cache_ptr->SetCacheFlag(pItemExpanding->lParam, NodeFlag::tree_created, ival);

					int ivalex = 1; //and expanded
					cache_ptr->SetCacheFlag(pItemExpanding->lParam, NodeFlag::expanded, &ivalex);
					
					//children #
					pItemExpanding->cChildren = vchildren.size();
				}
				else {

					// END EXPANDING NON MASTER
					size_t ctracks = m_rt_cache.get_level_one_vec_track_count(pItemExpanding->lParam);

					//todo: complete level 2 with release tracks
					pItemExpanding->cChildren = ctracks; // ? ctracks /*I_CHILDRENCALLBACK*/ : 0;
					//..
				}
			}
			else {

				//children were already available, just mark as expanded

				int ival = 1;
				cache_ptr->SetCacheFlag(cache_parent, NodeFlag::expanded, &ival);

				if (myparam.is_nmrelease()) {
					//todo: nmrelease tracks
					size_t ctracks = m_rt_cache.get_level_one_vec_track_count(pItemExpanding->lParam);
					pItemExpanding->cChildren = ctracks; // ? ctracks /*I_CHILDRENCALLBACK*/ : 0;
				}
				return FALSE;
			}
		}
		else {
			if (myparam.is_master()) {

				//children not done -> spawn
				//store hItem to expand after spawn

				m_hit = pItemExpanding->hItem;

				vec_iterator_t master_it;
				find_vec_by_lparam(*vec_items, pItemExpanding->lParam, master_it);

				if (master_it == vec_items->end())
					return TRUE;

				int selection_index = std::distance(vec_items->begin(), master_it);

				if (master_it != vec_items->end()) {
					int ival_expanded = 1;
					cache_ptr->SetCacheFlag(master_it->first, NodeFlag::expanded, &ival_expanded);

					if (ival_expanded) {
						int ival_added = 1;
						cache_ptr->SetCacheFlag(master_it->first, NodeFlag::added, &ival_added);
					}
				}
				on_release_selected(pItemExpanding->lParam);
			}
		}
	}
	return FALSE;
}

//

// TVN_GETDISPINFO

// On tree get info...

//

LRESULT CFindReleaseTree::OnReleaseTreeGetInfo(WORD /*wNotifyCode*/, LPNMHDR hdr, BOOL& /*bHandled*/) {
		
	if (!m_rt_cache.vec_Size() || !m_dispinfo_enabled)
		return FALSE;

	std::shared_ptr<filter_cache>& cache_ptr = m_rt_cache.get_bulk();

	NMTVDISPINFO* pDispInfo = reinterpret_cast<NMTVDISPINFO*>(hdr);
	TVITEMW* pItem = &(pDispInfo)->item;
	HTREEITEM* hItem = &(pItem->hItem);

	int lparam = pItem->lParam;
	mounted_param myparam(lparam);

	size_t ctracks = 0;

	//todo: level two uncompleted
	if (myparam.is_nmrelease()) {
		ctracks = m_rt_cache.get_level_one_vec_track_count(lparam);
	}
	else if (myparam.is_release()) {
		ctracks = m_rt_cache.get_level_two_cache_track_count(lparam);
	}

	// asking for virtual children ?

	if (pItem->mask & TVIF_CHILDREN)
	{
		if (myparam.is_master()) {

			bool fix_wine = g_os_is_wine;

			//generate them for wine, otherwise leave then virtual

			if (fix_wine) {
				if (pDispInfo->item.hItem == m_hit) {

					int count = 0;
					int master_i = mounted_param(lparam).master_ndx;
					auto fit = std::find_if(cache_ptr->cache.begin(), cache_ptr->cache.end(), [=](const auto& au) {
						mounted_param thisparam = mounted_param(au.first);
						return thisparam.master_ndx == myparam.master_ndx; });

					while (fit != cache_ptr->cache.end()) {
						count++;
						fit = std::find_if(++fit, cache_ptr->cache.end(), [=](const auto& au) {
							mounted_param thisparam = mounted_param(au.first);
							return thisparam.master_ndx == myparam.master_ndx; });
					}

					pItem->cChildren = count;
				}
				else {
					pItem->cChildren = WINE_CHILDREN;
				}
			}
			else {

				pItem->cChildren = I_CHILDRENCALLBACK;
			}
		}
		else {
			// (tracks)
			pItem->cChildren = 0;
		}
	}

	if (pItem->mask & TVIF_TEXT)
	{
		//fetch cache - masters and releases
        TCHAR outBuffer[MAX_PATH + 1] = {};
        auto cit = cache_ptr->cache.find(lparam);
        auto row_data = cit->second.first.col_data_list.begin();

		//node literal...
        pfc::string8 nodetext(row_data->second);

		//total tracks...
        if (ctracks) nodetext << " [" << std::to_string(ctracks).c_str()  << "]";

		//to wide and transfer
        pfc::stringcvt::convert_utf8_to_wide(outBuffer, MAX_PATH,
            nodetext.get_ptr(), nodetext.get_length());

        _tcscpy_s(pItem->pszText, pItem->cchTextMax, const_cast<TCHAR*>(outBuffer));
	}

	if (pItem->mask & TVIF_IMAGE) {
	
		auto cit = cache_ptr->cache.find(lparam);

		int img_ndx;
		if (OnTreeDisplayCellImage(myparam.master_ndx, myparam.release_ndx, cit->second.first.id, cit, img_ndx)) {
			pItem->iImage = /*TRACER_IMG_NDX*/img_ndx;
			pItem->iSelectedImage = /*TRACER_IMG_NDX*/img_ndx;
			//pItem.mask |= LVIF_IMAGE;
		}
		else {
			//todo:
			pItem->iImage = I_IMAGECALLBACK;
			pItem->iSelectedImage = I_IMAGECALLBACK;
		}
	}
	return FALSE;
}

// ..

// ..

// ..

t_size release_tree_cache::get_level_one_vec_track_count(LPARAM lParam) {
	
	pfc::string8 release_id;
	size_t ctracks = 0;
	vec_iterator_t find_it;
	find_vec_by_lparam(*m_vec_ptr, lParam, find_it);
	if (find_it != m_vec_ptr->end()) {
		cache_iterator_t cache_it = find_it->first;
		row_col_data rcdata = cache_it->second.first;
		release_id << std::to_string(rcdata.id).c_str();
		Artist_ptr artist = m_rt_manager->get_find_release_artist();
        //not finding any realease...
		unsigned long lkey = encode_mr(0, release_id);

		Release_ptr release = discogs_interface->get_release(lkey, false);

		size_t cdiscs = release->discs.get_count();
		for (size_t walk = 0; walk < cdiscs; walk++) {
			ctracks += release->discs[walk]->tracks.size();
		}
	}
	return ctracks;
}

//todo: uncompleted
t_size release_tree_cache::get_level_two_cache_track_count(LPARAM lparam) {

	bool db_isready = DBFlags(CONF.db_dc_flag).IsReady();
	pfc::string8 release_id;
	size_t ctracks = 0;

	cache_iterator_t cache_it = m_cache_ptr->cache.find(lparam);

	if (cache_it != m_cache_ptr->cache.end()) {

		row_col_data rcdata = cache_it->second.first;
		release_id << std::to_string(rcdata.id).c_str();

		Artist_ptr artist = m_rt_manager->get_find_release_artist();

		//same as level one **

		unsigned long lkey = encode_mr(0, release_id);

		Release_ptr release = discogs_interface->get_release(lkey, false);

		size_t cdiscs = release->discs.get_count();
		for (size_t walk = 0; walk < cdiscs; walk++) {
			ctracks += release->discs[walk]->tracks.size();
		}
		if (!ctracks && db_isready)
			ctracks = release->db_total_tracks;
	}
	return ctracks;
}

//

// release tree selection changing

//

LRESULT CFindReleaseTree::OnReleaseTreeSelChanging(int, LPNMHDR hdr, BOOL& bHandled) {
	LPNMTREEVIEW pnmtv = (LPNMTREEVIEW)hdr;

    //debug...
	bool lost_focus = pnmtv->itemOld.hItem != pnmtv->itemNew.hItem;
	//..
	return FALSE;
}

//

// release tree selection changed

//

LRESULT CFindReleaseTree::OnReleaseTreeSelChanged(int, LPNMHDR hdr, BOOL& bHandled) {

	const std::shared_ptr<filter_cache> cache_ptr = m_rt_cache.get_bulk();

	LPNMTREEVIEW pnmtv = (LPNMTREEVIEW)hdr;
	TVITEM pItemMaster = { 0 };
	pItemMaster.mask = TVIF_CHILDREN | TVIF_PARAM;
	pItemMaster.hItem = pnmtv->itemNew.hItem;
	
	TreeView_GetItem(m_hwndTreeView, &pItemMaster);
	
	mounted_param myparam = mounted_param(pItemMaster.lParam);
	cache_iterator_t it = cache_ptr->cache.find(pItemMaster.lParam);
	
	if (it != cache_ptr->cache.end()) {

		size_t autofill_id = get_autofill_release_id(myparam, it->second.first.id);

		if (autofill_id != ~0)
			uSetWindowText(m_edit_release, std::to_string(autofill_id).c_str());
	}

	bHandled = FALSE;
	return 0;
}

//note: some_id could be master, release or nm-release
//		includes tracer checks (!master_id and release id)
size_t CFindReleaseTree::get_autofill_release_id(mounted_param myparam, size_t some_id) {

	size_t autofill_id = ~0;

	if (myparam.is_master()) {

		// autofill release id textbox

		// 11 tracer has both ids
		if (_idtracer_p->has_master() && _idtracer_p->has_release()) {

			if (_idtracer_p->master_id == some_id) {
				//autofill with tracer
				autofill_id = _idtracer_p->has_release() ? _idtracer_p->release_id : autofill_id;
			}
			else {
				//autofill with main release
				autofill_id = atoi(m_find_release_artist->master_releases[myparam.master_ndx]->main_release_id.c_str());
			}
		}
		// 01 tracer lacks only master id
		else if (!_idtracer_p->has_master() && _idtracer_p->has_release()) {
			//look in branches
			const MasterRelease_ptr* master_release = &(m_find_release_artist->master_releases[myparam.master_ndx]);
			if (!master_release->get()->sub_releases.get_count()) {
			
				//master releases not loaded ?
				//should we wait for expansion
				//now replace it with main release, will be overwritten on expanding the parent master
				
				autofill_id = atoi(m_find_release_artist->master_releases[myparam.master_ndx]->main_release_id.c_str());

			}
			else {
				for (size_t j = 0; j < master_release->get()->sub_releases.get_size(); j++) {
					if (atoi(master_release->get()->sub_releases[j]->id) == _idtracer_p->release_id) {
						autofill_id = _idtracer_p->release_id; //found
						break;
					}
					autofill_id = autofill_id == ~0 ? atoi(m_find_release_artist->master_releases[myparam.master_ndx]->main_release_id.c_str()) : autofill_id;
				}
			}
		}
		// 10, 00 tracer lacks only release_id
		else {
			//autofill with main release
			autofill_id = atoi(m_find_release_artist->master_releases[myparam.master_ndx]->main_release_id.c_str());
		}

	}
	else {
		autofill_id = some_id;
	}

	return autofill_id;
}

size_t CFindReleaseTree::test_getselected(TVITEM& out) {

	if (HTREEITEM hsel = TreeView_GetSelection(m_hwndTreeView)) {

		out.mask = TVIF_PARAM | TVIS_EXPANDED;
		out.hItem = hsel;

		TreeView_GetItem(m_hwndTreeView, &out);
		size_t lparam = out.lParam;

		return lparam;
	}
	return ~0;
}

size_t CFindReleaseTree::test_getatcursor(CPoint screen_pos, TVITEM& out) {

	TVHITTESTINFO tvhitinfo = { 0 };
	tvhitinfo.pt = screen_pos;
	::ScreenToClient(m_hwndTreeView, &tvhitinfo.pt);
		
	bool bres = TreeView_HitTest(m_hwndTreeView, &tvhitinfo);

	if (bres) {
		//TVITEM tvi;
		out.mask = TVIF_PARAM | TVIS_EXPANDED;
		out.hItem = tvhitinfo.hItem;

		TreeView_GetItem(m_hwndTreeView, &out);

		if (out.lParam != ~0) {
			
			return out.lParam;
		}
	}

	return ~0;
}


void CFindReleaseTree::hit_test_release_proceed() {

	TVITEM tvi = { 0 };
	int lparam;
	POINT p;
	GetCursorPos(&p);

	if ((lparam = test_getatcursor(p, tvi)) != ~0) {

		mounted_param myparam(lparam);

		if (!myparam.is_master()) {

			pfc::string8 release_id = get_param_id(myparam);

			m_dlg->on_write_tags(release_id);
		}
	}
}

// default action release tree items

void CFindReleaseTree::vkreturn_test_master_expand_release() {

	TVITEM tvitem = { 0 };
	size_t lparam = test_getselected(tvitem);

	if (lparam != ~0) {

		mounted_param myparam(lparam);

		if (!myparam.is_master()) {

			pfc::string8 release_id = get_param_id(myparam);

			m_dlg->on_write_tags(release_id);
		}
		else {

			HTREEITEM hsel = TreeView_GetSelection(m_hwndTreeView);
			TVITEM tvi;
			tvi.hItem = hsel;
			tvi.mask = TVIF_STATE;
			TreeView_GetItem(m_hwndTreeView, &tvi);
			if ((tvi.state & TVIS_EXPANDED) == TVIS_EXPANDED) {				
				TreeView_Expand(m_hwndTreeView, hsel, TVE_COLLAPSE);
			}
			else {
				TreeView_Expand(m_hwndTreeView, hsel, TVE_EXPAND); //TVE_TOGGLE
			}
			
		}
	}
	return;
}

// double click on release proceeds to match tracks

LRESULT CFindReleaseTree::OnReleaseTreeDoubleClickRelease(int, LPNMHDR hdr, BOOL& bHandled) {


	hit_test_release_proceed();

	return 0;
}

// on context menu

LRESULT CFindReleaseTree::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {

	bHandled = FALSE; // artist list also handles this event (rev)
	
	HWND hwndCtrl = (HWND)wParam;

	if (hwndCtrl != m_hwndTreeView) return FALSE;

	CPoint screen_position;

	bool vkapps = lParam == -1;

	if (vkapps)
	{	
		::GetCursorPos(&screen_position);
	}
	else {
		screen_position = CPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
	}

	TVITEM tviSelection = { 0 };
	TVITEM tviCursor = { 0 };

	size_t param_selection = param_selection = test_getselected(tviSelection);
	size_t param_cursor = test_getatcursor(screen_position, tviCursor);

	if (param_selection  == ~0) {

		if (vkapps && param_cursor == ~0) {
			
			// forward to context menu (empty selection)

			CRect rc;
			::GetWindowRect(hwndCtrl, &rc);
			screen_position = rc.CenterPoint();
			context_menu(~0, screen_position);
			return FALSE;
		}
		else
		{
			//bug2 needs selection
			m_post_selection_param = param_cursor;
		}
	}

	//oncontextmenu triggers before on right mouse button/selection changing
	//(set m_post_selection somewhere else?)

	size_t the_param = ~0;

	if (vkapps) {
		the_param = param_selection;
	}
	else {
		
		//bug1: right click on unselected node

		if (param_cursor != ~0 && param_cursor != param_selection) {
			the_param = param_cursor;
			m_post_selection_param = param_cursor;
			TreeView_SelectItem(m_hwndTreeView, tviCursor.hItem);
		}
		else {
			the_param = param_selection;
		}
	}

	context_menu(the_param, screen_position);

	bHandled = TRUE;
	return FALSE;
}

void CFindReleaseTree::context_menu(size_t param_mr, POINT screen_pos) {

	bool empty_sel = (param_mr == -1);

	mounted_param myparam(param_mr);

	pfc::string8 sourcepage = myparam.brelease ? "View release page" : "View master release page";
	pfc::string8 copytitle = "Copy title to clipboard";
	pfc::string8 copyrow = "Copy to clipboard";
	pfc::string8 filterversions = "filter loaded versions";
	pfc::string8 clearfilter = "Clear filter";
	pfc::string8 mainrole = "main role";

	//debugging
	m_post_selection_param = ~0;

	try {

		enum { ID_VIEW_PAGE = 1, ID_CLP_COPY_TITLE, ID_CLP_COPY_ROW, ID_DLG_CLEAR_FILTER, ID_DLG_FILTER_TOGGLE,  ID_DLG_MAIN_ROLE_TOGGLE };
		HMENU menu = CreatePopupMenu();

		if (!empty_sel) {
		
			uAppendMenu(menu, MF_STRING, ID_VIEW_PAGE, sourcepage);
			uAppendMenu(menu, MF_SEPARATOR, 0, 0);
			uAppendMenu(menu, MF_STRING, ID_CLP_COPY_TITLE, copytitle);
			uAppendMenu(menu, MF_STRING, ID_CLP_COPY_ROW, copyrow);
		}

		if (!empty_sel) {
			uAppendMenu(menu, MF_SEPARATOR, 0, 0);

		}

		foo_conf cfg = m_dlg->config();
		bool enabled_versions = cfg.find_release_filter_flag & FilterFlag::Versions;
		bool enabled_rolemain = cfg.find_release_filter_flag & FilterFlag::RoleMain;
		bool enabled_filter = uGetDlgItemText(m_dlg->m_hWnd, IDC_FILTER_EDIT).get_length();

		uAppendMenu(menu, MF_STRING | (enabled_filter ? 0 : MF_DISABLED | MF_GRAYED), ID_DLG_CLEAR_FILTER, clearfilter);
		uAppendMenu(menu, MF_SEPARATOR, 0, 0);
		uAppendMenu(menu, MF_STRING | (enabled_versions ? MF_CHECKED: 0), ID_DLG_FILTER_TOGGLE, filterversions);
		uAppendMenu(menu, MF_STRING | (enabled_rolemain ? MF_CHECKED : 0), ID_DLG_MAIN_ROLE_TOGGLE, mainrole);


		int cmd = TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD,
			screen_pos.x, screen_pos.y, 0, core_api::get_main_window(), 0);
		DestroyMenu(menu);

		switch (cmd)
		{
		case ID_VIEW_PAGE:
		{
			pfc::string8 url;

			if (myparam.is_release()) {
				url << "https://www.discogs.com/release/" << m_find_release_artist->master_releases[myparam.master_ndx]->sub_releases[myparam.release_ndx]->id;
			}
			else if (myparam.bmaster) {
				url << "https://www.discogs.com/master/" << m_find_release_artist->master_releases[myparam.master_ndx]->id;
			}
			else {
				url << "https://www.discogs.com/release/" << m_find_release_artist->releases[myparam.release_ndx]->id;
			}

			if (url.get_length()) display_url(url);

			break;
		}
		case ID_CLP_COPY_TITLE:
		{
			pfc::string8 buffer;

			if (myparam.is_release()) {

				buffer << m_find_release_artist->master_releases[myparam.master_ndx]->sub_releases[myparam.release_ndx]->title;
			}
			else if (myparam.bmaster) {
				buffer << m_find_release_artist->master_releases[myparam.master_ndx]->title;
			}
			else {
				buffer << m_find_release_artist->releases[myparam.release_ndx]->title;
			}

			if (buffer.get_length()) {

				ClipboardHelper::OpenScope scope;
				scope.Open(core_api::get_main_window());
				ClipboardHelper::SetString(buffer);
			}
			break;
		}
		case ID_CLP_COPY_ROW:
		{
			pfc::string8 utf_buffer;
			row_col_data rcd_out;

			//todo: might be simpler
			/*bool bres =*/ m_rt_cache.get_cached_find_release_node(myparam.lparam(), utf_buffer, rcd_out);
			ClipboardHelper::OpenScope scope; scope.Open(core_api::get_main_window());
			ClipboardHelper::SetString(trim(utf_buffer));

			break;
		}
		case ID_DLG_CLEAR_FILTER:
		{
			uSetDlgItemText(m_dlg->m_hWnd, IDC_FILTER_EDIT, "");

			break;
		}
		case ID_DLG_FILTER_TOGGLE:
		{
			bool checked = IsDlgButtonChecked(m_dlg->m_hWnd, IDC_CHK_FIND_RELEASE_FILTER_VERS) == BST_CHECKED;

			uButton_SetCheck(m_dlg->m_hWnd, IDC_CHK_FIND_RELEASE_FILTER_VERS, !checked);
			::SendMessage(m_dlg->m_hWnd, WM_COMMAND, MAKEWPARAM(IDC_CHK_FIND_RELEASE_FILTER_VERS, BN_CLICKED), 0);

			break;
		}
		case ID_DLG_MAIN_ROLE_TOGGLE:
		{

			bool checked = IsDlgButtonChecked(m_dlg->m_hWnd, IDC_CHK_FIND_RELEASE_FILTER_ROLEMAIN) == BST_CHECKED;

			uButton_SetCheck(m_dlg->m_hWnd, IDC_CHK_FIND_RELEASE_FILTER_ROLEMAIN, !checked);
			::SendMessage(m_dlg->m_hWnd, WM_COMMAND, MAKEWPARAM(IDC_CHK_FIND_RELEASE_FILTER_ROLEMAIN, BN_CLICKED), 0);

			break;
		}
		}
	}
	catch (...) {}

}

LRESULT CFindReleaseTree::OnClick(WORD /*wNotifyCode*/, LPNMHDR /*lParam*/, BOOL& /*bHandled*/) {

	//debug
	//..
	return FALSE;
}

//public serves find-rel dlg

size_t CFindReleaseTree::Get_Artist_List_Position() {
	size_t res;
	res = m_find_release_artist ? m_find_release_artist->search_role_list_pos : pfc_infinite;
	return res;
}

size_t CFindReleaseTree::Get_Size() {
	size_t res;
	res = m_rt_cache.bulk_Size();
	return res;
}

LRESULT CFindReleaseTree::OnRClickRelease(int, LPNMHDR hdr, BOOL&) {
    //debug
	return 0;
}