#include "stdafx.h"

#include "helpers/DarkMode.h"

#include "tags.h"
#include "db_fetcher_component.h"
#include "yesno_dialog.h"
#include "preview_dialog.h"
#include "utils_import_export_dialog.h"
#include "yesno_dialog.h"
#include "tag_mappings_dialog.h"

// {A84411C5-16F9-408D-A916-4DA40AC3196D}
static const GUID guid_cfg_dialog_position_tag_mapping_dlg = { 0xa84411c5, 0x16f9, 0x408d, { 0xa9, 0x16, 0x4d, 0xa4, 0xa, 0xc3, 0x19, 0x6d } };
static cfgDialogPosition cfg_dialog_position_tag_mapping_dlg(guid_cfg_dialog_position_tag_mapping_dlg);

#define WRITE_UPDATE_COL_WIDTH  85

inline void CTagMappingDialog::load_column_layout() {

	if (conf.edit_tags_dialog_col1_width != 0) {
		const SIZE DPI = QueryScreenDPIEx();
		m_tag_list.ResizeColumn(0, MulDiv(conf.edit_tags_dialog_col1_width, DPI.cx, 96), true);
		m_tag_list.ResizeColumn(1, MulDiv(conf.edit_tags_dialog_col2_width, DPI.cx, 96), true);
		m_tag_list.ResizeColumn(2, MulDiv(conf.edit_tags_dialog_col3_width, DPI.cx, 96), true);
	}
	else {
		update_list_width();
	}
}

inline bool CTagMappingDialog::build_current_cfg() {

	bool bres = false;

	int width1 = m_tag_list.GetColumnWidthF(0);
	int width2 = m_tag_list.GetColumnWidthF(1);
	int width3 = m_tag_list.GetColumnWidthF(2);

	if ((width1 != conf.edit_tags_dialog_col1_width ||
		width2 != conf.edit_tags_dialog_col2_width ||
		width3 != conf.edit_tags_dialog_col3_width)) {
		
		conf.edit_tags_dialog_col1_width = width1;
		conf.edit_tags_dialog_col2_width = width2;
		conf.edit_tags_dialog_col3_width = width3;
		bres |= true;
	}

	pfc::string8 hl_word;
	uGetDlgItemText(m_hWnd, IDC_EDIT_TAG_MATCH_HL, hl_word);
	if (stricmp_utf8(hl_word, conf.edit_tags_dlg_hl_keyword)) {
		conf.edit_tags_dlg_hl_keyword = hl_word;
		bres |= true;
	}
	return bres;
}

bool CTagMappingDialog::check_mapping_changed() {

	bool mapping_changed = cfg_tag_mappings.get_count() != m_ptag_mappings->get_count();
	if (!mapping_changed) {
		for (unsigned int i = 0; i < cfg_tag_mappings.get_count(); i++) {

			if (!cfg_tag_mappings.get_item(i).equals(m_ptag_mappings->get_item(i))) {
				mapping_changed = true;
				break;
			}
		}
	}
	return mapping_changed;
}

void CTagMappingDialog::on_mapping_changed(bool changed) {
	::EnableWindow(uGetDlgItem(IDC_APPLY), changed);
}

void CTagMappingDialog::pushcfg() {

	if (build_current_cfg()) {
		CONF.save(CConf::cfgFilter::TAG, conf);
		CONF.load();
	}
}

void CTagMappingDialog::UpdateAltMode(bool erase) {

	showtitle();

	if (g_discogs) {

		conf.alt_write_flags = awt_update_mod_flag(/*from entry*/true);
	}

	set_tag_mapping(copy_tag_mappings());

	if (erase) {
		m_tag_list.ReloadItems(bit_array_true());
	}
}

LRESULT CTagMappingDialog::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {

	SetIcon(g_discogs->icon);
	UpdateAltMode();

	conf = CONF;

	//darkmode
	AddDialog(m_hWnd);
	HWND listctrl = uGetDlgItem(IDC_TAG_LIST);

	m_tag_list.CreateInDialog(*this, IDC_TAG_LIST);
	m_tag_list.InitializeHeaderCtrl(HDS_DRAGDROP);

	const SIZE DPI = QueryScreenDPIEx();
	m_tag_list.AddColumn("Tag Name", 10);
	m_tag_list.AddColumn("Formatting String", 10);
	m_tag_list.AddColumn("Action", MulDiv(WRITE_UPDATE_COL_WIDTH, DPI.cx, 96) );
	m_tag_list.SetRowStyle(conf.list_style);

	HWND wnd_hledit = GetDlgItem(IDC_EDIT_TAG_MATCH_HL);
	cewb_highlight.SubclassWindow(wnd_hledit);
	cewb_highlight.SetEnterEscHandlers();

	uSetWindowText(wnd_hledit, conf.edit_tags_dlg_hl_keyword);

	help_link.SubclassWindow(GetDlgItem(IDC_SYNTAX_HELP));
	COLORREF lnktx = IsDark() ? GetSysColor(/*COLOR_BTNHIGHLIGHT*/COLOR_MENUHILIGHT) : (COLORREF)(-1);
	help_link.m_clrLink = lnktx;
	help_link.m_clrVisited = lnktx;
	pfc::string8 url = profile_usr_components_path();
	url << "\\" << "foo_discogs_help.html";

	pfc::stringcvt::string_wide_from_utf8 wtext(url.get_ptr());
	help_link.SetHyperLink((LPCTSTR)const_cast<wchar_t*>(wtext.get_ptr()));

	DlgResize_Init(mygripp.enabled, true); 
	cfg_dialog_position_tag_mapping_dlg.AddWindow(m_hWnd);

	//dark mode
	AddControls(m_hWnd);

	load_column_layout();
	show();

	on_mapping_changed(false);
	::uPostMessage(m_hWnd, WM_NEXTDLGCTL, (WPARAM)(HWND)GetDlgItem(IDC_APPLY), TRUE);

	return FALSE;
}

bool CTagMappingDialog::update_tag(int pos, const tag_mapping_entry *entry) {

	if (!(*m_ptag_mappings)[pos].equals(*entry)) {

		m_ptag_mappings->replace_item(pos, *entry);
		m_tag_list.InvalidateRect(m_tag_list.GetItemRect(pos), true);
		return true;
	}
	return false;
}

bool CTagMappingDialog::update_freezer(int pos, bool enable_write, bool enable_update) {

	bool bchanged = false;
	tag_mapping_entry& entry = (*m_ptag_mappings)[pos];

	if (!STR_EQUAL(TAG_RELEASE_ID, entry.tag_name.get_ptr())) {
		if (!(entry.enable_write == enable_write) &&
			!(entry.enable_update == enable_update)) {
			entry.enable_write = enable_write;
			entry.enable_update = enable_update;
			update_tag(pos, &entry);
			return true;
		}
		return false;
	}

	for (size_t walk = 0; walk < m_ptag_mappings->get_count(); walk++) {

		tag_mapping_entry& entry = (*m_ptag_mappings)[walk];

		bool bwalk_update = false;
		if (entry.freeze_tag_name) {
			if (STR_EQUAL(TAG_RELEASE_ID, entry.tag_name.get_ptr())) {
				if (!enable_write)
					break;
			}
			bwalk_update = entry.enable_write != enable_write;
			bwalk_update |= entry.enable_update != enable_update;

			if (bwalk_update) {				
				entry.enable_write = enable_write;
				entry.enable_update = enable_update;
			}
		}

		if (bwalk_update) {
			m_tag_list.InvalidateRect(m_tag_list.GetItemRect(walk), true);
			bchanged |= bwalk_update;
		}
	}
	return bchanged;
}

void CTagMappingDialog::update_list_width() {
	CRect client_rectangle;
	::GetClientRect(m_tag_list, &client_rectangle);

	int c1, c2, c3;
	int width = client_rectangle.Width();

	width -= WRITE_UPDATE_COL_WIDTH;
	c1 = width / 3;
	c2 = width / 3 * 2 - GetSystemMetrics(SM_CXVSCROLL);;
	c3 = WRITE_UPDATE_COL_WIDTH;		

	m_tag_list.ResizeColumn(0, c1, true);
	m_tag_list.ResizeColumn(1, c2, true);
	m_tag_list.ResizeColumn(2, c3, true);
}

void CTagMappingDialog::applymappings() {

	set_cfg_tag_mappings(m_ptag_mappings);

	if (awt_unmatched_flag()) {

		conf.alt_write_flags = awt_update_mod_flag(/*from flag*/false);
		CONF.save(CConf::cfgFilter::TAG, CONF, CFG_ALT_WRITE_FLAGS);
	}

	if (g_discogs->preview_tags_dialog) {
		CPreviewTagsDialog* dlg = g_discogs->preview_tags_dialog;
		dlg->tag_mappings_updated();
	}
}

void CTagMappingDialog::add_new_tag(size_t pos, tag_mapping_entry entry) {

	if (pos == ~0) {
		entry.tag_name = "[tag name]";
		entry.formatting_script = "[formatting string]";
		entry.enable_write = true;
		entry.enable_update = false;
		entry.freeze_tag_name = false;
		entry.freeze_update = false;
		entry.freeze_write = false;
	}

	size_t index = m_ptag_mappings->add_item(*entry.clone());

	m_tag_list.OnItemsInserted(index, 1, true);
	m_tag_list.EnsureItemVisible(index, true);
	on_mapping_changed(check_mapping_changed());

	return;
}

void CTagMappingDialog::showtitle() {
	if (CONF.awt_alt_mode()) { uSetWindowText(m_hWnd, "Tag Mapping +"); }
	else { uSetWindowText(m_hWnd, "Tag Mapping"); }
}

LRESULT CTagMappingDialog::OnOk(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	applymappings();
	destroy();
	return TRUE;
}

LRESULT CTagMappingDialog::OnApply(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	applymappings();
	on_mapping_changed(check_mapping_changed());

	return FALSE;
}

LRESULT CTagMappingDialog::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	conf = CONF;
	destroy();
	return TRUE;
}

LRESULT CTagMappingDialog::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	if (m_tag_list.TableEdit_IsActive()) {
		m_tag_list.TableEdit_Abort(false);
	}
	pushcfg();
	cfg_dialog_position_tag_mapping_dlg.RemoveWindow(m_hWnd);
	return FALSE;
}

LRESULT CTagMappingDialog::OnDefaults(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	CYesNoDialog yndlg;
	auto res = yndlg.query(m_hWnd, { "Tag mapping configuration", "Restore default configuration ?" });
	if (res) {

		delete m_ptag_mappings;

		switch (wID) {
		case 0:
			set_tag_mapping(copy_default_tag_mappings());
			break;
		case 1:
			//all id3
			set_tag_mapping(copy_id3_default_tag_mappings(false, false));
			break;
		case 2:
			//mp3 ms explorer set
			set_tag_mapping(copy_id3_default_tag_mappings(true, false));
			break;
		}

		//refresh sliders and invalidate
		m_tag_list.OnItemsInserted(0, m_ptag_mappings->get_count(), false);
		m_tag_list.EnsureItemVisible(0, false);

		on_mapping_changed(check_mapping_changed());
	}
	return FALSE;
}

LRESULT CTagMappingDialog::OnImport(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	//todo: merge from file
	pfc::string8 title = wID ? "Append from file..." : "Import from file...";
	
	std::wstring wfilename;
	pfc::stringcvt::string_wide_from_utf8 wtext(title.get_ptr());
	
	const TCHAR wfilter[255] = L"Tag Mapping Files (*.tm)\0*.tm\0All Files (*.*)\0*.*\0";

	if (!OpenImportDlg(m_hWnd, (LPCTSTR)const_cast<wchar_t*>(wtext.get_ptr()), wfilter, wfilename)) {

		return FALSE;
	}

	service_ptr_t<file> f;
	abort_callback_impl p_abort;
	tag_mapping_entry *buf = new tag_mapping_entry();

	try {
		char fullpath[MAX_PATH] = "";
		pfc::stringcvt::convert_wide_to_utf8(fullpath, MAX_PATH, wfilename.c_str(), MAX_PATH);

		foobar2000_io::filesystem::g_open(f, fullpath, foobar2000_io::filesystem::open_mode_read, p_abort);
		stream_reader_formatter<false> srf(*f.get_ptr(), p_abort);

		bool deleted = false;

		while (!f->is_eof(p_abort)) {
			srf >> *buf;
			if (!deleted && !wID) {
				m_ptag_mappings->remove_all();
				deleted = true;
			}
			buf->is_multival_meta = is_multivalue_meta(buf->tag_name);
			m_ptag_mappings->add_item(*buf);
		}
	}
	catch (const foobar2000_io::exception_io &e) {
		foo_discogs_exception ex;
		ex << "Error importing tags: " << e.what();
		add_error(ex);
		display_errors();
		clear_errors();
	}

	delete buf;

	//refresh sliders and invalidate
	m_tag_list.OnItemsInserted(0, m_ptag_mappings->get_count(), false);
	m_tag_list.EnsureItemVisible(0, false);
	on_mapping_changed(check_mapping_changed());

	return FALSE;
}

LRESULT CTagMappingDialog::OnExport(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	std::wstring wfilename;
	const TCHAR filter[255] = L"Tag Mapping Files (*.tm)\0*.tm\0All Files(*.*)\0*.*\0\0";

	if (!OpenExportDlg(m_hWnd, filter, wfilename)) {

		return FALSE;
	}

	std::filesystem::path os_file(wfilename);

	if (std::filesystem::exists(os_file)) {
		CYesNoDialog yndlg;
		if (!yndlg.query(m_hWnd, { "Export tag mappings", "Overwrite existing file ?" })) {

			return FALSE;
		}
	}

	service_ptr_t<file> f;
	abort_callback_impl p_abort;

	try {
		char fullpath[MAX_PATH] = "";
		pfc::stringcvt::convert_wide_to_utf8(fullpath, MAX_PATH, wfilename.c_str(), MAX_PATH);

		foobar2000_io::filesystem::g_open(f, fullpath, foobar2000_io::filesystem::open_mode_write_new, p_abort);

		stream_writer_formatter<false> swf(*f.get_ptr(), p_abort);
		for (size_t i = 0; i < m_ptag_mappings->get_count(); i++) {
			tag_mapping_entry &e = m_ptag_mappings->get_item(i);
			bool release_id_mod = STR_EQUAL(TAG_RELEASE_ID, e.tag_name.get_ptr());
			if (release_id_mod) {
				if (!(e.enable_write)) {
					e.enable_write = true;
				}
			}
			swf << e;
		}
	}
	catch (const foobar2000_io::exception_io &e) {
		foo_discogs_exception ex;
		ex << "Error exporting tags: " << e.what();
		add_error(ex);
		display_errors();
		clear_errors();
	}
	return FALSE;
}

LRESULT CTagMappingDialog::OnEditHLText(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& /*bHandled*/) {
	
	if (wNotifyCode == EN_CHANGE)
	{
		pfc::string8 hl_str = pfc::stringToLower(trim(pfc::string8(uGetWindowText(hWndCtl).c_str())));
		m_tag_list.SetHLString(hl_str, 1);
	}
	return FALSE;
}

LRESULT CTagMappingDialog::OnBtnRemoveTag(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	m_tag_list.RequestRemoveSelection();
	return FALSE;
}


LRESULT CTagMappingDialog::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if ((HWND)wParam == m_tag_list) {
		CPoint pt(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		pfc::bit_array_bittable selmask = m_tag_list.GetSelectionMask();
		show_context_menu(pt, selmask);
	}
	return FALSE;
}

void CTagMappingDialog::show_context_menu(CPoint& pt, pfc::bit_array_bittable& selmask) {
	try {
		CMenu menu;
		bool bshift = (GetKeyState(VK_SHIFT) & 0x8000) == 0x8000;
		WIN32_OP(menu.CreatePopupMenu());
		enum
		{
			ID_SEL_COUNT = 1,
			ID_WRITE,
			ID_UPDATE,
			ID_UPDATE_AND_WRITE,
			ID_DISABLE,
			ID_RESTORE
		};

		size_t csel = m_tag_list.GetSelectedCount();
		size_t isel = selmask.find_first(true, 0, m_tag_list.GetItemCount());
		if (isel >= m_tag_list.GetItemCount()) return;
		bool ball_freezed = [this, selmask]()->bool
		{
			size_t sel = selmask.find_first(true, 0, m_tag_list.GetItemCount());
			do {
				tag_mapping_entry tmp_entry = m_ptag_mappings->get_item(sel);
				if (!tmp_entry.freeze_tag_name) {
					return false;					
				}
				sel = selmask.find_next(true, sel, m_tag_list.GetItemCount());
			} while (sel < m_tag_list.GetItemCount());
			return true;
		}();

		if (csel >= 0 && isel < m_ptag_mappings->get_count()) {

			bool single_sel = (csel == 1);
			tag_mapping_entry entry = m_ptag_mappings->get_item(isel);
			bool sop_w, sop_u, sop_wu, sop_nwu;
			sop_w = sop_u = sop_wu = sop_nwu = single_sel; 
			sop_w &= entry.enable_write && !entry.enable_update;
			sop_u &= !entry.enable_write && entry.enable_update;
			sop_wu &= entry.enable_write && entry.enable_update;
			sop_nwu &= !entry.enable_write && !entry.enable_update;
			bool release_id_mod = single_sel && STR_EQUAL(TAG_RELEASE_ID, entry.tag_name.get_ptr());
			release_id_mod &= !CONF.awt_alt_mode();
			bool frozen_mod = single_sel && entry.freeze_tag_name;
			bool nfsop_w, nfsop_u, nfsop_wu, nfsop_nwu;
			nfsop_w = nfsop_u = nfsop_wu = nfsop_nwu = !bshift && (release_id_mod || frozen_mod);
			nfsop_u |= release_id_mod;
			nfsop_nwu |= release_id_mod;
			if (csel > 1) {
				const pfc::stringcvt::string_os_from_utf8 lbl(PFC_string_formatter() << csel << " items selected");
				menu.AppendMenu(MF_STRING | MF_DISABLED | MF_GRAYED, ID_SEL_COUNT, lbl);
				menu.AppendMenu(MF_SEPARATOR);
			}
			menu.AppendMenu(MF_STRING | (sop_w ? MF_CHECKED : 0) | (nfsop_w ? MF_DISABLED | MF_GRAYED : 0), ID_WRITE, TEXT("&Write\tW"));
			menu.AppendMenu(MF_STRING | (sop_u ? MF_CHECKED : 0) | (nfsop_u ? MF_DISABLED | MF_GRAYED : 0), ID_UPDATE, TEXT("&Update\tU"));
			menu.AppendMenu(MF_STRING | (sop_wu ? MF_CHECKED : 0) |	(nfsop_wu ? MF_DISABLED | MF_GRAYED : 0), ID_UPDATE_AND_WRITE, TEXT("Write and upd&ate\tA"));
			menu.AppendMenu(MF_STRING | (sop_nwu ? MF_CHECKED : 0) | (nfsop_nwu ? MF_DISABLED | MF_GRAYED : 0), ID_DISABLE, TEXT("&Disable\tD"));
			
			//restore item default titleformat menu option
			if (single_sel && !entry.freeze_tag_name) {
				pfc::string8 default_value = get_default_tag(entry.tag_name);
				if (default_value.get_length()) {
					menu.AppendMenu(MF_SEPARATOR);
					menu.AppendMenu(MF_STRING | ((csel == 1) && (STR_EQUAL(default_value, entry.formatting_script))  ? MF_DISABLED | MF_GRAYED : 0),
						ID_RESTORE, TEXT("&Restore default item script\tR"));
				}
			}

			int cmd;
			CMenuDescriptionMap receiver(*this);
			receiver.Set(ID_WRITE, "Write tag.");
			receiver.Set(ID_UPDATE, "Update tag.");
			receiver.Set(ID_UPDATE_AND_WRITE, "Write and update tag.");
			receiver.Set(ID_DISABLE, "Do not write or update tag.");

			cmd = menu.TrackPopupMenu(TPM_LEFTBUTTON | TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, pt.x, pt.y, receiver);

			bool bchanged = false;
			bool bloop, bl_write, bl_update;
			bloop = bl_write = bl_update = false;

			switch (cmd) {
			case ID_RESTORE:
				if (entry.freeze_tag_name) { ; }
				else {
					pfc::string8 default_value = get_default_tag(entry.tag_name);
					if (default_value.get_length()) {
						entry.formatting_script = default_value;
						bchanged = update_tag(isel, &entry);
					}					
				}
				break;
			case ID_WRITE:
				bloop = true;
				bl_write = true;
				bl_update = false;
				break;
			case ID_UPDATE:
				bloop = true;
				bl_write = false;
				bl_update = true;
				break;
			case ID_UPDATE_AND_WRITE:
				bloop = true;
				bl_write = true;
				bl_update = true;
				break;
			case ID_DISABLE:
				bloop = true;
				bl_write = false;
				bl_update = false;
				break;
			}
			if (bloop && !(cmd == ID_RESTORE && (csel >= 1))) {

				do {
					entry = m_ptag_mappings->get_item(isel);
					release_id_mod = STR_EQUAL(TAG_RELEASE_ID, entry.tag_name.get_ptr()) && !bl_write;
					release_id_mod &= !CONF.awt_alt_mode();

					if ((entry.freeze_tag_name && bshift && !release_id_mod) || !entry.freeze_tag_name) {

						entry.enable_write = bl_write;
						entry.enable_update = bl_update;
						bchanged |= update_tag(isel, &entry);
					}
					else {
						if (bshift && release_id_mod) {
							entry.enable_write = true;
							entry.enable_update = false;
							bchanged |= update_tag(isel, &entry);
						}
					}
					isel = selmask.find_next(true, isel, m_tag_list.GetItemCount());
				} while (isel < m_tag_list.GetItemCount());
			}

			if (bchanged) {
				on_mapping_changed(check_mapping_changed());
			}
		}
	}
	catch (std::exception const& e) {
		console::complain("Context menu failure", e);
	}
}

LRESULT CTagMappingDialog::OnSplitDropDown(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& /*bHandled*/) {

	CRect rcButton;
	::GetWindowRect(hWndCtl, rcButton);
	if (wID == IDC_SPLIT_BTN_TAG_FILE_DEF)
	{
		POINT pt{0};
		pt.x = rcButton.left;
		pt.y = rcButton.bottom;

		enum { MENU_DEFAULT = 1, MENU_MORE, MENU_EXPORT , MENU_IMPORT, MENU_APPEND, MENU_MORE_ALL_ID3_23 = 10, MENU_MORE_ID3_23_MSEXPLORER };
		HMENU hSplitMenu = CreatePopupMenu();
		HMENU hSplitMenuMore = CreatePopupMenu();

		AppendMenu(hSplitMenu, MF_STRING, MENU_DEFAULT, L"Restore defaults");

		MENUITEMINFO menu_more_info;
		menu_more_info = {};
		menu_more_info.cbSize = sizeof(MENUITEMINFO);
		menu_more_info.fMask = MIIM_SUBMENU | MIIM_STRING | MIIM_ID;
		menu_more_info.hSubMenu = hSplitMenuMore;
		menu_more_info.dwTypeData = L"Other defaults...";
		menu_more_info.wID = MENU_MORE;
		InsertMenuItem(hSplitMenu, MENU_MORE, true, &menu_more_info);

		const size_t iSubs = 2;
		HMENU submenu_other[iSubs];
		LPWSTR submenus_data[iSubs] = {
		_T("ID3 template"), _T("ID3 MS Explorer"), };

		MENUITEMINFO submenus_infos[iSubs];
		for (size_t walk_sub = 0; walk_sub < iSubs; walk_sub++) {
			AppendMenu(hSplitMenuMore, MF_STRING, MENU_MORE_ALL_ID3_23 + walk_sub, submenus_data[walk_sub]);
		}

		AppendMenu(hSplitMenu, MF_STRING, MENU_EXPORT, L"Export...");
		AppendMenu(hSplitMenu, MF_STRING, MENU_IMPORT, L"Import...");
		AppendMenu(hSplitMenu, MF_STRING, MENU_APPEND, L"Append from ...");

		int cmd = TrackPopupMenu(hSplitMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD, pt.x, pt.y, 0, m_hWnd, NULL);
		DestroyMenu(hSplitMenu);

		switch (cmd)
		{
		BOOL bDummy;
		case MENU_DEFAULT:
			OnDefaults(0, 0, NULL, bDummy);
			break;
		case MENU_IMPORT:
			OnImport(0, 0, NULL, bDummy);
			break;
		case MENU_EXPORT:
			OnExport(0, 0, NULL, bDummy);
			break;
		case MENU_APPEND:
			OnImport(0, 1, NULL, bDummy);
			break;
		default:
			if (cmd >= MENU_MORE_ALL_ID3_23 && cmd <= MENU_MORE_ID3_23_MSEXPLORER) {
				OnDefaults(0, cmd - MENU_MORE_ALL_ID3_23 + 1, NULL, bDummy);
			}
			break;
		}
	}
	else if (wID == IDC_SPLIT_BTN_TAG_ADD_NEW) {

		POINT pt = {0};
		pt.x = rcButton.left;
		pt.y = rcButton.bottom;

		enum {
			MENU_ADD_NEW = 1,
			MENU_SUB_A = 100, MENU_SUB_B = 200,
			MENU_SUB_C = 300, MENU_SUB_D = 400,
			MENU_SUB_E = 500, MENU_SUB_F = 600,
			MENU_SUB_G = 700, MENU_SUB_H = 800,
		};

		HMENU hSplitMenu = CreatePopupMenu();
		AppendMenu(hSplitMenu, MF_STRING, MENU_ADD_NEW, L"<Add new>");
		size_t csubmenus = 8;
		LPWSTR submenus_data[8] = {
			_T("Basic"), _T("Label"),
			_T("Id"), _T("Format and series"),
			_T("Community"), _T("Artist"),
			_T("Notes and credit"), _T("Track")
		};
		UINT submenus_ids[8] = {
			MENU_SUB_A, MENU_SUB_B,
			MENU_SUB_C, MENU_SUB_D,
			MENU_SUB_E, MENU_SUB_F,
			MENU_SUB_G, MENU_SUB_H
		};
		pfc::array_t<HMENU> submenus; submenus.resize(csubmenus);
		pfc::array_t<MENUITEMINFO> submenu_infos; submenu_infos.resize(csubmenus);
		for (size_t n = 0; n < csubmenus; n++) {
			submenus[n] = CreatePopupMenu();
			submenu_infos[n] = { 0 };
			submenu_infos[n].cbSize = sizeof(MENUITEMINFO);
			submenu_infos[n].fMask = MIIM_SUBMENU | MIIM_STRING | MIIM_ID;
			submenu_infos[n].hSubMenu = submenus[n];
			submenu_infos[n].dwTypeData = submenus_data[n];
			submenu_infos[n].wID = submenus_ids[n];
		}

		pfc::list_t<tag_mapping_entry>* tmp_def_mappings = copy_default_tag_mappings();
		tag_mapping_entry* entry;

		//A BASIC
		size_t item = 0;
		for (size_t n = 0; n < 14; n++) {
			entry = (tag_mapping_entry*)&(tmp_def_mappings->get_item_ref(n));
			uAppendMenu(submenus[item], MF_STRING, submenus_ids[item] + n, entry->tag_name);
		}
		InsertMenuItem(hSplitMenu, submenus_ids[item], true, &submenu_infos[item]);

		//B LABEL
		++item;
		for (size_t n = 14; n < 17; n++) {
			entry = (tag_mapping_entry*)&(tmp_def_mappings->get_item_ref(n));
			uAppendMenu(submenus[item], MF_STRING, submenus_ids[item] + n, entry->tag_name);
		}
		InsertMenuItem(hSplitMenu, submenus_ids[item], true, &submenu_infos[item]);

		//C IDS
		++item;
		for (size_t n = 17; n < 21; n++) {
			entry = (tag_mapping_entry*)&(tmp_def_mappings->get_item_ref(n));
			uAppendMenu(submenus[item], MF_STRING | MF_DISABLED | MF_GRAYED, submenus_ids[item] + n, entry->tag_name);
		}
		InsertMenuItem(hSplitMenu, submenus_ids[item], true, &submenu_infos[item]);

		//D FORMAT, SERIES
		++item;
		for (size_t n = 21; n < 27; n++) {
			entry = (tag_mapping_entry*)&(tmp_def_mappings->get_item_ref(n));
			uAppendMenu(submenus[item], MF_STRING, submenus_ids[item] + n, entry->tag_name);
		}
		InsertMenuItem(hSplitMenu, submenus_ids[item], true, &submenu_infos[item]);

		//E COMMUNITY
		++item;
		for (size_t n = 27; n < 32; n++) {
			entry = (tag_mapping_entry*)&(tmp_def_mappings->get_item_ref(n));
			uAppendMenu(submenus[item], MF_STRING, submenus_ids[item] + n, entry->tag_name);
		}
		InsertMenuItem(hSplitMenu, submenus_ids[item], true, &submenu_infos[item]);

		//F ARTISTS
		++item;
		for (size_t n = 32; n < 39; n++) {
			entry = (tag_mapping_entry*)&(tmp_def_mappings->get_item_ref(n));
			uAppendMenu(submenus[item], MF_STRING, submenus_ids[item] + n, entry->tag_name);
		}
		InsertMenuItem(hSplitMenu, submenus_ids[item], true, &submenu_infos[item]);

		//G NOTES, CREDITS
		++item;
		for (size_t n = 39; n < 48; n++) {
			entry = (tag_mapping_entry*)&(tmp_def_mappings->get_item_ref(n));
			uAppendMenu(submenus[item], MF_STRING, submenus_ids[item] + n, entry->tag_name);
		}
		InsertMenuItem(hSplitMenu, submenus_ids[item], true, &submenu_infos[item]);

		//H TRACK
		++item;
		for (size_t n = 48; n < tmp_def_mappings->get_count(); n++) {
			entry = (tag_mapping_entry*)&(tmp_def_mappings->get_item_ref(n));
			uAppendMenu(submenus[item], MF_STRING, submenus_ids[item] + n, entry->tag_name);
		}
		InsertMenuItem(hSplitMenu, submenus_ids[item], true, &submenu_infos[item]);

		int cmd = TrackPopupMenu(hSplitMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD, pt.x, pt.y, 0, m_hWnd, NULL);
		DestroyMenu(hSplitMenu);
		
		if (cmd) {

			tag_mapping_entry entry;
			size_t index  = 0;
			bool addnew = cmd == 1;
			if (cmd > 1) { //submenu or <add new>?
				pfc::string8 strcmd = std::to_string(cmd).c_str();
				strcmd = strcmd.subString(strcmd.length() - 2);
				index = atoi(strcmd);			
				if (tmp_def_mappings) {
					entry = tmp_def_mappings->get_item(index);
					entry.enable_write = true;
				}
			}
			add_new_tag(addnew ? ~0 : index, entry);
		}
		delete tmp_def_mappings;
	}
	//ID3V23 template
	else if (wID == IDC_SPLIT_BTN_TAG_ID3_ADD_NEW) {

		POINT pt = { 0 };
		pt.x = rcButton.left;
		pt.y = rcButton.bottom;

		enum {
			MENU_ADD_NEW = 1,
			MENU_SUB_A = 100, MENU_SUB_B = 200,
			MENU_SUB_C = 300, MENU_SUB_D = 400,
			MENU_SUB_E = 500, MENU_SUB_F = 600,
			MENU_SUB_G = 700, MENU_SUB_H = 800,
			MENU_SUB_I = 900, MENU_SUB_J = 1000,
		};

		HMENU hSplitMenu = CreatePopupMenu();
		AppendMenu(hSplitMenu, MF_STRING, MENU_ADD_NEW, L"<Add new>");
		size_t csubmenus = 10;
		LPWSTR submenus_data[10] = {
			_T("Titles"), _T("People && Organization"),
			_T("Counts and Indexes"), _T("Dates"),
			_T("Identifiers"), _T("Flags"),
			_T("Ripping && Encoding"), _T("URL"),
			_T("Style"), _T("Misc")
		};
		UINT submenus_ids[10] = {
			MENU_SUB_A, MENU_SUB_B,
			MENU_SUB_C, MENU_SUB_D,
			MENU_SUB_E, MENU_SUB_F,
			MENU_SUB_G, MENU_SUB_H,
			MENU_SUB_I, MENU_SUB_J
		};
		pfc::array_t<HMENU> submenus; submenus.resize(csubmenus);
		pfc::array_t<MENUITEMINFO> submenu_infos; submenu_infos.resize(csubmenus);
		for (size_t n = 0; n < csubmenus; n++) {
			submenus[n] = CreatePopupMenu();
			submenu_infos[n] = { 0 };
			submenu_infos[n].cbSize = sizeof(MENUITEMINFO);
			submenu_infos[n].fMask = MIIM_SUBMENU | MIIM_STRING | MIIM_ID;
			submenu_infos[n].hSubMenu = submenus[n];
			submenu_infos[n].dwTypeData = submenus_data[n];
			submenu_infos[n].wID = submenus_ids[n];
		}

		pfc::list_t<tag_mapping_entry>* tmp_def_mappings = copy_id3_default_tag_mappings(false, true);
		tag_mapping_entry* entry;

		//A TITLES
		size_t item = 0;
		for (size_t n = 0; n < 9; n++) {
			entry = (tag_mapping_entry*)&(tmp_def_mappings->get_item_ref(n));
			uAppendMenu(submenus[item], MF_STRING, submenus_ids[item] + n, entry->tag_name);
		}
		InsertMenuItem(hSplitMenu, submenus_ids[item], true, &submenu_infos[item]);

		//B PEOPLE ORGANIZATION
		++item;
		for (size_t n = 9; n < 24; n++) {
			entry = (tag_mapping_entry*)&(tmp_def_mappings->get_item_ref(n));
			uAppendMenu(submenus[item], MF_STRING, submenus_ids[item] + n, entry->tag_name);
		}
		InsertMenuItem(hSplitMenu, submenus_ids[item], true, &submenu_infos[item]);

		//C COUNTS INDEXES
		++item;
		for (size_t n = 24; n < 31; n++) {
			entry = (tag_mapping_entry*)&(tmp_def_mappings->get_item_ref(n));
			uAppendMenu(submenus[item], MF_STRING, submenus_ids[item] + n, entry->tag_name);
		}
		InsertMenuItem(hSplitMenu, submenus_ids[item], true, &submenu_infos[item]);

		//D DATES
		++item;
		for (size_t n = 31; n < 35; n++) {
			entry = (tag_mapping_entry*)&(tmp_def_mappings->get_item_ref(n));
			uAppendMenu(submenus[item], MF_STRING, submenus_ids[item] + n, entry->tag_name);
		}
		InsertMenuItem(hSplitMenu, submenus_ids[item], true, &submenu_infos[item]);

		//E IDENTIFIERS
		++item;
		/*for (size_t n = ..; n < ..; n++) {
			entry = (tag_mapping_entry*)&(tmp_def_mappings->get_item_ref(n));
			uAppendMenu(submenus[item], MF_STRING, submenus_ids[item] + n, entry->tag_name);
		}*/
		InsertMenuItem(hSplitMenu, submenus_ids[item], true, &submenu_infos[item]);

		//F FLAGS
		++item;
		for (size_t n = 35; n < 36; n++) {
			entry = (tag_mapping_entry*)&(tmp_def_mappings->get_item_ref(n));
			uAppendMenu(submenus[item], MF_STRING, submenus_ids[item] + n, entry->tag_name);
		}
		InsertMenuItem(hSplitMenu, submenus_ids[item], true, &submenu_infos[item]);

		//G RIPPING ENCODING
		++item;
		for (size_t n = 36; n < 39; n++) {
			entry = (tag_mapping_entry*)&(tmp_def_mappings->get_item_ref(n));
			uAppendMenu(submenus[item], MF_STRING, submenus_ids[item] + n, entry->tag_name);
		}
		InsertMenuItem(hSplitMenu, submenus_ids[item], true, &submenu_infos[item]);

		//H URL
		++item;
		for (size_t n = 39; n < 48; n++) {
			entry = (tag_mapping_entry*)&(tmp_def_mappings->get_item_ref(n));
			uAppendMenu(submenus[item], MF_STRING, submenus_ids[item] + n, entry->tag_name);
		}
		InsertMenuItem(hSplitMenu, submenus_ids[item], true, &submenu_infos[item]);

		//I STYLE
		++item;
		/*for (size_t n = ..; n < ..; n++) {
			entry = (tag_mapping_entry*)&(tmp_def_mappings->get_item_ref(n));
			uAppendMenu(submenus[item], MF_STRING, submenus_ids[item] + n, entry->tag_name);
		}*/
		InsertMenuItem(hSplitMenu, submenus_ids[item], true, &submenu_infos[item]);

		//J MISC
		++item;
		for (size_t n = 48; n < tmp_def_mappings->get_count(); n++) {
			entry = (tag_mapping_entry*)&(tmp_def_mappings->get_item_ref(n));
			uAppendMenu(submenus[item], MF_STRING, submenus_ids[item] + n, entry->tag_name);
		}
		InsertMenuItem(hSplitMenu, submenus_ids[item], true, &submenu_infos[item]);

		int cmd = TrackPopupMenu(hSplitMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD, pt.x, pt.y, 0, m_hWnd, NULL);
		DestroyMenu(hSplitMenu);

		if (cmd) {

			bool addnew = cmd == 1;
			tag_mapping_entry entry;
			size_t index = 0;
		
			if (cmd > 1) { //submenu or <add new>?
				pfc::string8 strcmd = std::to_string(cmd).c_str();
				strcmd = strcmd.subString(strcmd.length() - 2);
				index = atoi(strcmd);
				if (tmp_def_mappings) {
					entry = tmp_def_mappings->get_item(index);
					entry.enable_write = true;
				}
			}
			add_new_tag(addnew ? -1 : index, entry);
		}
		delete tmp_def_mappings;
	}

	return FALSE;
}
