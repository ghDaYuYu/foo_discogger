#include "stdafx.h"

#include "tags.h"
#include "foo_discogs.h"


// {F932A0A3-554C-4F4C-B908-7A3758ED2620}
static const GUID guid_cfg_tag_mappings = {0xf932a0a3, 0x554c, 0x4f4c, {0xb9, 0x8, 0x7a, 0x37, 0x58, 0xed, 0x26, 0x20}};

cfg_tag_mapping_list_type cfg_tag_mappings(guid_cfg_tag_mappings);

static tag_mapping_entry default_tag_mappings[] = {
	// tagname | write | update | freezewrite | freezeupdate | freeztagname | formattingstring
	{"ARTIST", true, false, false, false, false, "$joinnames(%<ARTISTS_NAME>%,%<ARTISTS_JOIN>%)"},
	{"TITLE", true, false, false, false, false, "%TRACK_TITLE%"},
	{"ALBUM", true, false, false, false, false, "%RELEASE_TITLE%"},
	{"DATE", true, false, false, false, false, "%RELEASE_YEAR%"},
	{"GENRE", true, false, false, false, false, "%<RELEASE_GENRES>%"},
	{"COMPOSER", false, false, false, false, false, ""},
	{"PERFORMER", false, false, false, false, false, ""},
	{"ALBUM ARTIST", true, false, false, false, false, "$joinnames(%<RELEASE_ARTISTS_NAME>%,%<RELEASE_ARTISTS_JOIN>%)"},
	{"TRACKNUMBER", true, false, false, false, false, "$num(%TRACK_DISC_TRACK_NUMBER%,2)"},
	{"TOTALTRACKS", true, false, false, false, false, "%RELEASE_TOTAL_TRACKS%"},
	{"DISCNUMBER", true, false, false, false, false, "%DISC_NUMBER%"},
	{"TOTALDISCS", true, false, false, false, false, "%RELEASE_TOTAL_DISCS%"},
	{"COMMENT", false, false, false, false, false, ""},
	{"STYLE", true, false, false, false, false, "%<RELEASE_STYLES>%"},
	{"DISCOGS_LABEL", true, false, false, false, false, "$unique(%<RELEASE_LABELS_NAME>%)"},
	{"DISCOGS_CATALOG", true, false, false, false, false, "%<RELEASE_LABELS_CATALOG_NUMBER>%"},
	{"DISCOGS_COUNTRY", true, false, false, false, false, "%RELEASE_COUNTRY%"},
	{TAG_RELEASE_ID, true, false, false, false, true, "%RELEASE_ID%"},
	{TAG_MASTER_RELEASE_ID, true, false, false, false, true, "%MASTER_RELEASE_ID%"},
	{TAG_ARTIST_ID, true, false, false, false, true, "%<ARTISTS_ID>%"},
	{TAG_LABEL_ID, true, false, false, false, true, "$unique(%<RELEASE_LABELS_ID>%)"},
	{"DISCOGS_FORMAT", false, false, false, false, false, "$zip2($multi_if($multi_greater(%<RELEASE_FORMATS_QUANTITY>%,1),$zip(%<RELEASE_FORMATS_QUANTITY>%,' x '),),%<RELEASE_FORMATS_NAME>%,$multi_if($put(D,$join(%<<RELEASE_FORMATS_DESCRIPTIONS>>%)),', ',),$get(D),$multi_if($put(T,%<RELEASE_FORMATS_TEXT>%),', ',),$multi_if2($get(T),))"},
	{"DISCOGS_RELEASED", true, false, false, false, false, "%RELEASE_DATE_RAW%"},
	{"DISCOGS_RELEASE_MONTH", true, false, false, false, false, "%RELEASE_MONTH%"},
	{"DISCOGS_RELEASE_DAY", false, false, false, false, false, "%RELEASE_DAY%"}, 
	{"DISCOGS_SERIES", false, false, false, false, false, "%<RELEASE_SERIES_NAME>%"},
	{"DISCOGS_SERIES_NUMBER", false, false, false, false, false, "%<RELEASE_SERIES_NUMBER>%"},
	{"DISCOGS_RATING", true, true, false, false, false, "%RELEASE_DISCOGS_AVG_RATING%"},
	{"DISCOGS_VOTES", true, true, false, false, false, "%RELEASE_DISCOGS_RATING_VOTES%"},
	{"DISCOGS_SUBMITTED_BY", false, false, false, false, false, "%RELEASE_DISCOGS_SUBMITTED_BY%"},
	{"DISCOGS_MEMBERS_HAVE", false, false, false, false, false, "%RELEASE_DISCOGS_USERS_WANT%"},
	{"DISCOGS_MEMBERS_WANT", false, false, false, false, false, "%RELEASE_DISCOGS_USERS_HAVE%"},
	{"DISCOGS_ARTISTS_ALIASES", false, false, false, false, false, "%<<ARTISTS_ALIASES>>%"},
	{"DISCOGS_ARTISTS_INGROUPS", false, false, false, false, false, "%<<ARTISTS_GROUPS>>%"},
	{"DISCOGS_ARTISTS_ALL_NAME_VARIATIONS", false, false, false, false, false, "%<<ARTISTS_ALL_NAME_VARIATIONS>>%"},
	{"DISCOGS_ARTISTS_MEMBERS", false, false, false, false, false, "%<<ARTISTS_MEMBERS>>%"},
	{"DISCOGS_ARTISTS_URLS", false, false, false, false, false, "%<<ARTISTS_URLS>>%"},
	{"DISCOGS_ARTIST_REALNAME", false, false, false, false, false, "%<ARTISTS_REAL_NAME>%"},
	{"DISCOGS_ARTIST_PROFILE", false, false, false, false, false, "%<ARTISTS_PROFILE>%"},
	{"DISCOGS_RELEASE_NOTES", false, false, false, false, false, "%RELEASE_NOTES%"},
	{"DISCOGS_RELEASE_CREDITS", false, false, false, false, false, "$zip($join(%<<RELEASE_CREDITS_ROLES>>%),' - ',$join(%<<RELEASE_CREDITS_ARTISTS_NAME>>%))"},
	{"DISCOGS_TRACK_CREDITS", false, false, false, false, false, "$zip($join(%<<TRACK_CREDITS_ROLES>>%),' - ',$join(%<<TRACK_CREDITS_ARTISTS_NAME>>%))"},
	{"DISCOGS_CREDIT_FEATURING", false, false, false, false, false, "$flatten($multi_if($any($multi_strcmp($sextend(%<<TRACK_CREDITS_SHORT_ROLES>>%,%<<RELEASE_CREDITS_SHORT_ROLES>>%),'Featuring')),$multi_if($put(aj,$sextend(%<<TRACK_CREDITS_ARTISTS_JOIN>>%,%<<RELEASE_CREDITS_ARTISTS_JOIN>>%)),$joinnames($put(an,$sextend(%<<TRACK_CREDITS_ARTISTS_NAME>>%,%<<RELEASE_CREDITS_ARTISTS_NAME>>%)),$get(aj)),$get(an)),))"},
	{"DISCOGS_CREDIT_VOCALS", false, false, false, false, false, "$flatten($multi_if($any($multi_strcmp($sextend(%<<TRACK_CREDITS_SHORT_ROLES>>%,%<<RELEASE_CREDITS_SHORT_ROLES>>%),'Vocals')),$multi_if($put(aj,$sextend(%<<TRACK_CREDITS_ARTISTS_JOIN>>%,%<<RELEASE_CREDITS_ARTISTS_JOIN>>%)),$joinnames($put(an,$sextend(%<<TRACK_CREDITS_ARTISTS_NAME>>%,%<<RELEASE_CREDITS_ARTISTS_NAME>>%)),$get(aj)),$get(an)),))"},
	{"DISCOGS_CREDIT_REMIXED_BY", false, false, false, false, false, "$flatten($multi_if($any($multi_strcmp($sextend(%<<TRACK_CREDITS_SHORT_ROLES>>%,%<<RELEASE_CREDITS_SHORT_ROLES>>%),'Remix')),$multi_if($put(aj,$sextend(%<<TRACK_CREDITS_ARTISTS_JOIN>>%,%<<RELEASE_CREDITS_ARTISTS_JOIN>>%)),$joinnames($put(an,$sextend(%<<TRACK_CREDITS_ARTISTS_NAME>>%,%<<RELEASE_CREDITS_ARTISTS_NAME>>%)),$get(aj)),$get(an)),))"},
	{"DISCOGS_ARTISTS", false, false, false, false, false, "%<ARTISTS_NAME>%"},
	{"DISCOGS_ALBUM_ARTISTS", false, false, false, false, false, "%<RELEASE_ARTISTS_NAME>%"},
	{"DISCOGS_ARTIST_NAME_VARIATIONS", false, false, false, false, false, "%<ARTISTS_NAME_VARIATION>%"},
	{"DISCOGS_TRACK_HEADING", false, false, false, false, false, "%TRACK_HEADING%"},
	{"DISCOGS_INDEX_TRACK_TITLE", false, false, false, false, false, "%TRACK_INDEXTRACK_TITLE%"},
	{"DISCOGS_SUB_TRACK_TITLE", false, false, false, false, false, "%TRACK_SUBTRACK_TITLE%"},
	{"DISCOGS_TRACK_DURATION", false, false, false, false, false, "%TRACK_DISCOGS_DURATION_RAW%"},
	{"VINYLTRACK", false, false, false, false, false, "$multi_if($multi_or($multi_strstr($put(fmt,%<RELEASE_FORMATS_NAME>%),'Vinyl'),$multi_strstr($get(fmt),'Acetate')),%TRACK_DISCOGS_TRACK_NUMBER%,)"},
};


bool operator ==(const tag_mapping_entry &a, const tag_mapping_entry &b) {
	return a.tag_name == b.tag_name;  // compares memory...?
}
void init_tag_mappings() {
	if (!cfg_tag_mappings.get_count()) {
		init_default_tag_mappings();
	}
	else {
		for (size_t i = 0; i < cfg_tag_mappings.get_count(); i++) {
			if (cfg_tag_mappings[i].freeze_tag_name || cfg_tag_mappings[i].freeze_update || cfg_tag_mappings[i].freeze_write) {
				const size_t count = sizeof(default_tag_mappings) / sizeof(tag_mapping_entry);
				bool found = false;
				for (size_t j = 0; j < count; j++) {
					
					if (STR_EQUAL(default_tag_mappings[j].tag_name, cfg_tag_mappings[i].tag_name)) {
						found = true;
						break;
					}
				}
				if (!found) {
					cfg_tag_mappings[i].freeze_tag_name = false;
					cfg_tag_mappings[i].freeze_update = false;
					cfg_tag_mappings[i].freeze_write = false;
					cfg_tag_mappings[i].enable_write = false;
					cfg_tag_mappings[i].enable_update = false;
				}
			}
		}
	}
}
void init_default_tag_mappings() {
	cfg_tag_mappings.remove_all();
	const size_t count = sizeof(default_tag_mappings) / sizeof(tag_mapping_entry);
	for (size_t i = 0; i < count; i++) {
		cfg_tag_mappings.add_item(*(default_tag_mappings[i].clone()));
		cfg_tag_mappings[i].is_multival_meta = is_multivalue_meta(default_tag_mappings[i].tag_name);
	}
}

pfc::list_t<tag_mapping_entry> * copy_tag_mappings() {
	pfc::list_t<tag_mapping_entry>* mappings = new pfc::list_t<tag_mapping_entry>();
	for (size_t i = 0; i < cfg_tag_mappings.get_count(); i++) {
		mappings->add_item(cfg_tag_mappings.get_item(i));
	}
	return mappings;
}
pfc::list_t<tag_mapping_entry> * copy_default_tag_mappings() {
	pfc::list_t<tag_mapping_entry> *mappings = new pfc::list_t<tag_mapping_entry>();
	const size_t count = sizeof(default_tag_mappings) / sizeof(tag_mapping_entry);
	for (size_t i = 0; i < count; i++) {
		mappings->add_item(*(default_tag_mappings[i].clone()));
		tag_mapping_entry& new_map = mappings->get_item(i);
		new_map.is_multival_meta = is_multivalue_meta(default_tag_mappings[i].tag_name);
	}
	return mappings;
}
void set_cfg_tag_mappings(pfc::list_t<tag_mapping_entry> *mappings) {
	cfg_tag_mappings.remove_all();
	for (size_t i = 0; i < mappings->get_count(); i++) {
		cfg_tag_mappings.add_item(mappings->get_item(i));
	}
}
pfc::string8 get_default_tag(const pfc::string8 &name) {
	const size_t count = sizeof(default_tag_mappings) / sizeof(tag_mapping_entry);
	for (size_t i = 0; i < count; i++) {
		if (STR_EQUAL(default_tag_mappings[i].tag_name, name)) {
			return default_tag_mappings[i].formatting_script;
		}
	}
	return "";
}
