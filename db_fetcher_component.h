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
	foo_db_cmd_open_exception() : foo_db_cmd_exception(404, "Couldn´t open database file") {}
};


class db_fetcher_component {

public:

	~db_fetcher_component() {

		for (auto& thread : m_threads) {
			if (thread.joinable())
				thread.join();
		}	
	}

	// db CTAG GENERATION
	int add_def_credit(sqldb* db, Release_ptr release, size_t gx);
	size_t insert_def_credit_detail(sqldb* db, pfc::string8& err_msg, size_t credit_id, size_t inc_parsed_credit_id, size_t i, size_t j, pfc::string8 credit_spec, pfc::string8 thisartists);
	vppair query_release_credit_categories(int gx, pfc::string8 non);

	// db CTAG DEFINITIONS
	vppair generate_dialog_credit_definitions();

	// dlg
	vppair generate_dialog_credit_categories(vppair& vcats, vppair& vsubcats);
	bool update_dialog_credit_definitions(vppair& vdefs);

	// history
	int add_history(sqldb* db, Release_ptr release,	pfc::string8 cmd_id, pfc::string8 cmd_text);

	// test
	bool test_discogs_db(pfc::string8 db_path, abort_callback& p_abort, threaded_process_status& p_status);

private:

	sqlite3* m_pDb;

	std::vector<std::thread> m_threads;
	std::mutex m_threadsMutex;

	bool m_finished_task{ true };
	std::mutex m_finishedMutex;
	std::condition_variable m_finishedCondVar;

	static void removeThread(db_fetcher_component* db_fetcher, std::thread::id id)
	{

		std::lock_guard<std::mutex> lock(db_fetcher->m_threadsMutex);
		auto iter = std::find_if(db_fetcher->m_threads.begin(), db_fetcher->m_threads.end(), [=](std::thread& t) { return (t.get_id() == id); });
		if (iter != db_fetcher->m_threads.end())
		{
			iter->detach();
			db_fetcher->m_threads.erase(iter);
		}
	}

	//wait consume
	std::function<bool(db_fetcher_component* db_fetcher, int list_index)> stdf_wait_consume =
		[this](db_fetcher_component* db_fetcher, int list_index) -> bool {
			{
				std::unique_lock <std::mutex > lk{ m_finishedMutex };
				m_finishedCondVar.wait(lk, [db_fetcher] {
						return db_fetcher->m_finished_task;
					});
			}
			return true;
	};
};


