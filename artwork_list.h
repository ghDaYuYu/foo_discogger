#pragma once

#include "libPPUI/CListControlOwnerData.h"

class CArtworkList : public CListControlOwnerData {

public:

	CArtworkList(IListControlOwnerDataSource* ilo);

	~CArtworkList() {
		//..
	}

	// CListControlOwnerData overrides

	int GetItemHeight() const override;
	bool IsSubItemGrayed(size_t item, size_t subItem) override;
	bool RenderCellImageTest(size_t item, size_t subItem) const override;
	void RenderCellImage(size_t item, size_t subItem, CDCHandle, const CRect&) const override;

private:

};
