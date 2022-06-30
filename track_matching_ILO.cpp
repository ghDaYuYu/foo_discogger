#include "stdafx.h"

#include "track_matching_dialog_presenter.h"
#include "track_matching_dialog.h"
#include "track_matching_ILO.h"

HWND get_ctx_lvlist(HWND hwnd, int ID) {

	if (!(ID == IDC_UI_LIST_DISCOGS || ID == IDC_UI_LIST_FILES))
	{
		return nullptr;
	}

	return uGetDlgItem(hwnd, ID);
}

//libPPUI IListControlOwnerDataSource overrides

size_t ILOD_track_matching::listGetItemCount(ctx_t ctx) {

	size_t citems = pfc_infinite;
	HWND wnd = ((TParent*)this)->m_hWnd;

	// ctx: calling object
	PFC_ASSERT(ctx == ilo_get_idc_list() || ctx == ilo_get_ifile_list());
	HWND lvlist = get_ctx_lvlist(wnd, ctx->GetDlgCtrlID());

	coord_presenters* coord = ilo_get_coord();

	citems = ctx->GetDlgCtrlID() == IDC_UI_LIST_DISCOGS ?
		coord->GetDiscogsTrackUiLvSize() : coord->GetFileTrackUiLvSize();

	return citems;
}

// fb2k lib - get subitems

pfc::string8 ILOD_track_matching::listGetSubItemText(ctx_t ctx, size_t item, size_t subItem) {

	pfc::string8 buffer = "";
	HWND wnd = ((TParent*)this)->m_hWnd;
	HWND lvlist = get_ctx_lvlist(wnd, ctx->GetDlgCtrlID());

	coord_presenters* coord = ilo_get_coord();

	if (ctx->GetDlgCtrlID() == IDC_UI_LIST_FILES) {
		file_it file_match;
		size_t res = coord->GetFileTrackUiAtLvPos(item, file_match);
		buffer = !subItem ? file_match->first.first : file_match->first.second;
	}
	else {
		track_it track_match;
		size_t res = coord->GetDiscogsTrackUiAtLvPos(item, track_match);
		buffer = !subItem ? track_match->first.first : track_match->first.second;
	}
	return buffer;
}

// fb2k lib - can reorder

bool ILOD_track_matching::listCanReorderItems(ctx_t) {

	return true;
}

// fb2k lib - reorder

bool ILOD_track_matching::listReorderItems(ctx_t ctx, const size_t* order, size_t count) {

	HWND wnd = ((TParent*)this)->m_hWnd;

	int listid = ctx->GetDlgCtrlID();
	HWND hlist = uGetDlgItem(wnd, listid);

	ilo_get_coord()->reorder_map_elements(hlist, order, count, lsmode::tracks_ui);
	((TParent*)this)->match_message_update(match_manual);

	return true;
}

// fb2k lib - remove

bool ILOD_track_matching::listRemoveItems(ctx_t ctx, pfc::bit_array const& mask) {

	HWND wnd = ((TParent*)this)->m_hWnd;

	int listid = ctx->GetDlgCtrlID();
	HWND hlist = uGetDlgItem(wnd, listid);

	coord_presenters* coord = ilo_get_coord();

	size_t count = listid == IDC_UI_LIST_DISCOGS ?
		coord->GetDiscogsTrackUiLvSize() : coord->GetFileTrackUiLvSize();

	bit_array_bittable delmask;
	delmask.resize(count);

	t_size n, total = 0;
	n = total = mask.find(true, 0, count);

	if (n < count) {

		delmask.set(n, true);

		for (n = mask.find(true, n + 1, count - n - 1);
			n < count; n = mask.find(true, n + 1, count - n - 1))
		{
			delmask.set(n, true);
		}
	}
	else {
	    return true;
    }

	coord->ListUserCmd(hlist, lsmode::tracks_ui, ID_REMOVE, delmask, pfc::bit_array_bittable(), false);

	((TParent*)this)->match_message_update(match_manual);

	return true;
}

// fb2k lib - action, edit, clicked - not implemented

void ILOD_track_matching::listItemAction(ctx_t, size_t item) {
	//..
}
void ILOD_track_matching::listSubItemClicked(ctx_t, size_t item, size_t subItem) {
	//..
}
void ILOD_track_matching::listSetEditField(ctx_t ctx, size_t item, size_t subItem, const char* val) {
	//..
}
bool ILOD_track_matching::listIsColumnEditable(ctx_t, size_t subItem) {
	return false;
}