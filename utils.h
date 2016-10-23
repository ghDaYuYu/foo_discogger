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

extern dll_inflateInit2 inflateInit2;
extern dll_inflate inflate;
extern dll_inflateEnd inflateEnd;
extern HINSTANCE hGetProcIDDLL;


void load_dlls();
void unload_dlls();


struct cmp_str
{
	bool operator()(char const *a, char const *b) {
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
extern pfc::string8 makeFsCompliant(const char *s);

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
