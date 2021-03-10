#pragma once

#include "foo_discogs.h"
#include "resource.h"
#include "file_info_manager.h"
#include "multiformat.h"

using namespace Discogs;

class expand_master_release_process_callback;
class get_artist_process_callback;
class search_artist_process_callback;

extern struct mounted_param  myparam;


class CFindReleaseDialog : public MyCDialogImpl<CFindReleaseDialog>, public CDialogResize<CFindReleaseDialog>, public CMessageFilter
{
private:
	bool dropId = false;
	bool conf_changed = false;
	foo_discogs_conf conf;

	playable_location_impl location;
	file_info_impl info;

	metadb_handle_list items;

	titleformat_hook_impl_multiformat_ptr hook;

	pfc::array_t<Artist_ptr> artist_exact_matches;
	pfc::array_t<Artist_ptr> artist_other_matches;

	pfc::array_t<Artist_ptr> find_release_artists;
	Artist_ptr find_release_artist;

	void insert_item(const pfc::string8 &item, int list_index, int item_data);
	void get_item_text(HWND list, int list_index, int col, pfc::string8& out);
	void get_item_param(HWND list, int list_index, int col, LPARAM& out_p);
	void get_mounted_param(mounted_param& pm, LPARAM lparam);
	mounted_param get_mounted_param(LPARAM lparam);
	
	HWND artist_list, release_list;
	HWND release_url_edit, search_edit, filter_edit;

	service_ptr_t<expand_master_release_process_callback> *active_task = nullptr;

	void load_size();
	bool build_current_cfg();
	void pushcfg();

	//TODO: add persistence to find release dialog columns

	void fill_artist_list();
	void select_first_release();
	void filter_releases(const pfc::string8 &text);

	void search_artist();
	void on_search_artist_done(pfc::array_t<Artist_ptr> &p_artist_exact_matches, const pfc::array_t<Artist_ptr> &p_artist_other_matches);

	void get_selected_artist_releases();
	void on_get_artist_done(Artist_ptr &artist);

	void on_write_tags(const pfc::string8 &release_id);

	void expand_master_release(MasterRelease_ptr &master_release, int pos);
	void on_expand_master_release_done(const MasterRelease_ptr &master_release, int pos, threaded_process_status &p_status, abort_callback &p_abort);
	void on_expand_master_release_complete();

	void on_release_selected(int pos);

	void extract_id_from_url(pfc::string8 &s);
	void extract_release_from_link(pfc::string8 &s);

public:
	enum { IDD = IDD_DIALOG_FIND_RELEASE };

	virtual BOOL PreTranslateMessage(MSG* pMsg) override {
		return ::IsDialogMessage(m_hWnd, pMsg);
	}

	MY_BEGIN_MSG_MAP(CFindReleaseDialog)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDC_PROCESS_RELEASE_BUTTON, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		COMMAND_ID_HANDLER(IDC_RELEASE_URL_TEXT, OnEditReleaseIdText)
		COMMAND_ID_HANDLER(IDC_SEARCH_TEXT, OnEditSearchText)
		COMMAND_ID_HANDLER(IDC_FILTER_EDIT, OnEditFilterText)
		COMMAND_HANDLER(IDC_ARTIST_LIST, LBN_SELCHANGE, OnSelectArtist)
		COMMAND_HANDLER(IDC_ARTIST_LIST, LBN_DBLCLK, OnDoubleClickArtist)
		COMMAND_HANDLER(IDC_RELEASE_LIST, LBN_SELCHANGE, OnSelectRelease)
		COMMAND_HANDLER(IDC_RELEASE_LIST, LBN_DBLCLK, OnDoubleClickRelease)
		COMMAND_ID_HANDLER(IDC_ONLY_EXACT_MATCHES_CHECK, OnCheckOnlyExactMatches)
		COMMAND_ID_HANDLER(IDC_SEARCH_BUTTON, OnSearch)
		COMMAND_ID_HANDLER(IDC_CLEAR_FILTER_BUTTON, OnClearFilter)
		COMMAND_ID_HANDLER(IDC_CONFIGURE_BUTTON, OnConfigure)
		CHAIN_MSG_MAP(CDialogResize<CFindReleaseDialog>)
	MY_END_MSG_MAP()

	BEGIN_DLGRESIZE_MAP(CFindReleaseDialog)
		DLGRESIZE_CONTROL(IDC_LABEL_RELEASE_ID, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_RELEASE_URL_TEXT, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_FILTER_EDIT, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_RESULTS_GROUP, DLSZ_SIZE_X | DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_LABEL_FILTER, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_CLEAR_FILTER_BUTTON, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_ARTIST_LIST, DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_RELEASE_LIST, DLSZ_SIZE_Y)
		BEGIN_DLGRESIZE_GROUP()
			DLGRESIZE_CONTROL(IDC_ARTIST_LIST, DLSZ_SIZE_X)
			DLGRESIZE_CONTROL(IDC_RELEASE_LIST, DLSZ_SIZE_X)
			DLGRESIZE_CONTROL(IDC_LABEL_RELEASES, DLSZ_MOVE_X)
		END_DLGRESIZE_GROUP()
		DLGRESIZE_CONTROL(IDC_ONLY_EXACT_MATCHES_CHECK, DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_CONFIGURE_BUTTON, DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_PROCESS_RELEASE_BUTTON, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y)
	END_DLGRESIZE_MAP()

	void DlgResize_UpdateLayout(int cxWidth, int cyHeight) {
		CDialogResize<CFindReleaseDialog>::DlgResize_UpdateLayout(cxWidth, cyHeight);
	}

	CFindReleaseDialog(HWND p_parent, metadb_handle_list items, bool dropId) : items(items), dropId(dropId) {
		find_release_artist = nullptr;
		Create(p_parent);
	};
	~CFindReleaseDialog();
	
	void OnFinalMessage(HWND /*hWnd*/) override;
	

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnEditReleaseIdText(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnEditSearchText(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnEditFilterText(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnSelectArtist(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnDoubleClickArtist(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnSelectRelease(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnDoubleClickRelease(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCheckOnlyExactMatches(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnSearch(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnClearFilter(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnConfigure(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	friend class expand_master_release_process_callback;
	friend class get_artist_process_callback;
	friend class search_artist_process_callback;

	void enable(bool is_enabled) override;
};
