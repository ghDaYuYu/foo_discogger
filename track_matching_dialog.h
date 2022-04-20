#pragma once
#include "resource.h"


#include "icon_map.h"
#include "track_matching_dialog_presenter.h"
#include "track_match_lstdrop.h"

#include "track_matching_ILOsrc_uilists.h"
#include "libPPUI\CListControlOwnerData.h"

using namespace Discogs;

struct musicbrainz_info {
	pfc::string8 release_group;
	pfc::string8 release;
	pfc::string8 artist;
	bool coverart = false;

	bool empty() {
		return (
		!release_group.get_length() &&
		!release.get_length() &&
		!artist.get_length() &&
		!coverart);
	}
};

struct preview_job {
	preview_job(bool aisfile, size_t anindex_art, bool anartist_art, bool anonly_cache, bool aget_mibs) :
		isfile(aisfile), index_art(anindex_art), artist_art(anartist_art), only_cache(anonly_cache), get_mibs(aget_mibs) {
		preview_job::inc++;
	};

	bool isfile;
	size_t index_art;
	bool artist_art;
	bool only_cache;
	bool get_mibs;

	inline static size_t inc;
};

class CTrackMatchingDialog : public MyCDialogImpl<CTrackMatchingDialog>,
	public CDialogResize<CTrackMatchingDialog>,	public CMessageFilter,
	private ILODsrc/*IListControlOwnerDataSource*/ {

private:

	HWND get_ctx_lvlist(int ID) {
		if (ID != IDC_UI_LIST_DISCOGS && ID != IDC_UI_LIST_FILES) return nullptr;
		return uGetDlgItem(ID);		
	}

	//.. Dlg lists

	bool init_count();

	lsmode get_mode() {
		return uButton_GetCheck(m_hWnd, IDC_TRACK_MATCH_ALBUM_ART) == TRUE ?
			lsmode::art : lsmode::default;
	}

	void load_size();
	bool build_current_cfg();
	void pushcfg();

	void insert_track_mappings();
	void generate_track_mappings(track_mappings_list_type &track_mappings);

	void update_list_width(HWND list, bool initialize=false);
	bool context_menu_form(HWND wnd, LPARAM coords);
	bool context_menu_track_show(HWND wnd, int idFrom, LPARAM coords);
	bool context_menu_track_switch(HWND wnd, POINT point, bool isfiles, int cmd, bit_array_bittable selmask, pfc::array_t<t_size> order, CListControlOwnerData* ilist);
	void context_menu_art_attrib_switch(HWND wnd, af afalbum, af afart, UINT IDATT, lsmode mode);
	bool context_menu_art_attrib_show(HWND wnd, HMENU* menu);

protected:

	void LibUIAsOwnerData(bool OwnerToLibUi);

public:

	std::function<bool(HWND wndlist, size_t item)>stdf_change_notifier =
		//todo: revise, modded after cross referenced credit tag mapping
		[this](HWND x, size_t i) -> bool {


		NMLVKEYDOWN info;
		info.wVKey = 0x9F;
		info.flags = i;
		list_key_down(x, (LPNMHDR)&info);

		return true; };

	virtual BOOL PreTranslateMessage(MSG* pMsg) override {
		return ::IsDialogMessage(m_hWnd, pMsg);
	}

#pragma warning( push )
#pragma warning( disable : 26454 )

	MY_BEGIN_MSG_MAP(CTrackMatchingDialog)

		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnColorStatic)

		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_ID_HANDLER(IDC_CHECK_SKIP_ARTWORK, OnCheckSkipArtwork)
		COMMAND_ID_HANDLER(IDC_TRACK_MATCH_ALBUM_ART, OnCheckManageArtwork)

		NOTIFY_HANDLER_EX(IDC_FILE_LIST, NM_DBLCLK, OnListDoubleClick)
		NOTIFY_HANDLER_EX(IDC_DISCOGS_TRACK_LIST, NM_DBLCLK, OnListDoubleClick)

		NOTIFY_HANDLER_EX(IDC_FILE_LIST, LVN_KEYDOWN, OnListKeyDown)
		NOTIFY_HANDLER_EX(IDC_DISCOGS_TRACK_LIST, LVN_KEYDOWN, OnListKeyDown)

		NOTIFY_HANDLER_EX(IDC_FILE_LIST, LVN_GETDISPINFO, OnDiscogListGetDispInfo)
		NOTIFY_HANDLER_EX(IDC_DISCOGS_TRACK_LIST, LVN_GETDISPINFO, OnDiscogListGetDispInfo)

		COMMAND_ID_HANDLER(IDC_BACK_BUTTON, OnButtonBack)
		COMMAND_ID_HANDLER(IDC_WRITE_TAGS_BUTTON, OnButtonWriteTags)
		COMMAND_ID_HANDLER(IDC_PREVIEW_TAGS_BUTTON, OnButtonPreviewTags)
		COMMAND_ID_HANDLER(IDC_WRITE_ARTWORK_BUTTON, OnButtonWriteArtwork)


		CHAIN_MSG_MAP_MEMBER(m_list_drop_handler)
		CHAIN_MSG_MAP(CDialogResize<CTrackMatchingDialog>)
	MY_END_MSG_MAP()

#pragma warning( pop )

	BEGIN_DLGRESIZE_MAP(CTrackMatchingDialog)
		DLGRESIZE_CONTROL(IDC_DISCOGS_TRACK_LIST, DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_FILE_LIST, DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_STATIC_MATCH_TRACKING_REL_NAME, DLSZ_MOVE_Y)
		BEGIN_DLGRESIZE_GROUP()
		DLGRESIZE_CONTROL(IDC_DISCOGS_TRACK_LIST, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_FILE_LIST, DLSZ_SIZE_X)
		END_DLGRESIZE_GROUP()
		DLGRESIZE_CONTROL(IDC_STATIC_MATCH_TRACKING_REL_NAME, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_MATCH_TRACKS_MSG, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_WRITE_ARTWORK_BUTTON, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_PREVIEW_TAGS_BUTTON, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_WRITE_TAGS_BUTTON, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_BACK_BUTTON, DLSZ_MOVE_X)
	END_DLGRESIZE_MAP()

	void DlgResize_UpdateLayout(int cxWidth, int cyHeight) {
		CDialogResize<CTrackMatchingDialog>::DlgResize_UpdateLayout(cxWidth, cyHeight);

		//omitted by dlgresize...

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


	// constructor

	CTrackMatchingDialog(HWND p_parent, TagWriter_ptr tag_writer, bool use_update_tags = false) : ILODsrc(&this->m_coord, &this->m_idc_list, &this->m_ifile_list),
		m_tag_writer(tag_writer), m_list_drop_handler(),
		m_conf(CONF), m_coord(p_parent, CONF),
		m_idc_list(this), m_ifile_list(this)
	{
		m_conf.SetName("TrackMatchingDlg");
		g_discogs->track_matching_dialog = this;
		m_rec_icon = LoadDpiBitmapResource(Icon::Record);

	}

	~CTrackMatchingDialog() {

		//DeleteObject(m_hImageList);
		DeleteObject(m_rec_icon);
		g_discogs->track_matching_dialog = nullptr;
	}

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	LRESULT OnColorStatic(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT OnMoveTrackUp(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnMoveTrackDown(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT OnButtonPreviewTags(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnButtonBack(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnButtonWriteTags(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnButtonWriteArtwork(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT OnCheckSkipArtwork(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCheckManageArtwork(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT OnWriteArtworkKnownIds(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	LRESULT OnDiscogListGetDispInfo(LPNMHDR lParam);
	LRESULT DiscogTrackGetDispInfo(LPNMHDR lParam);
	LRESULT DiscogArtGetDispInfo(LPNMHDR lParam);

	LRESULT OnListKeyDown(LPNMHDR lParam);
	LRESULT OnListDoubleClick(LPNMHDR lParam);

	LRESULT list_key_down(HWND wnd, LPNMHDR lParam);

	void match_message_update(pfc::string8 local_msg = "");

	//serves credit preview
	pfc::string8 get_discogs_release_id() { return m_tag_writer->release->id; };

	void enable(bool v) override;
	void destroy_all();
	void go_back();

	void init_track_matching_dialog();
	void show() override;
	void hide() override;

	size_t get_art_perm_selection(HWND list, bool flagselected, const size_t max_items, pfc::array_t<t_uint8>& selmask, bit_array_bittable& selalbum);
	void request_preview(size_t img_ndx, bool artist_art, bool onlycache, bool get_mibs = false);
	void request_file_preview(size_t img_ndx, bool artist_art);

	void process_artwork_preview_done(size_t img_ndx, bool artist_art, MemoryBlock callback_mb, musicbrainz_info musicbrainz_mibs);
	void process_file_artwork_preview_done(size_t img_ndx, bool artist_art, std::pair<HBITMAP, HBITMAP> callback_pair_hbitmaps, std::pair<pfc::string8, pfc::string8> temp_file_names);
	void process_download_art_paths_done(pfc::string8 callback_release_id, std::shared_ptr<std::vector<std::pair<pfc::string8, bit_array_bittable>>> vres,pfc::array_t<GUID> album_art_ids);

	void set_image_list();

public:

	const TCHAR m_szArtist[50] = _T("Artist");
	const TCHAR m_szAlbum[50] = _T("Album");
	const TCHAR m_szWrite[50] = _T("Write");
	const TCHAR m_szOverwrite[50] = _T("Overwrite");
	const TCHAR m_szEmbed[50] = _T("Embed");

	enum { IDD = IDD_DIALOG_MATCH_TRACKS };

	std::shared_ptr<std::vector<pfc::string8>> vres;

	void pending_previews_done(size_t n = 1);

private:

	foo_conf m_conf;
	uartwork m_last_run_uart;

	std::vector<preview_job> m_vpreview_jobs;
	size_t m_pending_previews = 0;
	std::mutex m_mx_pending_previews_mod;
	void add_pending_previews(size_t n);

	TagWriter_ptr m_tag_writer;

	size_t tw_index = 0;
	size_t tw_skip = 0;

	file_info_impl info;
	playable_location_impl location;
	titleformat_hook_impl_multiformat hook;

	MatchListDropHandler m_list_drop_handler;

	CListControlOwnerData m_idc_list;
	CListControlOwnerData m_ifile_list;

	coord_presenters m_coord;

	HBITMAP m_rec_icon;
	CImageList m_hImageList;

	musicbrainz_info m_musicbrainz_mibs;
};
