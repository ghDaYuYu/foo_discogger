#include "stdafx.h"

#include "libPPUI/clipboard.h"
#include "preview_dialog.h"
#include "preview_modal_tag_dialog.h"

static const GUID guid_cfg_window_placement_preview_modal_tag_dlg = { 0x59e76248, 0x99ac, 0x4d54, { 0x98, 0x67, 0x79, 0x2e, 0x2c, 0x75, 0xb7, 0xaf } };
static cfg_window_placement cfg_window_placement_preview_modal_tag_dlg(guid_cfg_window_placement_preview_modal_tag_dlg);

static t_uint32 st_preview_current_tab;
size_t CPreviewModalTagDialog::get_current_tab_index() { return st_preview_current_tab; }

LRESULT CPreviewModalTagDialog::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	pushcfg();
	cfg_window_placement_preview_modal_tag_dlg.on_window_destruction(m_hWnd);
	return 0;
}

CPreviewModalTagDialog::~CPreviewModalTagDialog() {

	g_discogs->preview_modal_tag_dialog = nullptr;
}

void CPreviewModalTagDialog::init_tabs_defs() {

	m_tab_table.append_single(tab_entry("Preliminary result", nullptr, -1));
	m_tab_table.append_single(tab_entry("Difference", nullptr, -1));
	m_tab_table.append_single(tab_entry("Original", nullptr, -1));
}

void CPreviewModalTagDialog::enable(bool is_enabled, bool change_focus) {
	
	//::uEnableWindow(GetDlgItem(IDCANCEL), is_enabled);	
}

LRESULT CPreviewModalTagDialog::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {

	init_tabs_defs();

	//(1) resize
	DlgResize_Init(mygripp.enabled, true);
	//(2) restore dlg position
	cfg_window_placement_preview_modal_tag_dlg.on_window_creation(m_hWnd, true);

	HWND hWndTab = uGetDlgItem(IDC_PREVIEW_MODAL_TAB);

	uTCITEM item = { 0 };
	item.mask = TCIF_TEXT;
	for (size_t n = 0; n < MY_NUM_TABS; n++) {
		PFC_ASSERT(m_tab_table[n].m_pszName != nullptr);
		item.pszText = m_tab_table[n].m_pszName;
		uTabCtrl_InsertItem(hWndTab, n, &item);
	}

	RECT rcTab;
	GetChildWindowRect(m_hWnd, IDC_PREVIEW_MODAL_TAB, &rcTab);
	uSendMessage(hWndTab, TCM_ADJUSTRECT, FALSE, (LPARAM)&rcTab);

	uSetDlgItemText(m_hWnd, IDC_PREVIEW_MODAL_TAG_NAME, m_item_result->tag_entry->tag_name);

	//todo: rev column creation and ILO
	HWND hWndTabDlgList = GetDlgItem(IDC_MODAL_TAG_LIST);
	const SIZE DPI = QueryScreenDPIEx();
	m_ui_list.CreateInDialog(*this, IDC_MODAL_TAG_LIST);
	m_ui_list.AddColumn("#", MulDiv(25, DPI.cx, 96), HDF_RIGHT, false);
	m_ui_list.AddColumn("Track", MulDiv(150, DPI.cx, 96), 0, false);
	m_ui_list.AddColumn("Value", LVSCW_AUTOSIZE, 0, false);
	
	// cols width
	load_size();

	show();

	st_preview_current_tab = m_parent_preview_mode - 1;
	uSendMessage(hWndTab, TCM_SETCURSEL, st_preview_current_tab, 0);

	::uPostMessage(m_hWnd, WM_NEXTDLGCTL, (WPARAM)(HWND)GetDlgItem(IDD_TAB_CTRL), TRUE);

	return FALSE;
}

void CPreviewModalTagDialog::ReLoad(HWND p_parent, size_t item, tag_result_ptr item_result, std::vector<pfc::string8> vtracks_desc, int parent_preview_mode) {
	
	m_iItem = item;
	m_item_result = item_result;
	m_vtracks_desc = vtracks_desc;
	m_parent_preview_mode = parent_preview_mode;

	((ILOD_preview_modal*)this)->Reload(&this->m_ui_list, item_result, &m_vtracks_desc);

	CRect rc_visible;
	m_ui_list.GetClientRect(&rc_visible);
	::RedrawWindow(m_ui_list.m_hWnd, &rc_visible, 0, RDW_INVALIDATE);
}

LRESULT CPreviewModalTagDialog::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	//update values
	if (g_discogs->preview_tags_dialog) {
		CPreviewTagsDialog* dlg = g_discogs->preview_tags_dialog;
		dlg->replace_tag_result(m_iItem, m_item_result);
		dlg->cb_generate_tags();
	}

	destroy();

	return TRUE;
}

LRESULT CPreviewModalTagDialog::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	destroy();

	return TRUE;
}


LRESULT CPreviewModalTagDialog::OnChangingTab(WORD /*wNotifyCode*/, LPNMHDR /*lParam*/, BOOL& /*bHandled*/) {

	//..
	return FALSE;
}

LRESULT CPreviewModalTagDialog::OnChangeTab(WORD /*wNotifyCode*/, LPNMHDR /*lParam*/, BOOL& /*bHandled*/) {

	//set new current tab

	st_preview_current_tab = (t_uint32)::SendDlgItemMessage(m_hWnd, IDD_TAB_CTRL, TCM_GETCURSEL, 0, 0);

	//set mode

	//todo: rev next call
	((ILOD_preview_modal*)this)->set_mode(st_preview_current_tab);

	// refresh mode values
	CRect rc_visible;
	m_ui_list.GetClientRect(&rc_visible);
	::RedrawWindow(m_ui_list.m_hWnd, &rc_visible, 0, RDW_INVALIDATE);

	return FALSE;
}

bool CPreviewModalTagDialog::context_menu_show(HWND wnd, size_t isel, LPARAM lParamCoords) {

	bool bvk_apps = lParamCoords == -1;

	isel = m_ui_list.GetFirstSelected();

	bool bselected = isel != ~0;

	POINT point;

	if (bvk_apps) {
		CRect rect;
		CWindow(wnd).GetWindowRect(&rect);
		point = rect.CenterPoint();
	}
	else {
		point.x = GET_X_LPARAM(lParamCoords);
		point.y = GET_Y_LPARAM(lParamCoords);
	}

	try {

		HMENU menu = CreatePopupMenu();

		int csel = -1;
		size_t citems = 0;
		bit_array_bittable selmask(0);

		bool is_results = wnd == m_ui_list.m_hWnd;

		uAppendMenu(menu, MF_STRING	| (bselected ? 0 : MF_DISABLED | MF_GRAYED), ID_CMD_COPY, "Copy\tCtrl+C");

		int cmd = TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, point.x, point.y, 0, wnd, 0);
		DestroyMenu(menu);

		context_menu_switch(wnd, point, is_results, cmd, selmask);
	}
	catch (...) {}
	return false;
}

bool CPreviewModalTagDialog::context_menu_switch(HWND wnd, POINT point, bool is_files, int cmd, bit_array_bittable selmask /*, CListControlOwnerData* ilist*/ ) {


	size_t isel = m_ui_list.GetFirstSelected();

	switch (cmd)
	{
	case ID_CMD_COPY: {

		pfc::string8 out;
		m_ui_list.GetSubItemText(isel, 2, out);

		ClipboardHelper::OpenScope scope;
		scope.Open(core_api::get_main_window());
		ClipboardHelper::SetString(out);

		return true;
	}
	default: {
		//..
	}
	}
	return false;
}

LRESULT CPreviewModalTagDialog::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {

	HWND hwndCtrl = (HWND)wParam;
	size_t iItem = ListView_GetFirstSelection(m_ui_list);

	context_menu_show(hwndCtrl, iItem, lParam /*Coords*/);
	return FALSE;
}

void CPreviewModalTagDialog::update_list_width() {
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

inline void CPreviewModalTagDialog::load_size() {

	if (CONF.preview_modal_tags_dlg_cols_width != 0) {
		const SIZE DPI = QueryScreenDPIEx();
		m_ui_list.ResizeColumn(0, MulDiv(LOWORD(CONF.preview_modal_tags_dlg_cols_width), DPI.cx, 96), true);
		m_ui_list.ResizeColumn(1, MulDiv(HIWORD(CONF.preview_modal_tags_dlg_cols_width), DPI.cx, 96), true);
	}
	else {
		update_list_width();
	}
}

inline bool CPreviewModalTagDialog::build_current_cfg(int& out) {

	bool bres = false;

	int colwidth1 = m_ui_list.GetHeaderItemWidth(0);
	int colwidth2 = m_ui_list.GetHeaderItemWidth(1);

	out = (int)MAKELPARAM(colwidth1, colwidth2);

	if (out != CONF.preview_modal_tags_dlg_cols_width) {		
		bres |= true;
	}

	return bres;
}

void CPreviewModalTagDialog::pushcfg() {
	int newwidths = 0;
	if (build_current_cfg(newwidths)) {
		CONF.save_preview_modal_tab_widths(newwidths);
	}
}