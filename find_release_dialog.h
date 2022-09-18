#pragma once

#include "atlwin.h"

#include "helpers/DarkMode.h"

#include "resource.h"
#include "foo_discogs.h"
#include "multiformat.h"
#include "my_editwithbuttons.h"

#include "history_oplog.h"
#include "find_release_tree.h"
#include "find_artist_list.h"
#include "find_artist_list_ILO.h"

using namespace Discogs;

class expand_master_release_process_callback;
class get_artist_process_callback;
class search_artist_process_callback;

class CFindReleaseDialog : public MyCDialogImpl<CFindReleaseDialog>,
	public CDialogResize<CFindReleaseDialog>, public CMessageFilter,
	public ILOD_artist_list, public fb2k::CDarkModeHooks {

public:

	CListControlOwnerData* ilo_get_uilist() override {
		return dynamic_cast<CListControlOwnerData*>(&m_alist);
	}

	// constructor / destructor

	CFindReleaseDialog(HWND p_parent, metadb_handle_list items, foo_conf cfg);

	~CFindReleaseDialog();

	virtual BOOL PreTranslateMessage(MSG* pMsg) override {
		return ::IsDialogMessage(m_hWnd, pMsg);
	}

#pragma warning( push )
#pragma warning( disable : 26454 )

	BEGIN_MSG_MAP(CFindReleaseDialog)
		
		MSG_WM_TIMER(OnTypeFilterTimer)
		
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)

		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_ID_HANDLER(IDC_EDIT_FILTER, OnEditFilter)

		COMMAND_ID_HANDLER(IDC_BTN_SEARCH, OnButtonSearch)
		COMMAND_ID_HANDLER(IDC_BTN_CONFIGURE, OnButtonConfigure)
		COMMAND_ID_HANDLER(IDC_BTN_PROCESS_RELEASE, OnButtonNext)

		COMMAND_ID_HANDLER(IDC_CHK_ONLY_EXACT_MATCHES, OnCheckboxOnlyExactMatches)
		COMMAND_ID_HANDLER(IDC_CHK_RELEASE_SHOW_PROFILE, OnCheckboxBindProfilePanel)
		COMMAND_ID_HANDLER(IDC_CHK_FIND_RELEASE_FILTER_VERS, OnCheckboxFindReleaseFilterFlags)
		COMMAND_ID_HANDLER(IDC_CHK_FIND_RELEASE_FILTER_ROLEMAIN, OnCheckboxFindReleaseFilterFlags)

		CHAIN_MSG_MAP(CDialogResize<CFindReleaseDialog>)
		CHAIN_MSG_MAP_MEMBER(m_dctree)
		CHAIN_MSG_MAP_MEMBER(m_alist)
		REFLECT_NOTIFICATIONS();
	END_MSG_MAP()

#pragma warning( pop )

	BEGIN_DLGRESIZE_MAP(CFindReleaseDialog)

		DLGRESIZE_CONTROL(IDC_LABEL_RELEASE_ID, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_RELEASE_URL_TEXT, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_STATIC_FIND_REL_STATS_EXT, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_EDIT_FILTER, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_ARTIST_LIST, DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_RELEASE_TREE, DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_CHK_FIND_RELEASE_FILTER_ROLEMAIN, DLSZ_MOVE_Y)

		BEGIN_DLGRESIZE_GROUP()

			DLGRESIZE_CONTROL(IDC_ARTIST_LIST, DLSZ_SIZE_X)
			DLGRESIZE_CONTROL(IDC_RELEASE_TREE, DLSZ_SIZE_X)
			DLGRESIZE_CONTROL(IDC_LABEL_RELEASES, DLSZ_MOVE_X)
			DLGRESIZE_CONTROL(IDC_CHK_FIND_RELEASE_FILTER_ROLEMAIN, DLSZ_MOVE_X)

		END_DLGRESIZE_GROUP()

		DLGRESIZE_CONTROL(IDC_CHK_FIND_RELEASE_FILTER_VERS, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_LABEL_FILTER, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_CHK_ONLY_EXACT_MATCHES, DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_CHK_RELEASE_SHOW_PROFILE, DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_BTN_CONFIGURE, DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_STATIC_FIND_REL_STATS, DLSZ_MOVE_Y | DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_BTN_PROCESS_RELEASE, DLSZ_MOVE_X)

	END_DLGRESIZE_MAP()

	void DlgResize_UpdateLayout(int cxWidth, int cyHeight) {
		CDialogResize<CFindReleaseDialog>::DlgResize_UpdateLayout(cxWidth, cyHeight);
	}

	// constructor / destructor

	CFindReleaseDialog(HWND p_parent, metadb_handle_list items, foo_conf cfg);

	~CFindReleaseDialog();

	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		destroy();
		return TRUE;
	}

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	LRESULT OnEditFilter(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnButtonNext(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	
	LRESULT OnButtonSearch(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnButtonConfigure(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT OnCheckboxBindProfilePanel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCheckboxOnlyExactMatches(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCheckboxFindReleaseFilterFlags(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	//..

	void OnTypeFilterTimer(WPARAM id);
	void apply_filter(pfc::string8 strFilter, bool force_redraw = false, bool force_rebuild = false);

	//todo: 
	friend class expand_master_release_process_callback;
	friend class get_artist_process_callback;
	friend class get_various_artists_process_callback;
	friend class search_artist_process_callback;

	friend class CArtistList;
	friend class CFindReleaseTree;
	//..

	void enable(bool is_enabled) override;
	void setitems(metadb_handle_list track_matching_items) { items = track_matching_items; }

	//serves credits dlg test/preview
	metadb_handle_list getitems() {	return items; }

	//serves EnterKeySubclassProc
	bool ForwardVKReturn();


public:

	enum { IDD = IDD_DIALOG_FIND_RELEASE };

	enum flg_fr {
		FLG_PROFILE_DLG_SHOW = 1 << 0,
		FLG_PROFILE_DLG_ATTACHED = 1 << 1,
		FLG_SHOW_RELEASE_TREE_STATS = 1 << 2
	};

	void show() override;
	void hide() override;

	history_oplog* get_oplogger() { return &m_oplogger; }

	size_t get_tree_artist_list_pos() { return m_dctree.Get_Artist_List_Position(); }

	void UpdateArtistProfile(Artist_ptr artist);

	//todo:
	pfc::string EscapeWin(pfc::string8 keyWord) {
		pfc::string8 out_keyWord(keyWord);
		//out_keyWord.replace_string("/", "//");
		//out_keyWord.replace_string("'", "''");
		//out_keyWord.replace_string("[", "/[");
		//out_keyWord.replace_string("]", "/]");
		//out_keyWord.replace_string("%", "/%");
		out_keyWord.replace_string("&", "&&");
		//out_keyWord.replace_string("_", "/_");
		//out_keyWord.replace_string("(", "/(");
		//out_keyWord.replace_string(")", "/)");
		return out_keyWord;
	}

	void print_root_stats(rppair root_stat, bool save = true);

	bool add_history(oplog_type optype, std::string cmd, pfc::string8 ff, pfc::string8 fs, pfc::string8 sf, pfc::string8 ss);
	bool add_history(oplog_type optype, std::string cmd, rppair row);

	bool Set_Config_Flag(int ID, int flag, bool flagvalue);

	std::mutex m_loading_selection_rw_mutex;
	size_t m_loading_selection_id = pfc_infinite;

private:

	enum {
		KTypeFilterTimerID = 0xd21e0907,
		KTypeFilterTimeOut = 500
	};
		
	//controls

	std::function<bool()>stdf_enteroverride_artist = [this]() {

		BOOL bdummy;
		HWND button = uGetDlgItem(IDC_BTN_SEARCH);

		if (::IsWindowEnabled(button)) OnButtonSearch(0L, 0L, NULL, bdummy);
		return false;
	};

	std::function<bool()>stdf_enteroverride_url = [this]() {

		BOOL bdummy;
		HWND button = uGetDlgItem(IDC_BTN_PROCESS_RELEASE);

		if (::IsWindowEnabled(button)) OnButtonNext(0L, 0L, NULL, bdummy);
		return false;
	};

	std::function<bool(HWND hwnd, wchar_t* editval)> m_stdf_call_history = [&](HWND hwnd, wchar_t* wstrt) {

		oplog_type optype =

			hwnd == this->m_edit_artist ? oplog_type::artist :
			hwnd == this->m_edit_release ? oplog_type::release :
			hwnd == this->m_edit_filter ? oplog_type::filter : oplog_type::filter;


		//show & capture menu cmd

		pfc::string8 menu_cmd;
		bool menu_hit = this->get_oplogger()->do_history_menu(optype, hwnd, menu_cmd);

		//process menu cmd

		if (menu_hit) {

			pfc::string8 in = pfc::stringcvt::string_utf8_from_os(wstrt).get_ptr();

			wchar_t wide_menu_cmd[MAX_PATH + 1];

			pfc::stringcvt::convert_utf8_to_wide(wide_menu_cmd, MAX_PATH, menu_cmd, menu_cmd.get_length());

			//transfer result text
			_tcscpy_s(wstrt, MAX_PATH, wide_menu_cmd);

			//return false if input = output
			return (stricmp_utf8(in, menu_cmd) != 0);
		}

		return false;
	};

	void set_history_key_override() {

		cewb_release_filter.SetHistoryHandler(m_stdf_call_history);
		cewb_artist_search.SetHistoryHandler(m_stdf_call_history);
		cewb_release_url.SetHistoryHandler(m_stdf_call_history);
	}

	void set_enter_key_override(bool enter_ovr) {

		if (enter_ovr) {
			cewb_artist_search.SetEnterOverride(stdf_enteroverride_artist);
			cewb_release_url.SetEnterOverride(stdf_enteroverride_url);
		}
		else {
			cewb_artist_search.SetEnterOverride(nullptr);
			cewb_release_url.SetEnterOverride(nullptr);
		}
	}

	void init_cfged_dialog_controls();
	bool build_current_cfg();
	void pushcfg();

	// update releases

	std::pair<rppair_t, rppair_t> update_releases(const pfc::string8& filter, updRelSrc updsrc, bool init_expand);

	// get/search artists service callbacks

	void on_get_artist_done(cupdRelSrc updsrc, Artist_ptr& artist);
	void on_search_artist_done(const pfc::array_t<Artist_ptr>& p_artist_exact_matches, const pfc::array_t<Artist_ptr>& p_artist_other_matches, bool append);

	// route artist search

	DBFlags calc_dbdc_flag();

	void route_artist_search(pfc::string8 artistname, bool dlgbutton, bool idded);

	// release service callbacks

	void call_expand_master_service(MasterRelease_ptr& master_release, int pos); //serves tree
	void on_expand_master_release_done(const MasterRelease_ptr& master_release, int pos, threaded_process_status& p_status, abort_callback& p_abort);
	void on_expand_master_release_complete();

	bool full_olcache() { return ol::can_read() && ol::can_write(); };

	void convey_artist_list_selection(cupdRelSrc in_cupdsrc);

	// on write tags

	void on_write_tags(const pfc::string8& release_id);

	//..

	bool id_from_url(HWND hwndCtrl, pfc::string8& out);

	void set_role_label(bool filtered) {

		pfc::string8 note = (PFC_string_formatter() << "main role " << (filtered ? "&& filtered versions" : ""));
		uSetDlgItemText(m_hWnd, IDC_CHK_FIND_RELEASE_FILTER_ROLEMAIN, note);
	}

	//filter box delay/timer
	bool is_filter_box_event_enabled() { return m_filter_box_events_enabled; }
	void block_filter_box_events(bool state) { m_filter_box_events_enabled = !state; }

	//filter box album autofill...
	bool is_filter_autofill_enabled() { return m_filter_box_autofill_enabled; }
	void block_autofill_release_filter(bool blocked) { m_filter_box_autofill_enabled = !blocked; }

	//timer
	void KFilterTimerOn(CWindow parent) { m_tickCount = 0; SetTimer(KTypeFilterTimerID, KTypeFilterTimeOut); }
	void KTurnOffFilterTimer() throw() { KillTimer(KTypeFilterTimerID); }


	metadb_handle_list items;

	foo_conf conf;
	history_oplog m_oplogger;
	id_tracer m_tracer;

	//CDlgItemsCallback m_items_callback;

	CFindReleaseTree m_dctree;
	CArtistList m_alist;

	rppair m_row_stats;

	HWND m_artist_list;
	HWND m_release_tree;
	HWND m_edit_release, m_edit_artist, m_edit_filter;

	CMyEditWithButtons cewb_artist_search;
	CMyEditWithButtons cewb_release_filter;
	CMyEditWithButtons cewb_release_url;

	CHyperLink artist_link;

	uint32_t m_tickCount;
	bool m_filter_box_events_enabled = true;
	bool m_filter_box_autofill_enabled = true;

	bool m_updating_releases;

	service_ptr_t<titleformat_object> m_album_name_script;
	service_ptr_t<titleformat_object> m_artist_name_script;
	service_ptr_t<titleformat_object> m_album_artist_script;

	service_ptr_t<expand_master_release_process_callback>* m_active_task = nullptr;

	friend class CArtistList;
	friend class ILOD_artist_list;

public:

		decltype(conf)const& config() const { return conf; }

};

static LRESULT CALLBACK EnterKeySubclassProc(
	HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch (uiMsg) {
	case WM_NCDESTROY:
		RemoveWindowSubclass(hwnd, EnterKeySubclassProc,
			uIdSubclass);
		break;
	case WM_GETDLGCODE:

		LPMSG lpMsg = (LPMSG)lParam;

		if (lpMsg == NULL) {

			return 0;
		}

		switch (lpMsg->message) {
		
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:

			switch (lpMsg->wParam) {
			
			case VK_RETURN: {

				//forward call

				bool bwant_return =
					g_discogs->find_release_dialog->ForwardVKReturn();

				if (bwant_return) {

					return DefSubclassProc(hwnd, uiMsg, wParam, lParam)
						& ~VK_RETURN;
				}

				return DefSubclassProc(hwnd, uiMsg, wParam, lParam)
					| DLGC_WANTALLKEYS;
			}
			case VK_ESCAPE:

				return DefSubclassProc(hwnd, uiMsg, wParam, lParam)
					| DLGC_WANTALLKEYS;

			default:
                ;
			}
		default:
			;
		}
	}
	return DefSubclassProc(hwnd, uiMsg, wParam, lParam);
}
