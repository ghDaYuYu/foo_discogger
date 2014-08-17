#pragma once

#include <uxtheme.h>
#include "../SDK/foobar2000.h"
#include "foo_discogs.h"
#include "tag_mappings_dialog.h"


#define NUM_TABS		6

#define CONF_FIND_RELEASE_TAB	0
#define CONF_MATCHING_TAB		1
#define CONF_TAGGING_TAB		2
#define CONF_CACHING_TAB		3
#define CONF_ART_TAB			4
#define CONF_OATH_TAB			5


class tab_entry
{
public:
	LPSTR m_pszName;
	DLGPROC m_lpDialogFunc;
	WORD m_nID;

	tab_entry(LPSTR pszName, DLGPROC lpDialogFunc, WORD nID) {
		m_pszName = pszName;
		m_lpDialogFunc = lpDialogFunc;
		m_nID = nID;
	}
	tab_entry() {
		m_pszName = "";
		m_lpDialogFunc = nullptr;
		m_nID = 0;
	}

	HWND CreateTabDialog(HWND hWndParent, LPARAM lInitParam = 0) {
		PFC_ASSERT(m_lpDialogFunc != nullptr);
		HWND dlg = uCreateDialog(m_nID, hWndParent, m_lpDialogFunc, lInitParam);
		if (dlg) {
			EnableThemeDialogTexture(dlg, ETDT_ENABLE | ETDT_USETABTEXTURE);
		}
		return dlg;
	}
};


class CConfigurationDialog : public MyCDialogImpl<CConfigurationDialog>, public CMessageFilter
{
private:
	pfc::array_t<tab_entry> tab_table;
	foo_discogs_conf conf;

	HWND album_art_dir_edit;
	HWND album_art_file_edit;

	bool original_parsing = false;

	CHyperLink help_link;

	static INT_PTR WINAPI searching_dialog_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	INT_PTR WINAPI on_searching_dialog_message(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	static INT_PTR WINAPI caching_dialog_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	INT_PTR WINAPI on_caching_dialog_message(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	static INT_PTR WINAPI matching_dialog_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	INT_PTR WINAPI on_matching_dialog_message(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	static INT_PTR WINAPI tagging_dialog_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	INT_PTR WINAPI on_tagging_dialog_message(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	static INT_PTR WINAPI art_dialog_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	INT_PTR WINAPI on_art_dialog_message(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	static INT_PTR WINAPI oauth_dialog_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	INT_PTR WINAPI on_oauth_dialog_message(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void init_searching_dialog(HWND wnd);
	void init_matching_dialog(HWND wnd);
	void init_caching_dialog(HWND wnd);
	void init_tagging_dialog(HWND wnd);
	void init_art_dialog(HWND wnd);
	void init_oauth_dialog(HWND wnd);

	void save_searching_dialog(HWND wnd);
	void save_caching_dialog(HWND wnd);
	void save_matching_dialog(HWND wnd); 
	void save_tagging_dialog(HWND wnd);
	void save_art_dialog(HWND wnd);
	void save_oauth_dialog(HWND wnd);

	void on_test_oauth(HWND wnd);
	void on_authorize_oauth(HWND wnd);
	void on_generate_oauth(HWND wnd);

public:
	enum { IDD = IDD_DIALOG_CONF };

	virtual BOOL PreTranslateMessage(MSG* pMsg) override {
		return ::IsDialogMessage(m_hWnd, pMsg);
	}

	MY_BEGIN_MSG_MAP(CConfigurationDialog)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		NOTIFY_HANDLER(IDC_TAB, TCN_SELCHANGE, OnChangeTab)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_ID_HANDLER(IDC_CONF_DEFAULTS_BUTTON, OnDefaults)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
	MY_END_MSG_MAP()

	CConfigurationDialog(HWND p_parent);
	~CConfigurationDialog();
	void CConfigurationDialog::OnFinalMessage(HWND /*hWnd*/) override;

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnChangeTab(WORD /*wNotifyCode*/, LPNMHDR /*lParam*/, BOOL& /*bHandled*/); 
	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnDefaults(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);

	HWND token_edit;
	HWND secret_edit;

	void show_tab(unsigned int num);

	void enable(bool v) override {}
};
