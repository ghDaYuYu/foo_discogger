#include "stdafx.h"

#include "tasks.h"
#include "update_tags_dialog.h"
#include "tag_mappings_dialog.h"


CUpdateTagsDialog::CUpdateTagsDialog(HWND p_parent, metadb_handle_list items) : items(items) {
	Create(p_parent);
}

CUpdateTagsDialog::~CUpdateTagsDialog() {
	g_discogs->update_tags_dialog = nullptr;
}

LRESULT CUpdateTagsDialog::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	CheckRadioButton(IDC_UPDATE_RADIO, IDC_WRITE_RADIO, IDC_UPDATE_RADIO);
	CheckDlgButton(IDC_MANUALLY_PROMPT, CONF.update_tags_manually_match);
	CheckDlgButton(IDC_PREVIEW_ALL_CHANGES, CONF.update_tags_preview_changes);
	CheckDlgButton(IDC_UPD_TAGS_REPLACE_ANV_CHECK, CONF.replace_ANVs);
	modeless_dialog_manager::g_add(m_hWnd);
	show();
	return TRUE;
}

LRESULT CUpdateTagsDialog::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	bool use_update_tags = !IsDlgButtonChecked(IDC_WRITE_RADIO) != 0;
	service_ptr_t<update_tags_task> task = new service_impl_t<update_tags_task>(items, use_update_tags);
	task->start();
	destroy();
	return TRUE;
}

LRESULT CUpdateTagsDialog::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	destroy();
	return TRUE;
}

LRESULT CUpdateTagsDialog::OnCheckManuallyPrompt(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	CONF.update_tags_manually_match = IsDlgButtonChecked(IDC_MANUALLY_PROMPT) != 0;
	conf_changed = true;
	return FALSE;
}

LRESULT CUpdateTagsDialog::OnCheckPreviewChanges(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	CONF.update_tags_preview_changes = IsDlgButtonChecked(IDC_PREVIEW_ALL_CHANGES) != 0;
	conf_changed = true;
	return FALSE;
}

LRESULT CUpdateTagsDialog::OnCheckReplaceANVs(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	CONF.replace_ANVs = IsDlgButtonChecked(IDC_UPD_TAGS_REPLACE_ANV_CHECK) != 0;
	conf_changed = true;
	return FALSE;
}

LRESULT CUpdateTagsDialog::OnEditTagMappings(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (!g_discogs->tag_mappings_dialog) {
		g_discogs->tag_mappings_dialog = new CNewTagMappingsDialog(core_api::get_main_window());
	}
	return FALSE;
}

void CUpdateTagsDialog::OnFinalMessage(HWND /*hWnd*/) {
	if (conf_changed) {
		CONF.save();
	}
	modeless_dialog_manager::g_remove(m_hWnd);
	delete this;
}
