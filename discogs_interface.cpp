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

Artist_ptr DiscogsInterface::get_artist(const pfc::string8 &artist_id, bool _load_releases, threaded_process_status &p_status, abort_callback &p_abort, bool bypass_cache, bool throw_all) {
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
			throw;
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

pfc::array_t<JSONParser_ptr> DiscogsInterface::get_all_pages_offline_cache(ol::GetFrom getFrom, pfc::string8& id, pfc::string8 &secid, pfc::string8 params, abort_callback& p_abort, const char* msg, threaded_process_status& p_status) {
	
	pfc::array_t<JSONParser_ptr> results;
	size_t page = 1;

	pfc::string8 foo_path;
	pfc::string8 pages_path_prefix;

	if (getFrom == ol::GetFrom::ArtistReleases) {
		foo_path = ol::GetOfflinePath(id, true, getFrom, "");
		pages_path_prefix = ol::GetOfflinePagesPath(id, pfc_infinite, true, getFrom, "");
	}
	else if (getFrom == ol::GetFrom::Versions) {
		foo_path = ol::GetOfflinePath(id, true, getFrom, secid);
		pages_path_prefix = ol::GetOfflinePagesPath(id, pfc_infinite, true, getFrom, secid);
	}
	else {
		foo_path = "";
		pages_path_prefix = "";
		PFC_ASSERT(getFrom == ol::GetFrom::ArtistReleases || getFrom == ol::GetFrom::Versions);
	}

	pfc::array_t<pfc::string8> page_paths = ol::GetFSPagesFilePaths(id, getFrom, secid);
	if (!page_paths.get_count()) {
		//empty offline data, type transient
		return get_all_pages(id, params, p_abort, msg, p_status);
	}

	size_t last = page_paths.get_count();
	if (last == 0) return results;

	do {
		pfc::string8 status(msg);

		if (page > 1) {
			status << " page " << page << "/" << last << " (100 x page)";
		}
		p_status.set_item(status);

		pfc::string8 json_page_path;
		json_page_path << pages_path_prefix << page - 1 << "\\root.json";

		json_t* j_t = offline_cache_artists->FReadAll(json_page_path.get_ptr());
		if (j_t == nullptr) {
			foo_discogs_exception ex("Invalid root.json file");
			throw ex;
		}

		size_t obj_size = json_object_size(j_t);
		pfc::string8 json(json_dumps(j_t, 0));

		JSONParser_ptr jp = pfc::rcnew_t<JSONParser>(json);
		results.append_single(std::move(jp));
		page++;

	} while (page <= last && !p_abort.is_aborting());

	return results;
}

//todo: unify next two
void DiscogsInterface::get_artist_offline_cache(pfc::string8& id, pfc::string8& html, abort_callback& p_abort, const char* msg, threaded_process_status& p_status) {

	pfc::string8 foo_path(ol::GetOfflinePath(id, true, ol::GetFrom::Artist, ""));

	pfc::string8 status(msg);
	p_status.set_item(status);

	pfc::string8 json_path;
	json_path << foo_path << "\\root.json";

	json_t* j_t = offline_cache_artists->FReadAll(json_path.get_ptr());
	/* prevent crash on faulty offline json files */
	/* todo: delete corrupted file*/
	if (j_t) {
		pfc::array_t<t_uint8> buffer;
		size_t obj_size = json_object_size(j_t);

		if (obj_size > 0) {
			html = pfc::string8(json_dumps(j_t, 0));
		}
	}
}

void DiscogsInterface::get_release_offline_cache(pfc::string8& id, pfc::string8 &secid, pfc::string8& html, abort_callback& p_abort, const char* msg, threaded_process_status& p_status) {

	pfc::string8 foo_path(ol::GetOfflinePath(id, true, ol::GetFrom::Release, secid));

	pfc::string8 status(msg);
	p_status.set_item(status);

	pfc::string8 json_path;
	json_path << foo_path << "\\root.json";

	json_t* j_t = offline_cache_release->FReadAll(json_path.get_ptr());

	size_t obj_size = json_object_size(j_t);
	if (obj_size) {
		pfc::string8 json(json_dumps(j_t, 0));
		html = json.get_ptr();
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

		const size_t count = pages.get_count();
		for (size_t i = 0; i < count; i++) {
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
	if (!isArtist) {
		id = release->id;
		if (!release->images.get_size() || (!release->images[img_ndx]->url150.get_length())) {
#ifdef DEBUG		
			pfc::string8 msg = "Unable to read thumbnail from cache (no artist);
			//msg << release->images[img_ndx]->url150;
			log_msg(msg);
#endif			
			return false;
		}
		else {
			images = &release->images;
		}
	}
	else {
		id = release->artists[0]->full_artist->id;
		if (!release->artists.get_size() || !release->artists[0]->full_artist->images.get_size() ||
			(!release->artists[0]->full_artist->images[img_ndx]->url150.get_length())) {
#ifdef DEBUG
			pfc::string8 msg = "Unable to read thumbnail from cache (artist)";
			//msg << release->images[img_ndx]->url150;
			log_msg(msg);
#endif			
			return false;
		}
		else {
			images = &release->artists[0]->full_artist->images;
		}
	}
	size_t artist_offset = isArtist ? release->images.get_count() : 0;
	pfc::array_t<pfc::string8> thumb_cache;
	thumb_cache = Offline::get_thumbnail_cache_path_filenames(id, isArtist ? art_src::art : art_src::alb,
		LVSIL_NORMAL, img_ndx/* + artist_offset*/);

	if (thumb_cache.get_count()) {

		p_status.set_item("Fetching artwork preview small album art from cache ...");

		try {

			bool bexists = true;

			pfc::string8 file_name = thumb_cache[0];
			extract_native_path(file_name, file_name);
		
			std::filesystem::path os_file_name = std::filesystem::u8path(file_name.get_ptr());
			
			int filesize = -1;
			if (std::filesystem::exists(os_file_name)) {
				filesize = std::filesystem::file_size(os_file_name);
			}
			else {
				return false;
			}

			FILE* fd = nullptr;
			if (fopen_s(&fd, os_file_name.string().c_str(), "rb") != 0) {
				pfc::string8 msg("can't open ");
				msg << file_name;
				log_msg(msg);
				return false;
			}

			small_art.set_size(filesize);
			size_t done = fread(small_art.get_ptr(), filesize, 1, fd);
			bres = done;

			fclose(fd);
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
	if (artist) {
		cache_artists->remove(artist_id);
		for (size_t walk = 0; walk < artist->master_releases.get_count(); walk++) {
			unsigned long lkey = encode_mr(artist->search_role_list_pos, artist->master_releases[walk]->id);
			cache_master_releases->remove(lkey/*artist->master_releases[walk]->id*/);
		}

		for (size_t walk = 0; walk < artist->releases.get_count(); walk++) {
			unsigned long lkey = encode_mr(artist->search_role_list_pos, artist->releases[walk]->id);
			cache_releases->remove(lkey);
		}
			

		pfc::string8 ol_artist_path = Offline::GetOfflinePath(artist_id, true, Offline::GetFrom::Artist, "");
		ol_artist_path.truncate(ol_artist_path.length() - pfc::string8("\\artist").length());
		
		std::filesystem::path fspath = std::filesystem::u8path(ol_artist_path.c_str());

		try {
			std::filesystem::remove_all(fspath);
		}
		catch (...) {
			return false;
		}

		return true;
	}
	return false;
}