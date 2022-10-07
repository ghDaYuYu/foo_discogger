#include "stdafx.h"

#include "utils_db.h"
#include "history_oplog.h"

bool history_oplog::init(bool enabled, size_t max_items) {

	if (m_enabled == enabled && m_max_items == max_items) return false;

	m_enabled = enabled;
	m_max_items = max_items;

	zap_vhistory();

	sqldb db;

	std::map<oplog_type, vppair*> out_history = {
		{oplog_type::release, &m_vres_release_history},
		{oplog_type::artist, &m_vres_artist_history},
		{oplog_type::filter, &m_vres_filter_history}
	};

	size_t inc = db.recharge_history(kcmdHistoryWashup, max_items, out_history);

	return inc != pfc_infinite;

}

//serves config dialog
void history_oplog::zap_vhistory() {

	m_vres_release_history.clear();
	m_vres_artist_history.clear();
	m_vres_filter_history.clear();
}

bool history_oplog::add_history_row(oplog_type optype, rppair row) {

	if (!get_row_key(optype, row).get_length()) return false;

	vppair& vh = get_history(optype);
	vppair::iterator fit;

	fit = std::find_if(vh.begin(), vh.end(),
		[=](const rppair& w) {
			return get_row_key(optype, w).equals(get_row_key(optype, row));
		});

	if (!vh.size() || fit == vh.end()) {

		vh.insert(vh.begin(), row);
		return true;
	}
	else {
		std::iter_swap(vh.begin(), fit);
	}
	return false;
}

bool history_oplog::do_history_menu(oplog_type optype, HWND hwndCtrl, pfc::string8 &out) {

	vppair& vophistory = get_history(optype);
	if (!vophistory.size()) return false;

	CRect clientRect;
	::GetWindowRect(hwndCtrl, &clientRect);
	POINT pt;
	pt.x = clientRect.left;
	pt.y = clientRect.bottom;

	//build menu

	enum { MENU_FIRST_ITEM = 1 };
	HMENU hMenu = CreatePopupMenu();

	for (auto walk_it = vophistory.begin(); walk_it != vophistory.end(); ++walk_it) {

		const pfc::string8 label = get_menu_label(optype, walk_it._Ptr);

		const pfc::stringcvt::string_os_from_utf8 os_str(label);
		size_t item_id = MENU_FIRST_ITEM + std::distance(vophistory.begin(), walk_it);

		AppendMenu(hMenu, MF_STRING, item_id, os_str);
	}

	//display menu

	int cmd = TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD, pt.x, pt.y, 0, hwndCtrl, NULL);
	DestroyMenu(hMenu);

	//return value

	if (cmd) {

		rppair row_h = vophistory.at(--cmd);
		out = get_row_val(optype, row_h);
		return out.get_length();
	}
	return false;
}