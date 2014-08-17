#pragma once

#include "error_manager.h"


template<class T>
class MyCDialogImpl : public CDialogImpl<T>, public ErrorManager
{
public:
	virtual void show() {
		if (placement_valid) {
			SetWindowPlacement(&my_placement);
		}
		ShowWindow(SW_SHOW);
	}

	virtual void hide() {
		GetWindowPlacement(&my_placement);
		placement_valid = true;
		ShowWindow(SW_HIDE);
	}

	virtual void destroy() {
		if (::IsWindow(m_hWnd)) {
			GetWindowPlacement(&my_placement);
			placement_valid = true;
			DestroyWindow();
		}
		else {
			delete this;
		}
	}

	static WINDOWPLACEMENT my_placement;
	static bool placement_valid;

	virtual void enable(bool v) = 0;
};
template <class T> WINDOWPLACEMENT MyCDialogImpl<T>::my_placement = { 0, 0, 0, { 0, 0 }, { 0, 0 }, { 0, 0, 0, 0 } };
template <class T> bool MyCDialogImpl<T>::placement_valid = false;


#define MY_BEGIN_MSG_MAP(theClass) \
public: \
	BOOL ProcessWindowMessage(_In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, \
	_In_ LPARAM lParam, _Inout_ LRESULT& lResult, _In_ DWORD dwMsgMapID = 0) \
	{ \
	try { \
		BOOL bHandled = TRUE; \
		(hWnd); \
		(uMsg); \
		(wParam); \
		(lParam); \
		(lResult); \
		(bHandled); \
		switch (dwMsgMapID) \
			{ \
			case 0:

#define MY_END_MSG_MAP() \
	break; \
		default: \
		ATLTRACE(ATL::atlTraceWindowing, 0, _T("Invalid message map ID (%i)\n"), dwMsgMapID); \
		ATLASSERT(FALSE); \
		break; \
		} \
		return FALSE; \
	} \
	catch (foo_discogs_exception &e) { \
		add_error(e, true); \
		display_errors(); \
		clear_errors(); \
		return FALSE; \
	} \
	}
