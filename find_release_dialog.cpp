#include "stdafx.h"

#include "configuration_dialog.h"
#include "tasks.h"
#include "utils.h"
#include "db_utils.h"

#include "find_release_dialog.h"

static const GUID guid_cfg_window_placement_find_release_dlg = { 0x7342d2f3, 0x235d, 0x4bed, { 0x86, 0x7e, 0x82, 0x7f, 0xa8, 0x8e, 0xd9, 0x87 } };
static cfg_window_placement cfg_window_placement_find_release_dlg(guid_cfg_window_placement_find_release_dlg);

void load_global_icons() {
	auto kk = GetSystemMetrics(SM_CXSMICON);

	auto dpiX = QueryScreenDPIEx(core_api::get_main_window()).cx;
	int colw = MulDiv(17, dpiX, USER_DEFAULT_SCREEN_DPI);

	bool bdark = fb2k::isDarkMode();
	g_hIcon_quian = LoadDpiIconResource(!bdark ? Icon::Quian : Icon::Quian_Dark, bdark);
	g_hIcon_rec = LoadDpiBitmapResource(Icon::Record, bdark);

	LOGFONTW lf;
    CWindowDC dc(core_api::get_main_window());
    HTHEME theme = OpenThemeData(core_api::get_main_window(), VSCLASS_AEROWIZARD);
    GetThemeFont(theme, dc, AW_CONTENTAREA, 0, TMT_FONT, &lf);
    g_hFont = CreateFontIndirectW(&lf);
}

// constructor

CFindReleaseDialog::CFindReleaseDialog(HWND p_parent, metadb_handle_list items, foo_conf cfg)
    : items(items), conf(cfg), cewb_artist_search(), cewb_release_filter(), cewb_release_url(),
     m_dctree(this, &m_tracer), m_alist(this, &m_tracer) {

	load_global_icons();

	//HWND

	m_artist_list = nullptr;
	m_release_tree = nullptr;
	m_edit_artist = nullptr;
	m_edit_release = nullptr;
	m_edit_filter = nullptr;

	//..

	m_updating_releases = false;
	m_tickCount = 0;

	static_api_ptr_t<titleformat_compiler>()->compile_force(m_album_name_script, "[%album%]");
	static_api_ptr_t<titleformat_compiler>()->compile_force(m_artist_name_script, "[%artist%]");
	static_api_ptr_t<titleformat_compiler>()->compile_force(m_album_artist_script, "[%album artist%]");
	//static_api_ptr_t<titleformat_compiler>()->compile_force(release_name_script, "[%album artist%] - [%album%]");
}

CFindReleaseDialog::~CFindReleaseDialog() {

	DeleteObject(g_hIcon_quian);
	DeleteObject(g_hIcon_rec);
	DeleteObject(g_hFont);

    g_discogs->find_release_dialog = nullptr;
}

//override
void CFindReleaseDialog::show() {
	
	if (g_discogs->find_release_artist_dialog) {
		::ShowWindow(g_discogs->find_release_artist_dialog->m_hWnd, SW_SHOW);
	}

	MyCDialogImpl::show();
	
}

//override
void CFindReleaseDialog::hide() {

	if (g_discogs->find_release_artist_dialog) {
		::ShowWindow(g_discogs->find_release_artist_dialog->m_hWnd, SW_HIDE);
	}

	MyCDialogImpl::hide();

}

void CFindReleaseDialog::enable(bool is_enabled) {
	//::uEnableWindow(GetDlgItem(IDC_PROCESS_RELEASE_BUTTON), is_enabled);
	//::uEnableWindow(GetDlgItem(IDCANCEL), is_enabled);
	//::uEnableWindow(GetDlgItem(IDC_SEARCH_BUTTON), is_enabled);
}

// settings

void CFindReleaseDialog::pushcfg() {

	bool conf_changed = build_current_cfg();

	if (conf_changed) {
		CONF.save(CConf::cfgFilter::FIND, conf);
		CONF.load();
	}
}

inline bool CFindReleaseDialog::build_current_cfg() {

	bool bres = false;

	bool bcheck = IsDlgButtonChecked(IDC_CHK_ONLY_EXACT_MATCHES);

	if (CONF.display_exact_matches != bcheck) {
		conf.display_exact_matches = bcheck;
		bres |= true;
	}

	if ((CONF.find_release_filter_flag & FilterFlag::Versions) != (conf.find_release_filter_flag & FilterFlag::Versions)) {
		bres |= true;
	}
	if ((CONF.find_release_filter_flag & FilterFlag::RoleMain) != (conf.find_release_filter_flag & FilterFlag::RoleMain)) {
		bres |= true;
	}

	//bind artist profile panel
	size_t attach_flagged = (IsDlgButtonChecked(IDC_CHK_RELEASE_SHOW_PROFILE) == BST_CHECKED) ?
		FLG_PROFILE_DLG_SHOW : 0;

	size_t open_flagged = g_discogs->find_release_artist_dialog ? FLG_PROFILE_DLG_ATTACHED : 0;

	conf.find_release_dlg_flags &= ~(FLG_PROFILE_DLG_SHOW | FLG_PROFILE_DLG_ATTACHED);
	conf.find_release_dlg_flags |= (attach_flagged | open_flagged);

	if (CONF.find_release_dlg_flags != conf.find_release_dlg_flags) {		
		bres |= true;
	}

	return bres;
}

void CFindReleaseDialog::init_cfged_dialog_controls() {

	set_enter_key_override(conf.release_enter_key_override);

	size_t max_items = LOWORD(conf.history_enabled_max);

	if (conf.history_enabled()) {
		m_oplogger.init(conf.history_enabled(), max_items);
		//set_history_key_override();
	}

	//init artist profile panel
	if (conf.find_release_dlg_flags & flg_fr::FLG_PROFILE_DLG_SHOW) {

		uButton_SetCheck(m_hWnd, IDC_CHK_RELEASE_SHOW_PROFILE, true);

		if (conf.find_release_dlg_flags & flg_fr::FLG_PROFILE_DLG_ATTACHED) {

			if (!g_discogs->find_release_artist_dialog) {

				// new profile dlg
				g_discogs->find_release_artist_dialog = 
					fb2k::newDialog<CFindReleaseArtistDialog>(core_api::get_main_window(), SW_HIDE, HIWORD(conf.custom_font));
			}
		}
		else {
			/*if (!g_discogs->find_release_artist_dialog)
				g_discogs->find_release_artist_dialog->DestroyWindow();*/
		}
	}

	// tree stats

	if (conf.find_release_dlg_flags & flg_fr::FLG_SHOW_RELEASE_TREE_STATS) {
		//do not save
		print_root_stats(m_row_stats, false);
	}
	else {
		
		pfc::string8 artist_name = m_alist.Get_Artist() ? m_alist.Get_Artist()->name : "";
		pfc::string8 artist_id = m_alist.Get_Artist() ? m_alist.Get_Artist()->id : "";
		rppair row_stats{ std::pair("",""), std::pair(artist_name, artist_id) };
		//do not overwrite
		print_root_stats(row_stats, /*overwrite*/ !m_row_stats.second.first.get_length());
	}
}

// message map

LRESULT CFindReleaseDialog::OnButtonNext(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	pfc::string8 release_id;
	release_id = trim(uGetWindowText(m_edit_release).c_str());

	if (is_number(release_id.c_str()) || id_from_url(m_edit_release, release_id)) {
	
		// ON WRITE TAGS

		on_write_tags(release_id);
	}
	return TRUE;
}

//from Enter key hook

bool CFindReleaseDialog::ForwardVKReturn()
{

	HWND hwnd = GetFocus();
	
	if (hwnd == m_artist_list) {
		
		m_alist.Default_Artist_Action();
		
		//dont want return
		return false;
	}
	else if (hwnd == m_release_tree) {

		m_dctree.vkreturn_test_master_expand_release();

		//want return
		return true;
	}
	return false;
}

bool OAuthCheck(const foo_conf& conf) {
	if (!g_discogs->gave_oauth_warning && (!conf.oauth_token.get_length() || !conf.oauth_token_secret.get_length())) {
		g_discogs->gave_oauth_warning = true;
		if (!g_discogs->configuration_dialog) {
			static_api_ptr_t<ui_control>()->show_preferences(guid_pref_page);
		}

		if (CConfigurationDialog* dlg = g_discogs->configuration_dialog) {
			dlg->show_oauth_msg("Please configure OAuth.", true);
			::SetFocus(dlg->m_hWnd);
		}
		return false;
	}
	return true;
}

// on init dialog

#define OVRCEDIT
#define VKHOOK

LRESULT CFindReleaseDialog::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {

	SetIcon(g_discogs->icon);

	m_edit_artist = GetDlgItem(IDC_EDIT_SEARCH);
	m_edit_filter = GetDlgItem(IDC_EDIT_FILTER);
	m_edit_release = GetDlgItem(IDC_RELEASE_URL_TEXT);
	m_artist_list = GetDlgItem(IDC_ARTIST_LIST);
	m_release_tree = GetDlgItem(IDC_RELEASE_TREE);

#ifdef OVRCEDIT
	cewb_artist_search.SubclassWindow(m_edit_artist);
	cewb_release_filter.SubclassWindow(m_edit_filter);
	cewb_release_url.SubclassWindow(m_edit_release);

	set_history_key_override();

	cewb_release_filter.SetEnterEscHandlers();
	cewb_artist_search.SetEnterEscHandlers();
	cewb_release_url.SetEnterEscHandlers();

	artist_link.SubclassWindow(GetDlgItem(IDC_STATIC_FIND_REL_STATS));
	COLORREF lnktx = IsDark() ? GetSysColor(COLOR_MENUHILIGHT) : (COLORREF)(-1);
	artist_link.m_clrLink = lnktx;
	artist_link.m_clrVisited = lnktx;

	// init cfged

	init_cfged_dialog_controls();

#endif

	//init versions-roles checkboxes
	if (conf.find_release_filter_flag & FilterFlag::Versions) {
		uButton_SetCheck(m_hWnd, IDC_CHK_FIND_RELEASE_FILTER_VERS, true);
	}
	if (conf.find_release_filter_flag & FilterFlag::RoleMain) {
		uButton_SetCheck(m_hWnd, IDC_CHK_FIND_RELEASE_FILTER_ROLEMAIN, true);
	}

#ifdef DB_DC

	bool db_ready_to_search = DBFlags(conf.db_dc_flag).IsReady() && DBFlags(conf.db_dc_flag).Search();
	uButton_SetCheck(m_hWnd, IDC_CHK_FIND_REL_DB_SEARCH_LIKE, conf.db_dc_flag & DBFlags::DB_SEARCH_LIKE);
	uButton_SetCheck(m_hWnd, IDC_CHK_FIND_REL_DB_SEARCH_ANV, conf.db_dc_flag & DBFlags::DB_SEARCH_ANV);
	::ShowWindow(uGetDlgItem(IDC_CHK_FIND_REL_DB_SEARCH_LIKE), db_ready_to_search);
	::ShowWindow(uGetDlgItem(IDC_CHK_FIND_REL_DB_SEARCH_ANV), db_ready_to_search);
#else
	bool db_ready_to_search = false;
#endif // DB_DC

	pfc::string8 frm_album;
	pfc::string8 frm_artist;
	pfc::string8 frm_album_artist;

	metadb_handle_ptr item = items[0];
	item->format_title(nullptr, frm_album, m_album_name_script, nullptr);			//ALBUM
	item->format_title(nullptr, frm_artist, m_artist_name_script, nullptr);			//ARTIST
	item->format_title(nullptr, frm_album_artist, m_album_artist_script, nullptr);	//ALBUM ARTIST

	m_tracer.init_tracker_tags(items);

	const char* artist = frm_album_artist.get_length() ? frm_album_artist.get_ptr() :
		frm_artist.get_length() ? frm_artist.get_ptr() : "";

	//init artist search/release id textboxes

	uSetWindowText(m_edit_artist, artist ? artist : "");
	uSetWindowText(m_edit_release, m_tracer.has_release() ? std::to_string(m_tracer.release_id).c_str() : "");

	// dlg resize and dimensions

	DlgResize_Init(mygripp.enabled, true);
	cfg_window_placement_find_release_dlg.on_window_creation(m_hWnd, true);

	HWND wndReplace = ::GetDlgItem(m_hWnd, IDC_ARTIST_LIST);
	m_alist.CreateInDialog(*this, IDC_ARTIST_LIST, wndReplace);
	m_alist.InitializeHeaderCtrl(HDS_HIDDEN);	
	m_artist_list = m_alist.m_hWnd;
	m_alist.Inititalize();

	m_dctree.Inititalize(m_release_tree, m_edit_filter, m_edit_release, items, frm_album /*ALBUM*/);

#ifdef VKHOOK
	SetWindowSubclass(m_artist_list, EnterKeySubclassProc, 0, 0);
	SetWindowSubclass(m_release_tree, EnterKeySubclassProc, 0, 0);
#endif

	// dark mode
	AddDialog(m_hWnd);
	AddControls(m_hWnd);

	CustomFont(m_hWnd, HIWORD(conf.custom_font));

	bool bvisible_dlg;

	//	Auto-search artist when opening find release dialog
	//	Artist Id loads artist preview

	{
		bool brelease_ided = m_tracer.release_tag && m_tracer.release_id != pfc_infinite;
		bool bskip_ided = conf.skip_mng_flag & SkipMng::SKIP_RELEASE_DLG_IDED;

		bool cfg_always_load_artist_ided_preview = true;

		if (!(bskip_ided && brelease_ided)) {

			//route
			route_artist_search(artist, false, m_tracer.artist_tag);
			bvisible_dlg = true;
		}
		else 
		{
			if (cfg_always_load_artist_ided_preview && m_tracer.artist_tag) {

				//route
				route_artist_search(artist, false, true);
			}
			bvisible_dlg = false;
		}
	}

	//autofill filter textbox (album)

	block_filter_box_events(true);

	if (m_tracer.is_multi_artist() || !m_tracer.has_amr() || !conf.enable_autosearch) {
		uSetWindowText(m_edit_filter, frm_album);
	}

	block_filter_box_events(false);

	// dlg resize and dimensions

	DlgResize_Init(mygripp.enabled, true);
	cfg_window_placement_find_release_dlg.on_window_creation(m_hWnd, true);

	
	HWND hwnd = uGetDlgItem(IDC_STATIC_FIND_REL_SEARCH_STATS);
	::ShowWindow(hwnd, SW_HIDE);

	// SHOW RELEASES DLG OR SKIP TO MATCH TRACKS

	if (OAuthCheck(conf) || (db_ready_to_search)) {

		bool brelease_ided = m_tracer.release_tag && m_tracer.release_id != pfc_infinite;
		bool bskip_ided = conf.skip_mng_flag & SkipMng::SKIP_RELEASE_DLG_IDED;

		//autofill release id
		uSetWindowText(m_edit_release, brelease_ided ? std::to_string(m_tracer.release_id).c_str() : "");

		if (bskip_ided && brelease_ided) {

			//.. skip find release dialog
  
			on_write_tags(std::to_string(m_tracer.release_id).c_str());

			//..

			return FALSE;
		}
		else {

			show();

			//default buttons

			UINT default_button = 0;
			if (m_tracer.has_amr() || m_tracer.release_tag) {
				default_button = IDC_BTN_PROCESS_RELEASE;
			}
			else if (!artist) {
				default_button = IDC_EDIT_SEARCH;
			}
			else if (artist) {
				default_button = IDC_BTN_SEARCH;
			}

			if (default_button) {
				uSendMessage(m_hWnd, WM_NEXTDLGCTL, (WPARAM)(HWND)GetDlgItem(default_button), TRUE);
			}
			else {
				uSendMessage(m_hWnd, WM_NEXTDLGCTL, (WPARAM)(HWND)cewb_release_filter, TRUE);
			}
		}
	}
	else {

		//failed, show empty dialog
		show();
	}

	//prevent default control focus
	return FALSE;
}

LRESULT CFindReleaseDialog::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {

	//chained artist list and release tree

	bHandled = FALSE;
	return FALSE;
}

LRESULT CFindReleaseDialog::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	pushcfg();

	if (g_discogs->find_release_artist_dialog
		&& (conf.find_release_dlg_flags & (FLG_PROFILE_DLG_SHOW | FLG_PROFILE_DLG_ATTACHED)) == (FLG_PROFILE_DLG_SHOW | FLG_PROFILE_DLG_ATTACHED)) {
		g_discogs->find_release_artist_dialog->DestroyWindow();
	}

	cfg_window_placement_find_release_dlg.on_window_destruction(m_hWnd);
	KillTimer(KTypeFilterTimerID);

	return FALSE;
}

LRESULT CFindReleaseDialog::OnButtonSearch(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	pfc::string8 artist_meta = trim(uGetWindowText(m_edit_artist));

	bool force_get_by_id = id_from_url(m_edit_artist, artist_meta);

	if (artist_meta.get_length()) {

		route_artist_search(artist_meta, true, force_get_by_id);
	}

	return FALSE;
}

void CFindReleaseDialog::on_get_artist_done(updRelSrc updsrc, Artist_ptr& artist) {

	m_alist.on_get_artist_done(updsrc, artist);
	
	if (m_dctree.Get_Artist().get() && atoi(m_dctree.Get_Artist()->id) == atoi(artist->id)) {

		return;
	}

	cupdRelSrc cupdsrc = updsrc;
	if (cupdsrc == updRelSrc::ArtistProfile) {
		cupdsrc.extended = full_olcache() && conf.auto_rel_load_on_select;
	}
	if (cupdsrc.oninit || (!cupdsrc.oninit && cupdsrc == updRelSrc::ArtistProfile && cupdsrc.extended)) {
		m_dctree.on_get_artist_done(cupdsrc, artist);

		if (conf.auto_rel_load_on_open) {
			if (updsrc == updRelSrc::UndefFast && m_dctree.Get_Size() && m_tracer.has_master()) {

				m_dctree.OnInitExpand(mounted_param(m_tracer.master_i, ~0, true, false).lparam());	
			}
		}
	}

}

void CFindReleaseDialog::on_search_artist_done(const pfc::array_t<Artist_ptr>& p_artist_exact_matches, const pfc::array_t<Artist_ptr>& p_artist_other_matches, bool append) {

	bool exact_matches = IsDlgButtonChecked(IDC_CHK_ONLY_EXACT_MATCHES) == BST_CHECKED;

	HWND hwnd = uGetDlgItem(IDC_STATIC_FIND_REL_SEARCH_STATS);

	if (!(p_artist_exact_matches.get_count() || p_artist_other_matches.get_count())) {
		
		uSetWindowText(hwnd, "No matches found");
		::ShowWindow(hwnd, SW_SHOW);
	}
	else {
		uSetWindowText(hwnd, "");
	}

	if (!p_artist_exact_matches.get_count() && p_artist_other_matches.get_count()) {
		CheckDlgButton(IDC_CHK_ONLY_EXACT_MATCHES, false);
	}
	
	if (m_tracer.is_multi_artist()) {
		m_alist.set_artists(exact_matches, append, nullptr, p_artist_exact_matches, p_artist_other_matches);
	}
	else {
		m_alist.set_artists(exact_matches, append, nullptr, p_artist_exact_matches, p_artist_other_matches);
	}
	
	//spawns on list item selection (not by default)
	if (!append) {
		m_alist.fill_artist_list(exact_matches, m_tracer.has_amr(), updRelSrc::ArtistSearch);
	}
}

//returns pair <total root elements, expanded elements> (filtered result stats)
//todo: clean up return values

std::pair<rppair_t, rppair_t> CFindReleaseDialog::update_releases(const pfc::string8& filter, updRelSrc updsrc, bool init_expand) {

	std::pair<rppair_t, rppair_t> res;

	m_dctree.EnableDispInfo(false);

	bool brolemain_filter = conf.find_release_filter_flag & FilterFlag::RoleMain;

	// forward update releases

	res = m_dctree.update_releases(filter, updsrc, init_expand, brolemain_filter);

	m_dctree.EnableDispInfo(true);

	return res;
}


void CFindReleaseDialog::apply_filter(pfc::string8 strFilter, bool force_redraw, bool force_rebuild) {

	// forward apply_filter

	m_dctree.apply_filter(strFilter, force_redraw, force_rebuild);

}

void CFindReleaseDialog::OnTypeFilterTimer(WPARAM id) {

	if (id == KTypeFilterTimerID) {

		switch (++m_tickCount) {
		case 1:
			break;
		case 2:
			break;
		case 3:

			pfc::string8 strFilter = trim(uGetWindowText(m_edit_filter));

			if (!is_filter_autofill_enabled() && strFilter.get_length()) {

				rppair row;
				row.first.first = strFilter;
				add_history(oplog_type::filter, kHistoryFilterButton, row);
			}

			//..

			apply_filter(strFilter);

			//..

			KillTimer(KTypeFilterTimerID);
			break;
		}
	}
}

LRESULT CFindReleaseDialog::OnEditFilter(WORD wNotifyCode, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	if (is_filter_box_event_enabled() && wNotifyCode == EN_CHANGE) {

		//supress auto-fill filter box on user interaction

		block_autofill_release_filter(true);

		pfc::string8 buffer = trim(uGetWindowText(m_edit_filter));

		if (buffer.length()) {

			KFilterTimerOn(m_hWnd);
		}
		else {
			
			KillTimer(KTypeFilterTimerID);
			
			//..
			apply_filter(buffer);
			//..
		}
	}
	return false;
}


LRESULT CFindReleaseDialog::OnCheckboxBindProfilePanel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	bool state = IsDlgButtonChecked(IDC_CHK_RELEASE_SHOW_PROFILE);

	conf.find_release_dlg_flags =
		state ?
			conf.find_release_dlg_flags |  flg_fr::FLG_PROFILE_DLG_ATTACHED
		:	conf.find_release_dlg_flags & ~flg_fr::FLG_PROFILE_DLG_ATTACHED;

	if (state && !g_discogs->find_release_artist_dialog)
		m_alist.ShowArtistProfile();

	return FALSE;
}

LRESULT CFindReleaseDialog::OnCheckboxOnlyExactMatches(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	if (wID == IDC_CHK_ONLY_EXACT_MATCHES) {

		conf.display_exact_matches = IsDlgButtonChecked(IDC_CHK_ONLY_EXACT_MATCHES) == BST_CHECKED;

		m_alist.fill_artist_list(conf.display_exact_matches, m_tracer.has_amr(), updRelSrc::ArtistListCheckExact);

	}
	return FALSE;
}

LRESULT CFindReleaseDialog::OnCheckboxFindReleaseFilterFlags(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	bool force_refresh, force_rebuild;

	//get dlg filter
	pfc::string8 strFilter = trim(uGetWindowText(m_edit_filter)); 

	if (wID == IDC_CHK_FIND_RELEASE_FILTER_ROLEMAIN) {
		bool checked = IsDlgButtonChecked(IDC_CHK_FIND_RELEASE_FILTER_ROLEMAIN) == BST_CHECKED;
		if (checked)
			conf.find_release_filter_flag |= FilterFlag::RoleMain;
		else
			conf.find_release_filter_flag &= ~FilterFlag::RoleMain;

		force_refresh = force_rebuild = true;
	}
	else if (wID == IDC_CHK_FIND_RELEASE_FILTER_VERS) {
		bool checked = IsDlgButtonChecked(IDC_CHK_FIND_RELEASE_FILTER_VERS) == BST_CHECKED;
		if (checked)
			conf.find_release_filter_flag |= FilterFlag::Versions;
		else
			conf.find_release_filter_flag &= ~FilterFlag::Versions;

		force_refresh = true; force_rebuild = false;
		set_role_label(checked);

		//skip if filter is empty

		if (!strFilter.get_length()) {

			return FALSE;
		}
	}
	else {

		return FALSE;
	}
	
	KillTimer(KTypeFilterTimerID);

	//

	apply_filter(strFilter, force_refresh, force_rebuild);

	//

	return FALSE;
}


LRESULT CFindReleaseDialog::OnButtonConfigure(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	if (!g_discogs->configuration_dialog) {

		static_api_ptr_t<ui_control>()->show_preferences(guid_pref_page);
	}
	else {

		::SetFocus(g_discogs->configuration_dialog->m_hWnd);
	}
	return FALSE;
}

//todo: rev
void CFindReleaseDialog::on_write_tags(const pfc::string8& release_id) {

	//note: offline_artist_id is mem cache denormalization
	//todo: v2.23 - TODO: PREPARE NEW FOLDER FOR TRACKS MISSING A VALID DISCOGS_ARTIST_ID
	
	if (std::atoi(release_id) == 0) {
		//todo: bug trace	
		uMessageBox(m_hWnd, "Release Id non-valid Release Id, unable to continue.", "foo_discogger: Error", MB_APPLMODAL | MB_ICONASTERISK);
		return;

	}

	pfc::string8 offline_artist_id;
	bool bskip_idded_release_dlg = conf.skip_mng_flag & SkipMng::SKIP_RELEASE_DLG_IDED;

	if (bskip_idded_release_dlg && m_tracer.has_artist()) {

		//from oninit

		size_t artist_id = m_tracer.get_artist_id_store();
		offline_artist_id = std::to_string(artist_id).c_str();

	}
	else {
	
		//dlg button

		if (m_alist.Get_Artist()) {
			
			// list selection artist has preference, even if is NOT the artist_id meta
			offline_artist_id = m_alist.Get_Artist()->id;
		}
		else if (m_tracer.has_artist()) {

			// artist_id meta
			offline_artist_id = std::to_string(m_tracer.get_artist_id_store()).c_str();
		}
		else {

			//offline cache not available
			offline_artist_id = "";
		}
	}

	service_ptr_t<process_release_callback> task = new service_impl_t<process_release_callback>(this, release_id, offline_artist_id, "", items);
	task->start(m_hWnd);
}

// expand_master_release_process_done

void CFindReleaseDialog::on_expand_master_release_done(const MasterRelease_ptr& master_release, int list_index, threaded_process_status& p_status, abort_callback& p_abort) {

	// forward on expand master release done

	m_dctree.on_expand_master_release_done(master_release, list_index, p_status, p_abort);
}

void CFindReleaseDialog::on_expand_master_release_complete() {

	// * ACTIVE TASK NULL

	m_active_task = NULL;
}

void CFindReleaseDialog::call_expand_master_service(MasterRelease_ptr& release, int pos) {

	if (release->sub_releases.get_size()) {

		//already loaded
		return;
	}

	pfc::string8 offlineArtistId = "";

	if (m_alist.Get_Artist())
		offlineArtistId = m_alist.Get_Artist()->id;
	else
		if (m_alist.Get_Artists().get_count() > 0)
			offlineArtistId = m_alist.Get_Artists()[0]->id;

	service_ptr_t<expand_master_release_process_callback> task =
		new service_impl_t<expand_master_release_process_callback>(release, pos, offlineArtistId);

	m_active_task = &task;
	task->start(m_hWnd);
}

// convey artist-list and artist-profile actions

void CFindReleaseDialog::convey_artist_list_selection(cupdRelSrc cupdsrc) {

	//from Default action (ArtistList) or List selection (ArtistProfile)

	PFC_ASSERT(cupdsrc == updRelSrc::ArtistList || cupdsrc == updRelSrc::ArtistProfile);

	t_size pos = m_alist.GetSingleSel();

	if (pos == ~0 || (cupdsrc != updRelSrc::ArtistList && cupdsrc != updRelSrc::ArtistProfile)) {

		return;
	}


	Artist_ptr artist = m_alist.Get_Artists()[pos];

	bool need_releases = cupdsrc == updRelSrc::ArtistList  && !artist->loaded_releases;

	bool need_data = !(artist->loaded_preview || artist->loaded);

	need_data |= need_releases;

	if (full_olcache() && cupdsrc == updRelSrc::ArtistProfile && conf.auto_rel_load_on_select) {
		need_data = true;
	}

	if (need_data || cupdsrc == updRelSrc::UndefFast || cupdsrc == updRelSrc::ArtistList)
	{
		{
			std::lock_guard<std::mutex> guard(m_loading_selection_rw_mutex);

			m_loading_selection_id = atoi(artist.get()->id);
		}

		//			artist will land on 'on_get_artist_done' with updSrc::ArtistProfile
		//			and refresh ui from there

		//moved to onselectchanged
		//cupdRelSrc cupdsrc(updsrc);
		//cupdsrc.extended = full_olcache() && conf.auto_rel_load_on_select;

		service_ptr_t<get_artist_process_callback> task =
			new service_impl_t<get_artist_process_callback>(cupdsrc, artist->id.get_ptr());

		task->start(m_hWnd);
	}
	else
	{
		{
			std::lock_guard<std::mutex> guard(m_loading_selection_rw_mutex);

			m_loading_selection_id = pfc_infinite; //pending udpates validation on done 
		}

		//			update data available
		
		UpdateArtistProfile(artist);
	}
}

// route

void CFindReleaseDialog::route_artist_search(pfc::string8 artistname, bool dlgbutton, bool idded) {

	bool b_idded_from_init = !dlgbutton && idded;
	
	//todo:
	//SkipMng::SKIP_FIND_RELEASE_DLG_IDED //artist id
	//add SkipMng::UndefFast to load just profile if idded

	pfc::string8 artist_id;

	//todo: tidy up

	bool cfg_skip_get_artist_if_present = true;
	bool cfg_fast_load_artist_if_idded = true;

	updRelSrc updsrc = updRelSrc::Undef;

	bool by_any =	dlgbutton ||
					conf.enable_autosearch ||
					(m_tracer.has_artist() && cfg_fast_load_artist_if_idded);

	bool by_name = artistname.get_length();
	bool by_id = false;
	bool by_id_cfg_fast = false;

	if (dlgbutton) {

		by_id = false;
		by_id |= idded;

		if (by_id)
			artist_id = artistname;
	}
	else {

		by_id = idded;
		by_id &= m_tracer.has_artist();
		by_id_cfg_fast = m_tracer.has_artist() && cfg_skip_get_artist_if_present;
		updsrc = (m_tracer.has_artist() && cfg_fast_load_artist_if_idded) ? updRelSrc::ArtistProfile : updsrc;
		updsrc = by_id_cfg_fast ? updRelSrc::UndefFast : updRelSrc::Undef;

		if (by_id) {
		
			artist_id = std::to_string(m_tracer.get_artist_id()).c_str();
		}
	}

	if (by_any) {

		if (by_id) {

			cupdRelSrc cupdsrc(updsrc);
			if (updsrc == updRelSrc::UndefFast) {
				cupdsrc.extended |= conf.auto_rel_load_on_open;
			}
			else if (updsrc == updRelSrc::ArtistProfile) {
				cupdsrc.extended |= full_olcache() && conf.auto_rel_load_on_select;
			}
			
			//anything other than ArtistProfile will also load releases
			bool bmulti_artist = !(dlgbutton && idded) && m_tracer.is_multi_artist();
			if (bmulti_artist) {
				//commented to fix auto-load multiple artist releases
				//cupdsrc.extended = false;
				service_ptr_t<get_various_artists_process_callback> task =
					new service_impl_t<get_various_artists_process_callback>(cupdsrc, m_tracer.get_vartist_ids());

				task->start(m_hWnd);
			}
			else {
				cupdsrc.extended = false;
				service_ptr_t<get_artist_process_callback> task =
					new service_impl_t<get_artist_process_callback>(cupdsrc, artist_id);

				task->start(m_hWnd);
			}
		}
		else if (by_name) {

			DBFlags dbdc_flag = calc_dbdc_flag();

			service_ptr_t<search_artist_process_callback> task =
				new service_impl_t<search_artist_process_callback>(artistname.get_ptr(), dbdc_flag.GetFlag());

			task->start(m_hWnd);
		}
	}
}

// utils

bool CFindReleaseDialog::id_from_url(HWND hwndCtrl, pfc::string8& out) {

	pfc::string8 prefix;
	pfc::string8 suffix = "]";
	pfc::string8 url = "https://www.discogs.com/";

	char mode_param;
	pfc::string8 buffer = trim(out);

	if (hwndCtrl == m_edit_artist) {

		if (buffer.has_prefix("[a=")) {

			out = out.subString(3, buffer.get_length() - 4);
			return false;
		}

		prefix = "[a";
		url << "artist/";
		mode_param = buffer.has_prefix(url) ? 'w' : 'a';

	}
	else if (hwndCtrl == m_edit_release) {

		prefix = "[r";
		url << "release/";
		mode_param = buffer.has_prefix(url) ? 'w' : 'r';
	}
	else {

		return false;
	}

	if (mode_param == 'w' || (buffer.has_prefix(prefix) && buffer.has_suffix(suffix))) {

		if ((buffer = extract_max_number(buffer, mode_param)).get_length()) {

			out = buffer.c_str();
			return true;
		}
	}
	return false;
}

DBFlags CFindReleaseDialog::calc_dbdc_flag() {

	DBFlags db_dc_flags(conf.db_dc_flag);
	bool db_isready = DBFlags(conf.db_dc_flag).IsReady();

	bool find_like = IsDlgButtonChecked(IDC_CHK_FIND_REL_DB_SEARCH_LIKE);
	bool find_anv = IsDlgButtonChecked(IDC_CHK_FIND_REL_DB_SEARCH_ANV);
	db_dc_flags.SwitchFlag(DBFlags::DB_SEARCH_LIKE, db_isready && find_like);
	db_dc_flags.SwitchFlag(DBFlags::DB_SEARCH_ANV, db_isready && find_anv);

	return db_dc_flags;
}

//..

void CFindReleaseDialog::print_root_stats(rppair root_stats, bool save) {

	pfc::string8 stat_msg = "";


	if (conf.find_release_dlg_flags & flg_fr::FLG_SHOW_RELEASE_TREE_STATS) {

		//in-dlg
		int tot = atoi(root_stats.first.first) + atoi(root_stats.first.second);
		stat_msg << "Found: " << + tot;
		stat_msg << " - Masters: " << root_stats.first.first;
		//tree_stats << " - nm: " << root_stat.first.second;
	}

	if (root_stats.second.first.get_length()) {

		pfc::string8 url("https://www.discogs.com/artist/");
		url << root_stats.second.second;
		pfc::stringcvt::string_wide_from_utf8 wtext(url.get_ptr());
		artist_link.SetHyperLink((LPCTSTR)const_cast<wchar_t*>(wtext.get_ptr()));
		wtext.convert(root_stats.second.first);
		artist_link.SetLabel(wtext.get_ptr());
		CRect rc; ::GetWindowRect(artist_link, &rc);
		artist_link.RedrawWindow(0, 0, RDW_INVALIDATE);
	}
	else {
		stat_msg = "";
	}

	uSetDlgItemText(m_hWnd, IDC_STATIC_FIND_REL_STATS_EXT, stat_msg);

	if (save)
		m_row_stats = root_stats;
}

void replace_artist_refs(const pfc::string8 & profile, pfc::string8& outprofile, const pfc::array_t<Artist_ptr> artists) {

	outprofile = profile;

	if (!profile.get_length()) return;

	std::vector<std::pair<std::string, std::string>> vfound;

	std::regex regex_v("\\[a\\d+?\\]");

	std::string s(profile.c_str());
	std::sregex_iterator begin = std::sregex_iterator(s.begin(), s.end(), regex_v);
	std::sregex_iterator end = std::sregex_iterator();

	//extract unique artist refs
	for (std::sregex_iterator i = begin; i != end; i++) {

		std::string id(i->str());
		id = id.substr(2, id.size() - 3); //"[a1234]" -> 1234

		auto fit = std::find_if(vfound.begin(), vfound.end(), [&](const auto& item) {
			return !i->str().compare(item.first);
			});

		if (fit == vfound.end())
			vfound.emplace_back(id, "");
	}
	//look up artist names
	for (auto & walk_found : vfound) {
		for (auto w = artists.begin(); w < artists.end(); w++) {

			if (!walk_found.first.compare(w->get()->id.c_str())) {
				walk_found.second = w->get()->name;
				break;
			}
		}
	}
	//replace
	if (vfound.size()) {
		for (auto walk : vfound) {
			if (walk.second.size()) {
				pfc::string8 replace = PFC_string_formatter() << "[a" << walk.first.c_str() << "]";
				outprofile = outprofile.replace(replace, walk.second.c_str());
			}
		}	
	}	
}

//todo: rev #calls
void CFindReleaseDialog::UpdateArtistProfile(Artist_ptr artist) {

	const auto exactlist = m_alist.Get_Artists();

	if (g_discogs->find_release_artist_dialog) {
		pfc::string8 modprofile;
		replace_artist_refs(artist->profile, modprofile, exactlist);
		g_discogs->find_release_artist_dialog->UpdateProfile(artist, modprofile);
	}
}

bool CFindReleaseDialog::add_history(oplog_type optype, std::string cmd, pfc::string8 ff, pfc::string8 fs, pfc::string8 sf, pfc::string8 ss) {

	size_t inc_id = pfc_infinite;

	if (optype == oplog_type::artist) {
		rppair row = std::pair(std::pair("", ""), std::pair(sf, ss));

		if (m_oplogger.add_history_row(optype, row)) {
			sqldb db;
			inc_id = db.insert_history(optype, cmd, row);
		}
	}
	else if (optype == oplog_type::release) {
		rppair row = rppair(std::pair(ff, fs), std::pair(sf, ss));

		if (m_oplogger.add_history_row(optype, row)) {
			sqldb db;
			inc_id = db.insert_history(optype, cmd, row);
		}
	}
	else if (optype == oplog_type::filter) {
		rppair row = rppair(std::pair(ff, fs), std::pair(sf, ss));

		if (m_oplogger.add_history_row(optype, row)) {
			sqldb db;
			inc_id = db.insert_history(optype, cmd, row);
		}
	}
	return inc_id;
}

bool CFindReleaseDialog::add_history(oplog_type optype, std::string cmd, rppair row) {
	return add_history(optype, cmd, row.first.first, row.first.second, row.second.first, row.second.second);
}

bool CFindReleaseDialog::Set_Config_Flag(int ID, int flag, bool flagvalue) {
	
	FlgMng fv_stats;
	conf.GetFlagVar(ID, fv_stats);
	
	fv_stats.SetFlag(flag, flagvalue);

	return true;
}
