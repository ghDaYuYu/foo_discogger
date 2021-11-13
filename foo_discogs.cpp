#include "stdafx.h"

#include "libPPUI/gdiplus_helpers.h"

#include "utils.h"
#include "tags.h"
#include "multiformat.h"

#include "track_matching_dialog.h"
#include "preview_dialog.h"
#include "tag_mappings_dialog.h"
#include "tag_mappings_credits_dlg.h"
#include "update_tags_dialog.h"
#include "configuration_dialog.h"
#include "find_release_dialog.h"

#include "track_matching_utils.h"

#include "foo_discogs.h"

using namespace std;

foo_discogs *g_discogs = nullptr;
DiscogsInterface *discogs_interface = nullptr;
HICON g_icon = nullptr;

#ifdef DC_DB
Discogs_DB_Interface* discogs_db_interface = nullptr;
#endif // DC_DB

#define MIN(a,b) ((a)<(b)?(a):(b))

static const char * ERROR_IMAGE_NUMBER_REQUIRED = "Cannot save multiple images with the same name. Use %IMAGE_NUMBER% when forming image names.";

class initquit_discogs : public initquit
{
	virtual void on_init() override {
		console::print("Loading");
		
		init_conf();
		discogs_interface = new DiscogsInterface();
#ifdef DC_DB
		discogs_db_interface = new Discogs_DB_Interface();
#endif // DC_DB

		g_discogs = new foo_discogs();
		load_dlls();
	}
	virtual void on_quit() override {
		console::print("Quitting");
		DeleteObject(g_discogs->icon);
		delete g_discogs;				//(1)
		delete discogs_interface;		//(2)
#ifdef DC_DB
		delete discogs_db_interface;
#endif // DC_DB

		unload_dlls();
	}
};

static initquit_factory_t<initquit_discogs> foo_initquit;

foo_discogs::foo_discogs() {

	CSize s(48, 48);
	icon = GdiplusLoadPNGIcon(IDB_PNG_DC_48, s);

	init_tag_mappings();
	discogs_interface->fetcher->set_oauth(CONF.oauth_token, CONF.oauth_token_secret);

	static_api_ptr_t<titleformat_compiler>()->compile_force(release_name_script, "[%album artist%] - [%album%]");
	gave_oauth_warning = false;
}

#ifndef absdlg

foo_discogs::~foo_discogs() {
	if (find_release_dialog)  {
		find_release_dialog->destroy();
	}
	if (find_release_artist_dialog)
		find_release_artist_dialog->DestroyWindow();

	if (tag_mappings_dialog) {
		tag_mappings_dialog->destroy();
	}
	if (tag_credit_dialog) {
		tag_credit_dialog->destroy();
	}

	if (preview_tags_dialog) {
		preview_tags_dialog->destroy();
	}
}

#else
foo_discogs::~foo_discogs() {
	if (find_release_dialog) {
		find_release_dialog->DestroyWindow();
	}
	if (track_matching_dialog) {
		track_matching_dialog->DestroyWindow();
	}
	if (tag_mappings_dialog) {
		tag_mappings_dialog->DestroyWindow();
	}

	if (update_art_dialog) {
		update_art_dialog->DestroyWindow();
	}
	if (update_tags_dialog) {
		update_tags_dialog->DestroyWindow();
	}
	if (preview_tags_dialog) {
		preview_tags_dialog->DestroyWindow();
	}
}

#endif


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
		else if (web_page == MASTER_RELEASE_PAGE) {
			tag = TAG_RELEASE_ID;
			url_prefix = "https://www.discogs.com/release/";
			if (file_info_get_tag(item, finfo, tag, tag_value, backup_tag)) {
				url << url_prefix << tag_value << url_postfix;
				display_url(url);
			}
		}
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

void foo_discogs::save_album_art(Release_ptr &release, metadb_handle_ptr item,
	art_download_attribs ada, pfc::array_t<GUID> my_album_art_ids, bit_array_bittable& saved_mask,
	pfc::array_t<pfc::string8> &done_files, threaded_process_status &p_status, abort_callback &p_abort) {

	if (!release->images.get_size()) {
		return;
	}

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
		// hardcode "nullptr" as don't write anything.  ???
		if (STR_EQUAL(directory, "nullptr")) {
			ada.write_it = false;
		}
	}
	if (ada.write_it) {
		if (directory[directory.get_length() - 1] != '\\') {
			directory.add_char('\\');
		}
		ensure_directory_exists(directory);
	}

	if (!ada.write_it && !ada.embed_it) {
		log_msg("skipping album art because not writing or embedding it");
		return;
	}

	pfc::string8 last_path = "";
	size_t count = 0;
	size_t offset = 0; //to keep similarity to artist save
	std::vector<bool> vwrite_it(release->images.get_size(), ada.write_it);
	std::vector<bool> voverwrite_it(release->images.get_size(), ada.overwrite_it);
	std::vector<bool> vembed_it(release->images.get_size(), ada.embed_it);

	uartwork uartconf = uartwork(CONF);
	bool bcust_save_or_embed = CONFARTWORK.ucfg_album_save_to_dir != uartconf.ucfg_album_save_to_dir;
	bcust_save_or_embed |= CONFARTWORK.ucfg_album_ovr != uartconf.ucfg_album_ovr;
	bcust_save_or_embed |= CONFARTWORK.ucfg_album_embed != uartconf.ucfg_album_embed;

	if (!bcust_save_or_embed) {
		count = ada.fetch_all_it ? release->images.get_size() : 1;
	}
	else {
		count = release->images.get_size();
		for (size_t i = 0; i < release->images.get_size(); i++) {

			//TODO: REVISE WE COME HERE FROM MULTIPLE save art THREADS (one for each track)
			vwrite_it[i] = CONFARTWORK.getflag(af::alb_sd, i + offset);
			voverwrite_it[i] = CONFARTWORK.getflag(af::alb_ovr, i + offset);
			vembed_it[i] = CONFARTWORK.getflag(af::alb_emb, i + offset);
		}
	}

	for (size_t i = 0; i < count; i++) {
		pfc::string8 path = directory;
		bool write_this = vwrite_it[i];
		if (write_this) {
			hook.set_custom("IMAGE_NUMBER", i + 1);
			hook.set_image(&release->images[i]);
			pfc::string8 file;

			bool cust_file = false;

			if (my_album_art_ids.size() > i + offset) {
				const char* art_id_name = album_art_ids::name_of(my_album_art_ids[i + offset]);
				size_t postfix = 0;
				if (art_id_name == nullptr) {
					art_id_name = template_art_ids::name_of(my_album_art_ids[i + offset]);
					if (art_id_name != nullptr) {
						
						for (size_t walk_vw = 0; walk_vw < i + offset; walk_vw++) {
							if (vwrite_it[walk_vw]) {
								if (pfc::guid_compare(my_album_art_ids[walk_vw + offset], my_album_art_ids[i + offset]) == 0)
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

			if (!file.get_length()) {
				CONF.album_art_filename_string->run_hook(item->get_location(), &info, &hook, file, nullptr);			
			}

			if (STR_EQUAL(file, "nullptr") || STR_EQUAL(file, "")) {
				log_msg("skipping album image because of empty file path");
				if (ada.to_path_only) ada.vpaths.emplace_back("skipping album image because of empty file path");
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
			for (size_t i = 0; i < done_files.get_count(); i++) {
				if (done_files[i].equals(path.get_ptr())) {
					write_this = false;
				}
			}
			if (write_this) {
				done_files.append_single_val(path);
			}
		}

		if (!write_this && !(vembed_it[i])) {
			continue;
		}

		if (ada.to_path_only) {
			continue;
		}


		MemoryBlock buffer;
		//g_discogs->fetch_image(buffer, release->images[i], p_abort);

		if (write_this) {
			//if (CONF.album_art_overwrite || !foobar2000_io::filesystem::g_exists(path, p_abort)) {
			if (voverwrite_it[i]/*overwrite_it*/ || !foobar2000_io::filesystem::g_exists(path, p_abort)) {
				g_discogs->fetch_image(buffer, release->images[i], p_abort);
				g_discogs->write_image(buffer, path, p_abort);
				saved_mask.set(i, true);
			}
			last_path = path;
		}

		if (/*i == 0 &&*/ vembed_it[i]/*embed_it*/) {
			if (!buffer.get_count()) {
				//todo: get it from prev fetch if exists
				g_discogs->fetch_image(buffer, release->images[i], p_abort);
			}
			if (i < my_album_art_ids.size()) {

				if (my_album_art_ids[i] == album_art_ids::icon) {

					MemoryBlock newbuffer = MemoryBlockToPngIcon(buffer);
					g_discogs->embed_image(newbuffer, item, my_album_art_ids[i], p_abort);
				}
				else {
					g_discogs->embed_image(buffer, item, my_album_art_ids[i]/*album_art_ids::cover_front*/, p_abort);
				}
			}
		}
	}
}


void foo_discogs::save_artist_art(Release_ptr &release, metadb_handle_ptr item,
	art_download_attribs ada, pfc::array_t<GUID> my_album_art_ids, bit_array_bittable& saved_mask,
	pfc::array_t<pfc::string8> &done_files, threaded_process_status &p_status, abort_callback &p_abort) {
	// ensure release is loaded
	release->load(p_status, p_abort);

	// get artist IDs from titleformat string
	MasterRelease_ptr master = discogs_interface->get_master_release(release->master_id);
	file_info_impl info;
	item->get_info(info);
	titleformat_hook_impl_multiformat hook(p_status, &master, &release);
	pfc::string8 str;
	pfc::array_t<int> ids;
	try {

		uartwork uartconf = uartwork(CONF);
		bool bcust_artist_save_or_embed = CONFARTWORK.ucfg_art_save_to_dir != uartconf.ucfg_art_save_to_dir;
		bcust_artist_save_or_embed |= CONFARTWORK.ucfg_art_ovr != uartconf.ucfg_art_ovr;
		bcust_artist_save_or_embed |= CONFARTWORK.ucfg_art_embed != uartconf.ucfg_art_embed;
		//in custom mode we might not have an artist id to hook to (nor multiple artists)
		if (bcust_artist_save_or_embed) {
			str =  release->artists[0]->full_artist->id;
		}
		else {
			CONF.artist_art_id_format_string->run_hook(item->get_location(), (file_info*)&info, &hook, str, nullptr);
			//artist tag not found, continue with release full artist
			if (STR_EQUAL(str, "?")) str = release->artists[0]->full_artist->id;
		}

		string_encoded_array result(str);
		if (result.has_array()) {
			for (size_t i = 0; i < result.get_width(); i++) {
				const pfc::string8 &n = result.get_citem(i).get_pure_cvalue();
				int num = std::stoi(n.get_ptr());
				for (size_t j = 0; j < ids.get_size(); j++) {
					if (ids[j] == num) {
						num = 0;
						break;
					}
				}
				if (num != 0) {
					ids.append_single(num);
				}
			}
		}
		else {
			ids.append_single(std::stoi(result.get_pure_cvalue().get_ptr()));
		}

		for (size_t i = 0; i < ids.get_size(); i++) {
			pfc::string8 artist_id;
			artist_id << ids[i];
			Artist_ptr artist = discogs_interface->get_artist(artist_id);
			//if (CONF.save_artist_art || (CONF.embed_artist_art && i == 0)) {
			if (ada.write_it || ada.embed_it) {
				save_artist_art(artist, release, item, ada, my_album_art_ids, saved_mask, done_files, p_status, p_abort);
			}
		}
	}
	catch (foo_discogs_exception &e) {
		foo_discogs_exception ex;
		ex << "Error writing artist art " << " [" << e.what() << "]";
		throw ex;
	}
}


void foo_discogs::save_artist_art(Artist_ptr &artist, Release_ptr &release, metadb_handle_ptr item,
	art_download_attribs ada, pfc::array_t<GUID> my_album_art_ids, bit_array_bittable& saved_mask,
	pfc::array_t<pfc::string8> &done_files, threaded_process_status &p_status, abort_callback &p_abort) {

	if (!artist->images.get_size()) {
		return;
	}

	// ensure artist is loaded
	artist->load(p_status, p_abort);

	//bool write_it = CONF.save_artist_art;
	//bool embed_it = CONF.embed_artist_art;

	MasterRelease_ptr master = discogs_interface->get_master_release(release->master_id);

	file_info_impl info;
	titleformat_hook_impl_multiformat hook(p_status, &master, &release, &artist);
	pfc::string8 directory;

	if (ada.write_it) {
		CONF.artist_art_directory_string->run_hook(item->get_location(), &info, &hook, directory, nullptr);

		// hardcode "nullptr" as don't write anything.
		if (STR_EQUAL(directory, "nullptr")) {
			log_msg("skipping artist art because of nullptr directory");
			ada.write_it = false;
		}
	}

	if (ada.write_it) {
		if (directory[directory.get_length() - 1] != '\\') {
			directory.add_char('\\');
		}
		ensure_directory_exists(directory);
	}

	if (!ada.write_it && !ada.embed_it) {
		return;
	}

	pfc::string8 last_path = "";
	//const size_t count = CONF.artist_art_fetch_all ? artist->images.get_size() : 1;
	
	size_t count = 0;
	size_t album_offset = release->images.get_size();

	//todo: CONFARTWORK is limited to 30 images, past this it takes CONF values
	std::vector<bool> vwrite_it(artist->images.get_size(), ada.write_it);
	std::vector<bool> voverwrite_it(artist->images.get_size(), ada.overwrite_it);
	std::vector<bool> vembed_it(artist->images.get_size(), ada.embed_it);

	uartwork uartconf = uartwork(CONF);
	bool bcust_artist_save_or_embed = CONFARTWORK.ucfg_art_save_to_dir != uartconf.ucfg_art_save_to_dir;
	bcust_artist_save_or_embed |= CONFARTWORK.ucfg_art_ovr != uartconf.ucfg_art_ovr;
	bcust_artist_save_or_embed |= CONFARTWORK.ucfg_art_embed != uartconf.ucfg_art_embed;

	//debug
	/*if (true) {
		bcust_artist_save_or_embed = false;
	
	}*/


	if (!bcust_artist_save_or_embed) {
		count = ada.fetch_all_it ? artist->images.get_size() : 1;
	}
	else {
		count = artist->images.get_size();
		for (size_t i = 0; i < count; i++) {
			vwrite_it[i] = CONFARTWORK.getflag(af::art_sd, i /*+ album_offset*/);
			voverwrite_it[i] = CONFARTWORK.getflag(af::art_ovr, i /*+ album_offset*/);
			vembed_it[i] = CONFARTWORK.getflag(af::art_emb, i /*+ album_offset*/);
		}
	}

	for (size_t i = 0; i < count; i++) {
		pfc::string8 path = directory;
		bool write_this = /*write_it*/vwrite_it[i];
		if (write_this) {
			hook.set_custom("IMAGE_NUMBER", i + 1);
			hook.set_image(&artist->images[i]);

			pfc::string8 file;

			bool cust_file = false;

			if (my_album_art_ids.size() > i + album_offset) {
				//GUID debug = my_album_art_ids[i + album_offset];
				const char* art_id_name = album_art_ids::name_of(my_album_art_ids[i + album_offset]);
				file = art_id_name == nullptr ? "" : pfc::string8(art_id_name);

				pfc::chain_list_v2_t<pfc::string8> splitfile;
				pfc::splitStringByChar(splitfile, file.get_ptr(), ' ');
				if (splitfile.get_count()) file = *splitfile.first();
			}

			if (!file.get_length())
				CONF.artist_art_filename_string->run_hook(item->get_location(), &info, &hook, file, nullptr);

			if (STR_EQUAL(file, "nullptr")) {
				log_msg("skipping artist image because of nullptr directory");
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

			for (size_t i = 0; i < done_files.get_count(); i++) {
				if (done_files[i].equals(path)) {
					write_this = false;
					//todo: revise commented: continue;
				}
			}
			if (write_this) {
				done_files.append_single_val(path);
			}
		}

		if (!write_this && !(/*i == 0 &&*/ vembed_it[i]/*embed_it*/)) {
			continue;
		}

		if (ada.to_path_only) {
			continue;
		}

		MemoryBlock buffer;
		//g_discogs->fetch_image(buffer, artist->images[i], p_abort);

		if (write_this) {


			//if (CONF.artist_art_overwrite || !foobar2000_io::filesystem::filesystem::g_exists(path, p_abort)) {
			if (/*overwrite_it*/voverwrite_it[i] || !foobar2000_io::filesystem::filesystem::g_exists(path, p_abort)) {
				g_discogs->fetch_image(buffer, artist->images[i], p_abort);
				g_discogs->write_image(buffer, path, p_abort);

				saved_mask.set(i + album_offset, true);
			}
			last_path = path;
		}

		if (vembed_it[i]) {

			GUID art_id;
			if (my_album_art_ids.size() > i + album_offset) {
				art_id = my_album_art_ids[i + album_offset];
			}
			
			if (!buffer.get_count()) {
				//todo: get it from previously fetched
				g_discogs->fetch_image(buffer, artist->images[i], p_abort);
			}

			if (art_id == album_art_ids::icon) {
				
				MemoryBlock newbuffer = MemoryBlockToPngIcon(buffer);
				g_discogs->embed_image(newbuffer, item, my_album_art_ids[i + album_offset] /*album_art_ids::artist*/, p_abort);
			}
			else {
				g_discogs->embed_image(buffer, item, my_album_art_ids[i + album_offset] /*album_art_ids::artist*/, p_abort);
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
