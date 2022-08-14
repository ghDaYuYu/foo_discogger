#pragma once

#include <map>

#include "fetcher.h"
#include "discogs.h"
#include "exception.h"
#include "json_helpers.h"
#include "cache.h"
#include "ol_cache.h"


using namespace Discogs;
namespace ol = Offline;

const size_t MAX_ARTISTS = 32;

class SkipMng {
public:
	enum {
		SKIP_RELEASE_DLG_MATCHED		= 1 << 0,
		SKIP_RELEASE_DLG_IDED		    = 1 << 1,
		SKIP_PREVIEW_DLG				= 1 << 2,
		SKIP_LOAD_RELEASES_TASK_IDED	= 1 << 3,
		SKIP_BRAINZ_ID_FETCH			= 1 << 5,
	};

	SkipMng() = default;
	constexpr SkipMng(int flags) : value(flags) {}

private:
	int value;
};

class DiscogsInterface
{
private:
	lru_cache<unsigned long, Release_ptr> *cache_releases;
	lru_cache<unsigned long, MasterRelease_ptr> *cache_master_releases;
	lru_cache<pfc::string8, Artist_ptr> *cache_artists;
	lru_cache<pfc::string8, bool> *cache_deleted_releases;

	ol::ol_cache offline_cache;

	pfc::string8 username;
	pfc::array_t<pfc::string8> collection;

	inline Release_ptr get_release_from_cache(const unsigned long lkey) {
		return cache_releases->exists(lkey) ? cache_releases->get(lkey) : nullptr;
	}

	inline MasterRelease_ptr get_master_release_from_cache(const unsigned long lkey/*pfc::string8 &master_id*/) {
		return cache_master_releases->exists(lkey) ? cache_master_releases->get(lkey) : nullptr;
	}

	inline Artist_ptr get_artist_from_cache(const pfc::string8 &artist_id) {
		return cache_artists->exists(artist_id) ? cache_artists->get(artist_id) : nullptr;
	}

	inline void add_release_to_cache(const unsigned long lkey, Release_ptr &release) {

		cache_releases->put(lkey, release);
	}

	inline void add_master_release_to_cache(const int lkey, MasterRelease_ptr &master) {
		cache_master_releases->put(lkey/*master->id*/, master);
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

	pfc::array_t<JSONParser_ptr> get_all_pages_offline_cache(ol::GetFrom gpfFrom, pfc::string8 &id, pfc::string8 &secid, pfc::string8 params, abort_callback &p_abort, const char *msg, threaded_process_status &p_status);
	
	void get_entity_offline_cache(ol::GetFrom getfrom, pfc::string8& artist_id, pfc::string8& release_id, pfc::string8& html, abort_callback& p_abort, const char* msg, threaded_process_status& p_status);

	DiscogsInterface() {

		fetcher = new Fetcher();
		cache_releases = new lru_cache<unsigned long, Release_ptr>(CONF.cache_max_objects);
		cache_master_releases = new lru_cache<unsigned long, MasterRelease_ptr>(CONF.cache_max_objects);
		cache_artists = new lru_cache<pfc::string8, Artist_ptr>(CONF.cache_max_objects);
		cache_deleted_releases = new lru_cache<pfc::string8, bool>(CONF.cache_max_objects);
	}

	~DiscogsInterface() {

		delete fetcher;
		delete cache_releases;
		delete cache_master_releases;
		delete cache_artists;
		delete cache_deleted_releases;
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

	inline int master_release_cache_size() {
		return cache_master_releases->size();
	}
	inline int release_cache_size() {
		return cache_releases->size() + cache_deleted_releases->size();
	}
	inline int artist_cache_size() {
		return cache_artists->size();
	}
	inline int collection_cache_size() {
		return collection.get_count();
	}

	inline void set_cache_size(size_t x) {
		cache_releases->set_max_size(x);
		cache_master_releases->set_max_size(x);
		cache_deleted_releases->set_max_size(x);
		cache_artists->set_max_size(x);
	}

	inline bool offline_cache_save(pfc::string8 path, json_t* root) {
		return offline_cache.Dump_JSON(path, root);
	}

	bool img_ndx_to_artist(Release_ptr p_release, size_t img_ndx, Artist_ptr& artist, size_t& ndx) {

		ndx = img_ndx;
		Artist_ptr this_artist;
		size_t accimg = 0;

		for (auto wra : p_release->artists) {
			this_artist = wra->full_artist;
			if (accimg + this_artist->images.get_count() > img_ndx) break;
			accimg += this_artist->images.get_count();
		}

		ndx -= accimg;
		artist = this_artist;
		return this_artist.get() && ndx < this_artist->images.get_count();
	}

	bool get_thumbnail_from_cache(Release_ptr release, bool isArtist, size_t img_ndx, MemoryBlock& small_art,
		threaded_process_status& p_status, abort_callback& p_abort);

	void search_artist(const pfc::string8 &name, pfc::array_t<Artist_ptr> &exact_matches, pfc::array_t<Artist_ptr> &other_matches, threaded_process_status &p_status, abort_callback &p_abort);
	
	Release_ptr get_release(const unsigned long lkey, bool bypass_is_cache = true, bool bypass = false);
	Release_ptr get_release(const unsigned long lkey, threaded_process_status& p_status, abort_callback& p_abort, bool bypass_cache = false, bool throw_all = false);
	Release_ptr get_release(const unsigned long, const pfc::string8& offline_artist_id, threaded_process_status &p_status, abort_callback &p_abort, bool bypass_cache = false, bool throw_all = false);
	MasterRelease_ptr get_master_release(const pfc::string8 &master_id, const pfc::string8& artist_id, bool bypass_cache = false);
	MasterRelease_ptr get_master_release(const pfc::string8 &master_id, bool bypass_cache = false);
	MasterRelease_ptr get_master_release(const unsigned long lkey, bool bypass_cache = false);

	MasterRelease_ptr get_master_release(const pfc::string8 &master_id, threaded_process_status &p_status, abort_callback &p_abort, bool bypass_cache = false, bool throw_all = false);
	Artist_ptr get_artist(const pfc::string8 &artist_id, bool bypass_cache = false);
	Artist_ptr get_artist(const pfc::string8 &artist_id, bool load_releases, threaded_process_status &p_status, abort_callback &p_abort, bool bypass_cache = false, bool throw_all = false, bool throw_404 = true);

	pfc::string8 get_username(threaded_process_status &p_status, abort_callback &p_abort);
	pfc::string8 load_username(threaded_process_status &p_status, abort_callback &p_abort);
	pfc::array_t<pfc::string8> get_collection(threaded_process_status &p_status, abort_callback &p_abort);

	bool delete_artist_cache(const pfc::string8& artist_id);
};

inline DiscogsInterface *discogs_interface;
