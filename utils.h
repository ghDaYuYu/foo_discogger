#pragma once

#include "../../pfc/pfc.h"
#include <map>

extern "C" {
	#include "zlib.h"
}

#define STR_EQUAL(a,b)		::strcmp(a,b)==0
#define STR_EQUALN(a,b,n)	::strncmp(a,b,n)==0

typedef int(_cdecl *dll_inflateInit2)(z_stream*, int, const char*, int);
typedef int(_cdecl *dll_inflate)(z_stream*, int);
typedef int(_cdecl *dll_inflateEnd)(z_stream*);

extern bool cfg_preview_dialog_track_map;
extern bool cfg_preview_dialog_comp_track_map_in_v23;
extern bool cfg_find_release_dialog_idtracker;

extern double cfg_find_release_colummn_showid_width;
extern bool cfg_find_release_colummn_showid;

extern dll_inflateInit2 dllinflateInit2;
extern dll_inflate dllinflate;
extern dll_inflateEnd dllinflateEnd;
extern HINSTANCE hGetProcIDDLL;

void load_dlls();
void unload_dlls();

struct cmp_str
{
	bool operator()(char const *a, char const *b) const {
		return std::strcmp(a, b) < 0;
	}
};

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

//compensate gripp offset
//TODO: dpi check
static const struct rzgripp {
	int x;
	int y;
	bool grip;
} mygripp{ 22, 56, true };

const struct mounted_param {
	size_t master_ndx;
	size_t release_ndx;
	bool bmaster;
	bool brelease;

	mounted_param(size_t master_ndx, size_t release_ndx, bool bmaster, bool brelease)
		: master_ndx(master_ndx), release_ndx(release_ndx), bmaster(bmaster), brelease(brelease) {};
	mounted_param()
		: master_ndx(~0), release_ndx(~0), bmaster(false), brelease(false) {};

	bool mounted_param::is_master() {
		return bmaster && !brelease;
	}
	bool mounted_param::is_release() {
		return bmaster && brelease;
	}
	bool mounted_param::is_nmrelease() {
		return !bmaster && brelease;
	}
};

static const pfc::string8 match_failed("FAILED TO MATCH TRACK ORDER");
static const pfc::string8 match_success("MATCHED TRACK ORDER");
static const pfc::string8 match_assumed("ASSUMED TRACK ORDER");
static const pfc::string8 match_manual("...");

typedef pfc::array_t<t_uint8> MemoryBlock;

// Trim whitespace from strings
extern inline pfc::string8 trim(const pfc::string8 &str, const char *ch = whitespace);
extern inline pfc::string8 ltrim(const pfc::string8 &str, const char *ch = whitespace);
extern inline pfc::string8 rtrim(const pfc::string8 &str, const char *ch = whitespace);

// Make strings lowercase
extern pfc::string8 lowercase(pfc::string8 str);

// Join array/vector
extern pfc::string8 join(const pfc::array_t<pfc::string8> &in, const pfc::string8 &join_field);

// Insert into vector if missing
extern void add_first_if_not_exists(pfc::array_t<pfc::string8> &v, pfc::string8 s);

// Tokenize string
extern int tokenize(const pfc::string8 &src, const pfc::string8 &delim, pfc::array_t<pfc::string8> &tokens, bool remove_blanks);
//extern int tokenize(const pfc::string8 &src, const pfc::string8 &delim, pfc::array_t_ex<pfc::string8> &tokens, bool remove_blanks);
extern int tokenize_multi(const pfc::string8 &src, const pfc::array_t<pfc::string8> &delims, pfc::array_t<pfc::string8> &tokens, bool remove_blanks);

// TODO: use the titleformat filter instead?
extern void makeFsCompliant(pfc::string8 &str);

extern pfc::string8 urlEscape(const pfc::string8 &src);

extern inline pfc::string8 substr(const pfc::string8 &s, size_t start, size_t count = pfc::infinite_size);

// Open URL in default browser
extern void display_url(const pfc::string8 &url);

extern void list_replace_text(HWND list, int pos, const char *text);
extern void fill_combo_box(HWND combo_box, const pfc::array_t<pfc::string8> &data);

extern int myUncompress(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen);

template<typename T>
void log_msg(const T &msg) {
	console::print(pfc::string8("foo_discogs: ") << msg);
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

extern void CenterWindow(HWND hwnd, CRect rcCfg, HWND hwndCenter);

extern bool sortByVal(const std::pair<int, int>& a, const std::pair<int, int>& b);

namespace listview_helper {

	extern unsigned fr_insert_column(HWND p_listview, unsigned p_index, const char* p_name, unsigned p_width_dlu, int fmt);
	extern unsigned fr_insert_item(HWND p_listview, unsigned p_index, bool is_release_tracker, const char* p_name, LPARAM p_param);
	extern bool fr_insert_item_subitem(HWND p_listview, unsigned p_index, unsigned p_subitem, const char* p_name, LPARAM p_param);

}