#include "stdafx.h"

#include "conf.h"

new_conf CONF;

// {82CB353B-6D23-4D3B-86E1-A5D80EC9CC53}
static const GUID guid_cfg_bool_values = {0x82cb353b, 0x6d23, 0x4d3b, {0x86, 0xe1, 0xa5, 0xd8, 0xe, 0xc9, 0xcc, 0x53}};

// {D80DC54E-02E0-4E20-8207-0726D1497188}
static const GUID guid_cfg_int_values = {0xd80dc54e, 0x2e0, 0x4e20, {0x82, 0x7, 0x7, 0x26, 0xd1, 0x49, 0x71, 0x88}};

// {0DDFE396-B4DB-414B-8EC0-D4D05A2BB7F7}
static const GUID guid_cfg_string_values = {0xddfe396, 0xb4db, 0x414b, {0x8e, 0xc0, 0xd4, 0xd0, 0x5a, 0x2b, 0xb7, 0xf7}};

cfg_objList<conf_bool_entry> cfg_bool_entries(guid_cfg_bool_values);
cfg_objList<conf_int_entry> cfg_int_entries(guid_cfg_int_values);
cfg_objList<conf_string_entry> cfg_string_entries(guid_cfg_string_values);


conf_bool_entry make_conf_entry(int i, bool v) {
	conf_bool_entry result;
	result.id = i;
	result.value = v;
	return result;
}

conf_int_entry make_conf_entry(int i, int v) {
	conf_int_entry result;
	result.id = i;
	result.value = v;
	return result;
}

conf_string_entry make_conf_entry(int i, const pfc::string8 &v) {
	conf_string_entry result;
	result.id = i;
	result.value = v;
	return result;
}

bool new_conf::load() {
	bool changed = false;
	// { specs vector, boolvals, intvals, stringvals;
	vspec v000 { &vec_specs, 0, 0, 0 };
	vspec v199 { &vec_specs, 25, 16, 13 };
	vspec v200 { &vec_specs, 27, 22, 14 };
	vspec v201 { &vec_specs, 28, 28, 14 };
	vspec v202 { &vec_specs, 28, 29, 14 };
	
	vspec* vlast = &vec_specs.at(vec_specs.size() - 1);

	vspec vLoad = { nullptr, cfg_bool_entries.get_count(),
		cfg_int_entries.get_count(), cfg_string_entries.get_count() };

	if (!(vLoad == v000) && vLoad > *vlast) {
		//version not found
		//safety reset
		vLoad = *vlast;
		pfc::string8 title;
		title << "Configuration Reset";
		pfc::string8 msg("This version of foo_discogs is not compatible with the current setup.\n");
		msg << "Configuration will be reset.\n",
		msg << "Please backup your configuration file before continuing.\n",

		uMessageBox(core_api::get_main_window(), msg, title,
			MB_APPLMODAL | MB_ICONASTERISK);
	}

	for (unsigned int i = 0; i < cfg_bool_entries.get_count(); i++) {
		changed = true;
		const conf_bool_entry &item = cfg_bool_entries[i];
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
			case CFG_SKIP_RELEASE_DLG_IF_MATCHED:
				skip_release_dialog_if_matched = item.value;
				break;
			case CFG_SKIP_FIND_RELEASE_DLG_IF_IDED:
				skip_find_release_dialog_if_ided = item.value;
				break;
			case CFG_SKIP_PREVIEW_DIALOG:
				skip_preview_dialog = item.value;
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
			case CFG_FIND_RELEASE_DIALOG_SHOW_ID:
				find_release_dialog_show_id = item.value;
				break;
			//v201
			case CFG_RELEASE_ENTER_KEY_OVR:
				release_enter_key_override = item.value;

		}
	}
	if (vLoad < v200) {
		cfg_bool_entries.add_item(make_conf_entry(CFG_EDIT_TAGS_DIALOG_SHOW_TM_STATS, edit_tags_dlg_show_tm_stats));
		cfg_bool_entries.add_item(make_conf_entry(CFG_FIND_RELEASE_DIALOG_SHOW_ID, find_release_dialog_show_id));
	}
	if (vLoad < v201) {
		cfg_bool_entries.add_item(make_conf_entry(CFG_RELEASE_ENTER_KEY_OVR, release_enter_key_override));
	}

	for (unsigned int i = 0; i < cfg_int_entries.get_count(); i++) {
		changed = true; 
		const conf_int_entry &item = cfg_int_entries[i];
		switch (item.id) {
			case CFG_UPDATE_ART_FLAGS:
				update_art_flags = item.value;
				break;
			case CFG_FIND_RELEASE_DIALOG_WIDTH:
				find_release_dialog_width = item.value;
				break;
			case CFG_FIND_RELEASE_DIALOG_HEIGHT:
				find_release_dialog_height = item.value;
				break;
			case CFG_RELEASE_DIALOG_WIDTH:
				release_dialog_width = item.value;
				break;
			case CFG_RELEASE_DIALOG_HEIGHT:
				release_dialog_height = item.value;
				break;
			case CFG_EDIT_TAGS_DIALOG_WIDTH:
				edit_tags_dialog_width = item.value;
				break;
			case CFG_EDIT_TAGS_DIALOG_HEIGHT:
				edit_tags_dialog_height = item.value;
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
			case CFG_PREVIEW_DIALOG_WIDTH:
				preview_tags_dialog_width = item.value;
				break;
			case CFG_PREVIEW_DIALOG_HEIGHT:
				preview_tags_dialog_height = item.value;
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
			//v200 (mod v16)
			case CFG_PREVIEW_TAGS_DIALOG_COL1_WIDTH:
				preview_tags_dialog_col1_width = item.value;
				break;
			case CFG_PREVIEW_TAGS_DIALOG_COL2_WIDTH:
				preview_tags_dialog_col2_width = item.value;
				break;
			case CFG_MATCH_TRACKS_DISCOGS_COL1_WIDTH:
				match_tracks_dialog_discogs_col1_width = item.value;
				break;
			case CFG_MATCH_TRACKS_DISCOGS_COL2_WIDTH:
				match_tracks_dialog_discogs_col2_width = item.value;
				break;
			case CFG_MATCH_TRACKS_FILES_COL1_WIDTH:
				match_tracks_dialog_files_col1_width = item.value;
				break;
			case CFG_MATCH_TRACKS_FILES_COL2_WIDTH:
				match_tracks_dialog_files_col2_width = item.value;
				break;
			//v201 (mod v17)
			case CFG_MATCH_TRACKS_DISCOGS_STYLE:
				match_tracks_dialog_discogs_style = item.value;
				break;
			case CFG_MATCH_TRACKS_FILES_STYLE:
				match_tracks_dialog_files_style = item.value;
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
				//v202
			case CFG_CACHE_USE_OFFLINE_CACHE:
				cache_use_offline_cache = item.value;
				break;
		}
	}
	if (vLoad < v200) {
		//v200
		cfg_int_entries.add_item(make_conf_entry(CFG_PREVIEW_TAGS_DIALOG_COL1_WIDTH, preview_tags_dialog_col1_width));
		cfg_int_entries.add_item(make_conf_entry(CFG_PREVIEW_TAGS_DIALOG_COL2_WIDTH, preview_tags_dialog_col2_width));
		cfg_int_entries.add_item(make_conf_entry(CFG_MATCH_TRACKS_DISCOGS_COL1_WIDTH, match_tracks_dialog_discogs_col1_width));
		cfg_int_entries.add_item(make_conf_entry(CFG_MATCH_TRACKS_DISCOGS_COL2_WIDTH, match_tracks_dialog_discogs_col2_width));
		cfg_int_entries.add_item(make_conf_entry(CFG_MATCH_TRACKS_FILES_COL1_WIDTH, match_tracks_dialog_files_col1_width));
		cfg_int_entries.add_item(make_conf_entry(CFG_MATCH_TRACKS_FILES_COL2_WIDTH, match_tracks_dialog_files_col2_width));
	}
	if (vLoad < v201) {
		//v201
		cfg_int_entries.add_item(make_conf_entry(CFG_MATCH_TRACKS_DISCOGS_STYLE, match_tracks_dialog_discogs_style));
		cfg_int_entries.add_item(make_conf_entry(CFG_MATCH_TRACKS_FILES_STYLE, match_tracks_dialog_files_style));
		cfg_int_entries.add_item(make_conf_entry(CFG_PREVIEW_TAGS_DIALOG_W_WIDTH, preview_tags_dialog_w_width));
		cfg_int_entries.add_item(make_conf_entry(CFG_PREVIEW_TAGS_DIALOG_U_WIDTH, preview_tags_dialog_u_width));
		cfg_int_entries.add_item(make_conf_entry(CFG_PREVIEW_TAGS_DIALOG_S_WIDTH, preview_tags_dialog_s_width));
		cfg_int_entries.add_item(make_conf_entry(CFG_PREVIEW_TAGS_DIALOG_E_WIDTH, preview_tags_dialog_e_width));
	}
	if (vLoad < v202) {
		cfg_int_entries.add_item(make_conf_entry(CFG_CACHE_USE_OFFLINE_CACHE, cache_use_offline_cache));
	}

	for (unsigned int i = 0; i < cfg_string_entries.get_count(); i++) {
		changed = true; 
		const conf_string_entry &item = cfg_string_entries[i];
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
			case CFG_ARTIST_ART_IDS_TITLEFORMAT:
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

		}
	}
	if (vLoad < v200) {
		//CONF_FILTER_PREVIEW
		cfg_string_entries.add_item(make_conf_entry(CFG_EDIT_TAGS_DIALOG_HL_KEYWORD, edit_tags_dlg_hl_keyword));
	}

	return changed;
}

bool new_conf::id_to_val_bool(int id, new_conf in_conf) {
	
	switch (id) {
		case CFG_REPLACE_ANVS:
			return in_conf.replace_ANVs;
		case CFG_MOVE_THE_AT_BEGINNING:
			return in_conf.move_the_at_beginning;
		case CFG_DISCARD_NUMERIC_SUFFIX:
			return in_conf.discard_numeric_suffix;
		case CFG_MATCH_TRACKS_USING_DURATION:
			return in_conf.match_tracks_using_duration;
		case CFG_MATCH_TRACKS_USING_NUMBER:
			return in_conf.match_tracks_using_number;
		case CFG_SKIP_RELEASE_DLG_IF_MATCHED:
			return in_conf.skip_release_dialog_if_matched;
		case CFG_SKIP_FIND_RELEASE_DLG_IF_IDED:
			return in_conf.skip_find_release_dialog_if_ided;
		case CFG_SKIP_PREVIEW_DIALOG:
			return in_conf.skip_preview_dialog;
		case CFG_ASSUME_TRACKS_SORTED:
			return in_conf.assume_tracks_sorted;
		case CFG_SAVE_ALBUM_ART:
			return in_conf.save_album_art;
		case CFG_SAVE_ARTIST_ART:
			return in_conf.save_artist_art;
		case CFG_ALBUM_ART_EMBED:
			return in_conf.embed_album_art;
		case CFG_ARTIST_ART_EMBED:
			return in_conf.embed_artist_art;
		case CFG_ALBUM_ART_FETCH_ALL:
			return in_conf.album_art_fetch_all;
		case CFG_ALBUM_ART_OVERWRITE:
			return in_conf.album_art_overwrite;
		case CFG_ARTIST_ART_FETCH_ALL:
			return in_conf.artist_art_fetch_all;
		case CFG_ARTIST_ART_OVERWRITE:
			return in_conf.artist_art_overwrite;
		case CFG_DISPLAY_EXACT_MATCHES:
			return in_conf.display_exact_matches;
		case CFG_ENABLE_AUTOSEARCH:
			return in_conf.enable_autosearch;
		case CFG_DISPLAY_ANVS:
			return in_conf.display_ANVs;
		case CFG_REMOVE_OTHER_TAGS:
			return in_conf.remove_other_tags;
		case CFG_UPDATE_PREVIEW_CHANGES:
			return in_conf.update_tags_preview_changes;
		case CFG_UPDATE_TAGS_MANUALLY_PROMPT:
			return in_conf.update_tags_manually_match;
		case CFG_PARSE_HIDDEN_AS_REGULAR:
			return in_conf.parse_hidden_as_regular;
		case CFG_SKIP_VIDEO_TRACKS:
			return in_conf.skip_video_tracks;
		//v200
		case CFG_EDIT_TAGS_DIALOG_SHOW_TM_STATS:
			return in_conf.edit_tags_dlg_show_tm_stats;
		case CFG_FIND_RELEASE_DIALOG_SHOW_ID:
			return in_conf.find_release_dialog_show_id;
		//v201
		case CFG_RELEASE_ENTER_KEY_OVR:
			return in_conf.release_enter_key_override;
	}
	PFC_ASSERT(false);
	return false;
}

int new_conf::id_to_val_int(int id, new_conf in_conf) {
	int iret = 0;

	switch (id) {
		case CFG_UPDATE_ART_FLAGS:
			return in_conf.update_art_flags;
		case CFG_FIND_RELEASE_DIALOG_WIDTH:
			return in_conf.find_release_dialog_width;
		case CFG_FIND_RELEASE_DIALOG_HEIGHT:
			return in_conf.find_release_dialog_height;
		case CFG_RELEASE_DIALOG_WIDTH:
			return in_conf.release_dialog_width;
		case CFG_RELEASE_DIALOG_HEIGHT:
			return in_conf.release_dialog_height;
		case CFG_EDIT_TAGS_DIALOG_WIDTH:
			return in_conf.edit_tags_dialog_width;
		case CFG_EDIT_TAGS_DIALOG_HEIGHT:
			return in_conf.edit_tags_dialog_height;
		case CFG_EDIT_TAGS_DIALOG_COL1_WIDTH:
			return in_conf.edit_tags_dialog_col1_width;
		case CFG_EDIT_TAGS_DIALOG_COL2_WIDTH:
			return in_conf.edit_tags_dialog_col2_width;
		case CFG_EDIT_TAGS_DIALOG_COL3_WIDTH:
			return in_conf.edit_tags_dialog_col3_width;
		case CFG_PREVIEW_DIALOG_WIDTH:
			return in_conf.preview_tags_dialog_width;
		case CFG_PREVIEW_DIALOG_HEIGHT:
			return in_conf.preview_tags_dialog_height;
		case CFG_LAST_CONF_TAB:
			return in_conf.last_conf_tab;
		case CFG_PREVIEW_MODE:
			return in_conf.preview_mode;
		case CFG_CACHE_MAX_OBJECTS:
			return in_conf.cache_max_objects;
		//v200
		case CFG_PREVIEW_TAGS_DIALOG_COL1_WIDTH:
			return in_conf.preview_tags_dialog_col1_width;
		case CFG_PREVIEW_TAGS_DIALOG_COL2_WIDTH:
			return in_conf.preview_tags_dialog_col2_width;
		case CFG_MATCH_TRACKS_DISCOGS_COL1_WIDTH:
			return in_conf.match_tracks_dialog_discogs_col1_width;
		case CFG_MATCH_TRACKS_DISCOGS_COL2_WIDTH:
			return in_conf.match_tracks_dialog_discogs_col2_width;
		case CFG_MATCH_TRACKS_FILES_COL1_WIDTH:
			return in_conf.match_tracks_dialog_files_col1_width;
		case CFG_MATCH_TRACKS_FILES_COL2_WIDTH:
			return in_conf.match_tracks_dialog_files_col2_width;
		//v201
		case CFG_MATCH_TRACKS_DISCOGS_STYLE:
			return in_conf.match_tracks_dialog_discogs_style;
		case CFG_MATCH_TRACKS_FILES_STYLE:
			return in_conf.match_tracks_dialog_files_style;
		case CFG_PREVIEW_TAGS_DIALOG_W_WIDTH:
			return in_conf.preview_tags_dialog_w_width;
		case CFG_PREVIEW_TAGS_DIALOG_U_WIDTH:
			return in_conf.preview_tags_dialog_u_width;
		case CFG_PREVIEW_TAGS_DIALOG_S_WIDTH:
			return in_conf.preview_tags_dialog_s_width;
		case CFG_PREVIEW_TAGS_DIALOG_E_WIDTH:
			return in_conf.preview_tags_dialog_e_width;
		//v202
		case CFG_CACHE_USE_OFFLINE_CACHE:
			return in_conf.cache_use_offline_cache;
		//..
	}
	PFC_ASSERT(false);
	return iret;
}

void new_conf::id_to_val_str(int id, new_conf in_conf, pfc::string8& out) {
	
	switch (id) {
		case CFG_ALBUM_ART_DIRECTORY_STRING: {
			pfc::string8 str(in_conf.album_art_directory_string);
			out.set_string(str, str.get_length());
			return;
		}
		case CFG_ALBUM_ART_FILENAME_STRING:
		{
			pfc::string8 str(in_conf.album_art_filename_string);
			out.set_string(str, str.get_length());
			return;
		}
		case CFG_ARTIST_ART_DIRECTORY_STRING:
		{
			pfc::string8 str(in_conf.artist_art_directory_string);
			out.set_string(str, str.get_length());
			return;
		}
		case CFG_ARTIST_ART_FILENAME_STRING:
		{
			pfc::string8 str(in_conf.artist_art_filename_string);
			out.set_string(str, str.get_length());
			return;
		}
		case CFG_ARTIST_ART_IDS_TITLEFORMAT:
		{
			pfc::string8 str(in_conf.artist_art_id_format_string);
			out.set_string(str, str.get_length());
			return;
		}
		case CFG_OAUTH_TOKEN:
		{
			pfc::string8 str(in_conf.oauth_token);
			out.set_string(str, str.get_length());
			return;
		}
		case CFG_OAUTH_TOKEN_SECRET:
		{
			pfc::string8 str(in_conf.oauth_token_secret);
			out.set_string(str, str.get_length());
			return;
		}
		case CFG_REMOVE_EXCLUDE_TAGS:
		{
			pfc::string8 str(in_conf.raw_remove_exclude_tags);
			out.set_string(str, str.get_length());
			return;
		}
		case CFG_SEARCH_RELEASE_FORMAT_STRING:
		{
			pfc::string8 str(in_conf.search_release_format_string);
			out.set_string(str, str.get_length());
			return;
		}
		case CFG_SEARCH_MASTER_FORMAT_STRING:
		{
			pfc::string8 str(in_conf.search_master_format_string);
			out.set_string(str, str.get_length());
			return;
		}
		case CFG_SEARCH_MASTER_SUB_FORMAT_STRING:
		{
			pfc::string8 str(in_conf.search_master_sub_format_string);
			out.set_string(str, str.get_length());
			return;
		}
		case CFG_DISCOGS_TRACK_FORMAT_STRING:
		{
			pfc::string8 str(in_conf.release_discogs_format_string);
			out.set_string(str, str.get_length());
			return;
		}
		case CFG_FILE_TRACK_FORMAT_STRING:
		{
			pfc::string8 str(in_conf.release_file_format_string);
			out.set_string(str, str.get_length());
			return;
		}
		//v200
		case CFG_EDIT_TAGS_DIALOG_HL_KEYWORD:
		{
			pfc::string8 str(in_conf.edit_tags_dlg_hl_keyword);
			out.set_string(str, str.get_length());
			return;
		}

	}
	PFC_ASSERT(false);
}

void new_conf::save(ConfFilter cfgfilter, new_conf in_conf) {

	for (unsigned int i = 0; i < cfg_bool_entries.get_count(); i++) {
		const conf_bool_entry& item = cfg_bool_entries[i];
		int id = item.id;

		auto filterok =
			std::find_if(idarray.begin(), idarray.end(), [&](const std::pair<int, int>& e) {
				return e.first == asi(cfgfilter) && e.second == id;
			});

		if (filterok != std::end(idarray)) {
			bool bres = id_to_val_bool(id, in_conf);	
			cfg_bool_entries.replace_item(i, make_conf_entry(id, bres));
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
			cfg_int_entries.replace_item(i, make_conf_entry(id, (int)id_to_val_int(id, in_conf)));
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
			id_to_val_str(id, in_conf, str);			
			cfg_string_entries.replace_item(i, make_conf_entry(id, str));
		}
	}
}

void new_conf::save(ConfFilter cfgfilter, new_conf in_conf, int id) {
	
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
			bool bres = id_to_val_bool(id, in_conf);
			cfg_bool_entries.replace_item(i, make_conf_entry(id, bres));
			return;
		}
	}

	for (unsigned int i = 0; i < cfg_int_entries.get_count(); i++) {
		const conf_int_entry& item = cfg_int_entries[i];
		if (item.id == id) {
			cfg_int_entries.replace_item(i, make_conf_entry(id, id_to_val_int(id, in_conf)));
			return;
		}
	}
	for (unsigned int i = 0; i < cfg_string_entries.get_count(); i++) {
		const conf_string_entry& item = cfg_string_entries[i];
		if (item.id == id) {
			pfc::string8 str;
			id_to_val_str(id, in_conf, str);
			cfg_string_entries.replace_item(i, make_conf_entry(id, str));
			return;
		}
	}
}

void new_conf::save() {
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
	cfg_bool_entries.add_item(make_conf_entry(CFG_SKIP_RELEASE_DLG_IF_MATCHED, skip_release_dialog_if_matched));
	cfg_bool_entries.add_item(make_conf_entry(CFG_SKIP_FIND_RELEASE_DLG_IF_IDED, skip_find_release_dialog_if_ided));
	cfg_bool_entries.add_item(make_conf_entry(CFG_SKIP_PREVIEW_DIALOG, skip_preview_dialog));
	cfg_bool_entries.add_item(make_conf_entry(CFG_UPDATE_PREVIEW_CHANGES, update_tags_preview_changes));
	cfg_bool_entries.add_item(make_conf_entry(CFG_UPDATE_TAGS_MANUALLY_PROMPT, update_tags_manually_match));
	cfg_bool_entries.add_item(make_conf_entry(CFG_PARSE_HIDDEN_AS_REGULAR, parse_hidden_as_regular));
	cfg_bool_entries.add_item(make_conf_entry(CFG_SKIP_VIDEO_TRACKS, skip_video_tracks));
	//v200
	cfg_bool_entries.add_item(make_conf_entry(CFG_EDIT_TAGS_DIALOG_SHOW_TM_STATS, edit_tags_dlg_show_tm_stats));
	cfg_bool_entries.add_item(make_conf_entry(CFG_FIND_RELEASE_DIALOG_SHOW_ID, find_release_dialog_show_id));
	//v201
	cfg_bool_entries.add_item(make_conf_entry(CFG_RELEASE_ENTER_KEY_OVR, release_enter_key_override));
	
	cfg_int_entries.remove_all();
	cfg_int_entries.add_item(make_conf_entry(CFG_UPDATE_ART_FLAGS, update_art_flags));
	cfg_int_entries.add_item(make_conf_entry(CFG_FIND_RELEASE_DIALOG_WIDTH, find_release_dialog_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_FIND_RELEASE_DIALOG_HEIGHT, find_release_dialog_height));
	cfg_int_entries.add_item(make_conf_entry(CFG_RELEASE_DIALOG_WIDTH, release_dialog_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_RELEASE_DIALOG_HEIGHT, release_dialog_height));
	cfg_int_entries.add_item(make_conf_entry(CFG_EDIT_TAGS_DIALOG_WIDTH, edit_tags_dialog_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_EDIT_TAGS_DIALOG_HEIGHT, edit_tags_dialog_height));
	cfg_int_entries.add_item(make_conf_entry(CFG_EDIT_TAGS_DIALOG_COL1_WIDTH, edit_tags_dialog_col1_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_EDIT_TAGS_DIALOG_COL2_WIDTH, edit_tags_dialog_col2_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_EDIT_TAGS_DIALOG_COL3_WIDTH, edit_tags_dialog_col3_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_EDIT_TAGS_DIALOG_HEIGHT, edit_tags_dialog_height));
	cfg_int_entries.add_item(make_conf_entry(CFG_PREVIEW_DIALOG_WIDTH, preview_tags_dialog_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_PREVIEW_DIALOG_HEIGHT, preview_tags_dialog_height));
	cfg_int_entries.add_item(make_conf_entry(CFG_LAST_CONF_TAB, last_conf_tab));
	cfg_int_entries.add_item(make_conf_entry(CFG_PREVIEW_MODE, preview_mode));
	cfg_int_entries.add_item(make_conf_entry(CFG_CACHE_MAX_OBJECTS, cache_max_objects));
	//v200
	cfg_int_entries.add_item(make_conf_entry(CFG_PREVIEW_TAGS_DIALOG_COL1_WIDTH, preview_tags_dialog_col1_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_PREVIEW_TAGS_DIALOG_COL2_WIDTH, preview_tags_dialog_col2_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_MATCH_TRACKS_DISCOGS_COL1_WIDTH, match_tracks_dialog_discogs_col1_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_MATCH_TRACKS_DISCOGS_COL2_WIDTH, match_tracks_dialog_discogs_col2_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_MATCH_TRACKS_FILES_COL1_WIDTH, match_tracks_dialog_files_col1_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_MATCH_TRACKS_FILES_COL2_WIDTH, match_tracks_dialog_files_col2_width));
	//v201
	cfg_int_entries.add_item(make_conf_entry(CFG_MATCH_TRACKS_DISCOGS_STYLE, match_tracks_dialog_discogs_style));
	cfg_int_entries.add_item(make_conf_entry(CFG_MATCH_TRACKS_FILES_STYLE, match_tracks_dialog_files_style));
	cfg_int_entries.add_item(make_conf_entry(CFG_PREVIEW_TAGS_DIALOG_W_WIDTH, preview_tags_dialog_w_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_PREVIEW_TAGS_DIALOG_U_WIDTH, preview_tags_dialog_u_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_PREVIEW_TAGS_DIALOG_S_WIDTH, preview_tags_dialog_s_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_PREVIEW_TAGS_DIALOG_E_WIDTH, preview_tags_dialog_e_width));
	//v202
	cfg_int_entries.add_item(make_conf_entry(CFG_CACHE_USE_OFFLINE_CACHE, cache_use_offline_cache));

	//..

	cfg_string_entries.remove_all();
	cfg_string_entries.add_item(make_conf_entry(CFG_ALBUM_ART_DIRECTORY_STRING, pfc::string8((const char *)album_art_directory_string)));
	cfg_string_entries.add_item(make_conf_entry(CFG_ALBUM_ART_FILENAME_STRING, pfc::string8((const char *)album_art_filename_string)));
	cfg_string_entries.add_item(make_conf_entry(CFG_ARTIST_ART_DIRECTORY_STRING, pfc::string8((const char *)artist_art_directory_string)));
	cfg_string_entries.add_item(make_conf_entry(CFG_ARTIST_ART_FILENAME_STRING, pfc::string8((const char *)artist_art_filename_string)));
	cfg_string_entries.add_item(make_conf_entry(CFG_OAUTH_TOKEN, oauth_token));
	cfg_string_entries.add_item(make_conf_entry(CFG_OAUTH_TOKEN_SECRET, oauth_token_secret));
	cfg_string_entries.add_item(make_conf_entry(CFG_REMOVE_EXCLUDE_TAGS, raw_remove_exclude_tags));
	cfg_string_entries.add_item(make_conf_entry(CFG_SEARCH_RELEASE_FORMAT_STRING, pfc::string8((const char *)search_release_format_string)));
	cfg_string_entries.add_item(make_conf_entry(CFG_SEARCH_MASTER_FORMAT_STRING, pfc::string8((const char *)search_master_format_string)));
	cfg_string_entries.add_item(make_conf_entry(CFG_SEARCH_MASTER_SUB_FORMAT_STRING, pfc::string8((const char *)search_master_sub_format_string)));
	cfg_string_entries.add_item(make_conf_entry(CFG_DISCOGS_TRACK_FORMAT_STRING, pfc::string8((const char *)release_discogs_format_string)));
	cfg_string_entries.add_item(make_conf_entry(CFG_FILE_TRACK_FORMAT_STRING, pfc::string8((const char *)release_file_format_string)));
	cfg_string_entries.add_item(make_conf_entry(CFG_ARTIST_ART_IDS_TITLEFORMAT, pfc::string8((const char *)artist_art_id_format_string)));
	//v200
	cfg_string_entries.add_item(make_conf_entry(CFG_EDIT_TAGS_DIALOG_HL_KEYWORD, pfc::string8((const char*)edit_tags_dlg_hl_keyword)));
	//..
}


void new_conf::upgrade(foo_discogs_conf3 &conf2) {
	replace_ANVs = conf2.replace_ANVs;
	move_the_at_beginning = conf2.move_the_at_beginning;
	discard_numeric_suffix = conf2.discard_numeric_suffix;

	match_tracks_using_duration = conf2.match_discogs_tracks;

	album_art_directory_string = conf2.album_art_dir_titleformat;
	album_art_filename_string = conf2.album_art_file_titleformat;
	artist_art_directory_string = conf2.artist_art_dir_titleformat;

	save_album_art = conf2.save_album_art;
	save_artist_art = conf2.save_artist_art;
	album_art_fetch_all = artist_art_fetch_all = conf2.fetch_all_art;
	album_art_overwrite = artist_art_overwrite = conf2.overwrite_files;

	display_exact_matches = conf2.display_exact_matches;
	enable_autosearch = conf2.enable_autosearch;

	display_ANVs = conf2.display_ANVs;

	update_art_flags = conf2.update_art_flags;
	
	oauth_token = conf2.oauth_token;
	oauth_token_secret = conf2.oauth_token_secret;

	find_release_dialog_width = conf2.find_release_dialog_width;
	find_release_dialog_height = conf2.find_release_dialog_height;
	release_dialog_width = conf2.release_dialog_width;
	release_dialog_height = conf2.release_dialog_height;
}

void init_conf() {
	if (!CONF.load()) {
		foo_discogs_conf3 conf3 = cfg_conf3.get_value();
		if (!conf3.is_nullptr()) {
			console::print("Upgrading to new conf format.");
			CONF.upgrade(conf3);
		}
		CONF.save();
		CONF.load();
	}
}


// OLD CONF

// {4B7524E4-6B72-4D77-BE95-C09043C30EBE}
static const GUID guid_cfg_conf3 = {0x4b7524e4, 0x6b72, 0x4d77, {0xbe, 0x95, 0xc0, 0x90, 0x43, 0xc3, 0xe, 0xbe}};

cfg_struct_t<foo_discogs_conf3> cfg_conf3(guid_cfg_conf3, foo_discogs_conf3::get_nullptr());


foo_discogs_conf3 foo_discogs_conf3::initialize() {
	log_msg("Initializing conf3.");
	return foo_discogs_conf3::get_default();
}

foo_discogs_conf3 foo_discogs_conf3::get_default(const char *oauth_token, const char *oauth_secret) {
	foo_discogs_conf3 conf;

	conf.conf_version = 160;

	// tagging
	conf.replace_ANVs = false;
	conf.move_the_at_beginning = true;
	conf.discard_numeric_suffix = true;

	conf.double_digits_track_numbers = true;
	conf.number_each_disc_from_one = true;

	conf.match_discogs_tracks = false;
	conf.show_length_column = true;

	// album/artist art
	strcpy_s(conf.album_art_dir_titleformat, "$directory_path(%path%)");
	strcpy_s(conf.album_art_file_titleformat, "cover");
	strcpy_s(conf.artist_art_dir_titleformat, "$directory_path(%path%)");
	strcpy_s(conf.artist_art_file_titleformat, "artist[%DISCOGS_ARTIST_ID%]");
	conf.display_art = false;
	conf.save_album_art = false;
	conf.save_artist_art = false;
	conf.fetch_all_art = false;
	conf.use_api_for_images = true;
	conf.overwrite_files = false;

	// find release dialog
	conf.display_exact_matches = true;
	conf.enable_autosearch = false;

	// release dialog
	conf.write_all_genres = true;
	conf.write_all_styles = true;
	conf.display_ANVs = true;

	conf.update_art_flags = UPDATE_ART_ALBUM_BIT;
	//strcpy_s(conf.va, "");

	// oauth token
	strcpy_s(conf.oauth_token, "");
	strcpy_s(conf.oauth_token_secret, "");

	if (oauth_token) {
		strcpy_s(conf.oauth_token, oauth_token);
	}
	if (oauth_secret) {
		strcpy_s(conf.oauth_token_secret, oauth_secret);
	}

	return conf;
}
