#include "stdafx.h"

#include "tags.h"
#include "db_fetcher_component.h"

#include "preview_dialog.h"
#ifdef CAT_CRED
#include "tag_mappings_credits_dlg.h"
#endif // CAT_CRED

#include "tag_mappings_dialog.h"

static const GUID guid_cfg_window_placement_tag_mapping_dlg = { 0x7056137a, 0x8fe9, 0x4b5e, { 0x89, 0x8e, 0x13, 0x59, 0x83, 0xfd, 0x3, 0x95 } };
static cfg_window_placement cfg_window_placement_tag_mapping_dlg(guid_cfg_window_placement_tag_mapping_dlg);

#define WRITE_UPDATE_COL_WIDTH  85

inline void CTagMappingDialog::load_size() {

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

void CTagMappingDialog::pushcfg() {

	CONF.save(CConf::cfgFilter::TAG, conf);
	CONF.load();

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

LRESULT CTagMappingDialog::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	SetIcon(g_discogs->icon);
	conf = CONF;

	db_fetcher_component db;

#ifdef CAT_CRED
	v_cat_defs = db.load_db_def_credits();
#endif // CAT_CRED

	m_tag_list.CreateInDialog(*this, IDC_TAG_LIST);
	m_tag_list.InitializeHeaderCtrl(HDS_DRAGDROP);
	const SIZE DPI = QueryScreenDPIEx();
	m_tag_list.AddColumn("Tag Name", 10);
	m_tag_list.AddColumn("Formatting String", 10);
	m_tag_list.AddColumn("Action", MulDiv(WRITE_UPDATE_COL_WIDTH, DPI.cx, 96) );
	m_tag_list.SetRowStyle(conf.list_style);
	remove_button = GetDlgItem(IDC_REMOVE_TAG); 

	HWND wnd_hledit = GetDlgItem(IDC_EDIT_TAG_MATCH_HL);
	cewb_highlight.SubclassWindow(wnd_hledit);
	cewb_highlight.SetEnterEscHandlers();

	uSetWindowText(wnd_hledit, conf.edit_tags_dlg_hl_keyword);

	help_link.SubclassWindow(GetDlgItem(IDC_SYNTAX_HELP));
	pfc::string8 url(core_api::get_profile_path());
	url << "\\user-components\\foo_discogger\\foo_discogs_help.html";
	pfc::stringcvt::string_wide_from_utf8 wtext(url.get_ptr());
	help_link.SetHyperLink((LPCTSTR)const_cast<wchar_t*>(wtext.get_ptr()));

	DlgResize_Init(mygripp.enabled, true); 
	load_size();
	cfg_window_placement_tag_mapping_dlg.on_window_creation(m_hWnd, true);
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
	c2 = width / 3 * 2;
	c3 = WRITE_UPDATE_COL_WIDTH;		

	m_tag_list.ResizeColumn(0, c1, true);
	m_tag_list.ResizeColumn(1, c2, true);
	m_tag_list.ResizeColumn(2, c3, true);
}

void CTagMappingDialog::applymappings() {

	set_cfg_tag_mappings(m_ptag_mappings);
	on_mapping_changed(check_mapping_changed());

	if (g_discogs->preview_tags_dialog) {
		CPreviewTagsDialog* dlg = g_discogs->preview_tags_dialog;
		dlg->tag_mappings_updated();
	}
}

LRESULT CTagMappingDialog::OnOk(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	applymappings();
	destroy();
	return TRUE;
}

LRESULT CTagMappingDialog::OnApply(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	applymappings();
	return FALSE;
}

LRESULT CTagMappingDialog::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	conf = CONF;
	destroy();
	return TRUE;
}

LRESULT CTagMappingDialog::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {

	if (build_current_cfg()) {
		pushcfg();
	}
	cfg_window_placement_tag_mapping_dlg.on_window_destruction(m_hWnd);
	return FALSE;
}

LRESULT CTagMappingDialog::OnDefaults(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	pfc::string8 msg;
	msg << "Reset to default configuration?";
	if (uMessageBox(m_hWnd, msg, "Default Tag Mappings", MB_OKCANCEL | MB_ICONINFORMATION) == IDOK) {

		set_tag_mapping(copy_default_tag_mappings());
		m_tag_list.Invalidate(true);
		on_mapping_changed(check_mapping_changed());
	}
	return FALSE;
}

LRESULT CTagMappingDialog::OnImport(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	char filename[MAX_PATH];
	OPENFILENAMEA ofn;
	ZeroMemory(&filename, sizeof(filename));
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = m_hWnd;
	ofn.lpstrFilter = "Tag Mapping Files (*.tm)\0*.tm\0All Files (*.*)\0*.*\0";
	ofn.lpstrFile = filename;
	ofn.nMaxFile = MAX_PATH-1;
	ofn.lpstrTitle = "Load from file...";
	ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;

	if (GetOpenFileNameA(&ofn)) {
		log_msg(filename);
	}

	{
		pfc::string8 tmpFullPath(ofn.lpstrFile);
		if (!tmpFullPath.length()) {
			return FALSE;
		}
	}

	pfc::stringcvt::string_utf8_from_codepage cvt_utf8(0, ofn.lpstrFile);

	std::filesystem::path os_file = std::filesystem::u8path(cvt_utf8.get_ptr());

	service_ptr_t<file> f;
	abort_callback_impl p_abort;
	tag_mapping_entry *buf = new tag_mapping_entry();
	try {
		filesystem::g_open(f, os_file.string().c_str(), foobar2000_io::filesystem::open_mode_read, p_abort);
		stream_reader_formatter<false> srf(*f.get_ptr(), p_abort);
		m_ptag_mappings->remove_all();
		while (!f->is_eof(p_abort)) {
			srf >> *buf;
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
	m_tag_list.Invalidate(true);
	on_mapping_changed(check_mapping_changed());
	return FALSE;
}

LRESULT CTagMappingDialog::OnExport(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	char filename[MAX_PATH];
	OPENFILENAMEA ofn;
	ZeroMemory(&filename, sizeof(filename));
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = m_hWnd;
	ofn.lpstrFilter = "Tag Mapping Files (*.tm)\0*.tm\0All Files (*.*)\0*.*\0";
	ofn.lpstrFile = filename;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = "Save to file...";
	ofn.Flags = OFN_DONTADDTORECENT;

	GetSaveFileNameA(&ofn);

	pfc::string8 strFinalName(filename);
	pfc::string8 tmpNoExt(pfc::string_filename(filename).get_ptr());
	//check also extension without name
	tmpNoExt.truncate(tmpNoExt.get_length() - pfc::string_extension(filename).length() + (pfc::string_extension(filename).length() ? 1 /*+1 dot ext length*/ : 0) );
	if (!strFinalName.length() || !tmpNoExt.get_length()) {
		return FALSE;
	}

	pfc::string8 outExt;
	pfc::string8 filterExt = ofn.nFilterIndex == 1 ? ".tm" : "";
	outExt << "." <<  pfc::string_extension(filename);
	if (outExt.get_length() == 1) outExt = "";

	if ((ofn.nFilterIndex == 1) && (stricmp_utf8(outExt, ".tm"))) {
		strFinalName << filterExt;
	}

	pfc::stringcvt::string_utf8_from_codepage cvt_utf8(0, strFinalName);

	std::filesystem::path os_file = std::filesystem::u8path(cvt_utf8.get_ptr());
	
	service_ptr_t<file> f;
	abort_callback_impl p_abort;
	try {
		filesystem::g_open(f, os_file.string().c_str(), foobar2000_io::filesystem::open_mode_write_new, p_abort);
		stream_writer_formatter<false> swf(*f.get_ptr(), p_abort);
		for (size_t i = 0; i < m_ptag_mappings->get_count(); i++) {
			const tag_mapping_entry &e = m_ptag_mappings->get_item(i);
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

LRESULT CTagMappingDialog::OnAddNewTag(UINT, WPARAM wparam, LPARAM) {

	tag_mapping_entry * entry;

	size_t index;
	if (!wparam) {
		entry = new tag_mapping_entry();
		entry->tag_name = "[tag name]";
		entry->formatting_script = "[formatting string]";
		entry->enable_write = true;
		entry->enable_update = false;
		entry->freeze_tag_name = false;
		entry->freeze_update = false;
		entry->freeze_write = false;

		index = m_ptag_mappings->add_item(*entry);
		delete entry;
	}
	else {
		pfc::list_t<tag_mapping_entry>* tmp_def_mappings = copy_default_tag_mappings();
		entry = (tag_mapping_entry*)&(tmp_def_mappings->get_item_ref(wparam-1)); //incoming ndx is 1 based
		index = m_ptag_mappings->add_item(*entry);
		delete tmp_def_mappings;
	}

	m_tag_list.OnItemsInserted(index, 1, true);
	m_tag_list.EnsureItemVisible(index, true);
	on_mapping_changed(check_mapping_changed());
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

	//will visit listRemoveItems...

	m_tag_list.RequestRemoveSelection();

	return FALSE;
}

#ifdef CAT_CRED
LRESULT CTagMappingDialog::OnBtnCreditsClick(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	if (!g_discogs->tag_credit_dialog) {
		g_discogs->tag_credit_dialog = fb2k::newDialog<CTagCreditDialog>(core_api::get_main_window()/*, nullptr*/);
	}
	else {
		CDialogImpl* tmdlg = pfc::downcast_guarded<CDialogImpl*>(g_discogs->tag_credit_dialog);
		::SetFocus(tmdlg->m_hWnd);
	}
	return FALSE;
}
#endif // CAT_CRED

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
			
			//checkmarks (only single item selections)
			bool sop_w, sop_u, sop_wu, sop_nwu;
			sop_w = sop_u = sop_wu = sop_nwu = single_sel; 
			sop_w &= entry.enable_write && !entry.enable_update;
			sop_u &= !entry.enable_write && entry.enable_update;
			sop_wu &= entry.enable_write && entry.enable_update;
			sop_nwu &= !entry.enable_write && !entry.enable_update;

			bool release_id_mod = single_sel && STR_EQUAL(TAG_RELEASE_ID, entry.tag_name.get_ptr());
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
			
			if (bchanged) on_mapping_changed(check_mapping_changed());
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
		POINT pt;
		pt.x = rcButton.left;
		pt.y = rcButton.bottom;

		enum { MENU_DEFAULT = 1, MENU_EXPORT , MENU_IMPORT };
		HMENU hSplitMenu = CreatePopupMenu();

		AppendMenu(hSplitMenu, MF_STRING, MENU_DEFAULT, L"Restore defaults");
		AppendMenu(hSplitMenu, MF_STRING, MENU_EXPORT, L"Export...");
		AppendMenu(hSplitMenu, MF_STRING, MENU_IMPORT, L"Import...");

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
		}
	}
	else if (wID == IDC_SPLIT_BTN_TAG_ADD_NEW) {
		POINT pt;
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

		delete tmp_def_mappings;

		int cmd = TrackPopupMenu(hSplitMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD, pt.x, pt.y, 0, m_hWnd, NULL);
		DestroyMenu(hSplitMenu);
		
		if (cmd) {

			//convert cmd to message param
			//1 based index to insert default tag, 0 to add a new item

			if (cmd == 1) {
				cmd = 0;
			}
			else {
				pfc::string8 strcmd = std::to_string(cmd).c_str();
				cmd = atoi(substr(strcmd, 1, 2));
				++cmd;				
			}

			PostMessage(MSG_ADD_NEW, cmd, 0);
		}
	}

#ifdef CAT_CRED
	//cat credit split button
	else if (wID == IDC_SPLIT_BTN_TAG_CAT_CREDIT) {
	//else if (pDropDown->hdr.hwndFrom == GetDlgItem(IDC_SPLIT_BTN_TAG_CAT_CREDIT)) {
		POINT pt;
		pt.x = rcButton.left;
		pt.y = rcButton.bottom;

		enum { MENU_EDIT_CAT_CREDITS = 1, MENU_FIRST_CAT_CREDIT };
		HMENU hSplitMenu = CreatePopupMenu();
		AppendMenu(hSplitMenu, MF_STRING, MENU_EDIT_CAT_CREDITS, L"Edit...");

		size_t c = 0;
		for (auto walk_cat_credit : v_cat_defs) {
			const pfc::stringcvt::string_os_from_utf8 os_tag_name(walk_cat_credit.first.second);
			AppendMenu(hSplitMenu, MF_STRING, MENU_FIRST_CAT_CREDIT + c, os_tag_name);
			++c;
		}

		int cmd = TrackPopupMenu(hSplitMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD, pt.x, pt.y, 0, m_hWnd, NULL);
		DestroyMenu(hSplitMenu);

		if (cmd) {

			if (cmd == 1) {
				BOOL bDummy;
				OnBtnCreditsClick(0, wID, NULL, bDummy);
			}
			else {
				auto cat_credit = v_cat_defs.at(cmd - MENU_FIRST_CAT_CREDIT);
				pfc::string8 tf;
				tf << PFC_string_formatter() << "%" << cat_credit.second.first << "%";
				tag_mapping_entry* entry = new tag_mapping_entry();
				entry->tag_name = cat_credit.first.second;
				entry->formatting_script = tf;
				entry->enable_write = true;
				entry->enable_update = false;
				entry->freeze_tag_name = false;
				entry->freeze_update = false;
				entry->freeze_write = false;

				size_t index = m_ptag_mappings->add_item(*entry);
				update_tag(m_ptag_mappings->get_count() - 1, entry);

				delete entry;

				m_tag_list.OnItemsInserted(index, 1, true);
				m_tag_list.EnsureItemVisible(index, true);
				on_mapping_changed(check_mapping_changed());
			}
		}
	}
#endif // CAT_CRED

	return FALSE;
}
