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
	{pfc::guid_null,"ARTIST", true, false, false, false, false, "$joinnames(%<ARTISTS_NAME>%,%<ARTISTS_JOIN>%)"},
	{pfc::guid_null,"TITLE", true, false, false, false, false, "%TRACK_TITLE%"},
	{pfc::guid_null,"ALBUM", true, false, false, false, false, "%RELEASE_TITLE%"},
	{pfc::guid_null,"DATE", true, false, false, false, false, "%RELEASE_YEAR%"},
	{pfc::guid_null,"GENRE", true, false, false, false, false, "%<RELEASE_GENRES>%"},
	{pfc::guid_null,"COMPOSER", false, false, false, false, false, ""},
	{pfc::guid_null,"PERFORMER", false, false, false, false, false, ""},
	{pfc::guid_null,"ALBUM ARTIST", true, false, false, false, false, "$joinnames(%<RELEASE_ARTISTS_NAME>%,%<RELEASE_ARTISTS_JOIN>%)"},
	{pfc::guid_null,"TRACKNUMBER", true, false, false, false, false, "$num(%TRACK_DISC_TRACK_NUMBER%,2)"},
	{pfc::guid_null,"TOTALTRACKS", true, false, false, false, false, "%RELEASE_TOTAL_TRACKS%"},
	{pfc::guid_null,"DISCNUMBER", true, false, false, false, false, "%DISC_NUMBER%"},
	{pfc::guid_null,"TOTALDISCS", true, false, false, false, false, "%RELEASE_TOTAL_DISCS%"},
	{pfc::guid_null,"COMMENT", false, false, false, false, false, ""},
	{pfc::guid_null,"STYLE", true, false, false, false, false, "%<RELEASE_STYLES>%"},
	{pfc::guid_null,"DISCOGS_LABEL", true, false, false, false, false, "$unique(%<RELEASE_LABELS_NAME>%)"},
	{pfc::guid_null,"DISCOGS_CATALOG", true, false, false, false, false, "%<RELEASE_LABELS_CATALOG_NUMBER>%"},
	{pfc::guid_null,"DISCOGS_COUNTRY", true, false, false, false, false, "%RELEASE_COUNTRY%"},
	{TAG_GUID_RELEASE_ID,TAG_RELEASE_ID, true, false, false, false, true, "%RELEASE_ID%"},
	{TAG_GUID_MASTER_RELEASE_ID,TAG_MASTER_RELEASE_ID, true, false, false, false, true, "%MASTER_RELEASE_ID%"},
	{TAG_GUID_ARTIST_ID,TAG_ARTIST_ID, true, false, false, false, true, "%<ARTISTS_ID>%"},
	{TAG_GUID_LABEL_ID,TAG_LABEL_ID, true, false, false, false, true, "$unique(%<RELEASE_LABELS_ID>%)"},
	{pfc::guid_null,"DISCOGS_FORMAT", false, false, false, false, false, "$zip2($multi_if($multi_greater(%<RELEASE_FORMATS_QUANTITY>%,1),$zip(%<RELEASE_FORMATS_QUANTITY>%,' x '),),%<RELEASE_FORMATS_NAME>%,$multi_if($put(D,$join(%<<RELEASE_FORMATS_DESCRIPTIONS>>%)),', ',),$get(D),$multi_if($put(T,%<RELEASE_FORMATS_TEXT>%),', ',),$multi_if2($get(T),))"},
	{pfc::guid_null,"DISCOGS_RELEASED", true, false, false, false, false, "%RELEASE_DATE_RAW%"},
	{pfc::guid_null,"DISCOGS_MASTER_RELEASE_YEAR", true, false, false, false, false, "%MASTER_RELEASE_YEAR%"},
	{pfc::guid_null,"DISCOGS_RELEASE_YEAR", true, false, false, false, false, "%RELEASE_YEAR%"/*"$if2(%MASTER_RELEASE_YEAR%,%RELEASE_YEAR%)"*/},
	{pfc::guid_null,"DISCOGS_RELEASE_MONTH", true, false, false, false, false, "%RELEASE_MONTH%"},
	{pfc::guid_null,"DISCOGS_RELEASE_DAY", false, false, false, false, false, "%RELEASE_DAY%"},
	{pfc::guid_null,"DISCOGS_SERIES", false, false, false, false, false, "%<RELEASE_SERIES_NAME>%"},
	{pfc::guid_null,"DISCOGS_SERIES_NUMBER", false, false, false, false, false, "%<RELEASE_SERIES_NUMBER>%"},
	{pfc::guid_null,"DISCOGS_RATING", true, true, false, false, false, "%RELEASE_DISCOGS_AVG_RATING%"},
	{pfc::guid_null,"DISCOGS_VOTES", true, true, false, false, false, "%RELEASE_DISCOGS_RATING_VOTES%"},
	{pfc::guid_null,"DISCOGS_SUBMITTED_BY", false, false, false, false, false, "%RELEASE_DISCOGS_SUBMITTED_BY%"},
	{pfc::guid_null,"DISCOGS_DATE_ADDED", false, false, false, false, false, "%RELEASE_DISCOGS_DATE_ADDED%"},
	{pfc::guid_null,"DISCOGS_DATE_CHANGED", false, false, false, false, false, "%RELEASE_DISCOGS_DATE_CHANGED%"},
	{pfc::guid_null,"DISCOGS_MEMBERS_HAVE", false, false, false, false, false, "%RELEASE_DISCOGS_USERS_WANT%"},
	{pfc::guid_null,"DISCOGS_MEMBERS_WANT", false, false, false, false, false, "%RELEASE_DISCOGS_USERS_HAVE%"},
	{pfc::guid_null,"DISCOGS_ARTISTS_ALIASES", false, false, false, false, false, "%<<ARTISTS_ALIASES>>%"},
	{pfc::guid_null,"DISCOGS_ARTISTS_INGROUPS", false, false, false, false, false, "%<<ARTISTS_GROUPS>>%"},
	{pfc::guid_null,"DISCOGS_ARTISTS_ALL_NAME_VARIATIONS", false, false, false, false, false, "%<<ARTISTS_ALL_NAME_VARIATIONS>>%"},
	{pfc::guid_null,"DISCOGS_ARTISTS_MEMBERS", false, false, false, false, false, "%<<ARTISTS_MEMBERS>>%"},
	{pfc::guid_null,"DISCOGS_ARTISTS_URLS", false, false, false, false, false, "%<<ARTISTS_URLS>>%"},
	{pfc::guid_null,"DISCOGS_ARTIST_REALNAME", false, false, false, false, false, "%<ARTISTS_REAL_NAME>%"},
	{pfc::guid_null,"DISCOGS_ARTIST_PROFILE", false, false, false, false, false, "%<ARTISTS_PROFILE>%"},
	{pfc::guid_null,"DISCOGS_RELEASE_NOTES", false, false, false, false, false, "%RELEASE_NOTES%"},
	{pfc::guid_null,"DISCOGS_RELEASE_CREDITS", false, false, false, false, false, "$zip($join(%<<RELEASE_CREDITS_ROLES>>%),' - ',$join(%<<RELEASE_CREDITS_ARTISTS_NAME>>%))"},
	{pfc::guid_null,"DISCOGS_TRACK_CREDITS", false, false, false, false, false, "$zip($join(%<<TRACK_CREDITS_ROLES>>%),' - ',$join(%<<TRACK_CREDITS_ARTISTS_NAME>>%))"},
	{pfc::guid_null,"DISCOGS_CREDIT_FEATURING", false, false, false, false, false, "$flatten($multi_if($any($multi_strcmp($sextend(%<<TRACK_CREDITS_SHORT_ROLES>>%,%<<RELEASE_CREDITS_SHORT_ROLES>>%),'Featuring')),$multi_if($put(aj,$sextend(%<<TRACK_CREDITS_ARTISTS_JOIN>>%,%<<RELEASE_CREDITS_ARTISTS_JOIN>>%)),$joinnames($put(an,$sextend(%<<TRACK_CREDITS_ARTISTS_NAME>>%,%<<RELEASE_CREDITS_ARTISTS_NAME>>%)),$get(aj)),$get(an)),))"},
	{pfc::guid_null,"DISCOGS_CREDIT_VOCALS", false, false, false, false, false, "$flatten($multi_if($any($multi_strcmp($sextend(%<<TRACK_CREDITS_SHORT_ROLES>>%,%<<RELEASE_CREDITS_SHORT_ROLES>>%),'Vocals')),$multi_if($put(aj,$sextend(%<<TRACK_CREDITS_ARTISTS_JOIN>>%,%<<RELEASE_CREDITS_ARTISTS_JOIN>>%)),$joinnames($put(an,$sextend(%<<TRACK_CREDITS_ARTISTS_NAME>>%,%<<RELEASE_CREDITS_ARTISTS_NAME>>%)),$get(aj)),$get(an)),))"},
	{pfc::guid_null,"DISCOGS_CREDIT_REMIXED_BY", false, false, false, false, false, "$flatten($multi_if($any($multi_strcmp($sextend(%<<TRACK_CREDITS_SHORT_ROLES>>%,%<<RELEASE_CREDITS_SHORT_ROLES>>%),'Remix')),$multi_if($put(aj,$sextend(%<<TRACK_CREDITS_ARTISTS_JOIN>>%,%<<RELEASE_CREDITS_ARTISTS_JOIN>>%)),$joinnames($put(an,$sextend(%<<TRACK_CREDITS_ARTISTS_NAME>>%,%<<RELEASE_CREDITS_ARTISTS_NAME>>%)),$get(aj)),$get(an)),))"},
	{pfc::guid_null,"DISCOGS_ARTISTS", false, false, false, false, false, "%<ARTISTS_NAME>%"},
	{pfc::guid_null,"DISCOGS_ALBUM_ARTISTS", false, false, false, false, false, "%<RELEASE_ARTISTS_NAME>%"},
	{pfc::guid_null,"DISCOGS_ARTIST_NAME_VARIATIONS", false, false, false, false, false, "%<ARTISTS_NAME_VARIATION>%"},
	{pfc::guid_null,"DISCOGS_TRACK_HEADING", false, false, false, false, false, "%TRACK_HEADING%"},
	{pfc::guid_null,"DISCOGS_INDEX_TRACK_TITLE", false, false, false, false, false, "%TRACK_INDEXTRACK_TITLE%"},
	{pfc::guid_null,"DISCOGS_SUB_TRACK_TITLE", false, false, false, false, false, "%TRACK_SUBTRACK_TITLE%"},
	{pfc::guid_null,"DISCOGS_TRACK_DURATION", false, false, false, false, false, "%TRACK_DISCOGS_DURATION_RAW%"},
	{pfc::guid_null,"VINYLTRACK", false, false, false, false, false, "$multi_if($multi_or($multi_strstr($put(fmt,%<RELEASE_FORMATS_NAME>%),'Vinyl'),$multi_strstr($get(fmt),'Acetate')),%TRACK_DISCOGS_TRACK_NUMBER%,)"},
};

static tag_mapping_entry ID3_FrameDefs_MS[] =
{
	{TAG_GUID_RELEASE_ID,TAG_RELEASE_ID, true,  false, false, false, true,  "%RELEASE_ID%"},
	{TAG_GUID_MASTER_RELEASE_ID,TAG_MASTER_RELEASE_ID, false, false, false, false, true,  "%MASTER_RELEASE_ID%"},
	{TAG_GUID_ARTIST_ID,TAG_ARTIST_ID, false, false, false, false, true,  "%<ARTISTS_ID>%"},
	{TAG_GUID_LABEL_ID,TAG_LABEL_ID, false, false, false, false, true,  "$unique(%<RELEASE_LABELS_ID>%)"},
	{pfc::guid_null, "Title"                , true,  false, false, false, false, "%TRACK_TITLE%"},
	{pfc::guid_null, "Subtitle"             , true,  false, false, false, false, "%TRACK_SUBTRACK_TITLE%" },
	{pfc::guid_null, "RAting"               , true,  false, false, false, false, "%RELEASE_DISCOGS_AVG_RATING%"},
	{pfc::guid_null, "Comment"              , true,  false, false, false, false, "" },
	{pfc::guid_null, "Artist"               , true,  false, false, false, false, "$joinnames(%<ARTISTS_NAME>%,%<ARTISTS_JOIN>%)"},
	{pfc::guid_null, "Album artist"         , true,  false, false, false, false, "$joinnames(%<RELEASE_ARTISTS_NAME>%,%<RELEASE_ARTISTS_JOIN>%)"},
	{pfc::guid_null, "Album"                , true,  false, false, false, false, "%RELEASE_TITLE%"},
	{pfc::guid_null, "Date"                 , true,  false, false, false, false, "%RELEASE_YEAR%"},
	{pfc::guid_null, "Tracknumber"          , true,  false, false, false, false, "$ifequal($put(trks,$num(%TRACK_DISC_TRACK_NUMBER%,1)),0,,$get(trks))$ifequal($get(trks),0,,$ifequal(%RELEASE_TOTAL_TRACKS%,0,,/$num(%RELEASE_TOTAL_TRACKS%,1)))"},
	{pfc::guid_null, "Genre"                , true,  false, false, false, false, "%<RELEASE_GENRES>%"},
	{pfc::guid_null, "Publisher"            , true,  false, false, false, false, "$unique(%<RELEASE_LABELS_NAME>%)"},
	{pfc::guid_null, "Encoded by"           , true,  false, false, false, false, "$flatten($multi_if($any($multi_strcmp($sextend(%<<TRACK_CREDITS_SHORT_ROLES>>%,%<<RELEASE_CREDITS_SHORT_ROLES>>%),'Encoding By')),$multi_if($put(aj,$sextend(%<<TRACK_CREDITS_ARTISTS_JOIN>>%,%<<RELEASE_CREDITS_ARTISTS_JOIN>>%)),$joinnames($put(an,$sextend(%<<TRACK_CREDITS_ARTISTS_NAME>>%,%<<RELEASE_CREDITS_ARTISTS_NAME>>%)),$get(aj)),$get(an)),))"},
	{pfc::guid_null, "Artist webpage URL"   , true,  false, false, false, false, "https://www.discogs.com/artist/$element(%<ARTISTS_ID>%,0)"},
	{pfc::guid_null, "Copyright"            , true,  false, false, false, false, "$zip(%<RELEASE_COMPANIES_ENTITY_TYPE_NAME>%, - ,%<RELEASE_COMPANIES_NAME>%)"},
	{pfc::guid_null, "Composer"             , true,  false, false, false, false, "$flatten($multi_if($any($multi_strcmp($sextend(%<<TRACK_CREDITS_SHORT_ROLES>>%,%<<RELEASE_CREDITS_SHORT_ROLES>>%),'Written-By')),$multi_if($put(aj,$sextend(%<<TRACK_CREDITS_ARTISTS_JOIN>>%,%<<RELEASE_CREDITS_ARTISTS_JOIN>>%)),$joinnames($put(an,$sextend(%<<TRACK_CREDITS_ARTISTS_NAME>>%,%<<RELEASE_CREDITS_ARTISTS_NAME>>%)),$get(aj)),$get(an)),))"},  //Writing & Arrangement* "Composed By"* "Written-By"
	{pfc::guid_null, "Conductor"            , true,  false, false, false, false, "$flatten($multi_if($any($multi_strcmp($sextend(%<<TRACK_CREDITS_SHORT_ROLES>>%,%<<RELEASE_CREDITS_SHORT_ROLES>>%),'Conductor')),$multi_if($put(aj,$sextend(%<<TRACK_CREDITS_ARTISTS_JOIN>>%,%<<RELEASE_CREDITS_ARTISTS_JOIN>>%)),$joinnames($put(an,$sextend(%<<TRACK_CREDITS_ARTISTS_NAME>>%,%<<RELEASE_CREDITS_ARTISTS_NAME>>%)),$get(aj)),$get(an)),))"}, //Conducting & Leading "Conductor"* "Leader"* "Directed By"
	{pfc::guid_null, "Grouping"             , true,  false, false, false, false, "%<RELEASE_SERIES_NAME>%"},
	{pfc::guid_null, "DiscNumber"           , true,  false, false, false, false, "%DISC_NUMBER%"},
	{pfc::guid_null, "Initial key"          , true,  false, false, false, false, ""},
	{pfc::guid_null, "BPM"                  , true,  false, false, false, false, ""},
	{pfc::guid_null, "iTunesCompilation"    , true,  false, false, false, false, ""},
	{pfc::guid_null, "WWW"                  , true,  false, false, false, false, "https://www.discogs.com/release/%RELEASE_ID%"},
//..
	{pfc::guid_null, "TotalDiscs"           , true,  false, false, false, false, "%RELEASE_TOTAL_DISCS%"},
	{pfc::guid_null, "Lyricist"             , true,  false, false, false, false, "$flatten($multi_if($any($multi_strcmp($sextend(%<<TRACK_CREDITS_SHORT_ROLES>>%,%<<RELEASE_CREDITS_SHORT_ROLES>>%),'Lyrics By')),$multi_if($put(aj,$sextend(%<<TRACK_CREDITS_ARTISTS_JOIN>>%,%<<RELEASE_CREDITS_ARTISTS_JOIN>>%)),$joinnames($put(an,$sextend(%<<TRACK_CREDITS_ARTISTS_NAME>>%,%<<RELEASE_CREDITS_ARTISTS_NAME>>%)),$get(aj)),$get(an)),))"},
	{pfc::guid_null, "Remixed by"           , true,  false, false, false, false, "$flatten($multi_if($any($multi_strcmp($sextend(%<<TRACK_CREDITS_SHORT_ROLES>>%,%<<RELEASE_CREDITS_SHORT_ROLES>>%),'Remix')),$multi_if($put(aj,$sextend(%<<TRACK_CREDITS_ARTISTS_JOIN>>%,%<<RELEASE_CREDITS_ARTISTS_JOIN>>%)),$joinnames($put(an,$sextend(%<<TRACK_CREDITS_ARTISTS_NAME>>%,%<<RELEASE_CREDITS_ARTISTS_NAME>>%)),$get(aj)),$get(an)),))"},
	{pfc::guid_null, "Media type"           , true,  false, false, false, false, "$zip2($multi_if($multi_greater(%<RELEASE_FORMATS_QUANTITY>%,1),$zip(%<RELEASE_FORMATS_QUANTITY>%,' x '),),%<RELEASE_FORMATS_NAME>%,$multi_if($put(D,$join(%<<RELEASE_FORMATS_DESCRIPTIONS>>%)),', ',),$get(D),$multi_if($put(T,%<RELEASE_FORMATS_TEXT>%),', ',),$multi_if2($get(T),))"},
	{pfc::guid_null, "Original release date", true,  false, false, false, false, "$if2(%MASTER_RELEASE_YEAR%,%RELEASE_YEAR%)"},
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
	GUID guid;
};

static ID3_FrameDef ID3_FrameDefs[] =
{
	//cpx (Complex)
	//cpx fb2kna	  w	      ms      id    na      na2   field
	{false, true,  true,  false, "DISCOGS_RELEASE_ID", false, false, "%RELEASE_ID%", pfc::guid_null},
	{false, true,  true,  false, "DISCOGS_MASTER_RELEASE_ID", false, false, "%MASTER_RELEASE_ID%", pfc::guid_null},
	{false, true,  true,  false, "DISCOGS_ARTIST_ID", false, false, "%<ARTISTS_ID>%", pfc::guid_null},
	{false, true,  true,  false, "DISCOGS_LABEL_ID", false, false, "$unique(%<RELEASE_LABELS_ID>%)", pfc::guid_null},

	{true,  false, false, false, "APIC", true,  false, "Attached picture", pfc::guid_null},
	{true,  false, false, false, "COMR", true,  false, "Commercial"/*"Commercial"*/, pfc::guid_null},
	{true,  false, false, false, "ENCR", true,  false, "Encryption method registration", pfc::guid_null},
	{true,  false, false, false, "EQUA", true,  true,  "Equalization", pfc::guid_null},
	{true,  false, false, false, "ETCO", true,  true,  "Event timing codes", pfc::guid_null},
	{true,  false, false, false, "GEOB", true,  false, "General encapsulated object", pfc::guid_null},
	{true,  false, false, false, "GRID", true,  false, "Group identification registration", pfc::guid_null},
	{true,  true,  false, false, "LINK", false, false, "Linked information", pfc::guid_null},
	{true,  true,  false, false, "MCDI", false, false, "Music CD identifier", pfc::guid_null},
	{true,  true,  false, false, "MLLT", false, true,  "MPEG location lookup table", pfc::guid_null},
	{true,  true,  false, false, "OWNE", false, false, "Ownership frame", pfc::guid_null},
	{true,  true,  false, false, "PRIV", false, false, "Private frame", pfc::guid_null},
	{false, true,  false, false, "PCNT", false, false, "Play counter", pfc::guid_null},
	{true,  true,  false, false, "POSS", false, true,  "Position synchronisation frame", pfc::guid_null},
	{true,  true,  false, false, "RBUF", false, false, "Recommended buffer size", pfc::guid_null},
	{true,  true,  false, false, "RVAD", false, true,  "Relative volume adjustment", pfc::guid_null},
	{true,  true,  false, false, "RVRB", false, false, "Reverb", pfc::guid_null},
	{true,  true,  false, false, "SYLT", false, false, "Synchronized lyric/text", pfc::guid_null},
	{true,  true,  false, false, "SYTC", false, true,  "Synchronized tempo codes", pfc::guid_null},
	{false, true,  false, false, "TDLY", false, false, "Playlist delay", pfc::guid_null},
	{false, false, false, false, "TFLT", true,  false, "File type", pfc::guid_null},
	{false, false, false, false, "TIME", false, false, "Time", pfc::guid_null},
	{true,  true,  false, false, "TSIZ", false, false, "Size", pfc::guid_null},
	{true,  true,  false, false, "TLEN", false, false, "Length", pfc::guid_null},

	//titles
	{false, false, true,  true,  "TALB", false, true,  "Album"/*"Album/Movie/Show title"*/, pfc::guid_null},
	{false, false, true,  true,  "TIT1", true,  false, "Grouping"/*"Content group description"*/, pfc::guid_null},
	{false, false, true,  true,  "TKEY", false, true,  "Initial key", pfc::guid_null},
	{false, true,  true,  false, "MVNM", false, true,  "Movement name", pfc::guid_null},
	{false, false, true,  false, "TOAL", false, true,  "Original album"/*"Original album/movie/show title"*/, pfc::guid_null},
	{false, true,  false, false, "TOFN", false, false, "Original filename", pfc::guid_null},
	{false, true,  true,  false, "TSST", false, true,  "Set subtitle", pfc::guid_null},
	{false, false, true,  true,  "TIT3", false, true,  "Subtitle"/*"Subtitle/Description refinement"*/, pfc::guid_null},
	{false, false, true,  true,  "TIT2", false, true,  "Title"/*"Title/songname/content description"*/, pfc::guid_null},
	{false, true,  true,  false, "TSOT", false, true,  "TitleSortOrder", pfc::guid_null},

	//people
	{false, false, true,  true,  "TPE1", false, true,  "Artist" /*"Lead performer(s)/Soloist(s)"*/, pfc::guid_null},
	{false, false, true,  true,  "TPE2", true,  true,  "Album artist" /*"Band/orchestra/accompaniment"*/, pfc::guid_null},
	{false, false, true,  true,  "TCOM", false, true,  "Composer", pfc::guid_null},
	{false, false, true,  true,  "TPE3", false, true,  "Conductor"/*"Conductor/performer refinement"*/, pfc::guid_null},
	{false, true,  false, false, "IPLS", false, false, "Involved people list", pfc::guid_null},
	{false, false, true,  false, "TEXT", false, true,  "Lyricist"/*"Lyricist/Text writer"*/, pfc::guid_null},
	{false, false, true,  true,  "TPUB", false, true,  "Publisher", pfc::guid_null},
	{false, false, true,  false, "TOPE", false, true,  "Original artist"/*"Original artist(s)/performer(s)"*/, pfc::guid_null},
	{false, true,  false, false, "TOLY", false, false, "Original lyricist"/*"Original lyricist(s)/text writer(s)"*/, pfc::guid_null},
	{false, false, true,  false, "TOWN", true,  true,  "Owner"/*"File owner/licensee"*/, pfc::guid_null},
	{false, false, true,  false, "TRSN", true,  true,  "Radio station"/*"Internet radio station name"*/, pfc::guid_null},
	{false, false, true,  false, "TRSO", true,  true,  "Radio station owner"/*"Internet radio station owner"*/, pfc::guid_null},
	{false, false, true,  false, "TPE4", true,  true,  "Remixed by"/*"Interpreted, remixed, or otherwise modified by"*/, pfc::guid_null},
	{false, false, true,  false, "TSOC", false, false, "ComposerSortOrder", pfc::guid_null},
	{false, false, true,  false, "TSOP", false, false, "ArtistSortOrder", pfc::guid_null},
	{false, false, true,  false, "TSOA", false, false, "AlbumSortOrder", pfc::guid_null},
	{false, false, true,  false, "TSO2", false, false, "AlbumArtistSortOrder", pfc::guid_null},

	//counts
	{false, false, true,  true, "TBPM", false, true,  "BPM"/*"BPM (beats per minute)"*/, pfc::guid_null},
	{false, false, true,  true, "TPOS", false, true,  "DiscNumber", pfc::guid_null},
	{false, true,  true,  false,"MVIN", false, true,  "Movement count", pfc::guid_null},
	{false, true,  true,  false,"MVIN", false, true,  "Movement index", pfc::guid_null},
	{true,  false, true,  true, "POPM", false, false, "Rating"/*"Popularimeter"*/, pfc::guid_null},
	{false, false, true,  true, "TRCK", false, true,  "Tracknumber"/*"Track number/Position in set"*/, pfc::guid_null},
	{false, false, true,  false,"TPOS", false, true,  "TotalDiscs", pfc::guid_null},

	//dates
	{false, false, true,  true, "TYER", true,  true,  "Date", pfc::guid_null},
	{false, true,  true,  false,"TDAT", false, false, "Date", pfc::guid_null},
	{false, true,  true,  false,"TORY", false, true,  "Original release date"/*"Original release year"*/, pfc::guid_null},
	{false, false, true,  false,"TRDA", false, true,  "Recording dates", pfc::guid_null},

	//identifier
	{false, true,  false, false,"UFID", false, false, "UFID"/*"Unique file identifier"*/, pfc::guid_null},
	{false, true,  false, false,"TSRC", false, true,  "ISRC" /*"ISRC (international standard recording code)"*/, pfc::guid_null},

	//flags
	{false, false, true,  true, "TCMP", false, true,  "iTunesCompilation", pfc::guid_null},

	//ripping
	{true,  false, false, false,"AENC", true,  false, "Audio encryption", pfc::guid_null},
	{false, false, true,  true, "TENC", false, true,  "Encoded by", pfc::guid_null},
	{false, false, true,  false,"TSSE", false, true,  "Encoding settings"/*"Software/Hardware and settings used for encoding"*/, pfc::guid_null},
	{false, false, true,  false,"TMED", false, true,  "Media type", pfc::guid_null},

	//url
	{false, false, true,  true, "WOAR", false, true,  "Artist webpage URL"/*"Official artist/performer webpage"*/, pfc::guid_null},
	{false, false, true,  false,"WCOM", false, true,  "Commercial information URL"/*"Commercial information"*/, pfc::guid_null},
	{false, false, true,  false,"WCOP", false, true,  "Copyright URL" /*"Copyright/Legal infromation"*/, pfc::guid_null},
	{false, false, true,  false,"WORS", false, true,  "Internet radio webpage URL" /*"Official internet radio station homepage"*/, pfc::guid_null},
	{false, false, true,  false,"WOAF", true,  true,  "File webpage URL"/*"Official audio file webpage"*/, pfc::guid_null},
	{false, false, true,  false,"WPAY", false, true,  "Payment URL"/*"Payment"*/, pfc::guid_null},
	{false, false, true,  false,"WPUB", false, true,  "Publisher URL"/*"Official publisher webpage"*/, pfc::guid_null},
	{false, false, true,  false,"WOAS", false, true,  "Source webpage URL"/*"Official audio source webpage"*/, pfc::guid_null },
	{false, false, true,  false,"WXXX", true,  true,  "WWW"/*"User defined URL link"*/, pfc::guid_null },

	//style
	{false, false, true,  true, "TCON", true,  true,  "Genre", pfc::guid_null },

	//misc
	{false, false, true,  true, "COMM", false, true,  "Comment"/*"Comments"*/, pfc::guid_null },
	{false, false, true,  true, "TCOP", true,  true,  "Copyright", pfc::guid_null },
	{false, false, true,  false,"TLAN", false, true,  "Language"/*"Language(s)"*/, pfc::guid_null },
	{true,  true,  false, false,"USER", false, false, "Terms of use", pfc::guid_null },
	{false, false, false, false,"USLT", false, true,  "Unsynced lyric"/*"Unsynchronized lyric/text transcription"*/, pfc::guid_null },

	//
	{false, false, false, false,"TXXX", false, false, "User defined text information", pfc::guid_null },
};

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

void copy_tag_mappings(tag_mapping_list_type* out_tmt) {

	out_tmt->remove_all();

	for (auto& tag_entry : cfg_tag_mappings) {

		out_tmt->add_item(*tag_entry.clone());
	}
}

void copy_default_tag_mappings(tag_mapping_list_type* out_tmt) {

	out_tmt->remove_all();

	const size_t count = sizeof(default_tag_mappings) / sizeof(tag_mapping_entry);

	for (size_t i = 0; i < count; i++) {

		out_tmt->add_item(*(default_tag_mappings[i].clone()));
		tag_mapping_entry& new_map_entry = out_tmt->get_item(i);
		new_map_entry.is_multival_meta = is_multivalue_meta(default_tag_mappings[i].tag_name);
	}
}

//menuctx (id3 split button) skips kPretags
void copy_id3_default_tag_mappings(tag_mapping_list_type * out_tmt, bool onlyms, bool menuctx) {

	out_tmt->remove_all();

	//init ms map
	std::map<std::string, pfc::string8> map_ms;
	const size_t cDefs = sizeof(ID3_FrameDefs_MS) / sizeof(tag_mapping_entry);
	for (size_t i = 0; i < cDefs; i++) {
		map_ms.insert(std::make_pair(ID3_FrameDefs_MS[i].tag_name.toLower(), ID3_FrameDefs_MS[i].formatting_script));
	}

	//id3 defs

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

			entry.guid_tag = fdef.guid;
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
			out_tmt->add_item(*entry.clone());
		}
	}
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
		//alt
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
