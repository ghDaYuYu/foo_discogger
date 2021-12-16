#pragma once

#include <condition_variable>

#include "db_utils.h"
#include "json_helpers.h"

typedef std::vector < std::pair<std::pair<pfc::string8, pfc::string8>, std::pair<pfc::string8, pfc::string8>>> vtrack_artists;

class foo_db_exception : public foo_discogs_exception
{
public:
	foo_db_exception() : foo_discogs_exception("Database Exception") {}
	foo_db_exception(const char* msg) : foo_discogs_exception(msg) {}
};

class foo_db_cmd_exception : public foo_db_exception
{
protected:
	const int status;
public:
	foo_db_cmd_exception() : status(0) {}
	foo_db_cmd_exception(int s, const char* cmd, const char* msg) : foo_db_exception("Database command error: "), status(s) {
		(*this) << cmd << " (" << s << ": " << msg << ")";
	}
	foo_db_cmd_exception(int s, const char* msg) : foo_db_exception(msg), status(s) {}
	int get_status() const {
		return status;
	}
};

class foo_db_cmd_open_exception : public foo_db_cmd_exception
{
public:
	foo_db_cmd_open_exception() : foo_db_cmd_exception(404, "Couldn�t open database file") {}
};


class db_fetcher_component {

public:

	~db_fetcher_component() {
		//
	}

	// db ctag generation
	bool lookup_credit_tk(sqldb* db, sqlite3_stmt* qry, pfc::string8& err_msg, /*Release_ptr release, size_t gx*/pfc::string8 thisrole, pfc::string8 thisfullrole, size_t& credit_id, pfc::string8& thisrole_spec);
	int add_parsed_credit(sqldb* db, Release_ptr release, size_t gx);
	size_t insert_parsed_credit_detail(sqldb* db, pfc::string8& err_msg, size_t credit_id, size_t inc_parsed_credit_id, size_t i, size_t j, pfc::string8 credit_spec, pfc::string8 thisartists);
	vppair query_release_credit_categories(int gx, pfc::string8 non);

	// db ctag definitions
	vppair load_db_def_credits();

	// dlg
	vppair load_db_def_credits(vppair& vcats, vppair& vsubcats);
	bool update_db_def_credits(vppair& vdefs);

	// history
	int add_history(sqldb* db, Release_ptr release,	pfc::string8 cmd_id, pfc::string8 cmd_text);

#ifdef DB_DC
	// test
	bool test_discogs_db(pfc::string8 db_path, abort_callback& p_abort, threaded_process_status& p_status);
#endif

private:

	//sqlite3* m_pDb;
};


