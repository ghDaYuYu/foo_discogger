#pragma once
#include "pfc/pfc.h"
#include <CommCtrl.h>

#include "foo_discogs.h"

#ifndef MOUNTEDPARAM_H
#define MOUNTEDPARAM_H

const struct mounted_param {
	size_t master_ndx;
	size_t release_ndx;
	bool bmaster;
	bool brelease;

	mounted_param(size_t master_ndx, size_t release_ndx, bool bmaster, bool brelease)
		: master_ndx(master_ndx), release_ndx(release_ndx), bmaster(bmaster), brelease(brelease) {}
	mounted_param()
		: master_ndx(~0), release_ndx(~0), bmaster(false), brelease(false) {}

	bool mounted_param::is_master() const {
		return bmaster && !brelease;
	}
	bool mounted_param::is_release() const {
		return bmaster && brelease;
	}
	bool mounted_param::is_nmrelease() const {
		return !bmaster && brelease;
	}

	mounted_param(LPARAM lparam) {
		if (lparam == 0) {
			lparam = lparam;
		}

		if (lparam == pfc_infinite || lparam == -1) {
			master_ndx = pfc_infinite;
			release_ndx = pfc_infinite;
			bmaster = false;
			brelease = false;
		}
		else {
			master_ndx = HIWORD(lparam);
			release_ndx = LOWORD(lparam);
			bmaster = (master_ndx != 9999);
			brelease = (release_ndx != 9999);
		}
	}

	t_size lparam() const {
		t_size ires = ~0;
		if (is_release()) {
			ires = master_ndx << 16;
			ires |= release_ndx;
		}
		else
			if (bmaster) {
				ires = master_ndx << 16 | 9999;
			}
			else
				if (brelease) {
					ires = 9999 << 16 | release_ndx;
				}
		return ires;
	}
};

#endif MOUNTEDPARAM_H

#ifndef IDTRACER_H
#define IDTRACER_H

struct id_tracer {

private:
	size_t artist_id = pfc_infinite;
	size_t artist_fr_id = pfc_infinite;

public:
	bool enabled = false;
	bool artist_tag = false;
	bool master_tag = false;
	bool release_tag = false;

	bool multi_artist = false;

	size_t artist_i = pfc_infinite;
	size_t master_i = pfc_infinite;
	mounted_param release_i = mounted_param(pfc_infinite, pfc_infinite, false, false);

	size_t artist_pos = pfc_infinite;
	size_t master_pos = pfc_infinite;
	size_t release_pos = pfc_infinite;

	size_t master_id = pfc_infinite;
	size_t release_id = pfc_infinite;

	size_t id_tracer::get_fr_artist_id() {
		
		return artist_fr_id ? artist_fr_id : artist_fr_id;

	}

	// artist_id related

	size_t id_tracer::get_artist_id() const {
		return artist_id;
	}
	size_t id_tracer::get_artist_fr_id() const {
		return artist_fr_id;
	}
	size_t id_tracer::get_some_artist_id() const {
		return artist_fr_id == pfc_infinite ? artist_id : artist_fr_id;
	}
	void id_tracer::set_artist_id(size_t id) {
		artist_id = id;
	}
	void id_tracer::set_artist_fr_id(size_t id) {
		artist_fr_id = id;
	}

	// has

	bool id_tracer::has_artist() const {
		//return artist_id != pfc_infinite;
		return get_some_artist_id() != pfc_infinite;
	}
	bool id_tracer::has_master() const {
		return master_id != pfc_infinite;
	}
	bool id_tracer::has_release() const {
		return release_id != pfc_infinite;
	}

	bool id_tracer::need_artist() {
		return (
			enabled &&
			has_artist() &&
			artist_pos == pfc_infinite);
	}

	bool id_tracer::need_master() {
		return (
			enabled &&
			drop_check() &&
			has_master() &&
			master_pos == pfc_infinite && !release_i.is_nmrelease());
	}

	bool id_tracer::need_release() {
		return 
			enabled &&
			drop_check() &&
			has_release() &&
			release_pos == pfc_infinite;
	}


	// reset 

	void id_tracer::artist_reset() {
		artist_pos = pfc_infinite;
	}

	void id_tracer::master_reset() {
		master_pos = pfc_infinite;
	}

	void id_tracer::release_reset() {
		release_pos = pfc_infinite;
	}

	bool id_tracer::artist_check(const pfc::string8 currentid, int currentndx) {
		if (need_artist()) {
			if (artist_id == std::atoi(currentid)) {
				artist_pos = currentndx;
				return true;
			}
		}
		return false;
	}

	bool id_tracer::release_check(const pfc::string8 currentid, int currentndx, const  mounted_param item, int masterndx, int masteri) {
		if (item.is_master()) {
			if (need_master()) {
				if (master_id == std::atoi(currentid)) {
					master_pos = 0;
					return true;
				}
			}
		}
		else {
			if (item.is_release()) {
				if (need_release()) {
					if (release_id == std::atoi(currentid)) {
						release_pos = 0;
						if (masteri != pfc_infinite) {
							//TODO: continue
							//master_i = masteri;
							//master_index = masterndx;
							//master = true;
						}
						return true;
					}
				}			
			}
			else {
				if (need_release()) {
					if (release_id == std::atoi(currentid)) {
						release_pos = 0;
						release_i = item;
						return true;
					}
				}
			}
		}
		return false;
	}

	bool id_tracer::drop_check() {
		if (has_artist()) {
			if (!has_master() && !has_release()) return true;
			else if (!has_master() && has_release()) return true;
			else if (has_master() && !has_release()) return false;
		}
		else {
			if (!has_master() && has_release()) return true;
			else if (has_master()) return false;
		}
		return false;
	}

	bool id_tracer::has_amr() {
		if (release_i.is_nmrelease()) 
			return has_artist() && has_release();
		else
			return has_artist() && has_master() && has_release();
	}

	bool id_tracer::init_tracker_tags(metadb_handle_list items) {

		pfc::string8 artist_id, master_id, release_id;

		file_info_impl finfo;

		pfc::string8 iartist_id;
		for (auto i = 0; i < items.get_count(); i++) {
			items[i]->get_info(finfo);
			g_discogs->file_info_get_tag(items[i], finfo, TAG_ARTIST_ID, artist_id);
			for (auto i = 0; i < items.get_count(); i++) {
                items[i]->get_info(finfo);
                g_discogs->file_info_get_tag(items[i], finfo, TAG_ARTIST_ID, artist_id);
                if (artist_id.get_length()) {
                    if (!iartist_id.get_length()) {
                        iartist_id = artist_id;
                    }
                    else
                    {
                        if (artist_idstricmp_utf8(artist_id, iartist_id)) {
                            artist_id = iartist_id; //keep first for offline cache
                            //
                            multi_artist = true;
                            //
                            break;
                        }
                    }
                }
            }
		}

		metadb_handle_ptr item = items[0];
		item->get_info(finfo);

		pfc::string8 artist_id, master_id, release_id;

		if (g_discogs->file_info_get_tag(item, finfo, TAG_ARTIST_ID, artist_id)) {
			this->artist_id = atoi(artist_id);
			artist_tag = true;
		}
		if (g_discogs->file_info_get_tag(item, finfo, TAG_MASTER_RELEASE_ID, master_id)) {
			this->master_id = atoi(master_id);
			master_tag = true;
		}
		if (g_discogs->file_info_get_tag(item, finfo, TAG_RELEASE_ID, release_id)) {
			this->release_id = atoi(release_id);
			release_tag = true;
		}
		
		return has_amr();
	}
};

#endif //IDTRACER_H

#ifndef ROW_COLUMN_DATA_H
#define ROW_COLUMN_DATA_H

//
//struct row_subitem {
//	int col;
//	pfc::string8 strContent;
//};

struct row_col_data {
	int id = -1;	//master id or release id
	std::list<std::pair<int, pfc::string8>> col_data_list = {}; //column #, column content
};

#endif //ROW_COLUMN_DATA_H

#ifndef FLAGREG
#define FLAGREG

struct flagreg { int ver; int flag; };

#endif //flagreg

#ifndef FILTERCACHE_H
#define FILTERCACHE_H

typedef std::unordered_map<int, std::pair<row_col_data, flagreg>> cache_t;
typedef cache_t::iterator cache_iterator_t;

typedef std::vector<std::pair<cache_iterator_t, HTREEITEM>> vec_t;
typedef vec_t::iterator vec_iterator_t;

namespace {
	void find_vec_by_lparam(vec_t& vec_items, int lparam, vec_iterator_t& out) {
		out = std::find_if(vec_items.begin(), vec_items.end(),
			[&](const std::pair<cache_iterator_t, HTREEITEM> e) {
				return e.first->first == lparam; });
	}
	void find_vec_by_id(vec_t& vec_items, int id, vec_iterator_t& out) {
		out = std::find_if(vec_items.begin(), vec_items.end(),
			[&](const std::pair<cache_iterator_t, HTREEITEM> e) {
				return e.first->second.first.id == id; });
	}
}

enum class NodeFlag {
	none = 0,
	added = 1,
	tree_created = 2,
	spawn = 4,
	expanded = 8,
	to_do = 16,
	filterok = 32
};

struct filter_cache {
	cache_t cache;
	int _ver;

	filter_cache() {
		cache = {};
		_ver = 0;
	}

	bool GetCacheFlag(t_size lparam, NodeFlag flagtype) {
		bool bres = false;
		auto& it = cache.find(lparam);
		if (it != cache.end()) {

			std::pair<row_col_data, flagreg >& my_rel_cache = it->second;
			flagreg& fr = my_rel_cache.second;

			if ((fr.flag & INDEXTOSTATEIMAGEMASK((int)flagtype)) == INDEXTOSTATEIMAGEMASK((int)flagtype)) {
				bres = true;
			}
			else {
				//fr.flag &= ~INDEXTOSTATEIMAGEMASK(flag);
				bres = false;
			}

			bres = bres && fr.ver == _ver;
		}
		else {
			bres = false;
		}
		return bres;
	}

	bool GetCacheFlag(
		cache_iterator_t it_cached_item,
		NodeFlag flagtype, bool* const& val) {

		bool bres = false;
		bool bval = false;

		if (it_cached_item->first > -1) {

			flagreg& fr = it_cached_item->second.second;

			if ((fr.flag & INDEXTOSTATEIMAGEMASK((int)flagtype)) == INDEXTOSTATEIMAGEMASK((int)flagtype)) {
				bval = true;
			}
			else {
				//fr.flag &= ~INDEXTOSTATEIMAGEMASK(flag);
				bval = false;
			}

			bres = bval && fr.ver == _ver;
			if (val != NULL)
				*val = bval;
		}
		else {
			bres = false;
		}

		return bres;
	}

	bool GetCacheFlag(t_size lparam, NodeFlag flagtype, bool& val) {
		bool bres = false;
		auto& it = cache.find(lparam);
		if (it != cache.end()) {
			std::pair<row_col_data, flagreg >& my_rel_cache = it->second;
			flagreg& fr = my_rel_cache.second;

			if ((fr.flag & INDEXTOSTATEIMAGEMASK((int)flagtype)) == INDEXTOSTATEIMAGEMASK((int)flagtype)) {
				val = true;
			}
			else {
				//fr.flag &= ~INDEXTOSTATEIMAGEMASK(flag);
				val = false;
			}

			bres = val && fr.ver == _ver;
		}
		else {
			bres = false;
		}
		return bres;
	}

	bool SetCacheFlag(t_size lparam, NodeFlag flagtype, int& val) {
		bool bres = false;
		auto& it = cache.find(lparam);

		if (it != cache.end()) {

			std::pair<row_col_data, flagreg >& my_rel_cache = it->second;
			flagreg& fr = my_rel_cache.second;

			switch ((int)flagtype) {
			case 0:
				break;
			case 1:
				break;
			case 2:
				break;
			case (int)NodeFlag::spawn: //4 (downloaded?)
				break;
			case (int)NodeFlag::expanded: //8
				break;
			case 16:
				break;
			case /*32*/(int)NodeFlag::filterok:
				fr.ver = _ver;
				break;
			default:
				break;
			}

			if (val == -1) {
				fr.flag ^= INDEXTOSTATEIMAGEMASK((int)flagtype);
				val = (fr.flag & INDEXTOSTATEIMAGEMASK((int)flagtype)) == INDEXTOSTATEIMAGEMASK((int)flagtype);
			}
			else if (val) {
				fr.flag |= INDEXTOSTATEIMAGEMASK((int)flagtype);
			}
			else {
				fr.flag &= ~INDEXTOSTATEIMAGEMASK((int)flagtype);
			}
			/*fr.ver = ver;*/
			bres = true;
		}
		else {
			bres = false;
		}
		return bres;
	}

	bool SetCacheFlag(t_size lparam, NodeFlag flagtype, bool val) {
		bool bres = false;
		auto& it = cache.find(lparam);
		if (it != cache.end()) {

			std::pair<row_col_data, flagreg >& my_rel_cache = it->second;
			flagreg& fr = my_rel_cache.second;

			switch ((int)flagtype) {
			case 0:
				break;
			case 1:
				break;
			case 2:
				break;
			case (int)NodeFlag::spawn: //4 (downloaded?)
				break;
			case (int)NodeFlag::expanded: //8
				break;
			case 16:
				break;
			case /*32*/(int)NodeFlag::filterok:
				fr.ver = _ver;
				break;
			default:
				break;
			}

			/*if (val == -1) {
				fr.flag ^= INDEXTOSTATEIMAGEMASK((int)flagtype);
				val = (fr.flag & INDEXTOSTATEIMAGEMASK((int)flagtype)) == INDEXTOSTATEIMAGEMASK((int)flagtype);
			}
			else*/ if (val) {
				fr.flag |= INDEXTOSTATEIMAGEMASK((int)flagtype);
			}
			else {
				fr.flag &= ~INDEXTOSTATEIMAGEMASK((int)flagtype);
			}
			/*fr.ver = ver;*/
			bres = true;
		}
		else {
			bres = false;
		}
		return bres;
	}

	bool SetCacheFlag(cache_t::iterator& it_cached_item,
		NodeFlag flagtype, int* const& val) {

		bool bres = false;

		if (it_cached_item->first > -1) {

			flagreg& fr = it_cached_item->second.second;

			switch ((int)flagtype) {
			case 0:
				break;
			case 1:
				break;
			case 2:
				break;
			case (int)NodeFlag::spawn: //4 (downloaded?)
				break;
			case (int)NodeFlag::expanded: //8
				break;
			case 16:
				break;
			case /*32*/(int)NodeFlag::filterok:
				fr.ver = _ver;
				break;
			default:
				break;
			}

			if (*val == -1) {
				fr.flag ^= INDEXTOSTATEIMAGEMASK((int)flagtype);
				*val = (fr.flag & INDEXTOSTATEIMAGEMASK((int)flagtype)) == INDEXTOSTATEIMAGEMASK((int)flagtype);
			}
			else if (*val) {
				fr.flag |= INDEXTOSTATEIMAGEMASK((int)flagtype);
			}
			else {
				fr.flag &= ~INDEXTOSTATEIMAGEMASK((int)flagtype);
			}
			//fr.ver = ver;
			bres = true;
		}
		else {
			bres = false;
		}
		return bres;
	}
};

#endif //FILTERCACHE_H