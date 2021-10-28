#include "stdafx.h"

#include "db_utils.h"
#include "db_fetcher.h"

//todo: LIKE revision
pfc::string sqliteEscapeLIKE(pfc::string8 keyWord) {
	pfc::string8 out_keyWord(keyWord);
	out_keyWord.replace_string("/", "//");
	out_keyWord.replace_string("'", "''");
	out_keyWord.replace_string("[", "/[");
	out_keyWord.replace_string("]", "/]");
	out_keyWord.replace_string("%", "/%");
	out_keyWord.replace_string("&", "/&");
	out_keyWord.replace_string("_", "/_");
	out_keyWord.replace_string("(", "/(");
	out_keyWord.replace_string(")", "/)");
	return out_keyWord;
}

sqlite3* db_fetcher::init() {

	pfc::string8 foo_dbpath = CONF.db_dc_path;
	extract_native_path(foo_dbpath, foo_dbpath);

	do
	{
		m_lib_initialized = true;

		if (SQLITE_OK != (m_ret = sqlite3_initialize()))
		{
			m_lib_initialized = false;
			m_error_msg << "Failed to initialize DB library: " << m_ret;
			break;
		}

		if (SQLITE_OK != (m_ret = sqlite3_open_v2(foo_dbpath, &m_pDb, SQLITE_OPEN_READWRITE, NULL)))
		{
			m_error_msg << "Failed to open DB connection: " << m_ret << ". ";
			m_error_msg << sqlite3_errmsg(m_pDb);
			break;
		}

#ifdef DO_TRANSACTIONS
		sqlite3_exec(m_pDb, "BEGIN TRANSACTION;", NULL, NULL, NULL);
#endif  

	} while (false);

	return m_pDb;
}

void db_fetcher::close() {
	if (nullptr != m_pDb) sqlite3_close(m_pDb);
	//if (m_lib_initialized) sqlite3_shutdown();
}

void db_fetcher::fetch_search_artist(pfc::string8 artist_hint, bool exact_match, pfc::string8 &json, threaded_process_status& p_status, abort_callback& p_abort) {

	//https://api.discogs.com/database/search
	
	bool ok_db = true;
	int ret = 0;

	pfc::string8 error_msg;

	const size_t CLIMIT = 100;

	sqlite3_stmt* stmt_query = NULL;

	ok_db = (m_pDb != NULL && ret == SQLITE_OK);

	if (ok_db) {

		do {

			pfc::string8 query_artist;

			query_artist = "SELECT json(\'{\"pagination\": {\"page\": 1, \"pages\": 1, \"per_page\": 100, \"items\":\' || count(*) || \', \"urls\": {}}, \"results\": [\' || "
				"group_concat(json_object(\'id\', b.id, \'title\', b.name)) || \']}\') AS json "
				"FROM("
				"	SELECT * "
				"	FROM artist a "
				"	where a.name LIKE \"%Front 242%\""
				//"	where instr(a.name,\"Front 242\") > 0"
				"	ORDER BY a.id "
				"	LIMIT 111 "
				") "
				"b; ";

			bool breplace;

			if (exact_match) {
				//breplace = query_artist.replace_string("instr(a.name, \"Front 242\") > 0", "a.name = \"Front 242\"");
				breplace = query_artist.replace_string("LIKE \"%Front 242%\"", "= \"Front 242\"");
				breplace = query_artist.replace_string("Front 242", artist_hint);
			}
			else {
				//todo: sqliteEscapeLIKE(artist_hint).get_ptr());		
				breplace = query_artist.replace_string("Front 242", artist_hint); 
			}

			//query row limit
			breplace = query_artist.replace_string("111", pfc::toString(CLIMIT).get_ptr());


#ifdef _DEBUG
			log_msg(query_artist);
#endif

			p_status.set_item("Searching artist in local database...");

			// prepare
			if (SQLITE_OK != (ret = sqlite3_prepare_v2(m_pDb, query_artist, -1, &stmt_query, NULL)))
			{
				ok_db = false;
				error_msg << "Failed to prepare artist select: " << ret << ". ";
				error_msg << sqlite3_errmsg(m_pDb);
				break;
			}

			if (SQLITE_DONE/*SQLITE_ROW*/ != (ret = sqlite3_step(stmt_query))) // see documentation, this can return more values as success
			{
				if (SQLITE_ROW == ret) {

					const char* tmp_val = reinterpret_cast<const char*>(sqlite3_column_text(stmt_query, 0));
					if (tmp_val) {
						pfc::string8 colstr = tmp_val;
						json << colstr.get_ptr();
					}
					else {
						int debug = 1;
					}
					break;
				}
			}

		} while (false);
	}

	if (NULL != stmt_query) sqlite3_finalize(stmt_query);
	if (NULL != m_pDb) sqlite3_close(m_pDb);
	if (m_lib_initialized) sqlite3_shutdown();

	if (error_msg.get_length()) {
		foo_discogs_exception ex;
		ex << error_msg;
		throw ex;
	}
}

pfc::array_t<JSONParser_ptr> db_fetcher::versions_get_all_pages(pfc::string8 & id, pfc::string8 & secid, pfc::string8 params, const char* msg, threaded_process_status & p_status, abort_callback & p_abort) {

	pfc::array_t<JSONParser_ptr> results;

	//https://api.discogs.com/masters/1234/versions

	bool ok_db = true;
	int ret = 0;

	const size_t CLIMIT = 10;
	pfc::string8 json;

	sqlite3_stmt* stmt_query = NULL;

	ok_db = (m_pDb != NULL && ret == SQLITE_OK);
	ret = 0;

	if (ok_db) {
		do {

			pfc::string8 query_artist;

			query_artist = "SELECT json(\'{\"pagination\": {\"page\": \' || CAST (CAST (mycount AS INT) + 1 AS TEXT) || "
				"\', \"pages\": \"#pages\"\' ||/* yepe || */ \', \"per_page\": 10, \"items\": \"#items\"\' || "
				"/* count(*) || */ \', \"urls\": {}}, \"versions\": [\' || myarray || \']}\') AS json_result "
				"FROM "
				"(select mycount, GROUP_CONCAT(json(\'{\"id\": \' || recsrc.jsonid || \',\' || "
				"\'\"label\": \"\' || recsrc.jsonlabel || \'\",\' || "
				"\'\"country\": \"\' || recsrc.jsoncountry || \'\",\' || "
				"\'\"title\": \"\' || REPLACE(recsrc.jsontitle, \'\"\', \'\\\"\') || \'\",\' || "
				"\'\"major_formats\": \' || json_array(recsrc.jsonmf) || \',\' || "
				"\'\"catno\": \"\' || recsrc.jsoncatno || \'\",\' || "
				"\'\"released\": \"\' || quote(recsrc.jsonyear) || \'\",\' || "
				"\'\"db_total_tracks\": \"\' || recsrc.jsondb_totaltracks || \'\"\' || "
				"\'}\')) myarray "
				"FROM "
				"(select "
				"row_number() OVER(ORDER BY case when CAST(substr(r.released, 1, 4) AS INT) != 0 then CAST(substr(r.released, 1, 4) AS INT) else NULL end) / 4321 mycount, "
				"r.id jsonid, "
				"(select l.name from release_label rl inner join label l on l.id = rl.label_id where rl.release_id = r.id) jsonlabel, "
				"(select GROUP_CONCAT(kk.catno) from(select rl.catno catno from release_label rl where rl.release_id = r.id) kk) jsoncatno, "
				"r.country jsoncountry, "
				"r.title jsontitle, "
				"(select GROUP_CONCAT(bk.subname) from(select rf.name subname from release_format rf where rf.release_id = r.id GROUP BY rf.name) bk) jsonmf, "
				"r.released, "
				"(case when CAST(substr(r.released, 1, 4) AS INT) != 0 then CAST(substr(r.released, 1, 4) AS INT) else NULL end) jsonyear, "
				"tl.total_tracks jsondb_totaltracks "
				"FROM release as r "
				"inner join master m on m.id = r.master_id "
				"inner join tracklist tl on tl.release_id = r.id "
				"where m.id = 123456 ORDER BY jsonyear) recsrc GROUP BY mycount)";


			bool breplace;

			//artist hint
			breplace = query_artist.replace_string("123456", secid);
			//query row limit
			breplace = query_artist.replace_string("4321", pfc::toString(CLIMIT).get_ptr());


#ifdef _DEBUG
			log_msg(query_artist);
#endif

			p_status.set_item("Fetching versions from local database...");

			// prepare
			if (SQLITE_OK != (ret = sqlite3_prepare_v2(m_pDb, query_artist, -1, &stmt_query, NULL)))
			{
				ok_db = false;
				m_error_msg << "Failed to prepare select: " << ret << ". ";
				m_error_msg << sqlite3_errmsg(m_pDb);
				break;
			}

			// step into 1st row of data
			if (SQLITE_DONE/*SQLITE_ROW*/ != (ret = sqlite3_step(stmt_query))) // see documentation, this can return more values as success
			{
				while (SQLITE_ROW == ret) {

					size_t page = 0;
					const char* tmp_val = reinterpret_cast<const char*>(sqlite3_column_text(stmt_query, 0));

#ifdef _DEBUG
					log_msg(tmp_val);
#endif

					if (tmp_val) {
						pfc::string8 colstr = tmp_val;
						json << colstr.get_ptr();
						JSONParser_ptr jp = pfc::rcnew_t<JSONParser>(tmp_val);
						results.append_single(std::move(jp));

					}
					else {
						int debug = 1;
						break;
					}

					++page;
					ret = sqlite3_step(stmt_query);
				}
			}

		} while (false);
	}

	if (NULL != stmt_query) sqlite3_finalize(stmt_query);
	if (NULL != m_pDb) sqlite3_close(m_pDb);
	if (m_lib_initialized) sqlite3_shutdown();

	if (m_error_msg.get_length()) {
		foo_discogs_exception ex;
		ex << m_error_msg;
		throw ex;
	}

	return results;

}

void db_fetcher::get_artist(pfc::string8& id, pfc::string8& html, abort_callback& p_abort, const char* msg, threaded_process_status& p_status) {

	bool ok_db = true;
	int ret = 0;

	sqlite3_stmt* stmt_query = NULL;

	ok_db = (m_pDb != NULL && ret == SQLITE_OK);
	ret = 0;

	if (ok_db && id.get_length()) {
		do {

			pfc::string8 query_artist =
				"SELECT "
				" JSON_OBJECT ( "
				" \'name\', [lookup].[name], \'id\', [lookup].[id], "
				" \'resource_url\', \'https://api.discogs.com/\' || \'artists/\' || [lookup].[id], "
				" \'uri\', \'https://www.discogs.com/\' || \'artist/\' || [lookup].[id] || \'-\' || [REPLACE] ([lookup].[name], \' \', \'-\'), "
				" \'releases_url\', \'https://api.discogs.com/\' || \'artists/\' || [lookup].[id] || \'/releases\', "
				" \'images\', JSON (\'[]\'), "
				" \'realname\', [lookup].[realname], "
				" \'profile\', [lookup].[profile], "
				" \'urls\', JSON (\'[\' || IFNULL([lookup].[url],\'\') || \']\'), "
				" \'namevariations\', JSON (\'[\' || IFNULL([lookup].[anv],\'\') || \']\'), "
				" \'aliases\', JSON (\'[\' || IFNULL([lookup].[alias],\'\') || \']\'), "
				" \'members\', JSON (\'[\' || IFNULL([lookup].[members],\'\') || \']\'), "
				" \'groups\', JSON (\'[\' || IFNULL([lookup].[in_groups],\'\') || \']\'), "
				" \'data_quality\', [lookup].[data_quality]) "
				"  "
				" FROM   (SELECT  "
				"                [art].[name] [name],  "
				"                [art].[id] [id],  "
				"                [art].[realname] [realname],  "
				"                [art].[profile] [profile],  "
				"                [art].[data_quality] [data_quality],                "
				"                (SELECT GROUP_CONCAT (JSON_QUOTE ([au].[url])) "
				"                FROM   [artist_url] [au] "
				"                       INNER JOIN [artist] [a] ON [au].[artist_id] = [a].[id] "
				"                WHERE  [au].[artist_id] = 13735) [url],  "
				"                (SELECT GROUP_CONCAT (JSON_QUOTE ([an].[name])) "
				"                FROM   [artist_namevariation] [an] "
				"                       INNER JOIN [artist] [a] ON [an].[artist_id] = [a].[id] "
				"                WHERE  [an].[artist_id] = 13735) [anv],  "
				"                (SELECT GROUP_CONCAT (JSON_OBJECT (\'id\', [aa].[alias_id], \'name\', [aa].[alias_name])) "
				"                FROM   [artist_alias] [aa] "
				"                       INNER JOIN [artist] [a] ON [aa].[artist_id] = [a].[id] "
				"                WHERE  [aa].[artist_id] = 13735) [alias],  "
				"                (SELECT GROUP_CONCAT (JSON_OBJECT (\'id\', [gm].[member_id], \'name\', [gm].[member_name])) "
				"                FROM   [group_member] [gm] "
				"                       INNER JOIN [artist] [a] ON [gm].[artist_id] = [a].[id] "
				"                WHERE  [gm].[artist_id] = 13735) [members],  "
				"                (SELECT GROUP_CONCAT (JSON_OBJECT (\'id\', [gm].[artist_id], \'name\', [gm].[artist_name])) "
				"                FROM   [group_member] [gm] "
				"                       INNER JOIN [artist] [a] ON [gm].[member_id] = [a].[id] "
				"                WHERE  [gm].[member_id] = 13735) [in_groups] "
				"         FROM   [artist] [art] "
				"         WHERE  [art].[id] = 13735) lookup; ";

			bool breplace = query_artist.replace_string("13735", id);

#ifdef _DEBUG
			log_msg(query_artist);
#endif

			if (SQLITE_OK != (ret = sqlite3_prepare_v2(m_pDb, query_artist, -1, &stmt_query, NULL)))
			{
				ok_db = false;
				m_error_msg << "Failed to prepare insert: " << ret << ". ";
				m_error_msg << sqlite3_errmsg(m_pDb);
				break;
			}
			p_status.set_item(msg);

			if (SQLITE_DONE/*SQLITE_ROW*/ != (ret = sqlite3_step(stmt_query))) // see documentation, this can return more values as success
			{
				if (SQLITE_ROW == ret) {

					pfc::string8 strcol = reinterpret_cast<const char*>(sqlite3_column_text(stmt_query, 0));
					html = strcol.get_ptr();
					break;
				}

#ifdef _DEBUG				
				pfc::string8 msg(sqlite3_errmsg(m_pDb));
				log_msg(msg);
#endif
			}

		} while (false);
	}

	/*if (NULL != stmt_query) sqlite3_finalize(stmt_query);
	if (NULL != m_pDb) sqlite3_close(m_pDb);
	if (m_lib_initialized) sqlite3_shutdown();*/

	if (m_error_msg.get_length()) {
		foo_discogs_exception ex;
		ex << m_error_msg;
		throw ex;
	}

	pfc::string8 status(msg);

	p_status.set_item(status);

}

pfc::array_t<JSONParser_ptr> db_fetcher::releases_get_all_pages(pfc::string8& id, pfc::string8& secid, pfc::string8 params, abort_callback& p_abort, const char* msg, threaded_process_status& p_status) {

	pfc::array_t<JSONParser_ptr> uns_results;

	if (mtx.try_lock()) {

		pfc::array_t<JSONParser_ptr> results;

		//https://api.discogs.com/artists/1234/releases

		bool ok_db = true;
		int ret = 0;

		const size_t CLIMIT = 10;
		pfc::string8 json;

		sqlite3_stmt* stmt_query = nullptr;
		try {

			ok_db = (m_pDb != NULL && ret == SQLITE_OK);
			ret = 0;

			if (ok_db) {
				do {

					pfc::string8 query_artist;

					query_artist = "SELECT json(\'{\"pagination\": {\"page\": \' || CAST (CAST (mycount AS INT) + 1 AS TEXT) || \', \"pages\": \"#pages\"\' ||/* yepe || */ \', \"per_page\": 4321, \"items\": \"#items\"\' ||/* count(*) || */ \', \"urls\": {}}, \"releases\": [\' || myarray || \']}\') AS json_result"
						"	FROM("
						//			query_artist =
						"SELECT /*art rels all app*/group_concat(json_object(\'id\', jsonid, \'title\', title, \'type\', type, \'year\', jsonyear, \'role\', jsonrole, \'main_release\', jsonmainrelease, \'format\', jsonformat, \'label\', jsonlabel, \'catno\', jsoncatno, \'country\', jsoncountry) ) myarray,"
						"                  mycount,"
						"                  type jsontype,"
						"                  GROUP_CONCAT(jsonid),"
						"                  title,"
						"                  jsonmainrelease,"
						"                  jsonrole,"
						"                  jsonyear,"
						"                  jsoncountry,"
						"                  jsonformat,"
						"                  jsonlabel,"
						"                  jsoncatno"
						"             FROM ("
						"                      SELECT/* ARTIST RELEASES */ row_number() OVER/* jsonid */ /* (order by (case when r.master_id isnull then r.id else r.master_id end)) */(ORDER BY CASE WHEN r.master_id ISNULL THEN 'release' ELSE 'master' END,"
						"                             CASE WHEN m.year ISNULL THEN CAST (substr(released, 1, 4) AS INT) ELSE m.year END) / 4321 mycount,"
						"                             (CASE WHEN r.master_id ISNULL THEN ("
						"                                     SELECT GROUP_CONCAT(bk.subname) "
						"                                       FROM ("
						"                                                SELECT rf.name subname"
						"                                                  FROM release_format rf"
						"                                                 WHERE rf.release_id = r.id"
						"                                                 GROUP BY rf.name"
						"                                            )"
						"                                            bk"
						"                                 )"
						"                             ELSE NULL END) jsonformat,"
						"                             (CASE WHEN r.master_id ISNULL THEN ("
						"                                     SELECT l.name"
						"                                       FROM release_label rl"
						"                                            LEFT OUTER JOIN"
						"                                            label l ON l.id = rl.label_id"
						"                                      WHERE rl.release_id = r.id"
						"                                 )"
						"                             ELSE NULL END) jsonlabel,"
						"                             (CASE WHEN r.master_id ISNULL THEN ("
						"                                     SELECT rl.catno"
						"                                       FROM release_label rl"
						"                                      WHERE rl.release_id = r.id"
						"                                 )"
						"                             ELSE NULL END) jsoncatno,"
						"                             r.released,"
						"                             m.year,"
						"                             (CASE WHEN r.master_id ISNULL THEN r.id ELSE r.master_id END) jsonid,"
						"                             (CASE WHEN m.year ISNULL THEN NULLIF(CAST (substr(released, 1, 4) AS INT), 0) ELSE NULLIF(m.year, 0) END) jsonyear,"
						"                             /*'Main'*/ r.src AS jsonrole,"
						"                             (CASE WHEN r.master_id ISNULL THEN 'release' ELSE 'master' END) type,"
						"                             (CASE WHEN r.master_id ISNULL THEN NULL ELSE m.main_release_id END) jsonmainrelease,"
						"                             (CASE WHEN r.master_id ISNULL THEN r.country ELSE NULL END) jsoncountry,"
						"                             r.id,"
						"                             r.master_id,"
						"                             r.title "
						""
						"/* release source */"
						""
						""
						"/*                       FROM [release] AS r */"
						" FROM ("
						"select /*artist app*/GROUP_CONCAT(src),"
						"allrelroles.src, allrelroles.title, artist_id, allrelroles.release_id id, allrelroles.release_id, allrelroles.master_id,"
						"allrelroles.country, allrelroles.released "
						//"--(select lkr.master_id FROM release lkr WHERE lkr.id = allrelroles.release_id) master_id,"
						//"--(select lkr.country FROM release lkr WHERE lkr.id = allrelroles.release_id) country,"
						//"--(select lkr.released FROM release lkr WHERE lkr.id = allrelroles.release_id) released"
						""
						" from"
						"((SELECT /*release artist main*/\"daMain\" src, 1 z, r.id release_id, r.master_id, r.title, r.country, r.released, subq.tlrid tracklist_release_id, subq.tlid tracklist_id/*, subq.tlrtitle*/, subq.artist_id from release r"
						" inner join ("
						"SELECT tl.id tlrid, tl.id tlid/*, tlt.title tlrtitle*/, tlta.id artist_id from release tl"
						"      inner join release_artist ra on ra.release_id = tl.id "
						"                 inner join artist tlta on tlta.id = ra.artist_id"
						"                             where tlta.id = 1343431) subq on subq.tlrid = r.id"
						"                             group by src, release_id "
						" UNION "
						""
						"SELECT /*release artist app */\"dbApp\" src, 2 z, r.id release_id, r.master_id, r.title, r.country, r.released, subq.tlrid tracklist_release_id, subq.tlid tracklist_id/*, subq.tlrtitle*/, subq.artist_id from release r "
						" inner join ("
						"SELECT tl.id tlrid, tl.id tlid/*, tlt.title tlrtitle*/, tlta.id artist_id from release tl"
						"      inner join release_extraartist ra on ra.release_id = tl.id "
						"                 inner join artist tlta on tlta.id = ra.artist_id"
						"                             where tlta.id = 1343431) subq on subq.tlrid = r.id"
						"                             group by src, release_id "
						" UNION "
						""
						"SELECT /*tl track artist appearance*/\"dcTA\" src, 3 z, r.id release_id, r.master_id, r.title, r.country, r.released, subq.tlrid tracklist_release_id, subq.tlid tracklist_id/*, subq.tlrtitle*/, subq.artist_id from release r "
						" inner join ("
						"SELECT tl.release_id tlrid, tl.id tlid/*, tlt.title tlrtitle*/, tlta.artist_id artist_id from tracklist tl"
						//"      --inner join tracklist_track tlt on tlt.tracklist_id = tl.id "
						"                 inner join tracklist_track_artist tlta on tlta.tracklist_id = tl.id"
						"                             where tlta.artist_id = 1343431) subq on subq.tlrid = r.id"
						"                             group by src, release_id "
						" UNION "
						""
						"SELECT /*tl track extra artist appearance*/\"ddTEA\" src, 4 z, r.id release_id, r.master_id, r.title, r.country, r.released, subq.tlrid tracklist_release_id, subq.tlid tracklist_id/*, subq.tlrtitle*/, subq.artist_id from release r "
						" inner join ("
						"SELECT tl.release_id tlrid, tl.id tlid/*, tlt.title tlrtitle*/, tlta.artist_id artist_id from tracklist tl"
						//"      --inner join tracklist_track tlt on tlt.tracklist_id = tl.id "
						"                 inner join tracklist_track_extraartist tlta on tlta.tracklist_id = tl.id"
						"                             where tlta.artist_id = 1343431) subq on subq.tlrid = r.id"
						"                             group by src, release_id) allrelroles) "
						" group by release_id ORDER BY src"
						""
						""
						""
						") AS r"
						"                        "
						""
						""
						"    /* end release source */"
						"                             /* JOIN "
						"                             release_artist ra ON ra.release_id = r.id*/"
						"                             LEFT JOIN "
						"                             master m ON m.id = r.master_id"
						""
						"    /* end join to master */                             "
						""
						" /* end release source */"
						""
						"                       WHERE r.artist_id = 1343431"
						"                       GROUP BY IFNULL(r.master_id, -r.id) "
						"                       ORDER BY type,"
						"                                jsonyear"
						"                  )"
						"                  insider "
						"            /*GROUP BY mycount*/"
						//"";
						"	)"
						"	GROUP BY mycount ORDER BY jsonrole; ";

					bool breplace;
					//artist hint
					breplace = query_artist.replace_string("1343431", id);
					//query row limit
					breplace = query_artist.replace_string("4321", pfc::toString(CLIMIT).get_ptr());


#ifdef _DEBUG
					log_msg(query_artist);
#endif

					p_status.set_item("Fetching pages from local database...");

					// prepare
					if (SQLITE_OK != (ret = sqlite3_prepare_v2(m_pDb, query_artist, -1, &stmt_query, NULL)))
					{
						ok_db = false;
						m_error_msg << "Failed to prepare select: " << ret << ". ";
						m_error_msg << sqlite3_errmsg(m_pDb);
						break;
					}
					
					// see documentation, this can return more values as success
					if (SQLITE_DONE != (ret = sqlite3_step(stmt_query))) 
					{
						while (SQLITE_ROW == ret && !p_abort.is_aborting()) {

							size_t page = 0;

							const char* tmp_val = reinterpret_cast<const char*>(sqlite3_column_text(stmt_query, 0));

							if (tmp_val) {
#ifdef _DEBUG
								log_msg(tmp_val);
#endif
								JSONParser_ptr jp = pfc::rcnew_t<JSONParser>(tmp_val);
								results.append_single(std::move(jp));
							}
							else {
								int debug = 1;
								break;
							}

							++page;
							ret = sqlite3_step(stmt_query);
						}
					}

				} while (false);
			}
		}
		catch (...) {
			pfc::string8 msg;
			int debug = 1;
		}

		/*if (NULL != m_query) sqlite3_finalize(m_query);
		if (NULL != m_pDb) sqlite3_close(m_pDb);
		if (lib_initialized) sqlite3_shutdown();*/

		if (m_error_msg.get_length()) {
			foo_discogs_exception ex;
			ex << m_error_msg;
			throw ex;
		}

		uns_results = results;
		mtx.unlock();
	}
	
	return uns_results;
}

void db_fetcher::get_release(pfc::string8& id, pfc::string8& html, pfc::string8 params, abort_callback& p_abort, const char* msg, threaded_process_status& p_status) {

	//https://api.discogs.com/releases/1234
	
	sqlite3_stmt* stmt_query = NULL;

	bool ok_db = (m_lib_initialized && m_pDb != NULL && m_ret == SQLITE_OK);

	if (ok_db) {
		do {

			pfc::string8 query_release = " select "
				"  "
				" json_object( "
				"  "
				" \'artists\', json(\'[\' || IFNULL(obj_list_artists,json_object(\'name\', \'Various\', \'id\', 194)) || \']\'), "
				" \'extraartists\', json(\'[\' || IFNULL(obj_list_extraartists,\'\') || \']\'), "
				" \'id\', json(rcfields.id), "
				" \'title\', rcfields.title, "
				" \'notes\', rcfields.notes, "
				" \'year\', rcfields.year, "
				" \'released\', rcfields.released, "
				" \'country\', rcfields.country, "
				" \'genres\', rcfields.json_genres, "
				" \'styles\', rcfields.json_styles, "
				" \'master_id\', rcfields.master_id, "
				" \'formats\', rcfields.obj_list_formats, "
				" \'identifiers\', rcfields.obj_list_identifiers, "
				"  "
				" \'labels\', json(\'[\' || rcfields.obj_list_label || \']\'), "
				" \'tracklist\', rcfields.tracklist) "
				"  "
				" from  "
				"  "
				" /* start record fields */ "
				" (select "
				"  "
				" r.id, id, "
				"  "
				"  "
				" /* expose release fields to parent query */ "
				"  "
				"    r.title title,    "
				"    r.notes notes,    "
				"    r.master_id master_id, "
				"    r.country country, "
				"    r.released released, "
				"    CASE WHEN CAST (substr(released, 1, 4) AS INT) = 0 THEN NULL ELSE substr(released, 1, 4) END year, "
				"  "
				" /* end expose fields to parent query */ "
				"  "
				" /* start release obj fields */ "
				" /* start tracklist lookup */ "
				" (Select "
				" /* TRACK LIST OBJECT MEMBER VALUE USING QUERY DATA */ "
				" json(\'[\' || GROUP_CONCAT(json(rec_element)) || \']\') js_tracklist_obj_member "
				" from (  "
				" Select json_object(\'position\', src.position, \'duration\', src.duration, \'title\', src.title, "
				" \'type_\', CASE WHEN src.type = 0 THEN \"track\" ELSE \"Header\" END, "
				" \'artists\',json(\'[\' || json_tlta || \']\'), \'extraartists\',json(\'[\' || json_tltea || \']\')  ) rec_element "
				"  "
				" from ( "
				" /* TRACK LIST QUERY DATA & ARTISTS JSON ARRAYS */ "
				" SELECT /*tracklist*/ tl.id, tlt.sequence, tlt.position, tlt.type, tlt.title, tlt.duration,  "
				" (select IFNULL(j_extraartists.concat, \'\') from (SELECT  "
				"     GROUP_CONCAT(json_object(\'id\', lktltea.artist_id, \'name\', a.name, \'role\', lktltea.role)) concat "
				"     from tracklist_track_extraartist lktltea     "
				"     inner join artist a on a.id = lktltea.artist_id "
				"     where lktltea.tracklist_id = tl.id and lktltea.tracklist_sequence = tlt.sequence) j_extraartists) json_tltea, "
				" (select IFNULL(j_artists.concat, \'\') from (SELECT  "
				"     GROUP_CONCAT(json_object(\'id\', lktlta.artist_id, \'name\', a.name, \'role\', lktlta.role)) concat "
				"     from tracklist_track_artist lktlta     "
				"     inner join artist a on a.id = lktlta.artist_id "
				"     where lktlta.tracklist_id = tl.id and lktlta.tracklist_sequence = tlt.sequence) j_artists) json_tlta "
				" from tracklist tl "
				" inner join tracklist_track tlt on tlt.tracklist_id = tl.id "
				" left outer join tracklist_track_extraartist tltea  "
				"     on tltea.tracklist_id = tlt.tracklist_id "
				" inner join release r on r.id = tl.release_id "
				" where r.id = 3655518 group by sequence) src) js_build_tracklist) tracklist, "
				" /* 1) end tracklist query data lookup *//* 2) end object member using query data *//* 3) end outer query tracklist field*/ "
				"  "
				" /*release genres*/ "
				" (select json(\'[\' || GROUP_CONCAT(json_quote(lkg.name)) || \']\' ) kk from release_genre rg "
				"     inner join release lkr on lkr.id = rg.release_id "
				"     inner join genre lkg on lkg.id = rg.genre_id "
				"     where lkr.id = r.id) json_genres, "
				" /* end genre */ "
				"  "
				" /*release styles*/ "
				" (select json(\'[\' || GROUP_CONCAT(json_quote(lks.name)) || \']\' ) kk from release_style rs "
				"     inner join release lkr on lkr.id = rs.release_id "
				"     inner join style lks on lks.id = rs.style_id "
				"     where lkr.id = r.id) json_styles, "
				" /* end styles */ "
				"  "
				" /*release labels FOR LIST OF OBJECT FOLLOW THIS PROCEDURE, ADD OBJECT WITH GROUP CONCAT AND ADD BRACKET WHEN INSERTING IT IN INSIDE THE CONTAINER OBJECT */"
				" (select  GROUP_CONCAT(json_object(\'name\', lkl.name, \'catno\', rl.catno, \'id\', rl.label_id, \'entity_type_name\', \"Label\" )) kk from release_label rl "
				"     inner join release lkr on lkr.id = rl.release_id "
				"     inner join label lkl on lkl.id = rl.label_id "
				"     where lkr.id = r.id) obj_list_label, "
				"  "
				" /*release identifiers*/ "
				" (select  json(\'[\' || GROUP_CONCAT(json(json_object(\'type\', ri.type, \'value\', ri.value, \'description\', ri.description)))|| \']\') kk from release_identifier ri "
				"     inner join release lkr on lkr.id = ri.release_id "
				"  "
				" where lkr.id = r.id) obj_list_identifiers, "
				"  "
				" /*release artists*/ "
				"  "
				" (select GROUP_CONCAT(json_object(\'name\', lka.name, \'id\', lka.id)) kk from release_artist ra "
				"     inner join release lkr on lkr.id = ra.release_id "
				"     inner join artist lka on lka.id = ra.artist_id "
				"     where lkr.id = r.id) obj_list_artists, "
				"  "
				" /*release extraartists*/ "
				"  "
				" (select GROUP_CONCAT(json(json_object(\'name\', lka.name, \'id\', lka.id, \'role\', rea.role))) kk from release_extraartist rea "
				"     inner join release lkr on lkr.id = rea.release_id "
				"     inner join artist lka on lka.id = rea.artist_id "
				"     where lkr.id = r.id) obj_list_extraartists, "
				"  "
				" /*release formats*/ "
				" (select json(\'[\' || GROUP_CONCAT(json(json_object(\'name\', rf.name, \'qty\', rf.qty /*,\'descriptions\', GROUP_CONCAT(lkfd.description)*/))) || \']\') kk from release_format rf "
				"     inner join release lkr on lkr.id = rf.release_id "
				"     /*left outer join release_format_descriptions lkf on lkf.id = rf.format_id*/ "
				"     where lkr.id = r.id GROUP BY rf.id) obj_list_formats , "
				"  "
				"  "
				"  "
				" \'true\' ending "
				" from release r where r.id = 3655518) rcfields "
				" /* end record fields */ ";

			bool breplace = query_release.replace_string("3655518", id);

#ifdef _DEBUG
			log_msg(query_release);
#endif

			p_status.set_item("Fetching release from local database...");

			// prepare
			if (SQLITE_OK != (m_ret = sqlite3_prepare_v2(m_pDb, query_release, -1, &stmt_query, NULL)))
			{
				ok_db = false;
				m_error_msg << "Failed to retrieve local db release (Err." << m_ret << ") ";
				m_error_msg << sqlite3_errmsg(m_pDb);
				break;
			}

			// step into row
			if (SQLITE_ROW == (m_ret = sqlite3_step(stmt_query)))
			{
				while (SQLITE_ROW == m_ret/* && !p_abort.is_aborting()*/) {

					const char* tmp_val = reinterpret_cast<const char*>(sqlite3_column_text(stmt_query, 0));
#ifdef DEBUG
					log_msg(tmp_val);
#endif

					if (tmp_val) {
						html << tmp_val;
					}
					else {
						break;
					}
					m_ret = sqlite3_step(stmt_query);
				}

				if (m_ret == SQLITE_DONE) break;
			}

			if (SQLITE_DONE != m_ret) {
				ok_db = false;
				m_error_msg << "Failed retrieving local db release (Err." << m_ret << ") ";
				m_error_msg << sqlite3_errmsg(m_pDb);
				break;
			}

		} while (false);
	}

	if (NULL != stmt_query) sqlite3_finalize(stmt_query);
	if (NULL != m_pDb) sqlite3_close(m_pDb);
	if (m_lib_initialized) sqlite3_shutdown();

	if (m_error_msg.get_length()) {
		foo_discogs_exception ex;
		ex << m_error_msg;
		throw ex;
	}
}
