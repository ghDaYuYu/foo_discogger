#include "stdafx.h"

#include "prompt_dialog.h"


LRESULT CPromptDialog::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	pfc::stringcvt::string_wide_from_ansi wtext(ask);
	SetDlgItemText(IDC_PROMPT_LABEL, (LPCTSTR)const_cast<wchar_t*>(wtext.get_ptr()));
	return TRUE;
}

LRESULT CPromptDialog::OnButtonOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	uGetDlgItemText(m_hWnd, IDC_EDIT_PROMPT, *result);
	EndDialog(0);
	return TRUE;
}

LRESULT CPromptDialog::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	EndDialog(0);
	return TRUE;
}
