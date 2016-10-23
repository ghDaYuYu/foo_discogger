#include "stdafx.h"

#include "discogs_interface.h"
#include "exception.h"
#include "utils.h"
#include "json_helpers.h"


Release_ptr DiscogsInterface::get_release(const pfc::string8 &release_id, bool bypass_cache) {
	assert_release_id_not_deleted(release_id);
	Release_ptr release = bypass_cache ? nullptr : get_release_from_cache(release_id);
	if (!release) {
		release = std::make_shared<Release>(release_id);
		if (!bypass_cache) {
			add_release_to_cache(release);
		}
	}
	return release;
}

Release_ptr DiscogsInterface::get_release(const pfc::string8 &release_id, threaded_process_status &p_status, abort_callback &p_abort, bool bypass_cache, bool throw_all) {
	assert_release_id_not_deleted(release_id);
	Release_ptr release = bypass_cache ? nullptr : get_release_from_cache(release_id);
	if (!release) {
		release = std::make_shared<Release>(release_id);
		if (!bypass_cache) {
			add_release_to_cache(release);
		}
	}
	if (!release->loaded) {
		try {
			release->load(p_status, p_abort, throw_all);
		}
		catch (http_404_exception) {
			add_deleted_release_id(release_id);
			throw;
		}
	}
	return release;
}

MasterRelease_ptr DiscogsInterface::get_master_release(const pfc::string8 &master_id, bool bypass_cache) {
	MasterRelease_ptr master = bypass_cache ? nullptr : get_master_release_from_cache(master_id);
	if (!master) {
		master = std::make_shared<MasterRelease>(master_id);
		if (!bypass_cache) {
			add_master_release_to_cache(master);
		}
	}
	return master;
}

MasterRelease_ptr DiscogsInterface::get_master_release(const pfc::string8 &master_id, threaded_process_status &p_status, abort_callback &p_abort, bool bypass_cache, bool throw_all) {
	MasterRelease_ptr master = bypass_cache ? nullptr : get_master_release_from_cache(master_id);
	if (!master) {
		master = std::make_shared<MasterRelease>(master_id);
		if (!bypass_cache) {
			add_master_release_to_cache(master);
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
		artist->load_releases(p_status, p_abort);
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

	JSONParser jp(json);

	parseArtistResults(jp.root, matches);

	pfc::string8 lowercase_query = lowercase(query);
	pfc::string8 stripped_query = strip_artist_name(query);

	for (size_t i = 0; i < matches.get_size(); i++) {
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
			status << " page " << page << "/" << last;
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


pfc::string8 DiscogsInterface::get_username(abort_callback &p_abort) {
	try {
		pfc::string8 json;
		pfc::string8 url;
		url << "https://api.discogs.com/oauth/identity";
		fetcher->fetch_html(url, "", json, p_abort);

		JSONParser jp(json);

		Identity i;
		parseIdentity(jp.root, &i);
		return i.username;
	}
	catch (network_exception &e) {
		throw;
	}
}


pfc::array_t<pfc::string8> DiscogsInterface::get_collection(threaded_process_status &p_status, abort_callback &p_abort) {
	if (collection.get_count()) {
		return collection;
	}

	pfc::string8 username = get_username(p_abort);

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
	catch (network_exception &e) {
		throw;
	}
	return collection;
}
