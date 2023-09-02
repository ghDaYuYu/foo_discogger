#include "stdafx.h"

#include <filesystem>

#include "conf.h"
#include "utils.h"
#include "utils_path.h"
#include "utils_db.h"

static const GUID guid_cfg_bool_values =
{ 0x22c5b65e, 0x84e8, 0x4d73, { 0xb3, 0xff, 0x34, 0x95, 0x1d, 0x2c, 0x8a, 0x65 } };
static const GUID guid_cfg_int_values =
{ 0x918de111, 0xa72a, 0x4215, { 0xaf, 0xa9, 0x49, 0x7f, 0x42, 0x71, 0x1d, 0x6b } };
static const GUID guid_cfg_string_values =
{ 0x945598de, 0x4895, 0x4894, { 0xb6, 0x88, 0xa6, 0x6f, 0xdb, 0xe1, 0x5, 0xf9 } };

cfg_objList<conf_bool_entry> cfg_bool_entries(guid_cfg_bool_values);
cfg_objList<conf_int_entry> cfg_int_entries(guid_cfg_int_values);
cfg_objList<conf_string_entry> cfg_string_entries(guid_cfg_string_values);

conf_bool_entry make_conf_entry(int i, bool v) {
	conf_bool_entry result = {0};
	result.id = i;
	result.value = v;
	return result;
}

conf_int_entry make_conf_entry(int i, int v) {
	conf_int_entry result = {0};
	result.id = i;
	result.value = v;
	return result;
}

conf_string_entry make_conf_entry(int i, const pfc::string8& v) {
	conf_string_entry result;
	result.id = i;
	result.value = v;
	return result;
}

bool copy_dbf_file(pfc::string8 src_dbpath, pfc::string8 dst_dbpath) {

	std::filesystem::path os_src = std::filesystem::u8path(src_dbpath.c_str());
	std::filesystem::path os_dst = std::filesystem::u8path(dst_dbpath.c_str());

	try {

		//copy dbf...
		
		if (std::filesystem::exists(os_dst)) {

			log_msg("a previous foo_discogger.cfg.db file version was found");
			return true;
		}
		return std::filesystem::copy_file(os_src, os_dst);
	}
	catch (const std::filesystem::filesystem_error& e)
	{
		pfc::string8 msg = "failed to install foo_discogger.cfg.db file";
		msg << (!std::filesystem::exists(os_src) ? " (missing db source file)" : "");
		log_msg(msg);
		return false;
	}

	return false;
}

bool prepare_dbf_and_cache(bool bimport = true) {

	bool bres = true;

	//n8_ -> native_utf8
	pfc::string8 n8_src_path = profile_usr_components_path();
	pfc::string8 n8_dst_path = profile_path("configuration");

	n8_src_path << "\\" << dll_db_filename();
	n8_dst_path << "\\" << dll_db_filename();
	try {

		std::filesystem::path os_src = std::filesystem::u8path(n8_src_path.c_str());
		std::filesystem::path os_dst = std::filesystem::u8path(n8_dst_path.c_str());

		bool b_dst_exists = std::filesystem::exists(os_dst);

		if (b_dst_exists && bimport) {

			//todo: test import definitions

			sqldb db;
			db.open(os_dst.string().c_str(), SQLITE_OPEN_READWRITE);
			sqlite3* pDb = db.db_handle();

			char* zErrMsg = 0;
			pfc::string8 sqlcmd;
			sqlcmd << "ATTACH DATABASE \'" << os_src.string().c_str() << "\' AS trg";
			size_t ret = sqlite3_exec(pDb, sqlcmd, NULL, NULL, &zErrMsg);
			sqlcmd = "INSERT INTO trg.def_credit (name, tagtype, desc, titleformat) SELECT name, tagtype, desc, titleformat FROM def_credit WHERE tagtype IS NULL;";
			ret = sqlite3_exec(pDb, sqlcmd, NULL, NULL, &zErrMsg);
			db.close();
		}
		else {

			if (bimport) {
				log_msg("failed to import former foo_discogger.cfg.db values, recreating...");
			}

			// copy database

			bres = copy_dbf_file(n8_src_path, n8_dst_path);

		}
	}
	catch (...) {
		
		log_msg("unexpected exception installing configuration files");
		return false;
	
	}

	try {
		//create offline cache folder
		pfc::string8 n8_olPath = profile_path(OC_NAME);		
		std::filesystem::path os_olDst = std::filesystem::u8path(n8_olPath.c_str());
		std::filesystem::create_directory(os_olDst);
		bres &= true;
	}
	catch (...) {

		log_msg("unexpected exception creating cache folder");
		return false;
	}

	return bres;
}

bool CConf::load() {

	// vspec vnnn { spec vector, # bools, # ints, # strings };
	bool bres = true;
	vspec v000{ &vec_specs, 0, 0, 0 };
	vspec v204{ &vec_specs, 28, 42, 14 }; // 1.0.4
	vspec v205{ &vec_specs, 24, 39, 15 }; // 1.0.6 + sqlite db
	vspec v206{ &vec_specs, 27, 42, 15 }; // 1.0.8
	vspec v207{ &vec_specs, 27, 50, 15 }; // 1.0.13
	vspec v208{ &vec_specs, 27, 50, 16 }; // 1.0.15
	vspec v209{ &vec_specs, 27, 51, 16 }; // 1.0.16.1

	vspec* vlast = &vec_specs.at(vec_specs.size() - 1);
	
	vspec vLoad = {
		nullptr,
		cfg_bool_entries.get_count(),
		cfg_int_entries.get_count(),
		cfg_string_entries.get_count()
	};

	if (vLoad == v000) {
		console::print("Installing foo_discogger");
		save();
		//EXIT
		return prepare_dbf_and_cache(false);
	}
	else {
		if (vLoad > *vlast || vLoad < v204) {
			//reset unknown version
			vLoad = *vlast;
			pfc::string8 title;
			title << "Configuration Reset";
			pfc::string8 msg("This upgrade is incompatible with the current foo_discogger configuration version.\n");
			msg << "Configuration will be reset.\n";
			uMessageBox(core_api::get_main_window(), msg, title, MB_APPLMODAL | MB_ICONASTERISK);

			save();
			//EXIT
			return prepare_dbf_and_cache(false);
		}
	}

	// ignore bres while upgrading (loading depricated values will fail)
	for (unsigned int i = 0; i < cfg_bool_entries.get_count(); i++) {
		/*bres &=*/ bool_load(cfg_bool_entries[i]);
	}

	if (vLoad < v206) {

		cfg_bool_entries.add_item(
			make_conf_entry(CFG_AUTO_REL_LOAD_ON_OPEN, auto_rel_load_on_open)
		);
		cfg_bool_entries.add_item(
			make_conf_entry(CFG_AUTO_REL_LOAD_ON_SELECT, auto_rel_load_on_select)
		);
		cfg_bool_entries.add_item(
			make_conf_entry(CFG_PARSE_HIDDEN_MERGE_TITLES, parse_hidden_merge_titles)
		);
	}

	for (unsigned int i = 0; i < cfg_int_entries.get_count(); i++) {
		/*bres &=*/ int_load(cfg_int_entries[i]);
	}

	if (vLoad < v206) {

		cfg_int_entries.add_item(
			make_conf_entry(CFG_DISCOGS_ARTWORK_INDEX_WIDTH, match_discogs_artwork_index_width)
		);
		cfg_int_entries.add_item(
			make_conf_entry(CFG_MATCH_FILE_ARTWORK_INDEX_WIDTH, match_file_artwork_index_width)
		);
		cfg_int_entries.add_item(
			make_conf_entry(CFG_PREVIEW_LEADING_TAGS_DLG_COLS_WIDTH, preview_leading_tags_dlg_cols_width)
		);
	}

	if (vLoad < v207) {

		cfg_int_entries.add_item(
			make_conf_entry(CFG_DISCOGS_ARTWORK_TL_RA_WIDTH, match_discogs_artwork_tl_ra_width)
		);
		cfg_int_entries.add_item(
			make_conf_entry(CFG_DISCOGS_ARTWORK_TL_TYPE_WIDTH, match_discogs_artwork_tl_type_width)
		);
		cfg_int_entries.add_item(
			make_conf_entry(CFG_DISCOGS_ARTWORK_TL_DIM_WIDTH, match_discogs_artwork_tl_dim_width)
		);
		cfg_int_entries.add_item(
			make_conf_entry(CFG_DISCOGS_ARTWORK_TL_SAVE_WIDTH, match_discogs_artwork_tl_save_width)
		);
		cfg_int_entries.add_item(
			make_conf_entry(CFG_DISCOGS_ARTWORK_TL_OVR_WIDTH, match_discogs_artwork_tl_ovr_width)
		);
		cfg_int_entries.add_item(
			make_conf_entry(CFG_DISCOGS_ARTWORK_TL_EMBED_WIDTH, match_discogs_artwork_tl_embed_width)
		);
		cfg_int_entries.add_item(
			make_conf_entry(CFG_DISCOGS_ARTWORK_TL_INDEX_WIDTH, match_discogs_artwork_tl_index_width)
		);
		cfg_int_entries.add_item(
			make_conf_entry(CFG_CUSTOM_FONT, custom_font)
		);
	}

	if (vLoad < v209) {
		cfg_int_entries.add_item(
			make_conf_entry(CFG_ALT_WRITE_FLAGS, alt_write_flags)
		);
	}

	if (vLoad == v204) {

		bres &= prepare_dbf_and_cache(false);

		std::vector<int> vdel = {
			DEPRI_CFG_PREVIEW_DIALOG_SIZE,
			DEPRI_CFG_PREVIEW_DIALOG_POSITION,
			DEPRI_CFG_FIND_RELEASE_DIALOG_SIZE,
			DEPRI_CFG_FIND_RELEASE_DIALOG_POSITION,
			DEPRI_CFG_RELEASE_DIALOG_SIZE,
			DEPRI_CFG_RELEASE_DIALOG_POSITION,
			DEPRI_CFG_EDIT_TAGS_DIALOG_SIZE,
			DEPRI_CFG_EDIT_TAGS_DIALOG_POSITION };

		for (auto walk_delete : vdel) {
			for (unsigned int i = 0; i < cfg_int_entries.get_count(); i++) {
				const conf_int_entry& item = cfg_int_entries[i];
				if (item.id == walk_delete) {
					cfg_int_entries.remove_item(item);
					break;
				}
			}
		}

		size_t dummy;
		cfg_bool_entries.sort_t(compare_id);
		conf_bool_entry entry, entry2;

		entry.id = DEPRI_CFG_FIND_RELEASE_DIALOG_SHOW_ID;
		auto found = cfg_bool_entries.bsearch_t(compare_id, entry, dummy);
		entry2 = cfg_bool_entries.get_item(dummy);
		cfg_bool_entries.remove_item(entry2);

		entry.id = DEPRI_CFG_SKIP_RELEASE_DLG_IF_MATCHED;
		found = cfg_bool_entries.bsearch_t(compare_id, entry, dummy);
		entry2 = cfg_bool_entries.get_item(dummy);
		skip_mng_flag |= entry2.value ? 1 << 0 : 0;
		cfg_bool_entries.remove_item(entry2);

		entry.id = DEPRI_CFG_SKIP_FIND_RELEASE_DLG_IF_IDED;
		found = cfg_bool_entries.bsearch_t(compare_id, entry, dummy);
		entry2 = cfg_bool_entries.get_item(dummy);
		skip_mng_flag |= entry2.value ? 1 << 1 : 0;
		cfg_bool_entries.remove_item(entry2);

		entry.id = DEPRI_CFG_SKIP_PREVIEW_DIALOG;
		found = cfg_bool_entries.bsearch_t(compare_id, entry, dummy);
		entry2 = cfg_bool_entries.get_item(dummy);
		skip_mng_flag |= entry2.value ? 1 << 2 : 0;
		cfg_bool_entries.remove_item(entry2);
		cfg_int_entries.add_item(make_conf_entry(CFG_DC_DB_FLAG, 0));
		cfg_int_entries.add_item(make_conf_entry(CFG_FIND_RELEASE_FILTER_FLAG, find_release_filter_flag));
		cfg_int_entries.add_item(make_conf_entry(CFG_SKIP_MNG_FLAG, skip_mng_flag));

		cfg_int_entries.add_item(make_conf_entry(CFG_LIST_STYLE, list_style));
		cfg_int_entries.add_item(make_conf_entry(CFG_HISTORY_MAX_ITEMS, history_enabled_max));

		cfg_int_entries.add_item(make_conf_entry(CFG_FIND_RELEASE_DIALOG_FLAG, find_release_dlg_flags));
	}
	//..

	for (unsigned int i = 0; i < cfg_string_entries.get_count(); i++) {
		/*bres &=*/ string_load(cfg_string_entries[i]);
	}

	if (vLoad == v204) {
		cfg_string_entries.add_item(make_conf_entry(CFG_DC_DB_PATH, db_dc_path));
	}

	if (vLoad < v208) {
		cfg_string_entries.add_item(make_conf_entry(CFG_MULTIVALUE_FIELDS, multivalue_fields));
	}
	//..

	return bres;
}

bool CConf::bool_load(const conf_bool_entry& item) {

	switch (item.id) {
	case CFG_REPLACE_ANVS:
		replace_ANVs = item.value;
		break;
	case CFG_MOVE_THE_AT_BEGINNING:
		move_the_at_beginning = item.value;
		break;
	case CFG_DISCARD_NUMERIC_SUFFIX:
		discard_numeric_suffix = item.value;
		break;
	case CFG_MATCH_TRACKS_USING_DURATION:
		match_tracks_using_duration = item.value;
		break;
	case CFG_MATCH_TRACKS_USING_NUMBER:
		match_tracks_using_number = item.value;
		break;
	case CFG_ASSUME_TRACKS_SORTED:
		assume_tracks_sorted = item.value;
		break;
	case CFG_SAVE_ALBUM_ART:
		save_album_art = item.value;
		break;
	case CFG_SAVE_ARTIST_ART:
		save_artist_art = item.value;
		break;
	case CFG_ALBUM_ART_EMBED:
		embed_album_art = item.value;
		break;
	case CFG_ARTIST_ART_EMBED:
		embed_artist_art = item.value;
		break;
	case CFG_ALBUM_ART_FETCH_ALL:
		album_art_fetch_all = item.value;
		break;
	case CFG_ALBUM_ART_OVERWRITE:
		album_art_overwrite = item.value;
		break;
	case CFG_ARTIST_ART_FETCH_ALL:
		artist_art_fetch_all = item.value;
		break;
	case CFG_ARTIST_ART_OVERWRITE:
		artist_art_overwrite = item.value;
		break;
	case CFG_DISPLAY_EXACT_MATCHES:
		display_exact_matches = item.value;
		break;
	case CFG_ENABLE_AUTOSEARCH:
		enable_autosearch = item.value;
		break;
	case CFG_DISPLAY_ANVS:
		display_ANVs = item.value;
		break;
	case CFG_REMOVE_OTHER_TAGS:
		remove_other_tags = item.value;
		break;
	case CFG_UPDATE_PREVIEW_CHANGES:
		update_tags_preview_changes = item.value;
		break;
	case CFG_UPDATE_TAGS_MANUALLY_PROMPT:
		update_tags_manually_match = item.value;
		break;
	case CFG_PARSE_HIDDEN_AS_REGULAR:
		parse_hidden_as_regular = item.value;
		break;
	case CFG_SKIP_VIDEO_TRACKS:
		skip_video_tracks = item.value;
		break;
	//v200
	case CFG_EDIT_TAGS_DIALOG_SHOW_TM_STATS:
		edit_tags_dlg_show_tm_stats = item.value;
		break;
	case CFG_RELEASE_ENTER_KEY_OVR:
		release_enter_key_override = item.value;
		break;
	//v206
	case CFG_AUTO_REL_LOAD_ON_OPEN:
		auto_rel_load_on_open = item.value;
		break;
	case CFG_AUTO_REL_LOAD_ON_SELECT:
		auto_rel_load_on_select = item.value;
		break;
	case CFG_PARSE_HIDDEN_MERGE_TITLES:
		parse_hidden_merge_titles = item.value;
		break;
	//..
	default:
		return false;
	}
	return true;
}

bool CConf::int_load(const conf_int_entry& item) {

	switch (item.id) {
	case CFG_TAGSAVE_FLAGS:
		tag_save_flags = item.value;
		break;
	case CFG_EDIT_TAGS_DIALOG_COL1_WIDTH:
		edit_tags_dialog_col1_width = item.value;
		break;
	case CFG_EDIT_TAGS_DIALOG_COL2_WIDTH:
		edit_tags_dialog_col2_width = item.value;
		break;
	case CFG_EDIT_TAGS_DIALOG_COL3_WIDTH:
		edit_tags_dialog_col3_width = item.value;
		break;
	case CFG_LAST_CONF_TAB:
		last_conf_tab = item.value;
		break;
	case CFG_PREVIEW_MODE:
		preview_mode = item.value;
		break;
	case CFG_CACHE_MAX_OBJECTS:
		cache_max_objects = item.value;
		break;
	//v200
	case CFG_PREVIEW_TAGS_DIALOG_COL1_WIDTH:
		preview_tags_dialog_col1_width = item.value;
		break;
	case CFG_PREVIEW_TAGS_DIALOG_COL2_WIDTH:
		preview_tags_dialog_col2_width = item.value;
		break;
	case CFG_MATCH_TRACKS_DISCOGS_COL1_WIDTH:
		match_tracks_discogs_col1_width = item.value;
		break;
	case CFG_MATCH_TRACKS_DISCOGS_COL2_WIDTH:
		match_tracks_discogs_col2_width = item.value;
		break;
	case CFG_MATCH_TRACKS_FILES_COL1_WIDTH:
		match_tracks_files_col1_width = item.value;
		break;
	case CFG_MATCH_TRACKS_FILES_COL2_WIDTH:
		match_tracks_files_col2_width = item.value;
		break;
	case CFG_MATCH_TRACKS_DISCOGS_STYLE:
		match_tracks_discogs_style = item.value;
		break;
	case CFG_MATCH_TRACKS_FILES_STYLE:
		match_tracks_files_style = item.value;
		break;
	case CFG_PREVIEW_TAGS_DIALOG_W_WIDTH:
		preview_tags_dialog_w_width = item.value;
		break;
	case CFG_PREVIEW_TAGS_DIALOG_U_WIDTH:
		preview_tags_dialog_u_width = item.value;
		break;
	case CFG_PREVIEW_TAGS_DIALOG_S_WIDTH:
		preview_tags_dialog_s_width = item.value;
		break;
	case CFG_PREVIEW_TAGS_DIALOG_E_WIDTH:
		preview_tags_dialog_e_width = item.value;
		break;
	case CFG_CACHE_OFFLINE_CACHE_FLAG:
		cache_offline_cache_flag = item.value;
		break;
	case CFG_DISCOGS_ARTWORK_RA_WIDTH:
		match_discogs_artwork_ra_width = item.value;
		break;
	case CFG_DISCOGS_ARTWORK_TYPE_WIDTH:
		match_discogs_artwork_type_width = item.value;
		break;
	case CFG_DISCOGS_ARTWORK_DIM_WIDTH:
		match_discogs_artwork_dim_width = item.value;
		break;
	case CFG_DISCOGS_ARTWORK_SAVE_WIDTH:
		match_discogs_artwork_save_width = item.value;
		break;
	case CFG_DISCOGS_ARTWORK_OVR_WIDTH:
		match_discogs_artwork_ovr_width = item.value;
		break;
	case CFG_DISCOGS_ARTWORK_EMBED_WIDTH:
		match_discogs_artwork_embed_width = item.value;
		break;
	case CFG_MATCH_FILE_ARTWORK_NAME_WIDTH:
		match_file_artwork_name_width = item.value;
		break;
	case CFG_MATCH_FILE_ARTWORK_DIM_WIDTH:
		match_file_artwork_dim_width = item.value;
		break;
	case CFG_MATCH_FILE_ARTWORK_SIZE_WIDTH:
		match_file_artwork_size_width = item.value;
		break;
	//todo: depri
	case CFG_MATCH_DISCOGS_ARTWORK_STYLE:
		match_discogs_artwork_art_style = item.value;
		break;
	case CFG_MATCH_FILE_ARTWORKS_STYLE:
		match_file_artworks_art_style = item.value;
		break;
	//..
	case CFG_ALBUM_ART_SKIP_DEFAULT_CUST:
		album_art_skip_default_cust = item.value;
		break;
	//v204
	case CFG_EDIT_TAGS_DIALOG_FLAGS:
		edit_tags_dlg_flags = item.value;
		break;
	//v205
	case CFG_DC_DB_FLAG:
		db_dc_flag = item.value;
		break;
	case CFG_FIND_RELEASE_FILTER_FLAG:
		find_release_filter_flag = item.value;
		break;
	case CFG_SKIP_MNG_FLAG:
		skip_mng_flag = item.value;
		break;
	case CFG_LIST_STYLE:
		list_style = item.value;
		break;
	case CFG_HISTORY_MAX_ITEMS:
		history_enabled_max = item.value;
		break;

	case CFG_FIND_RELEASE_DIALOG_FLAG:
		find_release_dlg_flags = item.value;
		break;
	//v206

	case CFG_DISCOGS_ARTWORK_INDEX_WIDTH:
		match_discogs_artwork_index_width = item.value;
		break;
	case CFG_MATCH_FILE_ARTWORK_INDEX_WIDTH:
		match_file_artwork_index_width = item.value;
		break;
	case CFG_PREVIEW_LEADING_TAGS_DLG_COLS_WIDTH:
		preview_leading_tags_dlg_cols_width = item.value;
		break;

	//v207
	case CFG_DISCOGS_ARTWORK_TL_RA_WIDTH:
		match_discogs_artwork_tl_ra_width = item.value;
		break;
	case CFG_DISCOGS_ARTWORK_TL_TYPE_WIDTH:
		match_discogs_artwork_tl_type_width = item.value;
		break;
	case CFG_DISCOGS_ARTWORK_TL_DIM_WIDTH:
		match_discogs_artwork_tl_dim_width = item.value;
		break;
	case CFG_DISCOGS_ARTWORK_TL_SAVE_WIDTH:
		match_discogs_artwork_tl_save_width = item.value;
		break;
	case CFG_DISCOGS_ARTWORK_TL_OVR_WIDTH:
		match_discogs_artwork_tl_ovr_width = item.value;
		break;
	case CFG_DISCOGS_ARTWORK_TL_EMBED_WIDTH:
		match_discogs_artwork_tl_embed_width = item.value;
		break;
	case CFG_DISCOGS_ARTWORK_TL_INDEX_WIDTH:
		match_discogs_artwork_tl_index_width = item.value;
		break;
	
	case CFG_CUSTOM_FONT:
		custom_font = item.value;
		break;

	//v209 (1.0.16.1)
	case CFG_ALT_WRITE_FLAGS:
		alt_write_flags = item.value;
		break;

	//..
	default:
		return false;
	}
	return true;
}

bool CConf::string_load(const conf_string_entry& item) {

	switch (item.id) {
	case CFG_ALBUM_ART_DIRECTORY_STRING:
		album_art_directory_string = item.value;
		break;
	case CFG_ALBUM_ART_FILENAME_STRING:
		album_art_filename_string = item.value;
		break;
	case CFG_ARTIST_ART_DIRECTORY_STRING:
		artist_art_directory_string = item.value;
		break;
	case CFG_ARTIST_ART_FILENAME_STRING:
		artist_art_filename_string = item.value;
		break;
	case CFG_ARTIST_ART_IDS_TF_STRING:
		artist_art_id_format_string = item.value;
		break;
	case CFG_OAUTH_TOKEN:
		oauth_token = item.value;
		break;
	case CFG_OAUTH_TOKEN_SECRET:
		oauth_token_secret = item.value;
		break;
	case CFG_REMOVE_EXCLUDE_TAGS:
		set_remove_exclude_tags(item.value);
		break;
	case CFG_SEARCH_RELEASE_FORMAT_STRING:
		search_release_format_string = item.value;
		break;
	case CFG_SEARCH_MASTER_FORMAT_STRING:
		search_master_format_string = item.value;
		break;
	case CFG_SEARCH_MASTER_SUB_FORMAT_STRING:
		search_master_sub_format_string = item.value;
		break;
	case CFG_DISCOGS_TRACK_FORMAT_STRING:
		release_discogs_format_string = item.value;
		break;
	case CFG_FILE_TRACK_FORMAT_STRING:
		release_file_format_string = item.value;
		break;
		//v200
	case CFG_EDIT_TAGS_DIALOG_HL_KEYWORD:
		edit_tags_dlg_hl_keyword = item.value;
		break;
		//205
	case CFG_DC_DB_PATH:
		db_dc_path = item.value;
		break;
	//v208
	case CFG_MULTIVALUE_FIELDS:
		multivalue_fields = item.value;
		break;
	//..

	default:
		PFC_ASSERT(false);
	}

	return true;
}
bool CConf::GetFlagVar(int ID, FlgMng& flgmng) {

	auto fit = std::find(vflags.begin(), vflags.end(), ID);

	if (fit != vflags.end()) {

		flgmng = FlgMng(this, ID);
		return true;
	}
	return false;
}

bool CConf::id_to_val_bool(int id, const CConf& in_conf, bool& out, bool assert) {

	switch (id) {
	case CFG_REPLACE_ANVS:
		out = in_conf.replace_ANVs;
		break;
	case CFG_MOVE_THE_AT_BEGINNING:
		out = in_conf.move_the_at_beginning;
		break;
	case CFG_DISCARD_NUMERIC_SUFFIX:
		out = in_conf.discard_numeric_suffix;
		break;
	case CFG_MATCH_TRACKS_USING_DURATION:
		out = in_conf.match_tracks_using_duration;
		break;
	case CFG_MATCH_TRACKS_USING_NUMBER:
		out = in_conf.match_tracks_using_number;
		break;
	case CFG_ASSUME_TRACKS_SORTED:
		out = in_conf.assume_tracks_sorted;
		break;
	case CFG_SAVE_ALBUM_ART:
		out = in_conf.save_album_art;
		break;
	case CFG_SAVE_ARTIST_ART:
		out = in_conf.save_artist_art;
		break;
	case CFG_ALBUM_ART_EMBED:
		out = in_conf.embed_album_art;
		break;
	case CFG_ARTIST_ART_EMBED:
		out = in_conf.embed_artist_art;
		break;
	case CFG_ALBUM_ART_FETCH_ALL:
		out = in_conf.album_art_fetch_all;
		break;
	case CFG_ALBUM_ART_OVERWRITE:
		out = in_conf.album_art_overwrite;
		break;
	case CFG_ARTIST_ART_FETCH_ALL:
		out = in_conf.artist_art_fetch_all;
		break;
	case CFG_ARTIST_ART_OVERWRITE:
		out = in_conf.artist_art_overwrite;
		break;
	case CFG_DISPLAY_EXACT_MATCHES:
		out = in_conf.display_exact_matches;
		break;
	case CFG_ENABLE_AUTOSEARCH:
		out = in_conf.enable_autosearch;
		break;
	case CFG_DISPLAY_ANVS:
		out = in_conf.display_ANVs;
		break;
	case CFG_REMOVE_OTHER_TAGS:
		out = in_conf.remove_other_tags;
		break;
	case CFG_UPDATE_PREVIEW_CHANGES:
		out = in_conf.update_tags_preview_changes;
		break;
	case CFG_UPDATE_TAGS_MANUALLY_PROMPT:
		out = in_conf.update_tags_manually_match;
		break;
	case CFG_PARSE_HIDDEN_AS_REGULAR:
		out = in_conf.parse_hidden_as_regular;
		break;
	case CFG_SKIP_VIDEO_TRACKS:
		out = in_conf.skip_video_tracks;
		break;
	//v200
	case CFG_EDIT_TAGS_DIALOG_SHOW_TM_STATS:
		out = in_conf.edit_tags_dlg_show_tm_stats;
		break;
	case CFG_RELEASE_ENTER_KEY_OVR:
		out = in_conf.release_enter_key_override;
		break;
	//v206
	case CFG_AUTO_REL_LOAD_ON_OPEN:
		out = in_conf.auto_rel_load_on_open;
		break;
	case CFG_AUTO_REL_LOAD_ON_SELECT:
		out = in_conf.auto_rel_load_on_select;
		break;
	case CFG_PARSE_HIDDEN_MERGE_TITLES:
		out = in_conf.parse_hidden_merge_titles;
		break;
	//..

	default:
		if (assert) {
			PFC_ASSERT(false);
		}
		return false;

	}
	return true;
}

int* CConf::id_to_ref_int(int ID, bool assert) {

	switch (ID) {
	case CFG_FIND_RELEASE_DIALOG_FLAG:

		return &find_release_dlg_flags;

	case CFG_SKIP_MNG_FLAG:

		return &skip_mng_flag;

	case CFG_CACHE_OFFLINE_CACHE_FLAG:

		return &cache_offline_cache_flag;

	default:
		if (assert) {
			PFC_ASSERT(false);
		}
		return nullptr;
	}

	return nullptr;
}

bool CConf::id_to_val_int(int id, const CConf& in_conf, int& out, bool assert) {

	switch (id) {
	case CFG_TAGSAVE_FLAGS:
		out = in_conf.tag_save_flags;
		break;
	case CFG_EDIT_TAGS_DIALOG_COL1_WIDTH:
		out = in_conf.edit_tags_dialog_col1_width;
		break;
	case CFG_EDIT_TAGS_DIALOG_COL2_WIDTH:
		out = in_conf.edit_tags_dialog_col2_width;
		break;
	case CFG_EDIT_TAGS_DIALOG_COL3_WIDTH:
		out = in_conf.edit_tags_dialog_col3_width;
		break;
	case CFG_LAST_CONF_TAB:
		out = in_conf.last_conf_tab;
		break;
	case CFG_PREVIEW_MODE:
		out = in_conf.preview_mode;
		break;
	case CFG_CACHE_MAX_OBJECTS:
		out = in_conf.cache_max_objects;
		break;
	//v200
	case CFG_PREVIEW_TAGS_DIALOG_COL1_WIDTH:
		out = in_conf.preview_tags_dialog_col1_width;
		break;
	case CFG_PREVIEW_TAGS_DIALOG_COL2_WIDTH:
		out = in_conf.preview_tags_dialog_col2_width;
		break;
	case CFG_MATCH_TRACKS_DISCOGS_COL1_WIDTH:
		out = in_conf.match_tracks_discogs_col1_width;
		break;
	case CFG_MATCH_TRACKS_DISCOGS_COL2_WIDTH:
		out = in_conf.match_tracks_discogs_col2_width;
		break;
	case CFG_MATCH_TRACKS_FILES_COL1_WIDTH:
		out = in_conf.match_tracks_files_col1_width;
		break;
	case CFG_MATCH_TRACKS_FILES_COL2_WIDTH:
		out = in_conf.match_tracks_files_col2_width;
		break;
	case CFG_MATCH_TRACKS_DISCOGS_STYLE:
		out = in_conf.match_tracks_discogs_style;
		break;
	case CFG_MATCH_TRACKS_FILES_STYLE:
		out = in_conf.match_tracks_files_style;
		break;
	case CFG_PREVIEW_TAGS_DIALOG_W_WIDTH:
		out = in_conf.preview_tags_dialog_w_width;
		break;
	case CFG_PREVIEW_TAGS_DIALOG_U_WIDTH:
		out = in_conf.preview_tags_dialog_u_width;
		break;
	case CFG_PREVIEW_TAGS_DIALOG_S_WIDTH:
		out = in_conf.preview_tags_dialog_s_width;
		break;
	case CFG_PREVIEW_TAGS_DIALOG_E_WIDTH:
		out = in_conf.preview_tags_dialog_e_width;
		break;
	case CFG_CACHE_OFFLINE_CACHE_FLAG:
		out = in_conf.cache_offline_cache_flag;
		break;
	case CFG_DISCOGS_ARTWORK_RA_WIDTH:
		out = in_conf.match_discogs_artwork_ra_width;
		break;
	case CFG_DISCOGS_ARTWORK_TYPE_WIDTH:
		out = in_conf.match_discogs_artwork_type_width;
		break;
	case CFG_DISCOGS_ARTWORK_DIM_WIDTH:
		out = in_conf.match_discogs_artwork_dim_width;
		break;
	case CFG_DISCOGS_ARTWORK_SAVE_WIDTH:
		out = in_conf.match_discogs_artwork_save_width;
		break;
	case CFG_DISCOGS_ARTWORK_OVR_WIDTH:
		out = in_conf.match_discogs_artwork_ovr_width;
		break;
	case CFG_DISCOGS_ARTWORK_EMBED_WIDTH:
		out = in_conf.match_discogs_artwork_embed_width;
		break;
	case CFG_MATCH_FILE_ARTWORK_NAME_WIDTH:
		out = in_conf.match_file_artwork_name_width;
		break;
	case CFG_MATCH_FILE_ARTWORK_DIM_WIDTH:
		out = in_conf.match_file_artwork_dim_width;
		break;
	case CFG_MATCH_FILE_ARTWORK_SIZE_WIDTH:
		out = in_conf.match_file_artwork_size_width;
		break;
	//todo: depri
	case CFG_MATCH_DISCOGS_ARTWORK_STYLE:
		out = in_conf.match_discogs_artwork_art_style;
		break;
	case CFG_MATCH_FILE_ARTWORKS_STYLE:
		out = in_conf.match_file_artworks_art_style;
		break;
	//..
	case CFG_ALBUM_ART_SKIP_DEFAULT_CUST:
		out = in_conf.album_art_skip_default_cust;
		break;
	//v204
	case CFG_EDIT_TAGS_DIALOG_FLAGS:
		out = in_conf.edit_tags_dlg_flags;
		break;
	//v205
	case CFG_DC_DB_FLAG:
		out = 0;
		break;
	case CFG_FIND_RELEASE_FILTER_FLAG:
		out = in_conf.find_release_filter_flag;
		break;
	case CFG_SKIP_MNG_FLAG:
		out = in_conf.skip_mng_flag;
		break;
	case CFG_LIST_STYLE:
		out = in_conf.list_style;
		break;
	case CFG_HISTORY_MAX_ITEMS:
		out = in_conf.history_enabled_max;
		break;

	case CFG_FIND_RELEASE_DIALOG_FLAG:
		out = in_conf.find_release_dlg_flags;
		break;
	//v206
	case CFG_DISCOGS_ARTWORK_INDEX_WIDTH:
		out = in_conf.match_discogs_artwork_index_width;
		break;
	case CFG_MATCH_FILE_ARTWORK_INDEX_WIDTH:
		out = in_conf.match_file_artwork_index_width;
		break;
	case CFG_PREVIEW_LEADING_TAGS_DLG_COLS_WIDTH:
		out = in_conf.preview_leading_tags_dlg_cols_width;
		break;
	//v207
	case CFG_DISCOGS_ARTWORK_TL_RA_WIDTH:
		out = in_conf.match_discogs_artwork_tl_ra_width;
		break;
	case CFG_DISCOGS_ARTWORK_TL_TYPE_WIDTH:
		out = in_conf.match_discogs_artwork_tl_type_width;
		break;
	case CFG_DISCOGS_ARTWORK_TL_DIM_WIDTH:
		out = in_conf.match_discogs_artwork_tl_dim_width;
		break;
	case CFG_DISCOGS_ARTWORK_TL_SAVE_WIDTH:
		out = in_conf.match_discogs_artwork_tl_save_width;
		break;
	case CFG_DISCOGS_ARTWORK_TL_OVR_WIDTH:
		out = in_conf.match_discogs_artwork_tl_ovr_width;
		break;
	case CFG_DISCOGS_ARTWORK_TL_EMBED_WIDTH:
		out = in_conf.match_discogs_artwork_tl_embed_width;
		break;
	case CFG_DISCOGS_ARTWORK_TL_INDEX_WIDTH:
		out = in_conf.match_discogs_artwork_tl_index_width;
		break;

	case CFG_CUSTOM_FONT:
		out = in_conf.custom_font;
		break;

	//v209 (1.0.16.1)
	case CFG_ALT_WRITE_FLAGS:
		out = in_conf.alt_write_flags;
		break;
	//..
	default:
		if (assert) {
			PFC_ASSERT(false);
		}
		return false;
	}
	return true;
}

bool CConf::id_to_val_str(int id, const CConf& in_conf, pfc::string8& out, bool assert) {

	switch (id) {
	case CFG_ALBUM_ART_DIRECTORY_STRING: {
		pfc::string8 str(in_conf.album_art_directory_string);
		out.set_string(str, str.get_length());
		break;
	}
	case CFG_ALBUM_ART_FILENAME_STRING:
	{
		pfc::string8 str(in_conf.album_art_filename_string);
		out.set_string(str, str.get_length());
		break;
	}
	case CFG_ARTIST_ART_DIRECTORY_STRING:
	{
		pfc::string8 str(in_conf.artist_art_directory_string);
		out.set_string(str, str.get_length());
		break;
	}
	case CFG_ARTIST_ART_FILENAME_STRING:
	{
		pfc::string8 str(in_conf.artist_art_filename_string);
		out.set_string(str, str.get_length());
		break;
	}
	case CFG_ARTIST_ART_IDS_TF_STRING:
	{
		pfc::string8 str(in_conf.artist_art_id_format_string);
		out.set_string(str, str.get_length());
		break;
	}
	case CFG_OAUTH_TOKEN:
	{
		pfc::string8 str(in_conf.oauth_token);
		out.set_string(str, str.get_length());
		break;
	}
	case CFG_OAUTH_TOKEN_SECRET:
	{
		pfc::string8 str(in_conf.oauth_token_secret);
		out.set_string(str, str.get_length());
		break;
	}
	case CFG_REMOVE_EXCLUDE_TAGS:
	{
		pfc::string8 str(in_conf.raw_remove_exclude_tags);
		out.set_string(str, str.get_length());
		break;
	}
	case CFG_SEARCH_RELEASE_FORMAT_STRING:
	{
		pfc::string8 str(in_conf.search_release_format_string);
		out.set_string(str, str.get_length());
		break;
	}
	case CFG_SEARCH_MASTER_FORMAT_STRING:
	{
		pfc::string8 str(in_conf.search_master_format_string);
		out.set_string(str, str.get_length());
		break;
	}
	case CFG_SEARCH_MASTER_SUB_FORMAT_STRING:
	{
		pfc::string8 str(in_conf.search_master_sub_format_string);
		out.set_string(str, str.get_length());
		break;
	}
	case CFG_DISCOGS_TRACK_FORMAT_STRING:
	{
		pfc::string8 str(in_conf.release_discogs_format_string);
		out.set_string(str, str.get_length());
		break;
	}
	case CFG_FILE_TRACK_FORMAT_STRING:
	{
		pfc::string8 str(in_conf.release_file_format_string);
		out.set_string(str, str.get_length());
		break;
	}
	//v200
	case CFG_EDIT_TAGS_DIALOG_HL_KEYWORD:
	{
		pfc::string8 str(in_conf.edit_tags_dlg_hl_keyword);
		out.set_string(str, str.get_length());
		break;
	}
	//v205
	case CFG_DC_DB_PATH:
	{
		pfc::string8 str(in_conf.db_dc_path);
		out.set_string(str, str.get_length());
		break;
	}
	//v208
	case CFG_MULTIVALUE_FIELDS:
	{
		pfc::string8 str(in_conf.multivalue_fields);
		out.set_string(str, str.get_length());
		break;
	}
	//..

	default:
		if (assert) {
			PFC_ASSERT(false);
		}
		return false;
	}
	return true;
}

void CConf::save(cfgFilter cfgfilter, const CConf& in_conf) {

	for (unsigned int i = 0; i < cfg_bool_entries.get_count(); i++) {
		const conf_bool_entry& item = cfg_bool_entries[i];
		int id = item.id;

		auto filterok =
			std::find_if(idarray.begin(), idarray.end(), [&](const std::pair<int, int>& e) {
			return e.first == asi(cfgfilter) && e.second == id;
				});

		if (filterok != std::end(idarray)) {
			bool val;
			/*bool bres = */id_to_val_bool(id, in_conf, val);
			cfg_bool_entries.replace_item(i, make_conf_entry(id, val));
		}
	}

	for (unsigned int i = 0; i < cfg_int_entries.get_count(); i++) {
		const conf_int_entry& item = cfg_int_entries[i];
		int id = item.id;

		auto filterok =
			std::find_if(idarray.begin(), idarray.end(), [&](const std::pair<int, int>& e) {
			return e.first == asi(cfgfilter) && e.second == id;
				});

		if (filterok != std::end(idarray)) {
			int val;
			/*bool bres = */id_to_val_int(id, in_conf, val);
			cfg_int_entries.replace_item(i, make_conf_entry(id, val));
		}
	}

	for (unsigned int i = 0; i < cfg_string_entries.get_count(); i++) {
		const conf_string_entry& item = cfg_string_entries[i];
		int id = item.id;

		auto filterok =
			std::find_if(idarray.begin(), idarray.end(), [&](const std::pair<int, int>& e) {
			return e.first == asi(cfgfilter) && e.second == id;
				});

		if (filterok != std::end(idarray)) {
			pfc::string8 str;
			/*bool bres = */id_to_val_str(id, in_conf, str);
			cfg_string_entries.replace_item(i, make_conf_entry(id, str));
		}
	}
}

void CConf::save(cfgFilter cfgfilter, const CConf& in_conf, int id) {

	auto filterok =
		std::find_if(idarray.begin(), idarray.end(), [&](const std::pair<int, int>& e) {
		return e.first == asi(cfgfilter) && e.second == id;
			});
	if (filterok == std::end(idarray)) {
		//wrong cfgfilter
		return;
	}

	for (unsigned int i = 0; i < cfg_bool_entries.get_count(); i++) {
		const conf_bool_entry& item = cfg_bool_entries[i];
		if (item.id == id) {
			bool val;
			bool bres = id_to_val_bool(id, in_conf, val);
			cfg_bool_entries.replace_item(i, make_conf_entry(id, val));
			return;
		}
	}

	for (unsigned int i = 0; i < cfg_int_entries.get_count(); i++) {
		const conf_int_entry& item = cfg_int_entries[i];
		if (item.id == id) {
			int val;
			bool bres = id_to_val_int(id, in_conf, val);
			cfg_int_entries.replace_item(i, make_conf_entry(id, val));
			return;
		}
	}
	for (unsigned int i = 0; i < cfg_string_entries.get_count(); i++) {
		const conf_string_entry& item = cfg_string_entries[i];
		if (item.id == id) {
			pfc::string8 str;
			bool res = id_to_val_str(id, in_conf, str);
			cfg_string_entries.replace_item(i, make_conf_entry(id, str));
			return;
		}
	}
}

void CConf::save() {
	cfg_bool_entries.remove_all();
	cfg_bool_entries.add_item(make_conf_entry(CFG_REPLACE_ANVS, replace_ANVs));
	cfg_bool_entries.add_item(make_conf_entry(CFG_MOVE_THE_AT_BEGINNING, move_the_at_beginning));
	cfg_bool_entries.add_item(make_conf_entry(CFG_DISCARD_NUMERIC_SUFFIX, discard_numeric_suffix));
	cfg_bool_entries.add_item(make_conf_entry(CFG_MATCH_TRACKS_USING_DURATION, match_tracks_using_duration));
	cfg_bool_entries.add_item(make_conf_entry(CFG_SAVE_ALBUM_ART, save_album_art));
	cfg_bool_entries.add_item(make_conf_entry(CFG_SAVE_ARTIST_ART, save_artist_art));
	cfg_bool_entries.add_item(make_conf_entry(CFG_ALBUM_ART_FETCH_ALL, album_art_fetch_all));
	cfg_bool_entries.add_item(make_conf_entry(CFG_ALBUM_ART_EMBED, embed_album_art));
	cfg_bool_entries.add_item(make_conf_entry(CFG_ALBUM_ART_OVERWRITE, album_art_overwrite));
	cfg_bool_entries.add_item(make_conf_entry(CFG_ARTIST_ART_FETCH_ALL, artist_art_fetch_all));
	cfg_bool_entries.add_item(make_conf_entry(CFG_ARTIST_ART_EMBED, embed_artist_art));
	cfg_bool_entries.add_item(make_conf_entry(CFG_ARTIST_ART_OVERWRITE, artist_art_overwrite));
	cfg_bool_entries.add_item(make_conf_entry(CFG_DISPLAY_EXACT_MATCHES, display_exact_matches));
	cfg_bool_entries.add_item(make_conf_entry(CFG_ENABLE_AUTOSEARCH, enable_autosearch));
	cfg_bool_entries.add_item(make_conf_entry(CFG_DISPLAY_ANVS, display_ANVs));
	cfg_bool_entries.add_item(make_conf_entry(CFG_REMOVE_OTHER_TAGS, remove_other_tags));
	cfg_bool_entries.add_item(make_conf_entry(CFG_MATCH_TRACKS_USING_NUMBER, match_tracks_using_number));
	cfg_bool_entries.add_item(make_conf_entry(CFG_ASSUME_TRACKS_SORTED, assume_tracks_sorted));
	cfg_bool_entries.add_item(make_conf_entry(CFG_UPDATE_PREVIEW_CHANGES, update_tags_preview_changes));
	cfg_bool_entries.add_item(make_conf_entry(CFG_UPDATE_TAGS_MANUALLY_PROMPT, update_tags_manually_match));
	cfg_bool_entries.add_item(make_conf_entry(CFG_PARSE_HIDDEN_AS_REGULAR, parse_hidden_as_regular));
	cfg_bool_entries.add_item(make_conf_entry(CFG_SKIP_VIDEO_TRACKS, skip_video_tracks));
	//v200
	cfg_bool_entries.add_item(make_conf_entry(CFG_EDIT_TAGS_DIALOG_SHOW_TM_STATS, edit_tags_dlg_show_tm_stats));
	cfg_bool_entries.add_item(make_conf_entry(CFG_RELEASE_ENTER_KEY_OVR, release_enter_key_override));
	//v206
	cfg_bool_entries.add_item(make_conf_entry(CFG_AUTO_REL_LOAD_ON_OPEN, auto_rel_load_on_open));
	cfg_bool_entries.add_item(make_conf_entry(CFG_AUTO_REL_LOAD_ON_SELECT, auto_rel_load_on_select));
	cfg_bool_entries.add_item(make_conf_entry(CFG_PARSE_HIDDEN_MERGE_TITLES, parse_hidden_merge_titles));
	//..

	cfg_int_entries.remove_all();
	cfg_int_entries.add_item(make_conf_entry(CFG_TAGSAVE_FLAGS, tag_save_flags));
	cfg_int_entries.add_item(make_conf_entry(CFG_EDIT_TAGS_DIALOG_COL1_WIDTH, edit_tags_dialog_col1_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_EDIT_TAGS_DIALOG_COL2_WIDTH, edit_tags_dialog_col2_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_EDIT_TAGS_DIALOG_COL3_WIDTH, edit_tags_dialog_col3_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_LAST_CONF_TAB, last_conf_tab));
	cfg_int_entries.add_item(make_conf_entry(CFG_PREVIEW_MODE, preview_mode));
	cfg_int_entries.add_item(make_conf_entry(CFG_CACHE_MAX_OBJECTS, cache_max_objects));
	//v200
	cfg_int_entries.add_item(make_conf_entry(CFG_PREVIEW_TAGS_DIALOG_COL1_WIDTH, preview_tags_dialog_col1_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_PREVIEW_TAGS_DIALOG_COL2_WIDTH, preview_tags_dialog_col2_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_MATCH_TRACKS_DISCOGS_COL1_WIDTH, match_tracks_discogs_col1_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_MATCH_TRACKS_DISCOGS_COL2_WIDTH, match_tracks_discogs_col2_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_MATCH_TRACKS_FILES_COL1_WIDTH, match_tracks_files_col1_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_MATCH_TRACKS_FILES_COL2_WIDTH, match_tracks_files_col2_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_MATCH_TRACKS_DISCOGS_STYLE, match_tracks_discogs_style));
	cfg_int_entries.add_item(make_conf_entry(CFG_MATCH_TRACKS_FILES_STYLE, match_tracks_files_style));
	cfg_int_entries.add_item(make_conf_entry(CFG_PREVIEW_TAGS_DIALOG_W_WIDTH, preview_tags_dialog_w_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_PREVIEW_TAGS_DIALOG_U_WIDTH, preview_tags_dialog_u_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_PREVIEW_TAGS_DIALOG_S_WIDTH, preview_tags_dialog_s_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_PREVIEW_TAGS_DIALOG_E_WIDTH, preview_tags_dialog_e_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_CACHE_OFFLINE_CACHE_FLAG, cache_offline_cache_flag));
	cfg_int_entries.add_item(make_conf_entry(CFG_DISCOGS_ARTWORK_RA_WIDTH, match_discogs_artwork_ra_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_DISCOGS_ARTWORK_TYPE_WIDTH, match_discogs_artwork_type_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_DISCOGS_ARTWORK_DIM_WIDTH, match_discogs_artwork_dim_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_DISCOGS_ARTWORK_SAVE_WIDTH, match_discogs_artwork_save_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_DISCOGS_ARTWORK_OVR_WIDTH, match_discogs_artwork_ovr_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_DISCOGS_ARTWORK_EMBED_WIDTH, match_discogs_artwork_embed_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_MATCH_FILE_ARTWORK_NAME_WIDTH, match_file_artwork_name_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_MATCH_FILE_ARTWORK_DIM_WIDTH, match_file_artwork_dim_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_MATCH_FILE_ARTWORK_SIZE_WIDTH, match_file_artwork_size_width));
	//todo: depri
	cfg_int_entries.add_item(make_conf_entry(CFG_MATCH_DISCOGS_ARTWORK_STYLE, match_discogs_artwork_art_style));
	cfg_int_entries.add_item(make_conf_entry(CFG_MATCH_FILE_ARTWORKS_STYLE, match_file_artworks_art_style));
	//..
	cfg_int_entries.add_item(make_conf_entry(CFG_ALBUM_ART_SKIP_DEFAULT_CUST, album_art_skip_default_cust));
	//v204
	cfg_int_entries.add_item(make_conf_entry(CFG_EDIT_TAGS_DIALOG_FLAGS, edit_tags_dlg_flags));
	//v205
	cfg_int_entries.add_item(make_conf_entry(CFG_DC_DB_FLAG, 0));
	cfg_int_entries.add_item(make_conf_entry(CFG_FIND_RELEASE_FILTER_FLAG, find_release_filter_flag));
	cfg_int_entries.add_item(make_conf_entry(CFG_SKIP_MNG_FLAG, skip_mng_flag));
	cfg_int_entries.add_item(make_conf_entry(CFG_LIST_STYLE, list_style));
	cfg_int_entries.add_item(make_conf_entry(CFG_HISTORY_MAX_ITEMS, history_enabled_max));

	cfg_int_entries.add_item(make_conf_entry(CFG_FIND_RELEASE_DIALOG_FLAG, find_release_dlg_flags));
	//v206
	cfg_int_entries.add_item(make_conf_entry(CFG_DISCOGS_ARTWORK_INDEX_WIDTH, match_discogs_artwork_index_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_MATCH_FILE_ARTWORK_INDEX_WIDTH, match_file_artwork_index_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_PREVIEW_LEADING_TAGS_DLG_COLS_WIDTH, this->preview_leading_tags_dlg_cols_width));
	//v207
	cfg_int_entries.add_item(make_conf_entry(CFG_DISCOGS_ARTWORK_TL_RA_WIDTH, match_discogs_artwork_tl_ra_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_DISCOGS_ARTWORK_TL_TYPE_WIDTH, match_discogs_artwork_tl_type_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_DISCOGS_ARTWORK_TL_DIM_WIDTH, match_discogs_artwork_tl_dim_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_DISCOGS_ARTWORK_TL_SAVE_WIDTH, match_discogs_artwork_tl_save_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_DISCOGS_ARTWORK_TL_OVR_WIDTH, match_discogs_artwork_tl_ovr_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_DISCOGS_ARTWORK_TL_EMBED_WIDTH, match_discogs_artwork_tl_embed_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_DISCOGS_ARTWORK_TL_INDEX_WIDTH, match_discogs_artwork_tl_index_width));

	cfg_int_entries.add_item(make_conf_entry(CFG_CUSTOM_FONT, custom_font));
	//v209 (1.0.16.1)
	cfg_int_entries.add_item(make_conf_entry(CFG_ALT_WRITE_FLAGS, alt_write_flags));
	//..

	cfg_string_entries.remove_all();
	cfg_string_entries.add_item(make_conf_entry(CFG_ALBUM_ART_DIRECTORY_STRING, pfc::string8((const char*)album_art_directory_string)));
	cfg_string_entries.add_item(make_conf_entry(CFG_ALBUM_ART_FILENAME_STRING, pfc::string8((const char*)album_art_filename_string)));
	cfg_string_entries.add_item(make_conf_entry(CFG_ARTIST_ART_DIRECTORY_STRING, pfc::string8((const char*)artist_art_directory_string)));
	cfg_string_entries.add_item(make_conf_entry(CFG_ARTIST_ART_FILENAME_STRING, pfc::string8((const char*)artist_art_filename_string)));
	cfg_string_entries.add_item(make_conf_entry(CFG_OAUTH_TOKEN, oauth_token));
	cfg_string_entries.add_item(make_conf_entry(CFG_OAUTH_TOKEN_SECRET, oauth_token_secret));
	cfg_string_entries.add_item(make_conf_entry(CFG_REMOVE_EXCLUDE_TAGS, raw_remove_exclude_tags));
	cfg_string_entries.add_item(make_conf_entry(CFG_SEARCH_RELEASE_FORMAT_STRING, pfc::string8((const char*)search_release_format_string)));
	cfg_string_entries.add_item(make_conf_entry(CFG_SEARCH_MASTER_FORMAT_STRING, pfc::string8((const char*)search_master_format_string)));
	cfg_string_entries.add_item(make_conf_entry(CFG_SEARCH_MASTER_SUB_FORMAT_STRING, pfc::string8((const char*)search_master_sub_format_string)));
	cfg_string_entries.add_item(make_conf_entry(CFG_DISCOGS_TRACK_FORMAT_STRING, pfc::string8((const char*)release_discogs_format_string)));
	cfg_string_entries.add_item(make_conf_entry(CFG_FILE_TRACK_FORMAT_STRING, pfc::string8((const char*)release_file_format_string)));
	cfg_string_entries.add_item(make_conf_entry(CFG_ARTIST_ART_IDS_TF_STRING, pfc::string8((const char*)artist_art_id_format_string)));
	//v200
	cfg_string_entries.add_item(make_conf_entry(CFG_EDIT_TAGS_DIALOG_HL_KEYWORD, pfc::string8((const char*)edit_tags_dlg_hl_keyword)));
	//v205
	cfg_string_entries.add_item(make_conf_entry(CFG_DC_DB_PATH, pfc::string8((const char*)db_dc_path)));
	//v208
	cfg_string_entries.add_item(make_conf_entry(CFG_MULTIVALUE_FIELDS, pfc::string8((const char*)multivalue_fields)));
	//..
}

void CConf::save_active_config_tab(int newtab) {
	const conf_int_entry& entry = make_conf_entry(CFG_LAST_CONF_TAB, last_conf_tab);
	auto pos = cfg_int_entries.find_item(entry);

	if (pos != pfc_infinite) {
		conf_int_entry new_entry = make_conf_entry(CFG_LAST_CONF_TAB, newtab);
		cfg_int_entries.swap_item_with(pos, new_entry);
		last_conf_tab = newtab;
	}
}

void CConf::save_preview_modal_tab_widths(int newwidths) {

	const conf_int_entry& entry = make_conf_entry(CFG_PREVIEW_LEADING_TAGS_DLG_COLS_WIDTH, preview_leading_tags_dlg_cols_width);
	auto pos = cfg_int_entries.find_item(entry);

	if (pos != pfc_infinite) {
		conf_int_entry new_entry = make_conf_entry(CFG_PREVIEW_LEADING_TAGS_DLG_COLS_WIDTH, newwidths);
		cfg_int_entries.swap_item_with(pos, new_entry);
		preview_leading_tags_dlg_cols_width = newwidths;
	}
}

bool CConf::history_enabled() {
	return HIWORD(history_enabled_max);
}

bool CConf::history_max() {
	return LOWORD(history_enabled_max);
}

bool CConf::awt_mode_changing() {
	return LOWORD(mode_write_alt) != HIWORD(mode_write_alt);
}

bool CConf::awt_alt_mode() {
	return HIWORD(CONF.mode_write_alt);
}

#ifdef SIM_VA_MA_BETA
void g_clear_va_ma_releases() {
	if (discogs_interface) {
		for (auto w : g_va_ma_releases) {
			discogs_interface->delete_artist_cache("", w.c_str());
		}
		g_va_ma_releases.clear();
	}
}
#endif