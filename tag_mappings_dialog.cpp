#include "stdafx.h"

#include "resource.h"
#include "tags.h"
#include "db_fetcher_component.h"

#include "preview_dialog.h"
#include "tag_mappings_credits_dlg.h"
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
		update_list_width(true);
	}
}

void CTagMappingDialog::pushcfg(bool force) {
	bool conf_changed = build_current_cfg();
	if (conf_changed || force) {
		CONF.save(new_conf::ConfFilter::CONF_FILTER_TAG, conf);
		CONF.load();
	}
}

inline bool CTagMappingDialog::build_current_cfg() {

	bool bres = false;
	bres |= check_mapping_changed();

	CRect rcDlg;
	GetWindowRect(&rcDlg);

	int dlgwidth = rcDlg.Width();
	int dlgheight = rcDlg.Height();

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

	//highlight keyword
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
	::EnableWindow(uGetDlgItem(IDAPPLY), changed);
}

LRESULT CTagMappingDialog::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	SetIcon(g_discogs->icon);
	conf = CONF;

	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(icex);
	icex.dwICC = ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx(&icex);

	HWND tag_split_default = GetDlgItem(IDC_SPLIT_BTN_TAG_FILE_DEF);
	BUTTON_SPLITINFO MyInfo;
	MyInfo.mask = BCSIF_STYLE;
	MyInfo.uSplitStyle = BCSS_STRETCH;
	Button_SetSplitInfo(tag_split_default, &MyInfo); // Send the BCM_SETSPLITINFO message to the control.

	HWND tag_split_add_new = GetDlgItem(IDC_SPLIT_BTN_TAG_ADD_NEW);
	Button_SetSplitInfo(tag_split_add_new, &MyInfo); // Send the BCM_SETSPLITINFO message to the control.

	HWND tag_split_cat_credits = GetDlgItem(IDC_SPLIT_BTN_TAG_CAT_CREDIT);
	Button_SetSplitInfo(tag_split_cat_credits, &MyInfo); // Send the BCM_SETSPLITINFO message to the control.

	db_fetcher_component db;
	vdefs = db.load_db_def_credits();
	
	m_tag_list.CreateInDialog(*this, IDC_TAG_LIST);
	m_tag_list.InitializeHeaderCtrl(HDS_DRAGDROP);
	const SIZE DPI = QueryScreenDPIEx();
	m_tag_list.AddColumn("Tag Name", 10);
	m_tag_list.AddColumn("Formatting String", 10);
	m_tag_list.AddColumn("Enabled", MulDiv(WRITE_UPDATE_COL_WIDTH, DPI.cx, 96) );
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

	on_mapping_changed(check_mapping_changed());
	::uPostMessage(m_hWnd, WM_NEXTDLGCTL, (WPARAM)(HWND)GetDlgItem(IDAPPLY), TRUE);
	return FALSE;
}

void CTagMappingDialog::update_tag(int pos, const tag_mapping_entry *entry) {
	tag_mapping_entry& dbgentry = (*m_ptag_mappings)[pos];
	m_ptag_mappings->replace_item(pos, *entry);
	m_tag_list.InvalidateRect(m_tag_list.GetItemRect(pos), true);
	on_mapping_changed(check_mapping_changed());
}

void CTagMappingDialog::update_list_width(bool initialize) {
	CRect client_rectangle;
	::GetClientRect(m_tag_list, &client_rectangle);
	int width = client_rectangle.Width();
	
	int c1, c2, c3;
	if (initialize) {	
		width -= WRITE_UPDATE_COL_WIDTH;
		c1 = width / 3;
		c2 = width / 3 * 2;
		c3 = WRITE_UPDATE_COL_WIDTH;		
		const SIZE DPI = QueryScreenDPIEx();
		m_tag_list.ResizeColumn(0, c1, true);
		m_tag_list.ResizeColumn(1, c2, true);
		m_tag_list.ResizeColumn(2, c3, true);
	}
	else {
		const SIZE DPI = QueryScreenDPIEx();
		m_tag_list.SetColumn(0, "Tag Name", 1);
		m_tag_list.SetColumn(1, "Formatting String", 2);
		m_tag_list.SetColumn(2, "Enabled", 3);
	}
}

void CTagMappingDialog::applymappings() {
	set_cfg_tag_mappings(m_ptag_mappings);
	pushcfg(true);
	on_mapping_changed(check_mapping_changed());

	if (g_discogs->preview_tags_dialog) {
		CPreviewTagsDialog* dlg = reinterpret_cast<CPreviewTagsDialog*>(g_discogs->preview_tags_dialog);
		dlg->tag_mappings_updated();
	}
}

LRESULT CTagMappingDialog::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
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
		//applymappings();
		pushcfg(true);
	}
	cfg_window_placement_tag_mapping_dlg.on_window_destruction(m_hWnd);
	return FALSE;
}

LRESULT CTagMappingDialog::OnDefaults(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	pfc::string8 msg;
	msg << "Confirm apply defaults?";
	if (uMessageBox(m_hWnd, msg, "Default Tag Mappings", MB_OKCANCEL | MB_ICONINFORMATION) == IDOK) {
		if (m_ptag_mappings != nullptr) {
			delete m_ptag_mappings;
			m_ptag_mappings = nullptr;
		}
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

	service_ptr_t<file> f;
	abort_callback_impl p_abort;
	tag_mapping_entry *buf = new tag_mapping_entry();
	try {
		filesystem::g_open(f, cvt_utf8, foobar2000_io::filesystem::open_mode_read, p_abort);
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
	
	service_ptr_t<file> f;
	abort_callback_impl p_abort;
	try {
		filesystem::g_open(f, cvt_utf8, foobar2000_io::filesystem::open_mode_write_new, p_abort);
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

LRESULT CTagMappingDialog::OnAddTag(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	trigger_add_new();
	return FALSE;
}

LRESULT CTagMappingDialog::OnAddNew(UINT, WPARAM wparam, LPARAM) {

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

void CTagMappingDialog::update_freezer(bool enable_write, bool enable_update) {

	for (size_t walk = 0; walk < m_ptag_mappings->get_count(); walk++) {
		tag_mapping_entry& entry = (*m_ptag_mappings)[walk];
		bool bupdate = false;
		if (STR_EQUAL(TAG_RELEASE_ID, entry.tag_name.get_ptr())) {
			entry.enable_write = enable_write;
			entry.enable_update = enable_update;
			bupdate = true;
		}
		else if (STR_EQUAL(TAG_MASTER_RELEASE_ID, entry.tag_name.get_ptr())) {

			entry.enable_write = enable_write;
			entry.enable_update = enable_update;
			bupdate = true;
		}
		else if (STR_EQUAL(TAG_ARTIST_ID, entry.tag_name.get_ptr())) {

			entry.enable_write = enable_write;
			entry.enable_update = enable_update;
			bupdate = true;
		}
		else if (STR_EQUAL(TAG_LABEL_ID, entry.tag_name.get_ptr())) {
		
				entry.enable_write = enable_write;
				entry.enable_update = enable_update;
				bupdate = true;
		}
	}
}

#define DISABLED_RGB	RGB(150, 150, 150)

LRESULT CTagMappingDialog::OnEditHLText(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& /*bHandled*/) {
	
	if (wNotifyCode == EN_CHANGE)
	{
		pfc::string8 hl_str = pfc::stringToLower(trim(pfc::string8(uGetWindowText(hWndCtl).c_str())));
		m_tag_list.SetHLString(hl_str, 1);
	}
	return FALSE;
}

LRESULT CTagMappingDialog::OnBtnRemoveTag(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	size_t oldCount = m_tag_list.GetItemCount();
	pfc::bit_array_bittable selmask = m_tag_list.GetSelectionMask();
	listRemoveItems(nullptr, selmask);
	m_tag_list.OnItemsRemoved(selmask, oldCount);
	on_mapping_changed(check_mapping_changed());
	return FALSE;
}

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
			ID_WRITE = 1,
			ID_UPDATE,
			ID_UPDATE_AND_WRITE,
			ID_DISABLE,
			ID_RESTORE
		};

		size_t csel = m_tag_list.GetSelectedCount();
		if (csel >= 0) {
			bool single_sel = (csel == 1);
			size_t isel = selmask.find_first(true, 0, m_tag_list.GetItemCount());
			tag_mapping_entry entry = m_ptag_mappings->get_item(isel);

			bool sop_w, sop_u, sop_wu, sop_nwu;
			sop_w = sop_u = sop_wu = sop_nwu = single_sel;
			sop_w &= entry.enable_write && !entry.enable_update;
			sop_u &= !entry.enable_write && entry.enable_update;
			sop_wu &= entry.enable_write && entry.enable_update;
			sop_nwu &= !entry.enable_write && !entry.enable_update;

			bool nfsop_w, nfsop_u, nfsop_wu, nfsop_nwu;
			nfsop_w = nfsop_u = nfsop_wu = nfsop_nwu = single_sel;
			nfsop_w &= (!bshift && entry.freeze_tag_name);//&& entry.freeze_write;
			nfsop_u &= (!bshift && entry.freeze_tag_name);//&& entry.freeze_update;
			nfsop_wu &= (!bshift && entry.freeze_tag_name);//&& entry.freeze_update&& entry.freeze_write;
			nfsop_nwu &= (entry.freeze_tag_name); //&& entry.freeze_update&& entry.freeze_write;

			menu.AppendMenu(MF_STRING | (sop_w ? MF_CHECKED : 0) | (nfsop_w ? MF_DISABLED : 0), ID_WRITE, TEXT("&Write\tW"));
			menu.AppendMenu(MF_STRING | (sop_u ? MF_CHECKED : 0) | (nfsop_u ? MF_DISABLED : 0), ID_UPDATE, TEXT("&Update\tU"));
			menu.AppendMenu(MF_STRING | (sop_wu ? MF_CHECKED : 0) |	(nfsop_wu ? MF_DISABLED : 0), ID_UPDATE_AND_WRITE, TEXT("Write and upd&ate\tA"));
			menu.AppendMenu(MF_STRING | (sop_nwu ? MF_CHECKED : 0) | (nfsop_nwu ? MF_DISABLED : 0), ID_DISABLE, TEXT("&Disable\tD"));
			
			//restore item default titleformat menu option
			if (single_sel && !entry.freeze_tag_name) {
				pfc::string8 default_value = get_default_tag(entry.tag_name);
				if (default_value.get_length()) {
					menu.AppendMenu(MF_SEPARATOR);
					menu.AppendMenu(MF_STRING | (STR_EQUAL(default_value, entry.formatting_script) ? MF_DISABLED : 0),
						ID_RESTORE, TEXT("&Restore default item script\tR"));
				}
			}

			int cmd;
			CMenuDescriptionMap receiver(*this);
			receiver.Set(ID_WRITE, "Write tag.");
			receiver.Set(ID_UPDATE, "Update tag.");
			receiver.Set(ID_UPDATE_AND_WRITE, "Write and update tag.");
			receiver.Set(ID_DISABLE, "Do not write or update tag.");

			cmd = menu.TrackPopupMenu(TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, pt.x, pt.y, receiver);

			bool bloop, bl_write, bl_update;
			bloop = bl_write = bl_update = false;

			switch (cmd) {
			case ID_RESTORE:
				if (entry.freeze_tag_name) { ; }
				else {
					pfc::string8 default_value = get_default_tag(entry.tag_name);
					if (default_value.get_length()) {
						entry.formatting_script = default_value;
						update_tag(isel, &entry);
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

			if (bloop) {
				do {
					entry = m_ptag_mappings->get_item(isel);

					if (STR_EQUAL(TAG_RELEASE_ID, entry.tag_name)) {
						if (bshift && (bl_write || bl_update))
							update_freezer(bl_write, bl_update);
					}
					else {
						entry.enable_write = bl_write;
						entry.enable_update = bl_update;
					}
					update_tag(isel, &entry);
					isel = selmask.find_next(true, isel, m_tag_list.GetItemCount());

				} while (isel < m_tag_list.GetItemCount());
			}
		}
	}
	catch (std::exception const& e) {
		console::complain("Context menu failure", e);
	}
}

LRESULT CTagMappingDialog::OnSplitDropDown(LPNMHDR lParam) {
	NMBCDROPDOWN* pDropDown = (NMBCDROPDOWN*)lParam;

	//default/import/export split button
	if (pDropDown->hdr.hwndFrom == GetDlgItem(IDC_SPLIT_BTN_TAG_FILE_DEF))
	{
		POINT pt;
		pt.x = pDropDown->rcButton.left;
		pt.y = pDropDown->rcButton.bottom;
		::ClientToScreen(pDropDown->hdr.hwndFrom, &pt);

		enum { MENU_EXPORT = 1, MENU_IMPORT, MENU_SUB_A = 100, MENU_SUB_B = 200 };
		HMENU hSplitMenu = CreatePopupMenu();

		AppendMenu(hSplitMenu, MF_STRING, MENU_EXPORT, L"Export...");
		AppendMenu(hSplitMenu, MF_STRING, MENU_IMPORT, L"Import...");

		int cmd = TrackPopupMenu(hSplitMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD, pt.x, pt.y, 0, m_hWnd, NULL);
		DestroyMenu(hSplitMenu);

		switch (cmd)
		{
			BOOL bDummy;
		case MENU_IMPORT:
			OnImport(0, 0, NULL, bDummy);
			break;
		case MENU_EXPORT:
			OnExport(0, 0, NULL, bDummy);
			break;
		}
	}
	//add new tag split button
	else if (pDropDown->hdr.hwndFrom == GetDlgItem(IDC_SPLIT_BTN_TAG_ADD_NEW)) {
		POINT pt;
		pt.x = pDropDown->rcButton.left;
		pt.y = pDropDown->rcButton.bottom;
		::ClientToScreen(pDropDown->hdr.hwndFrom, &pt);

		enum { /*MENU_EXPORT = 1, MENU_IMPORT,*/
			MENU_SUB_A = 100, MENU_SUB_B = 200,
			MENU_SUB_C = 300, MENU_SUB_D = 400,
			MENU_SUB_E = 500, MENU_SUB_F = 600,
			MENU_SUB_G = 700, MENU_SUB_H = 800,
		};

		HMENU hSplitMenu = CreatePopupMenu();

		size_t csubmenus = 8;
		LPWSTR submenus_data[8] = {
			_T("Basic"), _T("Label"),
			_T("Id"), _T("Formats and series"),
			_T("Community"), _T("Artist"),
			_T("Notes and credit"), _T("Tracks")
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
			uAppendMenu(submenus[item], MF_STRING, submenus_ids[item] + n, entry->tag_name);
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
			pfc::string8 strcmd = pfc::toString(cmd).get_ptr();
			strcmd = substr(strcmd, 1, 2);
			cmd = atoi(strcmd);
			PostMessage(MSG_ADD_NEW, cmd + 1, 0); //1 based to insert default tag by ndx (or ndx 0 to add new item)
		}
	}
	//cat credit split button
	else if (pDropDown->hdr.hwndFrom == GetDlgItem(IDC_SPLIT_BTN_TAG_CAT_CREDIT)) {
		POINT pt;
		pt.x = pDropDown->rcButton.left;
		pt.y = pDropDown->rcButton.bottom;
		::ClientToScreen(pDropDown->hdr.hwndFrom, &pt);

		enum { MENU_FIRST_CAT_CREDIT = 1 };
		HMENU hSplitMenu = CreatePopupMenu();
		size_t c = 0;
		for (auto walk_cat_credit : vdefs) {
			const pfc::stringcvt::string_os_from_utf8 os_tag_name(walk_cat_credit.first.second);
			AppendMenu(hSplitMenu, MF_STRING, MENU_FIRST_CAT_CREDIT + c, os_tag_name);
			++c;
		}

		int cmd = TrackPopupMenu(hSplitMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD, pt.x, pt.y, 0, m_hWnd, NULL);
		DestroyMenu(hSplitMenu);

		if (cmd) {
			auto cat_credit = vdefs.at(cmd - 1);
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
			update_tag(m_ptag_mappings->get_count()-1, entry);

			delete entry;

			m_tag_list.OnItemsInserted(index, 1, true);
			m_tag_list.EnsureItemVisible(index, true);
			on_mapping_changed(check_mapping_changed());

		}
	}

	return TRUE;
}
