#pragma once

#include "foo_discogs.h"
#include "resource.h"
#include "file_info_manager.h"
#include "multiformat.h"

#include "ceditwithbuttonsdim.h"

using namespace Discogs;

class expand_master_release_process_callback;
class get_artist_process_callback;
class search_artist_process_callback;

enum class updRelSrc { Artist, Releases, Filter, ArtistList, Undef };

#ifndef IDTRACKER_H
#define IDTRACKER_H

struct id_tracker {

	bool enabled = false;
	bool amr = false;

	bool artist = false;
	bool master = false;
	bool release = false;

	t_size artist_i = pfc_infinite; //array index
	t_size master_i = pfc_infinite;  //array index
	t_size release_i = pfc_infinite; //array index

	int artist_index = -1; //list_index
	int master_index = -1;  //list index
	int release_index = -1; //list index

	bool artist_lv_set = false;
	//bool master_lv_set = false;
	bool release_lv_set = false;

	int artist_id = pfc_infinite;
	int master_id = pfc_infinite;
	int release_id = pfc_infinite;

	bool id_tracker::artist_tracked() {
		return (
			enabled &&
			artist &&
			artist_id != pfc_infinite &&
			artist_index == -1);
	}

	bool id_tracker::master_tracked() {
		return (
			enabled &&
			master &&
			master_id != pfc_infinite &&
			master_index == -1);
	}

	bool id_tracker::release_tracked() {
		return (
			enabled && 
			release &&
			release_id != pfc_infinite &&
			release_index == -1);
	}

	void id_tracker::artist_reset() {
		artist_index = -1;
		artist_lv_set = false;
	}

	void id_tracker::release_reset() {
		release_index = -1;
		release_lv_set = false;
	}

	bool id_tracker::artist_check(const pfc::string8 currentid, int currentndx) {
		if (artist_tracked()) {
			if (artist_id == std::atoi(currentid)) {
				artist_index = currentndx;
				return true;
			}
		}
		return false;
	}

	bool id_tracker::release_check(const pfc::string8 currentid, int currentndx, bool ismaster, int masterndx, int masteri) {
		if (ismaster) {
			if (master_tracked()) {
				if (master_id == std::atoi(currentid)) {
					master_index = currentndx;
					return true;
				}
			}
		}
		else {
			if (release_tracked()) {
				if (release_id == std::atoi(currentid)) {
					release_index = currentndx;
					if (masteri != -1) {
						//TODO: continue
						//master_i = ...
						//marter_ndx = ...
					}
					return true;
				}
			}
		}
		return false;
	}
};

#endif //IDTRACKER_H

#ifndef ROW_COLUMN_DATA_H
#define ROW_COLUMN_DATA_H

struct row_col_data {
	int id;	//master id or release id
	std::list<std::pair<int, pfc::string8>> col_data_list; //column #, column content
};

#endif //ROW_COLUMN_DATA_H

class CFindReleaseDialog : public MyCDialogImpl<CFindReleaseDialog>,
	public CDialogResize<CFindReleaseDialog>,
	public CMessageFilter
{
private:

	pfc::string8 m_results_filter;

	t_size m_artist_index;
	t_size m_release_list_artist_id;

	foo_discogs_conf conf;

	playable_location_impl location;
	file_info_impl info;

	metadb_handle_list items;

	titleformat_hook_impl_multiformat_ptr hook;

	pfc::array_t<Artist_ptr> artist_exact_matches;
	pfc::array_t<Artist_ptr> artist_other_matches;

	pfc::array_t<Artist_ptr> find_release_artists;
	Artist_ptr find_release_artist;

	id_tracker _idtracker;
	t_size m_release_selected_index;

	std::vector<std::pair<int, int>> m_vec_lv_items;
	std::vector<std::pair<int, int>> m_vec_build_lv_items;
	std::vector<std::pair<int, int>> vec_icol_subitems;
	void update_sorted_icol_map(bool reset);

	COLORREF hlcolor;
	COLORREF hlfrcolor;
	COLORREF htcolor;
	bool m_bNoEnChange;

	HWND artist_list, release_list;
	HWND release_url_edit, search_edit, filter_edit;
	CEditWithButtonsDim cewb_artist_search;
	CEditWithButtonsDim cewb_release_filter;

	service_ptr_t<expand_master_release_process_callback> *active_task = nullptr;

	void load_size();
	bool build_current_cfg();
	void pushcfg();

	void fill_artist_list(bool force_exact, updRelSrc updsrc); //bool from_search);
	void select_release(int list_index = 0);
	void update_releases(const pfc::string8& filter, updRelSrc updsrc, bool init_expand);
	void expand_releases(const pfc::string8& filter, updRelSrc updsrc, t_size master_index, t_size master_pos);
	void search_artist(bool skip_tracer, pfc::string8 artistname);
	void on_search_artist_done(pfc::array_t<Artist_ptr> &p_artist_exact_matches, const pfc::array_t<Artist_ptr> &p_artist_other_matches);

	void get_selected_artist_releases(updRelSrc updsrc);
	void on_get_artist_done(updRelSrc updsrc, Artist_ptr &artist);

	void on_write_tags(const pfc::string8 &release_id);

	void expand_master_release(MasterRelease_ptr &master_release, int pos);
	void on_expand_master_release_done(const MasterRelease_ptr &master_release, int pos, threaded_process_status &p_status, abort_callback &p_abort);
	void on_expand_master_release_complete();
	void on_release_selected(t_size pos);
	
	void extract_id_from_url(pfc::string8 &s);
	void extract_release_from_link(pfc::string8 &s);

	void insert_item(const pfc::string8& item, int list_index, int item_data);
	int insert_item_row(const row_col_data row_data, int list_index, int item_data, t_size master_list_pos);
	pfc::string8 run_hook_columns(row_col_data& row_data, int item_data);
	
	void set_item_param(HWND list, int list_index, int col, LPARAM in_p);
	void get_item_param(HWND list, int list_index, int col, LPARAM& out_p);
	void get_mounted_param(mounted_param& pm, LPARAM lparam);
	mounted_param get_mounted_param(LPARAM lparam);
	mounted_param get_mounted_param(t_size item_index);
	int get_mounted_lparam(mounted_param myparam);
	pfc::string8 get_param_id(mounted_param myparam);
	pfc::string8 get_param_id_master(mounted_param myparam);
	bool get_lvstate_tracker(HWND wnd_lv, t_size list_index);
	void set_lvstate_expanded(t_size item_index, bool val);
	bool get_lvstate_expanded(t_size list_index);

	bool init_tracker();
	void init_tracker_i(pfc::string8 filter_master, pfc::string8 filter_release, bool expanded, bool fast);

	void reset_default_columns(bool reset);

public:

	enum { IDD = IDD_DIALOG_FIND_RELEASE };

#pragma warning( push )
#pragma warning( disable : 26454 )

	MY_BEGIN_MSG_MAP(CFindReleaseDialog)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDC_PROCESS_RELEASE_BUTTON, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		COMMAND_ID_HANDLER(IDC_SEARCH_EDIT, OnEditSearchText)
		COMMAND_ID_HANDLER(IDC_FILTER_EDIT, OnEditFilterText)
		COMMAND_HANDLER(IDC_ARTIST_LIST, LBN_SELCHANGE, OnSelectArtist)
		COMMAND_HANDLER(IDC_ARTIST_LIST, LBN_DBLCLK, OnDoubleClickArtist)
		NOTIFY_HANDLER(IDC_ARTIST_LIST, LVN_ITEMCHANGED, OnArtistListViewItemChanged)
		NOTIFY_HANDLER(IDC_RELEASE_LIST, LVN_KEYDOWN, OnKeyDownRelease);
		NOTIFY_HANDLER(IDC_ARTIST_LIST, NM_RCLICK, OnRClickRelease);
		NOTIFY_HANDLER(IDC_RELEASE_LIST, NM_RCLICK, OnRClickRelease);
		NOTIFY_HANDLER(IDC_RELEASE_LIST, NM_CLICK, OnClickRelease);
		NOTIFY_HANDLER(IDC_RELEASE_LIST, NM_CUSTOMDRAW, OnCustomDrawReleaseList)
		NOTIFY_HANDLER(IDC_ARTIST_LIST, NM_CUSTOMDRAW, OnCustomDrawArtistList)
		COMMAND_ID_HANDLER(IDC_ONLY_EXACT_MATCHES_CHECK, OnCheckOnlyExactMatches)
		COMMAND_ID_HANDLER(IDC_CHECK_RELEASE_SHOW_ID, OnCheckReleasesShowId)
		COMMAND_ID_HANDLER(IDC_SEARCH_BUTTON, OnButtonSearch)
		COMMAND_ID_HANDLER(IDC_CONFIGURE_BUTTON, OnButtonConfigure)
		CHAIN_MSG_MAP(CDialogResize<CFindReleaseDialog>)
	MY_END_MSG_MAP()

#pragma warning( pop )

	BEGIN_DLGRESIZE_MAP(CFindReleaseDialog)
		DLGRESIZE_CONTROL(IDC_LABEL_RELEASE_ID, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_RELEASE_URL_TEXT, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_FILTER_EDIT, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_RESULTS_GROUP, DLSZ_SIZE_X | DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_LABEL_FILTER, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_ARTIST_LIST, DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_RELEASE_LIST, DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_CHECK_RELEASE_SHOW_ID, DLSZ_MOVE_Y)
		BEGIN_DLGRESIZE_GROUP()
			DLGRESIZE_CONTROL(IDC_ARTIST_LIST, DLSZ_SIZE_X)
			DLGRESIZE_CONTROL(IDC_RELEASE_LIST, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_CHECK_RELEASE_SHOW_ID, DLSZ_MOVE_X)
			DLGRESIZE_CONTROL(IDC_LABEL_RELEASES, DLSZ_MOVE_X)
		END_DLGRESIZE_GROUP()
		DLGRESIZE_CONTROL(IDC_ONLY_EXACT_MATCHES_CHECK, DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_CONFIGURE_BUTTON, DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_STATIC_FIND_REL_REL_NAME, DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_PROCESS_RELEASE_BUTTON, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y)
	END_DLGRESIZE_MAP()

	void DlgResize_UpdateLayout(int cxWidth, int cyHeight) {
		CDialogResize<CFindReleaseDialog>::DlgResize_UpdateLayout(cxWidth, cyHeight);
	}

	CFindReleaseDialog(HWND p_parent, metadb_handle_list items, bool conf_release_id_tracker)
		: items(items), cewb_artist_search(L"Search artist"), cewb_release_filter(L"Filter releases") {

		_idtracker.enabled = conf_release_id_tracker;
		
		m_release_selected_index = pfc_infinite;

		find_release_artist = nullptr;

		Create(p_parent);
	};

	~CFindReleaseDialog();
	
	void OnFinalMessage(HWND /*hWnd*/) override;

	virtual BOOL PreTranslateMessage(MSG* pMsg) override {

		//return __super::PreTranslateMessage(pMsg); // allow default behavior
		return ::IsDialogMessage(m_hWnd, pMsg);
	}

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnEditSearchText(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnEditFilterText(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnSelectArtist(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnDoubleClickArtist(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCustomDrawReleaseList(int wParam, LPNMHDR lParam, BOOL bHandled);
	LRESULT OnCustomDrawArtistList(int wParam, LPNMHDR lParam, BOOL bHandled);
	LRESULT PointInReleaseListHeader(POINT pt);
	LRESULT OnRClick(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnListRClick(LPNMHDR lParam);
	void OnContextColumnCfgMenu(CWindow wnd, CPoint point);
	LRESULT OnArtistListViewItemChanged(int, LPNMHDR hdr, BOOL&);
	//LRESULT OnDoubleClickRelease(int, LPNMHDR hdr, BOOL&);
	LRESULT OnRClickRelease(int, LPNMHDR hdr, BOOL&); 
	LRESULT OnClickRelease(int, LPNMHDR hdr, BOOL&);
	LRESULT OnKeyDownRelease(int, LPNMHDR hdr, BOOL&);
	LRESULT OnCheckReleasesShowId(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCheckOnlyExactMatches(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnButtonSearch(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnButtonConfigure(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	friend class expand_master_release_process_callback;
	friend class get_artist_process_callback;
	friend class search_artist_process_callback;

	void enable(bool is_enabled) override;
	void setitems(metadb_handle_list track_matching_items) {
		items = track_matching_items;
	}
};


