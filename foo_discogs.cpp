#include "stdafx.h"
#include "libPPUI/gdiplus_helpers.h"
#include "utils.h"
#include "tags.h"
#include "multiformat.h"

#include "track_matching_dialog.h"
#include "preview_dialog.h"
#include "preview_leading_tag_dialog.h"
#include "tag_mappings_dialog.h"

#include "configuration_dialog.h"
#include "find_release_dialog.h"
#include "find_release_artist_dlg.h"
#include "track_matching_utils.h"

#include "helpers/ui_element_helpers.h"

#include "foo_discogs.h"

using namespace std;

HICON g_icon = nullptr;

static const char* ERROR_IMAGE_NUMBER_REQUIRED = "Cannot save multiple images with the same name. Use %IMAGE_NUMBER% when forming image names.";

class initquit_discogs : public initquit
{
	virtual void on_init() override {

		console::print("Loading foo_discogger");

		if (!CONF.load()) {

			console::print("foo_discogger configuration error");

			uMessageBox(core_api::get_main_window(),
				"Error loading configuration.",
				"foo_discogger initialization", MB_APPLMODAL | MB_ICONASTERISK);

			return;
		}

		for (auto & entry :  cfg_tag_mappings) {

			bool release_id_mod = STR_EQUAL(TAG_RELEASE_ID, entry.tag_name.get_ptr());
			if (release_id_mod) {
				bool alt_enabled = CONF.awt_get_alt_mode();
				//fix may be required
				bool bchanged = CONF.awt_set_alt_mode(alt_enabled);
				int current_alt = HIWORD(CONF.alt_write_flags);

				entry.enable_write = current_alt & (1 << 0);
				entry.enable_update = current_alt & (1 << 1);
			}


			entry.is_multival_meta = is_multivalue_meta(entry.tag_name);
		
		}
		init_tag_mappings();
		discogs_interface = new DiscogsInterface();

		g_discogs = new foo_discogs();
		g_os_is_wine = check_os_wine();
		load_dlls();
	}
	virtual void on_quit() override {
		console::print("Quitting");
		if (g_discogs) {
			DeleteObject(g_discogs->icon);
			delete g_discogs; //(1)
		}
		else {
			log_msg("Warning: skipping default on_quit clearance.");
		}

		delete discogs_interface; //(2)

		unload_dlls();
	}
};

static initquit_factory_t<initquit_discogs> foo_initquit;

foo_discogs::foo_discogs() {

	auto hInst = core_api::get_my_instance();
	auto dbg_micon = GetSystemMetrics(SM_CXSMICON);
	icon = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_DC), IMAGE_ICON, GetSystemMetrics( SM_CXSMICON ), GetSystemMetrics( SM_CYSMICON ), 0);
	//auto dpiX = QueryScreenDPIEx(core_api::get_main_window()).cx;
	//icon = GdiplusLoadPNGIcon(IDB_PNG_DC_32C, CSize(32, 32));

	discogs_interface->fetcher->set_oauth(CONF.oauth_token, CONF.oauth_token_secret);

	static_api_ptr_t<titleformat_compiler>()->compile_force(release_name_script, "[%album artist%] - [%album%]");
	static_api_ptr_t<titleformat_compiler>()->compile_force(dc_artist_id_script, (PFC_string_formatter() << "[%" << TAG_ARTIST_ID << "%]").c_str());
	static_api_ptr_t<titleformat_compiler>()->compile_force(dc_release_id_script, (PFC_string_formatter() << "[%" << TAG_RELEASE_ID << "%]").c_str());

	gave_oauth_warning = false;

	bool bv2 = core_version_info_v2::get()->test_version(2, 0, 0, 0);
	if (bv2) {
		ui_v2_cfg_callback = new ui_v2_config_callback(this);
		ui_config_manager::get()->add_callback(ui_v2_cfg_callback);
	}
	else {
		//..
	}
}

foo_discogs::~foo_discogs() {

	if (find_release_dialog)  {
		find_release_dialog->destroy();
	}

	if (find_release_artist_dialog)
		find_release_artist_dialog->DestroyWindow();
	
	if (preview_modal_tag_dialog) {
		preview_modal_tag_dialog->destroy();
	}

	if (preview_tags_dialog) {
		preview_tags_dialog->destroy();
	}

	if (track_matching_dialog) {
		track_matching_dialog->destroy();
	}

	if (tag_mappings_dialog) {
		tag_mappings_dialog->destroy();
	}

	bool bv2 = core_version_info_v2::get()->test_version(2, 0, 0, 0);
	if (bv2) {
		ui_config_manager::get()->remove_callback(ui_v2_cfg_callback);
		delete ui_v2_cfg_callback;
	}

	DeleteObject(g_hIcon_quian);
	DeleteObject(g_hIcon_rec);
	DeleteObject(g_hFont);
	DeleteObject(g_hFontTabs);

}

void foo_discogs::item_display_web_page(const metadb_handle_ptr item, discog_web_page web_page) {
	file_info_impl finfo;

	char *url_prefix;
	char *url_postfix = "";
	const char *tag;
	char *backup_tag = nullptr;
	switch (web_page) {
		case ARTIST_PAGE:
			tag = TAG_ARTIST_ID;
			backup_tag = "DISCOGS_ARTIST_LINK";
			url_prefix = "https://www.discogs.com/artist/";
			break;
		case ARTIST_ART_PAGE:
			tag = TAG_ARTIST_ID;
			backup_tag = "DISCOGS_ARTIST_LINK";
			url_prefix = "https://www.discogs.com/artist/";
			url_postfix = "/images";
			break;
		case RELEASE_PAGE:
			tag = TAG_RELEASE_ID;
			url_prefix = "https://www.discogs.com/x/release/";
			break;
		case LABEL_PAGE:
			tag = TAG_LABEL_ID;
			backup_tag = "DISCOGS_LABEL_LINK";
			url_prefix = "https://www.discogs.com/label/";
			break;
		case ALBUM_ART_PAGE:
			tag = TAG_RELEASE_ID;
			url_prefix = "https://www.discogs.com/release/";
			url_postfix = "/images";
			break;
		case MASTER_RELEASE_PAGE:
			tag = TAG_MASTER_RELEASE_ID;
			url_prefix = "https://www.discogs.com/master/";
			break;
		default:
			return;
	}

	try {
		pfc::string8 tag_value;
		pfc::string8 url;
		item->get_info(finfo);
		if (file_info_get_tag(item, finfo, tag, tag_value, backup_tag)) {
			url << url_prefix << tag_value << url_postfix;
			display_url(url);
		}

	catch (foo_discogs_exception &e) {
		add_error(e);
		display_errors();
		clear_errors();
	}
}

bool foo_discogs::file_info_get_tag(const metadb_handle_ptr item, file_info &finfo, const char* tag, pfc::string8 &value, const char *backup_tag) {
	const char *field = finfo.meta_get(tag, 0);
	if (field != nullptr) {
		value = field;
		return true;
	}
	else if (backup_tag != nullptr) {
		field = finfo.meta_get(backup_tag, 0);
		if (field != nullptr) {
			value = field;
			return true;
		}
	}
	return false;
}

bool foo_discogs::item_has_tag(const metadb_handle_ptr item, const char *tag, const char *backup_tag) {
	bool has_tag;
	file_info_impl file_info;
	pfc::string8 tmp;

	item->get_info(file_info);
	has_tag = file_info_get_tag(item, file_info, tag, tmp, backup_tag);

	return has_tag;
}

bool foo_discogs::some_item_has_tag(const metadb_handle_list items, const char*tag) {
	for (size_t i = 0; i < items.get_count(); i++) {
		if (item_has_tag(items.get_item(i), tag)) {
			return true;
		}
	}
	return false;
}

const ReleaseDisc_ptr& foo_discogs::get_discogs_disc(const Release_ptr &release, size_t index, size_t &disc_track_index) {
	for (size_t i = 0; i < release->discs.get_size(); i++) {
		if (index < release->discs[i]->tracks.get_size()) {
			disc_track_index = index;
			return release->discs[i];
		}
		index -= release->discs[i]->tracks.get_size();
	}
	throw foo_discogs_exception("Unable to map index to Discogs tracklist.");
}

typedef std::map<pfc::string8, MemoryBlock>::iterator done_fetches_it;

void foo_discogs::save_album_art(Release_ptr& release, metadb_handle_ptr item,
	art_download_attribs ada, pfc::array_t<GUID> my_album_art_ids, bit_array_bittable& saved_mask,
	pfc::array_t<pfc::string8>& done_files, std::map<pfc::string8, MemoryBlock>& done_fetches, threaded_process_status& p_status, abort_callback& p_abort) {
	if (!release->images.get_size()) {
		return;
	}
	static const GUID undef_guid = { 0xCDCDCDCD, 0xCDCD, 0xCDCD, { 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD } };
	static const GUID zero_guid = { 0, 0, 0, 0 };

	pfc::string8 formatted_release_name;
	item->format_title(nullptr, formatted_release_name, g_discogs->release_name_script, nullptr);

	char progress_text[512];
	sprintf_s(progress_text, "%s (fetching album art file list)", formatted_release_name.get_ptr());
	p_status.set_item(progress_text);

	pfc::string8 directory;
	file_info_impl info;
	titleformat_hook_impl_multiformat hook(p_status, &release);

	if (ada.write_it) {

		CONF.album_art_directory_string->run_hook(item->get_location(), &info, &hook, directory, nullptr);

		// hardcoded "nullptr" ?
		if (!STR_EQUAL(directory, "nullptr")) {
			if (directory[directory.get_length() - 1] != '\\') {
				directory.add_char('\\');
			}
			ensure_directory_exists(directory);
		}
		else {
			ada.write_it = false;
		}
	}
	
	if (!ada.write_it && !ada.embed_it) {
		log_msg("missing data... skipping album art writing or embedding");
		return;
	}

	size_t cimage_batch = 0;
	size_t offset = 0; //todo: unify album and artist routines
	pfc::string8 last_path = "";

	std::vector<bool> vwrite_it;
	std::vector<bool> voverwrite_it;
	std::vector<bool> vembed_it;

	if (!ada.custom_save_or_embed) {

		cimage_batch	= ada.fetch_all_it ? release->images.get_size() : 1;

		vwrite_it		= std::vector<bool>(cimage_batch, ada.write_it);
		voverwrite_it	= std::vector<bool>(cimage_batch, ada.overwrite_it);
		vembed_it		= std::vector<bool>(cimage_batch, ada.embed_it);
	}
	else {

		cimage_batch = release->images.get_count();
		for (size_t wib = 0; wib < cimage_batch; wib++) {

			vwrite_it.resize(cimage_batch);
			voverwrite_it.resize(cimage_batch);
			vembed_it.resize(cimage_batch);
			vwrite_it[wib]		= CONF_MULTI_ARTWORK.getflag(af::alb_sd, wib /*+ offset*/);
			voverwrite_it[wib]	= CONF_MULTI_ARTWORK.getflag(af::alb_ovr, wib /*+ offset*/);
			vembed_it[wib]		= CONF_MULTI_ARTWORK.getflag(af::alb_emb, wib /*+ offset*/);
		}
	}

	std::vector<GUID> vembeded_guids;

	for (size_t i = 0; i < cimage_batch; i++) {
		pfc::string8 path = directory;
		bool write_this = vwrite_it[i];
		bool embed_req = true; //only embed first album/artist image on non custom jobs

		if (write_this) {

			//skip file match zero guids
			if (i + offset >= my_album_art_ids.get_count() || my_album_art_ids[i + offset] == zero_guid) {
				continue;
			}

			pfc::string8 file;
			hook.set_custom("IMAGE_NUMBER", (unsigned)(i + 1));
			hook.set_image(&release->images[i]);

			if (my_album_art_ids.size() > i + offset) {
				const char* art_id_name = album_art_ids::name_of(my_album_art_ids[i + offset]);
				size_t postfix = 0;
				if (art_id_name == nullptr) {

					embed_req = false;

					art_id_name = template_art_ids::name_of(my_album_art_ids[i + offset]);
					if (art_id_name != nullptr) {
						for (size_t walk_vw = 0; walk_vw < vwrite_it.size(); walk_vw++) {
							if (vwrite_it[walk_vw]) {
								if (walk_vw == i /*+ offset*/) break;
								if (pfc::guid_equal(my_album_art_ids[walk_vw + offset], my_album_art_ids[i + offset]))
									postfix++;
							}
						}
					}
				}

				if (art_id_name == nullptr) {
					file = "";
				}
				else {
					file << art_id_name;
					if (postfix > 0)
						file << "_" << postfix;
				}

				pfc::chain_list_v2_t<pfc::string8> splitfile;
				pfc::splitStringByChar(splitfile, file.get_ptr(), ' ');
				if (splitfile.get_count()) file = *splitfile.first();
			}
			else {
				log_msg("Invalid index found generating artist artwork filename");
			}

			if (!file.get_length() || !ada.file_match) {
				if (!file.get_length() && !ada.file_match) {
					embed_req = true; //req is ok, vembed_it[i] will decide
				}
				CONF.album_art_filename_string->run_hook(item->get_location(), &info, &hook, file, nullptr);
			}

			if (STR_EQUAL(file, "nullptr") || STR_EQUAL(file, "")) {
				log_msg("empty file path... skipping album art writing or embedding");
				if (ada.to_path_only) ada.vpaths.emplace_back("empty file path... skipping album art writing or embedding");
				continue;
			}
			
			makeFsCompliant(file);
			path += file;
			path += ".jpg";

			if (STR_EQUAL(path, last_path)) {
				foo_discogs_exception ex;
				ex << ERROR_IMAGE_NUMBER_REQUIRED;
				throw ex;
			}
			for (size_t walk_done = 0; walk_done < done_files.get_count(); walk_done++) {
				if (done_files[walk_done].equals(path.get_ptr())) {
					write_this = false;
					break;
				}
			}
			if (write_this) {
				done_files.append_single_val(path);
			}
		}

		if (!write_this && (!vembed_it[i] || !embed_req)) {
			continue;
		}

		if (ada.to_path_only) {
			continue;
		}

		MemoryBlock buffer;

		if (write_this) {

			if (voverwrite_it[i] || !foobar2000_io::filesystem::g_exists(path, p_abort)) {
				buffer = done_fetches[release->images[i]->url];
				if (!buffer.get_ptr()) {					
					auto& mbmi = done_fetches.emplace(release->images[i]->url, buffer);
					g_discogs->fetch_image(mbmi.first->second, release->images[i], p_abort);
					buffer = mbmi.first->second;
				}			
				g_discogs->write_image(buffer, path, p_abort);

				saved_mask.set(i, true);
			}
			last_path = path;
		}

		//embeds

		if (vembed_it[i] && embed_req) {

			if (my_album_art_ids.size() > i + offset) {

				GUID this_guid = my_album_art_ids[i + offset];
				
				this_guid = IsEqualGUID(this_guid, undef_guid) ? album_art_ids::cover_front : this_guid;

				auto guid_it = std::find_if(vembeded_guids.begin(), vembeded_guids.end(), [=](const auto &w) {
					return IsEqualGUID(w, this_guid);
					});

				if (guid_it != std::end(vembeded_guids)) continue;

				if (!buffer.get_count()) {

					buffer = done_fetches[release->images[i]->url];

					if (!buffer.get_ptr()) {
						auto &mbmi = done_fetches.emplace(release->images[i]->url, buffer);
						g_discogs->fetch_image(mbmi.first->second, release->images[i], p_abort);
						buffer = mbmi.first->second;
					}
				}

				if (this_guid == album_art_ids::icon) {

					MemoryBlock iconbuffer = MemoryBlockToPngIcon(buffer);
					g_discogs->embed_image(iconbuffer, item, this_guid, p_abort);
				}
				else {
					if (buffer.get_ptr()) {
						g_discogs->embed_image(buffer, item, this_guid, p_abort);
					}
				}
				vembeded_guids.emplace_back(this_guid);
			}
			else {
				log_msg("Invalid index found embeding artist artwork");
			}
		}
	}
}

void foo_discogs::save_artist_art(Release_ptr &release, metadb_handle_ptr item,
	art_download_attribs ada, pfc::array_t<GUID> my_album_art_ids, bit_array_bittable& saved_mask,
	pfc::array_t<pfc::string8> &done_files, std::map<pfc::string8, MemoryBlock>& done_fetches, threaded_process_status &p_status, abort_callback &p_abort) {

	// ensure release is loaded
	release->load(p_status, p_abort);

	// get artist IDs from titleformat string

	size_t lkey = encode_mr(0, atol(release->master_id));

	MasterRelease_ptr master = discogs_interface->get_master_release(lkey/*release->master_id*/);

	file_info_impl info;
	item->get_info(info);
	titleformat_hook_impl_multiformat hook(p_status, &master, &release);

	pfc::string8 str;
	pfc::array_t<int> ids;
	try {
		formatting_script artists_format_string = "%<ARTISTS_ID>%";
		artists_format_string->run_hook(item->get_location(), (file_info*)&info, &hook, str, nullptr);
		if (STR_EQUAL(str, "?")) str = release->artists[0]->full_artist->id;
		string_encoded_array result(str);
		if (result.has_array()) {
			for (size_t walk_res = 0; walk_res < result.get_width(); walk_res++) {
				const pfc::string8 &n = result.get_citem(walk_res).get_pure_cvalue();
				int num = std::stoi(n.get_ptr());
				for (size_t j = 0; j < ids.get_size(); j++) {
					if (ids[j] == num) {
						continue;
					}
				}
				ids.append_single(num);
			}
		}
		else {
			ids.append_single(std::stoi(result.get_pure_cvalue().get_ptr()));
		}

		pfc::array_t<Artist_ptr>artists;
		for (size_t walk_id = 0; walk_id < ids.get_size(); walk_id++) {
			pfc::string8 artist_id;
			artist_id << ids[walk_id];
			Artist_ptr this_artist = discogs_interface->get_artist(artist_id);
			artists.append_single(this_artist);
		}

		if (ada.write_it || ada.embed_it) {
				save_artist_art(artists, release, item, ada, my_album_art_ids, saved_mask, done_files, done_fetches, p_status, p_abort);
		}
	}
	catch (foo_discogs_exception &e) {
		foo_discogs_exception ex;
		ex << "Error writing artist art " << " [" << e.what() << "]";
		throw ex;
	}
}


void foo_discogs::save_artist_art(pfc::array_t<Artist_ptr>& artists, Release_ptr &release, metadb_handle_ptr item,
	art_download_attribs ada, pfc::array_t<GUID> my_album_art_ids, bit_array_bittable& saved_mask,
	pfc::array_t<pfc::string8> &done_files, std::map<pfc::string8, MemoryBlock>& done_fetches, threaded_process_status &p_status, abort_callback &p_abort) {

	size_t cartist_art = 0;

	for (auto wra : artists) {
		wra->load(p_status, p_abort);
		cartist_art += wra->images.get_count();
	}

	if (!cartist_art) {
		return;
	}

	static const GUID undef_guid = { 0xCDCDCDCD, 0xCDCD, 0xCDCD, { 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD } };
	static const GUID zero_guid = { 0, 0, 0, 0 };

	size_t lkey = encode_mr(0, atol(release->master_id));
	MasterRelease_ptr master = discogs_interface->get_master_release(lkey/*release->master_id*/);

	pfc::string8 directory;
	file_info_impl info;
	titleformat_hook_impl_multiformat hook(p_status, &master, &release, &artists[0]);

	if (ada.write_it) {

		CONF.artist_art_directory_string->run_hook(item->get_location(), &info, &hook, directory, nullptr);

		// hardcoded "nullptr" ?
		if (STR_EQUAL(directory, "nullptr")) {
			ada.write_it = false;
		}
	}

	if (!ada.write_it && !ada.embed_it) {
		log_msg("missing data... skipping artist art writing or embedding");
		return;
	}

	size_t cimage_batch = 0;
	size_t offset = release->images.get_size();
	pfc::string8 last_path = "";

	std::vector<bool> vwrite_it;
	std::vector<bool> voverwrite_it;
	std::vector<bool> vembed_it;

	if (!ada.custom_save_or_embed) {

		cimage_batch = ada.fetch_all_it ? cartist_art : 1;

		vwrite_it = std::vector<bool>(cimage_batch, ada.write_it);
		voverwrite_it = std::vector<bool>(cimage_batch, ada.overwrite_it);
		vembed_it = std::vector<bool>(cimage_batch, ada.embed_it);
	}
	else {

		cimage_batch = cartist_art;
		for (size_t wib = 0; wib < cimage_batch; wib++) {

			vwrite_it.resize(cimage_batch);
			voverwrite_it.resize(cimage_batch);
			vembed_it.resize(cimage_batch);
			vwrite_it[wib] = CONF_MULTI_ARTWORK.getflag(af::art_sd, wib /*+ offset*/);
			voverwrite_it[wib] = CONF_MULTI_ARTWORK.getflag(af::art_ovr, wib /*+ offset*/);
			vembed_it[wib] = CONF_MULTI_ARTWORK.getflag(af::art_emb, wib /*+ offset*/);
		}
	}

	bool b_multiple_artist_head = false;

	std::vector<GUID> vembeded_guids;

	Artist_ptr artist;

	for (size_t i = 0; i < cimage_batch; i++) {

		Artist_ptr artist;
		size_t artist_img_ndx = ~0;
		discogs_interface->img_artists_ndx_to_artist(release, i, artist, artist_img_ndx);

		hook.set_artist(&artist);
		CONF.artist_art_directory_string->run_hook(item->get_location(), &info, &hook, directory, nullptr);

		if (vwrite_it[i] && !(STR_EQUAL(directory, "nullptr"))) {
			if (directory[directory.get_length() - 1] != '\\') {
				directory.add_char('\\');
			}
			ensure_directory_exists(directory);
		}
		else {
			vwrite_it[i] = false;
		}

		if (i == 0) {
			pfc::string8 tmp_str, tmp_file;
			CONF.artist_art_filename_string->run_hook(item->get_location(), &info, &hook, tmp_file, nullptr);
			tmp_str = extract_max_number(tmp_file, 'z');
			b_multiple_artist_head = STR_EQUAL(tmp_str, release->artists[0]->full_artist->id);
		}

		pfc::string8 path = directory;
		bool write_this = vwrite_it[i];
		bool embed_req = true & b_multiple_artist_head;

		if (write_this) {
			if (i + offset >= my_album_art_ids.get_count() || my_album_art_ids[i + offset] == zero_guid) {
				continue;
			}

			pfc::string8 file;
			hook.set_custom("IMAGE_NUMBER", (unsigned)(artist_img_ndx + 1));
			hook.set_image(&artist->images[artist_img_ndx]);

			if (my_album_art_ids.size() > i + offset) {
				const char* art_id_name = album_art_ids::name_of(my_album_art_ids[i + offset]);
				
				size_t postfix = 0;

				if (art_id_name == nullptr) {

					embed_req = false;

					art_id_name = template_art_ids::name_of(my_album_art_ids[i + offset]);
					if (art_id_name != nullptr) {
						for (size_t walk_vw = 0; walk_vw < vwrite_it.size(); walk_vw++) {
							if (vwrite_it[walk_vw]) {
								if (walk_vw == i /*+ album_offset*/) break;
								if (pfc::guid_equal(my_album_art_ids[walk_vw + offset], my_album_art_ids[i + offset]))
									postfix++;
							}
						}
					}
				}

				if (art_id_name == nullptr) {
					file = "";
				}
				else {
					file << art_id_name;
					if (postfix > 0)
						file << "_" << postfix;
				}

				//--

				pfc::chain_list_v2_t<pfc::string8> splitfile;
				pfc::splitStringByChar(splitfile, file.get_ptr(), ' ');
				if (splitfile.get_count()) file = *splitfile.first();
			}
			else {
				log_msg("Invalid index found generating artist artwork filename");
			}
			if (!file.get_length() || !ada.file_match || !b_multiple_artist_head) {
				if (!file.get_length() && !ada.file_match) {
					embed_req = true; //req is ok, vembed_it[i] will decide
				}
				CONF.artist_art_filename_string->run_hook(item->get_location(), &info, &hook, file, nullptr);
			}

			if (STR_EQUAL(file, "nullptr") || STR_EQUAL(file, "")) {
				log_msg("empty file path... skipping artist art writing or embedding");
				if (ada.to_path_only) ada.vpaths.emplace_back("empty file path... skipping artist art writing or embedding");
				continue;
			}

			makeFsCompliant(file);
			path += file;
			path += ".jpg";

			if (STR_EQUAL(path, last_path)) {
				foo_discogs_exception ex;
				ex << ERROR_IMAGE_NUMBER_REQUIRED;
				throw ex;
			}

			for (size_t walk_done = 0; walk_done < done_files.get_count(); walk_done++) {
				if (done_files[walk_done].equals(path)) {
					write_this = false;
					break;
				}
			}
			if (write_this) {
				done_files.append_single_val(path);
			}
		}

		if (!write_this && (!vembed_it[i] || !embed_req)) {
			continue;
		}

		if (ada.to_path_only) {
			continue;
		}

		MemoryBlock buffer;

		if (write_this) {

			if (voverwrite_it[i] || !foobar2000_io::filesystem::filesystem::g_exists(path, p_abort)) {
				
				buffer = done_fetches[artist->images[artist_img_ndx]->url];
				if (!buffer.get_ptr()) {
					auto& mbmi = done_fetches.emplace(artist->images[artist_img_ndx]->url, buffer);
					g_discogs->fetch_image(mbmi.first->second, artist->images[artist_img_ndx], p_abort);
					buffer = mbmi.first->second;
				}

				g_discogs->write_image(buffer, path, p_abort);
				saved_mask.set(i + offset, true);
			}
			last_path = path;
		}

		//embeds do not check overwrites (always writes)

		if (vembed_it[i] && embed_req) {

			if (my_album_art_ids.size() > i + offset) {
			
				GUID this_guid = my_album_art_ids[i + offset];
				//default to artist
				this_guid = IsEqualGUID(this_guid, undef_guid) ? album_art_ids::artist : this_guid;

				auto guid_it = std::find_if(vembeded_guids.begin(), vembeded_guids.end(), [=](const auto& w) {
					return IsEqualGUID(w, this_guid);
					});

				if (guid_it != std::end(vembeded_guids)) continue;

				buffer = done_fetches[artist->images[artist_img_ndx]->url];
				if (!buffer.get_ptr()) {					
					auto& mbmi = done_fetches.emplace(artist->images[artist_img_ndx]->url, buffer);
					g_discogs->fetch_image(mbmi.first->second, artist->images[artist_img_ndx], p_abort);
					buffer = mbmi.first->second;
				}

				if (this_guid == album_art_ids::icon) {

					MemoryBlock iconbuffer = MemoryBlockToPngIcon(buffer);
					g_discogs->embed_image(iconbuffer, item, this_guid, p_abort);
				}
				else {
					g_discogs->embed_image(buffer, item, this_guid, p_abort);
				}

				vembeded_guids.emplace_back(this_guid);
			}
			else {
				log_msg("Invalid index found embeding artist artwork");
			}
		}
	}
}


void foo_discogs::fetch_image(MemoryBlock &buffer, Image_ptr &image, abort_callback &p_abort) {
	if (!image->url.get_length()) {
		foo_discogs_exception ex;
		ex << "Image URLs unavailable - Is OAuth working?";
		throw ex;
	}

	try
	{
		bool use_api;

		pfc::string8 url = image->url;
		use_api = false;

		discogs_interface->fetcher->fetch_url(url, "", buffer, p_abort, use_api);
	}
	catch (foo_discogs_exception) {
		throw;
	}
}

void foo_discogs::write_image(MemoryBlock &buffer, const pfc::string8 &full_path, abort_callback &p_abort) {
	service_ptr_t<file> file;
	try {
		foobar2000_io::filesystem::g_open_write_new(file, full_path.get_ptr(), p_abort);
	}
	catch (const foobar2000_io::exception_io &e) {
		foo_discogs_exception ex;
		ex << "Error creating file \"" << full_path << "\" [" << e.what() << "]";
		throw ex;
	}

	try {
		try {
			file->write(buffer.get_ptr(), buffer.get_size(), p_abort);
		}
		catch (foobar2000_io::exception_io e) {
			foo_discogs_exception ex;
			ex << "Error writing to file " << full_path << " [" << e.what() << "]";
			throw ex;
		}
	}
	catch (foo_discogs_exception) {
		try {
			foobar2000_io::filesystem::g_remove(full_path.get_ptr(), p_abort);
		}
		catch (foobar2000_io::exception_io) {
		}
		throw;
	}
}

void foo_discogs::embed_image(MemoryBlock &buffer, metadb_handle_ptr item, GUID embed_guid, abort_callback &p_abort) {
	static_api_ptr_t<file_lock_manager> api;
	file_lock_ptr lock = api->acquire_write(item->get_path(), p_abort);
	service_ptr_t<file> file;
	album_art_editor_instance_ptr editor = album_art_editor_v2::g_open(nullptr, item->get_path(), p_abort);
	album_art_data_ptr data = album_art_data_impl::g_create(buffer.get_ptr(), buffer.get_count());
	editor->remove(embed_guid);
	editor->set(embed_guid, data, p_abort);
	editor->commit(p_abort);
}

void foo_discogs::ui_v2_config_callback::ui_fonts_changed() {

	g_hFont = ui_config_manager::get()->query_font(ui_font_lists);
	g_hFontTabs = ui_config_manager::get()->query_font(ui_font_tabs);
	p_discogs->notify(ui_element_notify_font_changed, 0, nullptr, 0);
}

void foo_discogs::ui_v2_config_callback::ui_colors_changed() {
	//..
}

void foo_discogs::notify(const GUID& p_what, t_size p_param1, const void* p_param2, t_size p_param2size) {

	if (ui_v2_cfg_callback) {
		if (p_what == ui_element_notify_font_changed) {

			//FIND RELEASE
			if (find_release_dialog) {
				CustomFont(find_release_dialog->m_hWnd, HIWORD(CONF.custom_font));
				find_release_dialog->Invalidate(true);
			}
			//FIND RELEASE ARTIST
			if (find_release_artist_dialog) {
				CustomFont(find_release_artist_dialog->m_hWnd, HIWORD(CONF.custom_font));
				find_release_artist_dialog->Invalidate(true);
			}
			//TRACK MATCHING
			if (track_matching_dialog) {
				CustomFont(track_matching_dialog->m_hWnd, HIWORD(CONF.custom_font));
				track_matching_dialog->Invalidate(true);
			}
			//PREVIEW TAGS
			if (preview_tags_dialog) {
				CustomFont(preview_tags_dialog->m_hWnd, HIWORD(CONF.custom_font));
				preview_tags_dialog->Invalidate(true);
			}
			//PREVIEW LEADING
			if (preview_modal_tag_dialog) {
				CustomFont(preview_modal_tag_dialog->m_hWnd, HIWORD(CONF.custom_font));
				preview_modal_tag_dialog->Invalidate(true);
			}
			//TAG MAPPING
			if (tag_mappings_dialog) {
				CustomFont(tag_mappings_dialog->m_hWnd, HIWORD(CONF.custom_font));
				tag_mappings_dialog->Invalidate(true);
			}
		}
		return;
	}
	else {
		//..
	}
}

#ifdef CUI_CALLBACK
//cui also calls DUI on font changed when common list font gets modified
void foo_discogs::cui_v2_common_callback::on_font_changed(uint32_t changed_items_mask) const {
	//..
}
#endif
