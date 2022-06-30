#pragma once
#include "libPPUI\CListControlOwnerData.h"

class CPreviewModalTagDialog;

inline const size_t LINES_LONGFIELD = 4;
inline const size_t CHARS_SHORTFIELD = 50;

class ILOD_preview_modal : public IListControlOwnerDataSource {

private:

	PreView m_view_mode;
	tag_result_ptr m_item_result;

public:

	typedef CPreviewModalTagDialog TParent;

	// constructor (item results, view_mode)

	ILOD_preview_modal(tag_result_ptr item_result, PreView view_mode)
		: m_item_result(item_result), m_view_mode(view_mode) {
		//..
	}

	void ReloadItem() {

		m_item_result = ilo_get_item_result();
	}

	void set_mode(PreView view_mode);

	//serves preview modal context menu
	void trigger_action();

	//virtual void listItemAction(ctx_t, size_t) override;

private:
	
	// local, can diverge from preview dlg mode
	PreView get_mode();

	// pure virtual
	virtual PreView ilo_get_view_mode() = 0;
	virtual tag_result_ptr ilo_get_item_result() = 0;
	virtual CListControlOwnerData* ilo_get_ui_list() = 0;
	virtual const std::vector<pfc::string8>& ilo_get_vtracks_desc() = 0;
	virtual const size_t ilo_get_finfo_count() = 0;

	//libPPUI IListControlOwnerDataSource overrides

	//get item count
	size_t listGetItemCount(ctx_t ctx) override;

	//get subitems
	pfc::string8 listGetSubItemText(ctx_t ctx, size_t item, size_t subItem) override;

	//can reorder
	bool listCanReorderItems(ctx_t) override;

	//reorder
	bool listReorderItems(ctx_t ctx, const size_t* order, size_t count) override;

	//remove
	bool listRemoveItems(ctx_t ctx, pfc::bit_array const& mask) override;

	//action, editable, clicked, edit, get/set
	void listItemAction(ctx_t, size_t item) override;
	bool listIsColumnEditable(ctx_t, size_t subItem) override;
	void listSubItemClicked(ctx_t, size_t item, size_t subItem) override;
	void listSetEditField(ctx_t ctx, size_t item, size_t subItem, const char* val) override;
	pfc::string8 listGetEditField(ctx_t ctx, size_t item, size_t subItem, size_t& lineCount) override;
};
