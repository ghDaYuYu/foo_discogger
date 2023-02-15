#pragma once
#include "resource.h"
#include "helpers/DarkMode.h"

#include "libPPUI/PaintUtils.h"
#include "libPPUI/CListControlOwnerData.h"

#include "foo_discogs.h"
#include "tags.h"
#include "my_editwithbuttons.h"

static const char* TEXT_WRITE = "write";
static const char* TEXT_UPDATE = "update";
static const char* TEXT_DISABLED = "disabled";
static const char* TEXT_WRITE_UPDATE = "write + update";

class CTagMappingList : public CListControlOwnerData, public fb2k::CDarkModeHooks {

public:

	CTagMappingList(IListControlOwnerDataSource* h)
		: CListControlOwnerData(h) {}

	~CTagMappingList() {
		//..
	}

	BEGIN_MSG_MAP_EX(CTagMappingList)
		CHAIN_MSG_MAP(CListControlOwnerData)
	END_MSG_MAP()

	void ui_colors_changed() override {

		this->SetDark(fb2k::isDarkMode());
		CRect rc_visible;
		GetClientRect(&rc_visible);
		::RedrawWindow(m_hWnd, &rc_visible, 0, RDW_INVALIDATE);
	}

	void RenderHLBackground(CDCHandle p_dc, const CRect& p_itemRect, size_t item, uint32_t bkColor, pfc::string8 str, pfc::string8 hlstr, bool freeze) {

		Gdiplus::Color gdiHLColor, gdiROColor;

		if (IsDark()) {
			gdiHLColor.SetFromCOLORREF(GetSysColor(COLOR_HOTLIGHT));
		}
		else {
			gdiHLColor = Gdiplus::Color(255, 180, 180, 180);
			gdiROColor = Gdiplus::Color(255, 240, 240, 240);
		}

		const CRect* rc = &p_itemRect;
		Gdiplus::Graphics gr(p_dc);
		Gdiplus::Pen pen(freeze? gdiROColor : gdiHLColor, static_cast<Gdiplus::REAL>(rc->bottom - rc->top));
		gr.DrawLine(&pen, rc->left, rc->top + ((rc->bottom - rc->top) / 2), rc->right, rc->top + ((rc->bottom - rc->top) / 2));

		DeleteObject(&pen);
		DeleteObject(&gr);
	}

	void RenderItemBackground(CDCHandle p_dc, const CRect& p_itemRect, size_t item, uint32_t bkColor) override {

		TParent::RenderItemBackground(p_dc, p_itemRect, item, bkColor);
		pfc::string8 freeze; GetSubItemText(item, 3, freeze);
		
		bool bfreeze = freeze.get_length();
		if (!m_hl_string.get_length() && !bfreeze) return;

		pfc::string8 strItem;
		bool do_hlight = false;

		if (m_hl_string.get_length()) {			
			GetSubItemText(item, 0, strItem);
			do_hlight = pfc::string_find_first(pfc::stringToLower(strItem), m_hl_string, 0) != pfc_infinite;
		}

		if (do_hlight) {
			RenderHLBackground(p_dc, p_itemRect, item, bkColor, strItem, m_hl_string, false);
		}
	}

	void SetHLString(pfc::string8 hlstring, bool erase = 1) {
		m_hl_string = hlstring;
		this->Invalidate(erase);
	}

private:

	bool TableEdit_CanAdvanceHere(size_t item, size_t subItem, uint32_t whatHappened) const override {

		pfc::string8 freeze;
		GetSubItemText(item, 3, freeze);

		if (freeze.get_length() || (subItem != 0 && subItem != 1)) {

			return false;
		}

		(void)item; (void)subItem; (void)whatHappened;

		return true;
	}

	// CListControlOwnerData overrides

	bool IsSubItemGrayed(size_t item, size_t subItem) override {
		pfc::string8 freeze; GetSubItemText(item, 3, freeze);

		bool bfreeze = freeze.get_length();

		if (bfreeze) {
			pfc::string8 strItem;
			bool do_hlight = false;

			if (m_hl_string.get_length()) {
				GetSubItemText(item, 0, strItem);
				do_hlight = pfc::string_find_first(pfc::stringToLower(strItem), m_hl_string, 0) != pfc_infinite;
			}
			return !do_hlight;
		}
		return bfreeze;
	}

private:

	pfc::string8 m_hl_string;

};

class CTagMappingDialog : public MyCDialogImpl<CTagMappingDialog>,	
	public CMessageFilter,	private IListControlOwnerDataSource,
	public CDialogResize<CTagMappingDialog>, public fb2k::CDarkModeHooks {

public:

	enum {
		IDD = IDD_DIALOG_TAG_MAPPINGS,
	};

	CTagMappingDialog::CTagMappingDialog(HWND p_parent, UINT default_button = IDOK)
		: cewb_highlight(), m_tag_list(this) {
		//..
	}

	CTagMappingDialog::~CTagMappingDialog() {

		g_discogs->tag_mappings_dialog = nullptr;
		delete m_ptag_mappings;
	}


	virtual BOOL PreTranslateMessage(MSG* pMsg) override {

		if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
		{
			HWND focused = ::GetFocus();
			int idCtrl = ::GetDlgCtrlID(focused);
			if (idCtrl == IDC_EDIT_TAG_MATCH_HL)
			{
				return TRUE;
			}
		}
		return ::IsDialogMessage(m_hWnd, pMsg);
	}

	void UpdateAltMode(bool erase = false);

#pragma warning( push )
#pragma warning( disable : 26454 )

	MY_BEGIN_MSG_MAP(CTagMappingDialog)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnOk)
		COMMAND_ID_HANDLER(IDC_APPLY, OnApply)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		COMMAND_ID_HANDLER(IDC_EDIT_TAG_MATCH_HL, OnEditHLText)
		COMMAND_ID_HANDLER(IDC_SPLIT_BTN_TAG_ADD_NEW, OnSplitDropDown)
		COMMAND_ID_HANDLER(IDC_SPLIT_BTN_TAG_ID3_ADD_NEW, OnSplitDropDown)
		COMMAND_ID_HANDLER(IDC_SPLIT_BTN_TAG_FILE_DEF, OnSplitDropDown)
		COMMAND_ID_HANDLER(IDC_REMOVE_TAG, OnBtnRemoveTag)
		MESSAGE_HANDLER_EX(WM_CONTEXTMENU, OnContextMenu)
		CHAIN_MSG_MAP(CDialogResize<CTagMappingDialog>)
	MY_END_MSG_MAP()

#pragma warning( pop )

	BEGIN_DLGRESIZE_MAP(CTagMappingDialog)
		DLGRESIZE_CONTROL(IDC_SPLIT_BTN_TAG_FILE_DEF, DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_APPLY, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_SPLIT_BTN_TAG_ADD_NEW, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_SPLIT_BTN_TAG_ID3_ADD_NEW, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_REMOVE_TAG, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_TAG_LIST, DLSZ_SIZE_X | DLSZ_SIZE_Y)
	END_DLGRESIZE_MAP()

	void DlgResize_UpdateLayout(int cxWidth, int cyHeight) {
		CDialogResize<CTagMappingDialog>::DlgResize_UpdateLayout(cxWidth, cyHeight);
	}

	LRESULT OnGetInfo(LPNMHDR lParam);

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnOk(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnApply(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnEditHLText(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnDefaults(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnImport(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnExport(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBtnRemoveTag(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnSplitDropDown(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void enable(bool v) override {}

private:

	void set_tag_mapping(tag_mapping_list_type* ptag_mapping) {
		m_ptag_mappings = ptag_mapping;
	}

	void show_context_menu(CPoint& pt, pfc::bit_array_bittable& selmask);
	void add_new_tag(size_t pos, tag_mapping_entry entry);
	void update_list_width();
	bool update_tag(int pos, const tag_mapping_entry* entry);
	bool update_freezer(int pos, bool enable_write, bool enable_update);

	void applymappings();

	void load_column_layout();
	bool build_current_cfg();
	void pushcfg();
	void showtitle();

	bool check_mapping_changed();
	void on_mapping_changed(bool changed);

	// IListControlOwnerDataSource 

	// get Item Count

	size_t listGetItemCount(ctx_t ctx) override {

		//pointer to object calling us
		PFC_ASSERT(ctx == &m_tag_list);
		return m_ptag_mappings->get_count();
	}

	// get subitems

	pfc::string8 listGetSubItemText(ctx_t ctx, size_t item, size_t subItem) override {

		pfc::string8 buffer;
		if (item < m_ptag_mappings->get_count()) {

			const tag_mapping_entry* entry = &m_ptag_mappings->get_item_ref(item);

			if (subItem == 0) {
				buffer = entry->tag_name;
			}
			else if (subItem == 1) {
				buffer = pfc::string8(entry->formatting_script);
			}
			else if (subItem == 2) {
				const char* text =
					(entry->enable_update ? 
						(entry->enable_write ?
							TEXT_WRITE_UPDATE 
							: TEXT_UPDATE) :
					(entry->enable_write ?
						TEXT_WRITE
						: TEXT_DISABLED));

				buffer = pfc::string8(text);
			}
			else if (subItem == 3) {
				//hidden, assists highlighter
				const char* text = entry->freeze_tag_name ? "freeze" : "";
				buffer = pfc::string8(text);
			}
		}
		return buffer;
	}

	// can reorder

	bool listCanReorderItems(ctx_t) override {
		return true;
	}

	// reorder

	bool listReorderItems(ctx_t ctx, const size_t* order, size_t count) override {

		pfc::reorder_t(*m_ptag_mappings, order, m_ptag_mappings->get_count());
		on_mapping_changed(check_mapping_changed());
		return true;
	}

	// remove

	bool listRemoveItems(ctx_t ctx, pfc::bit_array const& mask) override {
		
		//remove not freezed masked

		size_t deleted = 0;
		size_t walk = -1;
		size_t max = m_ptag_mappings->get_count();

		while ((walk = mask.find_next(true, walk, max)) < max) {

			auto entry = m_ptag_mappings->get_item(walk - deleted);

			if (!entry.freeze_tag_name) {
				m_ptag_mappings->remove_by_idx(walk - deleted);
				++deleted;
			}
		}

		if (deleted) {
			on_mapping_changed(check_mapping_changed());
		}
		return deleted;
	}

	// item action

	void listItemAction(ctx_t, size_t item) override {
		tag_mapping_entry& entry = (*m_ptag_mappings)[item];
		if (!entry.freeze_tag_name) {
			m_tag_list.TableEdit_Start(item, 1);
		}
	}

	// subitem action

	void listSubItemClicked(ctx_t, size_t item, size_t subItem) override {

		tag_mapping_entry& entry = (*m_ptag_mappings)[item];

		if ((!entry.freeze_tag_name) && (subItem == 0 || subItem == 1)) {	
			m_tag_list.TableEdit_Start(item, subItem);
		}
		if (subItem == 2) {
			CPoint p;
			::GetCursorPos(&p);
			LPARAM lparam = MAKELPARAM(p.x, p.y);
			OnContextMenu(this->IDD, (WPARAM)(HWND)m_tag_list, lparam);
		}
	}

	// set edited text

	void listSetEditField(ctx_t ctx, size_t item, size_t subItem, const char* val) override {

		tag_mapping_entry& entry = (*m_ptag_mappings)[item];

		if (subItem == 0) {
			entry.tag_name = val;
		}
		else if (subItem == 1) {
			entry.formatting_script = val;
		}
		update_tag(item, &entry);
		on_mapping_changed(check_mapping_changed());
	}

	// can edit

	bool listIsColumnEditable(ctx_t, size_t subItem) override {
		return (subItem == 0 || subItem == 1);
	}

	//..end IListControlOwnerDataSource

	void TableEdit_Finished() {
		on_mapping_changed(check_mapping_changed());
	}

	void loadComboCreditUserDefinitions(HWND hparent, UINT id, vppair v, const char* selected_name) {

		HWND hwnd_cmb = ::GetDlgItem(hparent, id);

		((void)SNDMSG(hwnd_cmb, WM_SETREDRAW, (WPARAM)(BOOL)(false), 0L));

		SendMessage(hwnd_cmb, CB_RESETCONTENT, 0, 0);

		if (!v.size() && selected_name)
			uSendDlgItemMessageText(hparent, id, CB_ADDSTRING, 0, selected_name);

		size_t pos = 0;
		for (auto walk : v) {

			pfc::string8 walk_name(walk.first.second);

			int rowId = uSendDlgItemMessageText(hparent, id, CB_INSERTSTRING, pos, walk_name);
			::uSendDlgItemMessage(hparent, id, CB_SETITEMDATA, rowId, atoi(walk.first.first));
			
			if (selected_name && std::string(selected_name).compare(std::string(walk.first.second)) == 0) {
				rowId = uSendDlgItemMessageText(hparent, id, CB_SETCURSEL, rowId, 0);
			}

			++pos;
		}

		((void)SNDMSG(hwnd_cmb, WM_SETREDRAW, (WPARAM)(BOOL)(true), 0L));

		RECT rc;
		::GetClientRect(hwnd_cmb, &rc);
		::RedrawWindow(hwnd_cmb, &rc, NULL, RDW_ERASENOW /*| RDW_VALIDATE*/ | RDW_INVALIDATE | RDW_ALLCHILDREN);
	}

private:

	foo_conf conf;
	tag_mapping_list_type* m_ptag_mappings = nullptr;
	CTagMappingList m_tag_list;
	CMyEditWithButtons cewb_highlight;
	CHyperLink help_link;

};
