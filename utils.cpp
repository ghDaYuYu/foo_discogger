#include "stdafx.h"

#include "utils.h"
#include <algorithm>

const char *whitespace = " \t\r\n";


dll_inflateInit2 inflateInit2 = NULL;
dll_inflate inflate = NULL;
dll_inflateEnd inflateEnd = NULL;
HINSTANCE hGetProcIDDLL = NULL;


inline pfc::string8 trim(const pfc::string8 &str, const char *ch) {
	return ltrim(rtrim(str, ch), ch);
}

// TODO: get rid of reliance upon std::string

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
	size_t pos;
	tokens.force_reset();
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

/*int tokenize(const pfc::string8 &src, const pfc::string8 &delim, pfc::array_t_ex<pfc::string8> &tokens, bool remove_blanks) {
	size_t pos;
	tokens.force_reset();
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
}*/


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

void add_first_if_not_exists(pfc::array_t<pfc::string8> &v, pfc::string8 s) {
	bool found = false;
	for (size_t i = 0; i < v.get_size() && !found; i++) {
		if (v[i] == s) {
			found = true;
		}
	}
	if (!found) {
		if (v.get_size()) {
			v.increase_size(1);
			for (size_t i = v.get_size() - 1; i >= 0; i++) {
				v[i + 1] = std::move(v[i]);
			}
			v[0] = s;
		}
		else {
			v.append_single(s);
		}
	}
}

void list_replace_text(HWND list, int pos, const char *text) {
	LRESULT item_data = uSendMessage(list, LB_GETITEMDATA, pos, 0);
	uSendMessage(list, LB_DELETESTRING, pos, 0);
	uSendMessageText(list, LB_INSERTSTRING, pos, text);
	uSendMessage(list, LB_SETITEMDATA, pos, (LPARAM)item_data);
}


void load_dlls()
{
	hGetProcIDDLL = LoadLibrary(L"zlib1.dll");
	if (!hGetProcIDDLL) {
		console::print("Error loading zlib1.dll");
		return;
	}
	inflateInit2 = (dll_inflateInit2)GetProcAddress(hGetProcIDDLL, "inflateInit2_");
	inflate = (dll_inflate)GetProcAddress(hGetProcIDDLL, "inflate");
	inflateEnd = (dll_inflateEnd)GetProcAddress(hGetProcIDDLL, "inflateEnd");
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

	err = (*inflateInit2)(&stream, 15 + 32, ZLIB_VERSION, (int)sizeof(z_stream));
	if (err != Z_OK) {
		return err;
	}

	err = (*inflate)(&stream, Z_FINISH);
	if (err != Z_STREAM_END) {
		(*inflateEnd)(&stream);
		if (err == Z_NEED_DICT || (err == Z_BUF_ERROR && stream.avail_in == 0)) {
			return Z_DATA_ERROR;
		}
		return err;
	}
	*destLen = stream.total_out;

	err = (*inflateEnd)(&stream);
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
