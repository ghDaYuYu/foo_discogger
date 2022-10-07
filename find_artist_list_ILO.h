#pragma once
#include "libPPUI\CListControlOwnerData.h"

class CArtistList;

class ILOD_artist_list : public IListControlOwnerDataSource {

public:

	typedef CArtistList TParent;

	ILOD_artist_list() {
		//..
	}
private:

	CArtistList* get_uilist();

	// ILO pure virtual
	virtual CListControlOwnerData* ilo_get_uilist() = 0;

	// IListControlOwnerDataSource overrides

	size_t listGetItemCount(ctx_t ctx) override;
	pfc::string8 listGetSubItemText(ctx_t, size_t item, size_t subItem) override;
	void listSelChanged(ctx_t) override;
	void listItemAction(ctx_t, size_t) override;
};