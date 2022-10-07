#pragma once
#include "libPPUI\CListControlOwnerData.h"

class CPreviewLeadingTagDialog;

class ILOD_preview_leading : public IListControlOwnerDataSource {

public:

	// constructor

	ILOD_preview_leading(tag_result_ptr item_result, PreView view_mode)
		: m_ptag_result(item_result), m_viewmode(view_mode) {
		//..
	}

	void PullDlgResult() {

		m_ptag_result = ilo_get_item_result();
	}

	void SetMode(PreView view_mode);

	//serves preview modal context menu
	void TriggerAction();

	void CopyFromOriginal();

private:
	
	// local, can diverge from preview dlg mode
	PreView get_mode();

	// pure virtual
	virtual PreView ilo_get_view_mode() = 0;
	virtual tag_result_ptr ilo_get_item_result() = 0;
	virtual CListControlOwnerData* ilo_get_ui_list() = 0;
	virtual const std::vector<pfc::string8>& ilo_get_vtracks_desc() = 0;
	virtual const size_t ilo_get_finfo_count() = 0;

	// IListControlOwnerDataSource overrides

	//get item count
	size_t listGetItemCount(ctx_t ctx) override;

	//get subitems
	pfc::string8 listGetSubItemText(ctx_t ctx, size_t item, size_t subItem) override;

	//action, editable, clicked, edit, get/set
	void listItemAction(ctx_t, size_t item) override;
	bool listIsColumnEditable(ctx_t, size_t subItem) override;
	void listSubItemClicked(ctx_t, size_t item, size_t subItem) override;
	void listSetEditField(ctx_t ctx, size_t item, size_t subItem, const char* val) override;
	pfc::string8 listGetEditField(ctx_t ctx, size_t item, size_t subItem, size_t& lineCount) override;

	PreView m_viewmode;
	tag_result_ptr m_ptag_result;
};
