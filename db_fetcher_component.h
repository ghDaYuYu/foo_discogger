#pragma once

#include <condition_variable>
#include "foo_discogs.h"
#include "utils_db.h"
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


class db_fetcher_component {

public:

	~db_fetcher_component() {
		//..
	}

	// history
	int insert_history(sqldb* db, oplog_type optype, std::string cmd_log, pfc::string8 cmd_param, rppair& out);
	bool recharge_history(sqldb* db, std::string delete_cmd, size_t cmd_param, std::map<oplog_type, vppair*>allout);


private:
	//..
};