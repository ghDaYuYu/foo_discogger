#pragma once

#include "foo_discogs.h"
#include "resource.h"
#include "file_info_manager.h"
#include "multiformat.h"

#include "ceditwithbuttonsdim.h"
#include "find_release_tree.h"

using namespace Discogs;

class expand_master_release_process_callback;
class get_artist_process_callback;
class search_artist_process_callback;

enum class updRelSrc { Artist, Releases, Filter, ArtistList, Undef };

enum FilterFlag {
	Versions	= 1 << 0,
	//..
	RoleMain	= 1 << 4,
	//..
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
	bool m_skip_fill_filter = false;

	t_size m_artist_index;

	foo_conf conf;

	playable_location_impl location;
	file_info_impl info;

	metadb_handle_list items;

	titleformat_hook_impl_multiformat_ptr hook;

	pfc::array_t<Artist_ptr> m_artist_exact_matches;
	pfc::array_t<Artist_ptr> m_artist_other_matches;

	pfc::array_t<Artist_ptr> find_release_artists;
	Artist_ptr find_release_artist;

	id_tracer _idtracer;

	std::shared_ptr<filter_cache> m_cache_find_release_ptr;
	std::shared_ptr<vec_t> m_vec_items_ptr;
	std::vector<std::pair<int, int>> m_vec_filter;

	std::vector<std::pair<int, int>> vec_icol_subitems;
	void update_sorted_icol_map(bool reset);

	COLORREF hlcolor;
	COLORREF hlfrcolor;
	COLORREF htcolor;
	bool m_DisableFilterBoxEvents;

	HWND m_artist_list;
	HWND m_release_tree;
	HWND m_release_url_edit, m_search_edit, m_filter_edit;

	CEditWithButtonsDim cewb_artist_search;
	CEditWithButtonsDim cewb_release_filter;
	CEditWithButtonsDim cewb_release_url;

	CFindReleaseTree m_release_ctree;

	vppair m_vres_release_history;
	vppair m_vres_artist_history;
	vppair m_vres_filter_history;

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

	pfc::string8 build_history_menu(HWND hwnd);

	std::function<bool(HWND hwnd, wchar_t* editval)> m_stdf_call_history =
		[&](HWND hwnd, wchar_t* wstrt) {

		pfc::string8 menu_option = build_history_menu(hwnd);
		if (menu_option.get_length()) {
			wchar_t newval[MAX_PATH + 1];
			pfc::stringcvt::convert_utf8_to_wide(newval, MAX_PATH,
				menu_option, menu_option.get_length());
			_tcscpy_s(wstrt, MAX_PATH, newval);
			return true;
		}
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

	pfc::string8 run_hook_columns(row_col_data& row_data, int item_data);
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
		NOTIFY_HANDLER(IDC_ARTIST_LIST, LVN_ITEMCHANGED, OnArtistListViewItemChanged)
		COMMAND_ID_HANDLER(IDC_ONLY_EXACT_MATCHES_CHECK, OnCheckOnlyExactMatches)
		COMMAND_ID_HANDLER(IDC_SEARCH_BUTTON, OnButtonSearch)
		COMMAND_ID_HANDLER(IDC_CONFIGURE_BUTTON, OnButtonConfigure)
		COMMAND_ID_HANDLER(IDC_CHK_FIND_RELEASE_FILTER_VERS, OnCheckFindReleaseFilterFlags)
		COMMAND_ID_HANDLER(IDC_CHK_FIND_RELEASE_FILTER_ROLEMAIN, OnCheckFindReleaseFilterFlags)
		CHAIN_MSG_MAP_MEMBER(m_release_ctree)
		CHAIN_MSG_MAP(CDialogResize<CFindReleaseDialog>)
		REFLECT_NOTIFICATIONS();
	END_MSG_MAP()

#pragma warning( pop )

	BEGIN_DLGRESIZE_MAP(CFindReleaseDialog)

		DLGRESIZE_CONTROL(IDC_LABEL_RELEASE_ID, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_RELEASE_URL_TEXT, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_FILTER_EDIT, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_EDIT_SEPARATOR, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_ARTIST_LIST, DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_RELEASE_TREE, DLSZ_SIZE_Y)

		DLGRESIZE_CONTROL(IDC_CHK_FIND_RELEASE_FILTER_ROLEMAIN, DLSZ_MOVE_Y)

		BEGIN_DLGRESIZE_GROUP()

			DLGRESIZE_CONTROL(IDC_ARTIST_LIST, DLSZ_SIZE_X)
			DLGRESIZE_CONTROL(IDC_RELEASE_TREE, DLSZ_SIZE_X)
			DLGRESIZE_CONTROL(IDC_LABEL_RELEASES, DLSZ_MOVE_X)
			//MOVE X CHECKBOX ROLEMAIN INSIDE THIS GROUP TO ALIGN WITH RELEASE_TREE LEFT POSITION
			DLGRESIZE_CONTROL(IDC_CHK_FIND_RELEASE_FILTER_ROLEMAIN, DLSZ_MOVE_X)

		END_DLGRESIZE_GROUP()

		DLGRESIZE_CONTROL(IDC_CHK_FIND_RELEASE_FILTER_VERS, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_LABEL_FILTER, DLSZ_MOVE_X)

		DLGRESIZE_CONTROL(IDC_PROFILE_EDIT, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_ONLY_EXACT_MATCHES_CHECK, DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_PROFILE_EDIT, DLSZ_MOVE_Y)
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
		cewb_release_url(L"Release url"),	m_release_ctree(p_parent, IDD),	m_hHit(NULL) {

		find_release_artist = nullptr;

		_idtracer.enabled = conf_release_id_tracker;
		m_cache_find_release_ptr = std::make_shared<filter_cache>(filter_cache());
		m_vec_items_ptr = std::make_shared<vec_t>(vec_t());
	};

	~CFindReleaseDialog() { 
		g_discogs->find_release_dialog = nullptr;
	};

	virtual BOOL PreTranslateMessage(MSG* pMsg) override {
		return ::IsDialogMessage(m_hWnd, pMsg);
	}

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnEditFilterText(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT ApplyFilter(pfc::string8 strFilter, bool force_redraw = false, bool force_rebuild = false);
	LRESULT OnSelectArtist(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnRClick(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	void OnContextColumnCfgMenu(CWindow wnd, CPoint point);
	LRESULT OnArtistListViewItemChanged(int, LPNMHDR hdr, BOOL&);
	LRESULT OnCheckOnlyExactMatches(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCheckFindReleaseFilterFlags(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnButtonSearch(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnButtonConfigure(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	void SetEnterKeyOverride(bool enable);
	void SetHistoryServiceButtonOverride();

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

	//serves credits dlg test/preview
	metadb_handle_list getitems() {
		return items;
	}

	//serves config dialog
	void zap_vhistory() {
		m_vres_release_history.clear();
		m_vres_artist_history.clear();
		m_vres_filter_history.clear();
	}

	//from
	// process release callback on success
	// search_artist_process_callback on success
	// get_artist_process_callback

	bool add_history_row(int type, rppair row) {

		bool bres = false;

		rppair copyrow =
			std::pair(
				std::pair(row.first.first.c_str(), row.first.second.c_str()),
				std::pair(row.second.first.c_str(), row.second.second.c_str())
			);

		if (type == 0) {
			if (!row.first.first.get_length()) return false;
			vppair* v_p = &m_vres_release_history;
			auto s = std::find_if(v_p->begin(), v_p->end(),
				[row](const rppair& w) { return w.first.first.equals(row.first.first); });

			if (bres = s == v_p->end(); bres) //not found
				v_p->emplace_back(copyrow);
		}
		else if (type == 1) {
			if (!row.second.first.get_length()) return false;
			vppair* v_p = &m_vres_artist_history;
			auto s = std::find_if(v_p->begin(), v_p->end(),
				[row](const rppair& w) { return w.second.first.equals(row.second.first); });

			if (bres = s == v_p->end(), bres) //not found
				v_p->emplace_back(copyrow);
		}
		else if (type == 2) {
			if (!row.first.first.get_length()) return false;
			vppair* v_p = &m_vres_filter_history;
			auto s = std::find_if(v_p->begin(), v_p->end(),
				[row](const rppair& w) { return w.first.first.equals(row.first.first); });

			if (bres = s == v_p->end(), bres) //not found
				v_p->emplace_back(copyrow);
		}
		return bres;
	}
};


