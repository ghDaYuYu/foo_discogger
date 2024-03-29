#pragma once
#include <mutex>
#include "sqlite3.h"

#include "foo_discogs.h"
#include "utils_path.h"
#define THREAD_SAFE_SQL
#define BUSY_TIMEOUT_SQL 20000
inline std::mutex open_readwrite_mutex;

const std::string kcmdHistoryWashup{ "cmd_leave_latest" };
const std::string kcmdHistoryDeleteAll{ "cmd_delete_all" };
const std::string kHistoryFilterButton{ "filter button" };
const std::string kHistoryGetArtist{ "get_artist_process_callback" };
const std::string kHistorySearchArtist{ "search_artist_process_callback" };
const std::string kHistoryProccessRelease{ "process_release_callback" };

enum oplog_type {
	artist = 1, release, filter
};

static int int_type_sqlite3_exec_callback(void* data, int argc, char** argv, char** azColName) {
	int& pint_results = *static_cast<int*>(data);
	pint_results = atoi(argv[0]);
	return 0; //callback status
}

static int sizet_type_sqlite3_exec_callback(void* data, int argc, char** argv, char** azColName) {
	size_t& pint_results = *static_cast<size_t*>(data);
	pint_results = atoi(argv[0]);
	return 0; //callback status
}

static pfc::string8 db_dblq_apos(pfc::string8 field_data) { pfc::string8 tmp = field_data; tmp.replace_string("'", "''", 0); return tmp; }
static pfc::string8 db_slhq_apos(pfc::string8 field_data) { return PFC_string_formatter() << "\'" << field_data << "\'"; }

class sqldb {

public:
	
	sqldb() {};
	~sqldb() {
		if (m_pDb) {
			sqlite3_close(m_pDb);
		}
	};

	sqlite3* db_handle() { return m_pDb; }

	size_t open(pfc::string8 dbname, size_t openmode);
	void close();

	int prepare(pfc::string8 query, sqlite3_stmt** m_query, pfc::string8& error_msg);

	bool debug_sql_return(int ret, pfc::string8 op, pfc::string8 msg_subject, pfc::string8 ext_subject, size_t top, pfc::string8& msg);

	//history
	size_t insert_history(oplog_type optype, std::string cmd, rppair& out);	
	bool recharge_history(std::string delete_cmd, size_t top_rows, std::map<oplog_type, vppair*>allout);

	
private:

	pfc::string8 m_dbname;
	sqlite3* m_pDb = nullptr;
	sqlite3_stmt** m_query = nullptr;
	size_t m_openmode = SQLITE_OPEN_READONLY;

	int m_ret = -1;
	pfc::string8 m_error_msg;
};
