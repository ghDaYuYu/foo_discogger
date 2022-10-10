#pragma once
#include "resource.h"
#include "helpers/DarkMode.h"

#include "foo_discogs.h"

class CFindReleaseArtistDialog :
	public CDialogImpl<CFindReleaseArtistDialog>, public CMessageFilter, public fb2k::CDarkModeHooks {

public:
	enum { IDD = IDD_DIALOG_FIND_ARTIST };
	
	BEGIN_MSG_MAP(CFindReleaseArtistDialog)
		MSG_WM_INITDIALOG(OnInitDialog)
		MSG_WM_DESTROY(OnDestroy)
		MSG_WM_CLOSE(OnClose)
		MSG_WM_SIZE(OnSize)
		MSG_WM_GETMINMAXINFO(OnGetMinMaxInfo)
		COMMAND_HANDLER_EX(IDCANCEL, BN_CLICKED, OnCancel)
	END_MSG_MAP()

	CFindReleaseArtistDialog(HWND p_parent, size_t SW_FLAG, bool customfont);

	~CFindReleaseArtistDialog() {

		g_discogs->find_release_artist_dialog = nullptr;

	}

	virtual BOOL PreTranslateMessage(MSG* pMsg) override {
		return ::IsDialogMessage(m_hWnd, pMsg);
	}

	void UpdateProfile(Artist_ptr& artist, pfc::string8 modprofile);

private:

	size_t m_sw_flag;
	size_t m_id = pfc_infinite;
	bool m_loaded = false;
	bool m_customfont = false;

	LRESULT OnInitDialog(HWND hWnd, LPARAM lParam);
	void OnDestroy();
	void OnClose();
	void OnSize(UINT nType, CSize newSize);
	void OnGetMinMaxInfo(LPMINMAXINFO lpMinMaxInfo);
	void OnCancel(UINT nCode, int nId, HWND hWnd);

	dialog_resize_helper m_resize_helper;
	static dialog_resize_helper::param m_resize_helper_table[];
};