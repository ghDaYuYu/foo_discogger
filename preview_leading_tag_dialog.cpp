#include "stdafx.h"

#include "libPPUI/clipboard.h"
#include "preview_dialog.h"
#include "preview_leading_tag_dialog.h"

static const GUID guid_cfg_window_placement_preview_modal_tag_dlg = { 0x59e76248, 0x99ac, 0x4d54, { 0x98, 0x67, 0x79, 0x2e, 0x2c, 0x75, 0xb7, 0xaf } };
static cfg_window_placement cfg_window_placement_preview_modal_tag_dlg(guid_cfg_window_placement_preview_modal_tag_dlg);

static t_uint32 st_preview_current_tab;
size_t CPreviewLeadingTagDialog::get_current_tab_index() { return st_preview_current_tab; }


LRESULT CPreviewLeadingTagDialog::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	pushcfg();
	cfg_window_placement_preview_modal_tag_dlg.on_window_destruction(m_hWnd);
	return 0;
}

CPreviewLeadingTagDialog::~CPreviewLeadingTagDialog() {

	g_discogs->preview_modal_tag_dialog = nullptr;
}

void CPreviewLeadingTagDialog::init_results() {

	//verify upper bound
	if (m_isel >= m_tag_writer->tag_results.get_count())
		m_isel = m_tag_writer->tag_results.get_count() - 1;

	m_ptag_result = m_tag_writer->tag_results[m_isel];

	file_info_manager_ptr finfo_manager = m_tag_writer->finfo_manager;
	track_mappings_list_type track_mappings = m_tag_writer->track_mappings;

	m_vtracks_desc.clear();
	for (size_t i = 0; i < track_mappings.get_count(); i++) {
		track_mapping tm = track_mappings[i];
		if (tm.enabled) {
			pfc::string8 track_desc;
			metadb_handle_ptr item = finfo_manager->items[tm.file_index];
			item->format_title(nullptr, track_desc, m_track_desc_script, nullptr)
			m_vtracks_desc.emplace_back(track_desc);
		}
	}
}

void CPreviewLeadingTagDialog::init_tabs_defs() {

	m_tab_table.append_single(tab_entry("Lead-in", nullptr, -1));
	m_tab_table.append_single(tab_entry("Difference", nullptr, -1));
	m_tab_table.append_single(tab_entry("Original", nullptr, -1));
}

void CPreviewLeadingTagDialog::enable(bool is_enabled, bool change_focus) {
	
	//..	
}

//from libPPUI\CDialogResizeHelper.cpp
static BOOL GetChildWindowRect(HWND wnd, UINT id, RECT* child)
{
	RECT temp;
	HWND wndChild = GetDlgItem(wnd, id);
	if (wndChild == NULL) return FALSE;
	if (!GetWindowRect(wndChild, &temp)) return FALSE;
	if (!MapWindowPoints(0, wnd, (POINT*)&temp, 2)) return FALSE;
	*child = temp;
	return TRUE;
}

LRESULT CPreviewLeadingTagDialog::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {

	init_tabs_defs();

	DlgResize_Init(mygripp.enabled, true);
	cfg_window_placement_preview_modal_tag_dlg.on_window_creation(m_hWnd, true);

	HWND hWndTab = uGetDlgItem(IDC_TAB_PREVIEW_MODAL);

	// insert tab items
	uTCITEM item = { 0 };
	item.mask = TCIF_TEXT;
	for (size_t n = 0; n < MY_NUM_TABS; n++) {
		PFC_ASSERT(m_tab_table[n].m_pszName != nullptr);
		item.pszText = m_tab_table[n].m_pszName;
		uTabCtrl_InsertItem(hWndTab, n, &item);
	}

	// inner part of the tab control
	RECT rcTab;
	GetChildWindowRect(m_hWnd, IDC_TAB_PREVIEW_MODAL, &rcTab);
	uSendMessage(hWndTab, TCM_ADJUSTRECT, FALSE, (LPARAM)&rcTab);

	// set tag name textbox
	uSetDlgItemText(m_hWnd, IDC_PREVIEW_LEADING_TAG_NAME, m_ptag_result->tag_entry->tag_name);
	
	//todo: rev column creation and ILO
	HWND hWndTabDlgList = GetDlgItem(IDC_LEADING_TAG_LIST);
	const SIZE DPI = QueryScreenDPIEx();
	m_ui_list.CreateInDialog(*this, IDC_LEADING_TAG_LIST);

	//darkmode
	AddDialog(m_hWnd);
	AddControls(m_hWnd);

	CustomFont(m_hWnd, m_customfont);

	m_ui_list.AddColumn("#", MulDiv(25, DPI.cx, 96), HDF_RIGHT, false);
	m_ui_list.AddColumn("Track", MulDiv(150, DPI.cx, 96), 0, false);
	m_ui_list.AddColumn("Value", LVSCW_AUTOSIZE, 0, false);
	// cols width
	load_column_layout();

	show();

	//set active tab
	st_preview_current_tab = (int)m_parent_preview_mode;
	uSendMessage(hWndTab, TCM_SETCURSEL, st_preview_current_tab, 0);

	//set next control focus
	::uPostMessage(m_hWnd, WM_NEXTDLGCTL, (WPARAM)(HWND)GetDlgItem(IDD_TAB_CTRL), TRUE);

	return FALSE;
}

void CPreviewLeadingTagDialog::ReloadItem(HWND p_parent, size_t item, PreView parent_preview_mode) {
	
	m_isel = item;

	if (parent_preview_mode != PreView::Undef) {
		m_parent_preview_mode = parent_preview_mode;
	}

	init_results();

	// set tag name textbox
	uSetDlgItemText(m_hWnd, IDC_PREVIEW_LEADING_TAG_NAME, m_ptag_result->tag_entry->tag_name);

	dynamic_cast<ILOD_preview_leading*>(this)->PullDlgResult();

	CRect rc_visible;
	m_ui_list.GetClientRect(&rc_visible);
	::RedrawWindow(m_ui_list.m_hWnd, &rc_visible, 0, RDW_INVALIDATE);

	//set next control focus
	::uPostMessage(m_hWnd, WM_NEXTDLGCTL, (WPARAM)(HWND)GetDlgItem(IDD_TAB_CTRL), TRUE);
}

void CPreviewLeadingTagDialog::push_updates() {
	//push updates
	if (g_discogs->preview_tags_dialog) {

		CPreviewTagsDialog* dlg = g_discogs->preview_tags_dialog;
		dlg->replace_tag_result(m_isel, m_ptag_result);
		dlg->cb_generate_tags();
	}
}

LRESULT CPreviewLeadingTagDialog::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	push_updates();
	destroy();
	return TRUE;
}

LRESULT CPreviewLeadingTagDialog::OnApply(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	push_updates();	
	return TRUE;
}

LRESULT CPreviewLeadingTagDialog::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	destroy();
	return TRUE;
}

LRESULT CPreviewLeadingTagDialog::OnChangingTab(WORD /*wNotifyCode*/, LPNMHDR /*lParam*/, BOOL& /*bHandled*/) {
	//or crash
	m_ui_list.TableEdit_Abort(false);
	return FALSE;
}

LRESULT CPreviewLeadingTagDialog::OnChangeTab(WORD /*wNotifyCode*/, LPNMHDR /*lParam*/, BOOL& /*bHandled*/) {

	//set new current tab
	st_preview_current_tab = (t_uint32)::SendDlgItemMessage(m_hWnd, IDD_TAB_CTRL, TCM_GETCURSEL, 0, 0);

	//set mode
	PreView pv_curr = (PreView)st_preview_current_tab;

	//static_cast unnecessary when casting upwards
	((ILOD_preview_leading*)this)->SetMode(pv_curr);

	//redraw
	CRect rc_visible;
	m_ui_list.GetClientRect(&rc_visible);
	::RedrawWindow(m_ui_list.m_hWnd, &rc_visible, 0, RDW_INVALIDATE);

	return FALSE;
}

bool CPreviewLeadingTagDialog::context_menu_show(HWND wnd, size_t isel, LPARAM lParamPos) {

	bool bvk_apps = lParamPos == -1;
	bool bselected = isel != ~0;

	POINT point;

	if (bvk_apps) {
		CRect rect;
		CWindow(wnd).GetWindowRect(&rect);
		point = rect.CenterPoint();
	}
	else {
		point.x = GET_X_LPARAM(lParamPos);
		point.y = GET_Y_LPARAM(lParamPos);
	}

	try {

		HMENU menu = CreatePopupMenu();

		int csel = -1;
		size_t citems = 0;
		bit_array_bittable selmask(0);

		bool is_results = wnd == m_ui_list.m_hWnd;

		uAppendMenu(menu, MF_STRING	| (bselected ? 0 : MF_DISABLED | MF_GRAYED), ID_CMD_COPY, "Copy\tCtrl+C");
		uAppendMenu(menu, MF_STRING | (bselected && st_preview_current_tab == 0 ? 0 : MF_DISABLED | MF_GRAYED), ID_CMD_EDIT, "&Edit");
		uAppendMenu(menu, MF_SEPARATOR, 0, 0);
		uAppendMenu(menu, MF_STRING, ID_CMD_APPLY, "&Apply");
		int cmd = TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, point.x, point.y, 0, wnd, 0);
		DestroyMenu(menu);

		context_menu_switch(wnd, point, is_results, cmd, selmask /*,ilist*/);
	}
	catch (...) {}
	return false;
}

bool CPreviewLeadingTagDialog::context_menu_switch(HWND wnd, POINT point, bool is_files, int cmd, bit_array_bittable selmask /*, CListControlOwnerData* ilist*/ ) {

	size_t isel = m_ui_list.GetFirstSelected();

	switch (cmd)
	{

	case ID_CMD_EDIT:

		((ILOD_preview_leading*)this)->TriggerAction();
		return true;
	case ID_CMD_COPY: {

		pfc::string8 out;
		m_ui_list.GetSubItemText(isel, 2, out);

		ClipboardHelper::OpenScope scope;
		scope.Open(core_api::get_main_window());
		ClipboardHelper::SetString(out);
		return true;
	}
	case ID_CMD_APPLY:

		push_updates();
		return true;

	default: {
		//..
	}
	}
	return false;
}

LRESULT CPreviewLeadingTagDialog::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {

	HWND hwndCtrl = (HWND)wParam;
	size_t iItem = ListView_GetFirstSelection(m_ui_list);

	context_menu_show(hwndCtrl, iItem, lParam /*Coords*/);
	return FALSE;
}

void CPreviewLeadingTagDialog::update_list_width() {
	CRect rc;
	::GetClientRect(m_ui_list, &rc);

	int c1, c2;
	int width = rc.Width();

	width -= ORD_COL_DEF_WITH;
	c1 = ORD_COL_DEF_WITH;
	c2 = width / 3;

	m_ui_list.ResizeColumn(0, c1, true);
	m_ui_list.ResizeColumn(1, c2, true);
}

inline void CPreviewLeadingTagDialog::load_column_layout() {

	if (CONF.preview_leading_tags_dlg_cols_width != 0) {
		const SIZE DPI = QueryScreenDPIEx();
		m_ui_list.ResizeColumn(0, MulDiv(LOWORD(CONF.preview_leading_tags_dlg_cols_width), DPI.cx, 96), true);
		m_ui_list.ResizeColumn(1, MulDiv(HIWORD(CONF.preview_leading_tags_dlg_cols_width), DPI.cx, 96), true);
	}
	else {
		update_list_width();
	}
}

inline bool CPreviewLeadingTagDialog::build_current_cfg(int& out) {

	bool bres = false;

	int colwidth1 = m_ui_list.GetHeaderItemWidth(0);
	int colwidth2 = m_ui_list.GetHeaderItemWidth(1);

	out = (int)MAKELPARAM(colwidth1, colwidth2);

	if (out != CONF.preview_leading_tags_dlg_cols_width) {		
		bres |= true;
	}
	return bres;
}

void CPreviewLeadingTagDialog::pushcfg() {
	int newwidths = 0;
	if (build_current_cfg(newwidths)) {
		CONF.save_preview_modal_tab_widths(newwidths);
	}
}