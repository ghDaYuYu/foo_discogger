#pragma once

#include "../SDK/foobar2000.h"
#include "foo_discogs.h"
#include "resource.h"
#include "tags.h"
#include "ceditwithbuttonsdim.h"

class CNewTagMappingsDialog : public MyCDialogImpl<CNewTagMappingsDialog>,
	public CDialogResize<CNewTagMappingsDialog>,
	public CMessageFilter,
	private InPlaceEdit::CTableEditHelperV2_ListView
{

private:

	HWND tag_list;
	HWND remove_button;
	foo_discogs_conf conf;

	metadb_handle_list items;
	tag_mappings_list_type *tag_mappings = nullptr;

	CHyperLink help_link;

	pfc::string8 highlight_label;
	CEditWithButtonsDim cewb_highlight;

	void insert_tag_mappings();
	void insert_tag(int pos, const tag_mapping_entry *entry);
	void update_list_width(bool initialize=false);
	void update_tag(int pos, const tag_mapping_entry *entry);
	void show_context_menu(CPoint &pt, int selection);

	void applymappings();

	bool TableEdit_IsColumnEditable(size_t subItem) const override {
		return subItem == 0 || subItem == 1;
	}

	size_t TableEdit_GetColumnCount() const override {
		return 3;
	}

	HWND TableEdit_GetParentWnd() const override {
		return tag_list;
	}

	void TableEdit_SetField(size_t item, size_t subItem, const char * value) override {
		CTableEditHelperV2_ListView::TableEdit_SetField(item, subItem, value);
		if (item < tag_mappings->get_count()) {
			if (subItem == 0) {
				(*tag_mappings)[item].tag_name = value;
			}
			else if (subItem == 1) {
				(*tag_mappings)[item].formatting_script = value;
			}
		}
	}

	void TableEdit_Finished() {
		on_mapping_changed(get_mapping_changed());
	}

	void trigger_add_new() {
		PostMessage(MSG_ADD_NEW, 0, 0);
	}

	void trigger_edit(size_t item, size_t sub_item) {
		PostMessage(MSG_EDIT, item, sub_item);
	}

	bool delete_selection() {
		const int index = ListView_GetSingleSelection(tag_list);
		if (index < 0) {
			return false;
		}
		const tag_mapping_entry &entry = tag_mappings->get_item_ref(index);
		if (entry.freeze_tag_name) {
			return false;
		}
		delete_entry(index);
		tag_mappings->remove_by_idx(index);
		
		on_mapping_changed(get_mapping_changed());
		return true;
	}

	void delete_entry(int index) {
		ListView_DeleteItem(tag_list, index);

		int total = ListView_GetItemCount(tag_list);
		if (index < total) {
			listview_helper::select_single_item(tag_list, index);
		}
		else if (total > 0) {
			listview_helper::select_single_item(tag_list, total - 1);
		}
	}

public:

	enum {
		IDD = IDD_DIALOG_TAG_MAPPINGS,
		MSG_EDIT = WM_APP,
		MSG_ADD_NEW
	};

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
	bool get_mapping_changed();
	void on_mapping_changed(bool changed);


	MY_BEGIN_MSG_MAP(CNewTagMappingsDialog)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDAPPLY, OnApply)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		COMMAND_ID_HANDLER(IDC_EDIT_TAG_MATCH_HL, OnEditHLText)
		COMMAND_ID_HANDLER(IDC_DEFAULT_TAGS, OnDefaults)
		COMMAND_ID_HANDLER(IDC_IMPORT_TAGS, OnImport)
		COMMAND_ID_HANDLER(IDC_EXPORT_TAGS, OnExport)
		COMMAND_ID_HANDLER(IDC_ADD_TAG, OnAddTag)
		COMMAND_ID_HANDLER(IDC_REMOVE_TAG, OnRemoveTag)
		NOTIFY_HANDLER_EX(IDC_TAG_LIST, LVN_ITEMCHANGED, OnListItemChanged)
		NOTIFY_HANDLER_EX(IDC_TAG_LIST, NM_CLICK, OnListClick)
		NOTIFY_HANDLER_EX(IDC_TAG_LIST, NM_DBLCLK, OnListDoubleClick)
		NOTIFY_HANDLER_EX(IDC_TAG_LIST, LVN_KEYDOWN, OnListKeyDown)
		MESSAGE_HANDLER_EX(MSG_EDIT, OnEdit)
		MESSAGE_HANDLER_EX(MSG_ADD_NEW, OnAddNew)
		MESSAGE_HANDLER_EX(WM_CONTEXTMENU, OnContextMenu)
		NOTIFY_HANDLER(IDC_TAG_LIST, NM_CUSTOMDRAW, OnCustomDraw)
		CHAIN_MSG_MAP(CDialogResize<CNewTagMappingsDialog>)
	MY_END_MSG_MAP()

	BEGIN_DLGRESIZE_MAP(CNewTagMappingsDialog)
		DLGRESIZE_CONTROL(IDC_DEFAULT_TAGS, DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_IMPORT_TAGS, DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_EXPORT_TAGS, DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDAPPLY, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_SYNTAX_HELP, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_ADD_TAG, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_REMOVE_TAG, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_TAG_LIST, DLSZ_SIZE_X | DLSZ_SIZE_Y)
	END_DLGRESIZE_MAP()

	void DlgResize_UpdateLayout(int cxWidth, int cyHeight) {
		CDialogResize<CNewTagMappingsDialog>::DlgResize_UpdateLayout(cxWidth, cyHeight);
	}

	CNewTagMappingsDialog::CNewTagMappingsDialog(HWND p_parent)
		: cewb_highlight(L" Highlight Tag"), tag_list(nullptr), remove_button(nullptr) {

		tag_mappings = copy_tag_mappings();
	}

	CNewTagMappingsDialog::~CNewTagMappingsDialog() {

		g_discogs->tag_mappings_dialog = nullptr;
		if (tag_mappings != nullptr) {
			delete tag_mappings;
			tag_mappings = nullptr;
		}
	}

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnApply(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnEditHLText(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnDefaults(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnImport(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnExport(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnAddTag(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnRemoveTag(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnListItemChanged(LPNMHDR lParam);
	LRESULT OnListClick(LPNMHDR lParam);
	LRESULT OnListDoubleClick(LPNMHDR lParam);
	LRESULT OnListKeyDown(LPNMHDR lParam);
	LRESULT OnColumnResized(LPNMHDR lParam);
	LRESULT OnEdit(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnAddNew(UINT, WPARAM, LPARAM);
	LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnCustomDraw(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

	void refresh_item(int pos);

	void enable(bool v) override {}
};
