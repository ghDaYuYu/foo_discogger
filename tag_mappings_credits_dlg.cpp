#include "stdafx.h"

#include "resource.h"
#include "strsafe.h"

#include "tags.h"
#include "tasks.h"
#include "db_fetcher_component.h"

#include "preview_dialog.h"
#include "track_matching_dialog.h"
#include "tag_mappings_dialog.h"
#include "tag_mappings_credits_dlg.h"

static const GUID guid_cfg_window_placement_tag_mappings_credits_dlg = { 0xc961852b, 0xd3c7, 0x49e4, { 0xbc, 0x96, 0x5f, 0x51, 0xfb, 0xa8, 0x72, 0x4b } };
static cfg_window_placement cfg_window_placement_tag_mappings_credits_dlg(guid_cfg_window_placement_tag_mappings_credits_dlg);

int CListControlCredit_Selection::OnCreate(LPCREATESTRUCT lpCreateStruct) {
	m_credits = m_dlg_parent->GetCredits();

	InitHeader(); // set up header control with columns
	SetWindowText(L"Selection"); // screen reader will see this
	return 0;
}

int CListControlCredits::OnCreate(LPCREATESTRUCT lpCreateStruct) {
	m_credits = m_dlg_parent->GetCredits();
	m_data = makeListData(m_credits);
	InitHeader();
	SetWindowText(L"Credits");
	return 0;
}

void RenderHLBackground(CDCHandle p_dc, const CRect& p_itemRect, size_t item, uint32_t bkColor, pfc::string8 str, pfc::string8 hlstr, bool freeze) {

	const Gdiplus::Color gdiHLColor = Gdiplus::Color(255, 180, 180, 180);

	const CRect* rc = &p_itemRect;
	Gdiplus::Graphics gr(p_dc);
	Gdiplus::Pen pen(gdiHLColor, static_cast<Gdiplus::REAL>(rc->bottom - rc->top));
	gr.DrawLine(&pen, rc->left, rc->top + ((rc->bottom - rc->top) / 2), rc->right, rc->top + ((rc->bottom - rc->top) / 2));
	DeleteObject(&pen);
	DeleteObject(&gr);

}

void CListControlCredits::RenderItemBackground(CDCHandle p_dc, const CRect& p_itemRect, size_t item, uint32_t bkColor) {

	TParent::RenderItemBackground(p_dc, p_itemRect, item, bkColor);

	if (!m_hl_string.get_length()) return;

	pfc::string8 strItem = m_data.at(item).m_credit;
	bool do_hlight = false;

	if (m_hl_string.get_length()) {		
		do_hlight = pfc::string_find_first(pfc::stringToLower(strItem), m_hl_string, 0) != pfc_infinite;
	}

	if (do_hlight)
		RenderHLBackground(p_dc, p_itemRect, item, bkColor, strItem, m_hl_string, false);
}

void CTagCreditDialog::pushcfg(bool force) {
	bool conf_changed = build_current_cfg();
	if (conf_changed || force) {
		CONF.save(CConf::cfgFilter::CAT_CREDIT, conf);
		CONF.load();
	}
}

inline void CTagCreditDialog::load_size() {
	//
}

inline bool CTagCreditDialog::build_current_cfg() {
	//
	return false;
}

void CTagCreditDialog::updateTagCreditUI(HWND list, bool remake_list) {

	m_rebuilding_tag_name = true;

	bool add_group = false;
	UINT idc_grp;
	if (m_ctag.isGXC())
		idc_grp = IDC_TAG_CREDITS_GRP_GXC;
	else if (m_ctag.isGXP())
		idc_grp = IDC_TAG_CREDITS_GRP_GXP;
	else {
		idc_grp = IDC_TAG_CREDITS_GRP_GXP;
		add_group = true;
	}

	uButton_SetCheck(m_hWnd, idc_grp, TRUE);
	BOOL bdummy = false;
	OnBtnCheckGroup(0, idc_grp, m_hWnd, bdummy);

	if (m_ctag.isAll()) {
		uButton_SetCheck(m_hWnd, IDC_TAG_CREDITS_RADIO_ALL, TRUE);
		uButton_SetCheck(m_hWnd, IDC_TAG_CREDITS_RADIO_RELEASE, FALSE);
		uButton_SetCheck(m_hWnd, IDC_TAG_CREDITS_RADIO_TRACKS, FALSE);
	}
	else if (m_ctag.isRelease()) {		
		uButton_SetCheck(m_hWnd, IDC_TAG_CREDITS_RADIO_ALL, FALSE);
		uButton_SetCheck(m_hWnd, IDC_TAG_CREDITS_RADIO_RELEASE, TRUE);
		uButton_SetCheck(m_hWnd, IDC_TAG_CREDITS_RADIO_TRACKS, FALSE);
	}
	else if (m_ctag.isTracks()) {		
		uButton_SetCheck(m_hWnd, IDC_TAG_CREDITS_RADIO_ALL, FALSE);
		uButton_SetCheck(m_hWnd, IDC_TAG_CREDITS_RADIO_RELEASE, FALSE);
		uButton_SetCheck(m_hWnd, IDC_TAG_CREDITS_RADIO_TRACKS, TRUE);
	}
	else {
		uButton_SetCheck(m_hWnd, IDC_TAG_CREDITS_RADIO_ALL, TRUE);
		uButton_SetCheck(m_hWnd, IDC_TAG_CREDITS_RADIO_RELEASE, FALSE);
		uButton_SetCheck(m_hWnd, IDC_TAG_CREDITS_RADIO_TRACKS, FALSE);
	}

	if (m_ctag.get_cat().get_length()) {
		uSetDlgItemText(m_hWnd, IDC_TAG_CREDITS_STATIC_PREVIEW_TF, m_ctag.get_tagname());
	}

	//todo: move to notifier/remove notifier

	if (remake_list) {
		bit_array_bittable credit_list_mask = m_credit_sel_list.InitCatInfo(&m_ctag, vcats, vsubcats);
		m_credit_sel_list.ReloadData();
		if (list == m_credit_sel_list)
			m_credit_list.SetChecked(credit_list_mask);
	}

	m_rebuilding_tag_name = false;
}

void loadComboCreditUserDefinitions(HWND hparent, UINT id, vppair v, const char* selectedItem) {
	HWND hwnd_cmb = GetDlgItem(hparent, id);
	((void)SNDMSG(hwnd_cmb, WM_SETREDRAW, (WPARAM)(BOOL)(false), 0L));
	
	SendMessage(hwnd_cmb, CB_RESETCONTENT, 0, 0);

	size_t pos = 0;
	for (auto walk : v) {
		
		pfc::string8 aname(walk.first.second);	
		int rowId = uSendDlgItemMessageText(hparent, id, CB_INSERTSTRING, pos, aname);
		uSendDlgItemMessage(hparent, id, CB_SETITEMDATA, rowId, atoi(walk.first.first));
		if (selectedItem && std::string(selectedItem).compare(std::string(walk.first.second)) == 0) {
			rowId = uSendDlgItemMessageText(hparent, id, CB_SETCURSEL, rowId, 0);
		}
		++pos;
	}

	((void)SNDMSG(hwnd_cmb, WM_SETREDRAW, (WPARAM)(BOOL)(true), 0L));
	RECT rc;
	GetClientRect(hwnd_cmb, &rc);
	//::InvalidateRect(hparent, &rc, true);
	RedrawWindow(hwnd_cmb, &rc, NULL, RDW_ERASENOW /*| RDW_VALIDATE*/ | RDW_INVALIDATE | RDW_ALLCHILDREN);
}

void loadComboHeadings(HWND hparent, UINT id, vppair v, const char* selectedItem, bool is_heading) {

	HWND ctrl = uGetDlgItem(hparent, id);

	SendMessage(hparent, WM_SETREDRAW, FALSE, 0);

	int itemId = uSendDlgItemMessageText(hparent, id, CB_ADDSTRING, (WPARAM)0, "All");
	uSendDlgItemMessage(hparent, id, CB_SETITEMDATA, itemId, (LPARAM)0);
	for (auto walkcredit : v) {
		pfc::string8 cbrow;
		cbrow << (walkcredit.first.first.get_length() < 2? " ": "") << walkcredit.first.first << (walkcredit.first.first.get_length() < 2 ? "  " : " ") << (is_heading ? walkcredit.first.second : walkcredit.second.first);
		int itemId = uSendDlgItemMessageText(hparent, id, CB_ADDSTRING, (WPARAM)0, cbrow.get_ptr());
		LPARAM lparam = (LPARAM)atoi(walkcredit.first.first);
		uSendDlgItemMessage(hparent, id, CB_SETITEMDATA, itemId, lparam);
	}
	uSendDlgItemMessage(hparent, id, CB_SETCURSEL, 0, 0);

	SendMessage(hparent, WM_SETREDRAW, TRUE, 0);
}

LRESULT CTagCreditDialog::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	
	SetIcon(g_discogs->icon);
	conf = CONF;

	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(icex);
	icex.dwICC = ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx(&icex);

	db_fetcher_component db;

	// build user credit definitions

	vdefs = db.load_db_def_credits();

	// fill user credit definitions combo

	UINT ctrl_id = IDC_TAG_CREDITS_CMB_DEFS;

	pfc::string8 tag_name;
	pfc::string8 tag_name_tf;

	if (vdefs.size()) {
		tag_name << vdefs.at(0).first.second;
		tag_name_tf << vdefs.at(0).second.first;
		loadComboCreditUserDefinitions(m_hWnd, ctrl_id, vdefs, tag_name);
	}
	else {
		//empty definitions, use defaults
		tag_name << m_ctag.get_default_name(vdefs.size() + 1);
		tag_name_tf << m_ctag.get_default_titleformat();
		m_ctag.init(tag_name_tf);		
		vdefs.emplace_back(std::pair("0", tag_name), std::pair(m_ctag.get_tagname(), ""));
		loadComboCreditUserDefinitions(m_hWnd, ctrl_id, vdefs, tag_name);
	}

	// init ui preview titleformat

	uSetDlgItemText(m_hWnd, IDC_TAG_CREDITS_STATIC_PREVIEW_TF, tag_name_tf);

	// credit_tag_nfo inititalization
		
	m_ctag.init(tag_name_tf);

	// build credit, heading and subheading definitions (before CreateInDialog)

	vcredits = db.load_db_def_credits(vcats, vsubcats);

	// create in dialog ui SELECTION list

	m_credit_sel_list.CreateInDialog(*this, IDC_TAG_CREDIT_SEL_LIST);
	m_ctag.SetNotifier(stdf_credit_info_change_notifier);
	
	//fill ui sel list with current selection
	bit_array_bittable selection_check_mask = m_credit_sel_list.InitCatInfo(&m_ctag, vcats, vsubcats);

	// create CREDIT list

	m_credit_list.CreateInDialog(*this, IDC_TAG_CREDIT_LIST);
	m_ctag.SetNotifier(stdf_credit_info_change_notifier);

	//fill ui credit list
	m_credit_list.InitCatInfo(&m_ctag);

	//check credit items based on current selection
	m_credit_list.SetChecked(selection_check_mask);

	//init/update other ui controls 
	updateTagCreditUI(m_credit_list, false);

	// fill HEADING and SUBHEADING combos

	loadComboHeadings(m_hWnd, IDC_TAG_CREDITS_CMB_HEADING, vcats, vcats.at(0).first.second, true);
	loadComboHeadings(m_hWnd, IDC_TAG_CREDITS_CMB_SUBHEADING, vsubcats, vsubcats.at(0).first.second, false);

	m_credit_list.SetRowStyle(conf.list_style);
	m_credit_sel_list.SetRowStyle(conf.list_style);

	if (m_ptag_mappings == nullptr)
		m_ptag_mappings = new pfc::list_t<tag_mapping_entry>();

	tag_mapping_entry* entry = new tag_mapping_entry();
	entry->tag_name = tag_name;
	pfc::string8 tagscript = "%";
	tagscript << m_ctag.rebuild_tag_name() << "%";
	entry->formatting_script = tagscript;
	entry->enable_write = true;
	entry->enable_update = false;
	entry->freeze_tag_name = false;
	entry->freeze_update = false;
	entry->freeze_write = false;
	size_t index = m_ptag_mappings->add_item(*entry);

	HWND wnd_hledit = GetDlgItem(IDC_TAG_CREDITS_EDIT_HL);
	cewb_highlight.SubclassWindow(wnd_hledit);
	cewb_highlight.SetEnterEscHandlers();

	DlgResize_Init(mygripp.enabled, true);
	//load_size();

	cfg_window_placement_tag_mappings_credits_dlg.on_window_creation(m_hWnd, true);

	show();

	//on_mapping_changed(get_mapping_changed());

	::uPostMessage(m_hWnd, WM_NEXTDLGCTL, (WPARAM)(HWND)GetDlgItem(IDC_APPLY), TRUE);

	return FALSE;
}

unsigned insert_item4(HWND p_listview, unsigned p_index, const char* col0, const char* col1, const char* col2, const char* col3, const char* col4, LPARAM p_param) {
	unsigned i = listview_helper::insert_item(p_listview, p_index, col0, p_param);
	if (i != ~0) {
		listview_helper::set_item_text(p_listview, i, 1, col1);
		listview_helper::set_item_text(p_listview, i, 2, col2);
		listview_helper::set_item_text(p_listview, i, 3, col3);
		listview_helper::set_item_text(p_listview, i, 4, col4);
	}
	return i;
}

void CTagCreditDialog::applymappings() {

	if (!update_current_template()) return;

	db_fetcher_component db;
	bool bres = db.update_db_def_credits(vdefs);
}

LRESULT CTagCreditDialog::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	applymappings();
	if (g_discogs->tag_mappings_dialog)
		g_discogs->tag_mappings_dialog->SetVDefs(vdefs);
	destroy();
	return TRUE;
}

LRESULT CTagCreditDialog::OnApply(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	applymappings();
	if (g_discogs->tag_mappings_dialog)
		g_discogs->tag_mappings_dialog->SetVDefs(vdefs);
	return FALSE;
}

LRESULT CTagCreditDialog::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	conf = CONF;
	destroy();
	return TRUE;
}

LRESULT CTagCreditDialog::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {

	if (build_current_cfg()) {
		pushcfg(true);
	}

	cfg_window_placement_tag_mappings_credits_dlg.on_window_destruction(m_hWnd);
	return FALSE;
}

LRESULT CTagCreditDialog::OnPreview(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	
	pfc::string8 msg;

	UINT ctrl_id = IDC_TAG_CREDITS_CMB_DEFS;
	int s = uSendDlgItemMessage(ctrl_id, CB_GETCURSEL, 0, 0);

	if (s == CB_ERR) return FALSE;

	auto def_row = vdefs.at(s);
	tag_mapping_entry entry;
	m_ptag_mappings->get_item_ex(entry, 0);

	entry.tag_name = def_row.first.second;
	pfc::string8 tagscript = "%";
	tagscript << m_ctag.rebuild_tag_name() << "%";
	entry.formatting_script = tagscript;

	m_ptag_mappings->replace_item(0, entry);

	m_release_id = g_discogs->track_matching_dialog ? g_discogs->track_matching_dialog->get_discogs_release_id() : "";
	if (g_discogs->find_release_dialog) {

		m_items = g_discogs->find_release_dialog->getitems();

		file_info_impl finfo;
		metadb_handle_ptr item = m_items[0];
		item->get_info(finfo);

		pfc::string8 buffer;

		if (!m_release_id.get_length() && g_discogs->file_info_get_tag(item, finfo, TAG_RELEASE_ID, buffer)) {
			m_release_id = buffer;
		}
	}

	if (m_release_id.get_length()) {

		pfc::string8 newwhere;
		m_ctag.build_vwhere_cats(newwhere);
		service_ptr_t<process_aside_release_callback> task = new service_impl_t<process_aside_release_callback>(this, m_release_id, "offlineArtistId", m_ctag.get_tagname()/*entry.formatting_script*/, newwhere/*, items*/);
		task->start(m_hWnd);
		generating_tags = true;
		return FALSE;
	}
	return FALSE;
}

void CTagCreditDialog::cb_add_credit() {
	//generating preview
	m_pfinfo_manager = std::make_shared<file_info_manager>(m_items);
	m_pfinfo_manager->read_infos();
	bool bypass_is_cache = false;
	bool bypass = true;
	Release_ptr release = discogs_interface->get_release(m_release_id, bypass_is_cache, bypass);
	m_tag_writer = std::make_shared<TagWriter>(m_pfinfo_manager, release);
	m_tag_writer->track_mappings.force_reset();

	track_mapping track;
	track.enabled = true;
	track.discogs_disc = 0; //DECODE_DISCOGS_DISK(dindex);
	track.discogs_track = 0;  //DECODE_DISCOGS_TRACK(dindex);
	track.file_index = 0;

	m_tag_writer->track_mappings.append_single(track);

	service_ptr_t<generate_tags_task> task = new service_impl_t<generate_tags_task>(this, m_tag_writer, m_ptag_mappings);
	task->start();
}

void CTagCreditDialog::cb_generate_tags() {
	generating_tags = false;
	pfc::string8 myres;
	for (size_t walk_results = 0; walk_results < m_tag_writer->tag_results.get_count(); walk_results++) {
		tag_result_ptr tag_res = m_tag_writer->tag_results[walk_results];
		for (size_t walk_values = 0; walk_values < tag_res->value.get_count(); walk_values++) {
			myres << tag_res->value[walk_values].print();
			int debug = tag_res->value.get_count();		
			debug = debug;
		}
	}
	uSetDlgItemText(m_hWnd, IDC_TAG_CREDITS_PREVIEW, myres);
}

LRESULT CTagCreditDialog::OnBtnRename(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	
	TemplateNameDialog dialog;

	int oldselected_ndx = uSendDlgItemMessage(IDC_TAG_CREDITS_CMB_DEFS, CB_GETCURSEL, 0, 0);

	if (dialog.query(m_hWnd, vdefs.at(oldselected_ndx).first.second)) {
		dialog.value.skip_trailing_char(' ');
		if (dialog.value.get_length()) {
			auto existing = std::find_if(vdefs.begin(), vdefs.end(), [&](const rppair arow) {
				return STR_EQUAL(arow.first.second, dialog.value.c_str()); });

			if (existing == vdefs.end()) {
				vdefs.at(oldselected_ndx).first.second = dialog.value;
				loadComboCreditUserDefinitions(m_hWnd, IDC_TAG_CREDITS_CMB_DEFS, vdefs, dialog.value);
			}
			else {
				//todo: template name already in use
			}
		}
	}
	return FALSE;
}

LRESULT CTagCreditDialog::OnBtnRemove(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	TemplateNameDialog dialog;
	pfc::string8 oldselection = m_ctag.get_default_name(vdefs.size() + 1);
	uGetDlgItemText(m_hWnd, IDC_TAG_CREDITS_CMB_DEFS, oldselection);
	size_t oldselected_ndx = uSendDlgItemMessage(IDC_TAG_CREDITS_CMB_DEFS, CB_GETCURSEL, 0, 0);
	pfc::string8 title;
	title << "Delete template \"" << oldselection << "\"";
	if (IDYES == uMessageBox(m_hWnd, "Are you sure?", title,
		MB_APPLMODAL | MB_YESNO | MB_ICONQUESTION)) {
		vdefs.erase(vdefs.begin() + oldselected_ndx);
		loadComboCreditUserDefinitions(m_hWnd, IDC_TAG_CREDITS_CMB_DEFS, vdefs, nullptr);
		CComboBox cmb = uGetDlgItem(IDC_TAG_CREDITS_CMB_DEFS);
		if (vdefs.size() > oldselected_ndx) {
			uSendDlgItemMessage(IDC_TAG_CREDITS_CMB_DEFS, CB_SETCURSEL, cmb.FindStringExact(0, pfc::stringcvt::string_os_from_utf8(vdefs.at(oldselected_ndx).first.second.get_ptr())), 0);
			SendMessage(m_hWnd, WM_COMMAND, MAKEWPARAM(IDC_TAG_CREDITS_CMB_DEFS, CBN_SELCHANGE), (LPARAM)cmb.m_hWnd);
		}
		else if (vdefs.size()) {
			cmb.SelectString(0, pfc::stringcvt::string_os_from_utf8(vdefs.at(vdefs.size() - 1).first.second.get_ptr()));
		}
		else {
			pfc::string8 tag_name, tag_name_ff;
			tag_name << m_ctag.get_default_name(vdefs.size() + 1);
			tag_name_ff << m_ctag.get_default_titleformat();
			m_ctag.init(tag_name_ff);

			vdefs.emplace_back(std::pair("0", tag_name), std::pair(m_ctag.get_tagname(), ""));
			uSendDlgItemMessage(IDC_TAG_CREDITS_CMB_DEFS, CB_SETCURSEL, 0, 0);
		}		
	}
	return FALSE;
}

LRESULT CTagCreditDialog::OnBtnDuplicate(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	TemplateNameDialog dialog;
	pfc::string8 oldselection = m_ctag.get_default_name(vdefs.size() + 1);
	uGetDlgItemText(m_hWnd, IDC_TAG_CREDITS_CMB_DEFS, oldselection);
	int oldselected_ndx = uSendDlgItemMessage(IDC_TAG_CREDITS_CMB_DEFS, CB_GETCURSEL, 0, 0);
	
	if (dialog.query(m_hWnd, oldselection)) {
		dialog.value.skip_trailing_char(' ');
		if (dialog.value.get_length()) {
			auto existing = std::find_if(vdefs.begin(), vdefs.end(), [&](const rppair arow) {
				return STR_EQUAL(arow.first.second, dialog.value.c_str()); });

			if (existing == vdefs.end() &&
				STR_EQUAL(vdefs.at(oldselected_ndx).first.second, oldselection.c_str())) {
				auto row = vdefs.at(oldselected_ndx);
				row.first.second = dialog.value;
				m_ctag.init(row.second.first);
			
				vdefs.emplace_back(row);

				UINT ctrl_id = IDC_TAG_CREDITS_CMB_DEFS;
				bit_array_bittable selection_check_mask = m_credit_sel_list.InitCatInfo(&m_ctag, vcats, vsubcats);
				m_credit_list.SetChecked(selection_check_mask);
				updateTagCreditUI(nullptr, true);
				loadComboCreditUserDefinitions(m_hWnd, IDC_TAG_CREDITS_CMB_DEFS, vdefs, dialog.value);
			}
		}
	}
	return FALSE;
}

LRESULT CTagCreditDialog::OnBtnAddNew(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	TemplateNameDialog dialog;
	pfc::string8 oldselection = m_ctag.get_default_name(vdefs.size() + 1);
	int oldselected_ndx = uSendDlgItemMessage(IDC_TAG_CREDITS_CMB_DEFS, CB_GETCURSEL, 0, 0);
	if (dialog.query(m_hWnd, oldselection)) {
		dialog.value.skip_trailing_char(' ');
		if (dialog.value.get_length()) {
			auto existing = std::find_if(vdefs.begin(), vdefs.end(), [&](const rppair arow) {
				return STR_EQUAL(arow.first.second, dialog.value.c_str()); });

			if (existing == vdefs.end()) {

				pfc::string8 tag_name, tag_name_ff;
				tag_name << dialog.value;
				tag_name_ff << m_ctag.get_default_titleformat();
				m_ctag.init(tag_name_ff);

				vdefs.emplace_back(std::pair("0", tag_name), std::pair(m_ctag.get_tagname(), ""));

				UINT ctrl_id = IDC_TAG_CREDITS_CMB_DEFS;
				bit_array_bittable selection_check_mask = m_credit_sel_list.InitCatInfo(&m_ctag, vcats, vsubcats);
				m_credit_list.SetChecked(selection_check_mask);
				updateTagCreditUI(nullptr, true);
				loadComboCreditUserDefinitions(m_hWnd, ctrl_id, vdefs, tag_name);
			}
		}
	}
	return FALSE;
}

bool CTagCreditDialog::update_current_template(int sel) {

	if (sel == -1) {
		UINT ctrl_id = IDC_TAG_CREDITS_CMB_DEFS;
		sel = uSendDlgItemMessage(ctrl_id, CB_GETCURSEL, 0, 0);
		if (sel == CB_ERR) {
			return false;
		}
	}
	pfc::string8 name, titleformat;
	titleformat << m_ctag.get_tagname();
	vdefs.at(sel).second.first = titleformat;
	return true;
}

LRESULT CTagCreditDialog::OnCmbDefinitionsSelection(WORD wNotifyCode, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	if (wNotifyCode == CBN_SELCHANGE) {

		int s = uSendDlgItemMessage(wID, CB_GETCURSEL, 0, 0);

		if (s != CB_ERR) {

			update_current_template(last_template_selection);
			last_template_selection = s;

			LPARAM lparam = uSendDlgItemMessage(wID, CB_GETITEMDATA, s, 0);
			auto row = vdefs.at(s);

			m_ctag.init(row.second.first);
			bit_array_bittable selection_check_mask = m_credit_sel_list.InitCatInfo(&m_ctag, vcats, vsubcats);
			m_credit_list.SetChecked(selection_check_mask);
			updateTagCreditUI(nullptr, true);
		}
	}
	return FALSE;
}

LRESULT CTagCreditDialog::OnSplitDropDown(LPNMHDR lParam) {
	NMBCDROPDOWN* pDropDown = (NMBCDROPDOWN*)lParam;
	if (pDropDown->hdr.hwndFrom == GetDlgItem(IDC_BTN_TAG_CREDIT_TEST))
	{
		POINT pt;
		pt.x = pDropDown->rcButton.left;
		pt.y = pDropDown->rcButton.bottom;
		::ClientToScreen(pDropDown->hdr.hwndFrom, &pt);
		enum { MENU_RENAME = 1, MENU_DUPLICATE };

		HMENU hSplitMenu = CreatePopupMenu();
		AppendMenu(hSplitMenu, MF_STRING, MENU_RENAME, L"Rename...");
		AppendMenu(hSplitMenu, MF_STRING, MENU_DUPLICATE, L"Duplicate...");
	
		int cmd = TrackPopupMenu(hSplitMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD, pt.x, pt.y, 0, m_hWnd, NULL);
		DestroyMenu(hSplitMenu);
		switch (cmd)
		{
			BOOL bDummy;
		case MENU_RENAME:
			OnBtnRename(NULL, NULL, NULL, bDummy);
			break;
		case MENU_DUPLICATE:
			OnBtnDuplicate(NULL, NULL, NULL, bDummy);
			break;
		}
	}
	return TRUE;
}

bool ScrollToHeading(CListControlCredits* list, size_t heading, size_t subheading) {
	size_t pos = list->GetFirstItemByHeader(heading, subheading);
	if (pos != ~0) {
		list->SelectSingle(pos);
		list->SetFocusItem(pos);
		list->EnsureItemVisible(pos);		
	}
	return pos != ~0;
}

LRESULT CTagCreditDialog::OnBtnAddHeadings(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	int id_heading, id_subheading;
	int cursel = uSendDlgItemMessage(IDC_TAG_CREDITS_CMB_HEADING, CB_GETCURSEL, 0, 0);
	if (cursel != CB_ERR) {
		id_heading = uSendDlgItemMessage(IDC_TAG_CREDITS_CMB_HEADING, CB_GETITEMDATA, cursel, 0);
	}
	else id_heading = -1;

	cursel = uSendDlgItemMessage(IDC_TAG_CREDITS_CMB_SUBHEADING, CB_GETCURSEL, 0, 0);
	if (cursel != CB_ERR) {
		id_subheading = uSendDlgItemMessage(IDC_TAG_CREDITS_CMB_SUBHEADING, CB_GETCURSEL, 0, 0);
	}
	else id_subheading = -1;

	bool bHeading = (wID == IDC_TAG_CREDITS_BTN_ADD_HEADING) || (wID == IDC_TAG_CREDITS_BTN_SCROLL_HEADING);
	bool bSubHeading = (wID == IDC_TAG_CREDITS_BTN_ADD_SUBHEADING) || (wID == IDC_TAG_CREDITS_BTN_SCROLL_SUBHEADING);
	bool bAdd = (wID == IDC_TAG_CREDITS_BTN_ADD_HEADING) || (wID == IDC_TAG_CREDITS_BTN_ADD_SUBHEADING);

	if (bHeading) {
		if (id_heading == CB_ERR) return FALSE;

		if (bAdd) {
			pfc::string8 newval;
			newval << id_heading;
			m_ctag.pass_cat_to_v(newval, true);
		}
		else {
			ScrollToHeading(&m_credit_list, id_heading, pfc_infinite);
		}
	}
	else if (bSubHeading) {
		if (id_subheading == CB_ERR) return FALSE;
		if (bAdd) {			
			pfc::string8 newval;
			newval << (id_subheading == 0 ? "1" : pfc::string8("1-") << id_subheading);
			m_ctag.pass_cat_to_v(newval, true);
		}
		else {
			ScrollToHeading(&m_credit_list, 1, id_subheading);
		}
	}
	else {
		return FALSE;
	}

	if (bAdd) {
		m_ctag.rebuild_tag_name();
		m_credit_sel_list.InitCatInfo(&m_ctag, vcats, vsubcats);
		updateTagCreditUI(nullptr, true);
	}

	return FALSE;
}

LRESULT CTagCreditDialog::OnBtnCheckGroup(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
	bool next_checked = uButton_GetCheck(m_hWnd, wID);
	bool changed = false;
	bHandled = TRUE;

	//checkbox pair
	if (!next_checked) {
		uButton_SetCheck(m_hWnd, IDC_TAG_CREDITS_GRP_GXC, m_ctag.isGXC());
		uButton_SetCheck(m_hWnd, IDC_TAG_CREDITS_GRP_GXP, m_ctag.isGXP());
		return 0;
	}

	if (wID == IDC_TAG_CREDITS_GRP_GXC) {
		changed = next_checked != m_ctag.isGXC();
		if (next_checked)
			m_ctag.set_groupby(GROUP_BY_GXC);

		uButton_SetCheck(m_hWnd, IDC_TAG_CREDITS_GRP_GXC, next_checked);
		uButton_SetCheck(m_hWnd, IDC_TAG_CREDITS_GRP_GXP, !next_checked);
	}
	else {
		changed = next_checked != m_ctag.isGXP();
		if (next_checked)
			m_ctag.set_groupby(GROUP_BY_GXP);

		uButton_SetCheck(m_hWnd, IDC_TAG_CREDITS_GRP_GXP, next_checked);
		uButton_SetCheck(m_hWnd, IDC_TAG_CREDITS_GRP_GXC, !next_checked);
	}

	if (changed) {
		m_ctag.rebuild_tag_name();
		updateTagCreditUI(nullptr, false);
	}
	return 0;
}

LRESULT CTagCreditDialog::OnBtnRadioSource(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	if (wID == IDC_TAG_CREDITS_RADIO_ALL) {
		m_ctag.set_src("ALL");
	}
	else if (wID == IDC_TAG_CREDITS_RADIO_RELEASE) {
		m_ctag.set_src("RELEASE");
	}
	else if (wID == IDC_TAG_CREDITS_RADIO_TRACKS) {
		m_ctag.set_src("TRACKS");
	}
	m_ctag.rebuild_tag_name();
	updateTagCreditUI(nullptr, false);

	return FALSE;
}

LRESULT CTagCreditDialog::OnEditHLText(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& /*bHandled*/) {

	if (wNotifyCode == EN_CHANGE)
	{
		pfc::string8 hl_str = pfc::stringToLower(trim(pfc::string8(uGetWindowText(hWndCtl).c_str())));
		m_credit_list.SetHLString(hl_str, 1);
	}
	return FALSE;

}
