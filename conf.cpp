#include "stdafx.h"

#include "conf.h"

new_conf CONF;

// {22C5B65E-84E8-4D73-B3FF-34951D2C8A65} //mod
static const GUID guid_cfg_bool_values =
{ 0x22c5b65e, 0x84e8, 0x4d73, { 0xb3, 0xff, 0x34, 0x95, 0x1d, 0x2c, 0x8a, 0x65 } };
// {918DE111-A72A-4215-AFA9-497F42711D6B} //mod
static const GUID guid_cfg_int_values =
{ 0x918de111, 0xa72a, 0x4215, { 0xaf, 0xa9, 0x49, 0x7f, 0x42, 0x71, 0x1d, 0x6b } };
// {945598DE-4895-4894-B688-A66FDBE105F9} //mod
static const GUID guid_cfg_string_values =
{ 0x945598de, 0x4895, 0x4894, { 0xb6, 0x88, 0xa6, 0x6f, 0xdb, 0xe1, 0x5, 0xf9 } };

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
	bool forceupdate = false;
	// { specs vector, boolvals, intvals, stringvals;
	vspec v000 { &vec_specs, 0, 0, 0 };
	vspec v199 { &vec_specs, 25, 16, 13 };
	vspec v200 { &vec_specs, 27, 22, 14 };
	vspec v201 { &vec_specs, 28, 28, 14 };
	vspec v202 { &vec_specs, 28, 29, 14 };
	vspec v203 { &vec_specs, 28, 41, 14 };
	vspec v204 { &vec_specs, 28, 42, 14 }; // 1.0.4
	
	vspec* vlast = &vec_specs.at(vec_specs.size() - 1);

	vspec vLoad = { nullptr, cfg_bool_entries.get_count(),
		cfg_int_entries.get_count(), cfg_string_entries.get_count() };

	if (vLoad == v000) {
		save();
		return true;
	}
	else {
		if (vLoad > * vlast) {
			//unknown config version... safety reset
			vLoad = *vlast;
			pfc::string8 title;
			title << "Configuration Reset";
			pfc::string8 msg("This version of foo_discogger is not compatible with the current setup.\n");
			msg << "Configuration will be reset.\n";
			uMessageBox(core_api::get_main_window(), msg, title, MB_APPLMODAL | MB_ICONASTERISK);
			forceupdate = true;
		}
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
			case CFG_FIND_RELEASE_DIALOG_SIZE:
				find_release_dialog_size = item.value;
				break;
			case CFG_FIND_RELEASE_DIALOG_POSITION:
				find_release_dialog_position = item.value;
				break;
			case CFG_RELEASE_DIALOG_SIZE:
				release_dialog_size = item.value;
				break;
			case CFG_RELEASE_DIALOG_POSITION:
				release_dialog_position = item.value;
				break;
			case CFG_EDIT_TAGS_DIALOG_SIZE:
				edit_tags_dialog_size = item.value;
				break;
			case CFG_EDIT_TAGS_DIALOG_POSITION:
				edit_tags_dialog_position = item.value;
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
			case CFG_PREVIEW_DIALOG_SIZE:
				preview_tags_dialog_size = item.value;
				break;
			case CFG_PREVIEW_DIALOG_POSITION:
				preview_tags_dialog_position = item.value;
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
			//v201 (mod v17)
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
			//v202
			case CFG_CACHE_USE_OFFLINE_CACHE:
				cache_use_offline_cache = item.value;
				break;
			//v203
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
			case CFG_MATCH_DISCOGS_ARTWORK_STYLE:
				match_discogs_artwork_art_style = item.value;
				break;
			case CFG_MATCH_FILE_ARTWORKS_STYLE:
				match_file_artworks_art_style = item.value;
					break;
			case CFG_ALBUM_ART_SKIP_DEFAULT_CUST:
				album_art_skip_default_cust = item.value;
				break;
			
			//v204 (1.0.4)
			case CFG_EDIT_TAGS_DIALOG_FLAGS:
				edit_tags_dialog_flags = item.value;
			//..
		}
	}
	if (vLoad < v200) {
		//v200
		cfg_int_entries.add_item(make_conf_entry(CFG_PREVIEW_TAGS_DIALOG_COL1_WIDTH, preview_tags_dialog_col1_width));
		cfg_int_entries.add_item(make_conf_entry(CFG_PREVIEW_TAGS_DIALOG_COL2_WIDTH, preview_tags_dialog_col2_width));
		cfg_int_entries.add_item(make_conf_entry(CFG_MATCH_TRACKS_DISCOGS_COL1_WIDTH, match_tracks_discogs_col1_width));
		cfg_int_entries.add_item(make_conf_entry(CFG_MATCH_TRACKS_DISCOGS_COL2_WIDTH, match_tracks_discogs_col2_width));
		cfg_int_entries.add_item(make_conf_entry(CFG_MATCH_TRACKS_FILES_COL1_WIDTH, match_tracks_files_col1_width));
		cfg_int_entries.add_item(make_conf_entry(CFG_MATCH_TRACKS_FILES_COL2_WIDTH, match_tracks_files_col2_width));
	}
	if (vLoad < v201) {
		//v201
		cfg_int_entries.add_item(make_conf_entry(CFG_MATCH_TRACKS_DISCOGS_STYLE, match_tracks_discogs_style));
		cfg_int_entries.add_item(make_conf_entry(CFG_MATCH_TRACKS_FILES_STYLE, match_tracks_files_style));
		cfg_int_entries.add_item(make_conf_entry(CFG_PREVIEW_TAGS_DIALOG_W_WIDTH, preview_tags_dialog_w_width));
		cfg_int_entries.add_item(make_conf_entry(CFG_PREVIEW_TAGS_DIALOG_U_WIDTH, preview_tags_dialog_u_width));
		cfg_int_entries.add_item(make_conf_entry(CFG_PREVIEW_TAGS_DIALOG_S_WIDTH, preview_tags_dialog_s_width));
		cfg_int_entries.add_item(make_conf_entry(CFG_PREVIEW_TAGS_DIALOG_E_WIDTH, preview_tags_dialog_e_width));
	}
	if (vLoad < v202) {
		cfg_int_entries.add_item(make_conf_entry(CFG_CACHE_USE_OFFLINE_CACHE, cache_use_offline_cache));
	}
	if (vLoad < v203) {
		cfg_int_entries.add_item(make_conf_entry(CFG_DISCOGS_ARTWORK_RA_WIDTH, match_discogs_artwork_ra_width));
		cfg_int_entries.add_item(make_conf_entry(CFG_DISCOGS_ARTWORK_TYPE_WIDTH, match_discogs_artwork_type_width));
		cfg_int_entries.add_item(make_conf_entry(CFG_DISCOGS_ARTWORK_DIM_WIDTH, match_discogs_artwork_dim_width));
		cfg_int_entries.add_item(make_conf_entry(CFG_DISCOGS_ARTWORK_SAVE_WIDTH, match_discogs_artwork_save_width));
		cfg_int_entries.add_item(make_conf_entry(CFG_DISCOGS_ARTWORK_OVR_WIDTH, match_discogs_artwork_ovr_width));
		cfg_int_entries.add_item(make_conf_entry(CFG_DISCOGS_ARTWORK_EMBED_WIDTH, match_discogs_artwork_embed_width));
		cfg_int_entries.add_item(make_conf_entry(CFG_MATCH_FILE_ARTWORK_NAME_WIDTH, match_file_artwork_name_width));
		cfg_int_entries.add_item(make_conf_entry(CFG_MATCH_FILE_ARTWORK_DIM_WIDTH, match_file_artwork_dim_width));
		cfg_int_entries.add_item(make_conf_entry(CFG_MATCH_FILE_ARTWORK_SIZE_WIDTH, match_file_artwork_size_width));
		cfg_int_entries.add_item(make_conf_entry(CFG_MATCH_DISCOGS_ARTWORK_STYLE, match_discogs_artwork_art_style));
		cfg_int_entries.add_item(make_conf_entry(CFG_MATCH_FILE_ARTWORKS_STYLE, match_file_artworks_art_style));
		cfg_int_entries.add_item(make_conf_entry(CFG_ALBUM_ART_SKIP_DEFAULT_CUST, album_art_skip_default_cust));
	}

	if (vLoad < v204) {
		//recycle width to size, height to position
		t_size pos = pfc_infinite;
		conf_int_entry entry;

		entry = make_conf_entry(CFG_FIND_RELEASE_DIALOG_SIZE, find_release_dialog_size);
		pos = cfg_int_entries.find_item(entry);
		find_release_dialog_size = MAKELPARAM(find_release_dialog_size, find_release_dialog_position);
		cfg_int_entries.swap_item_with(pos, make_conf_entry(CFG_FIND_RELEASE_DIALOG_SIZE, find_release_dialog_size));
		entry = make_conf_entry(CFG_FIND_RELEASE_DIALOG_POSITION, find_release_dialog_position);
		pos = cfg_int_entries.find_item(entry);
		cfg_int_entries.swap_item_with(pos, make_conf_entry(CFG_FIND_RELEASE_DIALOG_POSITION, 0));
		find_release_dialog_position = 0;

		entry = make_conf_entry(CFG_RELEASE_DIALOG_SIZE, release_dialog_size);
		pos = cfg_int_entries.find_item(entry);
		release_dialog_size = MAKELPARAM(release_dialog_size, release_dialog_position);
		cfg_int_entries.swap_item_with(pos, make_conf_entry(CFG_RELEASE_DIALOG_SIZE, release_dialog_size));
		entry = make_conf_entry(CFG_RELEASE_DIALOG_POSITION, release_dialog_position);
		pos = cfg_int_entries.find_item(entry);
		cfg_int_entries.swap_item_with(pos, make_conf_entry(CFG_RELEASE_DIALOG_POSITION, 0));
		release_dialog_position = 0;

		entry = make_conf_entry(CFG_EDIT_TAGS_DIALOG_SIZE, edit_tags_dialog_size);
		pos = cfg_int_entries.find_item(entry);
		edit_tags_dialog_size = MAKELPARAM(edit_tags_dialog_size, edit_tags_dialog_position);
		cfg_int_entries.swap_item_with(pos, make_conf_entry(CFG_EDIT_TAGS_DIALOG_SIZE, edit_tags_dialog_size));
		entry = make_conf_entry(CFG_EDIT_TAGS_DIALOG_POSITION, edit_tags_dialog_position);
		pos = cfg_int_entries.find_item(entry);
		cfg_int_entries.swap_item_with(pos, make_conf_entry(CFG_EDIT_TAGS_DIALOG_POSITION, 0));
		edit_tags_dialog_position = 0;

		entry = make_conf_entry(CFG_PREVIEW_DIALOG_SIZE, preview_tags_dialog_size);
		pos = cfg_int_entries.find_item(entry);
		preview_tags_dialog_size = MAKELPARAM(preview_tags_dialog_size, preview_tags_dialog_position);
		cfg_int_entries.swap_item_with(pos, make_conf_entry(CFG_PREVIEW_DIALOG_SIZE, preview_tags_dialog_size));
		entry = make_conf_entry(CFG_PREVIEW_DIALOG_POSITION, preview_tags_dialog_position);
		pos = cfg_int_entries.find_item(entry);
		cfg_int_entries.swap_item_with(pos, make_conf_entry(CFG_PREVIEW_DIALOG_POSITION, 0));
		preview_tags_dialog_position = 0;

		cfg_int_entries.add_item(make_conf_entry(CFG_EDIT_TAGS_DIALOG_FLAGS, edit_tags_dialog_flags));
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

	if (forceupdate) {
		save();
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
		case CFG_FIND_RELEASE_DIALOG_SIZE:
			return in_conf.find_release_dialog_size;
		case CFG_FIND_RELEASE_DIALOG_POSITION:
			return in_conf.find_release_dialog_position;
		case CFG_RELEASE_DIALOG_SIZE:
			return in_conf.release_dialog_size;
		case CFG_RELEASE_DIALOG_POSITION:
			return in_conf.release_dialog_position;
		case CFG_EDIT_TAGS_DIALOG_SIZE:
			return in_conf.edit_tags_dialog_size;
		case CFG_EDIT_TAGS_DIALOG_POSITION:
			return in_conf.edit_tags_dialog_position;
		case CFG_EDIT_TAGS_DIALOG_COL1_WIDTH:
			return in_conf.edit_tags_dialog_col1_width;
		case CFG_EDIT_TAGS_DIALOG_COL2_WIDTH:
			return in_conf.edit_tags_dialog_col2_width;
		case CFG_EDIT_TAGS_DIALOG_COL3_WIDTH:
			return in_conf.edit_tags_dialog_col3_width;
		case CFG_PREVIEW_DIALOG_SIZE:
			return in_conf.preview_tags_dialog_size;
		case CFG_PREVIEW_DIALOG_POSITION:
			return in_conf.preview_tags_dialog_position;
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
			return in_conf.match_tracks_discogs_col1_width;
		case CFG_MATCH_TRACKS_DISCOGS_COL2_WIDTH:
			return in_conf.match_tracks_discogs_col2_width;
		case CFG_MATCH_TRACKS_FILES_COL1_WIDTH:
			return in_conf.match_tracks_files_col1_width;
		case CFG_MATCH_TRACKS_FILES_COL2_WIDTH:
			return in_conf.match_tracks_files_col2_width;
		//v201
		case CFG_MATCH_TRACKS_DISCOGS_STYLE:
			return in_conf.match_tracks_discogs_style;
		case CFG_MATCH_TRACKS_FILES_STYLE:
			return in_conf.match_tracks_files_style;
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
		//v203
		case CFG_DISCOGS_ARTWORK_RA_WIDTH:
			return in_conf.match_discogs_artwork_ra_width;
		case CFG_DISCOGS_ARTWORK_TYPE_WIDTH:
			return in_conf.match_discogs_artwork_type_width;
		case CFG_DISCOGS_ARTWORK_DIM_WIDTH:
			return in_conf.match_discogs_artwork_dim_width;
		case CFG_DISCOGS_ARTWORK_SAVE_WIDTH:
			return in_conf.match_discogs_artwork_save_width;
		case CFG_DISCOGS_ARTWORK_OVR_WIDTH:
			return in_conf.match_discogs_artwork_ovr_width;
		case CFG_DISCOGS_ARTWORK_EMBED_WIDTH:
			return in_conf.match_discogs_artwork_embed_width;
		case CFG_MATCH_FILE_ARTWORK_NAME_WIDTH:
			return in_conf.match_file_artwork_name_width;
		case CFG_MATCH_FILE_ARTWORK_DIM_WIDTH:
			return in_conf.match_file_artwork_dim_width;
		case CFG_MATCH_FILE_ARTWORK_SIZE_WIDTH:
			return in_conf.match_file_artwork_size_width;
		case CFG_MATCH_DISCOGS_ARTWORK_STYLE:
			return in_conf.match_discogs_artwork_art_style;
		case CFG_MATCH_FILE_ARTWORKS_STYLE:
			return in_conf.match_file_artworks_art_style;
		case CFG_ALBUM_ART_SKIP_DEFAULT_CUST:
			return in_conf.album_art_skip_default_cust;
		//v204
		case CFG_EDIT_TAGS_DIALOG_FLAGS:
			return in_conf.edit_tags_dialog_flags;
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
	cfg_int_entries.add_item(make_conf_entry(CFG_FIND_RELEASE_DIALOG_SIZE, find_release_dialog_size));
	cfg_int_entries.add_item(make_conf_entry(CFG_FIND_RELEASE_DIALOG_POSITION, find_release_dialog_position));
	cfg_int_entries.add_item(make_conf_entry(CFG_RELEASE_DIALOG_SIZE, release_dialog_size));
	cfg_int_entries.add_item(make_conf_entry(CFG_RELEASE_DIALOG_POSITION, release_dialog_position));
	cfg_int_entries.add_item(make_conf_entry(CFG_EDIT_TAGS_DIALOG_SIZE, edit_tags_dialog_size));
	cfg_int_entries.add_item(make_conf_entry(CFG_EDIT_TAGS_DIALOG_POSITION, edit_tags_dialog_position));
	cfg_int_entries.add_item(make_conf_entry(CFG_EDIT_TAGS_DIALOG_COL1_WIDTH, edit_tags_dialog_col1_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_EDIT_TAGS_DIALOG_COL2_WIDTH, edit_tags_dialog_col2_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_EDIT_TAGS_DIALOG_COL3_WIDTH, edit_tags_dialog_col3_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_EDIT_TAGS_DIALOG_POSITION, edit_tags_dialog_position));
	cfg_int_entries.add_item(make_conf_entry(CFG_PREVIEW_DIALOG_SIZE, preview_tags_dialog_size));
	cfg_int_entries.add_item(make_conf_entry(CFG_PREVIEW_DIALOG_POSITION, preview_tags_dialog_position));
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
	//v201
	cfg_int_entries.add_item(make_conf_entry(CFG_MATCH_TRACKS_DISCOGS_STYLE, match_tracks_discogs_style));
	cfg_int_entries.add_item(make_conf_entry(CFG_MATCH_TRACKS_FILES_STYLE, match_tracks_files_style));
	cfg_int_entries.add_item(make_conf_entry(CFG_PREVIEW_TAGS_DIALOG_W_WIDTH, preview_tags_dialog_w_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_PREVIEW_TAGS_DIALOG_U_WIDTH, preview_tags_dialog_u_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_PREVIEW_TAGS_DIALOG_S_WIDTH, preview_tags_dialog_s_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_PREVIEW_TAGS_DIALOG_E_WIDTH, preview_tags_dialog_e_width));
	//v202
	cfg_int_entries.add_item(make_conf_entry(CFG_CACHE_USE_OFFLINE_CACHE, cache_use_offline_cache));
	//v203
	cfg_int_entries.add_item(make_conf_entry(CFG_DISCOGS_ARTWORK_RA_WIDTH, match_discogs_artwork_ra_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_DISCOGS_ARTWORK_TYPE_WIDTH, match_discogs_artwork_type_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_DISCOGS_ARTWORK_DIM_WIDTH, match_discogs_artwork_dim_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_DISCOGS_ARTWORK_SAVE_WIDTH, match_discogs_artwork_save_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_DISCOGS_ARTWORK_OVR_WIDTH, match_discogs_artwork_ovr_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_DISCOGS_ARTWORK_EMBED_WIDTH, match_discogs_artwork_embed_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_MATCH_FILE_ARTWORK_NAME_WIDTH, match_file_artwork_name_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_MATCH_FILE_ARTWORK_DIM_WIDTH, match_file_artwork_dim_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_MATCH_FILE_ARTWORK_SIZE_WIDTH, match_file_artwork_size_width));
	cfg_int_entries.add_item(make_conf_entry(CFG_MATCH_DISCOGS_ARTWORK_STYLE, match_discogs_artwork_art_style));
	cfg_int_entries.add_item(make_conf_entry(CFG_MATCH_FILE_ARTWORKS_STYLE, match_file_artworks_art_style));
	cfg_int_entries.add_item(make_conf_entry(CFG_ALBUM_ART_SKIP_DEFAULT_CUST, album_art_skip_default_cust));
	//v204 (from 1.0.4)
	cfg_int_entries.add_item(make_conf_entry(CFG_EDIT_TAGS_DIALOG_FLAGS, edit_tags_dialog_flags));
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

void new_conf::save_active_config_tab(int newval) {
	const conf_int_entry &entry = make_conf_entry(CFG_LAST_CONF_TAB, last_conf_tab); 
	auto pos = cfg_int_entries.find_item(entry);

	if (pos != pfc_infinite) {
		conf_int_entry new_entry = make_conf_entry(CFG_LAST_CONF_TAB, newval);
		cfg_int_entries.swap_item_with(pos, new_entry);
		last_conf_tab = newval;
	}
}

void init_conf() {
	
	if (!CONF.load()) {
		PFC_ASSERT(false);
	}

}
