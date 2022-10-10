#include "stdafx.h"

#include "utils_db.h"
#include "db_fetcher_component.h"

int db_fetcher_component::insert_history(sqldb* db, oplog_type optype, std::string cmd_log, pfc::string8 cmd_param, rppair& out) {

	if (!CONF.history_enabled()) return 0;

	pfc::string8 artist_id, artist_name, release_id, release_title, filter;
	
	if (optype == oplog_type::artist || optype == oplog_type::release) {

		release_id = out.first.first;
		release_title = out.first.second;
		artist_id = out.second.first;
		artist_name = out.second.second;
	}
	else {
		//filter saved as cmd param
		filter = cmd_param = out.first.first;
	}

	int ret = -1;
	pfc::string8 cmd, query, err_msg;

	sqlite3* pDb;
	size_t inc_insert = pfc_infinite;
	bool in_transaction = false;

	do {

		ret = db->open(dll_db_filename(), SQLITE_OPEN_READWRITE);

		if (!db->debug_sql_return(ret, "open", "foo_discogger", "", 0, err_msg)) break;

		pDb = db->db_handle();

		if (ret == SQLITE_OK) {

			ret = sqlite3_exec(db->db_handle(), "BEGIN TRANSACTION;", NULL, NULL, NULL);
			in_transaction = true;

			cmd = "insert";

			query = "INSERT INTO history_releases (release_id, title, date, cmd_id, cmd_text, artist_id, artist_name) ";
			query << "VALUES(";
			query << "\'" << release_id << "\', ";
			query << "\'" << db_dbl_apos(release_title) << "\', ";
			query << "DateTime(\'now\') " << ", ";
			query << "\'" << cmd_log.c_str() << "\', ";
			query << "\'" << db_dbl_apos(cmd_param) << "\', ";
			query << "\'" << artist_id << "\' , ";
			query << "\'" << db_dbl_apos(artist_name) << "\'";
			query << ");";

			char* err;
			ret = sqlite3_exec(pDb, query, int_type_sqlite3_exec_callback, &inc_insert, &err);

			//
			if (!db->debug_sql_return(ret, cmd, "add history details", "", 0, err_msg)) break;
			//
		}
	} while (false);

	if (in_transaction) {
		ret = sqlite3_exec(db->db_handle(), "END TRANSACTION;", NULL, NULL, NULL);
	}

	if (err_msg.get_length()) {
		db->close();
		foo_db_cmd_exception e(ret, cmd, err_msg);
		throw e;
	}

	db->close();
	return inc_insert;
}

bool db_fetcher_component::recharge_history(sqldb* db, std::string delete_cmd, size_t cmd_param, std::map<oplog_type, vppair*>allout) {

	bool bres = false;

	int ret = -1;
	pfc::string8 cmd;
	pfc::string8 err_msg;

	if (!delete_cmd.compare(kcmdHistoryDeleteAll)) {

		ret =  db->open(dll_db_filename(), SQLITE_OPEN_READWRITE);
		bool check_ret = db->debug_sql_return(ret, "open", "foo_discogger - on init history update", "", 0, err_msg);
		if (check_ret)	ret = sqlite3_exec(db->db_handle(), "DELETE FROM history_releases;", NULL, NULL, NULL);

		return ret == SQLITE_OK;
	}

	sqlite3_stmt* stmt_lk = 0;
	sqlite3_stmt* stmt_read = 0;

	sqlite3* pDb = nullptr;
	bool in_transaction = false;

	// maps

	std::map<oplog_type, std::string> task_map;
	std::map<oplog_type, std::string> flush_map;
	std::map<oplog_type, std::string> query_map;

	//
	task_map.emplace(oplog_type::artist, kHistorySearchArtist);
	task_map.emplace(oplog_type::release, kHistoryProccessRelease);
	task_map.emplace(oplog_type::filter, kHistoryFilterButton);
	std::string artist_sec_task = kHistoryGetArtist;

	pfc::string8 keep_top_artist =
	"DELETE FROM history_releases WHERE (cmd_id = @cmd_id OR cmd_id = @cmd_id_sec) AND id NOT IN ("
		"SELECT grp_calc.id FROM ( "
			"SELECT id, date, artist_name FROM ("
				"SELECT id, date, artist_id, release_id, cmd_text FROM history_releases "
				"WHERE cmd_id = @cmd_id OR cmd_id = @cmd_id_sec ORDER BY date DESC "
			") filter_date_desc GROUP BY filter_date_desc.artist_id) " //diff is group by artist
		"grp_calc ORDER BY date DESC LIMIT @param_pop_tops "
	")";

	pfc::string8 keep_top_release =
	"DELETE FROM history_releases WHERE cmd_id = @cmd_id AND id NOT IN ("
		"SELECT grp_calc.id FROM ( "
			"SELECT id, date FROM ("
				"SELECT id, date, artist_id, release_id, cmd_text FROM history_releases "
				"WHERE cmd_id = @cmd_id ORDER BY date DESC "
			") filter_date_desc GROUP BY filter_date_desc.release_id) " //diff is group by release
		"grp_calc ORDER BY date DESC LIMIT @param_pop_tops "
	")";

	pfc::string8 keep_top_filter =
	"DELETE FROM history_releases WHERE cmd_id = @cmd_id AND id NOT IN ("
		"SELECT grp_calc.id FROM ( "
			"SELECT id, date, artist_name FROM ("
				"SELECT id, date, artist_id, release_id, cmd_text FROM history_releases "
				"WHERE cmd_id = @cmd_id ORDER BY date DESC "
			") filter_date_desc GROUP BY filter_date_desc.cmd_text) " //diff is group by filter
		"grp_calc ORDER BY date DESC LIMIT @param_pop_tops "
	")";

	flush_map.emplace(oplog_type::release, keep_top_release);
	flush_map.emplace(oplog_type::artist, keep_top_artist);
	flush_map.emplace(oplog_type::filter, keep_top_filter);

	std::string query_artist =
		/*,
		"SELECT artist_id, artist_name, date, cmd_id, cmd_text, artist_id, artist_name "
		"FROM history_releases WHERE cmd_id = @cmd_id "
		"GROUP BY artist_id ORDER BY date DESC LIMIT @param_pop_tops;"*/
		"SELECT artist_id, artist_name, date, cmd_id, cmd_text, artist_id, artist_name "
		"FROM history_releases WHERE cmd_id == ? OR cmd_id == ? "
		"GROUP BY artist_id ORDER BY date DESC LIMIT @param_pop_tops;";

	std::string query_release =
		"SELECT release_id, title, date, cmd_id, cmd_text, artist_id, artist_name "
		"FROM history_releases WHERE cmd_id = @cmd_id "
		"GROUP BY release_id ORDER BY date DESC LIMIT @param_pop_tops;";

	std::string query_filter =
		"SELECT cmd_text, cmd_text, date, cmd_id, cmd_text, artist_id, artist_name "
		"FROM history_releases WHERE cmd_id = @cmd_id "
		"GROUP BY cmd_text ORDER BY date DESC LIMIT @param_pop_tops;";
	
	query_map.emplace(oplog_type::release, query_release);
	query_map.emplace(oplog_type::artist, query_artist);
	query_map.emplace(oplog_type::filter, query_filter);

	ret = db->open(full_dll_db_name(), SQLITE_OPEN_READWRITE);

	//
	if (!db->debug_sql_return(ret, "open", "foo_discogger - on init history update", "", 0, err_msg)) return false;
	//

	pDb = db->db_handle();

	ret = sqlite3_exec(pDb, "BEGIN TRANSACTION;", NULL, NULL, NULL);
	in_transaction = true;

	for (auto walk_flush : flush_map) {

		oplog_type thistype = walk_flush.first;

		const char* param_pop_tops = "@param_pop_tops";

		do {

			bool generate_vres = true;

			if (!delete_cmd.compare(kcmdHistoryWashup)) {

				int top_rows = cmd_param;

				ret = sqlite3_prepare_v2(db->db_handle(), walk_flush.second.c_str(), -1, &stmt_lk, NULL);
				int param_ndx;
				std::string cmd_label;

				param_ndx = sqlite3_bind_parameter_index(stmt_lk, "@param_pop_tops");
				ret = sqlite3_bind_int(stmt_lk, param_ndx, top_rows);
				
				cmd_label = task_map.at(thistype);
				param_ndx = sqlite3_bind_parameter_index(stmt_lk, "@cmd_id");				
				ret = sqlite3_bind_text(stmt_lk, param_ndx, cmd_label.c_str(), cmd_label.size(), NULL);

				if (thistype == oplog_type::artist) {

					param_ndx = sqlite3_bind_parameter_index(stmt_lk, "@cmd_id_sec");
					if (param_ndx) {
						ret = sqlite3_bind_text(stmt_lk, param_ndx, artist_sec_task.c_str(), artist_sec_task.size(), NULL);
					}
				}
				ret = sqlite3_step(stmt_lk); //SQLITE_DONE (101)

				sqlite3_reset(stmt_lk);
				
				if (!db->debug_sql_return(ret, "step delete", "foo_discogger - flush history", "", 0, err_msg)) break;

				//generate history vector

				if (generate_vres) {

					std::string query_gq = query_map.at(thistype);

					cmd = "prepare";

					ret = sqlite3_prepare_v2(pDb, query_gq.c_str(), -1, &stmt_read, NULL);
					//
					if (!db->debug_sql_return(ret, "prepare", PFC_string_formatter() << "foo_discogger - load releases history", "", 0, err_msg)) break;
					//

					int param_ndx;
					std::string cmd_idparam;

					param_ndx = sqlite3_bind_parameter_index(stmt_read, "@param_pop_tops");
					ret = sqlite3_bind_int(stmt_read, param_ndx, top_rows);

					cmd_idparam = task_map.at(thistype);
					if (thistype == oplog_type::artist) {
						
						param_ndx = sqlite3_bind_parameter_index(stmt_read, "@cmd_id");
						sqlite3_bind_text(stmt_read, 1, task_map.at(thistype).c_str(), -1, nullptr);
						sqlite3_bind_text(stmt_read, 2, artist_sec_task.c_str(), -1, nullptr);
					}
					else {
						cmd_idparam = task_map.at(thistype);
						cmd_idparam = task_map.at(thistype);
						param_ndx = sqlite3_bind_parameter_index(stmt_read, "@cmd_id");
						ret = sqlite3_bind_text(stmt_read, param_ndx, cmd_idparam.c_str(), cmd_idparam.size(), NULL);
					}

					//
					if (!db->debug_sql_return(ret, "bind", PFC_string_formatter() << "foo_discogger - load releases history", "", 0, err_msg)) break;
					//

					rppair rp_row;

					while (SQLITE_ROW == (ret = sqlite3_step(stmt_read)))
					{
						const char* tmp_val_1 = reinterpret_cast<const char*>(sqlite3_column_text(stmt_read, 0));	//release_id or filter
						const char* tmp_val_2 = reinterpret_cast<const char*>(sqlite3_column_text(stmt_read, 1));	//release_title

						const char* tmp_val_3 = reinterpret_cast<const char*>(sqlite3_column_text(stmt_read, 2));	//history date
						const char* tmp_val_4 = reinterpret_cast<const char*>(sqlite3_column_text(stmt_read, 5));	//artist_id
						const char* tmp_val_5 = reinterpret_cast<const char*>(sqlite3_column_text(stmt_read, 6));	//artist_name

						if (thistype == oplog_type::release) { //release
							if (tmp_val_1) {
								rp_row = std::pair(std::pair(tmp_val_1, tmp_val_2), std::pair(tmp_val_3, (PFC_string_formatter() << tmp_val_4 << " - " << tmp_val_5).c_str()));
								allout.at(thistype)->emplace_back(rp_row);
							}
						}
						else if (thistype == oplog_type::artist) { //artist
							if (tmp_val_5) {
								rp_row = std::pair(std::pair(tmp_val_1, tmp_val_2), std::pair(tmp_val_4, tmp_val_5));
								allout.at(thistype)->emplace_back(rp_row);
							}
						}
						else if (thistype == oplog_type::filter) { //filter
							if (tmp_val_1) {
								rp_row = std::pair(std::pair(tmp_val_1, tmp_val_2), std::pair(tmp_val_3, ""));
								allout.at(thistype)->emplace_back(rp_row);
							}
						}
					}

					if (ret == SQLITE_DONE) {
						//
					}
					else {
						err_msg << " Failed retrieving local db release (Err." << ret << ") ";
						break;
					}

					sqlite3_reset(stmt_read);
				}
			}


		} while (false);

		sqlite3_finalize(stmt_lk);
		sqlite3_finalize(stmt_read);

	} // end optype loop

	if (in_transaction) {
		ret = sqlite3_exec(pDb, "END TRANSACTION;", NULL, NULL, NULL);
	}

	if (err_msg.get_length()) {
		sqlite3_finalize(stmt_lk);
		sqlite3_finalize(stmt_read);
		db->close();
		foo_db_cmd_exception e(ret, cmd, err_msg);
		throw e;
	}

	db->close();

	return true;
}
