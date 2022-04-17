#pragma once
#include "libPPUI\CListControlOwnerData.h"

class CPreviewModalTagDialog;

#define LINES_LONGFIELD 4;

class ILOD_preview_modal : public IListControlOwnerDataSource {

private:

	CListControlOwnerData* m_ui_list;

	int m_idd_list;
	int m_view_mode;

	tag_result_ptr m_item_result;
	//todo: rev
	const std::vector<pfc::string8>* m_vtracks_p;

public:

	typedef CPreviewModalTagDialog TParent;

	// constructor (uilist, item results, vtrack desc, IID_LIST, view_mode)

	ILOD_preview_modal(CListControlOwnerData* ui_list, tag_result_ptr item_result, const std::vector<pfc::string8> * vtracks_desc_p, int IDD_LIST, int view_mode)
		: m_ui_list(ui_list), m_item_result(item_result), m_vtracks_p(vtracks_desc_p), m_idd_list(IDD_LIST), m_view_mode(view_mode) {
	
		//..
	}

	void set_mode(int view_mode);

private:
	
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

	//action, edit, clicked - not implemented

	void listItemAction(ctx_t, size_t item) override;
	void listSubItemClicked(ctx_t, size_t item, size_t subItem) override;
	void listSetEditField(ctx_t ctx, size_t item, size_t subItem, const char* val) override;
	pfc::string8 listGetEditField(ctx_t ctx, size_t item, size_t subItem, size_t& lineCount) override;
	bool listIsColumnEditable(ctx_t, size_t subItem) override;

	//..

	int get_mode();
};
