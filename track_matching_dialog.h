#pragma once
#include "resource.h"

#include "libPPUI/CListControlOwnerData.h"
#include "helpers/DarkMode.h"

#include "icon_map.h"
#include "track_matching_dialog_presenter.h"
#include "track_matching_ILO.h"
#include "artwork_list.h"

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
	private ILOD_track_matching, fb2k::CDarkModeHooks {

public:

	// constructor

	CTrackMatchingDialog(HWND p_parent, TagWriter_ptr tag_writer, bool use_update_tags = false) : ILOD_track_matching(tag_writer->release),
		m_tag_writer(tag_writer),	m_conf(CONF), m_coord(p_parent, CONF),
		m_idc_list(this), m_ifile_list(this), m_ida_list(this), m_ifa_list(this)
	{
		m_conf.SetName("TrackMatchingDlg");
		g_discogs->track_matching_dialog = this;
	}

	~CTrackMatchingDialog() {

		g_discogs->track_matching_dialog = nullptr;
	}

	virtual BOOL PreTranslateMessage(MSG* pMsg) override {
		return ::IsDialogMessage(m_hWnd, pMsg);
	}

	enum { IDD = IDD_DIALOG_MATCH_TRACKS };

#pragma warning( push )
#pragma warning( disable : 26454 )

	MY_BEGIN_MSG_MAP(CTrackMatchingDialog)

		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_CUST_UPDATE_SKIP_BUTTON, OnUpdateSkipButton)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnColorStatic)

		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_ID_HANDLER(IDC_CHK_SKIP_ARTWORK, OnCheckSkipArtwork)
		COMMAND_ID_HANDLER(IDC_CHK_MNG_ARTWORK, OnCheckManageArtwork)
		COMMAND_ID_HANDLER(IDC_CHK_ARTWORK_FILEMATCH, OnCheckArtworkFileMatch)

		COMMAND_ID_HANDLER(IDC_BTN_BACK, OnButtonBack)
		COMMAND_ID_HANDLER(IDC_BTN_WRITE_TAGS, OnButtonWriteTags)
		COMMAND_ID_HANDLER(IDC_BTN_PREVIEW_TAGS, OnButtonPreviewTags)
		COMMAND_ID_HANDLER(IDC_BTN_WRITE_ARTWORK, OnButtonWriteArtwork)

		CHAIN_MSG_MAP(CDialogResize<CTrackMatchingDialog>)
	MY_END_MSG_MAP()

#pragma warning( pop )

	BEGIN_DLGRESIZE_MAP(CTrackMatchingDialog)
		DLGRESIZE_CONTROL(IDC_UI_DC_ARTWORK_LIST, DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_UI_FILE_ARTWORK_LIST, DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_STATIC_MATCH_TRACKING_REL_NAME, DLSZ_MOVE_Y)
		BEGIN_DLGRESIZE_GROUP()
		DLGRESIZE_CONTROL(IDC_UI_DC_ARTWORK_LIST, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_UI_FILE_ARTWORK_LIST, DLSZ_SIZE_X)
		END_DLGRESIZE_GROUP()
		DLGRESIZE_CONTROL(IDC_STATIC_MATCH_TRACKING_REL_NAME, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_STATIC_MATCH_TRACKS_MSG, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_BTN_WRITE_ARTWORK, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_CHK_ARTWORK_FILEMATCH, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_BTN_PREVIEW_TAGS, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_BTN_WRITE_TAGS, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_BTN_BACK, DLSZ_MOVE_X)
	END_DLGRESIZE_MAP()

	void DlgResize_UpdateLayout(int cxWidth, int cyHeight) {
		CDialogResize<CTrackMatchingDialog>::DlgResize_UpdateLayout(cxWidth, cyHeight);

		//post-update

		if (!::IsWindowVisible(uGetDlgItem(IDC_UI_LIST_DISCOGS))) return;

		CRect rc_dcList;
		::GetWindowRect(uGetDlgItem(IDC_UI_DC_ARTWORK_LIST), &rc_dcList);
		CRect rc_fileList;
		::GetWindowRect(uGetDlgItem(IDC_UI_FILE_ARTWORK_LIST), &rc_fileList);

		ScreenToClient(&rc_dcList);
		::SetWindowPos(uGetDlgItem(IDC_UI_LIST_DISCOGS), HWND_TOP, rc_dcList.left, rc_dcList.top,
			rc_dcList.Width(), rc_dcList.Height(), SWP_NOACTIVATE | SWP_SHOWWINDOW);
		ScreenToClient(&rc_fileList);
		::SetWindowPos(uGetDlgItem(IDC_UI_LIST_FILES), HWND_TOP, rc_fileList.left, rc_fileList.top,
			rc_fileList.Width(), rc_fileList.Height(), SWP_NOACTIVATE | SWP_SHOWWINDOW);
	}

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnColorStatic(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);

	LRESULT OnButtonPreviewTags(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnButtonBack(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnButtonWriteTags(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnButtonWriteArtwork(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT OnCheckSkipArtwork(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCheckManageArtwork(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCheckArtworkFileMatch(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT OnUpdateSkipButton(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	void match_message_update(pfc::string8 local_msg = "");

	//serves credit preview
	pfc::string8 get_discogs_release_id() { return m_tag_writer->release->id; };

	const metadb_handle_list get_tag_writer_items() {return m_tag_writer->finfo_manager->items; }

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
	void process_file_artwork_preview_done(size_t img_ndx, bool artist_art, imgpairs callback_pair_hbitmaps, std::pair<pfc::string8, pfc::string8> temp_file_names);
	void process_download_art_paths_done(pfc::string8 callback_release_id, std::shared_ptr<std::vector<std::pair<pfc::string8, bit_array_bittable>>> vres,pfc::array_t<GUID> album_art_ids);

	void set_image_list();

	const TCHAR m_szArtist[50] = _T("Artist");
	const TCHAR m_szAlbum[50] = _T("Album");
	const TCHAR m_szWrite[50] = _T("Write");
	const TCHAR m_szOverwrite[50] = _T("Overwrite");
	const TCHAR m_szEmbed[50] = _T("Embed");

	std::shared_ptr<std::vector<pfc::string8>> vres;

	void pending_previews_done(size_t n = 1);

protected:

		void LibUIAsTrackList(bool OwnerToLibUi);

private:

	HWND get_ctx_lvlist(int ID) {

		if (m_coord.GetCtrIDMode(ID) != lsmode::unknown)
			return uGetDlgItem(ID);
		else
			return nullptr;
	}

	CListControlOwnerData* GetUIListById(UINT listID) {
		if (listID == IDC_UI_LIST_DISCOGS) return (CListControlOwnerData*)&m_idc_list;
		else if (listID == IDC_UI_LIST_FILES) return (CListControlOwnerData*)&m_ifile_list;
		else if (listID == IDC_UI_DC_ARTWORK_LIST) return (CListControlOwnerData*)&m_ida_list;
		else if (listID == IDC_UI_FILE_ARTWORK_LIST) return (CListControlOwnerData*)&m_ifa_list;
		else return nullptr;
	}

	CListControlOwnerData* GetUIListByWnd(HWND hwnd) {
		UINT ID = ::GetWindowLong(hwnd, GWL_ID);
		return GetUIListById(ID);
	}

	lsmode get_mode() {
		return uButton_GetCheck(m_hWnd, IDC_CHK_MNG_ARTWORK) == TRUE ?
			lsmode::art : lsmode::default;
	}

	bool build_current_cfg();
	void pushcfg();

	void generate_track_mappings(track_mappings_list_type& track_mappings);
	bool generate_artwork_guids(pfc::array_t<GUID>& my_album_art_ids, bool cfg_default);

	bool context_menu_form(HWND wnd, LPARAM coords);
	bool context_menu_track_show(HWND wnd, int idFrom, LPARAM coords);
	bool context_menu_track_switch(HWND wnd, POINT point, bool isfiles, int cmd,
		bit_array_bittable selmask, bit_array_bittable mixedvals);
	void context_menu_art_attrib_switch(HWND wnd, af afalbum, af afart, UINT IDATT, bool mixedvals);
	bool context_menu_art_attrib_show(HWND wnd, HMENU* menu, bit_array_bittable &mixedvals);

	virtual coord_presenters* ilo_get_coord() override { return &m_coord; }

	foo_conf m_conf;
	multi_uartwork m_last_run_multi_uart;

	size_t m_pending_previews = 0;
	std::mutex m_mx_pending_previews_mod;
	std::vector<preview_job> m_vpreview_jobs;
	void add_pending_previews(size_t n);

	TagWriter_ptr m_tag_writer;

	size_t m_tw_index = 0;
	size_t m_tw_skip = 0;

	file_info_impl info;
	playable_location_impl location;
	titleformat_hook_impl_multiformat hook;

	CListControlOwnerData m_idc_list;
	CListControlOwnerData m_ifile_list;

	CArtworkList					m_ida_list;
	CArtworkList					m_ifa_list;

	coord_presenters m_coord;

	CImageList m_hImageList;

	musicbrainz_info m_musicbrainz_mibs;

	friend CArtworkList;
};
