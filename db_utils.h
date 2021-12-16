#pragma once
#include "sqlite3.h"

#include "foo_discogs.h"

static int int_type_sqlite3_exec_callback(void* data, int argc, char** argv, char** azColName) {
	int& pint_results = *static_cast<int*>(data);
	pint_results = atoi(argv[0]);
	return 0; //callback status
}

static pfc::string8 dll_db_name() {
	pfc::string8 str = core_api::get_my_file_name();
	str << ".dll.db";
	return str;
}

// foo_discogger dll.db helper class
// may also be used as a generic db helper class

static pfc::string8 db_dbl_apos(pfc::string8 field_data) { pfc::string8 tmp = field_data; tmp.replace_string("'", "''", 0); return tmp; }
static pfc::string8 db_slh_apos(pfc::string8 field_data) { return PFC_string_formatter() << "\'" << field_data << "\'"; }

class sqldb {

public:
	
	sqldb() {};
	~sqldb() {
		if (m_pDb) {
			//should not be here...
			sqlite3_close(m_pDb);
		}
	};

	sqlite3* db_handle() { return m_pDb; }
	size_t open(pfc::string8 dbname);
	void close();
	int prepare(pfc::string8 query, sqlite3_stmt** m_query, pfc::string8& error_msg);
	bool debug_sql_return(int ret, pfc::string8 op, pfc::string8 msg_subject, pfc::string8 ext_subject, size_t top, pfc::string8& msg);

	size_t gen_def_credit(Release_ptr release, pfc::string8 cat_credit_name, pfc::string8 inno);
	size_t add_release_history(Release_ptr release, pfc::string8 cmd);
	
	bool test_dc_database(pfc::string8 db_path, abort_callback& p_abort, threaded_process_status& p_status);
	
private:

	pfc::string8 m_dbname = pfc::string8();

	sqlite3* m_pDb = nullptr;
	sqlite3_stmt** m_query = nullptr;

	bool m_lib_initialized = false;
	bool m_ok = false;
	int m_ret = -1;

	pfc::string8 m_error_msg;
};