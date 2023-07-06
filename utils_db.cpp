#include "stdafx.h"
#include "db_fetcher_component.h"
#include "utils_db.h"
#include "utils.h"

size_t sqldb::open(pfc::string8 dbname, size_t openmode) {

	pfc::string8 n8_dbPath;
	m_error_msg = "";

	if (!stricmp_utf8(dll_db_filename(), dbname)) {
		n8_dbPath = profile_path("configuration");
		n8_dbPath << "\\" << dbname;
	}
	else {
		n8_dbPath = dbname;
		extract_native_path(n8_dbPath, n8_dbPath);
	}

	if (NULL != m_query) {
		m_error_msg << "sqldb: error opening sqldb, query member should be null";
		close();
		return pfc_infinite;
	}

	do
	{
#ifdef THREAD_SAFE_SQL
		if (SQLITE_OK != (m_ret = sqlite3_config(SQLITE_CONFIG_MULTITHREAD))) {
			//will fail on already initiated sqlite
		}
#endif

		if (SQLITE_OK != (m_ret = sqlite3_initialize()))
		{
			m_error_msg << "sqldb: failed to initialize DB library: " << m_ret;
			break;
		}

		// open connection
		{
			std::lock_guard<std::mutex> guard(open_readwrite_mutex);
			if (SQLITE_OK != (m_ret = sqlite3_open_v2(n8_dbPath, &m_pDb, openmode, NULL)))
			{
				m_error_msg << "sqldb: failed to open DB connection: " << m_ret << ". ";
				m_error_msg << sqlite3_errmsg(m_pDb);
				break;
			}
			if (openmode == SQLITE_OPEN_READWRITE) {
				sqlite3_busy_timeout(m_pDb, BUSY_TIMEOUT_SQL);
			}
		}
	} while (false);

	if (m_error_msg.get_length()) {
		foo_db_cmd_exception ex(m_ret, "open", m_error_msg);
		throw ex;
	}

	return m_ret;
}

void sqldb::close() {

	bool query_finalized_ok = true;

	if (nullptr != m_query) {
		if (SQLITE_OK == (m_ret = sqlite3_finalize(*m_query))) {
			m_query = nullptr;
			query_finalized_ok = (m_ret == SQLITE_OK);
		};
	}

	if (nullptr != m_pDb) {
		m_ret = sqlite3_close(m_pDb);
		if (m_ret == SQLITE_OK) {
			m_pDb = nullptr;
		}
	}

	if (SQLITE_OK != m_ret || !query_finalized_ok)
	{
		m_error_msg << "sqldb: failed to close DB connection (Err. " << m_ret << "). ";
		m_error_msg << (m_pDb != nullptr ? sqlite3_errmsg(m_pDb) : "");
	}

	if (m_error_msg.get_length()) {
		foo_db_cmd_exception ex(m_ret, "close", m_error_msg);
		throw ex;
	}
}

int sqldb::prepare(pfc::string8 query, sqlite3_stmt** stmt_query, pfc::string8& error_msg) {

	if (SQLITE_OK != (m_ret = sqlite3_prepare_v2(m_pDb, query, -1, stmt_query, NULL)))
	{
		m_error_msg << "sqldb: failed to prepare query. (Err. " << m_ret << ") " << error_msg;
		m_error_msg << sqlite3_errmsg(m_pDb);
	}

	if (m_error_msg.get_length()) {
		
		foo_db_cmd_exception ex(m_ret, "prepare", m_error_msg);
		throw ex;
	}

	m_query = stmt_query;
	return m_ret;
}

bool sqldb::debug_sql_return(int ret, pfc::string8 op, pfc::string8 msg_subject, pfc::string8 ext_subject, size_t top, pfc::string8& out_msg) {

	bool bres = true;

	do {

		if (!stricmp_utf8(op, "open")) {
		
			if (SQLITE_OK != ret && SQLITE_DONE != ret)
			{
				out_msg = ext_subject << ": " << sqlite3_errmsg(m_pDb);
				bres = false;
			}
			break;
		}

		if (!stricmp_utf8(op, "bind")) {
			if (SQLITE_OK != ret)
			{
				out_msg = ext_subject << ": " << sqlite3_errmsg(m_pDb);
				bres = false;
			}
			break;
		}
		if (!stricmp_utf8(op, "step")) {
			out_msg = "";

			if (SQLITE_OK != ret && SQLITE_ROW != ret)
			{
				out_msg << msg_subject << "(" << top << ") " << sqlite3_errmsg(m_pDb);
				bres = false;
			}
			break;
		}
		if (!stricmp_utf8(op, "step delete")) {
			out_msg = "";

			if (SQLITE_DONE != ret)
			{
				out_msg << msg_subject << "(" << top << ") " << sqlite3_errmsg(m_pDb);
				bres = false;
			}
			break;
		}
		if (!stricmp_utf8(op, "insert")) {
			out_msg = "";

			if (SQLITE_CONSTRAINT == ret) {
				break;
			}
			else {
				if (SQLITE_OK != ret && SQLITE_ROW != ret)
				{
					out_msg << msg_subject << "(" << top << ") " << sqlite3_errmsg(m_pDb);
					bres = false;
				}
			}
			break;
		}

		if (!stricmp_utf8(op, "select")) {

			if (SQLITE_OK != ret)
			{
				out_msg = ext_subject << " :" << sqlite3_errmsg(m_pDb);
				bres = false;
			}
			break;
		}

		if (SQLITE_OK != ret) {
				out_msg = ext_subject << ", " << sqlite3_errmsg(m_pDb);
				bres = false;
				break;
		}

	} while (false);

	return bres;
}



size_t sqldb::insert_history(oplog_type optype, std::string cmd, rppair& out) {

	if (!CONF.history_enabled()) {
		return pfc_infinite;
	}

	db_fetcher_component dbfetcher;
	return dbfetcher.insert_history(this, optype, cmd, "", out);
}

bool sqldb::recharge_history(std::string delete_cmd, size_t top_rows, std::map<oplog_type, vppair*>allout) {

	db_fetcher_component dbfetcher;
	return dbfetcher.recharge_history(this, delete_cmd, top_rows, allout);
}

