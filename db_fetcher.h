#pragma once

#include "version.h"

#ifdef DB_DC

#include "sqlite3.h"
#include "json_helpers.h"

class db_fetcher {

public:

	db_fetcher() : m_pDb(nullptr), m_ret(0), m_error_msg(""), m_lib_initialized(false)/*, foo_dbpath("")*/ {};
	~db_fetcher() {
		if (m_pDb != nullptr)
			sqlite3_close(m_pDb);
	};

	sqlite3* init();
	void close();

	void fetch_search_artist(pfc::string8 artist_hint, const int db_dc_flags, pfc::string8& json, threaded_process_status& p_status, abort_callback& p_abort);

	pfc::array_t<JSONParser_ptr> versions_get_all_pages(pfc::string8& id, pfc::string8& secid, pfc::string8 params, const char* msg, threaded_process_status& p_status, abort_callback& p_abort);
	pfc::array_t<JSONParser_ptr> releases_get_all_pages(pfc::string8& id, pfc::string8& secid, pfc::string8 params, abort_callback& p_abort, const char* msg, threaded_process_status& p_status);

	void get_artist(pfc::string8& id, pfc::string8& html, abort_callback& p_abort, const char* msg, threaded_process_status& p_status);
	void get_release(pfc::string8& id, pfc::string8& html, pfc::string8 params, abort_callback& p_abort, const char* msg, threaded_process_status& p_status);

private:

	sqlite3* m_pDb = nullptr;
	size_t m_ret = 0;
	pfc::string8 m_error_msg = pfc::string8();
	bool m_lib_initialized = false;
};

#else
class db_fetcher {
	//
};
#endif // DB_DC