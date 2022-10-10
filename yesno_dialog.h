#pragma once
#include "resource.h"
#include "helpers/DarkMode.h"

class CYesNoDialog : private dialog_helper::dialog_modal, public fb2k::CDarkModeHooks {
	BOOL on_message(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/) final {
		switch (uMsg) {
		case WM_INITDIALOG:
			AddDialog(get_wnd());
			AddControls(get_wnd());
			if (m_values.size() > 0) {
				SetWindowText(get_wnd(), pfc::stringcvt::string_wide_from_utf8(m_values[0]));
				uSetDlgItemText(get_wnd(), IDC_STATIC_QUESTION, m_values[1]);
			}
			return 0;
		case WM_COMMAND:
			if (HIWORD(wParam) == BN_CLICKED) {
				if (LOWORD(wParam) == IDOK) {
					end_dialog(1);
				}
				else if (LOWORD(wParam) == IDCANCEL) {
					end_dialog(0);
				}
			}
			else if (HIWORD(wParam) == EN_CHANGE) {
			}
			return 0;
		}
		return 0;
	}

public:

	int query(HWND parent, std::vector<pfc::string8> defValues) {
		m_values = defValues;
#pragma warning(suppress:4996)
		return run(idd, parent);  // NOLINT
	}

	UINT idd = IDD_DIALOG_YESNO;

	std::vector<pfc::string8> m_values;
};