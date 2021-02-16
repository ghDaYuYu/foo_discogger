#pragma once

#include "formatting_script.h"

#define PREVIEW_NORMAL		1
#define PREVIEW_DIFFERENCE	2
#define PREVIEW_ORIGINAL	3
#define PREVIEW_DEBUG		4


#define CFG_REPLACE_ANVS					1
#define CFG_MOVE_THE_AT_BEGINNING			2
#define CFG_DISCARD_NUMERIC_SUFFIX			3
#define CFG_DISPLAY_EXACT_MATCHES			13
#define CFG_ENABLE_AUTOSEARCH				14

#define CFG_DISPLAY_ANVS					17

#define CFG_UPDATE_ART_FLAGS				18
#define CFG_FIND_RELEASE_DIALOG_WIDTH		19
#define CFG_FIND_RELEASE_DIALOG_HEIGHT		20
#define CFG_RELEASE_DIALOG_WIDTH			21
#define CFG_RELEASE_DIALOG_HEIGHT			22
#define CFG_EDIT_TAGS_DIALOG_WIDTH			23
#define CFG_EDIT_TAGS_DIALOG_HEIGHT			24
#define CFG_EDIT_TAGS_DIALOG_COL1_WIDTH		25
#define CFG_EDIT_TAGS_DIALOG_COL2_WIDTH		26
#define CFG_EDIT_TAGS_DIALOG_COL3_WIDTH		27

#define CFG_SAVE_ALBUM_ART					9
#define CFG_ALBUM_ART_DIRECTORY_STRING		28
#define CFG_ALBUM_ART_FILENAME_STRING		29
#define CFG_ALBUM_ART_FETCH_ALL				11
#define CFG_ALBUM_ART_OVERWRITE				12
#define CFG_ALBUM_ART_EMBED                 59

#define CFG_SAVE_ARTIST_ART					10
#define CFG_ARTIST_ART_DIRECTORY_STRING		30
#define CFG_ARTIST_ART_FILENAME_STRING		31
#define CFG_ARTIST_ART_FETCH_ALL			36
#define CFG_ARTIST_ART_OVERWRITE			37
#define CFG_ARTIST_ART_IDS_TITLEFORMAT		45
#define CFG_ARTIST_ART_EMBED                60

#define CFG_OAUTH_TOKEN						32
#define CFG_OAUTH_TOKEN_SECRET				33

#define CFG_REMOVE_OTHER_TAGS				34
#define CFG_REMOVE_EXCLUDE_TAGS				35

#define CFG_MATCH_TRACKS_USING_DURATION		6
#define CFG_MATCH_TRACKS_USING_NUMBER		38
#define CFG_MATCH_TRACKS_USING_META			39
#define CFG_ASSUME_TRACKS_SORTED			40

#define CFG_SKIP_RELEASE_DLG_IF_MATCHED		41
#define CFG_SKIP_FIND_RELEASE_DLG_IF_IDED	42

#define CFG_SEARCH_RELEASE_FORMAT_STRING    43
#define CFG_SEARCH_MASTER_FORMAT_STRING	    44
#define CFG_SEARCH_MASTER_SUB_FORMAT_STRING 51

#define CFG_LAST_CONF_TAB					46

#define CFG_DISCOGS_TRACK_FORMAT_STRING		47
#define CFG_FILE_TRACK_FORMAT_STRING		48

#define CFG_SKIP_PREVIEW_DIALOG				49

#define CFG_PREVIEW_MODE					51
#define CFG_UPDATE_PREVIEW_CHANGES			52
#define CFG_PREVIEW_DIALOG_WIDTH			53
#define CFG_PREVIEW_DIALOG_HEIGHT			54

#define CFG_UPDATE_TAGS_MANUALLY_PROMPT		55
#define CFG_PARSE_HIDDEN_AS_REGULAR			56
#define CFG_SKIP_VIDEO_TRACKS				58

#define CFG_CACHE_MAX_OBJECTS				57


typedef struct
{
	int id;
	bool value;
} conf_bool_entry;


typedef struct
{
	int id;
	int value;
} conf_int_entry;


typedef struct
{
	int id;
	pfc::string8 value;
} conf_string_entry;

extern conf_bool_entry make_conf_entry(int i, bool v);
extern conf_int_entry make_conf_entry(int i, int v);
extern conf_string_entry make_conf_entry(int i, const pfc::string8 &v);

FB2K_STREAM_READER_OVERLOAD(conf_bool_entry) {
	int id;
	bool v;
	stream >> id >> v;
	value.id = id;
	value.value = v;
	return stream;
}

FB2K_STREAM_READER_OVERLOAD(conf_int_entry) {
	int id;
	int v;
	stream >> id >> v;
	value.id = id;
	value.value = v;
	return stream;
}

FB2K_STREAM_READER_OVERLOAD(conf_string_entry) {
	int id;
	pfc::string8 v;
	stream >> id >> v;
	value.id = id;
	value.value = v;
	return stream;
}

FB2K_STREAM_WRITER_OVERLOAD(conf_bool_entry) {
	stream << value.id << value.value;
	return stream;
}

FB2K_STREAM_WRITER_OVERLOAD(conf_int_entry) {
	stream << value.id << value.value;
	return stream;
}

FB2K_STREAM_WRITER_OVERLOAD(conf_string_entry) {
	//pfc::string8 v(value.value.get_ptr());
	//stream << value.id << v;
	stream << value.id << value.value;
	return stream;
}


struct foo_discogs_conf3;


class new_conf
{
public:
	bool load();
	int id_to_val_int(int id, new_conf in_conf);
	bool id_to_val_bool(int id, new_conf in_conf);
	void id_to_val_str(int id, new_conf in_conf, pfc::string8& out);
	void save();

	enum class ConfFilter :uint8_t {
		CONF_FILTER_CONF = 0,
		CONF_FILTER_FIND,
		CONF_FILTER_PREVIEW,
		CONF_FILTER_TAG,
		CONF_FILTER_TRACK,
		CONF_FILTER_UPDATE_ART,
		CONF_FILTER_UPDATE_TAG,
	};

	template <typename Enumeration>
	//as_integer...
	auto asi(Enumeration const value)
		-> typename std::underlying_type<Enumeration>::type
	{
		return static_cast<typename std::underlying_type<Enumeration>::type>(value);
	}

	std::vector<std::pair<int, int>> idarray
	{
		// CONF_FILTER_CONF (configuration_dialog)
		{ asi(ConfFilter::CONF_FILTER_CONF), CFG_REPLACE_ANVS },
		{ asi(ConfFilter::CONF_FILTER_CONF), CFG_MOVE_THE_AT_BEGINNING},
		{ asi(ConfFilter::CONF_FILTER_CONF), CFG_DISCARD_NUMERIC_SUFFIX},
		{ asi(ConfFilter::CONF_FILTER_CONF), CFG_ENABLE_AUTOSEARCH},
		{ asi(ConfFilter::CONF_FILTER_CONF), CFG_DISPLAY_ANVS},
		{ asi(ConfFilter::CONF_FILTER_CONF), CFG_SAVE_ALBUM_ART},
		{ asi(ConfFilter::CONF_FILTER_CONF), CFG_ALBUM_ART_DIRECTORY_STRING},
		{ asi(ConfFilter::CONF_FILTER_CONF), CFG_ALBUM_ART_FILENAME_STRING},
		{ asi(ConfFilter::CONF_FILTER_CONF), CFG_ALBUM_ART_FETCH_ALL},
		{ asi(ConfFilter::CONF_FILTER_CONF), CFG_ALBUM_ART_OVERWRITE},
		{ asi(ConfFilter::CONF_FILTER_CONF), CFG_ALBUM_ART_EMBED},
		{ asi(ConfFilter::CONF_FILTER_CONF), CFG_SAVE_ARTIST_ART},
		{ asi(ConfFilter::CONF_FILTER_CONF), CFG_ARTIST_ART_DIRECTORY_STRING},
		{ asi(ConfFilter::CONF_FILTER_CONF), CFG_ARTIST_ART_FILENAME_STRING},
		{ asi(ConfFilter::CONF_FILTER_CONF), CFG_ARTIST_ART_FETCH_ALL},
		{ asi(ConfFilter::CONF_FILTER_CONF), CFG_ARTIST_ART_OVERWRITE},
		{ asi(ConfFilter::CONF_FILTER_CONF), CFG_ARTIST_ART_IDS_TITLEFORMAT},
		{ asi(ConfFilter::CONF_FILTER_CONF), CFG_ARTIST_ART_EMBED},
		{ asi(ConfFilter::CONF_FILTER_CONF), CFG_OAUTH_TOKEN},
		{ asi(ConfFilter::CONF_FILTER_CONF), CFG_OAUTH_TOKEN_SECRET},
		{ asi(ConfFilter::CONF_FILTER_CONF), CFG_REMOVE_OTHER_TAGS},
		{ asi(ConfFilter::CONF_FILTER_CONF), CFG_REMOVE_EXCLUDE_TAGS},
		{ asi(ConfFilter::CONF_FILTER_CONF), CFG_MATCH_TRACKS_USING_DURATION},
		{ asi(ConfFilter::CONF_FILTER_CONF), CFG_MATCH_TRACKS_USING_NUMBER},
		{ asi(ConfFilter::CONF_FILTER_CONF), CFG_MATCH_TRACKS_USING_META},
		{ asi(ConfFilter::CONF_FILTER_CONF), CFG_ASSUME_TRACKS_SORTED},
		{ asi(ConfFilter::CONF_FILTER_CONF), CFG_SKIP_RELEASE_DLG_IF_MATCHED},
		{ asi(ConfFilter::CONF_FILTER_CONF), CFG_SKIP_FIND_RELEASE_DLG_IF_IDED},
		{ asi(ConfFilter::CONF_FILTER_CONF), CFG_SEARCH_RELEASE_FORMAT_STRING},
		{ asi(ConfFilter::CONF_FILTER_CONF), CFG_SEARCH_MASTER_FORMAT_STRING},
		{ asi(ConfFilter::CONF_FILTER_CONF), CFG_SEARCH_MASTER_SUB_FORMAT_STRING},
		{ asi(ConfFilter::CONF_FILTER_CONF), CFG_LAST_CONF_TAB},
		{ asi(ConfFilter::CONF_FILTER_CONF), CFG_DISCOGS_TRACK_FORMAT_STRING},
		{ asi(ConfFilter::CONF_FILTER_CONF), CFG_FILE_TRACK_FORMAT_STRING},
		{ asi(ConfFilter::CONF_FILTER_CONF), CFG_SKIP_PREVIEW_DIALOG},
		{ asi(ConfFilter::CONF_FILTER_CONF), CFG_PARSE_HIDDEN_AS_REGULAR},
		{ asi(ConfFilter::CONF_FILTER_CONF), CFG_SKIP_VIDEO_TRACKS},
		{ asi(ConfFilter::CONF_FILTER_CONF), CFG_CACHE_MAX_OBJECTS},

		// CONF_FILTER_FIND (find_release_dialog)
		{ asi(ConfFilter::CONF_FILTER_FIND), CFG_DISPLAY_EXACT_MATCHES },
		{ asi(ConfFilter::CONF_FILTER_FIND), CFG_FIND_RELEASE_DIALOG_WIDTH },
		{ asi(ConfFilter::CONF_FILTER_FIND), CFG_FIND_RELEASE_DIALOG_HEIGHT },

		// CONF_FILTER_PREVIEW (preview_dialog)
		{ asi(ConfFilter::CONF_FILTER_PREVIEW), CFG_PREVIEW_MODE },
		//{ asi(ConfFilter::CONF_FILTER_PREVIEW), preview_tags_dialog_col1_width }, missing id
		//{ asi(ConfFilter::CONF_FILTER_PREVIEW), preview_tags_dialog_col2_width }, missing id
		{ asi(ConfFilter::CONF_FILTER_PREVIEW), CFG_PREVIEW_DIALOG_WIDTH },
		{ asi(ConfFilter::CONF_FILTER_PREVIEW), CFG_PREVIEW_DIALOG_HEIGHT },
		{ asi(ConfFilter::CONF_FILTER_PREVIEW), CFG_REPLACE_ANVS }, //<<

		// CONF_FILTER_TAG (tag_mappings_dialog)
		{ asi(ConfFilter::CONF_FILTER_TAG), CFG_EDIT_TAGS_DIALOG_COL1_WIDTH },
		{ asi(ConfFilter::CONF_FILTER_TAG), CFG_EDIT_TAGS_DIALOG_COL2_WIDTH },
		{ asi(ConfFilter::CONF_FILTER_TAG), CFG_EDIT_TAGS_DIALOG_COL3_WIDTH },
		{ asi(ConfFilter::CONF_FILTER_TAG), CFG_PREVIEW_DIALOG_WIDTH },
		{ asi(ConfFilter::CONF_FILTER_TAG), CFG_PREVIEW_DIALOG_HEIGHT },

		// CONF_FILTER_TRACK (track_matching_dialog)
		{ asi(ConfFilter::CONF_FILTER_TRACK), CFG_FIND_RELEASE_DIALOG_WIDTH },
		{ asi(ConfFilter::CONF_FILTER_TRACK), CFG_FIND_RELEASE_DIALOG_HEIGHT },

		// CONF_FILTER_UPDATE_ART (update_art_dialog)
		{ asi(ConfFilter::CONF_FILTER_UPDATE_ART), CFG_UPDATE_ART_FLAGS },

		// CONF_FILTER_TAG (update_tags_dialog)
		{ asi(ConfFilter::CONF_FILTER_TAG), CFG_REPLACE_ANVS },	//<<
		{ asi(ConfFilter::CONF_FILTER_TAG), CFG_UPDATE_TAGS_MANUALLY_PROMPT },
		{ asi(ConfFilter::CONF_FILTER_TAG), CFG_UPDATE_PREVIEW_CHANGES },
		
	};

	void save(ConfFilter cfgfilter, new_conf in_conf);
	void save(ConfFilter cfgfilter, new_conf in_conf, int id);
	void upgrade(foo_discogs_conf3 &old_conf);
	
	bool replace_ANVs = false;
	bool move_the_at_beginning = true;
	bool discard_numeric_suffix = true;

	bool double_digits_track_numbers = true;
	bool number_each_disc_from_one = true;

	bool match_tracks_using_duration = true;
	bool match_tracks_using_number = true;
	bool assume_tracks_sorted = false;
	bool skip_release_dialog_if_matched = false;
	bool skip_find_release_dialog_if_ided = false;
	bool skip_preview_dialog = false;

	bool save_album_art = true;
	bool album_art_fetch_all = false;
	bool album_art_overwrite = false;

	bool save_artist_art = false;
	bool artist_art_fetch_all = false;
	bool artist_art_overwrite = false;

	bool embed_album_art = false;
	bool embed_artist_art = false;

	formatting_script album_art_directory_string = "$directory_path(%path%)";
	formatting_script artist_art_directory_string = "$directory_path(%path%)";

	formatting_script album_art_filename_string = "cover$ifequal(%IMAGE_NUMBER%,1,,_%IMAGE_NUMBER%)";
	formatting_script artist_art_filename_string = "artist%ARTIST_ID%$ifequal(%IMAGE_NUMBER%,1,,_%IMAGE_NUMBER%)";
	formatting_script artist_art_id_format_string = "%DISCOGS_ARTIST_ID%";

	bool display_exact_matches = true;
	bool enable_autosearch = true;

	bool update_tags_preview_changes = true;
	bool update_tags_manually_match = true;

	bool parse_hidden_as_regular = false;
	bool skip_video_tracks = false;

	int preview_mode = PREVIEW_NORMAL;

	// TODO: maybe remove
	bool display_ANVs = false;

	// TODO: expand
	int update_art_flags;

	pfc::string8 oauth_token = "";
	pfc::string8 oauth_token_secret = "";

	int find_release_dialog_width = 0;
	int find_release_dialog_height = 0;
	int release_dialog_width = 0;
	int release_dialog_height = 0;
	int edit_tags_dialog_width = 0;
	int edit_tags_dialog_height = 0;
	int edit_tags_dialog_col1_width = 0;
	int edit_tags_dialog_col2_width = 0;
	int edit_tags_dialog_col3_width = 0;

	int preview_tags_dialog_width = 0;
	int preview_tags_dialog_col1_width = 0;
	int preview_tags_dialog_col2_width = 0;

	int preview_tags_dialog_height = 0;

	int last_conf_tab = 0;

	int cache_max_objects = 1000;

	bool remove_other_tags = false;
	pfc::array_t<pfc::string8> remove_exclude_tags;
	pfc::string8 raw_remove_exclude_tags;

	inline void set_remove_exclude_tags(const pfc::string8 &s) {
		tokenize(s, ",", remove_exclude_tags, true);
		raw_remove_exclude_tags = s;
	}

	formatting_script search_release_format_string = "$join($append(%RELEASE_TITLE%,%RELEASE_SEARCH_LABELS%,%RELEASE_SEARCH_FORMATS%,%RELEASE_YEAR%,%RELEASE_SEARCH_CATNOS%))";
	formatting_script search_master_format_string = "'[master] '$join($append(%MASTER_RELEASE_TITLE%,%MASTER_RELEASE_YEAR%))";
	formatting_script search_master_sub_format_string = "'  '$ifequal(%RELEASE_ID%,%MASTER_RELEASE_MAIN_RELEASE_ID%,'* ','   ')$join($append(%RELEASE_TITLE%,%RELEASE_SEARCH_LABELS%,%RELEASE_SEARCH_MAJOR_FORMATS%,%RELEASE_SEARCH_FORMATS%,%RELEASE_YEAR%,%RELEASE_SEARCH_CATNOS%,%RELEASE_COUNTRY%))";

	formatting_script release_discogs_format_string = "$ifgreater(%RELEASE_TOTAL_DISCS%,1,%DISC_NUMBER%-,)$num(%TRACK_DISC_TRACK_NUMBER%,2) - $multi_if($multi_and(%ARTISTS_NAME_VARIATION%,$multi_not(%REPLACE_ANVS%)),%ARTISTS_NAME_VARIATION%$multi_if(%DISPLAY_ANVS%,*,),%ARTISTS_NAME%) - %TRACK_TITLE%$ifequal(%TRACK_TOTAL_HIDDEN_TRACKS%,0,,'   [+'%TRACK_TOTAL_HIDDEN_TRACKS%' HIDDEN]')";
	formatting_script release_file_format_string = "$if($strcmp($ext(%path%),tags),$info(@),%path%)";
};

typedef new_conf foo_discogs_conf;

extern new_conf CONF;

void init_conf();


// OLD CONF

#define MAX_CONF_STRING_LEN 256

// deprecate this shit
#define IS_BIT_SET(x,b)		((x)&(b)?true:false)
#define SET_BIT(x,b)		((x)|=(b))

#define UNSET_BIT(x,b)		((x)&=~(b))
#define TOGGLE_BIT(x,b)		(IS_BIT_SET(x,b)?UNSET_BIT(x,b):SET_BIT(x,b))

#define UPDATE_ART_ALBUM_BIT		8
#define UPDATE_ART_ARTIST_BIT		16


struct foo_discogs_conf3
{
	// version of conf struct
	int conf_version;
	bool conf_initialized;

	// tagging
	bool replace_ANVs; // false
	bool move_the_at_beginning; // true
	bool discard_numeric_suffix; // true;

	bool double_digits_track_numbers; //true
	bool number_each_disc_from_one; // true
	bool preserve_zero_track_number; // unused

	bool match_discogs_tracks; // true
	bool show_length_column; // true

	// album/artist art
	char album_art_dir_titleformat[MAX_CONF_STRING_LEN]; // "$directory_path(%path%)"
	char album_art_file_titleformat[MAX_CONF_STRING_LEN]; // "cover"
	char artist_art_dir_titleformat[MAX_CONF_STRING_LEN]; // "$directory_path(%path%)"
	char artist_art_file_titleformat[MAX_CONF_STRING_LEN]; // "artist[%DISCOGS_ARTIST_ID%]"

	bool display_art; // false
	bool save_album_art; // true
	bool save_artist_art; // true
	bool fetch_all_art; //false
	bool use_api_for_images; // unused
	bool overwrite_files; // false

	// find release dialog
	bool display_exact_matches; // true
	bool enable_autosearch; // false

	// release dialog
	bool write_all_genres; // true
	bool write_all_styles; // true
	bool display_ANVs; //true

	int update_art_flags;

	char va[MAX_CONF_STRING_LEN]; // "Various Artists"

	// OAUTH
	char oauth_token[MAX_CONF_STRING_LEN];
	char oauth_token_secret[MAX_CONF_STRING_LEN];

	// SIZING
	int find_release_dialog_width = 0;
	int find_release_dialog_height = 0;
	int release_dialog_width = 0;
	int release_dialog_height = 0;
	int find_release_sizer = 0; // unused
	int release_sizer = 0; // unused

	static foo_discogs_conf3 get_default(const char *oauth_token = nullptr, const char *oauth_secret = nullptr);
	static foo_discogs_conf3 initialize();

	bool is_nullptr() {
		return conf_version == 0;
	}

	static foo_discogs_conf3 get_nullptr() {
		foo_discogs_conf3 conf3;
		conf3.conf_version = 0;
		return conf3;
	}
};

extern cfg_struct_t<foo_discogs_conf3> cfg_conf3;
