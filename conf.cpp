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
			case CFG_PARSE_HIDDEN_AS_REGULAR:
				parse_hidden_as_regular = item.value;
			case CFG_SKIP_VIDEO_TRACKS:
				skip_video_tracks = item.value;
		}
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
		}
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
		}
	}
	return changed;
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
