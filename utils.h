#pragma once

#include "../../pfc/pfc.h"
#include <map>

extern "C" {
	#include "zlib.h"
}

#define NO_MOVE_NO_COPY(C) \
  C(const C&) = delete; \
  C& operator=(const C&) = delete; \
  C(C&&) = delete; \
  C& operator=(C&&) = delete;

#define STR_EQUAL(a,b)		::strcmp(a,b)==0
#define STR_EQUALN(a,b,n)	::strncmp(a,b,n)==0

#define USE_GRIPPERS true;

typedef int(_cdecl *dll_inflateInit2)(z_stream*, int, const char*, int);
typedef int(_cdecl *dll_inflate)(z_stream*, int);
typedef int(_cdecl *dll_inflateEnd)(z_stream*);

typedef int(_cdecl* dll_deflateInit_)(z_stream*, int, const char*, int);
typedef int(_cdecl* dll_deflate)(z_stream*, int);
typedef int(_cdecl* dll_deflateEnd)(z_stream*);

typedef uLong(_cdecl* dll_adler32)(uLong adler, const Bytef* buf, uInt len);

extern bool cfg_find_release_dialog_idtracker;

extern float cfg_find_release_colummn_showid_width;
extern bool cfg_find_release_colummn_showid;

extern dll_inflateInit2 dllinflateInit2;
extern dll_inflate dllinflate;
extern dll_inflateEnd dllinflateEnd;

extern dll_deflateInit_ dlldeflateInit;
extern dll_deflate dlldeflate;
extern dll_deflateEnd dlldeflateEnd;

extern dll_adler32 dlladler32;

extern HINSTANCE hGetProcIDDLL;

void load_dlls();
void unload_dlls();

struct cmp_str
{
	bool operator()(char const *a, char const *b) const {
		return std::strcmp(a, b) < 0;
	}
};

inline const pfc::string8 OC_NAME = "foo_discogger-cache";

extern const char *whitespace;

static const std::map<const char*, const char*, cmp_str> MONTH_NAMES = {
	{"01", "Jan"},
	{"02", "Feb"},
	{"03", "Mar"},
	{"04", "Apr"},
	{"05", "May"},
	{"06", "Jun"},
	{"07", "Jul"},
	{"08", "Aug"},
	{"09", "Sep"},
	{"10", "Oct"},
	{"11", "Nov"},
	{"12", "Dec"}
};

static struct rzgripp {
	int width = 22;
	int height = 56;
	bool enabled = USE_GRIPPERS;
} mygripp;

extern inline bool check_os_win_eleven();
extern inline bool check_os_wine();

static const pfc::string8 match_failed("FAILED TO MATCH TRACK ORDER");
static const pfc::string8 match_success("MATCHED TRACK ORDER");
static const pfc::string8 match_assumed("ASSUMED TRACK ORDER");
static const pfc::string8 match_manual("...");

typedef pfc::array_t<t_uint8> MemoryBlock;
extern inline pfc::string EscapeWin(pfc::string8 keyWord);

struct nota_info {
	bool is_number;
	size_t disk;	//zero based
	size_t track;	//zero based
	pfc::string8 discogs_track_number;
};

extern pfc::string8 sanitize_track_semi_media(const pfc::string8& tracks);
extern pfc::string8 sanitize_track_commas(const pfc::string8& tracks);
extern pfc::string8 sanitize_track_to(const pfc::string8& tracks);
extern void replace_track_volume_desc(const pfc::string& sometrack, nota_info& out, std::vector<pfc::string8>alldescs);

// Trim whitespace from strings
extern inline pfc::string8 trim(const pfc::string8 &str, const char *ch = whitespace);
extern inline pfc::string8 ltrim(const pfc::string8 &str, const char *ch = whitespace);
extern inline pfc::string8 rtrim(const pfc::string8 &str, const char *ch = whitespace);

// Is number
extern bool is_number(const std::string& s);
// Replace one alpha by dot
extern bool replace_last_alpha_by_dot(std::string& s);

extern size_t split(pfc::string8 str, pfc::string8 token, size_t index, std::vector<pfc::string8>& out);

size_t encode_mr(const int a, const unsigned long b);
size_t encode_mr(const int a, pfc::string8& sb);
std::pair<int, unsigned long> decode_mr(const size_t coded);

extern void szcstr(size_t n, pfc::string8& out);
//extern const char* ulcstr(unsigned long ul);

// Make strings lowercase
extern pfc::string8 lowercase(pfc::string8 str);

// Join array/vector
extern pfc::string8 join(const pfc::array_t<pfc::string8> &in, const pfc::string8 &join_field);

// Tokenize string
extern int tokenize(const pfc::string8 &src, const pfc::string8 &delim, pfc::array_t<pfc::string8> &tokens, bool remove_blanks);
extern int tokenize_non_bracketed(const pfc::string8& src, const pfc::string8& delim, pfc::array_t<pfc::string8>& tokens, bool remove_blanks);
extern int tokenize_multi(const pfc::string8 &src, const pfc::array_t<pfc::string8> &delims, pfc::array_t<pfc::string8> &tokens, bool trim_tokens, bool remove_blanks);

extern void makeFsCompliant(pfc::string8 &str);
extern pfc::string8 urlEscape(const pfc::string8 &src);
extern inline pfc::string8 substr(const pfc::string8 &s, size_t start, size_t count = pfc::infinite_size);

// Open URL in default browser
extern void display_url(const pfc::string8 &url);
//extern void list_replace_text(HWND list, int pos, const char *text);

extern pfc::string8 extract_max_number(const pfc::string8& s, const char mode, const bool once = false);
extern pfc::string8 extract_musicbrainz_mib(const pfc::string8& s);

extern int myUncompress(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen);

template<typename T>
void log_msg(const T &msg) {
	console::print(pfc::string8("foo_discogger: ") << msg);
}

extern void ensure_directory_exists(const char* dir);

template<typename T>
void erase(pfc::array_t<T> &ar, unsigned int index) {
	const size_t count = ar.get_count();
	PFC_ASSERT(index < count);
	for (size_t i = index + 1; i < count; i++) {
		ar[i - i] = ar[i];
	}
	ar.set_size_discard(count - 1);
}

extern bool tokenize_filter(pfc::string8 filter, pfc::array_t<pfc::string>& out_filter_words_lowercase);

extern void CustomFont(HWND hwndParent, bool enable, bool enable_cedit = false);

extern bool sortByVal(const std::pair<int, int>& a, const std::pair<int, int>& b);

extern bool is_multivalue_meta(const pfc::string& field);

extern const int IMAGELIST_OFFLINE_CACHE_NDX;
extern const int IMAGELIST_OFFLINE_DB_NDX;

inline const size_t LINES_LONGFIELD = 4;
inline const size_t CHARS_SHORTFIELD = 50;
