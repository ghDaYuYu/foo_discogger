#include "stdafx.h"

#include "foo_discogs.h"

#include "utils.h"
#include <algorithm>
#include <regex>

const char *whitespace = " \t\r\n";

bool cfg_preview_dialog_track_map = true;
bool cfg_find_release_dialog_idtracker = true;

float cfg_find_release_colummn_showid_width = 50.0f;
bool cfg_find_release_colummn_showid = false;

dll_inflateInit2 dllinflateInit2 = NULL;
dll_inflate dllinflate = NULL;
dll_inflateEnd dllinflateEnd = NULL;

dll_deflateInit_ dlldeflateInit = NULL;
dll_deflate dlldeflate = NULL;
dll_deflateEnd dlldeflateEnd = NULL;

dll_adler32 dlladler32 = NULL;

HINSTANCE hGetProcIDDLL = NULL;

const int IMAGELIST_OFFLINE_CACHE_NDX = 8;
const int IMAGELIST_OFFLINE_DB_NDX = 9;

//todo:
inline pfc::string EscapeWin(pfc::string8 keyWord) {
	pfc::string8 out_keyWord(keyWord);
	//out_keyWord.replace_string("/", "//");
	//out_keyWord.replace_string("'", "''");
	//out_keyWord.replace_string("[", "/[");
	//out_keyWord.replace_string("]", "/]");
	//out_keyWord.replace_string("%", "/%");
	out_keyWord.replace_string("&", "&&");
	//out_keyWord.replace_string("_", "/_");
	//out_keyWord.replace_string("(", "/(");
	//out_keyWord.replace_string(")", "/)");
	return out_keyWord;
}

inline pfc::string8 trim(const pfc::string8 &str, const char *ch) {
	return ltrim(rtrim(str, ch), ch);
}

// TODO: get rid std::string?

inline pfc::string8 ltrim(const pfc::string8 &str, const char *ch) {
	std::string dest((const char*)str);
	dest.erase(0, dest.find_first_not_of(ch));
	return pfc::string8(dest.c_str());
}

inline pfc::string8 rtrim(const pfc::string8 &str, const char *ch) {
	std::string dest((const char*)str);
	dest.erase(dest.find_last_not_of(ch) + 1);
	return pfc::string8(dest.c_str());
}

extern void szcstr(size_t n, pfc::string8& out) {
	out = std::to_string(n).c_str();	
}

bool is_number(const std::string& s)
{
	return !s.empty() && std::find_if(s.begin(),
		s.end(), [](unsigned char c) { return !std::isdigit(c); }) == s.end();
}

size_t encode_mr(const int a, const unsigned long b) {
	//search artists array ndx (last 6)
	size_t enc_a = (size_t)a << 26;
	//marter id (first 26)	
	size_t coded = enc_a | b;
	return coded;
}

size_t encode_mr(const int a, pfc::string8& sb) {
	return encode_mr(a, atoi(sb));
}

std::pair<int,unsigned long> decode_mr(const size_t coded) {
#ifdef _WIN64
	size_t dec_a = (coded >> 26);
	dec_a &= (1ULL << 38/*(64-26)*/) - 1;
	//master id (first 26)
	unsigned long dec_b = coded & ((1 << 26) - 1);
#else
	size_t dec_a = (coded >> 26) & ((1 << (32-26)) - 1);
	unsigned long dec_b = coded & ((1 << 26) - 1);
#endif

	return std::pair((int)dec_a, dec_b);
}

pfc::string8 lowercase(pfc::string8 str) {
	std::string result((const char*)str);
	std::transform(result.begin(), result.end(), result.begin(), ::tolower);
	return pfc::string8(result.c_str());
}

pfc::string8 join(const pfc::array_t<pfc::string8> &in, const pfc::string8 &join_field) {
	pfc::string8 out = "";
	const size_t count = in.get_count();
	for (size_t i = 0; i < count; i++) {
		out << in[i];
		if (i != count - 1) {
			out << join_field;
		}
	}
	return out;
}

int tokenize(const pfc::string8 &src, const pfc::string8 &delim, pfc::array_t<pfc::string8> &tokens, bool remove_blanks) {
	
	tokens.force_reset();
	if (!src.get_length()) return 0;

	size_t pos;
	pfc::string8 tmp = src;
	size_t dlength = delim.get_length();
	while ((pos = tmp.find_first(delim)) != pfc::infinite_size) {
		pfc::string8 token = substr(tmp, 0, pos);
		if (remove_blanks) {
			token = trim(token);
		}
		tokens.append_single(token);
		tmp = substr(tmp, pos + dlength, tmp.length() - 1);
	}
	if (remove_blanks) {
		tmp = trim(tmp);
	}
	tokens.append_single(tmp);
	return (int)tokens.get_size();
}

bool replace_bracketed_commas(pfc::string8& out, pfc::string8 what, pfc::string8 with) {

	//all bracketed commas
	std::regex regex_v;
	std::string str_exp(what);
	//ej. ",(?=((?!\\[).)*?\\])"
	str_exp.append("(?=((?!\\[).)*?\\])");
	//try {
	regex_v = std::regex(str_exp);

	std::string res_no_bracketed_commas(out.c_str());
	std::sregex_iterator begin = std::sregex_iterator(res_no_bracketed_commas.begin(), res_no_bracketed_commas.end(), regex_v);
	std::sregex_iterator end = std::sregex_iterator();
	if (begin != end) {

#ifdef DEBUG
		int occurrences = 0;
		for (std::sregex_iterator i = begin; i != end; i++) {
			occurrences++;
		}
#endif

		try {
			res_no_bracketed_commas = std::regex_replace(res_no_bracketed_commas.c_str(), regex_v, with.c_str());
			out = res_no_bracketed_commas.c_str();
			return true;
		}
		catch (std::regex_error e) {
			//res.set_string(e.what());
			return false;
		}

	}
	return false;
}

int tokenize_non_bracketed(const pfc::string8& src, const pfc::string8& delim, pfc::array_t<pfc::string8>& tokens, bool remove_blanks) {

	tokens.force_reset();
	if (!src.get_length()) return 0;

	pfc::string8 tmp_src(src);

	if (size_t open_bracket_pos = tmp_src.find_first('[') != pfc_infinite) {
		replace_bracketed_commas(tmp_src, ",", "%");
	}

	size_t pos;
	pfc::string8 tmp = tmp_src;
	size_t dlength = delim.get_length();

	while ((pos = tmp.find_first(delim)) != pfc::infinite_size) {
		pfc::string8 token = substr(tmp, 0, pos);
		if (remove_blanks) {
			token = trim(token);
		}

		if (token.find_first('[') != pfc_infinite) {
			replace_bracketed_commas(token, "%", ",");
		}

		tokens.append_single(token);
		tmp = substr(tmp, pos + dlength, tmp.length() - 1);
	}
	if (remove_blanks) {
		tmp = trim(tmp);
	}

	if (tmp.find_first('[') != pfc_infinite) {
		//last or unique token
		replace_bracketed_commas(tmp, "%", ",");
	}
	tokens.append_single(tmp);

	//bring back bracketed commas


	return (int)tokens.get_size();
}

int tokenize_multi(const pfc::string8 &src, const pfc::array_t<pfc::string8> &delims, pfc::array_t<pfc::string8> &tokens, bool remove_blanks) {
	size_t pos, pos2, dlen;
	tokens.force_reset();
	pfc::string8 tmp = src;
	const size_t dcount = delims.get_size();
	while (true) {
		pos = pfc::infinite_size;
		for (size_t i = 0; i < dcount; i++) {
			pos2 = tmp.find_first(delims[i]);
			if (pos2 < pos) {
				pos = pos2;
				dlen = delims[i].get_length();
			}
		}
		if (pos == pfc::infinite_size) {
			break;
		}
		pfc::string8 token = substr(tmp, 0, pos);
		if (remove_blanks) {
			token = trim(token);
		}
		tokens.append_single(token);
		tmp = substr(tmp, pos + dlen, tmp.length() - 1);
	}
	if (remove_blanks) {
		tmp = trim(tmp);
	}
	tokens.append_single(tmp);
	return (int)tokens.get_size();
}

bool tokenize_filter(pfc::string8 filter, pfc::array_t<pfc::string>& out_filter_words_lowercase) {
	pfc::array_t<pfc::string8> filter_words;
	tokenize(filter, " ", filter_words, true);

	for (size_t i = 0; i < filter_words.get_size(); i++) {
		out_filter_words_lowercase.append_single(pfc::string(filter_words[i].get_ptr()).toLower());
	}
	return out_filter_words_lowercase.get_count() > 0;
}

void makeFsCompliant(pfc::string8 &str) {
	for (t_size n = 0; n < str.get_length(); n++)
	{
		auto s = &(str[n]);
		if (*s == '\\' || *s == '/' || *s == ':' || *s == '*' || *s == '?' || *s == '"' || *s == '<' || *s == '>' || *s == '|') {
			str.set_char(n, '_');
		}
	}
}

pfc::string8 urlEscape(const pfc::string8 &src) {
	pfc::string8 dst = "";
	for (size_t i = 0; i < src.get_length(); i++) {
		switch (src[i]) {
			case ' ': dst << "%20"; break;
			case '<': dst << "%3C"; break;
			case '>': dst << "%3E"; break;
			case '#': dst << "%23"; break;
			case '%': dst << "%25"; break;
			case '+': dst << "%2B"; break;
			case '{': dst << "%7B"; break;
			case '}': dst << "%7D"; break;
			case '|': dst << "%7C"; break;
			case '\\': dst << "%5C"; break;
			case '^': dst << "%5E"; break;
			case '~': dst << "%7E"; break;

			case '[': dst << "%5B"; break;
			case ']': dst << "%5D"; break;
			case '`': dst << "%60"; break;
			case ';': dst << "%3B"; break;
			case '/': dst << "%2F"; break;
			case '?': dst << "%3F"; break;
			case ':': dst << "%3A"; break;
			case '@': dst << "%40"; break;
			case '=': dst << "%3D"; break;
			case '&': dst << "%26"; break;
			case '$': dst << "%24"; break;
			
			case '*': dst << "%2A"; break;
			case '(': dst << "%28"; break;
			case ')': dst << "%29"; break;
			case ',': dst << "%2C"; break;
			case '\'': dst << "%27"; break;
			case '"': dst << "%22"; break;
			case '!': dst << "%21"; break;

			default:
				if (src[i] < 0) {
					unsigned char c = src[i];
					char buf[16];
					sprintf(buf, "%%%X", (unsigned int)c);
					dst << buf;
				}
				else {
					dst.add_char(src[i]);
					break;
				}
		}
	}
	return dst;
}

void display_url(const pfc::string8 &url) {
	pfc::stringcvt::string_wide_from_ansi wurl(url.get_ptr());

	SHELLEXECUTEINFO sinfo;
	memset(&sinfo, 0, sizeof(sinfo));
	sinfo.cbSize = sizeof(sinfo);
	sinfo.fMask = SEE_MASK_CLASSNAME;
	sinfo.lpVerb = _T("open");
	sinfo.lpFile = wurl;
	sinfo.nShow = SW_SHOWNORMAL;
	sinfo.lpClass = _T("https");

	if (!ShellExecuteEx(&sinfo)) {
		DWORD err = GetLastError();
		if (err != 0) {
			LPSTR messageBuffer = nullptr;
			FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, nullptr);

			foo_discogs_exception ex;
			ex << "Error opening browser page: " << wurl << messageBuffer;

			LocalFree(messageBuffer);

			throw ex;
		}
	}
}

pfc::string8 substr(const pfc::string8 &s, size_t start, size_t count) {
	return pfc::string8(s.get_ptr() + start, count);
}

pfc::string8 extract_max_number(const pfc::string8& s) {
	std::regex regex_v("\\[a\\d+?\\]");

	size_t max = 0;
	std::string str(s);
	std::sregex_iterator begin = std::sregex_iterator(str.begin(), str.end(), regex_v);
	std::sregex_iterator end = std::sregex_iterator();

	for (std::sregex_iterator i = begin; i != end; i++) {
		const std::string_view sv{
			str.data() + i->position(),
			i->str().length()
		};
		if (size_t ival = atoi(sv.data()); ival > max)
			max = ival;
	}
	return max ? std::to_string(max).c_str() : "";
}

pfc::string8 extract_musicbrainz_mib(const pfc::string8& s) {

	std::regex regex_rmb("[0-9a-f]+\\-[0-9a-f]+\\-[0-9a-f]+\\-[0-9a-f]+\\-[0-9a-f]+");

	std::smatch match;
	std::string str(s);
	if (std::regex_search(str, match, regex_rmb)) {
		return pfc::string8(match.str().c_str());
	}
	return "";
}

void load_dlls()
{
	hGetProcIDDLL = LoadLibrary(L"zlib1.dll");
	if (!hGetProcIDDLL) {
		console::print("Error loading zlib1.dll");
		return;
	}
	dllinflateInit2 = (dll_inflateInit2)GetProcAddress(hGetProcIDDLL, "inflateInit2_");
	dllinflate = (dll_inflate)GetProcAddress(hGetProcIDDLL, "inflate");
	dllinflateEnd = (dll_inflateEnd)GetProcAddress(hGetProcIDDLL, "inflateEnd");
	dlladler32 = (dll_adler32)GetProcAddress(hGetProcIDDLL, "adler32");
}


void unload_dlls()
{
	if (hGetProcIDDLL) {
		FreeLibrary(hGetProcIDDLL);
	}
	hGetProcIDDLL = NULL;
}


// replacement for zlib uncompress() that deals with 
int myUncompress(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen) {
	z_stream stream;
	int err;

	stream.next_in = (Bytef*)source;
	stream.avail_in = (uInt)sourceLen;
	// Check for source > 64K on 16-bit machine:
	if ((uLong)stream.avail_in != sourceLen) {
		return Z_BUF_ERROR;
	}

	stream.next_out = dest;
	stream.avail_out = (uInt)*destLen;
	if ((uLong)stream.avail_out != *destLen) {
		return Z_BUF_ERROR;
	}

	stream.zalloc = (alloc_func)nullptr;
	stream.zfree = (free_func)nullptr;

	err = (*dllinflateInit2)(&stream, 15 + 32, ZLIB_VERSION, (int)sizeof(z_stream));
	if (err != Z_OK) {
		return err;
	}

	err = (*dllinflate)(&stream, Z_FINISH);
	if (err != Z_STREAM_END) {
		(*dllinflateEnd)(&stream);
		if (err == Z_NEED_DICT || (err == Z_BUF_ERROR && stream.avail_in == 0)) {
			return Z_DATA_ERROR;
		}
		return err;
	}
	*destLen = stream.total_out;

	err = (*dllinflateEnd)(&stream);
	return err;
}

void ensure_directory_exists(const char* dir) {
	abort_callback_impl _abort;

	pfc::string8 path = pfc::string8(dir);
	int pos;
	pfc::array_t<pfc::string8> missing;

	try {
		while (true) {
			//log_msg("testing directory: %s", path.c_str());
			if (!filesystem::g_exists(path.get_ptr(), _abort)) {
				missing.append_single(path);
			}
			else {
				//log_msg("directory exists: %s", path.c_str());

				for (int i = missing.get_size(); i > 0; i--) {
					pfc::string8 make = missing[i - 1];
					pfc::string8 msg("creating directory: %s");
					msg << make;
					log_msg(msg);

					filesystem::g_create_directory(make.get_ptr(), _abort);

					if (!filesystem::g_exists(make.get_ptr(), _abort)) {
						foo_discogs_exception ex;
						ex << "Error creating directory: \"" << make.get_ptr() << "\"";
						throw ex;
						//log_msg("create directory failed!? %s", make.c_str());
					}
				}
				return;
			}

			if (path[path.length() - 1] == '\\') {
				path = substr(path, 0, path.length() - 2);
			}
			if ((pos = path.find_last("\\")) == pfc::infinite_size) {
				break;
			}
			else {
				path = substr(path, 0, pos + 1);
			}
		}
	}
	catch (foobar2000_io::exception_io &e) {
		foo_discogs_exception ex;
		ex << "Error creating directory: \"" << dir << "\" [" << e.what() << "]";
		throw ex;
	}
}

bool check_os_win_eleven() {
	OSVERSIONINFO ver = { sizeof(ver) };
	WIN32_OP_D(GetVersionEx(&ver));
	if (ver.dwMajorVersion == 10 && ver.dwBuildNumber >= 22000)
		return false;
	else
		return true;
}

bool check_os_wine() {

#ifdef OS_IS_WINE
	return TRUE;
#endif

	//static const char* (CDECL * pwine_get_version)(void);
	HMODULE hntdll = GetModuleHandle(L"ntdll.dll");
	if (!hntdll)
	{
		puts("Not running on NT.");
		return false;
	}

	auto pwine_get_version = (void*)GetProcAddress(hntdll, "wine_get_version");
	if (pwine_get_version)
	{
		//log_msg(PFC_string_formatter() << "Running on Wine... " << pwine_get_version());
		log_msg("Wine detected.");
		return true;
	}
	else
	{
		return false;
	}
}

void CenterWindow(HWND hwnd, CRect rcCfg, HWND hwndCenter, LPARAM lparamLeftTop)
{
	// Determine owner window to center against.
	DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);
	if (hwndCenter == NULL)
	{
		if (dwStyle & WS_CHILD)
			hwndCenter = GetParent(hwnd);
		else
			hwndCenter = GetWindow(hwnd, GW_OWNER);
	}

	// Get coordinates of the window relative to its parent.
	RECT rcDlg;
	if (rcCfg != CRect()) {
		rcDlg = rcCfg;
	}
	else
		GetWindowRect(hwnd, &rcDlg);

	RECT rcArea;
	RECT rcCenter;
	HWND hwndParent;
	if ((dwStyle & WS_CHILD) == 0)
	{
		// Don't center against invisible or minimized windows.
		if (hwndCenter != NULL)
		{
			DWORD dwStyleCenter = GetWindowLong(hwndCenter, GWL_STYLE);
			if (!(dwStyleCenter & WS_VISIBLE) || (dwStyleCenter & WS_MINIMIZE))
				hwndCenter = NULL;
		}

		// Center within screen coordinates.
		SystemParametersInfo(SPI_GETWORKAREA, NULL, &rcArea, NULL);
		if (hwndCenter == NULL)
			rcCenter = rcArea;
		else
			GetWindowRect(hwndCenter, &rcCenter);
	}
	else
	{
		// Center within parent client coordinates.
		hwndParent = GetParent(hwnd);
		GetClientRect(hwndParent, &rcArea);
		GetClientRect(hwndCenter, &rcCenter);
		MapWindowPoints(hwndCenter, hwndParent, (POINT*)&rcCenter, 2);
	}

	int nDlgWidth = rcDlg.right - rcDlg.left;
	int nDlgHeight = rcDlg.bottom - rcDlg.top;

	POINT point;
	point.x = GET_X_LPARAM(lparamLeftTop);
	point.y = GET_Y_LPARAM(lparamLeftTop);

	// Find dialog's upper left based on rcCenter.
	int xLeft = point.x;
	int yTop = point.y;

	if (xLeft == 0 && yTop == 0) {
		xLeft = (rcCenter.left + rcCenter.right) / 2 - nDlgWidth / 2;
		yTop = (rcCenter.top + rcCenter.bottom) / 2 - nDlgHeight / 2;

		if (xLeft < rcArea.left)
			xLeft = rcArea.left;
		else if (xLeft + nDlgWidth > rcArea.right)
			xLeft = rcArea.right - nDlgWidth;

		if (yTop < rcArea.top)
			yTop = rcArea.top;
		else if (yTop + nDlgHeight > rcArea.bottom)
			yTop = rcArea.bottom - nDlgHeight;

		int grippX = mygripp.enabled ? mygripp.width : 0;
		int grippY = mygripp.enabled ? mygripp.height : 0;
		SetWindowPos(hwnd, core_api::get_main_window(), xLeft - grippX / 2, yTop - grippY / 2, nDlgWidth + grippX, nDlgHeight + grippY, /*SWP_NOSIZE |*/ SWP_NOZORDER | SWP_NOACTIVATE);
	}
	else {
		SetWindowPos(hwnd, core_api::get_main_window(), xLeft, yTop, nDlgWidth, nDlgHeight, /*SWP_NOSIZE |*/ SWP_NOZORDER | SWP_NOACTIVATE);
	}
	
	// Map screen coordinates to child coordinates.
	
}
void CustomFont(HWND hwndParent, bool enabled, bool enableedits) {
	if (enabled) {
		for (HWND walk = ::GetWindow(hwndParent, GW_CHILD); walk != NULL; ) {
			wchar_t buffer[128] = {};
			::GetClassName(walk, buffer, (int)(std::size(buffer) - 1));
			const wchar_t* cls = buffer;
			HWND next = ::GetWindow(walk, GW_HWNDNEXT);
			if (next && ::IsWindow(next)) {
				if (g_hFont && (((_wcsnicmp(cls, L"libPPUI:", 8) == 0) || (_wcsnicmp(cls, L"SysTreeV", 8) == 0))
					|| (enableedits && ((_wcsnicmp(cls, L"Edit", 4) == 0))))) {
					CWindow* cwnd = &CWindow(walk);
					cwnd->SetFont(g_hFont);
				}
			}
			walk = next;
		}
	}
}

bool sortByVal(const std::pair<int, int>& a, const std::pair<int, int>& b)
{
	if (a.second == b.second)
		return a.first > b.first;
	return a.second < b.second;
}

namespace listview_helper {

	unsigned fr_insert_column(HWND p_listview, unsigned p_index, const char* p_name, unsigned p_width_dlu, int fmt)
	{

		int colcount = ListView_GetColumnCount(p_listview);
		pfc::stringcvt::string_os_from_utf8 os_string_temp(p_name);
		RECT rect = { 0, 0, static_cast<LONG>(p_width_dlu), 0 };
		MapDialogRect(GetParent(p_listview), &rect);
		LVCOLUMNW data = {};
		data.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT | LVCF_SUBITEM;
		data.fmt = fmt;
		data.cx = rect.right;
		data.pszText = const_cast<TCHAR*>(os_string_temp.get_ptr());
		data.iSubItem = colcount/*p_index*/;
		LRESULT ret = uSendMessage(p_listview, LVM_INSERTCOLUMNW, colcount/*p_index*/, (LPARAM)&data);
		if (ret < 0) return ~0;
		else return (unsigned)ret;
	}

	unsigned insert_lvItem_tracer(HWND p_listview, unsigned p_index,
		bool tracer_match, const char* p_name, LPARAM p_param, int icon_offline)
	{
		if (p_index == ~0) p_index = ListView_GetItemCount(p_listview);
		LVITEM item = {};
		pfc::stringcvt::string_os_from_utf8 os_string_temp(p_name);
		item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
		item.iItem = p_index;
		item.lParam = p_param;
		item.iImage = icon_offline;
		item.pszText = const_cast<TCHAR*>(os_string_temp.get_ptr());

		if (tracer_match) {
			item.mask |= LVIF_STATE;
			item.state =  LVIS_GLOW;
			item.stateMask = LVIS_GLOW;
		}

		LRESULT ret = SendMessage(p_listview, LVM_INSERTITEM, 0, (LPARAM)&item);

		if (ret < 0) return ~0;
		else return (unsigned)ret;
	}

	bool fr_insert_item_subitem(HWND p_listview, unsigned p_index, unsigned p_subitem, const char* p_name, LPARAM p_param)
	{
		return set_item_text(p_listview, p_index, p_subitem, p_name);
	}
}