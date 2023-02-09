#include "stdafx.h"

#include "track_matching_dialog_presenter.h"
#include "track_matching_dialog.h"
#include "track_matching_ILO.h"

HWND get_ctx_lvlist(HWND hwnd, int ID) {

	if (!(ID == IDC_UI_LIST_DISCOGS || ID == IDC_UI_LIST_FILES || ID == IDC_UI_DC_ARTWORK_LIST || ID == IDC_UI_FILE_ARTWORK_LIST))
	{
		return nullptr;
	}

	return uGetDlgItem(hwnd, ID);
}

//IListControlOwnerDataSource overrides

size_t ILOD_track_matching::listGetItemCount(ctx_t ctx) {

	HWND wnd = ((TParent*)this)->m_hWnd;
	// ctx: calling object
	HWND lvlist = get_ctx_lvlist(wnd, ctx->GetDlgCtrlID());

	PFC_ASSERT(get_ctx_lvlist(wnd, ctx->GetDlgCtrlID()) != nullptr);
	
	coord_presenters* coord = ilo_get_coord();
	size_t citems = pfc_infinite;

	switch (ctx->GetDlgCtrlID()) {
	case IDC_UI_LIST_DISCOGS:
		citems = coord->GetDiscogsTrackUiLvSize();
		break;
	case IDC_UI_LIST_FILES:
		citems = coord->GetFileTrackUiLvSize();
		break;
	case IDC_UI_DC_ARTWORK_LIST:
		citems = coord->GetDiscogsArtRowLvSize();
		break;
	case IDC_UI_FILE_ARTWORK_LIST:
		citems = coord->GetFileArtRowLvSize();
		break;
	default:
		citems = 0;
	}
	return citems;
}

// fb2k lib - get subitems

pfc::string8 ILOD_track_matching::listGetSubItemText(ctx_t ctx, size_t item, size_t subItem) {

	pfc::string8 buffer = "";

	HWND wnd = ((TParent*)this)->m_hWnd;

	HWND lvlist = get_ctx_lvlist(wnd, ctx->GetDlgCtrlID());

	coord_presenters* coord = ilo_get_coord();

	multi_uartwork uart = *coord->GetUartwork();

	size_t res;

	switch (ctx->GetDlgCtrlID()) {
	case IDC_UI_LIST_DISCOGS: {

		CHeaderCtrl header = ctx->GetHeaderCtrl();
		auto cols = header.GetItemCount();
		if (cols == 3) {
			if (subItem == 2)
				return std::to_string(item).c_str();
		}

		track_it track_match;
		res = coord->GetDiscogsTrackUiAtLvPos(item, track_match);
		buffer = !subItem ? track_match->first.first : track_match->first.second;
		break;
	}
	case IDC_UI_LIST_FILES: {

		CHeaderCtrl header = ctx->GetHeaderCtrl();
		auto cols = header.GetItemCount();
		if (cols == 3) {
			if (subItem == 2)
				return std::to_string(item).c_str();
		}

		file_it file_match;
		res = coord->GetFileTrackUiAtLvPos(item, file_match);
		buffer = !subItem ? file_match->first.first : file_match->first.second;
		break;
	}
	case IDC_UI_DC_ARTWORK_LIST: {
		getimages_it track_match;
		size_t perm_pos = coord->GetTrackArtAtLvPos(item, track_match);
		size_t cAlbumArt = m_release->images.get_count();
		ndx_image_info_row_t image_info_row = *track_match; // (ndx_image, imageinfo);
		ndx_image_t ndx_image = image_info_row.first; // ((int)art_src::alb, pi);
		bool dc_isalbum = ndx_image.first == (int)art_src::alb;
		perm_pos = dc_isalbum ? perm_pos : perm_pos - cAlbumArt;

		switch (subItem) {
			case 0:
				buffer = dc_isalbum ? "R" : "A";
				break;
			case 1:
				buffer = track_match->first.second->type;
				break;
			case 2:
				buffer = PFC_string_formatter() << track_match->first.second->get_width() << " x " << track_match->first.second->get_height();
				break;
			// write
			case 3: {
				af att = dc_isalbum ? af::alb_sd : af::art_sd;
				bool bflag = uart.getflag(att, perm_pos);
				pfc::string8 str(bflag ? /*bTile ? "Write" :*/ "x" : "");
				buffer = str;			
				break;
			}
			// overwrite
			case 4: {
				af att = dc_isalbum ? af::alb_ovr : af::art_ovr;
				bool bflag = uart.getflag(att, perm_pos);
				pfc::string8 str(bflag ? /*bTile ? "Write" :*/ "x" : "");
				buffer = str;
				break;
			}
			// embed
			case 5: {
				af att = dc_isalbum ? af::alb_emb : af::art_emb;
				bool bflag = uart.getflag(att, perm_pos);
				pfc::string8 str(bflag ? /*bTile ? "Write" :*/ "x" : "");
				buffer = str;
				break;
			}
			case 6:
				buffer = std::to_string(item).c_str();
				break;
		}
		break;
	}
	case IDC_UI_FILE_ARTWORK_LIST: {
		getimages_file_it track_match;
		size_t res = coord->GetFileArtAtLvPos(item, track_match);
		buffer = !subItem ? "0 fl": "fl subitem" ;

		size_t pos = item;

		getimages_file_it imagefilerow;
		size_t ndx_pos = coord->GetLvFileArtRow(item, imagefilerow);

		if (ndx_pos == pfc_infinite) return 0;

		switch (subItem) {
		//file name
		case 0: {
			pfc::string8 str(imagefilerow->second.at(0));
			pfc::string8 fullpath = pfc::string_filename_ext(str);
			buffer = fullpath;
			break;
		}
		//dims
		case 1: {
			pfc::string8 str(imagefilerow->second.at(1));
			buffer = str;

			break;
		}
		//file size
		case 2: {
			pfc::string8 str(imagefilerow->second.at(2));
			buffer = str;
			break;
		}
		//position
		case 3: {
			pfc::string8 str = std::to_string(item).c_str();
			buffer = str;
			break;
		}
		default: {
			//..
		}
		}

		break;
	}
	default:
		buffer = "na";
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
	HWND dbg = ctx->m_hWnd;
	HWND hlist = uGetDlgItem(wnd, ctx->GetDlgCtrlID());
	
	coord_presenters* coord = ilo_get_coord();
	lsmode mode = coord->GetCtrIDMode(ctx->GetDlgCtrlID());

	size_t count = ctx->GetItemCount();
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

	size_t nextfocus = coord->ListUserCmd(hlist, mode, ID_REMOVE, delmask, pfc::bit_array_bittable(), pfc::array_t<size_t>(), false);

	((TParent*)this)->match_message_update(match_manual);
	return true;
}

// fb2k lib - action, edit, clicked - not implemented

void ILOD_track_matching::listItemAction(ctx_t ctx, size_t item) {

	switch (ctx->GetDlgCtrlID()) {
	case IDC_UI_LIST_DISCOGS: {
		break;
	}
	case IDC_UI_LIST_FILES: {
		break;
	}
	case IDC_UI_DC_ARTWORK_LIST: {
		coord_presenters* coord = ilo_get_coord();
		CPoint curpos;
		GetCursorPos(&curpos);
		::ScreenToClient(ctx->m_hWnd, &curpos);

		CHeaderCtrl header = ctx->GetHeaderCtrl();
		CRect rc; header.GetClientRect(rc);
		curpos.y = rc.bottom / 2;
		HDHITTESTINFO lptest = { 0 };
		lptest.pt = curpos;
		header.HitTest(&lptest);


		//calculate subItem by hittest
		int subItem = 9999;
		if (lptest.flags == HHT_ONHEADER) {
			subItem = lptest.iItem;
		}

		//
		bool battrib = subItem == 3 || subItem == 4 || subItem == 5;
		af att;
		bool bval, dc_isalbum = false;		
		multi_uartwork* multi_uart = nullptr;
		size_t perm_pos;

		if (battrib) {
			multi_uart = coord->GetUartwork();
			getimages_it track_match;
			perm_pos = coord->GetTrackArtAtLvPos(item, track_match);
			size_t cAlbumArt = m_release->images.get_count();
			ndx_image_info_row_t image_info_row = *track_match; // (ndx_image, imageinfo);
			ndx_image_t ndx_image = image_info_row.first; // ((int)art_src::alb, pi);
			dc_isalbum = ndx_image.first == (int)art_src::alb;
			perm_pos = dc_isalbum ? perm_pos : perm_pos - cAlbumArt;
		}
		//

		switch (subItem) {
		case 0:
			coord->SetModeTile(!coord->isTile());
			break;
		case 3:
			att = dc_isalbum ? af::alb_sd : af::art_sd;
			break;
		case 4:
			att = dc_isalbum ? af::alb_ovr : af::art_ovr;
			break;

		case 5:
			att = dc_isalbum ? af::alb_emb : af::art_emb;
			break;
		}

		if (battrib) {

			bval = multi_uart->getflag(att, perm_pos);
			bval = !bval;
			multi_uart->setflag(att, perm_pos, bval);
			CListControlOwnerData* uilist = (CListControlOwnerData*)ctx;
			uilist->UpdateItem(item);
			uSendMessage(((TParent*)this)->m_hWnd, WM_CUST_UPDATE_SKIP_BUTTON, 0, 0);
		}
		break;
	}
	case IDC_UI_FILE_ARTWORK_LIST: {
		coord_presenters* coord = ilo_get_coord();
		getimages_file_it track_match;
		size_t res = coord->GetFileArtAtLvPos(item, track_match);

		size_t pos = item;

		getimages_file_it fit;
		size_t ndx_pos = coord->GetLvFileArtRow(item, fit);
		if (ndx_pos == pfc_infinite) return;
		ndx_image_file_t ndx_img = fit->first;
		service_ptr_t<fb2k::imageViewer> img_viewer = fb2k::imageViewer::get();
		const GUID aaType = ndx_img.second;


		//image viewer req >= 1.6.2
		bool has_viewer = core_version_info_v2::get()->test_version(1, 6, 2, 0);
		if (has_viewer) {
			auto items = ((TParent*)this)->get_tag_writer_items();
			img_viewer->load_and_show(ctx->m_hWnd, items, aaType/*ndx_img.second*/, 0);
		}
		break;
		}
		default: {
			int debug = 1;
		}
	}
}

void ILOD_track_matching::listSubItemClicked(ctx_t ctx, size_t item, size_t subItem) {
	//..
	return;
}
void ILOD_track_matching::listSetEditField(ctx_t ctx, size_t item, size_t subItem, const char* val) {
	//..
}
bool ILOD_track_matching::listIsColumnEditable(ctx_t, size_t subItem) {
	return subItem > 2 && subItem < 6;
}
