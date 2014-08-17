#pragma once

#include "../SDK/foobar2000.h"
#include "resource.h"


class CUpdateArtDialog : public MyCDialogImpl<CUpdateArtDialog>, public CMessageFilter
{
private:
	metadb_handle_list items;

public:
	enum { IDD = IDD_UPDATE_ART_DIALOG };

	virtual BOOL PreTranslateMessage(MSG* pMsg) override {
		return ::IsDialogMessage(m_hWnd, pMsg);
	}

	MY_BEGIN_MSG_MAP(CUpdateArtDialog)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_ID_HANDLER(IDC_UPDATE_ART_ALBUM_CHECK, OnCheckUpdateArt)
		COMMAND_ID_HANDLER(IDC_UPDATE_ART_ARTIST_CHECK, OnCheckUpdateArtistArt)
	MY_END_MSG_MAP()

	CUpdateArtDialog(HWND p_parent, metadb_handle_list items);
	~CUpdateArtDialog();
	
	void CUpdateArtDialog::OnFinalMessage(HWND /*hWnd*/) override;

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCheckUpdateArt(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCheckUpdateArtistArt(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	void enable(bool v) override {}
};
