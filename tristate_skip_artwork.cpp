#include "stdafx.h"
#include "tristate_skip_artwork.h"

void CTristate::Init(UINT state, BOOL enabled) {

  HWND wnd = uGetDlgItem(parent->m_hWnd, IDC);

  //BST_CHECKED : BST_UNCHECKED
  ::CheckDlgButton(parent->m_hWnd, IDC, state);
  ::EnableWindow(wnd, enabled);
}

bool CTristate::IsTri() {

	HWND wnd = uGetDlgItem(parent->m_hWnd, IDC);
	const DWORD btnType = CWindow(wnd).GetStyle() & BS_TYPEMASK;
	return (btnType == BS_AUTO3STATE);
}

UINT  CTristate::GetState() {

	//BST_UNCHECKED      0x0000
	//BST_CHECKED        0x0001
	//BST_INDETERMINATE  0x0002

	auto checkstate = SendDlgItemMessage(parent->m_hWnd, IDC, BM_GETSTATE, (WPARAM)0, (LPARAM)0);
	return checkstate;
}

void CTristate::SetTristate(UINT prestate, UINT poststate) {

	::CheckDlgButton(parent->m_hWnd, IDC_CHK_SKIP_ARTWORK, BST_INDETERMINATE);

	// SET TRI-STATE STYLE 

	SendMessage(getWnd(), BM_SETSTYLE
		, (WPARAM)0
		, (LPARAM)0);//true redraws button

	SendMessage(getWnd() , BM_SETSTYLE
		, (WPARAM)BS_AUTO3STATE | BS_CHECKBOX | BS_FLAT
		, (LPARAM)0);//true redraws button

	::CheckDlgButton(parent->m_hWnd, IDC, BST_INDETERMINATE);

}

void CTristate::RemoveTristate() {
	::CheckDlgButton(parent->m_hWnd, IDC, BST_UNCHECKED);

	// REMOVE TRI-STATE STYLE 

	SendMessage(getWnd()
		, BM_SETSTYLE
		, (WPARAM)0/*BS_CHECKBOX| BS_FLAT BS_3STATE | BS_CHECKBOX*/
		, (LPARAM)0);//true redraws button
}

void CTristate::SetBistate() {

	// REMOVE TRI-STATE STYLE

	SendMessage(getWnd(), BM_SETSTYLE
		, (WPARAM)0/*BS_CHECKBOX| BS_FLAT BS_3STATE | BS_CHECKBOX*/
		, (LPARAM)0);//true redraws button

	// SET BISTATE

	SendMessage(getWnd(), BM_SETSTYLE
		, (WPARAM)BS_CHECKBOX | BS_AUTOCHECKBOX
		, (LPARAM)0);//true redraws button

}

void CTristate::SetBistate(UINT state) {
	// SET BI-STATE

	SendMessage(getWnd(), BM_SETSTYLE
		, (WPARAM)BS_CHECKBOX | BS_AUTOCHECKBOX
		, (LPARAM)0);//true redraws button

	//BST_CHECKED : BST_UNCHECKED);
	::CheckDlgButton(parent->m_hWnd, IDC, state);
}