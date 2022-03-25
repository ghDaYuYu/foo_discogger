#pragma once
#include <atltypes.h>
#include <windef.h>
#include "resource.h"
#include "libPPUI\clipboard.h"

#include "discogs.h"
#include "discogs_interface.h"
#include "discogs_db_interface.h"

#include "history_oplog.h"
#include "find_release_utils.h"
#include "find_release_artist_dlg.h"

using namespace Discogs;

//#define DEBUG_TREE

enum FilterFlag { 
	
	Versions = 1 << 0,	RoleMain = 1 << 4,
	
};

class CFindReleaseDialog;
class CFindReleaseTree;

class release_tree_cache {

private:

	//bulk cache
	std::shared_ptr<filter_cache> m_cache_ptr;

	//referenced cache items
	std::shared_ptr<vec_t> m_vec_ptr;

	//filter mask
	std::vector<std::pair<int, int>> m_vec_filter;

	//owner messagemap
	CFindReleaseTree* m_rt_manager;


public:

	//constructor

	release_tree_cache(CFindReleaseTree* rt_manager) : m_rt_manager(rt_manager) {

		m_cache_ptr = std::make_shared<filter_cache>();
		m_vec_ptr = std::make_shared<vec_t>();
	}

	~release_tree_cache() {
		//..
	}

private:

	//stubs
	std::shared_ptr<filter_cache>& get_bulk() { return m_cache_ptr; }
	void bulk_Clear() {	m_cache_ptr->cache.clear();	}
	void vec_Clear() { m_vec_ptr.get()->clear(); }
	size_t bulk_Size() { return m_cache_ptr->cache.size(); }
	size_t vec_Size() { return m_vec_ptr->size(); }
	void new_Version() { m_cache_ptr->_ver++; }
	int get_Version() { return m_cache_ptr->_ver; }
	void vec_PushBack(std::pair<cache_iterator_t, HTREEITEM> item);
	//..

	//update releases

	rppair_t init_filter(const Artist_ptr find_release_artist, pfc::string8 strfilter, bool expanded, bool fast, bool no_alloc, foo_conf* conf_ptr, id_tracer* tracer_p);
	void expand_releases(const pfc::string8& filter, updRelSrc updsrc, t_size master_index, t_size master_list_pos, foo_conf* conf_p, id_tracer* tracer_p);

	std::pair<rppair_t, rppair_t> update_releases(const pfc::string8 & filter, updRelSrc updsrc,
		bool init_expand, id_tracer* tracer_r, bool artist_changed, bool brolemain_filter, foo_conf* conf_p);

	int get_src_param(updRelSrc updsrc, id_tracer* tracer_p);

	//serves rt manager
	const std::shared_ptr<vec_t> get_vec() { return m_vec_ptr; }
	std::shared_ptr<vec_t>& get_vec_ref() { return m_vec_ptr; }
	const std::vector<std::pair<int, int>> get_filter_vec() { return m_vec_filter; }
	bool get_cached_find_release_node(int lparam, pfc::string8& item, row_col_data& rowdata);
	t_size get_level_one_vec_track_count(LPARAM lparam);
	t_size get_level_two_cache_track_count(LPARAM lparam); //for master releases
	//..

	friend CFindReleaseTree;
};

class CFindReleaseTree : public CMessageMap {

private:

	CFindReleaseDialog* m_dlg = nullptr;
	foo_conf* conf_p = nullptr;
	history_oplog* m_oplogger_p;

	id_tracer* _idtracer_p;

	release_tree_cache m_rt_cache;


	HWND m_hwndParent;
	HWND m_hwndTreeView;
	HWND m_release_url_edit,m_filter_edit;

	HTREEITEM m_tvi_selected = NULL;
	HTREEITEM m_hit = NULL;


	DiscogsInterface* m_discogs_interface;	
	Artist_ptr m_find_release_artist;
	pfc::array_t<Artist_ptr> m_find_release_artists;

	bool m_dispinfo_enabled;

	pfc::string8 m_results_filter;

	std::function<bool(int lparam)>stdf_on_release_selected_notifier;
	std::function<bool()>stdf_on_ok_notifier;

	pfc::string8 get_param_id(mounted_param myparam);
	int get_param_id_master(mounted_param myparam);

	size_t test_hit();

	file_info_impl* m_info_p;
	metadb_handle_list m_items;

public:

	const unsigned m_ID;

	void set_results_filter(pfc::string8 filter) { m_results_filter = filter; }
	pfc::string8 get_results_filter() { return m_results_filter; }

	CFindReleaseTree(HWND hwndParent, unsigned ID) : /*CContainedWindowT<CTreeViewCtrl>(core_api::get_main_window(),*/
		m_ID(ID),  m_tvi_selected(NULL), m_rt_cache(this) {

		m_dispinfo_enabled = true;
		m_hit = nullptr;

		m_hwndParent = hwndParent;
		m_hwndTreeView = NULL;

		m_release_url_edit = NULL;
		m_filter_edit = NULL;

		m_discogs_interface = nullptr;
		m_find_release_artist = nullptr;
		m_find_release_artists = {};
	}

	~CFindReleaseTree() {
        //..
	}

	playable_location_impl location;
	titleformat_hook_impl_multiformat_ptr hook;
	pfc::string8 run_hook_columns(row_col_data& row_data, int item_data);

	void expand_releases(const pfc::string8& filter, updRelSrc updsrc, t_size master_index, t_size master_list_pos);
	void on_expand_master_release_done(const MasterRelease_ptr& master_release, int list_index, threaded_process_status& p_status, abort_callback& p_abort);
	
	//update & filter

	std::pair<rppair_t, rppair_t> update_releases(const pfc::string8& filter, updRelSrc updsrc, bool init_expand, bool brolemain_filter);
	LRESULT apply_filter(pfc::string8 strFilter, bool force_redraw, bool force_rebuild);

	//void on_get_artist_done_new(updRelSrc updsrc, Artist_ptr& artist);
	void on_get_artist_done(updRelSrc updsrc, Artist_ptr& artist);

	void on_release_selected(int src_lparam);

	bool set_node_expanded(t_size master_id, int& state, bool build);
	bool get_node_expanded(t_size master_id, int& out_state);


	void set_selected_notifier(std::function<bool(int)>stdf_notifier) {
		stdf_on_release_selected_notifier = stdf_notifier;
	}

	void Inititalize(CFindReleaseDialog* dlg,
			HWND treeview, /*HWND artistlist,*/ HWND filter_edit, HWND url_edit, /*HWND search_edit,*/
			DiscogsInterface* discogs_interface,
			foo_conf* conf_ptr, /*history_oplog* oplogger,*/ id_tracer* idtracer_p,
			/*file_info_impl* info,*/
			metadb_handle_list items) {

		conf_p = conf_ptr;

		_idtracer_p = idtracer_p;
		_idtracer_p->enabled = true;

		m_discogs_interface = discogs_interface;

		m_dlg = dlg;
		m_filter_edit = filter_edit;
		m_release_url_edit = url_edit;


		m_hwndTreeView = treeview;
		m_items = items;
	}

	DiscogsInterface* get_discogs_interface() {	return m_discogs_interface; }
	titleformat_hook_impl_multiformat_ptr get_hook() { return hook; }

	void SetHit(int lparam) {
		
		CTreeViewCtrl tree(m_hwndTreeView);
		HTREEITEM first = tree.GetRootItem();

		for (HTREEITEM walk = first; walk != NULL; walk = tree.GetNextVisibleItem(walk)) {

			TVITEM pItemMaster = { 0 };
			pItemMaster.mask = TVIF_PARAM;
			pItemMaster.hItem = walk;
			TreeView_GetItem(m_hwndTreeView, &pItemMaster);
			if (pItemMaster.lParam == lparam) {
				m_hit = walk;
				break;
			}
		}
	}

	void ExpandHit() {

#ifdef DEBUG_TREE
		log_msg(PFC_string_formatter() << " frt expandhit, m_hhit != null ? " << (m_hhit != NULL ? "true" : "false"));
#endif 

		if (m_hit != NULL) {
			TreeView_Expand(m_hwndTreeView, m_hit, TVM_EXPAND);
			m_hit = nullptr;
		}
	}


	// DEFAULT ACTIONS (nVKReturn)

	void nVKReturn_Releases();

	//..

	void EnableDispInfo(bool enable) { m_dispinfo_enabled = enable;	}

	//message handlers are chained, owner should not map

#pragma warning( push )
#pragma warning( disable : 26454 )

	BEGIN_MSG_MAP(CFindReleaseTree)
		NOTIFY_HANDLER(IDC_RELEASE_TREE, TVN_GETDISPINFO, OnReleaseTreeGetInfo)
		NOTIFY_HANDLER(IDC_RELEASE_TREE, TVN_ITEMEXPANDING, OnReleaseTreeExpanding)
		NOTIFY_HANDLER(IDC_RELEASE_TREE, TVN_SELCHANGING, OnReleaseTreeSelChanging)
		NOTIFY_HANDLER(IDC_RELEASE_TREE, TVN_SELCHANGED, OnReleaseTreeSelChanged)
		NOTIFY_HANDLER(IDC_RELEASE_TREE, NM_DBLCLK, OnReleaseTreeDoubleClickRelease)
		NOTIFY_HANDLER(IDC_RELEASE_TREE, NM_RCLICK, OnRClickRelease)
	END_MSG_MAP()

#pragma warning(pop)

private:

	//releases arrays setters/getters...
	//todo: shorter names
	
	//artist releases
	Artist_ptr get_find_release_artist() {
		return m_find_release_artist;
	}
	//set artist releases
	void set_find_release_artist(Artist_ptr find_release_artist_p/*, id_tracer idtracer*/) {
		m_find_release_artist = find_release_artist_p;
	}
	//artists releases
	pfc::array_t<Artist_ptr> get_find_release_artists() {
		return m_find_release_artists;
	}
	//artists releases
	void set_find_release_artists(pfc::array_t<Artist_ptr> find_release_artists_p) {
		m_find_release_artists = find_release_artists_p;
	}
	//..


	void init_tracker_i(pfc::string8 filter_master, pfc::string8 filter_release, bool expanded, bool fast);
	void rebuild_treeview();

	pfc::string8 get_edit_filter_string() { return uGetWindowText(m_filter_edit); }


	LRESULT OnRClickRelease(int, LPNMHDR hdr, BOOL&);
	LRESULT OnClick(WORD /*wNotifyCode*/, LPNMHDR /*lParam*/, BOOL& /*bHandled*/);

	LRESULT OnReleaseTreeDoubleClickRelease(int, LPNMHDR hdr, BOOL&);

	LRESULT OnReleaseTreeGetInfo(WORD /*wNotifyCode*/, LPNMHDR hdr, BOOL& /*bHandled*/);
	LRESULT OnReleaseTreeExpanding(int, LPNMHDR hdr, BOOL&);
	LRESULT OnReleaseTreeSelChanging(int, LPNMHDR hdr, BOOL& bHandled);
	LRESULT OnReleaseTreeSelChanged(int, LPNMHDR hdr, BOOL& bHandled);

	friend class release_tree_cache;
};