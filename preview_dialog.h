#pragma once

#include "../SDK/foobar2000.h"
#include "foo_discogs.h"
#include "resource.h"
#include "tag_writer.h"

#ifndef PREVIEW_STATS_H
#define PREVIEW_STATS_H

struct preview_stats {
	int totalwrites = 0;
	int totalupdates = 0;
	int totalsubwrites = 0;
	int totalsubupdates = 0;
	int totalskips = 0;
	int totalequal = 0;
};

#endif //PREVIEW_STATS_H

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
	
	//preview_stats
	std::vector<preview_stats> v_stats;
	void reset_default_columns(HWND wndlist, bool breset);
	std::vector<std::pair<int, int>> vec_icol_subitems;
	void update_sorted_icol_map(bool reset);

	bool generating_tags = false;
	
	int totalwrites = 0; int totalupdates = 0;

	bool cfg_preview_dialog_show_stats = false;

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

	void compute_stats(tag_results_list_type tag_results);
	void compute_stats_v23(tag_results_list_type tag_results);
	void compute_stats_track_map(tag_results_list_type tag_results);
	void reset_stats () {
		v_stats.clear();
	}
	void reset_tag_result_stats();
	void display_tag_result_stats();

	void set_preview_mode(int mode);
	int get_preview_mode();

	bool initialize();
	void GlobalReplace_ANV(bool state);

	bool TableEdit_IsColumnEditable(size_t subItem) const override {
		return subItem == 1;
	}

	size_t TableEdit_GetColumnCount() const override {
		return ListView_GetColumnCount(tag_results_list);
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
		COMMAND_ID_HANDLER(IDC_CHECK_PREV_DLG_SKIP_ARTWORK, OnCheckSkipArtwork)
		COMMAND_ID_HANDLER(IDC_CHECK_PREV_DLG_SHOW_STATS, OnCheckPreviewShowStats)
		COMMAND_ID_HANDLER(IDC_EDIT_TAG_MAPPINGS_BUTTON, OnEditTagMappings)
		COMMAND_ID_HANDLER(IDC_VIEW_NORMAL, OnChangePreviewMode)
		COMMAND_ID_HANDLER(IDC_VIEW_DIFFERENCE, OnChangePreviewMode)
		COMMAND_ID_HANDLER(IDC_VIEW_ORIGINAL, OnChangePreviewMode)
		COMMAND_ID_HANDLER(IDC_VIEW_DEBUG, OnChangePreviewMode)
		NOTIFY_HANDLER_EX(IDC_PREVIEW_LIST, NM_CLICK, OnListClick)
		NOTIFY_HANDLER_EX(IDC_PREVIEW_LIST, LVN_KEYDOWN, OnListKeyDown)
		MESSAGE_HANDLER_EX(MSG_EDIT, OnEdit)
		NOTIFY_HANDLER(IDC_PREVIEW_LIST, NM_CUSTOMDRAW, OnCustomDraw)
		CHAIN_MSG_MAP(CDialogResize<CPreviewTagsDialog>)
		MY_END_MSG_MAP()

		BEGIN_DLGRESIZE_MAP(CPreviewTagsDialog)
			DLGRESIZE_CONTROL(IDC_ALBUM_ART, DLSZ_MOVE_X)
			DLGRESIZE_CONTROL(IDC_OPTIONS_GROUP, DLSZ_MOVE_X)
			DLGRESIZE_CONTROL(IDC_REPLACE_ANV_CHECK, DLSZ_MOVE_X)
			DLGRESIZE_CONTROL(IDC_CHECK_PREV_DLG_SKIP_ARTWORK, DLSZ_MOVE_X)
			DLGRESIZE_CONTROL(IDC_CHECK_PREV_DLG_SHOW_STATS, DLSZ_MOVE_X)
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
			DLGRESIZE_CONTROL(IDC_PREVIEW_STATS, DLSZ_MOVE_X)
			DLGRESIZE_CONTROL(IDC_STATIC_PREV_DLG_STATS_TAG_UPD, DLSZ_MOVE_X)
		END_DLGRESIZE_MAP()

		void DlgResize_UpdateLayout(int cxWidth, int cyHeight) {
			CDialogResize<CPreviewTagsDialog>::DlgResize_UpdateLayout(cxWidth, cyHeight);
		}

		CPreviewTagsDialog(HWND p_parent, TagWriter_ptr tag_writer, bool use_update_tags)
			: tag_writer(tag_writer), tag_results_list(NULL), use_update_tags(use_update_tags) {

			g_discogs->preview_tags_dialog = this;
		}
		CPreviewTagsDialog(HWND p_parent, pfc::array_t<TagWriter_ptr> tag_writers, bool use_update_tags)
			: tag_writers(tag_writers), tag_results_list(NULL), use_update_tags(use_update_tags), multi_mode(true) {

			g_discogs->preview_tags_dialog = this;
			if (init_count()) {
			
			}
			else {
				finished_tag_writers();
				destroy();
			}
		}
		~CPreviewTagsDialog();

		LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
		LRESULT OnWriteTags(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
		LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
		LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
		LRESULT OnMultiSkip(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
		LRESULT OnBack(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
		LRESULT OnCheckReplaceANVs(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
		LRESULT OnCheckTrackMap(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
		LRESULT OnCheckSkipArtwork(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
		LRESULT OnCheckPreviewShowStats(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
		LRESULT OnEditTagMappings(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
		LRESULT OnChangePreviewMode(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
		LRESULT OnListClick(LPNMHDR lParam);
		LRESULT OnListKeyDown(LPNMHDR lParam);
		LRESULT OnEdit(UINT uMsg, WPARAM wParam, LPARAM lParam);
		LRESULT OnCustomDraw(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

		void refresh_item(int pos);

		void insert_tag_results(bool computestat);
		void tag_mappings_updated();
		void cb_generate_tags();
		void enable(bool v) override { enable(v, true); };
		void enable(bool v, bool change_focus);
		bool write_tag_button_enabled();
		void destroy_all();
		void go_back();
};
