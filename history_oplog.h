#pragma once

#include "utils.h"
#include "utils_db.h"

class history_oplog {

public:

	bool init(bool enabled, size_t max_items);
	bool do_history_menu(oplog_type optype, HWND hwndCtrl, pfc::string8& out);

	//serves config dialog
	void zap_vhistory();

	//from
	// process release callback on success
	// search_artist_process_callback on success
	// get_artist_process_callback

	bool add_history_row(oplog_type optype, rppair row);

private:

	bool m_enabled = false;
	size_t m_max_items = 0;

	vppair m_vres_release_history;
	vppair m_vres_artist_history;
	vppair m_vres_filter_history;

	vppair& get_history(oplog_type optype) {
		if (optype == oplog_type::artist)		return m_vres_artist_history;
		else if (optype == oplog_type::release)	return m_vres_release_history;
		else if (optype == oplog_type::filter)	return m_vres_filter_history;
		else return m_vres_filter_history;
	}

	pfc::string8 get_row_key(oplog_type optype, const rppair row) {
		if (optype == oplog_type::artist)		return row.second.first;
		else if (optype == oplog_type::release)	return row.first.first;
		else if (optype == oplog_type::filter)	return row.first.first;
		return "";
	}

	pfc::string8 get_row_val(oplog_type optype, const rppair row) {
		if (optype == oplog_type::artist)		return row.second.second;
		else if (optype == oplog_type::release)	return row.first.first;
		else if (optype == oplog_type::filter)	return row.first.first;
		return "";
	}

	// menu

	pfc::string8 get_menu_label(oplog_type optype, rppair* row) {
		if (optype == oplog_type::artist)		return ((PFC_string_formatter() << row->second.first << "\t" << EscapeWin(row->second.second)).c_str());
		else if (optype == oplog_type::release)	return ((PFC_string_formatter() << row->first.first << "\t" << row->first.second).c_str());
		else if (optype == oplog_type::filter)	return EscapeWin(row->first.first);
		return "";
	}
};