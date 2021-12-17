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
			m_getDlgCodeHandled(false),	m_dlgWantEnter(true),
			m_isDim(true), m_dimText(L" Dimmed text"),
			m_dimColor(RGB(150, 150, 150)),
			m_stdf_call_history(nullptr)
	{
	}

	CEditWithButtonsDim(const tstring& dimText) : CEditWithButtonsDim()
	{
		m_dimText = dimText;
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

	void SetHistoryHandlers(std::function<bool(HWND, wchar_t* wchart)> stdf_call_history) {
		m_stdf_call_history = stdf_call_history;
	}

	void SetEnterEscHandlers() {

		auto handler_onEnterKey = [this] {
			return false;
		};

		std::wstring clearValCopy(L"");
		std::wstring histValCopy(L"");

		auto handler_onEscKey = [this, clearValCopy] {
			this->SetWindowText(clearValCopy.c_str());
			return false;
		};

		auto handler_onHistKey = [this, histValCopy] {
			
			wchar_t wstr[MAX_PATH+1] = {};
			GetWindowText(wstr, MAX_PATH);			
			bool changed = this->m_stdf_call_history(this->m_hWnd, wstr);
			if (changed) {
				std::wstring updValCopy(wstr);
				SetWindowText(updValCopy.c_str());
			}			
			return false;
		};

		auto condition_clear = [clearValCopy](const wchar_t* txt) -> bool {
			return clearValCopy != txt;
		};
		auto condition_hist = [histValCopy](const wchar_t* txt) -> bool {
			return true;
		};

		this->onEnterKey = handler_onEnterKey;
		this->onEscKey = handler_onEscKey;

		AddButton(L"more", handler_onHistKey, condition_hist, L"\x2026");
		AddButton(L"clear", handler_onEscKey, condition_clear, L"\x00D7");
	}

	void FocusClear() {
		HWND wnd = this->GetNextDlgTabItem(FALSE);
		if (wnd)
			::SetFocus(wnd);
	}

	void SetEnterOverride(std::function<bool()>stdf_func) {
		//m_stdf_EnterOverride = stdf_func;
		this->onEnterKey = stdf_func;
	}

private:

	bool m_getDlgCodeHandled;
	bool m_dlgWantEnter;
	bool m_isDim;
	tstring m_dimText;
	DWORD m_dimColor;
	std::function<bool(HWND, wchar_t* wstr)> m_stdf_call_history;

};