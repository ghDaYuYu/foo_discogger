#pragma once

#include "../SDK/foobar2000.h"
#include "foo_discogs.h"
#include "resource.h"
#include "file_info_manager.h"
#include "multiformat.h"
#include "tag_writer.h"

#include "track_matching_discogs_listctrl.h"

using namespace Discogs;


class CTrackMatchingDialog : public MyCDialogImpl<CTrackMatchingDialog>,
	public CDialogResize<CTrackMatchingDialog>,
	public CMessageFilter
{
private:
	bool conf_changed = false;
	foo_discogs_conf conf;
	//foo_discogs_conf conf_dlg_edit;

	bool use_update_tags = false;
	bool multi_mode = false;
	size_t multi_count = 0;

	TagWriter_ptr tag_writer;
	pfc::array_t<TagWriter_ptr> tag_writers;
	size_t tw_index = 0;
	size_t tw_skip = 0;

	bool init_count();
	void finished_tag_writers();

	void update_window_title() {
		pfc::string8 text;
		text << "Match Tracks (" << tw_index - tw_skip << "/" << multi_count << ")";
		pfc::stringcvt::string_wide_from_ansi wtext(text);
		SetWindowText((LPCTSTR)const_cast<wchar_t*>(wtext.get_ptr()));
	}

	file_info_impl info;
	playable_location_impl location;
	titleformat_hook_impl_multiformat hook;

	HWND discogs_track_list, file_list;
	HWND match_failed, match_assumed, match_success;

	void load_size();
	void save_size(int x, int y);

	void insert_track_mappings();
	void list_swap_items(HWND list, unsigned int pos1, unsigned int pos2);

	void remove_selected_items(HWND list);
	void move_selected_items_up(HWND list);
	void move_selected_items_down(HWND list);

	void generate_track_mappings(track_mappings_list_type &track_mappings);

	void update_list_width(HWND list, bool initialize=false);

public:
	enum { IDD = IDD_DIALOG_MATCH_TRACKS };

	virtual BOOL PreTranslateMessage(MSG* pMsg) override {
		return ::IsDialogMessage(m_hWnd, pMsg);
	}

	MY_BEGIN_MSG_MAP(CTrackMatchingDialog)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnColorStatic)
		COMMAND_ID_HANDLER(IDC_MOVE_UP_BUTTON, OnMoveTrackUp)
		COMMAND_ID_HANDLER(IDC_MOVE_DOWN_BUTTON, OnMoveTrackDown)
		COMMAND_ID_HANDLER(IDC_REMOVE_DISCOGS_TRACK_BUTTON, OnRemoveTrackButton)
		COMMAND_ID_HANDLER(IDC_REMOVE_FILE_TRACK_BUTTON, OnRemoveTrackButton)
		NOTIFY_HANDLER_EX(IDC_FILE_LIST, LVN_KEYDOWN, OnListKeyDown)
		NOTIFY_HANDLER_EX(IDC_DISCOGS_TRACK_LIST, LVN_KEYDOWN, OnListKeyDown)
		NOTIFY_HANDLER_EX(IDC_FILE_LIST, NM_RCLICK, OnListRClick)
		NOTIFY_HANDLER_EX(IDC_DISCOGS_TRACK_LIST, NM_RCLICK, OnListRClick)
		COMMAND_ID_HANDLER(IDC_PREVIEW_TAGS_BUTTON, OnPreviewTags)
		COMMAND_ID_HANDLER(IDC_WRITE_TAGS_BUTTON, OnWriteTags)
		COMMAND_ID_HANDLER(IDC_NEXT_BUTTON, OnMultiNext)
		COMMAND_ID_HANDLER(IDC_PREVIOUS_BUTTON, OnMultiPrev)
		COMMAND_ID_HANDLER(IDC_BACK_BUTTON, OnBack)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_ID_HANDLER(IDC_SKIP_BUTTON, OnMultiSkip)
		CHAIN_MSG_MAP(CDialogResize<CTrackMatchingDialog>)
	MY_END_MSG_MAP()

	BEGIN_DLGRESIZE_MAP(CTrackMatchingDialog)
		DLGRESIZE_CONTROL(IDC_TRACKLIST_GROUP, DLSZ_SIZE_X | DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_DISCOGS_TRACK_LIST, DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_FILE_LIST, DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_REMOVE_FILE_TRACK_BUTTON, DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_FAILED_TO_MATCH_TRACKS, DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_ASSUMED_MATCH_TRACKS, DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_MATCHED_TRACKS, DLSZ_MOVE_Y)
		BEGIN_DLGRESIZE_GROUP()
			DLGRESIZE_CONTROL(IDC_DISCOGS_TRACK_LIST, DLSZ_SIZE_X)
			DLGRESIZE_CONTROL(IDC_FILE_LIST, DLSZ_SIZE_X)
			DLGRESIZE_CONTROL(IDC_REMOVE_FILE_TRACK_BUTTON, DLSZ_MOVE_X)
			DLGRESIZE_CONTROL(IDC_FAILED_TO_MATCH_TRACKS, DLSZ_MOVE_X)
			DLGRESIZE_CONTROL(IDC_ASSUMED_MATCH_TRACKS, DLSZ_MOVE_X)
			DLGRESIZE_CONTROL(IDC_MATCHED_TRACKS, DLSZ_MOVE_X)
		END_DLGRESIZE_GROUP()
		DLGRESIZE_CONTROL(IDC_REMOVE_DISCOGS_TRACK_BUTTON, DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_MOVE_UP_BUTTON, DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_MOVE_DOWN_BUTTON, DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_BACK_BUTTON, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_PREVIEW_TAGS_BUTTON, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_WRITE_TAGS_BUTTON, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_PREVIOUS_BUTTON, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_NEXT_BUTTON, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_SKIP_BUTTON, DLSZ_MOVE_X | DLSZ_MOVE_Y)
	END_DLGRESIZE_MAP()

	void DlgResize_UpdateLayout(int cxWidth, int cyHeight) {
		CDialogResize<CTrackMatchingDialog>::DlgResize_UpdateLayout(cxWidth, cyHeight);
		update_list_width(discogs_track_list);
		update_list_width(file_list);
		save_size(cxWidth, cyHeight);
	}

	CTrackMatchingDialog(HWND p_parent, TagWriter_ptr tag_writer, bool use_update_tags = false) : 
			tag_writer(tag_writer), use_update_tags(use_update_tags) {
		g_discogs->track_matching_dialog = this;
		Create(p_parent);
	}
	CTrackMatchingDialog(HWND p_parent, pfc::array_t<TagWriter_ptr> tag_writers, bool use_update_tags = false) :
			tag_writers(tag_writers), use_update_tags(use_update_tags), multi_mode(true) {
		
		g_discogs->track_matching_dialog = this;

		tag_writer = nullptr;

		if (init_count()) {
			Create(p_parent);
		}
		else {
			finished_tag_writers();
			destroy();
		}
	}
	~CTrackMatchingDialog();
	void CTrackMatchingDialog::OnFinalMessage(HWND /*hWnd*/) override;

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnColorStatic(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT OnMoveTrackUp(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnMoveTrackDown(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnRemoveTrackButton(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnPreviewTags(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnWriteTags(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnMultiNext(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnMultiPrev(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBack(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnMultiSkip(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnListKeyDown(LPNMHDR lParam);
	LRESULT OnListRClick(LPNMHDR lParam);
	
	LRESULT list_key_down(HWND wnd, LPNMHDR lParam);

	bool initialize();
	bool get_next_tag_writer();
	bool get_previous_tag_writer();

	void cb_generate_tags();

	void enable(bool v) override;
	void destroy_all();
	void go_back();
};
