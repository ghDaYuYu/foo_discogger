#pragma once
#include "resource.h"

#include "helpers/DarkMode.h"

#include "guids_discogger.h"
#include "foo_discogs.h"
#include "my_tabentry.h"

#define NUM_TABS 7

#define CONF_FIND_RELEASE_TAB    0
#define CONF_MATCHING_TAB        1
#define CONF_TAGGING_TAB         2
#define CONF_CACHING_TAB         3
#define CONF_ART_TAB             4
#define CONF_UI_TAB              5
#define CONF_OATH_TAB            6

class NOVTABLE my_threaded_process : public threaded_process {
public:
	bool run_modal(service_ptr_t<threaded_process_callback> p_callback, unsigned p_flags, HWND p_parent, const char* p_title, t_size p_title_len) override;
	bool run_modeless(service_ptr_t<threaded_process_callback> p_callback, unsigned p_flags, HWND p_parent, const char* p_title, t_size p_title_len) override;

	//! Decrements reference count; deletes the object if reference count reaches zero. This is normally not called directly but managed by service_ptr_t<> template. \n
	//! Implemented by service_impl_* classes.
	//! @returns New reference count. For debug purposes only, in certain conditions return values may be unreliable.
	int service_release() throw() override;
	//! Increments reference count. This is normally not called directly but managed by service_ptr_t<> template. \n
	//! Implemented by service_impl_* classes.
	//! @returns New reference count. For debug purposes only, in certain conditions return values may be unreliable.
	int service_add_ref() throw() override;
	//! Queries whether the object supports specific interface and retrieves a pointer to that interface. This is normally not called directly but managed by service_query_t<> function template. \n
	//! Checks the parameter against GUIDs of interfaces supported by this object, if the GUID is one of supported interfaces, p_out is set to service_base pointer that can be static_cast<>'ed to queried interface and the method returns true; otherwise the method returns false. \n
	//! Implemented by service_impl_* classes. \n
	//! Note that service_query() implementation semantics (but not usage semantics) changed in SDK for foobar2000 1.4; they used to be auto-implemented by each service interface (via FB2K_MAKE_SERVICE_INTERFACE macro); they're now implemented in service_impl_* instead. See SDK readme for more details. \n
	bool service_query(service_ptr& p_out, const GUID& p_guid) override;
};

class CConfigurationDialog : public MyCDialogImpl<CConfigurationDialog>, public CMessageFilter, public preferences_page_instance,
	public fb2k::CDarkModeHooks
{
private:

	my_threaded_process m_tp;

	pfc::array_t<tab_entry> tab_table;
	foo_conf conf;
	foo_conf conf_edit;

	bool original_parsing_merge_titles = false;
	bool original_parsing = false;
	bool original_skip_video = false;

	bool setting_dlg = false;
	bool cancel_dlg = true;

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

	static INT_PTR WINAPI ui_dialog_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	INT_PTR WINAPI on_ui_dialog_message(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	static INT_PTR WINAPI oauth_dialog_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	INT_PTR WINAPI on_oauth_dialog_message(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void init_searching_dialog(HWND wnd);
	void init_matching_dialog(HWND wnd);
	void init_memory_cache_buttons(HWND wnd);
	void init_caching_dialog(HWND wnd);
	void init_tagging_dialog(HWND wnd);
	void init_art_dialog(HWND wnd);
	void init_ui_dialog(HWND wnd);
	void init_oauth_dialog(HWND wnd);
	void init_current_tab();

	bool build_current_cfg(bool reset);
	void pushcfg(bool reset);

	void save_searching_dialog(HWND wnd, bool dlgbind);
	void save_caching_dialog(HWND wnd, bool dlgbind);
	void save_matching_dialog(HWND wnd, bool dlgbind);
	void save_tagging_dialog(HWND wnd, bool dlgbind);
	void save_art_dialog(HWND wnd, bool dlgbind);
	void save_ui_dialog(HWND wnd, bool dlgbind);
	void save_oauth_dialog(HWND wnd, bool dlgbind);

	bool cfg_searching_has_changed();
	bool cfg_caching_has_changed();
	bool cfg_db_has_changed();
	bool cfg_matching_has_changed();
	bool cfg_tagging_has_changed();
	bool cfg_art_has_changed();
	bool cfg_ui_has_changed();
	bool cfg_oauth_has_changed();

	void on_load_search_formatting(HWND wnd);
	void on_delete_history(HWND wnd, size_t max, bool zap);
	void on_test_oauth(HWND wnd);
	void on_authorize_oauth(HWND wnd);
	void on_generate_oauth(HWND wnd);


	bool HasChanged();
	void OnChanged();
	void OnEditChange(UINT, int, CWindow) { OnChanged(); }

	const preferences_page_callback::ptr m_callback;

public:

	enum { IDD = IDD_DIALOG_CONF };

	t_uint32 get_state();
	void apply();
	void reset();

	virtual BOOL PreTranslateMessage(MSG* pMsg) override {
		return ::IsDialogMessage(m_hWnd, pMsg);
	}

#pragma warning( push )
#pragma warning( disable : 26454 )

	MY_BEGIN_MSG_MAP(CConfigurationDialog)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		NOTIFY_HANDLER(IDC_TAB_CFG, TCN_SELCHANGING, OnChangingTab)
		NOTIFY_HANDLER(IDC_TAB_CFG, TCN_SELCHANGE, OnChangeTab)
		COMMAND_ID_HANDLER(IDC_BTN_CONF_DEFAULTS, OnDefaults)
		MESSAGE_HANDLER(WM_CUSTOM_ANV_CHANGED, OnCustomAnvChanged)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
	MY_END_MSG_MAP()

#pragma warning( pop )

	// constructor

	CConfigurationDialog(preferences_page_callback::ptr callback) :	m_callback(callback) {

		m_hwndTokenEdit = nullptr;
		m_hwndSecretEdit = nullptr;
		m_hwndOAuthMsg = nullptr;

		if (g_discogs) {
			g_discogs->configuration_dialog = this;
			conf = CConf(CONF); conf.SetName("Cfg");
			conf_edit = CConf(CONF); conf_edit.SetName("CfgEdit");
		}
	}

	~CConfigurationDialog();

	void InitTabs();
	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnChangingTab(WORD /*wNotifyCode*/, LPNMHDR /*lParam*/, BOOL& /*bHandled*/); 
	LRESULT OnChangeTab(WORD /*wNotifyCode*/, LPNMHDR /*lParam*/, BOOL& /*bHandled*/); 
	LRESULT OnDefaults(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCustomAnvChanged(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);

	HWND m_hwndTokenEdit;
	HWND m_hwndSecretEdit;
	HWND m_hwndOAuthMsg;

	void show_tab(unsigned int itab);
	void show_oauth_msg(pfc::string8 msg, bool iserror);

	void enable(bool v) override {}
};

class preferences_page_myimpl : public preferences_page_impl<CConfigurationDialog> {

public:

	const char* get_name() { return "Discogger"; }
	GUID get_guid() { return guid_pref_page; }
	GUID get_parent_guid() { return guid_tagging; }
};
