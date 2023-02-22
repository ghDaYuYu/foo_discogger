#include "stdafx.h"

#include "tags.h"
#include "foo_discogs.h"

// {F932A0A3-554C-4F4C-B908-7A3758ED2620}
static const GUID guid_cfg_tag_mappings = {0xf932a0a3, 0x554c, 0x4f4c, {0xb9, 0x8, 0x7a, 0x37, 0x58, 0xed, 0x26, 0x20}};

cfg_tag_mapping_list_type cfg_tag_mappings(guid_cfg_tag_mappings);

const size_t kPretags = 4;

//todo: move to db

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
	{"DISCOGS_DATE_ADDED", false, false, false, false, false, "%RELEASE_DISCOGS_DATE_ADDED%"},
	{"DISCOGS_DATE_CHANGED", false, false, false, false, false, "%RELEASE_DISCOGS_DATE_CHANGED%"},
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

static tag_mapping_entry ID3_FrameDefs_MS[] =
{
	{TAG_RELEASE_ID          , true,  false, false, false, true,  "%RELEASE_ID%"},
	{TAG_MASTER_RELEASE_ID   , false, false, false, false, true,  "%MASTER_RELEASE_ID%"},
	{TAG_ARTIST_ID           , false, false, false, false, true,  "%<ARTISTS_ID>%"},
	{TAG_LABEL_ID            , false, false, false, false, true,  "$unique(%<RELEASE_LABELS_ID>%)"},
	{ "Title"                , true,  false, false, false, false, "%TRACK_TITLE%"},
	{ "Subtitle"             , true,  false, false, false, false, "%TRACK_SUBTRACK_TITLE%" },
	{ "RAting"               , true,  false, false, false, false, "%RELEASE_DISCOGS_AVG_RATING%"},
	{ "Comment"              , true,  false, false, false, false, "" },
	{ "Artist"               , true,  false, false, false, false, "$joinnames(%<ARTISTS_NAME>%,%<ARTISTS_JOIN>%)"},
	{ "Album artist"         , true,  false, false, false, false, "$joinnames(%<RELEASE_ARTISTS_NAME>%,%<RELEASE_ARTISTS_JOIN>%)"},
	{ "Album"                , true,  false, false, false, false, "%RELEASE_TITLE%"},
	{ "Date"                 , true,  false, false, false, false, "%RELEASE_YEAR%"},
	{ "Tracknumber"          , true,  false, false, false, false, "$ifequal($put(trks,$num(%TRACK_DISC_TRACK_NUMBER%,1)),0,,$get(trks))$ifequal($get(trks),0,,$ifequal(%RELEASE_TOTAL_TRACKS%,0,,/$num(%RELEASE_TOTAL_TRACKS%,1)))"},
	{ "Genre"                , true,  false, false, false, false, "%<RELEASE_GENRES>%"},
	{ "Publisher"            , true,  false, false, false, false, "$unique(%<RELEASE_LABELS_NAME>%)"},
	{ "Encoded by"           , true,  false, false, false, false, "$flatten($multi_if($any($multi_strcmp($sextend(%<<TRACK_CREDITS_SHORT_ROLES>>%,%<<RELEASE_CREDITS_SHORT_ROLES>>%),'Encoding By')),$multi_if($put(aj,$sextend(%<<TRACK_CREDITS_ARTISTS_JOIN>>%,%<<RELEASE_CREDITS_ARTISTS_JOIN>>%)),$joinnames($put(an,$sextend(%<<TRACK_CREDITS_ARTISTS_NAME>>%,%<<RELEASE_CREDITS_ARTISTS_NAME>>%)),$get(aj)),$get(an)),))"},
	{ "Artist webpage URL"   , true,  false, false, false, false, "https://www.discogs.com/artist/$element(%<ARTISTS_ID>%,0)"},
	{ "Copyright"            , true,  false, false, false, false, "$zip(%<RELEASE_COMPANIES_ENTITY_TYPE_NAME>%, - ,%<RELEASE_COMPANIES_NAME>%)"},
	{ "Composer"             , true,  false, false, false, false, "$flatten($multi_if($any($multi_strcmp($sextend(%<<TRACK_CREDITS_SHORT_ROLES>>%,%<<RELEASE_CREDITS_SHORT_ROLES>>%),'Written-By')),$multi_if($put(aj,$sextend(%<<TRACK_CREDITS_ARTISTS_JOIN>>%,%<<RELEASE_CREDITS_ARTISTS_JOIN>>%)),$joinnames($put(an,$sextend(%<<TRACK_CREDITS_ARTISTS_NAME>>%,%<<RELEASE_CREDITS_ARTISTS_NAME>>%)),$get(aj)),$get(an)),))"},  //Writing & Arrangement* "Composed By"* "Written-By"
	{ "Conductor"            , true,  false, false, false, false, "$flatten($multi_if($any($multi_strcmp($sextend(%<<TRACK_CREDITS_SHORT_ROLES>>%,%<<RELEASE_CREDITS_SHORT_ROLES>>%),'Conductor')),$multi_if($put(aj,$sextend(%<<TRACK_CREDITS_ARTISTS_JOIN>>%,%<<RELEASE_CREDITS_ARTISTS_JOIN>>%)),$joinnames($put(an,$sextend(%<<TRACK_CREDITS_ARTISTS_NAME>>%,%<<RELEASE_CREDITS_ARTISTS_NAME>>%)),$get(aj)),$get(an)),))"}, //Conducting & Leading "Conductor"* "Leader"* "Directed By"
	{ "Grouping"             , true,  false, false, false, false, "%<RELEASE_SERIES_NAME>%"},
	{ "DiscNumber"           , true,  false, false, false, false, "%DISC_NUMBER%"},
	{ "Initial key"          , true,  false, false, false, false, ""},
	{ "BPM"                  , true,  false, false, false, false, ""},
	{ "iTunesCompilation"    , true,  false, false, false, false, ""},
	{ "WWW"                  , true,  false, false, false, false, "https://www.discogs.com/release/%RELEASE_ID%"},
//..
	{ "TotalDiscs"           , true,  false, false, false, false, "%RELEASE_TOTAL_DISCS%"},
	{ "Lyricist"             , true,  false, false, false, false, "$flatten($multi_if($any($multi_strcmp($sextend(%<<TRACK_CREDITS_SHORT_ROLES>>%,%<<RELEASE_CREDITS_SHORT_ROLES>>%),'Lyrics By')),$multi_if($put(aj,$sextend(%<<TRACK_CREDITS_ARTISTS_JOIN>>%,%<<RELEASE_CREDITS_ARTISTS_JOIN>>%)),$joinnames($put(an,$sextend(%<<TRACK_CREDITS_ARTISTS_NAME>>%,%<<RELEASE_CREDITS_ARTISTS_NAME>>%)),$get(aj)),$get(an)),))"},
	{ "Remixed by"           , true,  false, false, false, false, "$flatten($multi_if($any($multi_strcmp($sextend(%<<TRACK_CREDITS_SHORT_ROLES>>%,%<<RELEASE_CREDITS_SHORT_ROLES>>%),'Remix')),$multi_if($put(aj,$sextend(%<<TRACK_CREDITS_ARTISTS_JOIN>>%,%<<RELEASE_CREDITS_ARTISTS_JOIN>>%)),$joinnames($put(an,$sextend(%<<TRACK_CREDITS_ARTISTS_NAME>>%,%<<RELEASE_CREDITS_ARTISTS_NAME>>%)),$get(aj)),$get(an)),))"},
	{ "Media type"           , true,  false, false, false, false, "$zip2($multi_if($multi_greater(%<RELEASE_FORMATS_QUANTITY>%,1),$zip(%<RELEASE_FORMATS_QUANTITY>%,' x '),),%<RELEASE_FORMATS_NAME>%,$multi_if($put(D,$join(%<<RELEASE_FORMATS_DESCRIPTIONS>>%)),', ',),$get(D),$multi_if($put(T,%<RELEASE_FORMATS_TEXT>%),', ',),$multi_if2($get(T),))"},
};

struct ID3_FrameDef {
	bool bcpx;
	bool bfb2kna;
	bool w;
	bool ms;
	pfc::string8 fid;
	bool bna;
	bool bna2;
	pfc::string8 field;
};

static ID3_FrameDef ID3_FrameDefs[] =
{
	//cpx (Complex)
	//cpx fb2kna	  w	      ms      id    na      na2   field
	{false, true,  true,  false, "DISCOGS_RELEASE_ID", false, false, "%RELEASE_ID%"},
	{false, true,  true,  false, "DISCOGS_MASTER_RELEASE_ID", false, false, "%MASTER_RELEASE_ID%"},
	{false, true,  true,  false, "DISCOGS_ARTIST_ID", false, false, "%<ARTISTS_ID>%"},
	{false, true,  true,  false, "DISCOGS_LABEL_ID", false, false, "$unique(%<RELEASE_LABELS_ID>%)"},

	{true,  false, false, false, "APIC", true,  false, "Attached picture"},
	{true,  false, false, false, "COMR", true,  false, "Commercial"/*"Commercial"*/},
	{true,  false, false, false, "ENCR", true,  false, "Encryption method registration"},
	{true,  false, false, false, "EQUA", true,  true,  "Equalization"},
	{true,  false, false, false, "ETCO", true,  true,  "Event timing codes"},
	{true,  false, false, false, "GEOB", true,  false, "General encapsulated object"},
	{true,  false, false, false, "GRID", true,  false, "Group identification registration"},
	{true,  true,  false, false, "LINK", false, false, "Linked information"},
	{true,  true,  false, false, "MCDI", false, false, "Music CD identifier"},
	{true,  true,  false, false, "MLLT", false, true,  "MPEG location lookup table"},
	{true,  true,  false, false, "OWNE", false, false, "Ownership frame"},
	{true,  true,  false, false, "PRIV", false, false, "Private frame"},
	{false, true,  false, false, "PCNT", false, false, "Play counter"},
	{true,  true,  false, false, "POSS", false, true,  "Position synchronisation frame"},
	{true,  true,  false, false, "RBUF", false, false, "Recommended buffer size"},
	{true,  true,  false, false, "RVAD", false, true,  "Relative volume adjustment"},
	{true,  true,  false, false, "RVRB", false, false, "Reverb"},
	{true,  true,  false, false, "SYLT", false, false, "Synchronized lyric/text"},
	{true,  true,  false, false, "SYTC", false, true,  "Synchronized tempo codes"},
	{false, true,  false, false, "TDLY", false, false, "Playlist delay"},
	{false, false, false, false, "TFLT", true,  false, "File type"},
	{false, false, false, false, "TIME", false, false, "Time"},
	{true,  true,  false, false, "TSIZ", false, false, "Size"},
	{true,  true,  false, false, "TLEN", false, false, "Length"},

	//titles
	{false, false, true,  true,  "TALB", false, true,  "Album"/*"Album/Movie/Show title"*/},
	{false, false, true,  true,  "TIT1", true,  false, "Grouping"/*"Content group description"*/},
	{false, false, true,  true,  "TKEY", false, true,  "Initial key"},
	{false, true,  true,  false, "MVNM", false, true,  "Movement name"},
	{false, false, true,  false, "TOAL", false, true,  "Original album"/*"Original album/movie/show title"*/},
	{false, true,  false, false, "TOFN", false, false, "Original filename"},
	{false, true,  true,  false, "TSST", false, true,  "Set subtitle"},
	{false, false, true,  true,  "TIT3", false, true,  "Subtitle"/*"Subtitle/Description refinement"*/},
	{false, false, true,  true,  "TIT2", false, true,  "Title"/*"Title/songname/content description"*/},
	{false, true,  true,  false, "TSOT", false, true,  "TitleSortOrder"},

	//people
	{false, false, true,  true,  "TPE1", false, true,  "Artist" /*"Lead performer(s)/Soloist(s)"*/},
	{false, false, true,  true,  "TPE2", true,  true,  "Album artist" /*"Band/orchestra/accompaniment"*/},
	{false, false, true,  true,  "TCOM", false, true,  "Composer"},
	{false, false, true,  true,  "TPE3", false, true,  "Conductor"/*"Conductor/performer refinement"*/},
	{false, true,  false, false, "IPLS", false, false, "Involved people list"},
	{false, false, true,  false, "TEXT", false, true,  "Lyricist"/*"Lyricist/Text writer"*/},
	{false, false, true,  true,  "TPUB", false, true,  "Publisher"},
	{false, false, true,  false, "TOPE", false, true,  "Original artist"/*"Original artist(s)/performer(s)"*/},
	{false, true,  false, false, "TOLY", false, false, "Original lyricist"/*"Original lyricist(s)/text writer(s)"*/},
	{false, false, true,  false, "TOWN", true,  true,  "Owner"/*"File owner/licensee"*/},
	{false, false, true,  false, "TRSN", true,  true,  "Radio station"/*"Internet radio station name"*/},
	{false, false, true,  false, "TRSO", true,  true,  "Radio station owner"/*"Internet radio station owner"*/},
	{false, false, true,  false, "TPE4", true,  true,  "Remixed by"/*"Interpreted, remixed, or otherwise modified by"*/},
	{false, false, true,  false, "TSOC", false, false, "ComposerSortOrder"},
	{false, false, true,  false, "TSOP", false, false, "ArtistSortOrder"},
	{false, false, true,  false, "TSOA", false, false, "AlbumSortOrder"},
	{false, false, true,  false, "TSO2", false, false, "AlbumArtistSortOrder"},

	//counts
	{false, false, true,  true, "TBPM", false, true,  "BPM"/*"BPM (beats per minute)"*/},
	{false, false, true,  true, "TPOS", false, true,  "DiscNumber"},
	{false, true,  true,  false,"MVIN", false, true,  "Movement count"},
	{false, true,  true,  false,"MVIN", false, true,  "Movement index"},
	{true,  false, true,  true, "POPM", false, false, "Rating"/*"Popularimeter"*/},
	{false, false, true,  true, "TRCK", false, true,  "Tracknumber"/*"Track number/Position in set"*/},
	{false, false, true,  false,"TPOS", false, true,  "TotalDiscs"},

	//dates
	{false, false, true,  true, "TYER", true,  true,  "Date"},
	{false, true,  true,  false,"TDAT", false, false, "Date"},
	{false, true,  true,  false,"TORY", false, true,  "Original release date"/*"Original release year"*/},
	{false, false, true,  false,"TRDA", false, true,  "Recording dates"},

	//identifier
	{false, true,  false, false,"UFID", false, false, "UFID"/*"Unique file identifier"*/},
	{false, true,  false, false,"TSRC", false, true,  "ISRC" /*"ISRC (international standard recording code)"*/},

	//flags
	{false, false, true,  true, "TCMP", false, true,  "iTunesCompilation"},

	//ripping
	{true,  false, false, false,"AENC", true,  false, "Audio encryption"},
	{false, false, true,  true, "TENC", false, true,  "Encoded by"},
	{false, false, true,  false,"TSSE", false, true,  "Encoding settings"/*"Software/Hardware and settings used for encoding"*/},
	{false, false, true,  false,"TMED", false, true,  "Media type"},

	//url
	{false, false, true,  true, "WOAR", false, true,  "Artist webpage URL"/*"Official artist/performer webpage"*/},
	{false, false, true,  false,"WCOM", false, true,  "Commercial information URL"/*"Commercial information"*/},
	{false, false, true,  false,"WCOP", false, true,  "Copyright URL" /*"Copyright/Legal infromation"*/},
	{false, false, true,  false,"WORS", false, true,  "Internet radio webpage URL" /*"Official internet radio station homepage"*/},
	{false, false, true,  false,"WOAF", true,  true,  "File webpage URL"/*"Official audio file webpage"*/},
	{false, false, true,  false,"WPAY", false, true,  "Payment URL"/*"Payment"*/},
	{false, false, true,  false,"WPUB", false, true,  "Publisher URL"/*"Official publisher webpage"*/},
	{false, false, true,  false,"WOAS", false, true,  "Source webpage URL"/*"Official audio source webpage"*/},
	{false, false, true,  false,"WXXX", true,  true,  "WWW"/*"User defined URL link"*/},

	//style
	{false, false, true,  true, "TCON", true,  true,  "Genre"},

	//misc
	{false, false, true,  true, "COMM", false, true,  "Comment"/*"Comments"*/},
	{false, false, true,  true, "TCOP", true,  true,  "Copyright"},
	{false, false, true,  false,"TLAN", false, true,  "Language"/*"Language(s)"*/},
	{true,  true,  false, false,"USER", false, false, "Terms of use"},
	{false, false, false, false,"USLT", false, true,  "Unsynced lyric"/*"Unsynchronized lyric/text transcription"*/},

	//
	{false, false, false, false,"TXXX", false, false, "User defined text information"},
};

bool operator ==(const tag_mapping_entry &a, const tag_mapping_entry &b) {
	return a.tag_name == b.tag_name;  // compares memory...?
}
void init_tag_mappings() {

	if (!cfg_tag_mappings.get_count()) {

		init_with_default_tag_mappings();
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

void init_with_default_tag_mappings() {

	cfg_tag_mappings.remove_all();

	const size_t count = sizeof(default_tag_mappings) / sizeof(tag_mapping_entry);

	for (size_t i = 0; i < count; i++) {
		cfg_tag_mappings.add_item(*(default_tag_mappings[i].clone()));
		cfg_tag_mappings[i].is_multival_meta = is_multivalue_meta(default_tag_mappings[i].tag_name);
	}
}

pfc::list_t<tag_mapping_entry> * copy_tag_mappings() {
	pfc::list_t<tag_mapping_entry>* mappings = new pfc::list_t<tag_mapping_entry>();
	for (auto tag_entry : cfg_tag_mappings) {
		mappings->add_item(tag_entry);
	}
	return mappings;
}
pfc::list_t<tag_mapping_entry> * copy_default_tag_mappings() {

	pfc::list_t<tag_mapping_entry> *mappings = new pfc::list_t<tag_mapping_entry>();
	const size_t count = sizeof(default_tag_mappings) / sizeof(tag_mapping_entry);

	for (size_t i = 0; i < count; i++) {

		mappings->add_item(*(default_tag_mappings[i].clone()));
		tag_mapping_entry& new_map_entry = mappings->get_item(i);
		new_map_entry.is_multival_meta = is_multivalue_meta(default_tag_mappings[i].tag_name);
	}
	return mappings;
}

//menuctx (id3 split button) skips kPretags
pfc::list_t<tag_mapping_entry>* copy_id3_default_tag_mappings(bool onlyms, bool menuctx) {

	//init ms map
	std::map<std::string, pfc::string8> map_ms;
	const size_t cDefs = sizeof(ID3_FrameDefs_MS) / sizeof(tag_mapping_entry);
	for (size_t i = 0; i < cDefs; i++) {
		map_ms.insert(std::make_pair(ID3_FrameDefs_MS[i].tag_name.toLower(), ID3_FrameDefs_MS[i].formatting_script));
	}

	//id3 defs
	pfc::list_t<tag_mapping_entry>* mappings = new pfc::list_t<tag_mapping_entry>();
	const size_t cBase = sizeof(ID3_FrameDefs) / sizeof(ID3_FrameDef);

	size_t walk = menuctx ? kPretags : 0;
	for (walk; walk < cBase; walk++) {

		ID3_FrameDef fdef = ID3_FrameDefs[walk];
		bool pass = onlyms ? fdef.ms || walk < kPretags : fdef.w;

		if (pass) {

			pfc::string8 tagname;
			pfc::string8 formatstring;
			
			tagname = walk < kPretags ? fdef.fid : fdef.field;

			std::string what = tagname.toLower();
			auto foundit = std::find_if(map_ms.begin(), map_ms.end(), [&](const std::pair<std::string, pfc::string8>& val) {
				return val.first.compare(what) == 0;
				});

			if (foundit != map_ms.end()) {
				formatstring = foundit->second;
			}

			tag_mapping_entry entry;

			entry.tag_name = tagname;
			entry.formatting_script = formatstring;

			if (walk < kPretags) {
				entry.enable_write = !walk;
				entry.freeze_tag_name = true;				
			}
			else {
				entry.enable_write = true;
			}

			entry.is_multival_meta = is_multivalue_meta(entry.tag_name);
			mappings->add_item(*entry.clone());
		}
	}
	return mappings;
}

void set_cfg_tag_mappings(pfc::list_t<tag_mapping_entry> *mappings) {

	cfg_tag_mappings.remove_all();
	for (size_t i = 0; i < mappings->get_count(); i++) {
		cfg_tag_mappings.add_item(mappings->get_item(i));
	}
}

 bool awt_get_release_mod_flag(tag_mapping_entry& out) {
	for (auto& tag_entry : cfg_tag_mappings) {		
		bool release_id_mod = STR_EQUAL(TAG_RELEASE_ID, tag_entry.tag_name.get_ptr());
		if (release_id_mod) {
			out = tag_entry;
			return true;
		}
	}
	return false;
}

bool awt_set_release_mod_flag(tag_mapping_entry e) {
	for (auto & tag_entry : cfg_tag_mappings) {
		bool release_id_mod = STR_EQUAL(TAG_RELEASE_ID, tag_entry.tag_name.get_ptr());
		if (release_id_mod) {
			tag_entry.enable_write = e.enable_write;
			tag_entry.enable_update = e.enable_update;
			return true;
		}
	}
	return false;
}

int awt_update_mod_flag(bool fromFlag) {

	//lo former state, hi current

	tag_mapping_entry forth;
	tag_mapping_entry curr;
	awt_get_release_mod_flag(curr);
	auto leaving_val = (curr.enable_write ? 1 << 0 : 0) | (curr.enable_update ? 1 << 1 : 0);

	if (!fromFlag) {
		if (CONF.awt_alt_mode()) {
			CONF.alt_write_flags = MAKELPARAM(LOWORD(CONF.alt_write_flags), leaving_val);
		}
		else {
			CONF.alt_write_flags = MAKELPARAM(!leaving_val ? 1 : leaving_val, HIWORD(CONF.alt_write_flags));
		}
		return CONF.alt_write_flags;
	}

	if (CONF.awt_alt_mode()) {
			forth.enable_write = HIWORD(CONF.alt_write_flags) & (1 << 0);
			forth.enable_update = HIWORD(CONF.alt_write_flags) & (1 << 1);
	}
	else {
		//normal
		forth.enable_write = LOWORD(CONF.alt_write_flags) & (1 << 0);
		forth.enable_update = LOWORD(CONF.alt_write_flags) & (1 << 1);
	}
	awt_set_release_mod_flag(forth);

	return CONF.alt_write_flags;
}

bool awt_unmatched_flag() {

	//lo former state, hi current

	tag_mapping_entry forth;
	tag_mapping_entry curr;
	awt_get_release_mod_flag(curr);
	auto curr_val = (curr.enable_write ? 1 << 0 : 0) | (curr.enable_update ? 1 << 1 : 0);

	if (CONF.awt_alt_mode()) {
		//alt tag write
		return HIWORD(CONF.alt_write_flags) != curr_val;		
	}
	else {
		//normal tag write
		return LOWORD(CONF.alt_write_flags) != curr_val;
	}
}

void awt_save_normal_mode() {

	if (!CONF.awt_alt_mode()) {
		awt_update_mod_flag(/*from entry*/false);
	}
}

void update_loaded_tagmaps_multivalues() {
	for (auto& entry : cfg_tag_mappings) {
		entry.is_multival_meta = is_multivalue_meta(entry.tag_name);
	}

	for (auto& entry : default_tag_mappings) {
		entry.is_multival_meta = is_multivalue_meta(entry.tag_name);
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
