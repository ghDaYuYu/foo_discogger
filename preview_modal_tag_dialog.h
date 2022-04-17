#pragma once
#include "resource.h"

#include "my_tabentry.h"
#include "tag_writer.h"

#include "libPPUI\CListControlOwnerData.h"
#include "preview_modal_tag_dialog_ILO_uilist.h"

class CPreviewModalTagDialog : public MyCDialogImpl<CPreviewModalTagDialog>,
	public CDialogResize<CPreviewModalTagDialog>,
	private ILOD_preview_modal
{

private:

	pfc::array_t<tab_entry> m_tab_table;

	CListControlOwnerData m_ui_list;

	int m_parent_preview_mode;

	size_t m_iItem;
	tag_result_ptr m_item_result;
	std::vector<pfc::string8> m_vtracks_desc;

	void init_tabs_defs();
	int get_tabctrl_uid() { return IDD_TAB_CTRL; }
	size_t get_current_tab_index();

	bool context_menu_show(HWND wnd, size_t isel, LPARAM lParamCoords);
	bool context_menu_switch(HWND wnd, POINT point, bool is_files, int cmd, bit_array_bittable selmask /*, pfc::array_t<t_size> order, CListControlOwnerData* ilist*/ );


public:

	enum {
		IDD = IDD_DIALOG_PREVIEW_MODAL_TAG,
		IDD_TAB_CTRL = IDC_PREVIEW_MODAL_TAB,
		IDD_TAB_LIST = IDC_MODAL_TAG_LIST,
		MY_NUM_TABS = 3,
		ORD_COL_DEF_WITH = 40,
	};

	enum {
		ID_CMD_COPY = 1, ID_CMD_PASTE
	};

	void update_list_width();
	void load_size();
	bool build_current_cfg(int& out);
	void pushcfg();

	MY_BEGIN_MSG_MAP(CPreviewModalTagDialog)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		NOTIFY_HANDLER(IDC_PREVIEW_MODAL_TAB, TCN_SELCHANGING, OnChangingTab)
		NOTIFY_HANDLER(IDC_PREVIEW_MODAL_TAB, TCN_SELCHANGE, OnChangeTab)
		NOTIFY_HANDLER(IDD_TAB_LIST, NM_CLICK, OnListClick)
		CHAIN_MSG_MAP(CDialogResize<CPreviewModalTagDialog>)
	MY_END_MSG_MAP()

	BEGIN_DLGRESIZE_MAP(CPreviewModalTagDialog)
		DLGRESIZE_CONTROL(IDC_PREVIEW_MODAL_TAB, DLSZ_SIZE_X | DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_MODAL_TAG_LIST, DLSZ_SIZE_X | DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_PREVIEW_MODAL_TAG_NAME, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_PREVIEW_MODAL_TAG_STATIC_FOOTNOTE, DLSZ_MOVE_Y)
	END_DLGRESIZE_MAP()


	const std::vector<pfc::string8>* get_vtracks_desc() { return &m_vtracks_desc; }

	void DlgResize_UpdateLayout(int cxWidth, int cyHeight) {
		CDialogResize<CPreviewModalTagDialog>::DlgResize_UpdateLayout(cxWidth, cyHeight);
	}

	//constructor

	CPreviewModalTagDialog(HWND p_parent, size_t item, tag_result_ptr item_result, std::vector<pfc::string8> vtracks_desc, int parent_preview_mode)
		: m_iItem(item), m_item_result(item_result), m_vtracks_desc(vtracks_desc), m_parent_preview_mode(parent_preview_mode), m_ui_list(this),
		ILOD_preview_modal(&this->m_ui_list, item_result, &m_vtracks_desc, IDD_TAB_CTRL, 0)
	{

		//..
	}

	~CPreviewModalTagDialog();

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT OnChangingTab(WORD /*wNotifyCode*/, LPNMHDR /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnChangeTab(WORD /*wNotifyCode*/, LPNMHDR /*lParam*/, BOOL& /*bHandled*/); 
	LRESULT OnListClick(WORD /*wNotifyCode*/, LPNMHDR /*lParam*/, BOOL& /*bHandled*/);

	void enable(bool v) override { enable(v, true); };
	void enable(bool v, bool change_focus);
};

