#pragma once

#include "../SDK/foobar2000.h"
#include "resource.h"


class CPromptDialog : public MyCDialogImpl<CPromptDialog>
{
private:
	const pfc::string8 &ask;
	pfc::string8 *result;

public:
	enum { IDD = IDD_DIALOG_PROMPT };

	MY_BEGIN_MSG_MAP(CPromptDialog)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	MY_END_MSG_MAP()

	CPromptDialog(const pfc::string8 &ask, pfc::string8 *result) : ask(ask), result(result) {}
	~CPromptDialog() {};

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	void enable(bool v) override {}
};
