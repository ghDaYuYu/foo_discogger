#include "stdafx.h"

#include "discogs_interface.h"
#include "exception.h"
#include "utils.h"

namespace ol = Offline;

Release_ptr DiscogsInterface::get_release(const unsigned long lkey, bool bypass_is_cache, bool bypass) {
	pfc::string8 release_id = std::to_string(decode_mr(lkey).second).c_str();
	assert_release_id_not_deleted(release_id);
	Release_ptr release = bypass_is_cache && bypass ? nullptr : get_release_from_cache(lkey);
	if (!release) {
		std::pair<int, unsigned long> dec = decode_mr(lkey);
		release = std::make_shared<Release>(release_id);
		if (!(bypass_is_cache && bypass)) {
			add_release_to_cache(lkey, release);
		}
	}
	return release;
}

Release_ptr DiscogsInterface::get_release(const unsigned long lkey, threaded_process_status& p_status, abort_callback& p_abort, bool bypass_cache, bool throw_all) {
	//todo: depricate-candidates codes use this (ej. bulk updates), 
	const pfc::string8 dummy_offline_artist_id;
	return get_release(lkey, dummy_offline_artist_id, p_status, p_abort, bypass_cache, throw_all);
}

Release_ptr DiscogsInterface::get_release(const unsigned long lkey, const pfc::string8 &offline_artist_id, threaded_process_status &p_status, abort_callback &p_abort, bool bypass_cache, bool throw_all) {

	pfc::string8 release_id = std::to_string(decode_mr(lkey).second).c_str();

	assert_release_id_not_deleted(release_id);

	Release_ptr release = bypass_cache ? nullptr : get_release_from_cache(lkey);

	if (!release) {
		release = std::make_shared<Release>(release_id);
		if (!bypass_cache) {
			add_release_to_cache(lkey, release);
		}
	}

	std::lock_guard<std::mutex> ul(cache_releases->modify_mutex);
	if (!release->loaded) {
		try {
			release->load(p_status, p_abort, throw_all, offline_artist_id /*, nullptr*/);
		}
		catch (http_404_exception) {
			add_deleted_release_id(release_id);
			throw;
		}
	}
	return release;
}

MasterRelease_ptr DiscogsInterface::get_master_release(const unsigned long lkey, bool bypass_cache) {

	MasterRelease_ptr master = bypass_cache ? nullptr : get_master_release_from_cache(lkey/*master_id*/);
	
	if (!master) {
		
		auto decoded_mr = decode_mr(lkey);
		pfc::string8 strlkey_master = std::to_string(decoded_mr.second).c_str();

		master = std::make_shared<MasterRelease>(strlkey_master/*master_id*/);
		if (!bypass_cache) {
			add_master_release_to_cache(lkey, master);
		}
	}
	return master;
}

MasterRelease_ptr DiscogsInterface::get_master_release(const pfc::string8& master_id, const pfc::string8& artist_id, bool bypass_cache) {

	unsigned long lkey = encode_mr(atoi(artist_id), atoi(master_id));	
	
	MasterRelease_ptr master = bypass_cache ? nullptr : get_master_release_from_cache(lkey/*master_id*/);
	if (!master) {
		master = std::make_shared<MasterRelease>(master_id);
		if (!bypass_cache) {
			add_master_release_to_cache(lkey, master);
		}
	}
	return master;
}

MasterRelease_ptr DiscogsInterface::get_master_release(const pfc::string8& master_id, bool bypass_cache) {

	unsigned long lkey = encode_mr(0, atol(master_id));

	MasterRelease_ptr master = bypass_cache ? nullptr : get_master_release_from_cache(lkey/*master_id*/);
	if (!master) {
		master = std::make_shared<MasterRelease>(master_id);
		if (!bypass_cache) {
			add_master_release_to_cache(lkey, master);
		}
	}
	return master;
}

MasterRelease_ptr DiscogsInterface::get_master_release(const pfc::string8 &master_id, threaded_process_status &p_status, abort_callback &p_abort, bool bypass_cache, bool throw_all) {
	
	unsigned long lkey = encode_mr(0, atol(master_id));
	
	MasterRelease_ptr master = bypass_cache ? nullptr : get_master_release_from_cache(lkey/*master_id*/);
	if (!master) {
		master = std::make_shared<MasterRelease>(master_id);
		if (!bypass_cache) {
			add_master_release_to_cache(lkey, master);
		}
	}
	if (!master->loaded) {
		try {
			master->load(p_status, p_abort, throw_all);
		}
		catch (http_404_exception) {
			throw;
		}
	}
	return master;
}

Artist_ptr DiscogsInterface::get_artist(const pfc::string8 &artist_id, bool bypass_cache) {
	Artist_ptr artist = bypass_cache ? nullptr : get_artist_from_cache(artist_id);
	if (!artist) {
		artist = std::make_shared<Artist>(artist_id);
		if (!bypass_cache) {
			add_artist_to_cache(artist);
		}
	}
	return artist;
}

Artist_ptr DiscogsInterface::get_artist(const pfc::string8 &artist_id, bool _load_releases, threaded_process_status &p_status, abort_callback &p_abort, bool bypass_cache, bool throw_all, bool throw_404) {

	Artist_ptr artist = bypass_cache ? nullptr : get_artist_from_cache(artist_id);
	if (!artist) {
		artist = std::make_shared<Artist>(artist_id);
		if (!bypass_cache) {
			add_artist_to_cache(artist);
		}
	}
	if (!artist->loaded) {
		try {
			artist->load(p_status, p_abort, throw_all);
		}
		catch (http_404_exception) {
			if (throw_404)
				throw;
			else {
				artist->profile = "foo_discogger: 404 Artist not found.";
				return artist;
			}
		}
	}

	if (_load_releases && !artist->loaded_releases) {
		artist->load_releases(p_status, p_abort, false, nullptr);
	}
	return artist;
}

void DiscogsInterface::search_artist(const pfc::string8 &query, pfc::array_t<Artist_ptr> &exact_matches, pfc::array_t<Artist_ptr> &other_matches, threaded_process_status &p_status, abort_callback &p_abort) {
	pfc::string8 json;

	exact_matches.force_reset();
	other_matches.force_reset();

	pfc::array_t<Artist_ptr> good_matches;
	pfc::array_t<Artist_ptr> wtf_matches;
	pfc::array_t<Artist_ptr> matches;

	pfc::string8 params;
	params << "type=artist&q=" << urlEscape(query) << "&per_page=100";

	fetcher->fetch_html("https://api.discogs.com/database/search", params, json, p_abort);
#ifdef _DEBUG
	log_msg(json);
#endif
	JSONParser jp(json);
	parseArtistResults(jp.root, matches);

	pfc::string8 lowercase_query = lowercase(query);
	pfc::string8 stripped_query = strip_artist_name(query);

	for (size_t i = 0; i < matches.get_size() && i < MAX_ARTISTS; i++) {
		Artist_ptr &artist = matches[i];
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

pfc::array_t<JSONParser_ptr> DiscogsInterface::get_all_pages(pfc::string8 &url, pfc::string8 params, abort_callback &p_abort) {
	
	pfc::array_t<JSONParser_ptr> results;
	if (params.get_length()) {
		params << "&";
	}
	params << "per_page=100";
	size_t page = 1;
	size_t last;

	do {
		pfc::string8 page_params;
		page_params << params;
		page_params << "&page=" << page;

		pfc::string8 json;
		fetcher->fetch_html(url, page_params, json, p_abort);
		JSONParser_ptr jp = pfc::rcnew_t<JSONParser>(json);
		results.append_single(std::move(jp));
		if (page == 1) {
			last = jp->get_object_int("pagination", "pages");
		}
		page++;

	} while (page <= last && !p_abort.is_aborting());

	return results;
}

pfc::array_t<JSONParser_ptr> DiscogsInterface::get_all_pages(pfc::string8 &url, pfc::string8 params, abort_callback &p_abort, const char *msg, threaded_process_status &p_status) {
	
	pfc::array_t<JSONParser_ptr> results;
	if (params.get_length()) {
		params << "&";
	}
	params << "per_page=100";
	size_t page = 1;
	size_t last;
	do {
		pfc::string8 status(msg);
		if (page > 1) {
			status << " page " << page << "/" << last << " (100 x page)";
		}
		p_status.set_item(status);
		
		pfc::string8 page_params;
		page_params << params;
		page_params << "&page=" << page;

		pfc::string8 json;
		fetcher->fetch_html(url, page_params, json, p_abort);
		JSONParser_ptr jp = pfc::rcnew_t<JSONParser>(json);
		results.append_single(std::move(jp));
		if (page == 1) {
			last = jp->get_object_int("pagination", "pages");
		}
		page++;

	} while (page <= last && !p_abort.is_aborting());

	return results;
}

namespace fs =std::filesystem;

pfc::array_t<JSONParser_ptr> DiscogsInterface::get_all_pages_offline_cache(ol::GetFrom getFrom, pfc::string8& id, pfc::string8 &secid, pfc::string8 params, abort_callback& p_abort, const char* msg, threaded_process_status& p_status) {

	pfc::string8 path_container = get_offline_path(id, true, getFrom, secid);
	std::filesystem::path os_path_container = std::filesystem::u8path(path_container.c_str());

	pfc::array_t<JSONParser_ptr> jparsers;

	pfc::string8 status_msg(msg);

	try
	{
		if (fs::exists(os_path_container) && fs::is_directory(os_path_container))
		{
			//alt search recursive and detect root.json files
			std::filesystem::directory_iterator dirpos{ os_path_container };
			size_t msg_pos = 0;
			for (auto walk_dir : dirpos) {

				if (walk_dir.is_directory()) {

					if (p_abort.is_aborting()) break;

					p_status.set_item(status_msg);

					fs::path os_root = fs::u8path(walk_dir.path().string());
					os_root += "\\root.json";
					
					json_t* json_obj = offline_cache.Read_JSON(os_root.string().c_str());

					if (json_obj == nullptr) {
						foo_discogs_exception ex(PFC_string_formatter() << "Invalid cache file:" << os_root.c_str());
						throw ex;
					}

					pfc::string8 json(json_dumps(json_obj, 0));

					JSONParser_ptr jp = pfc::rcnew_t<JSONParser>(json);
					jparsers.append_single(std::move(jp));

					status_msg = msg;
					status_msg << PFC_string_formatter() << " page " << std::to_string(++msg_pos).c_str() << " (100 x page)";
				}
			}
		}
		else {

			//try transient
			return get_all_pages(id, params, p_abort, msg, p_status);
		}
	}
	catch (const fs::filesystem_error& err)
	{
		//..
	}
	catch (const std::exception& ex)
	{
		//..
	}

	return jparsers;
}

void DiscogsInterface::get_entity_offline_cache(ol::GetFrom getfrom, pfc::string8& artist_id, pfc::string8& release_id, pfc::string8& html, abort_callback& p_abort, const char* msg, threaded_process_status& p_status) {

	PFC_ASSERT(getfrom == ol::GetFrom::Artist || getfrom == ol::GetFrom::Release);
	PFC_ASSERT(!(getfrom == ol::GetFrom::Artist && release_id.get_length()));
	PFC_ASSERT(!(getfrom == ol::GetFrom::Release && !release_id.get_length()));

	pfc::string8 status(msg);
	p_status.set_item(status);

	pfc::string8 json_path;
	
	if (getfrom == ol::GetFrom::Artist) {
		json_path = ol::get_offline_path(artist_id, true, ol::GetFrom::Artist, "");
	}
	else {
		json_path = ol::get_offline_path(artist_id, true, ol::GetFrom::Release, release_id);
	}

	json_path << "\\root.json";

	json_t* js_obj = offline_cache.Read_JSON(json_path.get_ptr());
	
	/* prevent crash on invalid offline json files */

	if (js_obj) {

		size_t obj_size = json_object_size(js_obj);

		if (obj_size > 0) {
			html = pfc::string8(json_dumps(js_obj, 0));
		}
	}
}

pfc::string8 DiscogsInterface::get_username(threaded_process_status &p_status, abort_callback &p_abort) {
	if (!username.get_length()) {
		username = load_username(p_status, p_abort);
	}
	return username;
}

pfc::string8 DiscogsInterface::load_username(threaded_process_status &p_status, abort_callback &p_abort) {
	try {
		pfc::string8 status("Loading identity...");
		p_status.set_item(status);

		pfc::string8 json;
		pfc::string8 url;
		url << "https://api.discogs.com/oauth/identity";

		fetcher->fetch_html(url, "", json, p_abort);
		JSONParser jp(json);

		Identity i;
		parseIdentity(jp.root, &i);
		return i.username;
	}
	catch (network_exception &) {
		throw;
	}
}

pfc::array_t<pfc::string8> DiscogsInterface::get_collection(threaded_process_status &p_status, abort_callback &p_abort) {
	if (collection.get_count()) {
		return collection;
	}

	pfc::string8 username = get_username(p_status, p_abort);

	try {
		pfc::string8 json;
		pfc::string8 url;
		url << "https://api.discogs.com/users/" << username << "/collection/folders";		
		fetcher->fetch_html(url, "per_page=100", json, p_abort);
		JSONParser jp(json);

		pfc::array_t<pfc::string8> urls = jp.get_object_array_string("folders", "resource_url");

		url = urls[0];
		url << "/releases";
		pfc::array_t<JSONParser_ptr> pages = get_all_pages(url, "", p_abort, "Loading collection...", p_status);
		
		for (size_t i = 0; i < pages.get_count(); i++) {

			parseCollection(pages[i]->root, collection);
		}
	}
	catch (network_exception &) {
		throw;
	}
	return collection;
}

//pfc::array_t<pfc::string8> DiscogsInterface::load_profile(threaded_process_status& p_status, abort_callback& p_abort) {
//
//	pfc::string8 username = get_username(p_status, p_abort);
//
//	try {
//		pfc::string8 json;
//		pfc::string8 url;
//		url << "https://api.discogs.com/users/" << username;
//		fetcher->fetch_html(url, "", json, p_abort);
//		JSONParser jp(json);
//
//		Profile i;
//		parseProfile(jp.root, &i);
//		
//	}
//	catch (network_exception&) {
//		throw;
//	}
//	return collection;
//}

bool DiscogsInterface::get_thumbnail_from_cache(Release_ptr release, bool isArtist, size_t img_ndx, MemoryBlock& small_art,
	threaded_process_status& p_status, abort_callback& p_abort) {

	bool bres = false;

	pfc::string8 id;
	pfc::string8 url;
	pfc::array_t<Image_ptr>* images;
	size_t local_ndx = img_ndx;

	if (!isArtist) {
		id = release->id;
		if (!release->images.get_size() || (!release->images[img_ndx]->url150.get_length())) {
			
			pfc::string8 msg = "Unable to read thumbnail from cache (no artist), url: ";
			log_msg(msg);

			return false;
		}
		else {
			images = &release->images;
		}
	}
	else {

		Artist_ptr this_artist;

		img_ndx_to_artist(release, img_ndx, this_artist, local_ndx);

		id = this_artist->id;

		if (!release->artists.get_size() || !this_artist.get() ||
			!this_artist->images.get_size() ||
			!this_artist->images[local_ndx]->url150.get_length()) {

			pfc::string8 msg = "Unable to read thumbnail from cache (artist), url: ";
			log_msg(msg);

			return false;
		}
		else {
			images = &this_artist->images;
		}
	}

	size_t artist_offset = isArtist ? release->images.get_count() : 0;

	pfc::array_t<pfc::string8> thumb_cache;
	thumb_cache = ol::get_thumbnail_cache_path_filenames(id, isArtist ? art_src::art : art_src::alb, LVSIL_NORMAL, local_ndx);

	if (thumb_cache.get_count()) {

		p_status.set_item("Fetching artwork preview small album art from cache ...");

		bool bexists = true;
		pfc::string8 file_name = thumb_cache[0];
		extract_native_path(file_name, file_name);

		try {

			std::filesystem::path os_file_name = std::filesystem::u8path(file_name.get_ptr());
			
			if (std::filesystem::exists(os_file_name)) {

				int filesize = std::filesystem::file_size(os_file_name);

				FILE* file;
				errno = 0;

				if (fopen_s(&file, os_file_name.string().c_str(), "rb") == 0) {

					small_art.set_size(filesize);
					size_t done = fread(small_art.get_ptr(), filesize, 1, file);
					bres = done;
					fclose(file);
				}

				if (errno || !bres) {

					pfc::string8 msg("can't open ");
					msg << file_name;
					log_msg(msg);
					return false;
				}
			}
			else {
				return false;
			}
		}
		catch (foo_discogs_exception& e) {

			throw(e);
			return false;
		}
	}
	return bres;
}

bool DiscogsInterface::delete_artist_cache(const pfc::string8& artist_id) {

	Artist_ptr artist = get_artist_from_cache(artist_id);

	if (artist.get()) {

		// cache memory

		bool bdeleted = cache_artists->remove(artist_id);

		for (size_t walk = 0; walk < artist->master_releases.get_count(); walk++) {
			
			/* rev  */
			
			LPARAM lkey = MAKELPARAM(atoi(artist->master_releases[walk]->id), atoi(artist->id));

			//unsigned long lkey = encode_mr(artist->search_role_list_pos, artist->master_releases[walk]->id);

			/* rev */

			bdeleted &= cache_master_releases->remove(lkey/*artist->master_releases[walk]->id*/);
		}

		for (size_t walk = 0; walk < artist->releases.get_count(); walk++) {

			unsigned long lkey = encode_mr(artist->search_role_list_pos, artist->releases[walk]->id);
			bdeleted &= cache_releases->remove(lkey);
		}
		
		// disk cache

		pfc::string8 parent_path = ol::get_offline_path(artist_id, true, ol::GetFrom::Artist, "");
		std::filesystem::path os_path = std::filesystem::u8path(parent_path.c_str());

		os_path = os_path.parent_path();

		if (fs::exists(os_path)) {

			std::error_code ec;
			std::filesystem::remove_all(os_path, ec);
			bdeleted &= !ec.value();
			
			return !ec.value();
		}		
	}
	return false;
}