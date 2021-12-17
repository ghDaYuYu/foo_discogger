#pragma once

#include "../SDK/foobar2000.h"
#include "resource.h"


class CUpdateTagsDialog : public MyCDialogImpl<CUpdateTagsDialog>, public CMessageFilter
{
private:
	metadb_handle_list items;

	bool conf_changed = false;
	foo_conf conf;

public:
	enum { IDD = IDD_DIALOG_UPDATE_TAGS };

	virtual BOOL PreTranslateMessage(MSG* pMsg) override {
		return ::IsDialogMessage(m_hWnd, pMsg);
	}

	MY_BEGIN_MSG_MAP(CUpdateTagsDialog)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_ID_HANDLER(IDC_MANUALLY_PROMPT, OnCheckManuallyPrompt)
		COMMAND_ID_HANDLER(IDC_PREVIEW_ALL_CHANGES, OnCheckPreviewChanges)
		COMMAND_ID_HANDLER(IDC_UPD_TAGS_REPLACE_ANV_CHECK, OnCheckReplaceANVs)
		COMMAND_ID_HANDLER(IDC_EDIT_TAG_MAPPINGS_BUTTON, OnEditTagMappings)
	MY_END_MSG_MAP()

	CUpdateTagsDialog(HWND p_parent, metadb_handle_list items);
	~CUpdateTagsDialog();

	void CUpdateTagsDialog::OnFinalMessage(HWND /*hWnd*/) override;

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCheckManuallyPrompt(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCheckPreviewChanges(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCheckReplaceANVs(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnEditTagMappings(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	void enable(bool v) override {}
};
