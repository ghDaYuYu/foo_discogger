#pragma once
#include "resource.h"
#include "foo_discogs.h"

class CFindReleaseArtistDialog :
	public CDialogImpl<CFindReleaseArtistDialog>, public CMessageFilter
{
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

	CFindReleaseArtistDialog(HWND p_parent);

	~CFindReleaseArtistDialog() {
		g_discogs->find_release_artist_dialog = nullptr;
	}

	virtual BOOL PreTranslateMessage(MSG* pMsg) override {
		return ::IsDialogMessage(m_hWnd, pMsg);
	}

	void UpdateProfile(pfc::string8 name, pfc::string8 profile);

private:

	LRESULT OnInitDialog(HWND hWnd, LPARAM lParam);
	void OnDestroy();
	void OnClose();
	void OnSize(UINT nType, CSize newSize);
	void OnGetMinMaxInfo(LPMINMAXINFO lpMinMaxInfo);
	void OnCancel(UINT nCode, int nId, HWND hWnd);

	dialog_resize_helper m_resize_helper;
	static dialog_resize_helper::param m_resize_helper_table[];
};