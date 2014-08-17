#include "stdafx.h"

#include "tasks.h"
#include "update_art_dialog.h"


CUpdateArtDialog::CUpdateArtDialog(HWND p_parent, metadb_handle_list items) : items(items) {
	Create(p_parent);
}

CUpdateArtDialog::~CUpdateArtDialog() {
	g_discogs->update_art_dialog = nullptr;
}

LRESULT CUpdateArtDialog::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	CheckDlgButton(IDC_UPDATE_ART_ALBUM_CHECK, IS_BIT_SET(CONF.update_art_flags, UPDATE_ART_ALBUM_BIT));
	CheckDlgButton(IDC_UPDATE_ART_ARTIST_CHECK, IS_BIT_SET(CONF.update_art_flags, UPDATE_ART_ARTIST_BIT));
	modeless_dialog_manager::g_add(m_hWnd);
	show();
	return TRUE;
}

LRESULT CUpdateArtDialog::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	bool save_album_art = IS_BIT_SET(CONF.update_art_flags, UPDATE_ART_ALBUM_BIT);
	bool save_artist_art = IS_BIT_SET(CONF.update_art_flags, UPDATE_ART_ARTIST_BIT);
	service_ptr_t<update_art_task> task = new service_impl_t<update_art_task>(items, save_album_art, save_artist_art);

	task->start();
	destroy();
	return TRUE;
}

LRESULT CUpdateArtDialog::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	destroy();
	return TRUE;
}

LRESULT CUpdateArtDialog::OnCheckUpdateArt(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	TOGGLE_BIT(CONF.update_art_flags, UPDATE_ART_ALBUM_BIT);
	return FALSE;
}

LRESULT CUpdateArtDialog::OnCheckUpdateArtistArt(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	TOGGLE_BIT(CONF.update_art_flags, UPDATE_ART_ARTIST_BIT);
	return FALSE;
}

void CUpdateArtDialog::OnFinalMessage(HWND /*hWnd*/) {
	modeless_dialog_manager::g_remove(m_hWnd); 
	delete this;
}
