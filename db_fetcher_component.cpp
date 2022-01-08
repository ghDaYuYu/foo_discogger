#include "stdafx.h"
#include "db_utils.h"
#include "tag_mapping_credit_utils.h"
#include "db_fetcher_component.h"


vppair db_fetcher_component::query_release_credit_categories(int gx, pfc::string8 innon) {
	
	vppair vresult;
	pfc::string8 outnon = "and ((catid = 1 and subcatid = 4) or credit_id = 178))";

	sqldb db;
	db.open(dll_db_name());

	sqlite3_stmt* stmt_query = NULL;

	do {

		pfc::string8 query_gxc = 
			//note: REPLACE ',' to ', ' - check if still needed after grouping per credit spec mod
			"SELECT /*xC*/[REPLACE](GROUP_CONCAT(DISTINCT("
			"(SELECT c.name creditname "
			"FROM credit c "
			"WHERE c.id = pcd.credit_id"
			") "
			"|| IIF(pcd.credit_spec IS NULL,\'\', \' (\' || pcd.credit_spec || \')\' ))), \',\', \', \'), "

			"pcd.credit_id, "
			""
			"GROUP_CONCAT(DISTINCT(select cc.id catid "
			"from credit c left join credit_cat cc on cc.id = c.credit_cat_id where c.id = pcd.credit_id)) grp_catid , "
			"(select cc.id catid "
			"from credit c left join credit_cat cc on cc.id = c.credit_cat_id where c.id = pcd.credit_id) catid , "
			"(select cc.name catname "
			"from credit c left join credit_cat cc on cc.id = c.credit_cat_id where c.id = pcd.credit_id) catname , "
			"GROUP_CONCAT(DISTINCT (select ccs.name subcatname "
			"from credit c left join credit_cat cc on cc.id = c.credit_cat_id left join credit_cat_sub ccs where c.id = pcd.credit_id and ccs.credit_cat_id = cc.id)) grp_subcatname , "
			"pcd.id pcdid, "
			"[REPLACE](GROUP_CONCAT(DISTINCT(pcd.val)), \',\', \', \') pcdval, "
			"pcd.obs, "
			"GROUP_CONCAT(distinct (case when pcd.subval != \'SUBVAL\' then case when pcd.obs = 0 then pcd.subval else pcd.obs || \'.\' || pcd.subval end else pcd.subval end)) pcdsubval, "
			"(select c.credit_cat_sub_id ccsubid "
			"from credit c where c.id = pcd.credit_id) subcatid "
			""
			"from parsed_credit_det pcd "
			"inner join parsed_credit pc on pc.id = pcd.parsed_credit_id "
			""
			"where pc.subtype = \'C\' and ((catid = 1 and subcatid = 4) or credit_id = 178)) "
			//(commented to detail line for each credit spec mod) "group by /*pcdval*/credit_id;" 
			"group by /*pcdval*/credit_id, IIF(pcd.credit_spec IS NULL,\'\', \' (\' || pcd.credit_spec || \')\' );";

		size_t replace_res = query_gxc.replace_string(outnon, innon, 0);

		pfc::string8 query_gxp = 

			"SELECT /*xP*/[REPLACE](GROUP_CONCAT(DISTINCT("
													"(SELECT c.name creditname "
													"FROM credit c "
													"WHERE c.id = pcd.credit_id"
													") "
							"|| IIF(pcd.credit_spec IS NULL,\'\', \' (\' || pcd.credit_spec || \')\' ))), \',\', \', \'), "

			"GROUP_CONCAT(DISTINCT(pcd.credit_id)) credit_id, "
			""
			"(select cc.id catid "
			"from credit c left join credit_cat cc on cc.id = c.credit_cat_id where c.id = pcd.credit_id) catid , "
			"GROUP_CONCAT(DISTINCT(select cc.id catid "
			"from credit c left join credit_cat cc on cc.id = c.credit_cat_id where c.id = pcd.credit_id)) grp_catid , "
			""
			"(select cc.name catname "
			"from credit c left join credit_cat cc on cc.id = c.credit_cat_id where c.id = pcd.credit_id) catname , "
			"GROUP_CONCAT((select ccs.name subcatname "
			"from credit c left join credit_cat cc on cc.id = c.credit_cat_id left join credit_cat_sub ccs where c.id = pcd.credit_id and ccs.credit_cat_id = cc.id)) grp_subcatname , "
			"pcd.id pcdid, "
			"pcd.val pcdval, "
			"pcd.obs, "
			"GROUP_CONCAT(distinct(case when pcd.subval != \'SUBVAL\' then case when pcd.obs == \'OBS\' then pcd.subval else pcd.obs || \'.\' || pcd.subval end else pcd.subval end)) pcdsubval , "
			"/*GROUP_CONCAT(/*distinct*/(select c.credit_cat_sub_id ccsubid "
			"from credit c where c.id = pcd.credit_id)/*)*/ subcatid "
			""
			"from parsed_credit_det pcd "
			"inner join parsed_credit pc on pc.id = pcd.parsed_credit_id "
			"where pc.subtype = \'P\' and ((catid = 1 and subcatid = 4) or credit_id = 178))"
			"group by pcdval;";

		replace_res = query_gxp.replace_string(outnon, innon, 0);

		pfc::string8 query;
		if (gx) 
			query = query_gxc;
		else
			query = query_gxp;

#ifdef _DEBUG
		log_msg(query);
#endif

		//p_status.set_item("Fetching ...");

		int ret = 0;
		pfc::string8 err_msg;

		// prepare
		ret = db.prepare(query, &stmt_query, err_msg);

		pfc::string8 cmd = "prepare";
		pfc::string8 dbg_msg = "";

		//
		if (!db.debug_sql_return(ret, cmd, "", "", 0, dbg_msg))	break;
		//

		while (SQLITE_ROW == (ret = sqlite3_step(stmt_query)))
		{
			const char* tmp_val_credits_sub = reinterpret_cast<const char*>(sqlite3_column_text(stmt_query, 10));	//credit_sub role
			const char* tmp_val_credits_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt_query, 1));		//credit_id role

			const char* tmp_val_credits = reinterpret_cast<const char*>(sqlite3_column_text(stmt_query, 0));	//credit role
			const char* tmp_val_credits_1 = reinterpret_cast<const char*>(sqlite3_column_text(stmt_query, 3));	//cat_id
			const char* tmp_val_credits_2 = reinterpret_cast<const char*>(sqlite3_column_text(stmt_query, 9));	//track
			const char* tmp_val_artist_3 = reinterpret_cast<const char*>(sqlite3_column_text(stmt_query, 7));	//artist name

			if (tmp_val_credits) {

				vresult.emplace_back(std::pair(tmp_val_credits_id, tmp_val_credits), std::pair(tmp_val_artist_3, tmp_val_credits_2));
			}

#ifdef DEBUG
			if (!tmp_val_credits) {
				//orphan credit
				pfc::string8 warn_msg;
				warn_msg << "Unknown role credit for artist " << tmp_val_credits_2 << " found";
				log_msg(warn_msg);
			}
			else
				log_msg(tmp_val_credits);
#endif
		}
		
		if (ret == SQLITE_DONE) {
			sqlite3_finalize(stmt_query);
			stmt_query = nullptr;
		}
		else {
			err_msg << " Failed retrieving local db release (Err." << ret << ") ";
			break;
		}

	} while (false);

	db.close();

	return vresult;
}

bool db_fetcher_component::lookup_credit_tk(sqldb* db, sqlite3_stmt* qry, pfc::string8& err_msg, pfc::string8 thisrole, pfc::string8 thisfullrole, size_t& credit_id, pfc::string8& thisrole_spec) {
	bool bbreak = false;
	do {
		if (thisrole.get_length() != thisfullrole.get_length()) {
			// 1 + 1 = space + [
			thisrole_spec = substr(thisfullrole, thisrole.get_length() + 1 + 1, thisfullrole.get_length() - 1 - (thisrole.get_length() + 1 + 1));
		}

		// todo: test nulls on cat and sub cat
		int ret = -1;
		pfc::string8 cmd;
		sqlite3* pDb = db->db_handle();
		const char* param_role = "@param_role";
		int param_ndx = sqlite3_bind_parameter_index(qry, param_role);
		ret = sqlite3_bind_text(qry, param_ndx, thisrole, thisrole.get_length(), nullptr);
		cmd = "step";

		if (SQLITE_ROW == (ret = sqlite3_step(qry))) // see documentation, can return more values as success
		{
			int val = sqlite3_column_int(qry, 0);
			credit_id = val ? val : pfc_infinite;
		}
		else {
			if (SQLITE_DONE != ret) {
				//extra
				//ret = sqlite3_finalize(qry);
				err_msg = sqlite3_errmsg(pDb);
				break;
			}
			else {
				//non-matching roles are added without category reference
			}
		}

		cmd = "finalize";
		ret = sqlite3_reset(qry);
		//
		if (!db->debug_sql_return(ret, cmd, "", "", 0, err_msg)) break;
		//
		return true;

	} while (false);

	return false;
}

int db_fetcher_component::add_parsed_credit(sqldb* db, Release_ptr release, size_t gx) {

	int ret = -1;
	pfc::string8 cmd;
	pfc::string8 query;
	pfc::string8 err_msg;
	size_t inc_parsed_credit_id = pfc_infinite;

	pfc::string8 subtype_cell = (gx == 0 ? "P" : "C");

	sqlite3_stmt* stmt_lk = 0;
	pfc::string8 query_lk = "select * from credit c "
		"left outer join credit_cat cc on cc.id = c.credit_cat_id "
		"left outer join credit_cat_sub ccs on ccs.id = c.credit_cat_sub_id "
		"where c.name = @param_role;";
		//"where c.name = \'Bass\';";

	ret = db->open(dll_db_name());
	//
	if (!db->debug_sql_return(ret, "open", "foo_discogger", "", 0, err_msg)) goto quit;
	//
	ret = sqlite3_prepare_v2(db->db_handle(), query_lk, -1, &stmt_lk, 0);
	//
	if (!db->debug_sql_return(ret, "open", "foo_discogger - prepare credit lookup query", "", 0, err_msg)) goto quit;
	//

//#ifdef DO_TRANSACTIONS
	ret = sqlite3_exec(db->db_handle(), "BEGIN TRANSACTION;", NULL, NULL, NULL);
//#endif

	do {
	
		sqlite3* pDb = db->db_handle();

		if (ret == SQLITE_OK) {

			cmd = "delete";

			ret = sqlite3_exec(pDb, "PRAGMA foreign_keys=ON", NULL, NULL, NULL);

			query = "";
			query << "DELETE FROM parsed_credit where subtype = \'" << subtype_cell << "\'";

			ret |= sqlite3_exec(pDb, query, NULL, NULL, NULL);

			//
			if (!db->debug_sql_return(ret, cmd, "", "", 0, err_msg)) break;
			//

			ret = sqlite3_exec(pDb, "PRAGMA foreign_keys=OFF", NULL, NULL, NULL);

			cmd = "insert";

			query = "INSERT INTO parsed_credit (release_id, type, subtype, val, subval) ";
			query << "VALUES(";
			query << release->id << ", ";
			query << "\'Release\', " << "\'" << subtype_cell << "\', " << "\'VAL\', \'SUBVAL\'";
			query << ")";

			ret = sqlite3_exec(pDb, query, NULL, NULL, NULL);

			//
			if (!db->debug_sql_return(ret, cmd, "", "", 0, err_msg)) break;
			//

			size_t parsed_credit_id = pfc_infinite;
			size_t credit_id = pfc_infinite;

			char* err;

			cmd = "select";

			ret = sqlite3_exec(pDb,
				"select seq from sqlite_sequence where name = 'parsed_credit'",
				int_type_sqlite3_exec_callback,
				&inc_parsed_credit_id,
				&err
			);

			//
			if (!db->debug_sql_return(ret, cmd, "", "", 0, err_msg)) break;
			//

			for (size_t i = 0; i < release->credits.get_size(); i++) {

				ReleaseCredit_ptr walk_credit = release->credits[i];

				pfc::string8 raw_roles = walk_credit->raw_roles;
				pfc::array_t<pfc::string8> roles = walk_credit->roles;
				pfc::array_t<pfc::string8> full_roles = walk_credit->full_roles;

				for (size_t i = 0; i < roles.get_count(); i++) {
					pfc::string8 thisrole = roles[i];
					pfc::string8 thisfullrole = full_roles[i];
					pfc::string8 thisartists;
					pfc::string8 thisrole_spec;

					bool blkok = lookup_credit_tk(db, stmt_lk, err_msg, thisrole, thisfullrole, credit_id, thisrole_spec);
					
					if (!blkok) break;
					
					for (size_t j = 0; j < walk_credit->artists.get_count(); j++) {
						ReleaseArtist_ptr thisartist = walk_credit->artists[j];
					
						//type p (0): one credit detail per artist
						//type c (1): one detail containing comma separated artist names (per credit)

						if (gx == 0) {
							size_t insres = insert_parsed_credit_detail(db, err_msg, credit_id, inc_parsed_credit_id,
								~0, ~0, thisrole_spec, thisartist->name);
							if (insres == pfc_infinite) break;
						}
						else {
							thisartists << (j > 0 ? ", " : "") << thisartist->name; // << thisartist->join;
						}
					}

					if (gx == 1) {
						size_t insres = insert_parsed_credit_detail(db, err_msg,
							credit_id, inc_parsed_credit_id,
							~0, ~0, thisrole_spec, thisartists);
						if (insres == pfc_infinite) break;
					}

					//todo: tokenize queries
					//todo: swap obs and val, mod queries
					
				} // end for roles
			} //end for credits

			for (size_t i = 0; i < release->discs.get_size(); i++) {

				ReleaseDisc_ptr walk_disc = release->discs[i];

				for (size_t j = 0; j < walk_disc->tracks.get_size(); j++) {
					ReleaseTrack_ptr walk_track = walk_disc->tracks[j];
					//todo: recursion
					for (size_t k = 0; k < walk_track->credits.get_size(); k++) {

						ReleaseCredit_ptr walk_track_credit = walk_track->credits[k];;

						pfc::string8 raw_roles = walk_track_credit->raw_roles;
						pfc::array_t<pfc::string8> roles = walk_track_credit->roles;
						pfc::array_t<pfc::string8> full_roles = walk_track_credit->full_roles;

						for (size_t l = 0; l < roles.get_count(); l++) {
							pfc::string8 thisrole = roles[l];
							pfc::string8 thisfullrole = full_roles[l];
							pfc::string8 thisartists;
							pfc::string8 thisrole_spec;

							bool blkok = lookup_credit_tk(db, stmt_lk, err_msg, thisrole, thisfullrole, credit_id, thisrole_spec);

							if (!blkok) break;

							for (size_t m = 0; m < walk_track_credit->artists.get_count(); m++) {
								ReleaseArtist_ptr thisartist = walk_track_credit->artists[m];

								if (gx == 0) {
									//todo: add disc # to track #
									size_t insres = insert_parsed_credit_detail(db, err_msg,
										credit_id, inc_parsed_credit_id,
										i + 1, j + 1, thisrole_spec, thisartist->name);

									if (insres == pfc_infinite) break;
								}
								else {
									thisartists << (m > 0 ? ", " : "") << thisartist->name; // << thisartist->join;
								}

							}
							if (gx == 1) {
								size_t insres = insert_parsed_credit_detail(db, err_msg,
									credit_id, inc_parsed_credit_id,
									i + 1, j + 1, thisrole_spec, thisartists);
								if (insres == pfc_infinite) break;
							}
							
							//todo: swap obs and val and mod queries
							
						} //end roles
					} // end credits
				} // end tracks
			} // end discs
		}
	} while (false);

quit:
	if (stmt_lk)
		ret = sqlite3_finalize(stmt_lk);

//#ifdef DO_TRANSACTIONS
	ret = sqlite3_exec(db->db_handle(), "END TRANSACTION;", NULL, NULL, NULL);
//#endif

	db->close();

	if (err_msg.get_length()) {
		//db->close();
		foo_db_cmd_exception e(ret, cmd, err_msg);
		throw e;
	}
	else {
		//
	}

	return inc_parsed_credit_id;
}

size_t db_fetcher_component::insert_parsed_credit_detail(sqldb* db, pfc::string8 &err_msg, size_t credit_id, size_t inc_parsed_credit_id, size_t i, size_t j, pfc::string8 credit_spec, pfc::string8 thisartists) {
	
	//note: thisartists contains just one artist in gxp
	//todo: swap obs and val and mod queries
	//todo: default obs and subval to null

	pfc::string8 query;
	query = "INSERT INTO parsed_credit_det (parsed_credit_id, credit_id, credit_spec, obs, val, subval) ";
	query << "VALUES(";
	query << inc_parsed_credit_id << ", ";
	query << (credit_id == pfc_infinite ? "NULL" : std::to_string(credit_id).c_str()) << ", ";

	query << (!credit_spec.get_length() ? "NULL" : (PFC_string_formatter() << db_slh_apos(credit_spec))) << ", ";

	//note: i and j are ~0 for releases detail (no disc, no track)

	if ((i == pfc_infinite) || (j == pfc_infinite)) {
		query << db_slh_apos("OBS") << ", " << db_slh_apos(db_dbl_apos(thisartists)) << ", ";
		query << db_slh_apos("SUBVAL");
	}
	else {
		query << "\'" << i << "\', " << db_slh_apos(db_dbl_apos(thisartists)) << ", " << "\'" << j << "\'";
	}
	query << ");";

	pfc::string8 cmd = "exec";
	int ret = sqlite3_exec(db->db_handle(), query, NULL, NULL, NULL);

	//
	if (!db->debug_sql_return(ret, cmd, "insert credit detail", "", 0, err_msg)) return pfc_infinite;
	//

	return ret == 0 ? ret : pfc_infinite;
}


vppair db_fetcher_component::load_db_def_credits() {

	vppair vresults;

	sqldb db;
	db.open(dll_db_name());

	sqlite3_stmt* stmt_query = NULL;

	do {

		pfc::string8 query_defs =
			"select id, name, titleformat from def_credit order by id";

		pfc::string8 query = query_defs;

#ifdef _DEBUG
		log_msg(query_defs);
#endif

		//p_status.set_item("Fetching credit definitions...");
		
		int ret = 0;
		pfc::string8 error_msg;

		// prepare
		if (SQLITE_OK != (ret = db.prepare(query, &stmt_query, error_msg)))
		{
			error_msg << "Failed to retrieve local db credit definitions (Err." << ret << ") ";
			break;
		}

		while (SQLITE_ROW == (ret = sqlite3_step(stmt_query)))
		{

			const char* tmp_val_row = reinterpret_cast<const char*>(sqlite3_column_text(stmt_query, 0));	//catid
			const char* tmp_val_row_1 = reinterpret_cast<const char*>(sqlite3_column_text(stmt_query, 1));	//catname
			const char* tmp_val_row_2 = reinterpret_cast<const char*>(sqlite3_column_text(stmt_query, 2));	//track

			vresults.emplace_back(std::pair(tmp_val_row, tmp_val_row_1), std::pair(tmp_val_row_2, ""));

#ifdef DEBUG
			//log_msg(tmp_val_credits);
#endif
		}

		if (ret == SQLITE_DONE) {
			sqlite3_finalize(stmt_query);
			stmt_query = nullptr;
		}
		else {
			error_msg << "Failed retrieving local db release (Err." << ret << ") ";
			break;
		}

	} while (false);

	db.close();

	return vresults;
}

bool db_fetcher_component::update_db_def_credits(vppair& vdefs) {

	bool bres = false;

	vppair volddefs = load_db_def_credits();

	sqldb db;
	int ret = 0; pfc::string8 error_msg;

	db.open(dll_db_name());

	{
		char* err;
		ret = sqlite3_exec(db.db_handle(),
			"DELETE FROM def_credit", NULL, NULL, &err);
		ret = sqlite3_exec(db.db_handle(),
			"DELETE FROM parsed_credit_det", NULL, NULL, &err);
		ret = sqlite3_exec(db.db_handle(),
			"DELETE FROM parsed_credit", NULL, NULL, &err);
	}

	pfc::string8 upd_query;
	sqlite3_stmt* stmt_query = NULL;
	bool isUpdate = false;

	//todo: tokenize query

	for (rppair& walk_row : vdefs) {

		upd_query = "INSERT INTO def_credit (name, titleformat) VALUES (";
		upd_query << "\'" << walk_row.first.second << "\', ";
		upd_query << "\'" << walk_row.second.first << "\'";
		upd_query << ")";
		
#ifdef _DEBUG
		log_msg(upd_query);
#endif

		//p_status.set_item("Updating credit definition...");

		if (SQLITE_OK != (ret = db.prepare(upd_query, &stmt_query, error_msg))) {
			error_msg << "Failed to prepare update credit definitions (Err." << ret << ") ";
			break;
		}

		if (SQLITE_DONE != (ret = sqlite3_step(stmt_query))) {
			error_msg << "Failed to update credit definitions (Err." << ret << ") ";
			break;
		}

		if (!isUpdate) {
			size_t inc_id = pfc_infinite;
			size_t credit_id = pfc_infinite;
			char* err;
			ret = sqlite3_exec(db.db_handle(),
				"select seq from sqlite_sequence where name = 'def_credit'",
				int_type_sqlite3_exec_callback,
				&inc_id,
				&err
			);
			walk_row.first.first = std::to_string(inc_id).c_str();
		}
	
		if (SQLITE_OK != (ret = sqlite3_finalize(stmt_query))) {
			error_msg << "Failed to finalize update credit definitions (Err." << ret << ") ";
			break;
		}
		stmt_query = nullptr;
	}

	if (ret == SQLITE_DONE) {
		ret = sqlite3_finalize(stmt_query);
		stmt_query = nullptr;
	}

	if (ret != SQLITE_OK) {
		error_msg << "Failed to update definitions (Err." << ret << ") ";
		error_msg << sqlite3_errmsg(db.db_handle());
	}

	if (error_msg.get_length()) {
		db.close();
		foo_db_cmd_exception e(ret, "update credit definitions", error_msg);
		throw e;
	}
	db.close();
	return bres;
}

vppair db_fetcher_component::load_db_def_credits(vppair& vcats, vppair& vcatsubs) {

	vppair vresults;

	sqldb db;
	db.open(dll_db_name());

	sqlite3_stmt* stmt_query = NULL;

	do {

		pfc::string8 query_cats =
			"select id, name from credit_cat order by id";

		pfc::string8 query = query_cats;

#ifdef _DEBUG
		log_msg(query_cats);
#endif

		//p_status.set_item("Fetching release from local database...");
		int ret = 0; pfc::string8 error_msg;
		
		// prepare
		if (SQLITE_OK != (ret = db.prepare(query, &stmt_query, error_msg))) 
		{
			error_msg << "Failed to retrieve local db release (Err." << ret << ") ";
			break;
		}

		// step into row
		while (SQLITE_ROW == (ret = sqlite3_step(stmt_query)))
		{
			const char* tmp_val_row = reinterpret_cast<const char*>(sqlite3_column_text(stmt_query, 0));	//catid
			const char* tmp_val_row_1 = reinterpret_cast<const char*>(sqlite3_column_text(stmt_query, 1));	//catname

			vcats.emplace_back(std::pair(tmp_val_row, tmp_val_row_1), std::pair("", ""));

#ifdef DEBUG
			//log_msg(tmp_val_credits);
#endif
		}

		if (ret == SQLITE_DONE) {
			sqlite3_finalize(stmt_query);
			stmt_query = nullptr;
		}
		else {
			error_msg << "Failed retrieving local db release (Err." << ret << ") ";
			break;
		}

		pfc::string8 query_cat_sub =
			"select id, credit_cat_id, name from credit_cat_sub order by id";

		query = query_cat_sub;

		if (SQLITE_OK != (ret = db.prepare(query, &stmt_query, error_msg))) //, sqlite3_prepare_v2(pDb, query.get_ptr(), -1, &stmt_query, NULL)))
		{
			error_msg << "Failed to retrieve local db release (Err." << ret << ") ";
			break;
		}

		while (SQLITE_ROW == (ret = sqlite3_step(stmt_query)))
		{
			const char* tmp_val_row = reinterpret_cast<const char*>(sqlite3_column_text(stmt_query, 0));	//catsubid
			const char* tmp_val_row_1 = reinterpret_cast<const char*>(sqlite3_column_text(stmt_query, 1));	//catid
			const char* tmp_val_row_2 = reinterpret_cast<const char*>(sqlite3_column_text(stmt_query, 2));	//name

			vcatsubs.emplace_back(std::pair(tmp_val_row, tmp_val_row_1), std::pair(tmp_val_row_2, ""));
			
#ifdef DEBUG
			//log_msg(tmp_val_credits);
#endif

		}
		if (ret == SQLITE_DONE) {
			sqlite3_finalize(stmt_query);
			stmt_query = nullptr;
		}

		query = 
			"select c.id, c.credit_cat_id CreditId, c.name || \'|\' || c.notes NameNotes, c.credit_cat_sub_id SubCreditId, "
			"(select cc.name from credit_cat cc where cc.id = c.credit_cat_id) lkCatname, "
			"(select ccs.name from credit_cat_sub ccs where ccs.id = c.credit_cat_sub_id) lkSubCatName "
			"from credit c order by c.credit_cat_id, c.credit_cat_sub_id ";

		if (SQLITE_OK != (ret = db.prepare(query, &stmt_query, error_msg)))
		{
			error_msg << "Failed to retrieve local db release (Err." << ret << ") ";
			break;
		}

		while (SQLITE_ROW == (ret = sqlite3_step(stmt_query)))
		{
			const char* tmp_val_row_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt_query, 0));	//creditid
			const char* tmp_val_row = reinterpret_cast<const char*>(sqlite3_column_text(stmt_query, 1));	//catid
			const char* tmp_val_row_1 = reinterpret_cast<const char*>(sqlite3_column_text(stmt_query, 2));	//name | notes
			const char* tmp_val_row_2 = reinterpret_cast<const char*>(sqlite3_column_text(stmt_query, 3));	//subcatid
			const char* tmp_val_row_3 = reinterpret_cast<const char*>(sqlite3_column_text(stmt_query, 4));	//catname
			const char* tmp_val_row_4 = reinterpret_cast<const char*>(sqlite3_column_text(stmt_query, 5));	//subcatname

			size_t catsubcat = atoi(tmp_val_row ? tmp_val_row : "0") *100 + atoi(tmp_val_row_2 ? tmp_val_row_2 : "0");
			LPARAM lparam = MAKELPARAM(atoi(tmp_val_row_id ? tmp_val_row_id : "0"), catsubcat);
			pfc::string8 str_lparam;
			str_lparam << lparam;
			
			vresults.emplace_back(std::pair(std::pair(str_lparam, tmp_val_row_1),
				std::pair(tmp_val_row_3 == nullptr? "" : tmp_val_row_3, tmp_val_row_4 == nullptr ? "" : tmp_val_row_4)));

#ifdef DEBUG
			//log_msg(tmp_val_credits);
#endif
		}
		if (ret == SQLITE_DONE) {
			sqlite3_finalize(stmt_query);
			stmt_query = nullptr;
		}
		else {
			error_msg << "Failed retrieving local db release (Err." << ret << ") ";
			break;
		}

	} while (false);
    
    //todo: recheck
	//sqlite3_clear_bindings(stmt_query);  /* optional */
	//sqlite3_reset(stmt_query);
	//sqlite3_finalize(stmt_query);

	db.close();
	return vresults;
}

int db_fetcher_component::add_history(sqldb* db, Artist_ptr artist, Release_ptr release, pfc::string8 cmd_id, pfc::string8 cmd_text, rppair& out) {

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
bool db_fetcher_component::test_discogs_db(pfc::string8 db_path, abort_callback& p_abort, threaded_process_status& p_status) {

	bool lib_initialized = true;
	bool ok_db = true;

	pfc::string8 error_msg;

	pfc::string8 foo_dbpath;
	foo_dbpath << db_path;
	extract_native_path(foo_dbpath, foo_dbpath);

	sqlite3* m_pDb;
	sqlite3_stmt* m_query = NULL;

	int ret = 0;
	do
	{
		// initialize engine
		if (SQLITE_OK != (ret = sqlite3_initialize()))
		{
			lib_initialized = false;
			ok_db = false;
			error_msg << "Failed to initialize DB library: " << ret;
			break;
		}

		// open connection to a DB
		if (SQLITE_OK != (ret = sqlite3_open_v2(foo_dbpath, &m_pDb, SQLITE_OPEN_READWRITE /*| SQLITE_OPEN_CREATE*/, NULL)))
		{
			ok_db = false;
			error_msg << "Failed to open DB connection: " << ret;
			break;
		}

#ifdef DO_TRANSACTIONS
		sqlite3_exec(m_pDb, "BEGIN TRANSACTION;", NULL, NULL, NULL);
#endif

	} while (false);

	pfc::string8 query_catalog;
	query_catalog << "SELECT c.type, c.name, c.tbl_name from sqlite_master c ";
	query_catalog << "where c.tbl_name = \'release\';";

	ok_db = (lib_initialized && m_pDb != NULL && m_query == NULL && ret == SQLITE_OK);
	ret = 0;

	if (ok_db) {
		do {

#ifdef _DEBUG
			log_msg(query_catalog);
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