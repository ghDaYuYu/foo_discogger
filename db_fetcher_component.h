#pragma once

#include <condition_variable>

#include "db_utils.h"
#include "json_helpers.h"

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
	foo_db_cmd_open_exception() : foo_db_cmd_exception(404, "Couldn't open database file") {}
};

#ifdef CAT_CRED
struct nfo_parsed_credit_detail {
	size_t credit_id;
	size_t inc_parsed_credit_id;
	size_t i;
	size_t j;
	pfc::string8 credit_spec;
	pfc::string8 thisartists;
};
#endif // CAT_CRED

class db_fetcher_component {

public:

	~db_fetcher_component() {
		//
	}

	// db ctag generation
	bool lookup_credit_tk(sqldb* db, sqlite3_stmt* qry, pfc::string8& err_msg, /*Release_ptr release, size_t gx*/pfc::string8 thisrole, pfc::string8 thisfullrole, size_t& credit_id, pfc::string8& thisrole_spec);
	bool lookup_parsed_credit_tk(sqldb* db, sqlite3_stmt* qry, pfc::string8& err_msg, pfc::string8 release_id, pfc::string8 subtype, size_t& parsed_credit_id);
	int add_parsed_credit(sqldb* db, Release_ptr release, bool bycredit);
	int add_parsed_credit_old(sqldb* db, Release_ptr release, bool bycredit);
	unsigned long calc_parsed_credit(sqldb* db, Release_ptr release, bool bycredit, std::vector<nfo_parsed_credit_detail>& v_pcd, size_t& inc_parsed_credit_id, bool& matched);
	size_t insert_parsed_credit_detail(sqldb* db, pfc::string8& err_msg, size_t credit_id, size_t inc_parsed_credit_id, size_t i, size_t j, pfc::string8 credit_spec, pfc::string8 thisartists);
	vppair query_release_credit_categories(pfc::string8 inparsedid, int gx, pfc::string8 innon);

	// db ctag definitions
	vppair load_db_def_credits();

	// dlg
	vppair load_db_def_credits(vppair& vcats, vppair& vsubcats);
	bool update_db_def_credits(vppair& vdefs);
#endif

#ifdef DB_DC
	// test
	bool test_discogs_db(pfc::string8 db_path, abort_callback& p_abort, threaded_process_status& p_status);
#endif

private:

	//sqlite3* m_pDb;
};


