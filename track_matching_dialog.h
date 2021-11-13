#pragma once

#include "../SDK/foobar2000.h"

#include <libPPUI/CListControlOwnerData.h>

#include "resource.h"

#include "foo_discogs.h"
#include "file_info_manager.h"
#include "multiformat.h"
#include "tag_writer.h"

#include "icon_map.h"

#include "track_match_lstdrop.h"
#include "track_matching_dialog_presenter.h"

using namespace Discogs;

class CTrackMatchingDialog : public MyCDialogImpl<CTrackMatchingDialog>,
	public CDialogResize<CTrackMatchingDialog>,
	public CMessageFilter,
	private IListControlOwnerDataSource 
{
private:

	HBITMAP m_rec_icon;

	HWND get_ctx_lvlist(int ID) {
		HWND dc_list = uGetDlgItem(IDC_UI_LIST_DISCOGS);
		HWND dc_files = uGetDlgItem(IDC_UI_LIST_FILES);
		return (ID == IDC_UI_LIST_DISCOGS ? dc_list : dc_files);
	}

	// IListControlOwnerDataSource methods
	// LIBPPUI - Get Item Count

	size_t listGetItemCount(ctx_t ctx) override {
		// ctx is a pointer to the object calling us
		PFC_ASSERT(ctx == &m_idc_list || ctx == &m_ifile_list); 
		HWND lvlist = get_ctx_lvlist(ctx->GetDlgCtrlID());	

		size_t citems = ctx->GetDlgCtrlID() == IDC_UI_LIST_DISCOGS ?
			m_coord.GetDiscogsTrackUiLvSize() : m_coord.GetFileTrackUiLvSize();
		return citems;
	}

	// LIBPPUI - Get subitems

	pfc::string8 listGetSubItemText(ctx_t ctx, size_t item, size_t subItem) override {
		
		HWND lvlist = get_ctx_lvlist(ctx->GetDlgCtrlID());
		pfc::string8 buffer;
		
		if (ctx->GetDlgCtrlID() == IDC_UI_LIST_FILES) {
			file_it file_match;
			size_t res = m_coord.GetFileTrackUiAtLvPos(item, file_match);
			buffer = subItem == 0 ? file_match->first.first : file_match->first.second;
		}
		else {
			track_it track_match;
			size_t res = m_coord.GetDiscogsTrackUiAtLvPos(item, track_match);
			buffer = subItem == 0 ? track_match->first.first : track_match->first.second;
		}
		return buffer;
	}

	// LIBPPUI - Can reorder

	bool listCanReorderItems(ctx_t) override {
		return true;
	}

	// LIBPPUI - Reorder

	bool listReorderItems(ctx_t ctx, const size_t* order, size_t count) override {

		int listid = ctx->GetDlgCtrlID();
		HWND hlist = uGetDlgItem(listid);

		m_coord.reorder_map_elements(hlist, order, count, lsmode::tracks_ui);
		match_message_update(match_manual);

		return true;
	}

	// LIBPPUI - Remove

	bool listRemoveItems(ctx_t ctx, pfc::bit_array const& mask) override {

		//pfc::remove_mask_t(m_data, mask);

		int listid = ctx->GetDlgCtrlID();
		HWND hlist = uGetDlgItem(listid);
		size_t count = listid == IDC_UI_LIST_DISCOGS ?
			m_coord.GetDiscogsTrackUiLvSize() : m_coord.GetFileTrackUiLvSize();

		bit_array_bittable delmask; delmask.resize(count);
		t_size n, total = 0;
		n = total = mask.find(true, 0, count);
		if (n < count)
		{
			delmask.set(n, true);
			for (n = mask.find(true, n + 1, count - n - 1); n < count; n = mask.find(true, n + 1, count - n - 1)) {
				delmask.set(n, true);
			}
		}
		else return true;

		pfc::array_t<t_uint8> perm_selection;
		perm_selection.resize(delmask.size());
		bit_array_bittable are_albums(delmask.size());
		const size_t max_items = m_tag_writer->get_art_count();

		m_coord.ListUserCmd(hlist, lsmode::tracks_ui, ID_REMOVE, delmask, pfc::bit_array_bittable(), false);
		match_message_update(match_manual);
		return true;
	}

	void listItemAction(ctx_t, size_t item) override {
		//m_list.TableEdit_Start(item, 1);
	}
	void listSubItemClicked(ctx_t, size_t item, size_t subItem) override {
		//if (subItem == 1) {	m_list.TableEdit_Start(item, subItem); }
	}
	void listSetEditField(ctx_t ctx, size_t item, size_t subItem, const char* val) override {
		//if (subItem == 1) {m_data[item].m_value = val; }
	}

	bool listIsColumnEditable(ctx_t, size_t subItem) override {
		return false;
	}

	//end libPPUI list methods

	lsmode GetMode() {
		bool state = uButton_GetCheck(m_hWnd, IDC_TRACK_MATCH_ALBUM_ART) == TRUE;
		return state ? lsmode::art : lsmode::default;
	}

	bool init_count();
	void finished_tag_writers();

	void update_window_title() {
		pfc::string8 text;
		text << "Match Tracks (" << tw_index - tw_skip << "/" << multi_count << ")";
		pfc::stringcvt::string_wide_from_ansi wtext(text);
		SetWindowText((LPCTSTR)const_cast<wchar_t*>(wtext.get_ptr()));
	}

	void load_size();
	bool build_current_cfg();
	void pushcfg();

	void insert_track_mappings();
	void generate_track_mappings(track_mappings_list_type &track_mappings);

	void update_list_width(HWND list, bool initialize=false);
	bool track_context_menu(HWND wnd, LPARAM coords);
	bool track_url_context_menu(HWND wnd, LPARAM coords);
	bool switch_context_menu(HWND wnd, POINT point, bool isfiles, int cmd, bit_array_bittable selmask, CListControlOwnerData* ilist);
	void attrib_menu_command(HWND wnd, af afalbum, af afart, UINT IDATT, lsmode mode);
	bool append_art_context_menu(HWND wnd, HMENU* menu);

protected:

	void LibUIAsOwnerData(bool OwnerToLibUi);

public:

	std::function<bool(HWND wndlist)>stdf_change_notifier =
		//todo: revise, changed after cross reference with credit tag mapping
		[this](HWND x) -> bool {
		match_message_update();
		return true; };

	virtual BOOL PreTranslateMessage(MSG* pMsg) override {
		return ::IsDialogMessage(m_hWnd, pMsg);
	}

	MY_BEGIN_MSG_MAP(CTrackMatchingDialog)
		MSG_WM_CONTEXTMENU(OnContextMenu)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnColorStatic)
		MESSAGE_HANDLER(WM_RBUTTONUP, OnRButtonUp)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		NOTIFY_HANDLER_EX(IDC_FILE_LIST, NM_RCLICK, OnListRClick)
		NOTIFY_HANDLER_EX(IDC_FILE_LIST, NM_DBLCLK, OnListDBLClick)
		NOTIFY_HANDLER_EX(IDC_DISCOGS_TRACK_LIST, NM_DBLCLK, OnListDBLClick)
		NOTIFY_HANDLER_EX(IDC_DISCOGS_TRACK_LIST, NM_RCLICK, OnListRClick)
		NOTIFY_HANDLER_EX(IDC_UI_LIST_DISCOGS, NM_RCLICK, OnListRClick)
		NOTIFY_HANDLER_EX(IDC_UI_LIST_FILES, NM_RCLICK, OnListRClick)
		NOTIFY_HANDLER_EX(IDC_FILE_LIST, LVN_KEYDOWN, OnListKeyDown)
		NOTIFY_HANDLER_EX(IDC_DISCOGS_TRACK_LIST, LVN_KEYDOWN, OnListKeyDown)
		NOTIFY_HANDLER_EX(IDC_DISCOGS_TRACK_LIST, LVN_GETDISPINFO, OnDiscogListGetDispInfo)
		NOTIFY_HANDLER_EX(IDC_FILE_LIST, LVN_GETDISPINFO, OnDiscogListGetDispInfo)
		COMMAND_ID_HANDLER(IDC_PREVIEW_TAGS_BUTTON, OnPreviewTags)
		COMMAND_ID_HANDLER(IDC_WRITE_TAGS_BUTTON, OnWriteTags)
		COMMAND_ID_HANDLER(IDC_NEXT_BUTTON, OnMultiNext)
		COMMAND_ID_HANDLER(IDC_PREVIOUS_BUTTON, OnMultiPrev)
		COMMAND_ID_HANDLER(IDC_BACK_BUTTON, OnBack)
		COMMAND_ID_HANDLER(IDC_BTN_TRACK_MATCH_WRITE_ARTWORK, OnWriteArtwork)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_COMMAND, OnCommand)
		COMMAND_ID_HANDLER(IDC_SKIP_BUTTON, OnMultiSkip)
		CHAIN_MSG_MAP_MEMBER(list_drop_handler)
		CHAIN_MSG_MAP(CDialogResize<CTrackMatchingDialog>)
	MY_END_MSG_MAP()
	BEGIN_DLGRESIZE_MAP(CTrackMatchingDialog)
		DLGRESIZE_CONTROL(IDC_TRACKLIST_GROUP, DLSZ_SIZE_X | DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_DISCOGS_TRACK_LIST, DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_FILE_LIST, DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_STATIC_MATCH_TRACKING_REL_NAME, DLSZ_MOVE_Y)
		BEGIN_DLGRESIZE_GROUP()
		DLGRESIZE_CONTROL(IDC_STATIC_FILE_LST_TITLE, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_DISCOGS_TRACK_LIST, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_FILE_LIST, DLSZ_SIZE_X)
		END_DLGRESIZE_GROUP()
		DLGRESIZE_CONTROL(IDC_STATIC_MATCH_TRACKING_REL_NAME, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_MATCH_TRACKS_MSG, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_BTN_TRACK_MATCH_WRITE_ARTWORK, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_BACK_BUTTON, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_PREVIEW_TAGS_BUTTON, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_WRITE_TAGS_BUTTON, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_PREVIOUS_BUTTON, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_NEXT_BUTTON, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_SKIP_BUTTON, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_TRACK_MATCH_ALBUM_ART, DLSZ_MOVE_X)
	END_DLGRESIZE_MAP()

	void DlgResize_UpdateLayout(int cxWidth, int cyHeight) {
		CDialogResize<CTrackMatchingDialog>::DlgResize_UpdateLayout(cxWidth, cyHeight);
		//fix dlgresize
		if (!::IsWindowVisible(uGetDlgItem(IDC_UI_LIST_DISCOGS))) return;

		CRect rc_dcList;
		::GetWindowRect(uGetDlgItem(IDC_DISCOGS_TRACK_LIST), &rc_dcList);
		CRect rc_fileList;
		::GetWindowRect(uGetDlgItem(IDC_FILE_LIST), &rc_fileList);

		ScreenToClient(&rc_dcList);
		::SetWindowPos(uGetDlgItem(IDC_UI_LIST_DISCOGS), HWND_TOP, rc_dcList.left, rc_dcList.top,
			rc_dcList.Width(), rc_dcList.Height(), SWP_NOACTIVATE | SWP_SHOWWINDOW);
		ScreenToClient(&rc_fileList);
		::SetWindowPos(uGetDlgItem(IDC_UI_LIST_FILES), HWND_TOP, rc_fileList.left, rc_fileList.top,
			rc_fileList.Width(), rc_fileList.Height(), SWP_NOACTIVATE | SWP_SHOWWINDOW);
	}

	CTrackMatchingDialog(HWND p_parent, TagWriter_ptr tag_writer, bool use_update_tags = false) : 
		m_tag_writer(tag_writer), use_update_tags(use_update_tags),	list_drop_handler(),
		m_conf(CONF), m_coord(p_parent, CONF),
		m_idc_list(this), m_ifile_list(this) {

		g_discogs->track_matching_dialog = this;

		m_rec_icon = LoadDpiBitmapResource(Icon::Record);
	}
	
	CTrackMatchingDialog(HWND p_parent, pfc::array_t<TagWriter_ptr> tag_writers, bool use_update_tags = false) :
		tag_writers(tag_writers), use_update_tags(use_update_tags), multi_mode(true), list_drop_handler(),
		m_conf(CONF), m_coord(p_parent, CONF),
		m_idc_list(this), m_ifile_list(this) {

		g_discogs->track_matching_dialog = this;

		m_tag_writer = nullptr;

		if (init_count()) {
			m_rec_icon = LoadDpiBitmapResource(Icon::Record);
		}
		else {
			finished_tag_writers();
			destroy();
		}
	}

	~CTrackMatchingDialog() {
		DeleteObject(m_rec_icon);
		g_discogs->track_matching_dialog = nullptr;
	}

	void OnContextMenu(CWindow wnd, CPoint point) {

		UINT id = ::GetWindowLong(wnd, GWL_ID);
		if (id == IDC_UI_LIST_DISCOGS || id == IDC_UI_LIST_FILES) {
			NMHDR nmhdr = { 0 };
			nmhdr.hwndFrom = wnd;
			nmhdr.idFrom = id;
			OnListRClick(&nmhdr);
		}
		return;
	};

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
	LRESULT OnWriteArtwork(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnWriteArtworkKnownIds(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCommand(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnMultiSkip(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnDiscogListGetDispInfo(LPNMHDR lParam);
	LRESULT DiscogTrackGetDispInfo(LPNMHDR lParam);
	LRESULT DiscogArtGetDispInfo(LPNMHDR lParam);
	LRESULT OnListKeyDown(LPNMHDR lParam);
	LRESULT OnListRClick(LPNMHDR lParam);
	LRESULT OnListDBLClick(LPNMHDR lParam);
	LRESULT OnRButtonUp(UINT ctrl_id, WPARAM, LPARAM lParam, BOOL&);
	LRESULT list_key_down(HWND wnd, LPNMHDR lParam);

	bool initialize_next_tag_writer();
	void match_message_update(pfc::string8 local_msg = "");
	bool get_next_tag_writer();
	bool get_previous_tag_writer();

	//credits preview
	pfc::string8 get_discogs_release_id() { return m_tag_writer->release->id; };

	void enable(bool v) override;
	void destroy_all();
	void go_back();

	size_t get_art_perm_selection(HWND list, bool flagselected, const size_t max_items, pfc::array_t<t_uint8>& selmask, bit_array_bittable& selalbum);
	void request_preview(size_t img_ndx, bool artist_art, bool onlycache);
	void request_file_preview(size_t img_ndx, bool artist_art);

	void process_artwork_preview_done(size_t img_ndx, bool artist_art, MemoryBlock callback_mb);
	void process_file_artwork_preview_done(size_t img_ndx, bool artist_art, std::pair<HBITMAP, HBITMAP> callback_pair_hbitmaps, std::pair<pfc::string8, pfc::string8> temp_file_names);
	void process_download_art_paths_done(pfc::string8 callback_release_id, std::shared_ptr<std::vector<std::pair<pfc::string8, bit_array_bittable>>> vres,pfc::array_t<GUID> album_art_ids);

public:

	const TCHAR m_szArtist[50] = _T("Artist");
	const TCHAR m_szAlbum[50] = _T("Album");
	const TCHAR m_szWrite[50] = _T("Write");
	const TCHAR m_szOverwrite[50] = _T("Overwrite");
	const TCHAR m_szEmbed[50] = _T("Embed");

	enum { IDD = IDD_DIALOG_MATCH_TRACKS };

	std::shared_ptr<std::vector<pfc::string8>> vres;

private:

	foo_discogs_conf m_conf;

	bool use_update_tags = false;
	bool multi_mode = false;
	size_t multi_count = 0;

	TagWriter_ptr m_tag_writer;
	pfc::array_t<TagWriter_ptr> tag_writers;

	size_t tw_index = 0;
	size_t tw_skip = 0;

	file_info_impl info;
	playable_location_impl location;
	titleformat_hook_impl_multiformat hook;

	MatchListDropHandler list_drop_handler;

	CListControlOwnerData m_idc_list;
	CListControlOwnerData m_ifile_list;

	coord_presenters m_coord;
};
