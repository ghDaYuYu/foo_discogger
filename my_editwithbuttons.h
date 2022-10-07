#pragma once

#include "resource.h"

#include "foo_discogs.h"
#include "tags.h"

#include "libPPUI/CEditWithButtons.h"

class CMyEditWithButtons : public CEditWithButtons {

public:

	typedef CEditWithButtons TParent;
	BEGIN_MSG_MAP_EX(CMyEditWithButtons)
	MSG_WM_SETFOCUS(OnSetFocus)
	MSG_WM_KILLFOCUS(OnKillFocus)
	CHAIN_MSG_MAP(TParent)
	END_MSG_MAP()

	CMyEditWithButtons() : CEditWithButtons(), m_stdf_call_history(nullptr)
	{
		m_bMsgHandled = true;
	}
	void OnSetFocus(HWND hWnd)
	{
		DefWindowProc();
	}

	void OnKillFocus(HWND hWnd)
	{
		DefWindowProc();
	}

	void SetHistoryHandler(std::function<bool(HWND, wchar_t* wchart)> stdf_call_history) {

		m_stdf_call_history = stdf_call_history;

		std::wstring histValCopy(L"");

		auto condition_hist = [histValCopy](const wchar_t* txt) -> bool {
			return CONF.history_enabled();
		};

		auto handler_onHistKey = [this, histValCopy] {

			wchar_t wstr[MAX_PATH + 1] = {};
			GetWindowText(wstr, MAX_PATH);
			bool updated = this->m_stdf_call_history(this->m_hWnd, wstr);
			if (updated) {
				std::wstring updValCopy(wstr);
				SetWindowText(updValCopy.c_str());
				::SetFocus(this->m_hWnd);
			}
			return updated;
		};

		AddButton(L"more", handler_onHistKey, condition_hist, L"\x2026");
	}

	void SetEnterEscHandlers() {

		auto handler_onEnterKey = [this] {
			return false;
		};

		std::wstring clearValCopy(L"");
		auto handler_onClearKey = [this, clearValCopy] {
			this->SetWindowText(clearValCopy.c_str());
			return false;
		};

		auto condition_clear = [clearValCopy](const wchar_t* txt) -> bool {
			return clearValCopy != txt;
		};

		this->onEnterKey = handler_onEnterKey;
		AddButton(L"clear", handler_onClearKey, condition_clear, L"\x00D7");
	}

	void FocusClear() {
		HWND wnd = this->GetNextDlgTabItem(m_hWnd, FALSE);
		if (wnd)
			::SetFocus(wnd);
	}

	void SetEnterOverride(std::function<bool()>stdf_func) {
		if (stdf_func) {

			this->onEnterKey = stdf_func;
		}
		else {
			this->onEnterKey = nullptr;
			this->onEscKey = nullptr;
		}
	}

private:

	std::function<bool(HWND, wchar_t* wstr)> m_stdf_call_history;

};