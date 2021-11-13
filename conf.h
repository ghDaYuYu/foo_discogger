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
#define DEPRI_CFG_FIND_RELEASE_DIALOG_SIZE			19
#define DEPRI_CFG_FIND_RELEASE_DIALOG_POSITION		20
#define DEPRI_CFG_RELEASE_DIALOG_SIZE				21
#define DEPRI_CFG_RELEASE_DIALOG_POSITION			22
#define DEPRI_CFG_EDIT_TAGS_DIALOG_SIZE				23
#define DEPRI_CFG_EDIT_TAGS_DIALOG_POSITION			24
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


#define CFG_SEARCH_RELEASE_FORMAT_STRING    43
#define CFG_SEARCH_MASTER_FORMAT_STRING	    44
#define CFG_SEARCH_MASTER_SUB_FORMAT_STRING 51

#define CFG_LAST_CONF_TAB					46

#define CFG_DISCOGS_TRACK_FORMAT_STRING		47
#define CFG_FILE_TRACK_FORMAT_STRING		48


#define CFG_PREVIEW_MODE					51
#define CFG_UPDATE_PREVIEW_CHANGES			52
#define DEPRI_CFG_PREVIEW_DIALOG_SIZE				53
#define DEPRI_CFG_PREVIEW_DIALOG_POSITION			54

#define CFG_UPDATE_TAGS_MANUALLY_PROMPT		55
#define CFG_PARSE_HIDDEN_AS_REGULAR			56
#define CFG_SKIP_VIDEO_TRACKS				58

#define CFG_CACHE_MAX_OBJECTS				57

//v.20X
//2001 = 2->V2, 0->bool, 01->id
//2101 = 2->V2, 1->int,  01->id
//2201 = 2->V2, 2->str,  01->id

// BOOLS ---------------------------------------

#define CFG_EDIT_TAGS_DIALOG_SHOW_TM_STATS	2001
#define CFG_FIND_RELEASE_DIALOG_SHOW_ID		2002
#define CFG_RELEASE_ENTER_KEY_OVR			2003

//DEPRI v205
#define CFG_SKIP_RELEASE_DLG_IF_MATCHED		41
#define CFG_SKIP_FIND_RELEASE_DLG_IF_IDED	42
#define CFG_SKIP_PREVIEW_DIALOG				49
//

// INTS ----------------------------------------

#define CFG_PREVIEW_TAGS_DIALOG_COL1_WIDTH	2101
#define CFG_PREVIEW_TAGS_DIALOG_COL2_WIDTH	2102

#define CFG_MATCH_TRACKS_DISCOGS_COL1_WIDTH	2103
#define CFG_MATCH_TRACKS_DISCOGS_COL2_WIDTH	2104
#define CFG_MATCH_TRACKS_FILES_COL1_WIDTH	2105
#define CFG_MATCH_TRACKS_FILES_COL2_WIDTH	2106

//v.201

#define CFG_MATCH_TRACKS_DISCOGS_STYLE		2107
#define CFG_MATCH_TRACKS_FILES_STYLE		2108
#define CFG_PREVIEW_TAGS_DIALOG_W_WIDTH		2109
#define CFG_PREVIEW_TAGS_DIALOG_U_WIDTH		2110
#define CFG_PREVIEW_TAGS_DIALOG_S_WIDTH		2111
#define CFG_PREVIEW_TAGS_DIALOG_E_WIDTH		2112

//v202
#define CFG_CACHE_USE_OFFLINE_CACHE			2113

//v203
#define CFG_DISCOGS_ARTWORK_RA_WIDTH		2114
#define CFG_DISCOGS_ARTWORK_TYPE_WIDTH		2115
#define CFG_DISCOGS_ARTWORK_DIM_WIDTH		2116
#define CFG_DISCOGS_ARTWORK_SAVE_WIDTH		2117
#define CFG_DISCOGS_ARTWORK_OVR_WIDTH		2118
#define CFG_DISCOGS_ARTWORK_EMBED_WIDTH		2119
#define CFG_MATCH_FILE_ARTWORK_NAME_WIDTH	2120
#define CFG_MATCH_FILE_ARTWORK_DIM_WIDTH	2121
#define CFG_MATCH_FILE_ARTWORK_SIZE_WIDTH	2122
#define CFG_MATCH_DISCOGS_ARTWORK_STYLE 	2123
#define CFG_MATCH_FILE_ARTWORKS_STYLE 		2124

#define CFG_ALBUM_ART_SKIP_DEFAULT_CUST		2125
#define CFG_EDIT_TAGS_DIALOG_FLAGS			2126

//v205 (1.0.6/1.1.0)
#define CFG_DC_DB_FLAG						2127
#define CFG_FIND_RELEASE_FILTER_FLAG		2128
#define CFG_SKIP_MNG_FLAG					2129
#define DEPRI_CFG_EDIT_CAT_CREDIT_DIALOG_SIZE		2130
#define DEPRI_CFG_EDIT_CAT_CREDIT_DIALOG_POS		2131
#define CFG_LIST_STYLE						2132

// STRINGS -------------------------------------

#define CFG_EDIT_TAGS_DIALOG_HL_KEYWORD		2201
//v205 (1.0.6/1.1.0)
#define CFG_DC_DB_PATH						2202
//..

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

inline bool operator==(const conf_int_entry& lhs, const conf_int_entry& rhs) {
	return (lhs.id == rhs.id &&
		lhs.value == rhs.value);
}

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

#ifndef VSPECS
#define VSPECS
const struct vspec;
typedef std::vector<vspec> vec_spec_t;

const struct vspec {
	t_size boolvals; t_size intvals; t_size stringvals;
	vec_spec_t* vspecs;
	vspec() { vspecs = nullptr; boolvals = 0; intvals = 0; stringvals = 0; };
	vspec(vec_spec_t* specs, t_size boolvals, t_size intvals, t_size stringvals)
		: boolvals(boolvals), intvals(intvals), stringvals(stringvals) {
		if (specs != nullptr) {
			vspecs = specs;
			vspecs->emplace_back((vspec)*this);
		}
		else {
			vspecs = nullptr;
		}
	};
	inline bool operator==(const vspec& rhs) {
		return signature(rhs.boolvals, rhs.intvals, rhs.stringvals);
	}
	inline bool operator<(const vspec& rhs)
	{
		//specs vector in lhs can be null, but not in rhs
		PFC_ASSERT(rhs.vspecs != nullptr);

		vec_spec_t* vec = rhs.vspecs;
		auto itrhs = std::find_if(vec->begin(), vec->end(),
			[rhs](auto e) {return e == rhs; });
		auto itthis = std::find_if(vec->begin(), vec->end(),
			[=](auto e) {return e == *this; });
		return ((itthis != vec->end()) && itthis < itrhs);
	}
	inline bool operator>(const vspec& rhs)
	{
		//specs vector in lhs can be null, but not in rhs
		PFC_ASSERT(rhs.vspecs != nullptr);
		vec_spec_t* vec = rhs.vspecs;
		auto itrhs = std::find_if(vec->begin(), vec->end(),
			[rhs](auto e) {return e == rhs; });
		auto itthis = std::find_if(vec->begin(), vec->end(),
			[=](auto e) {return e == *this; });

		return (itthis == vec->end() || itthis > itrhs);
	}

	bool signature(t_size boolvals, t_size intvals, t_size stringvals) {
		return (this->boolvals == boolvals &&
			this->intvals == intvals &&
			this->stringvals == stringvals);
	}
};

#endif

class new_conf
{
private:
	vec_spec_t vec_specs;
	
public:
	bool load();
	int id_to_val_int(int id, new_conf in_conf);
	bool id_to_val_bool(int id, new_conf in_conf);
	void id_to_val_str(int id, new_conf in_conf, pfc::string8& out);
	void save();
	//save just the active config tab
	void save_active_config_tab(int newval);

	enum class ConfFilter :uint8_t {
		CONF_FILTER_CONF = 0,
		CONF_FILTER_FIND,
		CONF_FILTER_PREVIEW,
		CONF_FILTER_TAG,
		CONF_FILTER_TRACK,
		CONF_FILTER_UPDATE_ART,
		CONF_FILTER_UPDATE_TAG,
		//v.205
		CONF_CAT_CREDIT,
		//..
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
		{ asi(ConfFilter::CONF_FILTER_CONF), CFG_RELEASE_ENTER_KEY_OVR},
		{ asi(ConfFilter::CONF_FILTER_CONF), CFG_CACHE_USE_OFFLINE_CACHE},
		//v205 (1.0.6/1.1.0)
		{ asi(ConfFilter::CONF_FILTER_CONF), CFG_DC_DB_PATH},
		{ asi(ConfFilter::CONF_FILTER_CONF), CFG_DC_DB_FLAG},
		{ asi(ConfFilter::CONF_FILTER_CONF), CFG_SKIP_MNG_FLAG},
		{ asi(ConfFilter::CONF_FILTER_CONF), CFG_LIST_STYLE},

		//..

		// CONF_FILTER_FIND (find_release_dialog)
		{ asi(ConfFilter::CONF_FILTER_FIND), CFG_DISPLAY_EXACT_MATCHES },
		//v200
		{ asi(ConfFilter::CONF_FILTER_FIND), CFG_FIND_RELEASE_DIALOG_SHOW_ID },
		//v205
		{ asi(ConfFilter::CONF_FILTER_FIND), CFG_FIND_RELEASE_FILTER_FLAG },
		//..

		// CONF_FILTER_PREVIEW (preview_dialog)
		{ asi(ConfFilter::CONF_FILTER_PREVIEW), CFG_PREVIEW_MODE },
		{ asi(ConfFilter::CONF_FILTER_PREVIEW), CFG_REPLACE_ANVS }, //<<
		//v200
		{ asi(ConfFilter::CONF_FILTER_PREVIEW), CFG_PREVIEW_TAGS_DIALOG_COL1_WIDTH },
		{ asi(ConfFilter::CONF_FILTER_PREVIEW), CFG_PREVIEW_TAGS_DIALOG_COL2_WIDTH },
		{ asi(ConfFilter::CONF_FILTER_PREVIEW), CFG_EDIT_TAGS_DIALOG_SHOW_TM_STATS },
		//v201	
		{ asi(ConfFilter::CONF_FILTER_PREVIEW), CFG_PREVIEW_TAGS_DIALOG_W_WIDTH },
		{ asi(ConfFilter::CONF_FILTER_PREVIEW), CFG_PREVIEW_TAGS_DIALOG_U_WIDTH },
		{ asi(ConfFilter::CONF_FILTER_PREVIEW), CFG_PREVIEW_TAGS_DIALOG_S_WIDTH },
		{ asi(ConfFilter::CONF_FILTER_PREVIEW), CFG_PREVIEW_TAGS_DIALOG_E_WIDTH },
		//v203
		{ asi(ConfFilter::CONF_FILTER_PREVIEW), CFG_ALBUM_ART_SKIP_DEFAULT_CUST },
		//v204 (1.0.4)
		{ asi(ConfFilter::CONF_FILTER_PREVIEW), CFG_EDIT_TAGS_DIALOG_FLAGS},
		//..

		// CONF_FILTER_TAG (tag_mappings_dialog)
		{ asi(ConfFilter::CONF_FILTER_TAG), CFG_EDIT_TAGS_DIALOG_COL1_WIDTH },
		{ asi(ConfFilter::CONF_FILTER_TAG), CFG_EDIT_TAGS_DIALOG_COL2_WIDTH },
		{ asi(ConfFilter::CONF_FILTER_TAG), CFG_EDIT_TAGS_DIALOG_COL3_WIDTH },
		//v200
		{ asi(ConfFilter::CONF_FILTER_TAG), CFG_EDIT_TAGS_DIALOG_HL_KEYWORD },
		//..

		// CONF_FILTER_TRACK (track_matching_dialog)
		//v200
		{ asi(ConfFilter::CONF_FILTER_TRACK), CFG_MATCH_TRACKS_DISCOGS_COL1_WIDTH },
		{ asi(ConfFilter::CONF_FILTER_TRACK), CFG_MATCH_TRACKS_DISCOGS_COL2_WIDTH },
		{ asi(ConfFilter::CONF_FILTER_TRACK), CFG_MATCH_TRACKS_FILES_COL1_WIDTH },
		{ asi(ConfFilter::CONF_FILTER_TRACK), CFG_MATCH_TRACKS_FILES_COL2_WIDTH },
		{ asi(ConfFilter::CONF_FILTER_TRACK), CFG_MATCH_TRACKS_DISCOGS_STYLE },
		{ asi(ConfFilter::CONF_FILTER_TRACK), CFG_MATCH_TRACKS_FILES_STYLE },
		{ asi(ConfFilter::CONF_FILTER_TRACK), CFG_DISCOGS_ARTWORK_RA_WIDTH },
		{ asi(ConfFilter::CONF_FILTER_TRACK), CFG_DISCOGS_ARTWORK_TYPE_WIDTH },
		{ asi(ConfFilter::CONF_FILTER_TRACK), CFG_DISCOGS_ARTWORK_DIM_WIDTH },
		{ asi(ConfFilter::CONF_FILTER_TRACK), CFG_DISCOGS_ARTWORK_SAVE_WIDTH },
		{ asi(ConfFilter::CONF_FILTER_TRACK), CFG_DISCOGS_ARTWORK_OVR_WIDTH },
		{ asi(ConfFilter::CONF_FILTER_TRACK), CFG_DISCOGS_ARTWORK_EMBED_WIDTH },
		{ asi(ConfFilter::CONF_FILTER_TRACK), CFG_MATCH_FILE_ARTWORK_NAME_WIDTH },
		{ asi(ConfFilter::CONF_FILTER_TRACK), CFG_MATCH_FILE_ARTWORK_DIM_WIDTH },
		{ asi(ConfFilter::CONF_FILTER_TRACK), CFG_MATCH_FILE_ARTWORK_SIZE_WIDTH },
		{ asi(ConfFilter::CONF_FILTER_TRACK), CFG_MATCH_DISCOGS_ARTWORK_STYLE },
		{ asi(ConfFilter::CONF_FILTER_TRACK), CFG_MATCH_FILE_ARTWORKS_STYLE },
		{ asi(ConfFilter::CONF_FILTER_TRACK), CFG_ALBUM_ART_SKIP_DEFAULT_CUST },

		//..CFG_EDIT_CAT_CREDIT_DIALOG_POS

		//v205
		//..


		// CONF_FILTER_UPDATE_ART (update_art_dialog)
		{ asi(ConfFilter::CONF_FILTER_UPDATE_ART), CFG_UPDATE_ART_FLAGS },
		//v200
		//..

		// CONF_FILTER_UPDATE_TAG (update_tags_dialog)
		{ asi(ConfFilter::CONF_FILTER_UPDATE_TAG), CFG_REPLACE_ANVS },	//<<
		{ asi(ConfFilter::CONF_FILTER_UPDATE_TAG), CFG_UPDATE_TAGS_MANUALLY_PROMPT },
		{ asi(ConfFilter::CONF_FILTER_UPDATE_TAG), CFG_UPDATE_PREVIEW_CHANGES },
		//v200
		//..
	};

	void save(ConfFilter cfgfilter, new_conf in_conf);
	void save(ConfFilter cfgfilter, new_conf in_conf, int id);
	
	bool replace_ANVs = false;
	bool move_the_at_beginning = true;
	bool discard_numeric_suffix = true;

	bool double_digits_track_numbers = true;
	bool number_each_disc_from_one = true;

	bool match_tracks_using_duration = true;
	bool match_tracks_using_number = true;
	bool assume_tracks_sorted = false;

	//depri v205
	bool skip_release_dialog_if_matched_depri = false;
	bool skip_find_release_dialog_if_ided_depri = false;
	bool skip_preview_dialog_depri = false;
	//

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

	// TODO: depri
	bool display_ANVs = false;

	// TODO: expand
	int update_art_flags;

	pfc::string8 oauth_token = "";
	pfc::string8 oauth_token_secret = "";

	int edit_tags_dialog_col1_width = 0;
	int edit_tags_dialog_col2_width = 0;
	int edit_tags_dialog_col3_width = 0;

	int last_conf_tab = 0;

	int cache_max_objects = 1000;

	bool remove_other_tags = false;
	pfc::array_t<pfc::string8> remove_exclude_tags;
	pfc::string8 raw_remove_exclude_tags;

	inline void set_remove_exclude_tags(const pfc::string8 &s) {
		tokenize(s, ",", remove_exclude_tags, true);
		raw_remove_exclude_tags = s;
	}

	formatting_script search_release_format_string = "$join($append(%RELEASE_TITLE%,%RELEASE_SEARCH_LABELS%,%RELEASE_SEARCH_MAJOR_FORMATS%,%RELEASE_SEARCH_FORMATS%,%RELEASE_YEAR%,%RELEASE_SEARCH_CATNOS%))";
	formatting_script search_master_format_string = "'[master] '$join($append(%MASTER_RELEASE_TITLE%,%MASTER_RELEASE_YEAR%))";
	formatting_script search_master_sub_format_string = "'  '$ifequal(%RELEASE_ID%,%MASTER_RELEASE_MAIN_RELEASE_ID%,'* ','   ')$join($append(%RELEASE_TITLE%,%RELEASE_SEARCH_LABELS%,%RELEASE_SEARCH_MAJOR_FORMATS%,%RELEASE_SEARCH_FORMATS%,%RELEASE_YEAR%,%RELEASE_SEARCH_CATNOS%,%RELEASE_COUNTRY%))";

	formatting_script release_discogs_format_string = "$ifgreater(%RELEASE_TOTAL_DISCS%,1,%DISC_NUMBER%-,)$num(%TRACK_DISC_TRACK_NUMBER%,2) - $multi_if($multi_and(%ARTISTS_NAME_VARIATION%,$multi_not(%REPLACE_ANVS%)),%ARTISTS_NAME_VARIATION%$multi_if(%DISPLAY_ANVS%,*,),%ARTISTS_NAME%) - %TRACK_TITLE%$ifequal(%TRACK_TOTAL_HIDDEN_TRACKS%,0,,'   [+'%TRACK_TOTAL_HIDDEN_TRACKS%' HIDDEN]')";
	formatting_script release_file_format_string = "$if($strcmp($ext(%path%),tags),$info(@),%path%)";

	//v.200
	int preview_tags_dialog_col1_width = 0;
	int preview_tags_dialog_col2_width = 0;
	int match_tracks_discogs_col1_width = 0;
	int match_tracks_discogs_col2_width = 0;
	int match_tracks_files_col1_width = 0;
	int match_tracks_files_col2_width = 0;

	pfc::string8 edit_tags_dlg_hl_keyword = "";
	bool edit_tags_dlg_show_tm_stats = false;
	bool find_release_dialog_show_id = false;

	bool release_enter_key_override = true;
	int match_tracks_discogs_style = static_cast<int>(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_HEADERDRAGDROP);
	int match_tracks_files_style = static_cast<int>(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_HEADERDRAGDROP);
	
	int preview_tags_dialog_w_width = 40;
	int preview_tags_dialog_u_width = 40;
	int preview_tags_dialog_s_width = 40;
	int preview_tags_dialog_e_width = 40;

	//ol::CacheFlags::OC_READ = 1 << 0,
	//ol::CacheFlags::OC_WRITE = 1 << 1
	int cache_use_offline_cache = 1 << 0 | 1 << 1;

	int match_discogs_artwork_ra_width = 0;
	int match_discogs_artwork_type_width = 0;
	int match_discogs_artwork_dim_width = 0;
	int match_discogs_artwork_save_width = 0;
	int match_discogs_artwork_ovr_width = 0;
	int match_discogs_artwork_embed_width = 0;
	int match_file_artwork_name_width = 0;
	int match_file_artwork_dim_width = 0;
	int match_file_artwork_size_width = 0;
	int match_discogs_artwork_art_style = static_cast<int>(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_HEADERDRAGDROP);
	int match_file_artworks_art_style = static_cast<int>(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_HEADERDRAGDROP);
	int album_art_skip_default_cust = 0;
	//v204 from 1.0.4
	int edit_tags_dialog_flags = 0;
	//v205 from 1.0.6
	pfc::string8 db_dc_path = "";
	int db_dc_flag = 0;
	int find_release_filter_flag = 0;
	int skip_mng_flag = 0;
	int list_style = 2;
	//..
};

typedef new_conf foo_discogs_conf;

extern new_conf CONF;

void init_conf();
