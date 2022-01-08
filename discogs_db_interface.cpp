#include "stdafx.h"

#ifdef DB_DC

#include "db_fetcher.h"
#include "db_utils.h"
#include "db_fetcher_component.h"

//access to memory cache
#include "discogs_interface.h"
#include "discogs_db_interface.h"

using namespace Discogs;

bool Discogs_DB_Interface::test_DB(pfc::string8 db_path, abort_callback& p_abort, threaded_process_status& p_status) {
	sqldb db;
	return db.test_dc_database(db_path, p_abort, p_status);
}

//note: dbfetcher interrupts can be triggered on cancel
void Discogs_DB_Interface::search_artist(const pfc::string8& artist_hint, const int db_dc_flags, pfc::array_t<Artist_ptr>& exact_matches, pfc::array_t<Artist_ptr>& other_matches, threaded_process_status& p_status, abort_callback& p_abort, db_fetcher* dbfetcher) {
	
	pfc::string8 json;

	exact_matches.force_reset();
	other_matches.force_reset();

	pfc::array_t<Artist_ptr> good_matches;
	pfc::array_t<Artist_ptr> wtf_matches;
	pfc::array_t<Artist_ptr> matches;

	if (dbfetcher != nullptr) {
		dbfetcher->fetch_search_artist(artist_hint, db_dc_flags, json, p_status, p_abort);
	}

	if (json.get_length()) {

		exact_matches.force_reset();
		other_matches.force_reset();

		pfc::array_t<Artist_ptr> good_matches;
		pfc::array_t<Artist_ptr> wtf_matches;
		pfc::array_t<Artist_ptr> matches;


#ifdef _DEBUG
		log_msg(json);
#endif
		JSONParser jp(json);

		parseArtistResults(jp.root, matches);

		pfc::string8 lowercase_query = lowercase(artist_hint);
		pfc::string8 stripped_query = strip_artist_name(artist_hint);

		for (size_t i = 0; i < matches.get_size(); i++) {
			Artist_ptr& artist = matches[i];
			pfc::string8 lowercase_name = lowercase(artist->name);
			pfc::string8 stripped_name = strip_artist_name(artist->name);
			if (lowercase_query == lowercase_name) {
				exact_matches.append_single(std::move(artist));
			}
			else if (stripped_query == stripped_name) {
				good_matches.append_single(std::move(artist));
			}
			else if (lowercase_name.find_first(stripped_query) != pfc::infinite_size) {
				other_matches.append_single(std::move(artist));
			}
			else {
				wtf_matches.append_single(std::move(artist));
			}
		}
		exact_matches.append(good_matches);
		other_matches.append(wtf_matches);
	}
	else {
		//no results
	}
}

void Discogs_DB_Interface::get_artist_DB(pfc::string8& id, pfc::string8& html, abort_callback& p_abort, const char* msg, threaded_process_status& p_status, db_fetcher* dbfetcher) {

	if (dbfetcher != nullptr)
		dbfetcher->get_artist(id, html, p_abort, msg, p_status);

}

Artist_ptr Discogs_DB_Interface::get_artist(const pfc::string8& artist_id, bool _load_releases, threaded_process_status& p_status, abort_callback& p_abort, bool bypass_cache, bool throw_all, db_fetcher* dbfetcher) {
	
	//todo: decouple memory cache from discogs_interface
	Artist_ptr artist = bypass_cache ? nullptr : discogs_interface->get_artist(artist_id, bypass_cache);

	if (!artist->loaded) {
		try {
			if (!DBFlags(CONF.db_dc_flag).WantArtwork()) {
				pfc::string8 html;
				get_artist_DB(artist->id, html, p_abort, "Loading artist from db...", p_status, dbfetcher);

				JSONParser jp(html);
			
				parseArtist(&*artist, jp.root);
				artist->loaded = true;

			}
			else {
				artist->load(p_status, p_abort, throw_all);
				artist->loaded = true;
			}
		}
		catch (foo_db_cmd_open_exception) {
			throw;
		}
	}
	if (_load_releases && !artist->loaded_releases) {
		artist->load_releases(p_status, p_abort, false, dbfetcher);
	}
	return artist;
}

pfc::array_t<JSONParser_ptr> Discogs_DB_Interface::releases_get_all_pages(pfc::string8& id, pfc::string8& secid, pfc::string8 params, abort_callback& p_abort, const char* msg, threaded_process_status& p_status, db_fetcher* dbfetcher) {

	pfc::array_t<JSONParser_ptr> res;

	if (dbfetcher != nullptr)
		res = dbfetcher->releases_get_all_pages(id, secid, params, p_abort, msg, p_status);

	return res;
}

pfc::array_t<JSONParser_ptr> Discogs_DB_Interface::versions_get_all_pages(pfc::string8& id, pfc::string8& secid, pfc::string8 params, abort_callback& p_abort, const char* msg, threaded_process_status& p_status, db_fetcher* dbfetcher) {
	
	pfc::array_t<JSONParser_ptr> res;

	if (dbfetcher != nullptr) {
		res = dbfetcher->versions_get_all_pages(id, secid, params, msg, p_status, p_abort);
	}

	return res;
}

void Discogs_DB_Interface::get_release_db(pfc::string8& id, pfc::string8& html, pfc::string8 params, abort_callback& p_abort, const char* msg, threaded_process_status& p_status, db_fetcher* dbfetcher) {
	
	if (dbfetcher != nullptr) {
		dbfetcher->get_release(id, html, params, p_abort, msg, p_status);
	}
}

#endif //define DB_DC