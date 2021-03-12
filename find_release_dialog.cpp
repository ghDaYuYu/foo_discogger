#include "stdafx.h"

#include "find_release_dialog.h"
#include "configuration_dialog.h"
#include "tasks.h"
#include "multiformat.h"
#include "sdk_helpers.h"

#include "resource.h"
#include "utils.h"

namespace listview_helper {

	unsigned insert_column2(HWND p_listview, unsigned p_index, const char* p_name, unsigned p_width_dlu, int fmt)
	{
		pfc::stringcvt::string_os_from_utf8 os_string_temp(p_name);
		RECT rect = { 0, 0, static_cast<LONG>(p_width_dlu), 0 };
		MapDialogRect(GetParent(p_listview), &rect);
		LVCOLUMNW data = {};
		data.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
		data.fmt = fmt;
		data.cx = rect.right;
		data.pszText = const_cast<TCHAR*>(os_string_temp.get_ptr());
		data.iSubItem = p_index;
		LRESULT ret = uSendMessage(p_listview, LVM_INSERTCOLUMNW, p_index, (LPARAM)&data);
		if (ret < 0) return ~0;
		else return (unsigned)ret;
	}

	
	unsigned frel_insert_column2(HWND p_listview, unsigned p_index, const char* p_name, unsigned p_width_dlu, int fmt)
	{
		int colcount = ListView_GetColumnCount(p_listview);

		pfc::stringcvt::string_os_from_utf8 os_string_temp(p_name);
		RECT rect = { 0, 0, static_cast<LONG>(p_width_dlu), 0 };
		MapDialogRect(GetParent(p_listview), &rect);
		LVCOLUMNW data = {};
		data.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT | LVCF_SUBITEM;
		data.fmt = fmt;
		data.cx = rect.right;
		data.pszText = const_cast<TCHAR*>(os_string_temp.get_ptr());
		data.iSubItem = colcount/*p_index*/;
		LRESULT ret = uSendMessage(p_listview, LVM_INSERTCOLUMNW, colcount/*p_index*/, (LPARAM)&data);
		if (ret < 0) return ~0;
		else return (unsigned)ret;
	}

	bool frel_insert_item_subitem(HWND p_listview, unsigned p_index, unsigned p_subitem, const char* p_name, LPARAM p_param)
	{
		return set_item_text(p_listview, p_index, p_subitem, p_name);
	}
}

struct mounted_param {
	int master_ndx;
	int release_ndx;
	bool bmaster;
	bool brelease;
};

//compensate gripp offset
//TODO: dpi check
struct rzgripp {
	int x; 
	int y;
	bool grip;
} mygripp { 22, 56, true};

struct cfgcol {
	int ndx = 0;
	int id = 0;
	int icol = 0;
	pfc::string8 name = "";
	pfc::string8 titleformat = "";
	float width = 0.0f;
	int celltype = 1;
	int custtype = 0;
	bool factory = false;
	bool default = false;
	bool enabled = true;
	bool defuser = false;
};

using cfgColMap = std::map<std::int32_t, cfgcol>;

const int TOTAL_DEF_COLS = 10;
 
cfgcol column_configs[TOTAL_DEF_COLS] = {
	//#, ndx, icol, name, tf, width, celltype, factory, default, enabled, defuser
	{1, 1, 0,"Id", "%RELEASE_ID%", 30.0f, 0x0000, 0, true, false, false, false}, //min 17.5f
	{2, 2, 0,"Title", "%RELEASE_TITLE%", 120.0f, 0x0000, 0, true, false, false, false},
	{3, 3, 0,"Label", "%RELEASE_SEARCH_LABELS%", 50.0f, 0x0000, 0, true, false, false, false},
	{4, 4, 0,"Major F", "%RELEASE_SEARCH_MAJOR_FORMATS%", 60.0f, 0x0000, 0, true, false, false, false},
	{5, 5, 0,"Format", "%RELEASE_SEARCH_FORMATS%", 100.0f, 0x0000, 0, true, false, false, false},
	{6, 6, 0,"CatNo", "%RELEASE_SEARCH_CATNOS%", 20.0f, 0x0000, 0, true,false, false, false},
	{7, 7, 0,"Year", "%RELEASE_YEAR%", 20.0f, 0x0000, 0, true, false, false, false},
	{8, 8, 0,"---Released", "%length%", 20.0f, 0x0000, 0, false, false, false, false},
	{9, 9, 0,"Country", "%RELEASE_COUNTRY%", 50.0f, 0x0000, 0, true, false, false, false},
	{100, 10, 0,"Master/Release", "", 200.0f, 0x0000, 0, true, true, false, false},
};

struct find_lv_config {
	bool autoscale = false;
	cfgColMap colmap = {};
	uint32_t flags = 0;
} cfg_lv;

static void defaultCfgCols(find_lv_config& cfg_out) {
	cfg_out.autoscale = false;
	cfg_out.colmap.clear();
	for (int it = 0; it < PFC_TABSIZE(column_configs); it++) {
		if (!column_configs[it].name.has_prefix("---")) {
			cfgcol tmp_cfgcol{
				column_configs[it].id,
				column_configs[it].ndx,
				column_configs[it].icol,
				column_configs[it].name,
				column_configs[it].titleformat,
				column_configs[it].width,
				column_configs[it].celltype,
				column_configs[it].custtype,
				column_configs[it].factory,
				column_configs[it].default,
				column_configs[it].enabled,
				column_configs[it].defuser,
			};
			cfg_out.colmap.emplace(cfg_out.colmap.size(), tmp_cfgcol);
		}
	}
}

void CFindReleaseDialog::get_mounted_param(mounted_param& pm, LPARAM lparam) {

	pm.master_ndx = HIWORD(lparam);
	pm.release_ndx = LOWORD(lparam);
	pm.bmaster = (pm.master_ndx != 9999);
	pm.brelease = (pm.release_ndx != 9999);
}

mounted_param CFindReleaseDialog::get_mounted_param(LPARAM lparam) {
	mounted_param mpres;
	mpres.master_ndx = HIWORD(lparam);
	mpres.release_ndx = LOWORD(lparam);
	mpres.bmaster = (mpres.master_ndx != 9999);
	mpres.brelease = (mpres.release_ndx != 9999);
	return mpres;
}

inline void CFindReleaseDialog::load_size() {

	int width = conf.find_release_dialog_width;
	int height = conf.find_release_dialog_height;

	CRect offset;	
	client_center_offset(core_api::get_main_window(), offset, width, height);

	if (width != 0 && height != 0) {
		SetWindowPos(core_api::get_main_window(), offset.left, offset.top, width + mygripp.x, height + mygripp.y, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	}

	//columns

	if (false /*TODO: columns persistance*/) {
		//ListView_SetColumnWidth(GetDlgItem(IDC_FIND_RELEASE_LIST), 0, conf.find_dialog_col1_width);
		//ListView_SetColumnWidth(GetDlgItem(IDC_FIND_RELEASE_LIST), 1, conf.find_dialog_col2_width);
	}
	else {
		CRect rcCli;
		::GetClientRect(release_list, &rcCli);
		int width = rcCli.Width();

		ListView_SetColumnWidth(release_list, 0, width);

	}
}

void CFindReleaseDialog::pushcfg() {
	bool conf_changed = build_current_cfg();
	if (conf_changed) {
		CONF.save(new_conf::ConfFilter::CONF_FILTER_FIND, conf);
		CONF.load();
	}
}

inline bool CFindReleaseDialog::build_current_cfg() {
	bool bres = false;

	//check global settings
	conf.display_exact_matches = IsDlgButtonChecked(IDC_ONLY_EXACT_MATCHES_CHECK) != 0;

	bool local_display_exact_matches = IsDlgButtonChecked(IDC_ONLY_EXACT_MATCHES_CHECK);
	if (CONF.display_exact_matches != conf.display_exact_matches &&
		conf.display_exact_matches != local_display_exact_matches) {

		conf.display_exact_matches = local_display_exact_matches;
		bres = bres || true;
	}

	//... nothing here	

	//get current ui dimensions

	CRect rcDlg;
	GetClientRect(&rcDlg);

	int dlgwidth = rcDlg.Width();
	int dlgheight = rcDlg.Height();

	//dlg size

	if (dlgheight != conf.find_release_dialog_height || dlgwidth != conf.find_release_dialog_width) {
		conf.find_release_dialog_width = dlgwidth;
		conf.find_release_dialog_height = dlgheight;
		bres = bres || true;
	}
	
	return bres;
}

LRESULT CFindReleaseDialog::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	pushcfg();
	return 0;
}

LRESULT CFindReleaseDialog::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	pfc::string8 text;
	uGetWindowText(release_url_edit, text);
	pfc::string8 release_id = text.get_ptr();
	release_id = trim(release_id);
	extract_id_from_url(release_id);
	extract_release_from_link(release_id);
	if (release_id.get_length() != 0) {
		on_write_tags(release_id);
	}
	return TRUE;
}

LRESULT CFindReleaseDialog::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	destroy();
	return TRUE;
}

CFindReleaseDialog::~CFindReleaseDialog() {
	g_discogs->find_release_dialog = nullptr;
}

LRESULT CFindReleaseDialog::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	
	conf = CONF;

	defaultCfgCols(cfg_lv);

	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(icex);
	icex.dwICC = IDC_RELEASE_LIST;
	InitCommonControlsEx(&icex);

	filter_edit = GetDlgItem(IDC_FILTER_EDIT);
	search_edit = GetDlgItem(IDC_SEARCH_TEXT);
	artist_list = GetDlgItem(IDC_ARTIST_LIST);
	release_list = GetDlgItem(IDC_RELEASE_LIST);
	//uSendMessage(release_list, LB_SETHORIZONTALEXTENT, 600, 0);
	release_url_edit = GetDlgItem(IDC_RELEASE_URL_TEXT);

	ListView_SetExtendedListViewStyle(release_list, LVS_EX_FULLROWSELECT);
	ListView_SetExtendedListViewStyleEx(release_list, LVS_EX_COLUMNOVERFLOW | LVS_EX_FLATSB | LVS_EX_HEADERDRAGDROP | LVS_EX_GRIDLINES | LVS_EX_AUTOSIZECOLUMNS | LVS_EX_COLUMNSNAPPOINTS | LVS_EX_DOUBLEBUFFER, LVS_EX_COLUMNOVERFLOW | LVS_EX_FLATSB | LVS_EX_HEADERDRAGDROP | LVS_EX_GRIDLINES | LVS_EX_AUTOSIZECOLUMNS | LVS_EX_COLUMNSNAPPOINTS | LVS_EX_DOUBLEBUFFER);
	
	reset_default_columns(true);

	::SetFocus(search_edit);

	CheckDlgButton(IDC_ONLY_EXACT_MATCHES_CHECK, conf.display_exact_matches);

	file_info_impl finfo;
	metadb_handle_ptr item = items[0];
	item->get_info(finfo);

	bool release_id_unknown = true;
	if (g_discogs->file_info_get_tag(item, finfo, TAG_RELEASE_ID, release_id)) {
		uSetWindowText(release_url_edit, release_id.get_ptr());
		release_id_unknown = false;
	}

	// TODO: make formatting strings out of these
	const char *artist = finfo.meta_get("ARTIST", 0);
	if (!artist) {
		artist = finfo.meta_get("ALBUM ARTIST", 0);
	}
	if (artist) {
		uSetWindowText(search_edit, artist);
		if ((release_id_unknown || dropId) && conf.enable_autosearch && artist[0] != '\0') {
			search_artist();
		}
	}
	else {
		uSetWindowText(search_edit, "");
	}
	uSendMessage(m_hWnd, DM_SETDEFID, (WPARAM)dropId || release_id_unknown ? IDC_SEARCH_BUTTON : IDC_PROCESS_RELEASE_BUTTON, 0);

	const char *album = finfo.meta_get("ALBUM", 0);
	if (album && strcmp(album, "") != 0) {
		uSetWindowText(filter_edit, album);
	}

	DlgResize_Init(true, true);
	load_size();

	modeless_dialog_manager::g_add(m_hWnd);
	show();

	if (!g_discogs->gave_oauth_warning && (!conf.oauth_token.get_length() || !conf.oauth_token_secret.get_length())) {
		g_discogs->gave_oauth_warning = true;
		if (!g_discogs->configuration_dialog) {	
			static_api_ptr_t<ui_control>()->show_preferences(guid_pref_page);
		}
		g_discogs->configuration_dialog->show_oauth_msg(
				"OAuth is required to use the Discogs API.\n Please configure OAuth.",
				true);

		::SetFocus(g_discogs->configuration_dialog->m_hWnd);
		
	}
	else if ((!release_id_unknown && !dropId) && conf.skip_find_release_dialog_if_ided) {
		on_write_tags(release_id);
	}
	return FALSE;
}

void CFindReleaseDialog::fill_artist_list() {
	uSendMessage(artist_list, LB_RESETCONTENT, 0, 0);
	ListView_DeleteAllItems(release_list);

	find_release_artists = artist_exact_matches;

	bool exact_matches = IsDlgButtonChecked(IDC_ONLY_EXACT_MATCHES_CHECK) != 0;

	if (!exact_matches) {
		find_release_artists.append(artist_other_matches);
	}

	int artists_index = 0, list_index = 0;
	for (size_t i = 0; i < find_release_artists.get_size(); i++) {
		pfc::string8 name;
		name << find_release_artists[i]->name;
		uSendMessageText(artist_list, LB_ADDSTRING, 0, name.get_ptr());
		uSendMessage(artist_list, LB_SETITEMDATA, list_index, (LPARAM)artists_index);
		artists_index++;
		list_index++;
	}
	if (find_release_artists.get_size()) {
		uSendMessage(artist_list, LB_SETCURSEL, 0, 0);
		get_selected_artist_releases();
	}
}

void CFindReleaseDialog::on_get_artist_done(Artist_ptr &artist) {
	find_release_artist = artist;
	hook = std::make_shared<titleformat_hook_impl_multiformat>(&find_release_artist);
	pfc::string8 text;
	uGetWindowText(filter_edit, text);
	filter_releases(text);
}

void CFindReleaseDialog::enable(bool is_enabled) {
	::uEnableWindow(GetDlgItem(IDC_PROCESS_RELEASE_BUTTON), is_enabled);
	::uEnableWindow(GetDlgItem(IDCANCEL), is_enabled);
	::uEnableWindow(GetDlgItem(IDC_SEARCH_BUTTON), is_enabled);
}

void CFindReleaseDialog::OnFinalMessage(HWND /*hWnd*/) {
	modeless_dialog_manager::g_remove(m_hWnd);
	delete this;
}

void CFindReleaseDialog::on_search_artist_done(pfc::array_t<Artist_ptr> &p_artist_exact_matches, const pfc::array_t<Artist_ptr> &p_artist_other_matches) {
	artist_exact_matches = p_artist_exact_matches;
	artist_other_matches = p_artist_other_matches;
	if (!artist_exact_matches.get_size() && artist_other_matches.get_size()) {
		CheckDlgButton(IDC_ONLY_EXACT_MATCHES_CHECK, false);
	}
	fill_artist_list();
}

void CFindReleaseDialog::get_item_text(HWND list, int list_index, int col, pfc::string8& out) {
	LVITEM lvitem = { 0 };
	const int strsize = 1024;
	wchar_t str[strsize + 1] = { 0 };
	lvitem.cchTextMax = strsize;
	lvitem.mask = LVIF_TEXT;
	lvitem.iItem = list_index;
	lvitem.iSubItem = col;
	lvitem.pszText = str;
	int res = ListView_GetItem(list, &lvitem);

	pfc::stringcvt::string_utf8_from_os os_to_utf8(lvitem.pszText);
	out.set_string(os_to_utf8);
}

void CFindReleaseDialog::get_item_param(HWND list, int list_index, int col, LPARAM& out_p) {
	
	LVITEM lvitem = { 0 };
	lvitem.mask = LVIF_PARAM;
	lvitem.iItem = list_index;
	lvitem.iSubItem = col;
	int res = ListView_GetItem(list, &lvitem);

	out_p = MAKELPARAM(LOWORD(lvitem.lParam), HIWORD(lvitem.lParam));
	//out_p = lvitem.lParam;
}

void CFindReleaseDialog::insert_item(const pfc::string8 &item, int list_index, int item_data) {
	HWND release_list = uGetDlgItem(IDC_RELEASE_LIST);
	
	listview_helper::insert_item(release_list, list_index, item.c_str(), (LPARAM)item_data);
}

int CFindReleaseDialog::insert_item_row(const std::list<std::pair<int, pfc::string8>>coldata, int list_index, int item_data) {
	int iret = 0;
	HWND release_list = uGetDlgItem(IDC_RELEASE_LIST);
	std::list < std::pair<int, pfc::string8>>::iterator it;

	for (auto it = coldata.begin(); it != coldata.end(); it++) {
		if (it->first == 0)
			insert_item(it->second, list_index, item_data);
		else
			listview_helper::frel_insert_item_subitem(release_list, list_index, it->first, it->second, item_data);
		iret++;
	}
	return iret;
}

void CFindReleaseDialog::run_hook_columns(std::list<std::pair<int, pfc::string8>>&out_col_data_list, runHook ismaster, runHook issubrelease) {
	
	formatting_script column_script;
	pfc::string8 buffer;

	out_col_data_list.clear();
	bool runhook = true;

	for (unsigned int i = 0; i < vec_icol_subitems.size(); i++)
	{
		cfgcol walk_cfg = cfg_lv.colmap.at(vec_icol_subitems[i].first);

		if (ismaster == runHook::rhMaster) {
			if (!stricmp_utf8(walk_cfg.name, "Title")) {
				column_script = "'[master] '$join(%MASTER_RELEASE_TITLE%)";
			}
			else if (!stricmp_utf8(walk_cfg.name, "Year")) {
				column_script = "%MASTER_RELEASE_YEAR%";
			}
			else if (!stricmp_utf8(walk_cfg.name, "Id")) {
				column_script = walk_cfg.titleformat;
			}
			else if (!stricmp_utf8(walk_cfg.name, "Master/Release")) {
				column_script = conf.search_master_format_string;
			}
			else
			{
				runhook = false;
			}
		}
		else {
			if (!stricmp_utf8(walk_cfg.name, "Master/Release")) {
				if (issubrelease == runHook::rhSubrelease) {
					column_script = conf.search_master_sub_format_string;
				}
				else {
					column_script = conf.search_release_format_string;
				}
			}
			else {
				if ((issubrelease == runHook::rhSubrelease) 
					&& !stricmp_utf8(walk_cfg.name, "Title")) {
					buffer.set_string("'  '");
				}
				else
					column_script = walk_cfg.titleformat;
			}
		}

		pfc::string8 column_content;
		if (runhook) {
			column_script->run_hook(location, &info, hook.get(), column_content, nullptr);
			out_col_data_list.emplace_back(std::make_pair(walk_cfg.icol, column_content));
		}
	}
}

bool sortByVal(const std::pair<int, int>& a, const std::pair<int, int>& b)
{
	if (a.second == b.second)
		return a.first > b.first;
	return a.second < b.second;
}

void CFindReleaseDialog::update_sorted_icol_map(bool reset) {

	cfgColMap& colvals = cfg_lv.colmap;
	cfgColMap::iterator it;
	// copy key-value pairs from map to vector
	vec_icol_subitems.clear();
	for (it = colvals.begin(); it != colvals.end(); it++)
	{
		if (reset && it->second.default)
			vec_icol_subitems.push_back(std::make_pair(it->first, it->second.icol));
		else if (it->second.enabled)
			vec_icol_subitems.push_back(std::make_pair(it->first, it->second.icol));
	}
	// sort the vector by icol
	std::sort(vec_icol_subitems.begin(), vec_icol_subitems.end(), sortByVal);
}

void CFindReleaseDialog::reset_default_columns(bool breset) {
	
	cfgColMap::iterator it;

	for (int i = 0; i < ListView_GetColumnCount(release_list); i++) {
		ListView_DeleteColumn(release_list, 0);
	}

	//totcol = ListView_GetColumnCount(release_list);
	update_sorted_icol_map(breset /*reset to defaults*/);
	
	//createcols
	for (unsigned int i = 0; i < vec_icol_subitems.size(); i++)
	{
		cfgcol walk_cfg = cfg_lv.colmap.at(vec_icol_subitems[i].first);

		// INSERT

		int icol = listview_helper::frel_insert_column2(release_list, walk_cfg.ndx,
			walk_cfg.name, (unsigned int)walk_cfg.width, LVCFMT_LEFT);
		
		//write state to columsn (enabled)
		cfg_lv.colmap.at(vec_icol_subitems[i].first).enabled = true;
	}

}

int assist_dropid_marker(int releaseid, const pfc::string8 currentid, int currentndx, pfc::string8& out) {
	int ires = -1;
	if (releaseid == std::atoi(currentid)) {
		ires = currentndx;
		out.add_string(" <");
	}
	return ires;
}

void CFindReleaseDialog::filter_releases(const pfc::string8 &filter) {
	ListView_DeleteAllItems(release_list);

	if (!find_release_artist) {
		return;
	}

	//Write Tags (Update ReleaseId) menu option
	//TODO: recheck first and second run
	//first run - getting the initial master
	//second run to expand

	int drop_release_id;
	int list_index_dropId = -1;
	
	pfc::string8 tmp_release_id; bool release_id_unknown = true;
	file_info_impl finfo;	metadb_handle_ptr item = items[0];
	item->get_info(finfo);

	if (g_discogs->file_info_get_tag(item, finfo, TAG_RELEASE_ID, tmp_release_id)) {
		drop_release_id = std::atoi(tmp_release_id.get_ptr());
		release_id_unknown = false;
	}

	bool assist_dropId = (!release_id_unknown && dropId);
	//..

	
	int list_index = 0;
	int master_index = 0, release_index = 0;
	int item_data;
	pfc::array_t<pfc::string8> filter_words;
	tokenize(filter, " ", filter_words, true);
	pfc::array_t<pfc::string> filter_words_lowercase;
	for (size_t i = 0; i < filter_words.get_size(); i++) {
		filter_words_lowercase.append_single(pfc::string(filter_words[i].get_ptr()).toLower());
	}

	// TODO: cache all these to make filtering fast again
	try {
		for (size_t i = 0; i < find_release_artist->search_order_master.get_size(); i++) {
			
			std::list<std::pair<int, pfc::string8>>column_data_pair_list;

			
			bool is_master = find_release_artist->search_order_master[i];
			pfc::string8 item;

			if (is_master) {
				hook->set_master(&(find_release_artist->master_releases[master_index]));
				run_hook_columns(column_data_pair_list, runHook::rhMaster, runHook::rhUndef);
				item_data = (master_index << 16) | 9999;
			}
			else {
				hook->set_release(&(find_release_artist->releases[release_index]));
				run_hook_columns(column_data_pair_list, runHook::rhUndef, runHook::rhUndef);
				item_data = (9999 << 16) | release_index;
			}

			bool inserted = false;

			bool matches = true;
			if (filter.get_length()) {
				pfc::string item_lower = pfc::string(item.get_ptr()).toLower();
				for (size_t j = 0; j < filter_words_lowercase.get_count(); j++) {
					matches = item_lower.contains(filter_words_lowercase[j]);
					if (!matches) {
						break;
					}
				}
			}
			if (matches) {
				if (assist_dropId && list_index_dropId == -1) {
					list_index_dropId =	assist_id_marker(drop_release_id,
						find_release_artist->releases[release_index]->id,
						list_index, item);
				}

				insert_item_row(column_data_pair_list, list_index, item_data);
				list_index++;

				inserted = true;
			}
			if (is_master) {
				pfc::string8 sub_item;
				for (size_t j = 0; j < find_release_artist->master_releases[master_index]->sub_releases.get_size(); j++) {
					hook->set_release(&(find_release_artist->master_releases[master_index]->sub_releases[j]));
					run_hook_columns(column_data_pair_list, runHook::rhUndef, runHook::rhSubrelease);

					bool matches = true;
					if (filter.get_length()) {
						pfc::string sub_item_lower = pfc::string(sub_item.get_ptr()).toLower();
						for (size_t k = 0; k < filter_words_lowercase.get_count(); k++) {
							matches = sub_item_lower.contains(filter_words_lowercase[k]);
							if (!matches) {
								break;
							}
						}
					}
					if (matches) {
						if (!inserted) {
							insert_item_row(column_data_pair_list, list_index, item_data);
							list_index++;
							inserted = true;
						}
						item_data = (master_index << 16) | j;

						if (assist_dropId && list_index_dropId == -1) {
							list_index_dropId = assist_id_marker(drop_release_id,
								find_release_artist->master_releases[master_index]->sub_releases[j]->id,
								list_index, sub_item);
						}

						insert_item_row(column_data_pair_list, list_index, item_data);
						list_index++;
					}
				}
			}

			if (is_master) {
				master_index++;
			}
			else {
				release_index++;
			}
		}
	}
	catch (foo_discogs_exception &e) {
		foo_discogs_exception ex;
		ex << "Error formating release list item [" << e.what() << "]";
		throw ex;
	}
	select_first_release();
}

void CFindReleaseDialog::select_first_release() {
	if (uSendMessage(release_list, LB_GETCOUNT, 0, 0) == 1) {
		uSendMessage(release_list, LB_SETCURSEL, 0, 0);
		on_release_selected(0);
	}
}

void CFindReleaseDialog::on_release_selected(int list_index) {
	if (this->active_task) {
		return;
	}
	
	LPARAM out_p;
	get_item_param(release_list, list_index, 0, out_p);
	int item_data = (int)out_p;
	mounted_param myparam =	get_mounted_param(myparam);

	pfc::string8 selected_id;

	if (myparam.bmaster && !myparam.brelease) {
		expand_master_release(find_release_artist->master_releases[myparam.master_ndx], list_index);
		selected_id = find_release_artist->master_releases[myparam.master_ndx]->main_release_id;
	}
	else if (myparam.bmaster) {
		selected_id = find_release_artist->master_releases[myparam.master_ndx]->sub_releases[myparam.release_ndx]->id;
	}
	else {
		selected_id = find_release_artist->releases[myparam.release_ndx]->id;
	}
	uSetWindowText(release_url_edit, selected_id.get_ptr());
	uSendMessage(m_hWnd, DM_SETDEFID, IDC_PROCESS_RELEASE_BUTTON, 0);

}

LRESULT CFindReleaseDialog::OnEditReleaseIdText(WORD wNotifyCode, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (wNotifyCode == EN_SETFOCUS || wNotifyCode == EN_CHANGE) {
		uSendMessage(m_hWnd, DM_SETDEFID, IDC_PROCESS_RELEASE_BUTTON, 0);
	}
	return FALSE;
}

LRESULT CFindReleaseDialog::OnEditSearchText(WORD wNotifyCode, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (wNotifyCode == EN_SETFOCUS || wNotifyCode == EN_CHANGE) {
		uSendMessage(m_hWnd, DM_SETDEFID, IDC_SEARCH_BUTTON, 0);
	}
	return FALSE;
}

LRESULT CFindReleaseDialog::OnEditFilterText(WORD wNotifyCode, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	pfc::string8 text;
	if (wNotifyCode == EN_CHANGE) {
		uGetWindowText(filter_edit, text);
		filter_releases(text);
	}
	return FALSE;
}

LRESULT CFindReleaseDialog::OnSelectArtist(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	get_selected_artist_releases();
	return FALSE;
}

LRESULT CFindReleaseDialog::OnDoubleClickArtist(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int pos = (int)uSendMessage(artist_list, LB_GETCURSEL, 0, 0);
	if (pos != -1) {
		pfc::string8 url;
		url << "https://www.discogs.com/artist/" << find_release_artists[pos]->id;
		display_url(url);
	}
	return FALSE;
}

LRESULT CFindReleaseDialog::OnCustomDraw(int wParam, LPNMHDR lParam, BOOL bHandled) {
	LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;
	int pos = (int)lplvcd->nmcd.dwItemSpec;
	if (ListView_GetItemCount(release_list) == 0) return CDRF_DODEFAULT;
		
	LPARAM lp = MAKELPARAM(0, 0);;
	get_item_param(release_list, pos, 0, lp);
	int drop_release_id;
	pfc::string8 tmp_release_id; bool release_id_unknown = true;
	file_info_impl finfo;	metadb_handle_ptr item = items[0];
	item->get_info(finfo);

	if (g_discogs->file_info_get_tag(item, finfo, TAG_RELEASE_ID, tmp_release_id)) {
		drop_release_id = std::atoi(tmp_release_id.get_ptr());
		release_id_unknown = false;
	}

	bool assist_dropId = (!release_id_unknown && dropId);

	switch (lplvcd->nmcd.dwDrawStage) {
	
        case CDDS_PREPAINT: 
            return CDRF_NOTIFYITEMDRAW;
    
        case CDDS_ITEMPREPAINT: {
            int master_lst_ndx = lplvcd->nmcd.lItemlParam >> 16;
            int release_lst_ndx = lplvcd->nmcd.lItemlParam & 0xFFFF;
            bool bmaster = (master_lst_ndx != 9999);
            bool brelease = (release_lst_ndx != 9999);
    
            if (brelease && dropId && (list_index_dropId == pos))
            {
                lplvcd->clrText = RGB(0, 0, 0);
                lplvcd->clrTextBk = RGB(230, 230, 230);
                return CDRF_NEWFONT;
            }
            else {
                if (brelease) {
                    lplvcd->clrText = RGB(0, 0, 0);
                    lplvcd->clrTextBk = RGB(255, 255, 255);
                }
                else {
                    lplvcd->clrText = RGB(50, 50, 50);
                    lplvcd->clrTextBk = RGB(240, 240, 240);
                }
                return CDRF_NEWFONT;
            }
            break;
        }
        case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
            return CDRF_NEWFONT;
        }
	}
	return CDRF_DODEFAULT;
}

LRESULT CFindReleaseDialog::OnListViewItemChanged(int, LPNMHDR hdr, BOOL&) {
	NMLISTVIEW* lpStateChange = reinterpret_cast<NMLISTVIEW*>(hdr);
	if ((lpStateChange->uNewState ^ lpStateChange->uOldState) & LVIS_SELECTED) {
		int pos = ListView_GetSingleSelection(release_list);
		if (pos != -1) {
			on_release_selected(pos);
		}
		return FALSE;
	}
	return 0;
}

LRESULT CFindReleaseDialog::OnDoubleClickRelease(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int pos = (int)uSendMessage(release_list, LB_GETCURSEL, 0, 0);
	if (pos != -1) {

		LV_ITEM lvi;
		lvi.iItem = pos;
		lvi.mask = LVIF_PARAM;
		ListView_GetItem(release_list, &lvi);
		mounted_param mp = get_mounted_param(lvi.lParam);
		
		int master_index = mp.master_ndx;
		int release_index = mp.release_ndx;

		pfc::string8 url; 
		if (mp.bmaster && !mp.brelease) {
			url << "https://www.discogs.com/master/" << find_release_artist->master_releases[master_index]->id;
		}
		else if (mp.bmaster) {
			url << "https://www.discogs.com/release/" << find_release_artist->master_releases[master_index]->sub_releases[release_index]->id;
		}
		else {
			url << "https://www.discogs.com/release/" << find_release_artist->releases[release_index]->id;
		}
		display_url(url);
	}
	return FALSE;
}

LRESULT CFindReleaseDialog::OnCheckOnlyExactMatches(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	conf.display_exact_matches = IsDlgButtonChecked(IDC_ONLY_EXACT_MATCHES_CHECK) != 0;
	fill_artist_list();
	return FALSE;
}

LRESULT CFindReleaseDialog::OnSearch(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	search_artist();
	return FALSE;
}

LRESULT CFindReleaseDialog::OnClearFilter(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	uSetWindowText(filter_edit, "");
	filter_releases("");
	return FALSE;
}

LRESULT CFindReleaseDialog::OnConfigure(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (!g_discogs->configuration_dialog) {
		static_api_ptr_t<ui_control>()->show_preferences(guid_pref_page);
	}
	else {
		::SetFocus(g_discogs->configuration_dialog->m_hWnd);
	}
	return FALSE;
}

void CFindReleaseDialog::on_write_tags(const pfc::string8 &release_id) {
	service_ptr_t<process_release_callback> task = new service_impl_t<process_release_callback>(this, release_id, items);
	task->start(m_hWnd);
}

void CFindReleaseDialog::extract_id_from_url(pfc::string8 &s) {
	size_t pos = s.find_last('/');
	if (pos != pfc::infinite_size) {
		s = substr(s, pos + 1, s.get_length() - pos - 1);
	}
	pos = s.find_first('#');
	if (pos != pfc::infinite_size) {
		s = substr(s, 0, pos);
	}
	pos = s.find_first('-');
	if (pos != pfc::infinite_size) {
		s = substr(s, 0, pos);
	}
	pos = s.find_first('=');
	if (pos != pfc::infinite_size) {
		s = substr(s, pos + 1, s.get_length() - pos - 1);
	}
}

void CFindReleaseDialog::extract_release_from_link(pfc::string8 &s) {
	size_t pos = s.find_first("[r");
	if (pos == 0) {
		s = substr(s, 2, s.get_length() - 3);
	}
}

LRESULT CFindReleaseDialog::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	pfc::string8 text; 
	uGetWindowText(release_url_edit, text);
	pfc::string8 release_id = text.get_ptr();
	release_id = trim(release_id);
	extract_id_from_url(release_id);
	extract_release_from_link(release_id);
	if (release_id.get_length() != 0) {
		on_write_tags(release_id);
	}
	return TRUE;
}

LRESULT CFindReleaseDialog::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	destroy();
	return TRUE;
}

void CFindReleaseDialog::on_expand_master_release_done(const MasterRelease_ptr &master_release, int list_index, threaded_process_status &p_status, abort_callback &p_abort) {
	int master_index = 0;
	for (size_t i = 0; i < find_release_artist->master_releases.get_size(); i++) {
		if (find_release_artist->master_releases[i]->id == master_release->id) {
			master_index = i;
		}
	}
	pfc::string8 text;
	uGetWindowText(filter_edit, text);
	filter_releases(text);
	/*
	list_index++;
	for (size_t i = 0; i < master_release->sub_releases.get_size(); i++) {
		pfc::string8 item;
		titleformat_hook_impl_multiformat hook(p_status, &master_release, &(master_release->sub_releases[i]));
		try {
			CONF.search_master_sub_format_string->run_hook(location, &info, &hook, item, nullptr);
		}
		catch (foo_discogs_exception &e) {
			foo_discogs_exception ex;
			ex << "Error formating release list item [" << e.what() << "]";
			throw ex;
		}
		
		uSendMessageText(release_list, LB_INSERTSTRING, list_index, item.get_ptr());
		int item_data = (master_index << 16) | i;
		uSendMessage(release_list, LB_SETITEMDATA, list_index, (LPARAM)item_data);
	}
	*/
}

void CFindReleaseDialog::on_expand_master_release_complete() {
	active_task = NULL;
}

void CFindReleaseDialog::expand_master_release(MasterRelease_ptr &release, int pos) {
	if (release->sub_releases.get_size()) {
		return;
	}
	service_ptr_t<expand_master_release_process_callback> task = 
		new service_impl_t<expand_master_release_process_callback>(release, pos);
	active_task = &task;
	task->start(m_hWnd);
}

void CFindReleaseDialog::get_selected_artist_releases() {
	int pos = (int)uSendMessage(artist_list, LB_GETCURSEL, 0, 0);
	if (pos != -1) {
		Artist_ptr &artist = find_release_artists[pos];
		service_ptr_t<get_artist_process_callback> task =
			new service_impl_t<get_artist_process_callback>(artist->id.get_ptr());
		task->start(m_hWnd);
	}
}

void CFindReleaseDialog::search_artist() {
	pfc::string8 search;
	uGetWindowText(search_edit, search);

	service_ptr_t<search_artist_process_callback> task =
		new service_impl_t<search_artist_process_callback>(trim(search).get_ptr());
	task->start(m_hWnd);
}
