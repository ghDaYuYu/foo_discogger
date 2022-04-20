#pragma once

#include "libPPUI/PaintUtils.h"
#include "libPPUI/CListControlOwnerData.h"
#include "resource.h"
#include "foo_discogs.h"
#include "tags.h"
#include "my_editwithbuttons.h"

static const char* TEXT_WRITE = "write";
static const char* TEXT_UPDATE = "update";
static const char* TEXT_DISABLED = "disabled";
static const char* TEXT_WRITE_UPDATE = "write + update";

class CTagMappingListControlOwnerData : public CListControlOwnerData {

public:

	CTagMappingListControlOwnerData(IListControlOwnerDataSource* h)
		: CListControlOwnerData(h) {}

	~CTagMappingListControlOwnerData() {
		//
	}

	void RenderHLBackground(CDCHandle p_dc, const CRect& p_itemRect, size_t item, uint32_t bkColor, pfc::string8 str, pfc::string8 hlstr, bool freeze) {
		
		const Gdiplus::Color gdiHLColor = Gdiplus::Color(255, 180, 180, 180);
		const Gdiplus::Color gdiROColor = Gdiplus::Color(255, 240, 240, 240);

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

		if (do_hlight)
			RenderHLBackground(p_dc, p_itemRect, item, bkColor, strItem, m_hl_string, false);
		else {
			if (bfreeze) {
				pfc::bit_array_bittable selmask = GetSelectionMask();
				bool selected = selmask.get(item);
				if (!selected)
					RenderHLBackground(p_dc, p_itemRect, item, bkColor, strItem, m_hl_string, true);
			}
		}
	}

	void SetHLString(pfc::string8 hlstring, bool erase = 1) {
		m_hl_string = hlstring;
		this->Invalidate(erase);
	}

private:
	pfc::string8 m_hl_string;
};

class CTagMappingDialog : public MyCDialogImpl<CTagMappingDialog>,	public CDialogResize<CTagMappingDialog>,
	public CMessageFilter,	private IListControlOwnerDataSource
{

private:

	foo_conf conf;
	tag_mapping_list_type* m_ptag_mappings = nullptr;

#ifdef CAT_CRED
	vppair v_cat_defs;
#endif // CAT_CRED

	CTagMappingListControlOwnerData m_tag_list;
	CMyEditWithButtons cewb_highlight;
	CHyperLink help_link;

	HWND remove_button;
	UINT default_button;

	void set_tag_mapping(tag_mapping_list_type* ptag_mapping) {
		m_ptag_mappings = ptag_mapping;
	}

	void show_context_menu(CPoint& pt, pfc::bit_array_bittable& selmask);
	void update_list_width();
	bool update_tag(int pos, const tag_mapping_entry* entry);
	bool update_freezer(int pos, bool enable_write, bool enable_update);

	void applymappings();

// LIBPPUI list: IListControlOwnerDataSource 

	//- Get Item Count

	size_t listGetItemCount(ctx_t ctx) override {
		//pointer to object calling us
		PFC_ASSERT(ctx == &m_tag_list);
		return m_ptag_mappings->get_count();
	}

	//- Get subitems

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

	//- Can reorder

	bool listCanReorderItems(ctx_t) override {
		return true;
	}

	//- Reorder

	bool listReorderItems(ctx_t ctx, const size_t* order, size_t count) override {
		pfc::reorder_t(*m_ptag_mappings, order, m_ptag_mappings->get_count());
		on_mapping_changed(check_mapping_changed());
		return true;
	}

	//- Remove

	bool listRemoveItems(ctx_t ctx, pfc::bit_array const& mask) override {
		
		//remove not freezed masked
		size_t walk = 0;
		size_t deleted = 0;
		while (walk = mask.find_next(true, walk, m_ptag_mappings->get_count())) {
			if (walk >= m_ptag_mappings->get_count()) break;
			auto entry = m_ptag_mappings->get_item(walk - deleted);
			if (!entry.freeze_tag_name) {
				m_ptag_mappings->remove_by_idx(walk - deleted);
				++deleted;
			}
		}
		on_mapping_changed(check_mapping_changed());
		return true;
	}

	//- Item action

	void listItemAction(ctx_t, size_t item) override {
		m_tag_list.TableEdit_Start(item, 1);
	}

	//- Subitem action

	void listSubItemClicked(ctx_t, size_t item, size_t subItem) override {
		tag_mapping_entry& entry = (*m_ptag_mappings)[item];
		if ((!entry.freeze_tag_name) && (subItem == 0 || subItem == 1)) {	
			m_tag_list.TableEdit_Start(item, subItem);
		}
		if (subItem == 2) {
			CPoint p = ::GetCursorPos();
			LPARAM lparam = MAKELPARAM(p.x, p.y);
			OnContextMenu(this->IDD, (WPARAM)(HWND)m_tag_list, lparam);
		}
	}

	//- Set edited text

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

	//- Can edit

	bool listIsColumnEditable(ctx_t, size_t subItem) override {
		return (subItem == 0 || subItem == 1);
	}

//END LIBPPUI list methods


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

public:

	enum {
		IDD = IDD_DIALOG_TAG_MAPPINGS,
		MSG_ADD_NEW
	};

#ifdef CAT_CRED
	void SetVDefs(vppair v) {
		v_cat_defs = v;
	}
#endif // CAT_CRED

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

	void load_size();
	bool build_current_cfg();
	void pushcfg(bool force);
	bool check_mapping_changed();
	void on_mapping_changed(bool changed);

#pragma warning( push )
#pragma warning( disable : 26454 )

	MY_BEGIN_MSG_MAP(CTagMappingDialog)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnButtonNext)
		COMMAND_ID_HANDLER(IDC_APPLY, OnApply)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		COMMAND_ID_HANDLER(IDC_EDIT_TAG_MATCH_HL, OnEditHLText)
		COMMAND_ID_HANDLER(IDC_SPLIT_BTN_TAG_ADD_NEW, OnSplitDropDown)
		COMMAND_ID_HANDLER(IDC_SPLIT_BTN_TAG_CAT_CREDIT, OnSplitDropDown)
		COMMAND_ID_HANDLER(IDC_SPLIT_BTN_TAG_FILE_DEF, OnSplitDropDown)
		COMMAND_ID_HANDLER(IDC_REMOVE_TAG, OnBtnRemoveTag)

#ifdef CAT_CRED
		COMMAND_ID_HANDLER(IDC_SPLIT_BTN_TAG_CAT_CREDIT, OnBtnCreditsClick)
#endif // CAT_CRED

		MESSAGE_HANDLER_EX(MSG_ADD_NEW, OnAddNewTag)
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
		DLGRESIZE_CONTROL(IDC_SYNTAX_HELP, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_REMOVE_TAG, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_SPLIT_BTN_TAG_CAT_CREDIT, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_TAG_LIST, DLSZ_SIZE_X | DLSZ_SIZE_Y)
	END_DLGRESIZE_MAP()

	void DlgResize_UpdateLayout(int cxWidth, int cyHeight) {
		CDialogResize<CTagMappingDialog>::DlgResize_UpdateLayout(cxWidth, cyHeight);
	}

	CTagMappingDialog::CTagMappingDialog(HWND p_parent, UINT default_button = IDOK)
		: default_button(default_button), cewb_highlight(), m_tag_list(this), remove_button(nullptr) {

		set_tag_mapping(copy_tag_mappings());
	}

	CTagMappingDialog::~CTagMappingDialog() {

		g_discogs->tag_mappings_dialog = nullptr;
		if (m_ptag_mappings != nullptr) {
			delete m_ptag_mappings;
			m_ptag_mappings = nullptr;
		}
	}

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnButtonNext(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnApply(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnEditHLText(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnDefaults(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnImport(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnExport(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBtnRemoveTag(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
#ifdef CAT_CRED
	LRESULT OnBtnCreditsClick(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
#endif // CAT_CRED

	
	//LRESULT OnSplitDropDown(LPNMHDR lParam);
	LRESULT OnSplitDropDown(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnAddNewTag(UINT, WPARAM, LPARAM);
	//LRESULT OnAddNew(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void enable(bool v) override {}
};
