#include "stdafx.h"
#include "resource.h"

#include "find_release_artist_dlg.h"

static const GUID guid_cfg_window_placement_find_release_artist_dlg = { 0xbe0df069, 0x3e52, 0x4570, { 0xba, 0xbc, 0x59, 0xbe, 0x75, 0xb8, 0xe0, 0x45 } };
static cfg_window_placement cfg_window_placement_find_release_artist_dlg(guid_cfg_window_placement_find_release_artist_dlg);

dialog_resize_helper::param CFindReleaseArtistDialog::m_resize_helper_table[] =
{
	{IDC_FIND_ARTIST_PROFILE_EDIT, dialog_resize_helper::XY_SIZE},
	{IDCANCEL,    dialog_resize_helper::XY_MOVE},
};

CFindReleaseArtistDialog::CFindReleaseArtistDialog(HWND p_parent) :
	m_resize_helper(m_resize_helper_table, tabsize(m_resize_helper_table), 235, 94, 0, 0)
{
    //
}

void CFindReleaseArtistDialog::UpdateProfile(pfc::string8 name, pfc::string8 profile)
{
	pfc::string8 caption;
	caption << "Artist Profile - " << name;
	uSetWindowText(m_hWnd, caption);
	uSetDlgItemText(m_hWnd, IDC_FIND_ARTIST_PROFILE_EDIT, profile);
}

LRESULT CFindReleaseArtistDialog::OnInitDialog(HWND hWnd, LPARAM lParam)
{
	SetIcon(g_discogs->icon);
	m_resize_helper.process_message(m_hWnd, WM_INITDIALOG, (WPARAM)hWnd, lParam);
	m_resize_helper.add_sizegrip();
	cfg_window_placement_find_release_artist_dlg.on_window_creation(m_hWnd, true);
	ShowWindow(SW_SHOW);
	return TRUE;
}

void CFindReleaseArtistDialog::OnDestroy()
{
	m_resize_helper.process_message(m_hWnd, WM_DESTROY, 0, 0);
	cfg_window_placement_find_release_artist_dlg.on_window_destruction(m_hWnd);
}

void CFindReleaseArtistDialog::OnClose()
{
	DestroyWindow();
}

void CFindReleaseArtistDialog::OnSize(UINT nType, CSize newSize)
{
	m_resize_helper.process_message(m_hWnd, WM_SIZE, nType, MAKELPARAM(newSize.cx, newSize.cy));
	Invalidate();
}

void CFindReleaseArtistDialog::OnGetMinMaxInfo(LPMINMAXINFO lpMinMaxInfo)
{
	m_resize_helper.process_message(m_hWnd, WM_GETMINMAXINFO, 0, (LPARAM)lpMinMaxInfo);
}

void CFindReleaseArtistDialog::OnCancel(UINT nCode, int nId, HWND hWnd)
{
	DestroyWindow();
}
