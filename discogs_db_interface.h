#pragma once
#include "version.h"

#include "json_helpers.h"

#include "discogs.h"
#include "db_fetcher.h"

using namespace Discogs;

class DBFlags {
public:
	enum {
		DB_ENABLED			= 1 << 0,
		DB_WRITE			= 1 << 1,
		DB_CREATE			= 1 << 2,
		DB_SEARCH_ANV		= 1 << 3,
		DB_SEARCH			= 1 << 4,
		DB_SEARCH_LIKE		= 1 << 5,
		DB_DWN_ARTWORK		= 1 << 6,
		DB_ERROR			= 1 << 7
	};

	DBFlags() = default;
	constexpr DBFlags(int flags) : value(flags) {}

	constexpr bool IsReady() const { 
		return (value & (/*DB_ENABLED |*/ DB_SEARCH &~ DB_ERROR)) == (/*DB_ENABLED |*/ DB_SEARCH &~ DB_ERROR);
	}
	constexpr bool WantArtwork() const {
		return (value & DB_DWN_ARTWORK);
	}
	constexpr bool Search() const {
		return (value & DB_SEARCH);
	}
	constexpr bool SearchLike() const {
		return (value & DB_SEARCH_LIKE);
	}
	constexpr bool SearchAnv() const {
		return (value & DB_SEARCH_ANV);
	}
	void SwitchFlag(int flag, bool enabled) {
		if (enabled)
			value |= flag;
		else
			value &= ~flag;
	}
	constexpr int GetFlag() {
		return value;
	}
private:
	int value;
};

#ifdef DB_DC

class Discogs_DB_Interface {

public:

	Discogs_DB_Interface() {
		//
	}

	~Discogs_DB_Interface() {
		//
	}

	bool test_DB(pfc::string8 db_path, abort_callback& p_abort, threaded_process_status& p_status);

	void search_artist(const pfc::string8 &artist_hint, const int db_dc_flags, pfc::array_t<Artist_ptr>& exact_matches, pfc::array_t<Artist_ptr>& other_matches, threaded_process_status& p_status, abort_callback& p_abort, db_fetcher* dbfetcher = nullptr);
	void get_artist_DB(pfc::string8& id, pfc::string8& html, abort_callback& p_abort, const char* msg, threaded_process_status& p_status, db_fetcher* dbfetcher);
	Artist_ptr get_artist(const pfc::string8& artist_id, bool _load_releases, threaded_process_status& p_status, abort_callback& p_abort, bool bypass_cache, bool throw_all, db_fetcher* dbfetcher);
	void get_release_db(pfc::string8& id, pfc::string8& html, pfc::string8 params, abort_callback& p_abort, const char* msg, threaded_process_status& p_status, db_fetcher* dbfetcher);
	
	// api.discogs.com/artists/1234/releases
	pfc::array_t<JSONParser_ptr> releases_get_all_pages(pfc::string8& id, pfc::string8& secid, pfc::string8 params, abort_callback& p_abort, const char* msg, threaded_process_status& p_status, db_fetcher* dbfetcher);
	// api.discogs.com/versions/1234
	pfc::array_t<JSONParser_ptr> versions_get_all_pages(pfc::string8& id, pfc::string8& secid, pfc::string8 params, abort_callback& p_abort, const char* msg, threaded_process_status& p_status, db_fetcher* dbfetcher);

};

extern Discogs_DB_Interface* discogs_db_interface;
#endif //DB_DC