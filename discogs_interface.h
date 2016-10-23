#pragma once

#include <map>

#include "fetcher.h"
#include "discogs.h"
#include "exception.h"
#include "json_helpers.h"
#include "cache.h"


using namespace Discogs;


class DiscogsInterface
{
private:
	lru_cache<pfc::string8, Release_ptr> *cache_releases;
	lru_cache<pfc::string8, MasterRelease_ptr> *cache_master_releases;
	lru_cache<pfc::string8, Artist_ptr> *cache_artists;
	lru_cache<pfc::string8, bool> *cache_deleted_releases;
	
	pfc::string8 username;
	pfc::array_t<pfc::string8> collection;


	inline Release_ptr get_release_from_cache(const pfc::string8 &release_id) {
		return cache_releases->exists(release_id) ? cache_releases->get(release_id) : nullptr;
	}

	inline MasterRelease_ptr get_master_release_from_cache(const pfc::string8 &master_id) {
		return cache_master_releases->exists(master_id) ? cache_master_releases->get(master_id) : nullptr;
	}

	inline Artist_ptr get_artist_from_cache(const pfc::string8 &artist_id) {
		return cache_artists->exists(artist_id) ? cache_artists->get(artist_id) : nullptr;
	}

	inline void add_release_to_cache(Release_ptr &release) {
		cache_releases->put(release->id, release);
	}

	inline void add_master_release_to_cache(MasterRelease_ptr &master) {
		cache_master_releases->put(master->id, master);
	}

	inline void add_artist_to_cache(Artist_ptr &artist) {
		cache_artists->put(artist->id, artist);
	}

	inline void assert_release_id_not_deleted(const pfc::string8 &release_id) {
		if (cache_deleted_releases->exists(release_id)) {
			http_404_exception ex;
			ex << " Release " << release_id << " is deleted.";
			throw ex;
		}
	}

	inline void add_deleted_release_id(const pfc::string8 &release_id) {
		cache_deleted_releases->put(release_id, true);
	}

public:
	Fetcher *fetcher;

	pfc::array_t<JSONParser_ptr> get_all_pages(pfc::string8 &url, pfc::string8 params, abort_callback &p_abort);
	pfc::array_t<JSONParser_ptr> get_all_pages(pfc::string8 &url, pfc::string8 params, abort_callback &p_abort, const char *msg, threaded_process_status &p_status);

	DiscogsInterface() {
		fetcher = new Fetcher();
		cache_releases = new lru_cache<pfc::string8, Release_ptr>(CONF.cache_max_objects);
		cache_master_releases = new lru_cache<pfc::string8, MasterRelease_ptr>(CONF.cache_max_objects);
		cache_artists = new lru_cache<pfc::string8, Artist_ptr>(CONF.cache_max_objects);
		cache_deleted_releases = new lru_cache<pfc::string8, bool>(CONF.cache_max_objects);
	}

	~DiscogsInterface() {
		delete fetcher;
	}

	inline void reset_master_release_cache() {
		cache_master_releases->clear();
	}
	inline void reset_release_cache() {
		cache_releases->clear();
		cache_deleted_releases->clear();
	}
	inline void reset_artist_cache() {
		cache_artists->clear();
	}
	inline void reset_collection_cache() {
		collection.force_reset();
	}

	inline bool is_empty_master_release_cache() {
		return cache_master_releases->size() == 0;
	}
	inline bool is_empty_release_cache() {
		return cache_releases->size() == 0 && cache_deleted_releases->size() == 0;
	}
	inline bool is_empty_artist_cache() {
		return cache_artists->size() == 0;
	}
	inline bool is_empty_collection_cache() {
		return collection.get_count() == 0;
	}

	inline void set_cache_size(size_t x) {
		cache_releases->set_max_size(x);
		cache_master_releases->set_max_size(x);
		cache_deleted_releases->set_max_size(x);
		cache_artists->set_max_size(x);
	}
		
	void search_artist(const pfc::string8 &name, pfc::array_t<Artist_ptr> &exact_matches, pfc::array_t<Artist_ptr> &other_matches, threaded_process_status &p_status, abort_callback &p_abort);

	Release_ptr get_release(const pfc::string8 &release_id, bool bypass_cache = false);
	Release_ptr get_release(const pfc::string8 &release_id, threaded_process_status &p_status, abort_callback &p_abort, bool bypass_cache = false, bool throw_all = false);
	MasterRelease_ptr get_master_release(const pfc::string8 &master_id, bool bypass_cache = false);
	MasterRelease_ptr get_master_release(const pfc::string8 &master_id, threaded_process_status &p_status, abort_callback &p_abort, bool bypass_cache = false, bool throw_all = false);
	Artist_ptr get_artist(const pfc::string8 &artist_id, bool bypass_cache = false);
	Artist_ptr get_artist(const pfc::string8 &artist_id, bool load_releases, threaded_process_status &p_status, abort_callback &p_abort, bool bypass_cache = false, bool throw_all = false);

	pfc::string8 get_username(threaded_process_status &p_status, abort_callback &p_abort);
	pfc::string8 load_username(threaded_process_status &p_status, abort_callback &p_abort);
	pfc::array_t<pfc::string8> get_collection(threaded_process_status &p_status, abort_callback &p_abort);
};

extern DiscogsInterface *discogs_interface;
