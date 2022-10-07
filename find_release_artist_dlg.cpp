#include "stdafx.h"
#include "resource.h"
#include "conf.h"
#include "find_release_artist_dlg.h"

static const GUID guid_cfg_window_placement_find_release_artist_dlg = { 0xbe0df069, 0x3e52, 0x4570, { 0xba, 0xbc, 0x59, 0xbe, 0x75, 0xb8, 0xe0, 0x45 } };
static cfg_window_placement cfg_window_placement_find_release_artist_dlg(guid_cfg_window_placement_find_release_artist_dlg);

dialog_resize_helper::param CFindReleaseArtistDialog::m_resize_helper_table[] =
{
	{IDC_EDIT_FIND_ARTIST_PROFILE, dialog_resize_helper::XY_SIZE},
	{IDC_STATIC_PROFILE_REALNAME, dialog_resize_helper::X_SIZE},
	{IDC_STATIC_PROFILE_REALNAME, dialog_resize_helper::Y_MOVE},
	{IDCANCEL,    dialog_resize_helper::XY_MOVE},
};

CFindReleaseArtistDialog::CFindReleaseArtistDialog(HWND p_parent, size_t SW_FLAG, bool custfont) : m_sw_flag(SW_FLAG), m_customfont(custfont),
	m_resize_helper(m_resize_helper_table, tabsize(m_resize_helper_table), 235, 94, 0, 0)
{
    //..
}

void CFindReleaseArtistDialog::UpdateProfile(Artist_ptr& artist, pfc::string8 modprofile)
{
	if (artist) {

		size_t id = atoi(artist->id);
		bool loaded = artist->loaded;

		if (id != m_id || loaded != m_loaded) {

			pfc::string8 name = artist->name;
			pfc::string8 profile= modprofile;
			pfc::string8 realname = artist->realname;
			pfc::string8 buffer;

			buffer << "Artist Profile - " << name;
			uSetWindowText(m_hWnd, buffer);
			uSetDlgItemText(m_hWnd, IDC_EDIT_FIND_ARTIST_PROFILE, profile);

			if (loaded) {
				if (realname.get_length()) {
					buffer = "Real name: ";
					buffer << realname;
				}
				else {
					buffer = "";
				}
			}
			else {
				buffer = "Select artist to load profile.";
			}

			uSetDlgItemText(m_hWnd, IDC_STATIC_PROFILE_REALNAME, buffer);

			m_id = id;
			m_loaded = loaded;
		}
		
	}
	else {

		uSetWindowText(m_hWnd, "Artist Profile");
		uSetDlgItemText(m_hWnd, IDC_EDIT_FIND_ARTIST_PROFILE, "");
		uSetDlgItemText(m_hWnd, IDC_STATIC_PROFILE_REALNAME, "");

		m_id = pfc_infinite;
	}
}

LRESULT CFindReleaseArtistDialog::OnInitDialog(HWND hWnd, LPARAM lParam)
{
	SetIcon(g_discogs->icon);
	m_resize_helper.process_message(m_hWnd, WM_INITDIALOG, (WPARAM)hWnd, lParam);
	m_resize_helper.add_sizegrip();
	cfg_window_placement_find_release_artist_dlg.on_window_creation(m_hWnd, true);

	//darkmode
	AddDialog(m_hWnd);
	AddControls(m_hWnd);

	CustomFont(m_hWnd, m_customfont, true);

	ShowWindow(m_sw_flag);
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
