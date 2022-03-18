#include "stdafx.h"

#include "db_utils.h"

#ifdef CAT_CRED
#include "tag_mapping_credit_utils.h"
#endif

#include "db_fetcher_component.h"

int db_fetcher_component::insert_history(sqldb* db, oplog_type optype, std::string cmd_log, pfc::string8 cmd_param, rppair& out) {

	if (!CONF.history_enabled()) return 0;

	pfc::string8 artist_id;
	pfc::string8 artist_name;
	pfc::string8 release_id;
	pfc::string8 release_title;
	
	if (artist || release) {
		artist_id = artist ? artist->id : release->artists[0]->full_artist->id;
		artist_name = artist ? artist->name : release->artists[0]->full_artist->name;
		release_id = release ? release->id : "";
		release_title = release ? release->title : "";
	}
	else {
		//
	}

	if (release || artist) out = std::pair(std::pair(release_id.c_str(), release_title.c_str()), std::pair(artist_id.c_str(), artist_name.c_str()));

	int ret = -1;
	pfc::string8 cmd;
	pfc::string8 query;
	pfc::string8 err_msg;
	size_t inc_insert = pfc_infinite;
	sqlite3* pDb;
	bool in_transaction = false;

	do {

		ret = db->open(dll_db_name());

		if (!db->debug_sql_return(ret, "open", "foo_discogger", "", 0, err_msg)) break;

		pDb = db->db_handle();

		if (ret == SQLITE_OK) {

//#ifdef DO_TRANSACTIONS
			ret = sqlite3_exec(db->db_handle(), "BEGIN TRANSACTION;", NULL, NULL, NULL);
			in_transaction = true;
//#endif      

			cmd = "insert";

			query = "INSERT INTO history_releases (release_id, title, date, cmd_id, cmd_text, artist_id, artist_name) ";
			query << "VALUES(";
			query << "\'" << release_id << "\', ";
			query << "\'" << db_dbl_apos(release_title) << "\', ";
			query << "DateTime(\'now\') " << ", ";
			query << "\'" << cmd_id << "\', ";
			query << "\'" << db_dbl_apos(cmd_text) << "\', ";
			query << "\'" << artist_id << "\' , ";
			query << "\'" << db_dbl_apos(artist_name) << "\'";
			query << ");";

#ifdef DEBUG
			log_msg(query);
#endif 
			char* err;
			ret = sqlite3_exec(pDb, query, int_type_sqlite3_exec_callback, &inc_insert, &err);

			//
			if (!db->debug_sql_return(ret, cmd, "add history details", "", 0, err_msg)) break;
			//
		}

		;

	} while (false);

//#ifdef DO_TRANSACTIONS
	if (in_transaction)
		ret = sqlite3_exec(db->db_handle(), "END TRANSACTION;", NULL, NULL, NULL);
//#endif

	if (err_msg.get_length()) {
		db->close();
		foo_db_cmd_exception e(ret, cmd, err_msg);
		throw e;
	}

	db->close();
	return inc_insert;
}

int db_fetcher_component::read_history_tmp(sqldb* db, Artist_ptr artist, Release_ptr release, pfc::string8 cmd_id, pfc::string8 cmd_text, vppair& vres) {

	//todo: impl artist and filter

	if (!CONF.history_enabled()) return 0;

	pfc::string8 artist_id;
	pfc::string8 artist_name;
	pfc::string8 release_id;
	pfc::string8 release_title;

	if (artist || release) {
		artist_id = artist ? artist->id : release->artists[0]->full_artist->id;
		artist_name = artist ? artist->name : release->artists[0]->full_artist->name;
		release_id = release ? release->id : "";
		release_title = release ? release->title : "";
	}
	else {
		//
	}

	int ret = -1;
	pfc::string8 cmd;
	pfc::string8 query;
	pfc::string8 err_msg;
	size_t inc_insert = pfc_infinite;

	sqlite3_stmt* stmt_lk = 0;
	sqlite3* pDb = nullptr;

	do {

		int ret = db->open(dll_db_name());

		//
		if (!db->debug_sql_return(ret, "open", "foo_discogger - on read history", "", 0, err_msg)) break;
		//

		pDb = db->db_handle();

		//#ifdef DO_TRANSACTIONS
		ret = sqlite3_exec(pDb, "BEGIN TRANSACTION;", NULL, NULL, NULL);
		//#endif      

		query = "SELECT release_id, title, date, cmd_id, cmd_text, artist_id, artist_name "
			"FROM history_releases WHERE cmd_id = \"proccess_release_callback\" "
			"GROUP BY release_id ORDER BY date DESC LIMIT 15;";

		//p_status.set_item("Testing discogs local db...");

		pfc::string8 query_lk = query;
		ret = sqlite3_prepare_v2(pDb, cmd, -1, &stmt_lk, NULL);

		//
		if (!db->debug_sql_return(ret, "prepare", PFC_string_formatter() << "foo_discogger - load releases history", "", 0, err_msg)) break;
		//

		rppair rp_row;

		while (SQLITE_ROW == (ret = sqlite3_step(stmt_lk)))
		{
			const char* tmp_val_1 = reinterpret_cast<const char*>(sqlite3_column_text(stmt_lk, 0));	//release_id
			const char* tmp_val_2 = reinterpret_cast<const char*>(sqlite3_column_text(stmt_lk, 1));	//release_title

			const char* tmp_val_3 = reinterpret_cast<const char*>(sqlite3_column_text(stmt_lk, 2));	//history date
			const char* tmp_val_4 = reinterpret_cast<const char*>(sqlite3_column_text(stmt_lk, 3));	//artist_id
			const char* tmp_val_5 = reinterpret_cast<const char*>(sqlite3_column_text(stmt_lk, 4));	//artist_name

			if (tmp_val_1) {
				rp_row = std::pair(std::pair(tmp_val_1, tmp_val_2), std::pair(tmp_val_3, (PFC_string_formatter() << tmp_val_4 << "|" << tmp_val_5).c_str()));
				vres.emplace_back(rp_row);
			}
		}

		if (ret == SQLITE_DONE) {
			sqlite3_finalize(stmt_lk);
			stmt_lk = nullptr;
		}
		else {
			err_msg << " Failed retrieving local db release (Err." << ret << ") ";
			break;
		}

	} while (false);

	//#ifdef DO_TRANSACTIONS
	ret = sqlite3_exec(pDb, "END TRANSACTION;", NULL, NULL, NULL);
	//#endif

	if (err_msg.get_length()) {
		db->close();
		foo_db_cmd_exception e(ret, cmd, err_msg);
		throw e;
	}
	
quit:

	sqlite3_finalize(stmt_lk);
	db->close();
	return inc_insert;
}

int db_fetcher_component::delete_history(sqldb* db, Artist_ptr artist, Release_ptr release, pfc::string8 cmd_id, pfc::string8 cmd_text, std::vector<vppair*>allout) {

	int ret = -1;
	pfc::string8 cmd;
	pfc::string8 err_msg;
	size_t inc_insert = pfc_infinite;

	if (cmd_id.equals("delete all")) {
		ret =  db->open(dll_db_name());

		if (db->debug_sql_return(ret, "open", "foo_discogger - on init history update", "", 0, err_msg))
			ret = sqlite3_exec(db->db_handle(), "DELETE FROM history_releases;", NULL, NULL, NULL);

		return 0;
	}

	pfc::string8 artist_id;
	pfc::string8 artist_name;
	pfc::string8 release_id;
	pfc::string8 release_title;

	if (artist || release) {
		artist_id = artist ? artist->id : release->artists[0]->full_artist->id;
		artist_name = artist ? artist->name : release->artists[0]->full_artist->name;
		release_id = release ? release->id : "";
		release_title = release ? release->title : "";
	}
	else {
		//
	}

	sqlite3_stmt* stmt_lk = 0;
	sqlite3_stmt* stmt_read = 0;
	sqlite3* pDb = nullptr;
	bool in_transaction = false;

	std::vector<pfc::string8> v_cmd_id = {
		"proccess_release_callback",
		"search_artist_process_callback OR cmd_id = \'get_artist_process_callback\'"
		/*, "filter_releases"*/
	};

	std::vector<pfc::string8> v_group_flush = {
		//leave latest x unique release_id rows
		"DELETE FROM history_releases WHERE cmd_id = :cmd_id AND id NOT IN (SELECT grp_hr.id FROM "
		"(SELECT id FROM history_releases WHERE cmd_id = :cmd_id "
		"GROUP BY release_id ORDER BY date DESC LIMIT :param_pop_tops) grp_hr);"
		,
		//leave latest x unique release_id rows
		"DELETE FROM history_releases WHERE cmd_id = :cmd_id AND id NOT IN (SELECT grp_hr.id FROM "
		"(SELECT id FROM history_releases WHERE cmd_id = :cmd_id "
		"GROUP BY artist_id ORDER BY date DESC LIMIT :param_pop_tops) grp_hr);"
		/*,
		//leave latest x unique release_id rows
		"DELETE FROM history_releases WHERE cmd_id = :cmd_id AND id NOT IN (SELECT grp_hr.id FROM "
		"(SELECT id FROM history_releases WHERE cmd_id = :cmd_id "
		"GROUP BY cmd_text ORDER BY date DESC LIMIT :param_pop_tops AS INT) grp_hr);"*/
	};


	std::vector<pfc::string8> v_group_query = {

	"SELECT release_id, title, date, cmd_id, cmd_text, artist_id, artist_name "
	"FROM history_releases WHERE cmd_id = :cmd_id "
	"GROUP BY release_id ORDER BY date DESC LIMIT :param_pop_tops;"
	,
	"SELECT artist_id, artist_name, date, cmd_id, cmd_text, artist_id, artist_name "
	"FROM history_releases WHERE cmd_id = :cmd_id "
	"GROUP BY artist_id ORDER BY date DESC LIMIT :param_pop_tops;"
	/*,
	"SELECT cmd_text, \"\", date, cmd_id, cmd_text, artist_id, artist_name "
	"FROM history_releases WHERE cmd_id = :cmd_id "
	"GROUP BY cmd_text ORDER BY date DESC LIMIT :param_pop_tops;"*/
	};


	ret = db->open(dll_db_name());
	//
	if (!db->debug_sql_return(ret, "open", "foo_discogger - on init history update", "", 0, err_msg)) return 0;
	//
	pDb = db->db_handle();

//#ifdef DO_TRANSACTIONS		
	ret = sqlite3_exec(pDb, "BEGIN TRANSACTION;", NULL, NULL, NULL);
	in_transaction = true;
//#endif


	for (size_t walk_group = 0; walk_group < v_group_query.size()/*allout.size()*/; walk_group++) {

		const char* param_pop_tops = ":param_pop_tops";

		do {

			bool generate_vres = false;

			pfc::string8 cmd_leave_latest = "cmd_leave_latest";

			if (cmd_id.equals(cmd_leave_latest)) {

				std::string top_rows = cmd_text;
				if (top_rows.length()) {
					generate_vres = top_rows.at(0) == '+';
					top_rows = top_rows.erase(0, 1);
				}
				else
					top_rows = "";

				pfc::string8 query_gf = v_group_flush.at(walk_group);

				ret = sqlite3_prepare_v2(db->db_handle(), query_gf, -1, &stmt_lk, NULL);

				//
				//if (!db->debug_sql_return(ret, "prepare", PFC_string_formatter() << "foo_discogger - prepare history", "", 0, err_msg)) break;
				//

				int param_ndx = sqlite3_bind_parameter_index(stmt_lk, ":param_pop_tops");
				ret = sqlite3_bind_int(stmt_lk, param_ndx, atoi(top_rows.c_str()));
				pfc::string8 cmd_idparam = v_cmd_id.at(walk_group);
				param_ndx = sqlite3_bind_parameter_index(stmt_lk, ":cmd_id");
				ret = sqlite3_bind_text(stmt_lk, param_ndx, cmd_idparam, cmd_idparam.get_length(), NULL);
				ret = sqlite3_step(stmt_lk);

				sqlite3_reset(stmt_lk);
				
				if (!db->debug_sql_return(ret, "step delete", "foo_discogger - flush history", "", 0, err_msg)) break;

				//generate history vector

				if (generate_vres) {

					pfc::string8 query_gq = v_group_query.at(walk_group);

					cmd = "prepare";

					ret = sqlite3_prepare_v2(pDb, query_gq, -1, &stmt_read, NULL);
					//
					if (!db->debug_sql_return(ret, "prepare", PFC_string_formatter() << "foo_discogger - load releases history", "", 0, err_msg)) break;
					//
					int param_ndx = sqlite3_bind_parameter_index(stmt_read, ":param_pop_tops");
					ret = sqlite3_bind_int(stmt_read, param_ndx, atoi(top_rows.c_str()));
					pfc::string8 cmd_idparam = v_cmd_id.at(walk_group);
					param_ndx = sqlite3_bind_parameter_index(stmt_read, ":cmd_id");
					ret = sqlite3_bind_text(stmt_read, param_ndx, cmd_idparam, cmd_idparam.get_length(), NULL);

					//
					if (!db->debug_sql_return(ret, "bind", PFC_string_formatter() << "foo_discogger - load releases history", "", 0, err_msg)) break;
					//

					rppair rp_row;

					while (SQLITE_ROW == (ret = sqlite3_step(stmt_read)))
					{
						const char* tmp_val_1 = reinterpret_cast<const char*>(sqlite3_column_text(stmt_read, 0));	//release_id
						const char* tmp_val_2 = reinterpret_cast<const char*>(sqlite3_column_text(stmt_read, 1));	//release_title

						const char* tmp_val_3 = reinterpret_cast<const char*>(sqlite3_column_text(stmt_read, 2));	//history date
						const char* tmp_val_4 = reinterpret_cast<const char*>(sqlite3_column_text(stmt_read, 5));	//artist_id
						const char* tmp_val_5 = reinterpret_cast<const char*>(sqlite3_column_text(stmt_read, 6));	//artist_name

						if (walk_group == 0) { //release
							if (tmp_val_1) {
								rp_row = std::pair(std::pair(tmp_val_1, tmp_val_2), std::pair(tmp_val_3, (PFC_string_formatter() << tmp_val_4 << " - " << tmp_val_5).c_str()));
								allout.at(walk_group)->emplace_back(rp_row);
							}
						}
						else if (walk_group == 1) { //artist
							if (tmp_val_5) {
								rp_row = std::pair(std::pair(tmp_val_1, tmp_val_2), std::pair(tmp_val_4, tmp_val_5));
								allout.at(walk_group)->emplace_back(rp_row);
							}
						}
						else if (walk_group == 2) { //filter
							//		
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

	} // END GROUP LOOP

//#ifdef DO_TRANSACTIONS
	if (in_transaction)
		ret = sqlite3_exec(pDb, "END TRANSACTION;", NULL, NULL, NULL);
//#endif
	
	if (err_msg.get_length()) {
		sqlite3_finalize(stmt_lk);
		sqlite3_finalize(stmt_read);
		db->close();
		foo_db_cmd_exception e(ret, cmd, err_msg);
		throw e;
	}

	db->close();
	return inc_insert;
}

#ifdef DB_DC
#include "db_fecher_component_db_dc.cpp"
#endif
#ifdef CAT_CRED
#include "db_fetcher_component_cat_cred.cpp"
#endif

			p_status.set_item("Testing discogs local db...");

			if (SQLITE_OK != (ret = sqlite3_prepare_v2(m_pDb, query_catalog, -1, &m_query, NULL)))
			{

				ok_db = false;

				error_msg << "Failed to prepare DB selection: " << ret << " . ";
				error_msg << sqlite3_errmsg(m_pDb);

				break;
			}

			if (SQLITE_ROW == (ret = sqlite3_step(m_query)))
			{
				//
			}
			else {
				ok_db = false;
				error_msg << "Backup information not found. ";
				error_msg << pfc::string8(sqlite3_errmsg(m_pDb));

#ifdef _DEBUG			
				log_msg(error_msg);
#endif
			}

		} while (false);
	}

	if (NULL != m_query) sqlite3_finalize(m_query);
	if (NULL != m_pDb) sqlite3_close(m_pDb);
	if (lib_initialized) sqlite3_shutdown();

	if (error_msg.get_length()) {
		foo_discogs_exception ex;
		ex << error_msg;
		throw ex;
	}

	return ok_db;
}
#endif