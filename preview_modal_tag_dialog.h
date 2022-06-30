#pragma once
#include "resource.h"

#include "my_tabentry.h"
#include "tag_writer.h"

#include "libPPUI\CListControlOwnerData.h"
#include "preview_modal_tag_ILO.h"

class CPreviewModalTagDialog : public MyCDialogImpl<CPreviewModalTagDialog>,
	public CDialogResize<CPreviewModalTagDialog>,
	private ILOD_preview_modal {

private:

	pfc::array_t<tab_entry> m_tab_table;
	CListControlOwnerData m_ui_list;

	PreView m_parent_preview_mode;

	TagWriter_ptr m_tag_writer;
	service_ptr_t<titleformat_object> m_track_desc_script;
	size_t m_isel;
	tag_result_ptr m_item_result;
	std::vector<pfc::string8> m_vtracks_desc;
	
	void init_results();
	void init_tabs_defs();
	int get_tabctrl_uid() { return IDD_TAB_CTRL; }
	size_t get_current_tab_index();
	void push_updates();

	bool context_menu_show(HWND wnd, size_t isel, LPARAM lParamPos);
	bool context_menu_switch(HWND wnd, POINT point, bool is_files, int cmd, bit_array_bittable selmask /*, pfc::array_t<t_size> order, CListControlOwnerData* ilist*/ );

	//uilist ILO
	virtual CListControlOwnerData* ilo_get_ui_list() override { return &m_ui_list; }
	virtual PreView ilo_get_view_mode() override { return m_parent_preview_mode; }
	virtual tag_result_ptr ilo_get_item_result() override { return m_item_result; }
	virtual const std::vector<pfc::string8>& ilo_get_vtracks_desc() override { return m_vtracks_desc; }
	virtual const size_t ilo_get_finfo_count() override { return m_tag_writer->finfo_manager->get_item_count(); }
	//

public:

	enum {
		IDD = IDD_DIALOG_PREVIEW_MODAL_TAG,
		IDD_TAB_CTRL = IDC_TAB_PREVIEW_MODAL,
		IDD_TAB_LIST = IDC_MODAL_TAG_LIST,
		MY_NUM_TABS = 3,
		ORD_COL_DEF_WITH = 40,
	};

	enum {
		ID_CMD_EDIT = 1, ID_CMD_COPY, ID_CMD_PASTE, ID_CMD_APPLY
	};

	void update_list_width();
	void load_size();
	bool build_current_cfg(int& out);
	void pushcfg();

#pragma warning( push )
#pragma warning( disable : 26454 )
	MY_BEGIN_MSG_MAP(CPreviewModalTagDialog)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDC_APPLY, OnApply)
		NOTIFY_HANDLER(IDC_TAB_PREVIEW_MODAL, TCN_SELCHANGE, OnChangeTab)
		NOTIFY_HANDLER(IDC_TAB_PREVIEW_MODAL, TCN_SELCHANGING, OnChangingTab)
		CHAIN_MSG_MAP(CDialogResize<CPreviewModalTagDialog>)
	MY_END_MSG_MAP()
#pragma warning(pop)

	BEGIN_DLGRESIZE_MAP(CPreviewModalTagDialog)
		DLGRESIZE_CONTROL(IDC_TAB_PREVIEW_MODAL, DLSZ_SIZE_X | DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_MODAL_TAG_LIST, DLSZ_SIZE_X | DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_PREVIEW_MODAL_TAG_NAME, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_APPLY, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_STATIC_PREV_MODAL_TAG_FOOTNOTE, DLSZ_MOVE_Y)
	END_DLGRESIZE_MAP()

	void DlgResize_UpdateLayout(int cxWidth, int cyHeight) {
		CDialogResize<CPreviewModalTagDialog>::DlgResize_UpdateLayout(cxWidth, cyHeight);
	}

	//constructor

	CPreviewModalTagDialog(HWND p_parent, TagWriter_ptr tag_writer, size_t isel, PreView parent_preview_mode)
		: m_tag_writer(tag_writer), m_isel(isel), m_parent_preview_mode(parent_preview_mode), m_ui_list(this),
		ILOD_preview_modal(tag_writer->tag_results[isel], parent_preview_mode) {

		g_discogs->preview_modal_tag_dialog = this;

		static_api_ptr_t<titleformat_compiler>()->compile_force(m_track_desc_script, "[[%album artist%]] - [[%discnumber% .]%tracknumber% -] [%track artist% -] %title% ");
		init_results();
	}

	~CPreviewModalTagDialog();

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	size_t GetResult(tag_result_ptr& item_result) { item_result = m_item_result; return m_isel; };
	void SetTagWriter(TagWriter_ptr tag_writer) { m_tag_writer = tag_writer; }
	void ReloadItem(HWND p_parent, size_t item, PreView parent_preview_mode);

	LRESULT OnContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnApply(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT OnChangingTab(WORD /*wNotifyCode*/, LPNMHDR /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnChangeTab(WORD /*wNotifyCode*/, LPNMHDR /*lParam*/, BOOL& /*bHandled*/); 

	void enable(bool v) override { enable(v, true); };
	void enable(bool v, bool change_focus);
};