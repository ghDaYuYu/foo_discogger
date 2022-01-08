#pragma once

#include "resource.h"
#include "helpers/win32_dialog.h"

#include "../../libPPUI/CListControlComplete.h"

#include "foo_discogs.h"
#include "tags.h"
#include "tag_writer.h"
#include "ceditwithbuttonsdim.h"

#include "tag_mapping_credit_utils.h"

class CTagCreditDialog;

typedef vppair::iterator credit_iterator_t;
typedef std::pair<std::pair<pfc::string8, pfc::string8>, std::pair<pfc::string8, pfc::string8>> rppair;

struct listData_t {
	credit_iterator_t credit_it;
	pfc::string8 m_credit_id, m_credit, m_notes, m_key, m_value, m_subkey, m_subvalue;
	bool m_checkState = false;
};
static std::vector<listData_t> makeListData(std::vector<rppair> dlg_credits) {
	std::vector<listData_t> data;
	data.resize(dlg_credits.size());

	credit_iterator_t it = dlg_credits.begin();
	
	for (size_t walk = 0; walk < data.size(); ++walk) {	
		auto& rec = data[walk];
		rec.credit_it = it;
		LPARAM lparam = (LPARAM)atoi(it->first.first);
		size_t tmpval = LOWORD(lparam);
		rec.m_credit_id = tmpval ? std::to_string(tmpval).c_str() : "";
		tmpval = HIWORD(lparam) / 100;
		rec.m_key = tmpval ? std::to_string(tmpval).c_str() : "";
		tmpval = HIWORD(lparam) - ((HIWORD(lparam) / 100) * 100);
		rec.m_subkey = tmpval ? std::to_string(tmpval).c_str() : "";
		std::vector<pfc::string8> split_credit_name_notes;
		split(it->first.second, "|", 0, split_credit_name_notes);
		rec.m_credit = split_credit_name_notes.at(0);
		rec.m_notes = split_credit_name_notes.size() > 1 ? split_credit_name_notes.at(1) : "";
		rec.m_value = it->second.first;
		rec.m_subvalue = it->second.second;
		++it;
	}
	return data;
}

static std::vector<listData_t> makeSelListData(vppair dlg_vcredits, vppair dlg_vcats, vppair dlg_vsubcats, credit_tag_nfo* m_ctag, bit_array_bittable &checkmask) {
	
	checkmask.resize(dlg_vcredits.size());
	checkmask.set(checkmask.size(), false);

	std::vector<listData_t> data;
	data.resize(m_ctag->get_vsplit().size());

	for (size_t walk_elem = 0; walk_elem < m_ctag->get_vsplit().size(); walk_elem++) {
		size_t elem_type = m_ctag->GetElemType(walk_elem);
		if (elem_type == credit_tag_nfo::CREDIT) {
			auto credit_it = std::find_if(dlg_vcredits.begin(), dlg_vcredits.end(), [walk_elem, m_ctag](const rppair arow) {
				pfc::string8 tmp = m_ctag->get_vsplit()[walk_elem];
				if (tmp.has_prefix("+")) tmp = substr(tmp, 1, tmp.get_length() - 1);
				pfc::string8 arow_subcat_id = std::to_string(HIWORD(atoi(arow.first.first))).c_str();
				pfc::string8 arow_cat_id = std::to_string(LOWORD(atoi(arow.first.first))).c_str();
				return STR_EQUAL(arow_cat_id, tmp)/*atoi(arow.first.first) == atoi(tmp)*/;
				});

			if (credit_it != dlg_vcredits.end()) {
				auto& rec = data[walk_elem];
				rppair rpp_credit = *credit_it;
				size_t catsubcat =HIWORD(atoi(rpp_credit.first.first));
				pfc::string8 cat_id = std::to_string(catsubcat / 100).c_str();
				pfc::string8 subcat_id = std::to_string(catsubcat - (catsubcat / 100 * 100)).c_str();
				pfc::string8 credit_id = std::to_string(LOWORD(atoi(rpp_credit.first.first))).c_str();
				rec.credit_it = credit_it;
				rec.m_credit_id = credit_id;
				std::vector<pfc::string8> split_credit_name_notes;
				split(credit_it->first.second, "|", 0, split_credit_name_notes);
				rec.m_credit = split_credit_name_notes.at(0);

				if (catsubcat / 100) {
					rec.m_key = cat_id;
					auto row = dlg_vcats[atoi(cat_id) - 1]; //vector is zero based
					rec.m_value = row.first.second;
					rec.m_subkey = subcat_id.c_str();
					if (atoi(subcat_id)) {
						row = dlg_vsubcats[atoi(subcat_id) - 1];
						rec.m_subvalue = row.second.first;
					}
				}
				checkmask.set(std::distance(dlg_vcredits.begin(), credit_it), true);
			}
		}
		else {

			if (elem_type == credit_tag_nfo::SUBHEADER) {
				auto& rec = data[walk_elem];
				std::vector<pfc::string8> vtmp;
				split(m_ctag->get_vsplit()[walk_elem], "-", 0, vtmp);

				rppair row = dlg_vsubcats[atoi(vtmp.at(1).c_str())-1]; //vector is zero based
				rec.m_subkey = row.first.first;
				rec.m_subvalue = row.second.first;
				rec.m_key = row.first.second;
				row = dlg_vcats[atoi(rec.m_key.c_str()) - 1]; //vector is zero based
				rec.m_value = row.first.second;
				rec.m_credit = "*";
			}
			else if (elem_type == credit_tag_nfo::HEADER) {
				auto& rec = data[walk_elem];
				rppair row;
				if (atoi(m_ctag->get_vsplit()[walk_elem].c_str()) == 0) {
					row = rppair(std::pair("0","*"),std::pair("0","0"));
				}
				else {
					row = dlg_vcats[atoi(m_ctag->get_vsplit()[walk_elem].c_str()) - 1];
				}
				rec.m_key = row.first.first;
				rec.m_value = row.first.second;
				rec.m_credit = "*";
				rec.m_subvalue = "*";				
			}
		}
	}
	return data;
}

typedef CListControlComplete CListControlCreditsParent;

// * * * * * * * * * * * * * * * * * * * * * * * * *
// CREDITS LIST

class CListControlCredits : public CListControlCreditsParent {
	typedef CListControlCreditsParent parent_t;

public:
	BEGIN_MSG_MAP_EX(CListControlCredits)
	CHAIN_MSG_MAP(parent_t);
	MSG_WM_CREATE(OnCreate)
	//MSG_WM_CONTEXTMENU(OnContextMenu)
	//CHAIN_MSG_MAP(parent_t)
	END_MSG_MAP()

	CListControlCredits(CTagCreditDialog* parent) {
		SetSelectionModeSingle();
		m_dlg_parent = parent;
		m_ctag = nullptr;
	}
	~CListControlCredits() {
		m_dlg_parent = nullptr;
	}

	void SetHLString(pfc::string8 hlstring, bool erase = 1) {
		m_hl_string = hlstring;
		this->Invalidate(erase);
	}

	void RenderItemBackground(CDCHandle p_dc, const CRect& p_itemRect, size_t item, uint32_t bkColor);

	int OnCreate(LPCREATESTRUCT lpCreateStruct);

	void InitCatInfo(credit_tag_nfo* ctag) { 
		m_ctag = ctag;
		m_data = makeListData(m_credits);
	}

	size_t GetFirstItemByHeader(size_t header, size_t subheader) {
		auto res = std::find_if(m_data.begin(), m_data.end(),
			[&](const listData_t& arow) {
				if (subheader == pfc_infinite)
					return atoi(arow.m_key) == header; 
				else
					return atoi(arow.m_key) == header && atoi(arow.m_subkey) == subheader; 
			});
		if (res == m_data.end())
			return pfc_infinite;
		else
			return std::distance(m_data.begin(), res);
	}

	void SetChecked(pfc::bit_array_bittable checkmask) { 
		for (size_t walk_check = 0; walk_check < checkmask.size(); walk_check++) {
			m_data[walk_check].m_checkState = checkmask.get(walk_check);
		}
		ReloadData();
	}

	void InitHeader() {

		InitializeHeaderCtrl(HDS_FULLDRAG);
		auto DPI = m_dpi;
		AddColumn("", MulDiv(16, DPI.cx, 96));
		AddColumn("#c", MulDiv(30, DPI.cx, 96));
		AddColumn("Credit", MulDiv(120, DPI.cx, 96));
		AddColumn("#h", MulDiv(22, DPI.cx, 96));
		AddColumn("Heading", MulDiv(110, DPI.cx, 96));
		AddColumn("#sh", MulDiv(22, DPI.cx, 96));
		AddColumn("Subheading", MulDiv(110, DPI.cx, 96));

		size_t wpre_notes = 0, wnotes = 0;
		size_t notes_pos = 7;
		for (size_t pre_walk = 0; pre_walk < notes_pos; pre_walk++)
			wpre_notes += static_cast<size_t>(GetColumnWidthF(pre_walk));

		CRect reclist;
		GetHeaderCtrl().GetClientRect(reclist);

		int wframe = reclist.Width();
		int wdelta = wframe - wpre_notes - 40;

		if (wdelta < 0) {
			AddColumn("Notes", MulDiv(110, DPI.cx, 96));
		}
		else {
			AddColumn("Notes", wdelta);
		}
	}

	bool IsDragDropSupported() override	{ 
		return false;
	}

	bool CanSelectItem(size_t row) const override {
		// can not select footer
		return row != footerRow();
	}

	size_t footerRow() const {
		return m_data.size();
	}

	t_size GetItemCount() const override {
		return m_data.size() + 1; // footer
	}

	void onFooterClicked() {
		SelectNone();
	}

	void OnSubItemClicked(t_size item, t_size subItem, CPoint pt) override {
		if (item == footerRow()) {
			onFooterClicked(); return;
		}

		if (subItem == 0) {
			m_ctag->SetTagCredit(credit_tag_nfo::CREDIT, m_data[item].m_credit_id.c_str(), !m_data[item].m_checkState);			
		}
		__super::OnSubItemClicked(item, subItem, pt);
	}

	bool AllowScrollbar(bool vertical) const override {
		return true;
	}

	t_size InsertIndexFromPointEx(const CPoint& pt, bool& bInside) const override {
		// Drag&drop insertion point hook, for reordering only
		auto ret = __super::InsertIndexFromPointEx(pt, bInside);
		bInside = false; // never drop *into* an item, only between, as we only allow reorder
		if (ret > m_data.size()) ret = m_data.size(); // never allow drop beyond footer
		return ret;
	}

	void RequestReorder(size_t const* order, size_t count) override {
		// we've been asked to reorder the items, by either drag&drop or cursors+modifiers
		PFC_ASSERT(count == GetItemCount());

		if (order[footerRow()] != footerRow()) return;

		pfc::reorder_t(m_data, order, count);
		this->OnItemsReordered(order, count);
	}

	void RemoveMask(pfc::bit_array const& mask) {
		if (mask.get(footerRow())) return;
		auto oldCount = GetItemCount();
		pfc::remove_mask_t(m_data, mask);
		this->OnItemsRemoved(mask, oldCount);
	}

	void RequestRemoveSelection() override {
		// Delete key etc
		RemoveMask(GetSelectionMask());
	}

	void ExecuteDefaultAction(t_size index) override {
		//custom default action toggles item checkbox
		CRect chkrc = GetSubItemRect(index, 0);
		OnSubItemClicked(index, 0, CPoint(chkrc.CenterPoint()));
	}

	bool GetSubItemText(t_size item, t_size subItem, pfc::string_base& out) const override {
		if (item == footerRow()) {
			return false;
		}

		auto& rec = m_data[item];
		switch (subItem) {
		case 0:
			out = "check";
			return true;
		case 1:
			out = rec.m_credit_id.c_str();
			return true;
		case 2:
			out = rec.m_credit.get_ptr();
			return true;
		case 3:
			out = rec.m_key.c_str();
			return true;
		case 4:
			out = rec.m_value.c_str();
			return true;
		case 5:
			out = rec.m_subkey.c_str();
			return true;
		case 6:
			out = rec.m_subvalue.c_str();
			return true;
		case 7:
			out = rec.m_notes.c_str();
			return true;
		
		default:
			return false;
		}
	}

	size_t GetSubItemSpan(size_t row, size_t column) const override {
		if (row == footerRow() && column == 0) {
			return GetColumnCount();
		}
		return 1;
	}

	cellType_t GetCellType(size_t item, size_t subItem) const override {
		// cellType_t is a pointer to a cell class object supplying cell behavior specification & rendering methods
		// use PFC_SINGLETON to provide static instances of used cells
		if (item == footerRow()) {
			if (subItem == 0) {
				return &PFC_SINGLETON(CListCell_Button);
			}
			else {
				return nullptr;
			}
		}
		switch (subItem) {
		case 0:
			return &PFC_SINGLETON(CListCell_Checkbox);
		default:
			return &PFC_SINGLETON(CListCell_Text);
		}

	}

	bool GetCellTypeSupported() const override {
		return true;
	}

	bool GetCellCheckState(size_t item, size_t subItem) const override {
		if (subItem == 0) {
			auto& rec = m_data[item];
			return rec.m_checkState;
		}
		return false;
	}

	void SetCellCheckState(size_t item, size_t subItem, bool value) override {
		if (subItem == 0) {
			auto& rec = m_data[item];

			rec.m_checkState = value;
			__super::SetCellCheckState(item, subItem, value);

		}
	}

	uint32_t QueryDragDropTypes() const override { return dragDrop_reorder; }

	void TableEdit_SetField(t_size item, t_size subItem, const char* value) override {
		if (subItem == 2) {
			m_data[item].m_value = value;
			ReloadItem(item);
		}
	}

	bool TableEdit_IsColumnEditable(t_size subItem) const override {
		return false;
	}

public:

	credit_tag_nfo* m_ctag;

private:

	std::vector< listData_t > m_data;

	vppair m_credits;

	CTagCreditDialog* m_dlg_parent = nullptr;
	
	pfc::string8 m_hl_string;
};

// SELECTION LIST

class CListControlCredit_Selection : public CListControlCreditsParent {
	typedef CListControlCreditsParent parent_t;
public:
	BEGIN_MSG_MAP_EX(CListControlCredit_Selection)
	CHAIN_MSG_MAP(parent_t);
	MSG_WM_CREATE(OnCreate)
	MSG_WM_CONTEXTMENU(OnContextMenu)
	END_MSG_MAP()

	CListControlCredit_Selection(CTagCreditDialog* parent) {
		m_dlg_parent = parent;
		m_ctag = nullptr;
	}
	~CListControlCredit_Selection() {
		m_dlg_parent = nullptr;
	}

	void OnContextMenu(CWindow wnd, CPoint point) {
		// did we get a (-1,-1) point due to context menu key rather than right click?
		// GetContextMenuPoint fixes that, returning a proper point at which the menu should be shown
		point = this->GetContextMenuPoint(point);

		CMenu menu;
		// WIN32_OP_D() : debug build only return value check
		// Used to check for obscure errors in debug builds, does nothing (ignores errors) in release build
		WIN32_OP_D(menu.CreatePopupMenu());

		enum { ID_REMOVE = 1, ID_DISABLE, ID_SELECTALL, ID_SELECTNONE, ID_INVERTSEL };
		menu.AppendMenu(MF_STRING, ID_REMOVE, L"Remove");
		menu.AppendMenu(MF_SEPARATOR);
		// Note: Ctrl+A handled automatically by CListControl, no need for us to catch it
		menu.AppendMenu(MF_STRING, ID_SELECTALL, L"Select all\tCtrl+A");
		menu.AppendMenu(MF_STRING, ID_SELECTNONE, L"Select none");
		menu.AppendMenu(MF_STRING, ID_INVERTSEL, L"Invert selection");

		int cmd;
		{
			CMenuDescriptionMap descriptions(m_hWnd);

			descriptions.Set(ID_REMOVE, "Remove list items");
			descriptions.Set(ID_SELECTALL, "Selects all items");
			descriptions.Set(ID_SELECTNONE, "Deselects all items");
			descriptions.Set(ID_INVERTSEL, "Invert selection");

			cmd = menu.TrackPopupMenuEx(TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, point.x, point.y, descriptions, nullptr);
		}
		switch (cmd) {
		case ID_REMOVE:
		{
			pfc::bit_array_bittable selmask = GetSelectionMask();
			size_t erased = 0;
			for (size_t walk_sel = 0; walk_sel < selmask.size(); walk_sel++) {
				if (selmask.get(walk_sel)) {
					m_ctag->RemoveTagCredit(walk_sel - erased, false);
					++erased;
				}
			}
			if (erased) {
				m_ctag->ChangeNotify(wnd.m_hWnd, true);
			}
			break;
		}
		
		case ID_SELECTALL:
			this->SelectAll();
			break;
		case ID_SELECTNONE:
			this->SelectNone();
			break;
		case ID_INVERTSEL:
		{
			auto mask = this->GetSelectionMask();
			this->SetSelection(
				// Items which we alter - all of them
				pfc::bit_array_true(),
				// Selection values - NOT'd original selection mask
				pfc::bit_array_not(mask)
			);
		}
		break;
		}
	}

	void OnItemsReordered(const size_t* order, size_t count) override {
		std::vector<pfc::string8> vs = m_ctag->get_vsplit();
		std::vector<pfc::string8> vperm;		
		for (t_size walk = 0; walk < count; ++walk) {
			vperm.push_back(vs.at(order[walk]));
		}
		m_ctag->rebuild_tag_name(vperm);
		m_ctag->ChangeNotify(this->m_hWnd, true);
	}

	void OnItemsRemoved(pfc::bit_array const& mask, size_t oldCount) override {

		size_t erased = 0;
		for (size_t walk_sel = 0; walk_sel < oldCount; walk_sel++) {
			if (mask.get(walk_sel)) {
				m_ctag->RemoveTagCredit(walk_sel - erased, false);
				++erased;
			}
		}
		if (erased)
			m_ctag->ChangeNotify(this->m_hWnd, true);
	}

	int OnCreate(LPCREATESTRUCT lpCreateStruct);

	bit_array_bittable InitCatInfo(credit_tag_nfo* ctag,  vppair dlg_vcats, vppair dlg_vcatsub ) { 
		bit_array_bittable bab_res;
		m_ctag = ctag;
		m_data = makeSelListData(m_credits, dlg_vcats, dlg_vcatsub, ctag, bab_res);
		return bab_res;
	}

	void InitHeader() {
		InitializeHeaderCtrl(HDS_FULLDRAG);
		auto DPI = m_dpi;
		AddColumn("", MulDiv(0, DPI.cx, 96));
		AddColumn("#c", MulDiv(30, DPI.cx, 96));
		AddColumn("Credit Name", MulDiv(100, DPI.cx, 96));
		AddColumn("#h", MulDiv(30, DPI.cx, 96));
		AddColumn("Heading", MulDiv(75, DPI.cx, 96));
		AddColumn("#sh", MulDiv(30, DPI.cx, 96));
		AddColumn("Subheading", MulDiv(115, DPI.cx, 96));
	}

	bool CanSelectItem(size_t row) const override {
		return row != footerRow();
	}
	size_t footerRow() const {
		return ~0;
	}
	t_size GetItemCount() const override {
		return m_data.size();
	}
	void onFooterClicked() {
		SelectNone();
	}
	void OnSubItemClicked(t_size item, t_size subItem, CPoint pt) override {
		if (item == footerRow()) {
			onFooterClicked(); return;
		}

		if (subItem == 0) {
			int i = 0;
			m_ctag->SetTagCredit(0, m_data[item].m_credit_id.c_str(), !m_data[item].m_checkState);
		}
		__super::OnSubItemClicked(item, subItem, pt);
	}
	bool AllowScrollbar(bool vertical) const override {
		return true;
	}
	t_size InsertIndexFromPointEx(const CPoint& pt, bool& bInside) const override {
		// Drag&drop insertion point hook, for reordering only
		auto ret = __super::InsertIndexFromPointEx(pt, bInside);
		bInside = false; // never drop *into* an item, only between, as we only allow reorder
		if (ret > m_data.size()) ret = m_data.size(); // never allow drop beyond footer
		return ret;
	}
	void RequestReorder(size_t const* order, size_t count) override {
		// we've been asked to reorder the items, by either drag&drop or cursors+modifiers
		// we can either reorder as requested, reorder partially if some of the items aren't moveable, or reject the request

		PFC_ASSERT(count == GetItemCount());

		// Footer row cannot be moved
		if (footerRow() != pfc_infinite && order[footerRow()] != footerRow()) return;
		pfc::reorder_t(m_data, order, count);
		this->OnItemsReordered(order, count);
	}
	void RemoveMask(pfc::bit_array const& mask) {
		if (mask.get(footerRow())) return;
		auto oldCount = GetItemCount();
		pfc::remove_mask_t(m_data, mask);
		this->OnItemsRemoved(mask, oldCount);
	}
	void RequestRemoveSelection() override {
		// Delete key etc
		RemoveMask(GetSelectionMask());
	}
	void ExecuteDefaultAction(t_size index) override {
		// double click, enter key, etc
		if (index == footerRow())
			onFooterClicked();
	}

	bool GetSubItemText(t_size item, t_size subItem, pfc::string_base& out) const override {
		if (item == footerRow()) {
			return false;
		}
		auto& rec = m_data[item];
		switch (subItem) {
		case 0:
			out = "check";
			return true;
		case 1:
			out = rec.m_credit_id.c_str();
			return true;
		case 2:
			out = rec.m_credit.c_str();
			return true;
		case 3:
			out = rec.m_key.c_str();
			return true;
		case 4:
			out = rec.m_value.c_str();
			return true;
		case 5:
			out = rec.m_subkey.c_str();
			return true;
		case 6:
			out = rec.m_subvalue.c_str();
			return true;

		default:
			return false;
		}
	}

	size_t GetSubItemSpan(size_t row, size_t column) const override {
		if (row == footerRow() && column == 0) {
			return GetColumnCount();
		}
		return 1;
	}
	cellType_t GetCellType(size_t item, size_t subItem) const override {
		// cellType_t is a pointer to a cell class object supplying cell behavior specification & rendering methods
		// use PFC_SINGLETON to provide static instances of used cells
		if (item == footerRow()) {
			if (subItem == 0) {
				return &PFC_SINGLETON(CListCell_Button);
			}
			else {
				return nullptr;
			}
		}
		switch (subItem) {
		case 0:
			return &PFC_SINGLETON(CListCell_Checkbox);
		default:
			return &PFC_SINGLETON(CListCell_Text);
		}

	}
	bool GetCellTypeSupported() const override {
		return true;
	}
	bool GetCellCheckState(size_t item, size_t subItem) const override {
		if (subItem == 0) {
			auto& rec = m_data[item];
			return rec.m_checkState;
		}
		return false;
	}
	void SetCellCheckState(size_t item, size_t subItem, bool value) override {
		if (subItem == 0) {
			auto& rec = m_data[item];
			if (rec.m_checkState != value) {
				rec.m_checkState = value;
				__super::SetCellCheckState(item, subItem, value);
			}
		}
	}

	uint32_t QueryDragDropTypes() const override { return dragDrop_reorder; }

	// Inplace edit handlers
	// Overrides of CTableEditHelperV2 methods
	void TableEdit_SetField(t_size item, t_size subItem, const char* value) override {
		if (subItem == 2) {
			m_data[item].m_value = value;
			ReloadItem(item);
		}
	}
	bool TableEdit_IsColumnEditable(t_size subItem) const override {
		return subItem == 1000;
	}

public:
	credit_tag_nfo* m_ctag;
private:
	std::vector< listData_t > m_data;
	vppair m_credits;
	CTagCreditDialog* m_dlg_parent = nullptr;
};

class TemplateNameDialog : private dialog_helper::dialog_modal {
	BOOL on_message(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/) final {
		switch (uMsg) {
		case WM_INITDIALOG:
			if (value != nullptr) {
				uSetDlgItemText(get_wnd(), IDC_EDIT_CAT_CREDIT_TMPL_NAME, value);
			}
			SetFocus(uGetDlgItem(get_wnd(), IDC_EDIT_CAT_CREDIT_TMPL_NAME));
			return 0;
		case WM_COMMAND:
			if (HIWORD(wParam) == BN_CLICKED) {
				if (LOWORD(wParam) == IDOK) {
					end_dialog(1);
				}
				else if (LOWORD(wParam) == IDCANCEL) {
					end_dialog(0);
				}
			}
			else if (HIWORD(wParam) == EN_CHANGE) {
				uGetDlgItemText(get_wnd(), IDC_EDIT_CAT_CREDIT_TMPL_NAME, value);
			}
			return 0;
		}
		return 0;
	}

public:
	int query(HWND parent, const char* defValue = "") {
		value = defValue;
#pragma warning(suppress:4996)
		return run(IDD_CONFIG_NAME, parent);  // NOLINT
	}

	pfc::string8 value;
};

//
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
//

class CTagCreditDialog : public MyCDialogImpl<CTagCreditDialog>,
	public CDialogResize<CTagCreditDialog>,
	public CMessageFilter
{

public:

	credit_tag_nfo m_ctag;

private:

	CListControlCredits m_credit_list;
	CListControlCredit_Selection m_credit_sel_list;

	vppair vcats;
	vppair vsubcats;
	vppair vcredits;
	vppair vdefs;

	std::vector<pfc::string8> vbuildcat;
	std::vector<pfc::string8> vbuildsubcat;
	std::vector<pfc::string8> vbuildfrom;
	std::vector<pfc::string8> vbuildgroup;

	int last_template_selection = 0;
	bool m_rebuilding_tag_name = false;

	//HWND tag_list;
	HWND remove_button;

	foo_conf conf;

	metadb_handle_list m_items;
	file_info_manager_ptr m_pfinfo_manager;
	
	pfc::string8 m_release_id;
	TagWriter_ptr m_tag_writer;
	tag_mapping_list_type* m_ptag_mappings = nullptr;
	
	bool generating_tags = false;

	pfc::string8 m_current_titleformat;
	
	UINT default_button;
	CEditWithButtonsDim cewb_highlight;

	pfc::string8 m_highlight_label;
	Release_ptr m_preview_release;

	bool update_current_template(int sel = -1);
	void applymappings();

	void show_context_menu(CPoint& pt, int selection);

	std::function<bool(HWND wndlist, bool updatedata)>stdf_credit_info_change_notifier =
		[this](HWND x, bool updatedata) -> bool {
		updateTagCreditUI(x, updatedata);
		return true; };

public:

	enum {
		IDD = IDD_DIALOG_TAG_MAPPINGS_CREDITS,
	};

	virtual BOOL PreTranslateMessage(MSG* pMsg) override {

		if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
		{
			HWND focused = ::GetFocus();
			int idCtrl = ::GetDlgCtrlID(focused);
			if (idCtrl == IDC_EDIT_TAG_MATCH_HL)
			{
				return TRUE;
			}
		}
		return ::IsDialogMessage(m_hWnd, pMsg);
	}

	vppair GetCredits() { return vcredits; }
	pfc::string8 GetHighlightHint() { return m_highlight_label; }

	CListControlCredits* GetCreditList() { return &m_credit_list; }
	
	void set_preview_release(Release_ptr preview_release) {
		m_preview_release = preview_release;
	}

	void cb_add_credit();
	void cb_generate_tags();

	void load_size();
	bool build_current_cfg();
	void pushcfg(bool force);

	void updateTagCreditUI(HWND list, bool remake_list);

	MY_BEGIN_MSG_MAP(CTagCreditDialog)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDC_APPLY, OnApply)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_ID_HANDLER(IDC_TAG_CREDITS_EDIT_HL, OnEditHLText)

		COMMAND_ID_HANDLER(IDC_TAG_CREDITS_BTN_ADD_HEADING, OnBtnAddHeadings)
		COMMAND_ID_HANDLER(IDC_TAG_CREDITS_BTN_ADD_SUBHEADING, OnBtnAddHeadings)
		COMMAND_ID_HANDLER(IDC_TAG_CREDITS_BTN_SCROLL_HEADING, OnBtnAddHeadings)
		COMMAND_ID_HANDLER(IDC_TAG_CREDITS_BTN_SCROLL_SUBHEADING, OnBtnAddHeadings)
		COMMAND_ID_HANDLER(IDC_TAG_CREDITS_RADIO_ALL, OnBtnRadioSource)
		COMMAND_ID_HANDLER(IDC_TAG_CREDITS_RADIO_RELEASE, OnBtnRadioSource)
		COMMAND_ID_HANDLER(IDC_TAG_CREDITS_RADIO_TRACKS, OnBtnRadioSource)
		COMMAND_ID_HANDLER(IDC_TAG_CREDITS_GRP_GXP, OnBtnCheckGroup)
		COMMAND_ID_HANDLER(IDC_TAG_CREDITS_GRP_GXC, OnBtnCheckGroup)
		COMMAND_ID_HANDLER(IDC_TAG_CREDITS_CMB_DEFS, OnCmbDefinitionsSelection)

		COMMAND_ID_HANDLER(IDC_BTN_TAG_CREDIT_SPLIT_ADD, OnBtnAddNew)
		COMMAND_ID_HANDLER(IDC_TAG_CREDITS_BTN_REMOVE, OnBtnRemove)
		COMMAND_ID_HANDLER(IDC_TAG_CREDITS_BTN_PREVIEW, OnPreview)
		NOTIFY_HANDLER_EX(IDC_BTN_TAG_CREDIT_SPLIT_ADD, BCN_DROPDOWN, OnSplitDropDown)

		CHAIN_MSG_MAP(CDialogResize<CTagCreditDialog>)
		MY_END_MSG_MAP()

	BEGIN_DLGRESIZE_MAP(CTagCreditDialog)
		DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_APPLY, DLSZ_MOVE_X | DLSZ_MOVE_Y)

		DLGRESIZE_CONTROL(IDC_TAG_CREDIT_SEL_LIST, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_TAG_CREDITS_STATIC_SELECTION, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_TAG_CREDIT_LIST, DLSZ_SIZE_X)

		BEGIN_DLGRESIZE_GROUP()
			DLGRESIZE_CONTROL(IDC_TAG_CREDIT_SEL_LIST, DLSZ_SIZE_Y)
			DLGRESIZE_CONTROL(IDC_TAG_CREDITS_STATIC_HL, /*DLSZ_MOVE_X |*/ DLSZ_MOVE_Y)
			DLGRESIZE_CONTROL(IDC_TAG_CREDITS_STATIC_CREDITS, DLSZ_MOVE_Y)
			DLGRESIZE_CONTROL(IDC_TAG_CREDIT_LIST, DLSZ_SIZE_Y)
			DLGRESIZE_CONTROL(IDC_TAG_CREDITS_EDIT_HL, /*DLSZ_MOVE_X |*/ DLSZ_MOVE_Y)
		END_DLGRESIZE_GROUP()

		DLGRESIZE_CONTROL(IDC_TAG_CREDITS_STATIC_PREVIEW, DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_TAG_CREDITS_STATIC_PREVIEW_TF, DLSZ_SIZE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_TAG_CREDITS_PREVIEW, DLSZ_SIZE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_TAG_CREDITS_BTN_PREVIEW, DLSZ_MOVE_Y)

	END_DLGRESIZE_MAP()

	void DlgResize_UpdateLayout(int cxWidth, int cyHeight) {
		CDialogResize<CTagCreditDialog>::DlgResize_UpdateLayout(cxWidth, cyHeight);
	}

	CTagCreditDialog::CTagCreditDialog(HWND p_parent, UINT default_button = IDOK)
		:  default_button(default_button), cewb_highlight(L" Highlight Tag"),
		remove_button(nullptr), m_credit_list(this), m_credit_sel_list(this) {
	}

	CTagCreditDialog::~CTagCreditDialog() {

		g_discogs->tag_credit_dialog = nullptr;
		if (m_ptag_mappings != nullptr) {

			delete m_ptag_mappings;
			m_ptag_mappings = nullptr;
		}
	}

	LRESULT OnBtnRadioSource(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBtnAddHeadings(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnApply(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnEditHLText(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnPreview(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBtnAddNew(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBtnRename(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBtnRemove(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBtnDuplicate(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBtnCheckGroup(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCmbDefinitionsSelection(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnSplitDropDown(LPNMHDR lParam);

	//void refresh_item(int pos);
	void enable(bool v) override {}

};
