#include "libPPUI/CListControlOwnerData.h"

class CPreviewList : public CListControlOwnerData {

public:

	CPreviewList(IListControlOwnerDataSource* ilo);

	~CPreviewList() {
		DeleteObject(m_hImageList);
	}
	void Inititalize() {

		set_image_list();

		ListView_SetExtendedListViewStyleEx(m_hWnd,
			LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_LABELTIP,
			LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_LABELTIP);

		CRect rc;
		::GetClientRect(m_hWnd, &rc);

		AddColumnEx("Artist", rc.Width(), LVCFMT_LEFT, true);
	}

	// DEFAULT ACTIONS (nVKReturn)

	void Default_Action();

	//..

	bool IsSubItemGrayed(size_t item, size_t subItem) override;

#pragma warning( push )
#pragma warning( disable : 26454 )
	typedef CListControlOwnerData TParent;
	BEGIN_MSG_MAP(CPreviewList)
		CHAIN_MSG_MAP(TParent)
	END_MSG_MAP()

#pragma warning(pop)

private:

	void context_menu(size_t list_index, POINT screen_pos);
	void set_image_list();
	CImageList m_hImageList;

	std::function<bool(int lparam)>stdf_on_artist_selected_notifier;
	std::function<bool()>stdf_on_ok_notifier;
};
