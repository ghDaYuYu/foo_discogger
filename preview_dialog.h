#pragma once

#include "../SDK/foobar2000.h"
#include "foo_discogs.h"
#include "resource.h"
#include "tag_writer.h"


class CPreviewTagsDialog : public MyCDialogImpl<CPreviewTagsDialog>,
	public CDialogResize<CPreviewTagsDialog>,
	public CMessageFilter,
	private InPlaceEdit::CTableEditHelperV2_ListView
{
private:
	metadb_handle_list items;
	HWND tag_results_list;

	foo_discogs_conf conf;

	bool use_update_tags = false;
	bool multi_mode = false;
	size_t multi_count = 0;

	bool generating_tags = false;

	TagWriter_ptr tag_writer;
	pfc::array_t<TagWriter_ptr> tag_writers;
	size_t tw_index = 0;
	size_t tw_skip = 0;

	bool init_count();
	bool get_next_tag_writer();
	void finished_tag_writers();

	void update_window_title() {
		pfc::string8 text;
		text << "Preview Tags (" << tw_index - tw_skip << "/" << multi_count << ")";
		pfc::stringcvt::string_wide_from_ansi wtext(text);
		SetWindowText((LPCTSTR)const_cast<wchar_t*>(wtext.get_ptr()));
	}

	void insert_tag_result(int pos, const tag_result_ptr &result);
	void set_preview_mode(int mode);
	int get_preview_mode();

	bool initialize();
	void GlobalReplace_ANV(bool state);

	bool TableEdit_IsColumnEditable(size_t subItem) const override {
		return subItem == 1;
	}

	size_t TableEdit_GetColumnCount() const override {
		return 2;
	}

	HWND TableEdit_GetParentWnd() const override {
		return tag_results_list;
	}

	void TableEdit_GetField(size_t item, size_t subItem, pfc::string_base & out, size_t & lineCount) override;

	void TableEdit_SetField(size_t item, size_t subItem, const char * value) override;

	void trigger_edit(size_t item, size_t sub_item) {
		PostMessage(MSG_EDIT, item, sub_item);
	}
	bool delete_selection();

public:
	enum {
		IDD = IDD_DIALOG_PREVIEW_TAGS,
		MSG_EDIT = WM_APP
	};

	virtual BOOL PreTranslateMessage(MSG* pMsg) override {
		return ::IsDialogMessage(m_hWnd, pMsg);
	}

	void load_size();
	bool build_current_cfg();
	void pushcfg();

	MY_BEGIN_MSG_MAP(CPreviewTagsDialog)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDC_WRITE_TAGS_BUTTON, OnWriteTags)
		COMMAND_ID_HANDLER(IDC_BACK_BUTTON, OnBack)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_ID_HANDLER(IDC_SKIP_BUTTON, OnMultiSkip)
		COMMAND_ID_HANDLER(IDC_REPLACE_ANV_CHECK, OnCheckReplaceANVs)
		COMMAND_ID_HANDLER(IDC_EDIT_TAG_MAPPINGS_BUTTON, OnEditTagMappings)
		COMMAND_ID_HANDLER(IDC_VIEW_NORMAL, OnChangePreviewMode)
		COMMAND_ID_HANDLER(IDC_VIEW_DIFFERENCE, OnChangePreviewMode)
		COMMAND_ID_HANDLER(IDC_VIEW_ORIGINAL, OnChangePreviewMode)
		COMMAND_ID_HANDLER(IDC_VIEW_DEBUG, OnChangePreviewMode)
		//NOTIFY_HANDLER_EX(IDC_PREVIEW_LIST, LVN_ITEMCHANGED, OnListItemChanged)
		NOTIFY_HANDLER_EX(IDC_PREVIEW_LIST, NM_CLICK, OnListClick)
		//NOTIFY_HANDLER_EX(IDC_PREVIEW_LIST, NM_DBLCLK, OnListDoubleClick)
		NOTIFY_HANDLER_EX(IDC_PREVIEW_LIST, LVN_KEYDOWN, OnListKeyDown)
		MESSAGE_HANDLER_EX(MSG_EDIT, OnEdit)
		//MESSAGE_HANDLER_EX(WM_CONTEXTMENU, OnContextMenu)
		NOTIFY_HANDLER(IDC_PREVIEW_LIST, NM_CUSTOMDRAW, OnCustomDraw)
		CHAIN_MSG_MAP(CDialogResize<CPreviewTagsDialog>)
		MY_END_MSG_MAP()

		BEGIN_DLGRESIZE_MAP(CPreviewTagsDialog)
			DLGRESIZE_CONTROL(IDC_ALBUM_ART, DLSZ_MOVE_X)
			DLGRESIZE_CONTROL(IDC_OPTIONS_GROUP, DLSZ_MOVE_X)
			DLGRESIZE_CONTROL(IDC_REPLACE_ANV_CHECK, DLSZ_MOVE_X)
			DLGRESIZE_CONTROL(IDC_VIEW_GROUP, DLSZ_MOVE_X)
			DLGRESIZE_CONTROL(IDC_VIEW_NORMAL, DLSZ_MOVE_X)
			DLGRESIZE_CONTROL(IDC_VIEW_DIFFERENCE, DLSZ_MOVE_X)
			DLGRESIZE_CONTROL(IDC_VIEW_ORIGINAL, DLSZ_MOVE_X)
			DLGRESIZE_CONTROL(IDC_VIEW_DEBUG, DLSZ_MOVE_X)
			DLGRESIZE_CONTROL(IDC_EDIT_TAG_MAPPINGS_BUTTON, DLSZ_MOVE_Y)
			DLGRESIZE_CONTROL(IDC_BACK_BUTTON, DLSZ_MOVE_Y)
			DLGRESIZE_CONTROL(IDC_WRITE_TAGS_BUTTON, DLSZ_MOVE_Y)
			DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_Y)
			DLGRESIZE_CONTROL(IDC_SKIP_BUTTON, DLSZ_MOVE_Y)
			DLGRESIZE_CONTROL(IDC_BACK_BUTTON, DLSZ_MOVE_X)
			DLGRESIZE_CONTROL(IDC_WRITE_TAGS_BUTTON, DLSZ_MOVE_X)
			DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X)
			DLGRESIZE_CONTROL(IDC_SKIP_BUTTON, DLSZ_MOVE_X)
			DLGRESIZE_CONTROL(IDC_PREVIEW_LIST, DLSZ_SIZE_X | DLSZ_SIZE_Y)
		END_DLGRESIZE_MAP()

		void DlgResize_UpdateLayout(int cxWidth, int cyHeight) {
			CDialogResize<CPreviewTagsDialog>::DlgResize_UpdateLayout(cxWidth, cyHeight);
		}

		CPreviewTagsDialog(HWND p_parent, TagWriter_ptr tag_writer, bool use_update_tags) : tag_writer(tag_writer), use_update_tags(use_update_tags) {
			g_discogs->preview_tags_dialog = this;
			Create(p_parent);
		}
		CPreviewTagsDialog(HWND p_parent, pfc::array_t<TagWriter_ptr> tag_writers, bool use_update_tags) : tag_writers(tag_writers), use_update_tags(use_update_tags), multi_mode(true) {
			g_discogs->preview_tags_dialog = this;
			if (init_count()) {
				Create(p_parent);
			}
			else {
				finished_tag_writers();
				destroy();
			}
		}
		~CPreviewTagsDialog();
		void CPreviewTagsDialog::OnFinalMessage(HWND /*hWnd*/) override;

		LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
		LRESULT OnWriteTags(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
		LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
		LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
		LRESULT OnMultiSkip(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
		LRESULT OnBack(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
		LRESULT OnCheckReplaceANVs(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
		LRESULT OnEditTagMappings(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
		LRESULT OnChangePreviewMode(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
		//LRESULT OnDefaults(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
		//LRESULT OnListItemChanged(LPNMHDR lParam);
		LRESULT OnListClick(LPNMHDR lParam);
		//LRESULT OnListDoubleClick(LPNMHDR lParam);
		LRESULT OnListKeyDown(LPNMHDR lParam);
		LRESULT OnEdit(UINT uMsg, WPARAM wParam, LPARAM lParam);
		//LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam);
		LRESULT CPreviewTagsDialog::OnCustomDraw(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

		void refresh_item(int pos);

		void insert_tag_results();
		void tag_mappings_updated();
		void cb_generate_tags();

		void enable(bool v) override;
		void destroy_all();
		void go_back();
};
