#include "stdafx.h"

#include "tag_mappings_dialog.h"
#include "resource.h"
#include "tags.h"
#include "preview_dialog.h"

#define WRITE_UPDATE_COL_WIDTH  85

void CNewTagMappingsDialog::pushcfg(bool force) {
	bool conf_changed = build_current_cfg();
	if (conf_changed || force) {
		CONF.save(new_conf::ConfFilter::CONF_FILTER_TAG, conf);
		CONF.load();
	}
}

inline void CNewTagMappingsDialog::load_size() {
	int width = conf.edit_tags_dialog_width;
	int height = conf.edit_tags_dialog_height;
	CRect rcCfg(0, 0, width, height);
	::CenterWindow(this->m_hWnd, rcCfg, core_api::get_main_window());

	if (conf.edit_tags_dialog_col1_width != 0) {
		ListView_SetColumnWidth(GetDlgItem(IDC_TAG_LIST), 0, conf.edit_tags_dialog_col1_width);
		ListView_SetColumnWidth(GetDlgItem(IDC_TAG_LIST), 1, conf.edit_tags_dialog_col2_width);
		ListView_SetColumnWidth(GetDlgItem(IDC_TAG_LIST), 2, conf.edit_tags_dialog_col3_width);
	}
	else {
		update_list_width(true);
	}
}

inline bool CNewTagMappingsDialog::build_current_cfg() {

	bool bres = false;

	//check global settings
	bres |= get_mapping_changed();

	CRect rcDlg;
	GetClientRect(&rcDlg);

	int dlgwidth = rcDlg.Width();
	int dlgheight = rcDlg.Height();

	//dlg size
	if (dlgwidth != conf.edit_tags_dialog_width || dlgheight != conf.edit_tags_dialog_height) {
		conf.edit_tags_dialog_width = dlgwidth;
		conf.edit_tags_dialog_height = dlgheight;
		bres |= true;
	}

	//columns size
	int width1 = ListView_GetColumnWidth(GetDlgItem(IDC_TAG_LIST), 0);
	int width2 = ListView_GetColumnWidth(GetDlgItem(IDC_TAG_LIST), 1);
	int width3 = ListView_GetColumnWidth(GetDlgItem(IDC_TAG_LIST), 2);

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

bool CNewTagMappingsDialog::get_mapping_changed() {
	bool mapping_changed = cfg_tag_mappings.get_count() != tag_mappings->get_count();
	if (!mapping_changed) {
		for (unsigned int i = 0; i < cfg_tag_mappings.get_count(); i++) {
			if (!cfg_tag_mappings.get_item(i).equals(tag_mappings->get_item(i))) {
				mapping_changed = true;
				break;
			}
		}
	}
	return mapping_changed;
}

void CNewTagMappingsDialog::on_mapping_changed(bool changed) {
	::EnableWindow(uGetDlgItem(IDAPPLY), TRUE);
}

LRESULT CNewTagMappingsDialog::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	
	conf = CONF;

	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(icex);
	icex.dwICC = ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx(&icex);

	tag_list = GetDlgItem(IDC_TAG_LIST);
	remove_button = GetDlgItem(IDC_REMOVE_TAG); 

	listview_helper::insert_column(tag_list, 0, "Tag Name", 0);
	listview_helper::insert_column(tag_list, 1, "Formatting String", 0);
	listview_helper::insert_column(tag_list, 2, "Enabled", WRITE_UPDATE_COL_WIDTH);

	ListView_SetExtendedListViewStyleEx(tag_list, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

	HWND wnd_hledit = GetDlgItem(IDC_EDIT_TAG_MATCH_HL);
	cewb_highlight.SubclassWindow(wnd_hledit);
	cewb_highlight.SetEnterEscHandlers();

	uSetWindowText(wnd_hledit, conf.edit_tags_dlg_hl_keyword);

	help_link.SubclassWindow(GetDlgItem(IDC_SYNTAX_HELP));
	pfc::string8 url(core_api::get_profile_path());
	url << "\\user-components\\foo_discogger\\foo_discogs_help.html";
	pfc::stringcvt::string_wide_from_utf8 wtext(url.get_ptr());
	help_link.SetHyperLink((LPCTSTR)const_cast<wchar_t*>(wtext.get_ptr()));

	//::SetFocus(GetDlgItem(IDC_OK));

	insert_tag_mappings();

	DlgResize_Init(mygripp.grip, true); 
	load_size();

	show();

	on_mapping_changed(get_mapping_changed());

	return FALSE;
}

void CNewTagMappingsDialog::insert_tag_mappings() {
	ListView_DeleteAllItems(tag_list);
	for (unsigned int i = 0; i < tag_mappings->get_count(); i++) {
		insert_tag(i, &(tag_mappings->get_item_ref(i)));
	}
}

static const char *TEXT_WRITE = "write";
static const char *TEXT_UPDATE = "update";
static const char *TEXT_DISABLED = "disabled";
static const char *TEXT_WRITE_UPDATE = "write + update";

void CNewTagMappingsDialog::insert_tag(int pos, const tag_mapping_entry *entry) {
	const char *text = (entry->enable_update ? (entry->enable_write ? TEXT_WRITE_UPDATE : TEXT_UPDATE) :
		(entry->enable_write ? TEXT_WRITE : TEXT_DISABLED));
	listview_helper::insert_item3(tag_list, pos, entry->tag_name,  entry->formatting_script, text, 0);
}

void CNewTagMappingsDialog::update_tag(int pos, const tag_mapping_entry *entry) {
	if (tag_mappings->get_item(pos).equals(*entry))
		return;
	
	tag_mappings->replace_item_ex(pos, *entry);
	const char *text = (entry->enable_update ? (entry->enable_write ? TEXT_WRITE_UPDATE : TEXT_UPDATE) :
		(entry->enable_write ? TEXT_WRITE : TEXT_DISABLED));
	listview_helper::set_item_text(tag_list, pos, 2, text);

	on_mapping_changed(get_mapping_changed());
}

void CNewTagMappingsDialog::refresh_item(int pos) {
	tag_mapping_entry entry = tag_mappings->get_item_ref(pos);
	listview_helper::set_item_text(tag_list, pos, 0, entry.tag_name);
	listview_helper::set_item_text(tag_list, pos, 1, entry.formatting_script);
}

void CNewTagMappingsDialog::update_list_width(bool initialize) {
	CRect client_rectangle;
	::GetClientRect(tag_list, &client_rectangle);
	int width = client_rectangle.Width();
	
	int c1, c2, c3;
	if (initialize) {
		width -= WRITE_UPDATE_COL_WIDTH;
		c1 = width / 3;
		c2 = width / 3 * 2;
		c3 = WRITE_UPDATE_COL_WIDTH;
		ListView_SetColumnWidth(tag_list, 0, c1);
		ListView_SetColumnWidth(tag_list, 1, c2);
		ListView_SetColumnWidth(tag_list, 2, c3);
	}
	else {
		c1 = ListView_GetColumnWidth(tag_list, 0);
		c2 = ListView_GetColumnWidth(tag_list, 1);
		c3 = ListView_GetColumnWidth(tag_list, 2);
	}
}

void CNewTagMappingsDialog::applymappings() {
	set_tag_mappings(tag_mappings);
	pushcfg(true);
	on_mapping_changed(get_mapping_changed());

	if (g_discogs->preview_tags_dialog) {
		CPreviewTagsDialog* dlg = reinterpret_cast<CPreviewTagsDialog*>(g_discogs->preview_tags_dialog);
		dlg->tag_mappings_updated();
	}
}

LRESULT CNewTagMappingsDialog::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	destroy();
	return TRUE;
}

LRESULT CNewTagMappingsDialog::OnApply(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	applymappings();
	return FALSE;
}

LRESULT CNewTagMappingsDialog::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	conf = CONF;
	destroy();
	return TRUE;
}

LRESULT CNewTagMappingsDialog::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	if (build_current_cfg()) {
		applymappings();
		pushcfg(true);
	}
	return 0;
}

LRESULT CNewTagMappingsDialog::OnDefaults(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (tag_mappings != nullptr) {
		delete tag_mappings;
		tag_mappings = nullptr;
	}
	tag_mappings = copy_default_tag_mappings();
	insert_tag_mappings();
	on_mapping_changed(get_mapping_changed());
	return FALSE;
}

LRESULT CNewTagMappingsDialog::OnImport(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	char filename[MAX_PATH];
	OPENFILENAMEA ofn;
	ZeroMemory(&filename, sizeof(filename));
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = m_hWnd;
	ofn.lpstrFilter = "Tag Mapping Files (*.tm)\0*.tm\0All Files (*.*)\0*.*\0";
	ofn.lpstrFile = filename;
	ofn.nMaxFile = MAX_PATH;
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
		tag_mappings->remove_all();
		while (!f->is_eof(p_abort)) {
			srf >> *buf;
			//f->read_object(buf, sizeof(tag_mapping_entry), p_abort);
			tag_mappings->add_item(*buf);
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
	insert_tag_mappings();
	on_mapping_changed(get_mapping_changed());
	return FALSE;
}

LRESULT CNewTagMappingsDialog::OnExport(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
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
	//check extension + no filename
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
		for (size_t i = 0; i < tag_mappings->get_count(); i++) {
			const tag_mapping_entry &e = tag_mappings->get_item(i);
			swf << e;
			//f->write(&e, sizeof(tag_mapping_entry), p_abort);
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

LRESULT CNewTagMappingsDialog::OnAddTag(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	trigger_add_new();
	return FALSE;
}

LRESULT CNewTagMappingsDialog::OnListItemChanged(LPNMHDR lParam) {
	const int index = ListView_GetSingleSelection(tag_list);
	if (index >= 0 && index < ListView_GetItemCount(tag_list)) {
		const tag_mapping_entry &entry = tag_mappings->get_item_ref(index);
		GetDlgItem(IDC_REMOVE_TAG).EnableWindow(!entry.freeze_tag_name);
	}
	else {
		GetDlgItem(IDC_REMOVE_TAG).EnableWindow(false);
	}
	return FALSE;
}

LRESULT CNewTagMappingsDialog::OnListClick(LPNMHDR lParam) {
	NMITEMACTIVATE* info = reinterpret_cast<NMITEMACTIVATE*>(lParam);
	if (info->iItem >= 0) {
		if (TableEdit_IsColumnEditable(info->iSubItem)) {
			const tag_mapping_entry &entry = tag_mappings->get_item_ref(info->iItem);
			if (!entry.freeze_tag_name) {
				trigger_edit(info->iItem, info->iSubItem);
			}
		}
		else {
			CPoint pt(info->ptAction);
			::ClientToScreen(tag_list, &pt);
			show_context_menu(pt, info->iItem);
		}
	}
	return FALSE;
}

LRESULT CNewTagMappingsDialog::OnListDoubleClick(LPNMHDR lParam) {
	NMITEMACTIVATE* info = reinterpret_cast<NMITEMACTIVATE*>(lParam);
	if (info->iItem < 0) {
		trigger_add_new();
	}
	return FALSE;
}

LRESULT CNewTagMappingsDialog::OnListKeyDown(LPNMHDR lParam) {
	NMLVKEYDOWN * info = reinterpret_cast<NMLVKEYDOWN*>(lParam);
	switch (info->wVKey) {
		case VK_DELETE:
			return delete_selection() ? TRUE : FALSE;

		case VK_F2:
			{
				int index = ListView_GetSingleSelection(tag_list);
				if (index >= 0) {
					trigger_edit(index, IsKeyPressed(VK_SHIFT) ? 0 : 1);
				}
			}
			return TRUE;
		
		case 'N':
			switch (GetHotkeyModifierFlags()) {
				case MOD_CONTROL:
					trigger_add_new();
					return TRUE;
				default:
					return FALSE;

			}
		default:
			return FALSE;
	}
}

LRESULT CNewTagMappingsDialog::OnEdit(UINT uMsg, WPARAM wParam, LPARAM lParam) {
	TableEdit_Start(wParam, lParam);
	return FALSE;
}

LRESULT CNewTagMappingsDialog::OnAddNew(UINT, WPARAM, LPARAM) {
	tag_mapping_entry * entry = new tag_mapping_entry();
	entry->tag_name = "[tag name]";
	entry->formatting_script = "[formatting string]";
	entry->enable_write = true;
	entry->enable_update = false;
	entry->freeze_tag_name = false;
	entry->freeze_update = false;
	entry->freeze_write = false;
	size_t index = tag_mappings->add_item(*entry);
	insert_tag(index, entry);
	TableEdit_Start(index, 0);
	return FALSE;
}

LRESULT CNewTagMappingsDialog::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if ((HWND)wParam == tag_list) {
		int selection;
		CPoint pt;
		ListView_GetContextMenuPoint(tag_list, lParam, pt, selection);
		show_context_menu(pt, selection);
	}
	return FALSE;
}

void CNewTagMappingsDialog::update_freezer(bool enable_write, bool enable_update) {

	for (size_t walk = 0; walk < tag_mappings->get_count(); walk++) {
		tag_mapping_entry tmp_entry;
		tag_mappings->get_item_ex(tmp_entry, walk);
		bool bupdate = false;
		if (STR_EQUAL(TAG_RELEASE_ID, tmp_entry.tag_name.get_ptr())) {
			tmp_entry.enable_write = enable_write;
			tmp_entry.enable_update = enable_update;
			bupdate = true;
		}
		else if (STR_EQUAL(TAG_MASTER_RELEASE_ID, tmp_entry.tag_name.get_ptr())) {
			tmp_entry.enable_write = enable_write;
			tmp_entry.enable_update = enable_update;
			bupdate = true;
		}
		else if (STR_EQUAL(TAG_ARTIST_ID, tmp_entry.tag_name.get_ptr())) {
			tmp_entry.enable_write = enable_write;
			tmp_entry.enable_update = enable_update;
			bupdate = true;
		}
		else if (STR_EQUAL(TAG_LABEL_ID, tmp_entry.tag_name.get_ptr())) {
			tmp_entry.enable_write = enable_write;
			tmp_entry.enable_update = enable_update;
			bupdate = true;
		}

		LVFINDINFO plvfi = { 0 };
		plvfi.flags = LVFI_STRING;
		pfc::stringcvt::string_wide_from_utf8 conv(tmp_entry.tag_name.get_ptr());
		plvfi.psz = conv.get_ptr();
		int item_pos = ListView_FindItem(tag_list, 0, &plvfi);
		if (bupdate && item_pos > -1) update_tag(item_pos, &tmp_entry);
	}
}

void CNewTagMappingsDialog::show_context_menu(CPoint &pt, int selection) {
	try {
		bool bshift = (GetKeyState(VK_SHIFT) & 0x8000) == 0x8000;

		CMenu menu;
		WIN32_OP(menu.CreatePopupMenu());
		enum
		{
			ID_WRITE = 1,
			ID_UPDATE,
			ID_UPDATE_AND_WRITE,
			ID_DISABLE,
			ID_RESTORE
		};
		if (selection >= 0) {
			tag_mapping_entry entry;

			tag_mappings->get_item_ex(entry, selection);

			pfc::string8 default_value = get_default_tag(entry.tag_name);

			menu.AppendMenu(MF_STRING | (entry.enable_write && !entry.enable_update ? MF_CHECKED : 0) |
				(entry.freeze_write && !bshift ? MF_DISABLED : 0), ID_WRITE, TEXT("&Write\tW"));
			menu.AppendMenu(MF_STRING | (!entry.enable_write && entry.enable_update ? MF_CHECKED : 0) |
				(entry.freeze_update && !bshift ? MF_DISABLED : 0), ID_UPDATE, TEXT("&Update\tU"));
			menu.AppendMenu(MF_STRING | (entry.enable_write && entry.enable_update ? MF_CHECKED : 0) |
				(entry.freeze_update && entry.freeze_write && !bshift ? MF_DISABLED : 0),
				ID_UPDATE_AND_WRITE, TEXT("Write+Upd&ate\tA"));
			menu.AppendMenu(MF_STRING | (!entry.enable_write && !entry.enable_update ? MF_CHECKED : 0) |
				(entry.freeze_update && entry.freeze_write && !bshift ? MF_DISABLED : 0),
				ID_DISABLE, TEXT("&Disable\tD"));
			if (default_value.get_length()) {
				menu.AppendMenu(MF_SEPARATOR);
				menu.AppendMenu(MF_STRING | (STR_EQUAL(default_value, entry.formatting_script) ? MF_DISABLED : 0),
					ID_RESTORE, TEXT("&Restore Default\tR"));
			}

			int cmd;
			CMenuDescriptionMap receiver(*this);
			receiver.Set(ID_WRITE, "Write tag.");
			receiver.Set(ID_UPDATE, "Update tag.");
			receiver.Set(ID_UPDATE_AND_WRITE, "Write and update tag.");
			receiver.Set(ID_DISABLE, "Do not write or update tag.");
			if (default_value.get_length()) {
				receiver.Set(ID_RESTORE, "Restore default tag value.");
			}
			cmd = menu.TrackPopupMenu(TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, pt.x, pt.y, receiver);

			switch (cmd) {
				case ID_WRITE:			
					if (STR_EQUAL(TAG_RELEASE_ID, entry.tag_name)) {
						update_freezer(true, false);
					}
					else {
						entry.enable_write = true;
						entry.enable_update = false;
						update_tag(selection, &entry);
					}
					break;
				case ID_UPDATE:
					if (STR_EQUAL(TAG_RELEASE_ID, entry.tag_name)) {
						update_freezer(false, true);
					}
					else {
						entry.enable_write = false;
						entry.enable_update = true;
						update_tag(selection, &entry);
					}
					break;
				case ID_UPDATE_AND_WRITE:
					if (STR_EQUAL(TAG_RELEASE_ID, entry.tag_name)) {
						update_freezer(true, true);
					}
					else {
						entry.enable_write = true;
						entry.enable_update = true;
						update_tag(selection, &entry);
					}
					break;
				case ID_DISABLE:
					if (STR_EQUAL(TAG_RELEASE_ID, entry.tag_name)) {
						update_freezer(false, false);
					}
					else {
						entry.enable_write = false;
						entry.enable_update = false;
						update_tag(selection, &entry);
					}
					break;
				case ID_RESTORE:
					entry.formatting_script = default_value;
					update_tag(selection, &entry);
					refresh_item(selection);
					break;
			}
		}
	}
	catch (std::exception const & e) {
		console::complain("Context menu failure", e);
	}
}

#define DISABLED_RGB	RGB(150, 150, 150)

LRESULT CNewTagMappingsDialog::OnCustomDraw(int idCtrl, LPNMHDR lParam , BOOL& bHandled) {
	LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;
	
	size_t pos = lplvcd->nmcd.dwItemSpec;
	int sub_item;
	if (tag_mappings->get_count() <= pos) {
		return CDRF_DODEFAULT;
	}
	const tag_mapping_entry &entry = tag_mappings->get_item_ref(pos);

	bool do_hlight = false;

	switch (lplvcd->nmcd.dwDrawStage) {
		case CDDS_PREPAINT:
			return CDRF_NOTIFYITEMDRAW;

		case CDDS_ITEMPREPAINT:
			return CDRF_NOTIFYSUBITEMDRAW;

		case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
			sub_item = lplvcd->iSubItem;
			if (sub_item == 0 && highlight_label.get_length()) {
				do_hlight = pfc::string(pfc::stringToLower(tag_mappings->get_item(pos).tag_name)).contains(highlight_label);
			}
			if (do_hlight)
				lplvcd->clrTextBk = /*hlcolor;*/ RGB(215, 215, 215);

			if ((sub_item == 0 || sub_item == 1) && entry.freeze_tag_name) {
				lplvcd->clrText = DISABLED_RGB;
			}
			
			if (do_hlight || entry.freeze_tag_name)
				return CDRF_NEWFONT;

			/*else if (sub_item == 2 && entry.freeze_write && entry.freeze_update) {
				lplvcd->clrText = DISABLED_RGB;
				return CDRF_NEWFONT;
			}
			else {
				lplvcd->clrText = DEFAULT_RGB;
				return CDRF_NEWFONT;
			}*/
	}
	return CDRF_DODEFAULT;
}

LRESULT CNewTagMappingsDialog::OnRemoveTag(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	delete_selection();
	return FALSE;
}

LRESULT CNewTagMappingsDialog::OnEditHLText(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& /*bHandled*/) {
	if (wNotifyCode == EN_CHANGE)
	{
		highlight_label = pfc::stringToLower(trim(pfc::string8(uGetWindowText(hWndCtl).c_str())));

		CRect listrc;
		::GetClientRect(tag_list, &listrc);
		int first = -1; int last = -1;
		for (int walk = 0; walk < ListView_GetItemCount(tag_list); walk++) {
			CRect lvrc;
			ListView_GetItemRect(tag_list, walk, lvrc, LVIR_BOUNDS);

			if (PtInRect(listrc, CPoint(lvrc.left, lvrc.top))) {
				if (first < 0) {
					first = last = walk;
				}
				else last = walk;
			}
			else {
				if (first > -1) {
					break;
				}
			}
		}

		if (first > -1) {
			ListView_RedrawItems(tag_list, first, last);
			CRect arc;
			::GetClientRect(tag_list, &arc);
			InvalidateRect(arc, false);
		}
	}
	return FALSE;
}
