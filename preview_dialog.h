#pragma once
#include "resource.h"
#include "icon_map.h"
#include "CGdiPlusBitmap.h"
#include "foo_discogs.h"
#include "tag_writer.h"

struct preview_stats {
	int totalwrites = 0;
	int totalupdates = 0;
	int totalsubwrites = 0;
	int totalsubupdates = 0;
	int totalskips = 0;
	int totalequal = 0;
};

class CPreviewTagsDialog : public MyCDialogImpl<CPreviewTagsDialog>,
	public CDialogResize<CPreviewTagsDialog>, public CMessageFilter,
	private InPlaceEdit::CTableEditHelperV2_ListView
{

private:

	HWND m_results_list;
	HBITMAP m_rec_icon;
	HBITMAP m_preview_bitmap;

	foo_conf conf;

	std::vector<preview_stats> v_stats;
	std::vector<std::pair<int, int>> vec_icol_subitems;

	bool generating_tags = false;
	
	int totalwrites = 0;
	int totalupdates = 0;

	bool cfg_preview_dialog_show_stats = false;

	TagWriter_ptr m_tag_writer;

	size_t tw_index = 0;
	size_t tw_skip = 0;

	void reset_default_columns(HWND wndlist, bool breset);
	void update_sorted_icol_map(bool reset);

	bool init_count();

	void insert_tag_result(int pos, const tag_result_ptr &result, PreView preview_mode);

	void compute_stats(tag_results_list_type tag_results);
	void compute_stats_track_map();

	void reset_stats () { v_stats.clear(); }
	void reset_tag_result_stats();

	void set_preview_mode(PreView mode);
	PreView get_preview_mode();

	bool initialize();
	void GlobalReplace_ANV(bool state);

	bool is_result_editable(size_t item);

	bool TableEdit_IsColumnEditable(size_t subItem) const override {
		return subItem == 1;
	}

	size_t TableEdit_GetColumnCount() const override {
		return ListView_GetColumnCount(m_results_list);
	}

	HWND TableEdit_GetParentWnd() const override {
		return m_results_list;
	}

	void TableEdit_GetField(size_t item, size_t subItem, pfc::string_base & out, size_t & lineCount) override;
	void TableEdit_SetField(size_t item, size_t subItem, const char * value) override;

	void trigger_edit(size_t item, size_t sub_item);

	bool delete_selection();

	void set_image_list();

	bool context_menu_show(HWND wnd, size_t isel, LPARAM lParamPos);
	bool context_menu_switch(HWND wnd, POINT point, int cmd, bit_array_bittable selmask);

public:
	enum {
		IDD = IDD_DIALOG_PREVIEW_TAGS,
		MSG_EDIT = WM_APP + 5000
	};

	enum {
		ID_PREVIEW_CMD_BACK = 1,
		ID_PREVIEW_CMD_SKIP_ARTWORK,
		ID_PREVIEW_CMD_SHOW_STATS,
		ID_PREVIEW_CMD_RESULTS_VIEW,
		ID_PREVIEW_CMD_DIFF_VIEW,
		ID_PREVIEW_CMD_ORI_VIEW,

		ID_PREVIEW_CMD_EDIT_RESULT_TAG,
		ID_PREVIEW_CMD_EDIT_RESULT_TAGINP,
		ID_PREVIEW_CMD_WRITE_TAGS,
		ID_PREVIEW_CMD_COPY,
		ID_SELECT_ALL,
		ID_INVERT_SELECTION, 
	};

	enum {
		FLG_TAGMAP_DLG_ATTACHED = 1 << 0,
		FLG_TAGMAP_DLG_OPENED = 1 << 1,
	};

	virtual BOOL PreTranslateMessage(MSG* pMsg) override {
		return ::IsDialogMessage(m_hWnd, pMsg);
	}

	void load_size();
	bool build_current_cfg();
	void pushcfg();

#pragma warning( push )
#pragma warning( disable : 26454 )

	MY_BEGIN_MSG_MAP(CPreviewTagsDialog)
		
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_ID_HANDLER(IDC_BTN_BACK, OnButtonBack)
		COMMAND_ID_HANDLER(IDC_BTN_TAG_MAPPINGS, OnButtonEditTagMappings)
		COMMAND_ID_HANDLER(IDC_BTN_WRITE_TAGS, OnButtonWriteTags)
		
		COMMAND_ID_HANDLER(IDC_CHK_REPLACE_ANV, OnCheckReplaceANVs)
		COMMAND_ID_HANDLER(IDC_CHK_PREV_DLG_SHOW_STATS, OnCheckPreviewShowStats)
		COMMAND_ID_HANDLER(IDC_CHK_SKIP_ARTWORK, OnCheckSkipArtwork)
		COMMAND_ID_HANDLER(IDC_CHK_BIND_TAGS_DLG, OnCheckAttachMappingPanel)

		COMMAND_ID_HANDLER(IDC_VIEW_NORMAL, OnChangePreviewMode)
		COMMAND_ID_HANDLER(IDC_VIEW_DIFFERENCE, OnChangePreviewMode)
		COMMAND_ID_HANDLER(IDC_VIEW_ORIGINAL, OnChangePreviewMode)
		COMMAND_ID_HANDLER(IDC_VIEW_DEBUG, OnChangePreviewMode)

		NOTIFY_HANDLER_EX(IDC_PREVIEW_LIST, NM_CLICK, OnListClick)
		NOTIFY_HANDLER_EX(IDC_PREVIEW_LIST, NM_DBLCLK, OnListDoubleClick)
		NOTIFY_HANDLER_EX(IDC_PREVIEW_LIST, LVN_KEYDOWN, OnListKeyDown)
		NOTIFY_HANDLER(IDC_PREVIEW_LIST, NM_CUSTOMDRAW, OnCustomDraw)

		MESSAGE_HANDLER_EX(MSG_EDIT, OnEdit)

		CHAIN_MSG_MAP(CDialogResize<CPreviewTagsDialog>)

	MY_END_MSG_MAP()

#pragma warning(pop)

	BEGIN_DLGRESIZE_MAP(CPreviewTagsDialog)
		DLGRESIZE_CONTROL(IDC_ALBUM_ART, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_OPTIONS_GROUP, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_CHK_REPLACE_ANV, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_CHK_SKIP_ARTWORK, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_CHK_PREV_DLG_SHOW_STATS, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_VIEW_GROUP, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_VIEW_NORMAL, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_VIEW_DIFFERENCE, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_VIEW_ORIGINAL, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_VIEW_DEBUG, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_BTN_TAG_MAPPINGS, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_BTN_BACK, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_BTN_WRITE_TAGS, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_PREVIEW_LIST, DLSZ_SIZE_X | DLSZ_SIZE_Y)
	END_DLGRESIZE_MAP()

	void DlgResize_UpdateLayout(int cxWidth, int cyHeight) {
		CDialogResize<CPreviewTagsDialog>::DlgResize_UpdateLayout(cxWidth, cyHeight);
	}

	CPreviewTagsDialog(HWND p_parent, TagWriter_ptr tag_writer)
		: m_tag_writer(tag_writer), m_results_list(NULL), conf(CONF), m_preview_bitmap(NULL) {

		multi_uartwork test = CONF_MULTI_ARTWORK;

		conf.SetName("PreviewDlg");
		g_discogs->preview_tags_dialog = this;

		m_rec_icon = LoadDpiBitmapResource(Icon::Record);

	}

	~CPreviewTagsDialog();

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnButtonWriteTags(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnButtonBack(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCheckReplaceANVs(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCheckPreviewShowStats(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCheckSkipArtwork(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCheckAttachMappingPanel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnButtonEditTagMappings(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnChangePreviewMode(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnListClick(LPNMHDR lParam);
	LRESULT OnListDoubleClick(LPNMHDR lParam);
	LRESULT OnListKeyDown(LPNMHDR lParam);
	LRESULT OnEdit(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnCustomDraw(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

	void replace_tag_result(size_t item, tag_result_ptr result);
	void insert_tag_results(bool computestat);
	void tag_mappings_updated();
	void cb_generate_tags();
	void enable(bool v) override { enable(v, true); };
	void enable(bool v, bool change_focus);
	bool check_write_tags_status();
	void destroy_all();
	void go_back();
};
