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

#ifndef IDTRACER_H
#define IDTRACER_H

struct id_tracer {

	bool enabled = false;
	bool amr = false;

	bool artist = false;
	bool master = false;
	bool release = false;

	size_t artist_i = pfc_infinite;
	size_t master_i = pfc_infinite;
	size_t release_i = pfc_infinite;

	size_t artist_index = -1;
	size_t master_index = -1;
	size_t release_index = -1;

	int artist_id = pfc_infinite;
	int master_id = pfc_infinite;
	int release_id = pfc_infinite;

	bool id_tracer::artist_tracked() {
		return (
			enabled &&
			artist &&
			artist_id != pfc_infinite &&
			artist_index == -1);
	}

	bool id_tracer::master_tracked() {
		return (
			enabled &&
			master &&
			master_id != pfc_infinite &&
			master_index == -1);
	}

	bool id_tracer::release_tracked() {
		return (
			enabled &&
			release &&
			release_id != pfc_infinite &&
			release_index == -1);
	}

	void id_tracer::artist_reset() {
		artist_index = -1;
		artist_lv_set = false;
	}

	void id_tracer::release_reset() {
		release_index = -1;
		release_lv_set = false;
	}

	bool id_tracer::artist_check(const pfc::string8 currentid, int currentndx) {
		if (artist_tracked()) {
			if (artist_id == std::atoi(currentid)) {
				artist_index = 0;
				return true;
			}
		}
		return false;
	}

	bool id_tracer::release_check(const pfc::string8 currentid, int currentndx, bool ismaster, int masterndx, int masteri) {
		if (ismaster) {
			if (master_tracked()) {
				if (master_id == std::atoi(currentid)) {
					master_index = 0;
					return true;
				}
			}
		}
		else {
			if (release_tracked()) {
				if (release_id == std::atoi(currentid)) {
					release_index = 0;
					if (masteri != -1) {
						//TODO: continue

					}
					return true;
				}
			}
		}
		return false;
	}
};

#endif //IDTRACER_H
//
#ifndef FLAGREG
#define FLAGREG

struct flagreg { int ver; int flag; };

#endif //flagreg
//
#ifndef ROW_COLUMN_DATA_H
#define ROW_COLUMN_DATA_H

struct row_col_data {
	int id;	//master id or release id
	std::list<std::pair<int, pfc::string8>> col_data_list; //column #, column content
};

#endif //ROW_COLUMN_DATA_H

enum class NodeFlag {
	none = 0,
	added = 1,
	tree_created = 2,
	spawn = 4,
	expanded = 8,
	to_do = 16,
	filterok = 32
};

class CFindReleaseDialog : public MyCDialogImpl<CFindReleaseDialog>,
	public CDialogResize<CFindReleaseDialog>,
	public CMessageFilter
{

private:

	enum {
		KTypeFilterTimerID = 0xd21e0907,
		KTypeFilterTimeOut = 500
	};

	uint32_t m_tickCount;

	HTREEITEM m_hHit;

	bool m_updating_releases;

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

	id_tracer _idtracer;

	int _ver = 0;

	std::unordered_map<int, std::pair<row_col_data, flagreg>> m_cache_find_releases;
	typedef typename std::unordered_map<int, std::pair<row_col_data, flagreg>>::iterator cache_iterator_t;

	std::vector<std::pair<cache_iterator_t, HTREEITEM*>> m_vec_items;
	std::vector<std::pair<int, int>> m_vec_filter; //master_id, release_id

	auto find_vec_by_lparam(int lparam);
	auto find_vec_by_id(int id);

	std::vector<std::pair<int, int>> vec_icol_subitems;
	void update_sorted_icol_map(bool reset);

	COLORREF hlcolor;
	COLORREF hlfrcolor;
	COLORREF htcolor;
	bool m_DisableFilterBoxEvents;

#ifdef RELCOL_FIND
	std::vector<int> CFindReleaseDialog::GetColumnOrderArray() const;
#endif

	HWND m_artist_list;
	HWND m_release_tree;
	HWND m_release_url_edit, m_search_edit, m_filter_edit;

	CEditWithButtonsDim cewb_artist_search;
	CEditWithButtonsDim cewb_release_filter;
	CEditWithButtonsDim cewb_release_url;

	std::function<bool()>stdf_enteroverride_artist = [this]() {
		BOOL bdummy;
		HWND button = uGetDlgItem(IDC_SEARCH_BUTTON);
		if (::IsWindowEnabled(button))
			OnButtonSearch(0L, 0L, NULL, bdummy);
		return false;
	};

	std::function<bool()>stdf_enteroverride_url = [this]() {
		BOOL bdummy;
		HWND button = uGetDlgItem(IDC_PROCESS_RELEASE_BUTTON);
		if (::IsWindowEnabled(button))
			OnOK(0L, 0L, NULL, bdummy);
		return false;
	};

	service_ptr_t<expand_master_release_process_callback>* m_active_task = nullptr;

	void load_size();
	bool build_current_cfg();
	void pushcfg();

	void update_releases(const pfc::string8& filter, updRelSrc updsrc, bool init_expand);
	void expand_releases(const pfc::string8& filter, updRelSrc updsrc, t_size master_index, t_size master_pos);
	
	bool set_node_expanded(t_size master_i, int& state, bool build);
	bool get_node_expanded(t_size master_i, int& out_state);
	
	void fill_artist_list(bool force_exact, updRelSrc updsrc);
	void search_artist(bool skip_tracer, pfc::string8 artistname);
	void on_search_artist_done(pfc::array_t<Artist_ptr>& p_artist_exact_matches, const pfc::array_t<Artist_ptr>& p_artist_other_matches);
	void get_selected_artist_releases(updRelSrc updsrc);
	void on_get_artist_done(updRelSrc updsrc, Artist_ptr& artist);

	void on_write_tags(const pfc::string8& release_id);

	void expand_master_release(MasterRelease_ptr& master_release, int pos);
	void on_expand_master_release_done(const MasterRelease_ptr& master_release, int pos, threaded_process_status& p_status, abort_callback& p_abort);
	void on_expand_master_release_complete();
	void on_release_selected(int src_lparam);

	void extract_id_from_url(pfc::string8& s);
	void extract_release_from_link(pfc::string8& s);
	
	pfc::string8 run_hook_columns(row_col_data& row_data, int item_data);

	mounted_param mount_param(LPARAM lparam);
	t_size get_mounted_lparam(mounted_param myparam);
	void get_mounted_param(mounted_param& pm, LPARAM lparam);
	pfc::string8 get_param_id(mounted_param myparam);
	int get_param_id_master(mounted_param myparam);
	void set_artist_item_param(HWND list, int list_index, int col, LPARAM in_p);

	bool get_cached_find_release_node(int lparam, pfc::string8& item, row_col_data& rowdata);
	bool init_tracker();
	void init_tracker_i(pfc::string8 filter_master, pfc::string8 filter_release, bool expanded, bool fast);
	std::pair<t_size, t_size> init_filter_i(pfc::string8 filter_master, pfc::string8 filter_release,
		bool ontitle, bool expanded, bool fast, bool countreserve);

	void reset_default_columns(bool reset);

public:

	void KFilterTimerOn(CWindow parent) {
		m_tickCount = 0;
		SetTimer(KTypeFilterTimerID, KTypeFilterTimeOut);
	}

	void KTurnOffFilterTimer() throw() {
		KillTimer(KTypeFilterTimerID);
	}

	enum { IDD = IDD_DIALOG_FIND_RELEASE };

#pragma warning( push )
#pragma warning( disable : 26454 )

	BEGIN_MSG_MAP_EX(CFindReleaseDialog)
		MSG_WM_TIMER(OnTypeFilterTimer)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDC_PROCESS_RELEASE_BUTTON, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		COMMAND_ID_HANDLER(IDC_FILTER_EDIT, OnEditFilterText)
		COMMAND_HANDLER(IDC_ARTIST_LIST, LBN_SELCHANGE, OnSelectArtist)
		NOTIFY_HANDLER(IDC_RELEASE_TREE, NM_DBLCLK, OnDoubleClickRelease)		
		NOTIFY_HANDLER(IDC_RELEASE_TREE, TVN_GETDISPINFO, OnReleaseTreeGetDispInfo)
		NOTIFY_HANDLER(IDC_RELEASE_TREE, TVN_ITEMEXPANDING, OnReleaseTreeExpanding)
		NOTIFY_HANDLER(IDC_RELEASE_TREE, TVN_SELCHANGED, OnReleaseTreeSelChanged)
		NOTIFY_HANDLER(IDC_RELEASE_TREE, NM_RCLICK, OnRClickRelease);
		NOTIFY_HANDLER(IDC_ARTIST_LIST, /*LVN_ODSTATECHANGED*/LVN_ITEMCHANGED, OnArtistListViewItemChanged)
		NOTIFY_HANDLER(IDC_ARTIST_LIST, NM_RCLICK, OnRClickRelease);
		COMMAND_ID_HANDLER(IDC_ONLY_EXACT_MATCHES_CHECK, OnCheckOnlyExactMatches)
		COMMAND_ID_HANDLER(IDC_SEARCH_BUTTON, OnButtonSearch)
		COMMAND_ID_HANDLER(IDC_CONFIGURE_BUTTON, OnButtonConfigure)
		CHAIN_MSG_MAP(CDialogResize<CFindReleaseDialog>)
		END_MSG_MAP()

#pragma warning( pop )
	BEGIN_DLGRESIZE_MAP(CFindReleaseDialog)
		DLGRESIZE_CONTROL(IDC_LABEL_RELEASE_ID, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_RELEASE_URL_TEXT, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_FILTER_EDIT, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_RESULTS_GROUP, DLSZ_SIZE_X | DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_LABEL_FILTER, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_ARTIST_LIST, DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_RELEASE_TREE, DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_CHECK_RELEASE_SHOW_ID, DLSZ_MOVE_Y)
		BEGIN_DLGRESIZE_GROUP()
		DLGRESIZE_CONTROL(IDC_ARTIST_LIST, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_RELEASE_TREE, DLSZ_SIZE_X)
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
		: items(items), cewb_artist_search(L"Search artist"), cewb_release_filter(L"Filter releases"),
		
		cewb_release_url(L"Release url"), m_hHit(NULL) {
		_idtracer.enabled = conf_release_id_tracker;
		find_release_artist = nullptr;
		Create(p_parent);
	};

	~CFindReleaseDialog();

	void OnFinalMessage(HWND /*hWnd*/) override;

	virtual BOOL PreTranslateMessage(MSG* pMsg) override {
		return ::IsDialogMessage(m_hWnd, pMsg);
	}

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnEditFilterText(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT ApplyFilter(pfc::string8 strFilter);
	LRESULT OnSelectArtist(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT PointInReleaseListHeader(POINT pt);
	LRESULT OnRClick(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnListRClick(LPNMHDR lParam);
	void OnContextColumnCfgMenu(CWindow wnd, CPoint point);
	LRESULT OnArtistListViewItemChanged(int, LPNMHDR hdr, BOOL&);
	LRESULT OnDoubleClickRelease(int, LPNMHDR hdr, BOOL&);
	LRESULT OnRClickRelease(int, LPNMHDR hdr, BOOL&);
	LRESULT OnReleaseTreeGetDispInfo(int, LPNMHDR hdr, BOOL&);
	LRESULT OnReleaseTreeExpanding(int, LPNMHDR hdr, BOOL&);
	LRESULT OnReleaseTreeSelChanged(int, LPNMHDR hdr, BOOL&);
	LRESULT OnCheckOnlyExactMatches(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnButtonSearch(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnButtonConfigure(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	void SetEnterKeyOverride(bool enable);

	void OnTypeFilterTimer(WPARAM id) {
		if (id == KTypeFilterTimerID) {
			switch (++m_tickCount) {
			case 1:
				break;
			case 2:
				break;
			case 3:
				pfc::string8 strFilter;
				uGetWindowText(m_filter_edit, strFilter);
				ApplyFilter(strFilter);
				KillTimer(KTypeFilterTimerID);
				break;
			}
		}
	}

	friend class expand_master_release_process_callback;
	friend class get_artist_process_callback;
	friend class search_artist_process_callback;

	void enable(bool is_enabled) override;
	void setitems(metadb_handle_list track_matching_items) {
		items = track_matching_items;
	}

	bool SetCacheFlag(t_size lparam, NodeFlag flagtype, bool val);
	bool SetCacheFlag(t_size lparam, NodeFlag flagtype, int& val);
	bool SetCacheFlag(cache_iterator_t& cached_item, NodeFlag flagtype, int* const& val);
	bool GetCacheFlag(t_size lparam, NodeFlag flagtype);
	bool GetCacheFlag(t_size lparam, NodeFlag flagtype, bool& val);
	bool GetCacheFlag(cache_iterator_t it_cached_item, NodeFlag flagtype, bool* const& val = NULL);

};


