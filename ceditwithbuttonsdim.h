#pragma once

#include "../SDK/foobar2000.h"
#include "foo_discogs.h"
#include "resource.h"
#include "tags.h"

#include "libPPUI/CEditWithButtons.h"

#ifndef tstring
typedef std::basic_string< TCHAR > tstring;
#endif

class CEditWithButtonsDim : public CEditWithButtons {

public:

	typedef CEditWithButtons TParent;
	BEGIN_MSG_MAP_EX(CEditWithButtonsDim)
	MSG_WM_SETFOCUS(OnSetFocus)
	MSG_WM_KILLFOCUS(OnKillFocus)
	CHAIN_MSG_MAP(TParent)
	END_MSG_MAP()

	CEditWithButtonsDim() : CEditWithButtons(),
			m_isDim(true), m_dimText(L" dimmed text"),
			m_dimColor(RGB(150, 150, 150)) {
	}

	CEditWithButtonsDim(const tstring& dimText) :
			m_isDim(true), m_dimText(dimText),
			m_dimColor(RGB(150, 150, 150)) {
	}

	virtual ~CEditWithButtonsDim() {
	}

	void OnSetFocus(HWND hWnd)
	{
		DefWindowProc();
		if (GetWindowTextLength() == 0)
		{
			m_isDim = false;
			Invalidate();
		}
	}

	void OnKillFocus(HWND hWnd)
	{
		DefWindowProc();
		if (GetWindowTextLength() == 0)
		{
			m_isDim = true;
		}
	}

	void SetEnterEscHandlers() {

		auto handler_onEnterKey = [this] {
			return false;
		};

		std::wstring clearValCopy(L"");
		auto handler_onEscKey = [this, clearValCopy] {
			this->SetWindowText(clearValCopy.c_str());
			return false;
		};

		auto condition = [clearValCopy](const wchar_t* txt) -> bool {
			return clearValCopy != txt;
		};

		this->onEnterKey = handler_onEnterKey;
		this->onEscKey = handler_onEscKey;

		AddButton(L"clear", handler_onEscKey, condition, L"\x00D7");
	}

	void FocusClear() {
		HWND wnd = this->GetNextDlgTabItem(false);
		::SetFocus(wnd);
	}

private:
	bool m_isDim;
	tstring m_dimText;
	DWORD m_dimColor;
};