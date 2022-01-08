#include "stdafx.h"
#include "db_fetcher_component.h"
#include "tag_mapping_credit_utils.h"
#include "db_utils.h"

size_t sqldb::open(pfc::string8 dbname) {

	pfc::string8 dbpath;
	m_error_msg = "";
	
	if (!stricmp_utf8(dll_db_name(), dbname)) {
		dbpath << (core_api::pathInProfile("configuration\\"));
		dbpath << dbname;
	}
	else {
		dbpath = dbname;
	}
	
	extract_native_path(dbpath, dbpath);

	if (NULL != m_query) {
		m_error_msg << "query not null opening sqldb";
		close();
		return pfc_infinite;
	}

	do
	{
		m_lib_initialized = true;

		if (SQLITE_OK != (m_ret = sqlite3_initialize()))
		{
			m_lib_initialized = false;
			m_ok = false;
			m_error_msg << "Failed to initialize DB library: " << m_ret;
			break;
		}

		// open connection

		if (SQLITE_OK != (m_ret = sqlite3_open_v2(dbpath, &m_pDb, SQLITE_OPEN_READWRITE, NULL)))
		{
			m_ok = false;
			m_error_msg << "Failed to open DB connection: " << m_ret << ". ";
			m_error_msg << sqlite3_errmsg(m_pDb);
			break;
		}

#ifdef DO_TRANSACTIONS
		sqlite3_exec(m_pDb, "BEGIN TRANSACTION;", NULL, NULL, NULL);
#endif

	} while (false);

	if (m_error_msg.get_length()) {
		foo_db_cmd_exception ex(m_ret, "open", m_error_msg);
		throw ex;
	}

	m_ok = (m_pDb != NULL && m_ret == SQLITE_OK);
	return m_ret;
}

void sqldb::close() {

	bool bclosed = false;
	if (nullptr != m_query) m_ret = sqlite3_finalize(*m_query);
	if (nullptr != m_pDb) {
		bclosed = (m_ret |= sqlite3_close(m_pDb));
		if (m_ret == SQLITE_OK) m_pDb = nullptr;
	}
	if (m_lib_initialized) m_ret |= sqlite3_shutdown();

	if (SQLITE_OK != m_ret)
	{
		m_ok = false;
		m_error_msg << "Failed to close DB connection (Err. " << m_ret << "). ";
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
		m_ok = false;
		m_error_msg << "Failed to prepare insert. (Err. " << m_ret << ") " << error_msg;
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
		if (!stricmp_utf8(op, "bind")) {
			if (SQLITE_OK != ret)
			{
				out_msg = ext_subject << ": " << sqlite3_errmsg(m_pDb);
#ifdef _DEBUG               
				log_msg(out_msg);
#endif
				bres = false;
			}
			break;
		}
		if (!stricmp_utf8(op, "step")) {
			out_msg = "";

			if (SQLITE_OK != ret && SQLITE_ROW != ret)
			{
				out_msg << msg_subject << "(" << std::to_string(top).c_str() << ") " << sqlite3_errmsg(m_pDb);
#ifdef _DEBUG               
				log_msg(out_msg);
#endif					
				bres = false;
			}
			break;
		}
		if (!stricmp_utf8(op, "step delete")) {
			out_msg = "";

			if (SQLITE_DONE != ret)
			{
				out_msg << msg_subject << "(" << std::to_string(top).c_str() << ") " << sqlite3_errmsg(m_pDb);
#ifdef _DEBUG               
				log_msg(out_msg);
#endif					
				bres = false;
			}
			break;
		}
		if (!stricmp_utf8(op, "insert")) {
			out_msg = "";

			if (SQLITE_CONSTRAINT == ret) {
				
				//todo: revise dll.db constraints
				//msg << "Error in constraint rule ignored...";
				//sqlite3_finalize(m_query);
				//m_query = NULL;

				break;
			}
			else {
				if (SQLITE_OK != ret && SQLITE_ROW != ret)
				{
					out_msg << msg_subject << "(" << std::to_string(top).c_str() << ") " << sqlite3_errmsg(m_pDb);
#ifdef _DEBUG               
					log_msg(out_msg);
#endif					
					bres = false;
				}				
			}
			break;
		}

		if (!stricmp_utf8(op, "select")) {

			if (SQLITE_OK != ret)
			{
				out_msg = ext_subject << " :" << sqlite3_errmsg(m_pDb);
#ifdef _DEBUG
				log_msg(out_msg);
#endif
				bres = false;
			}
			break;
		}

		if (SQLITE_OK != ret) {
				out_msg = ext_subject << ", " << sqlite3_errmsg(m_pDb);
#ifdef _DEBUG               
				log_msg(out_msg);
#endif
				bres = false;
				break;						
		}

	} while (false);

	return bres;
}

#ifdef DB_DC
bool sqldb::test_dc_database(pfc::string8 db_path, abort_callback& p_abort, threaded_process_status& p_status) {
	db_fetcher_component dbfetcher;
	return dbfetcher.test_discogs_db(db_path, p_abort, p_status);
}
#endif

size_t sqldb::gen_def_credit(Release_ptr release, pfc::string8 ctag_name, pfc::string8 inno) {

	size_t newrec = pfc_infinite;
	credit_tag_nfo ctag;

	ctag.init(ctag_name);
	bool by_credit = ctag.isGXC();

	std::map<pfc::string8, vppair>* catcredits = by_credit ? &release->catcredits_gxc : &release->catcredits_gxp;
	
	db_fetcher_component dbfetcher;
	if (pfc_infinite != (newrec = dbfetcher.add_parsed_credit(this, release, by_credit))) {

		vppair result;
		result = dbfetcher.query_release_credit_categories(by_credit, inno);

		vppair catdetails;
		for (auto walk_credit_cat : result) {
			catdetails.emplace_back(walk_credit_cat);
		}

		//add result set to map 
		catcredits->emplace(ctag_name, catdetails);
	}
	return newrec;
}

size_t sqldb::add_release_history(Release_ptr release, pfc::string8 cmd, rppair& out) {

	size_t newrec = pfc_infinite;
	db_fetcher_component dbfetcher;
	newrec = dbfetcher.add_history(this, nullptr, release, cmd, "", out);

	return newrec;
}

size_t sqldb::add_artist_history(Artist_ptr artist, pfc::string8 cmd, rppair& row) {
	size_t newrec = pfc_infinite;
	db_fetcher_component dbfetcher;
	newrec = dbfetcher.add_history(this, artist, nullptr, cmd, "", row);
	return newrec;
}

size_t sqldb::add_filter_history(pfc::string8 cmd, rppair &out) {
	size_t newrec = pfc_infinite;
	db_fetcher_component dbfetcher;

	newrec = dbfetcher.add_history(this, nullptr, nullptr, cmd, "", out);
	return newrec;
}

size_t sqldb::delete_history(pfc::string8 cmd, pfc::string8 top_rows, std::vector<vppair*>allout) {
	size_t newrec = pfc_infinite;
	db_fetcher_component dbfetcher;

	//cmd: "cmd_leave_latest"  //top_row: 100 (+ prefix to build vec)
	newrec = dbfetcher.delete_history(this, nullptr, nullptr, cmd, top_rows, allout);
	return newrec;
}