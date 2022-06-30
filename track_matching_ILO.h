#pragma once
#include "libPPUI\CListControlOwnerData.h"

class CTrackMatchingDialog;
class coord_presenters;

class ILOD_track_matching : public IListControlOwnerDataSource {

public:

	typedef CTrackMatchingDialog TParent;
	ILOD_track_matching() {
	    //..
	}

private:

	// pure virtual
	virtual coord_presenters* ilo_get_coord() = 0;	
	virtual CListControlOwnerData* ilo_get_idc_list() = 0;
	virtual CListControlOwnerData* ilo_get_ifile_list() = 0;

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

	//action, edit, clicked - not implemented

	void listItemAction(ctx_t, size_t item) override;
	void listSubItemClicked(ctx_t, size_t item, size_t subItem) override;
	void listSetEditField(ctx_t ctx, size_t item, size_t subItem, const char* val) override;
	bool listIsColumnEditable(ctx_t, size_t subItem) override;

};