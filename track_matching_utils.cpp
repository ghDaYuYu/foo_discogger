#include "stdafx.h"

#include <GdiPlus.h>

#pragma comment(lib, "gdiplus.lib")

#include "ol_cache.h"
#include "track_matching_utils.h"

using namespace Gdiplus;

pfc::string8 duration_to_str(int seconds) {
	int hours = seconds / 3600;
	seconds %= 3600;
	int minutes = seconds / 60;
	seconds = seconds % 60;
	pfc::string8 result = "";
	if (hours) {
		result << hours << ":";
	}
	if (hours && minutes < 10) {
		result << "0";
	}
	result << minutes << ":";
	if (seconds < 10) {
		result << "0";
	}
	result << seconds;
	return result;
}

size_t get_extended_style(HWND list) {
	DWORD dwStyle = ::SendMessage(list,	LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
	return static_cast<size_t>(dwStyle);
}

std::vector<int> build_woas(HWND list) {

	CHeaderCtrl header_ctrl = ListView_GetHeader(list);
	
	if (!header_ctrl) return std::vector<int>{};

	std::vector<int> v_woa;

	DWORD dwStyle = ListView_GetExtendedListViewStyle(list);
	DWORD dwView = ListView_GetView(list);
	
	if (dwView == ID_ART_TILE) return std::vector<int>();

	int cCol = ListView_GetColumnCount(list);

	if (cCol == 0) 	return v_woa;

	std::vector<int> vorder; vorder.resize(cCol);
	ListView_GetColumnOrderArray(list, cCol, &vorder[0]);

	v_woa.resize(cCol);

	for (auto & it_woa = v_woa.begin(); it_woa != v_woa.end(); ++it_woa) {

		int index = std::distance(v_woa.begin(), it_woa);
		int width = ListView_GetColumnWidth(list, index);

		HDITEM headerItem = { 0 };
		headerItem.mask = HDI_FORMAT | HDI_WIDTH;
		//CHeaderCtrl header = ListView_GetHeader(list);
		header_ctrl.GetItem(index, &headerItem);
		int justify_mask = (headerItem.fmt & LVCFMT_JUSTIFYMASK);
		int lwoa = justify_mask * 10 + vorder[index];
		int hwoa = width; // headerItem.cxy;
		*it_woa = MAKELPARAM(lwoa, hwoa);
		//});
	}
	return v_woa;
}

std::vector<int> build_woas_libppui(CListControlOwnerData* ui_list) {

	std::vector<int> vw_res = {};
	CHeaderCtrl header_ctrl = ui_list->GetHeaderCtrl();

	//header not available
	if (!header_ctrl) return vw_res;

	int ccol = ui_list->GetColumnCount();
	if (!ccol) 	return vw_res;

	vw_res.resize(ccol);

	//columns order
	std::vector<int> v_order;
	v_order.resize(ccol);
	header_ctrl.GetOrderArray(ccol, &v_order[0]);

	//justification
	for (auto it_woa = vw_res.begin(); it_woa != vw_res.end(); ++it_woa) {
		int index = std::distance(vw_res.begin(), it_woa);

		HDITEM headerItem = { 0 }; headerItem.mask = HDI_FORMAT /*| HDI_WIDTH*/;
		header_ctrl.GetItem(index, &headerItem);

		int justify_masked = (headerItem.fmt & LVCFMT_JUSTIFYMASK);
		int lwoa = justify_masked * 10 + v_order[index];
		int hwoa = ui_list->GetColumnWidthF(index);

		//store woa
		*it_woa = MAKELPARAM(lwoa, hwoa);
	}
	return vw_res;
}

bool isPrimary(pfc::string8 type) {
	return STR_EQUAL(type, "primary") || STR_EQUAL(type, "secondary+");
}

//for cases when discogs artwork image list lacks a primary item
bool fixPrimary(pfc::string8& type) {
	if (STR_EQUAL(type, "secondary") && !isPrimary(type)) {
		type << "+";
		return true;
	}
	return false;
}

//human readable file sizes

double roundOff(double n) {
	double d = n * 100.0;
	int i = d + 0.5;
	d = (float)i / 100.0;
	return d;
}

pfc::string8 round_file_size_units(size_t size) {
	static const char* SIZES[] = { "B", "KB", "MB", "GB" };
	int div = 0;
	size_t rem = 0;

	while (size >= 1024 && div < (sizeof SIZES / sizeof * SIZES)) {
		rem = (size % 1024);
		div++;
		size /= 1024;
	}

	double size_d = (float)size + (float)rem / 1024.0;
	size_d = roundOff(size_d);
	
	char cftos[10 + 1];
	pfc::float_to_string(cftos, 10, size_d, 0, false);
	pfc::string8 result(cftos, 10);
	result << " " << SIZES[div];
	return result;
}

//..

namespace listview_helper {

	unsigned insert_column(HWND p_listview, unsigned p_index, const char* p_name, unsigned p_width_dlu, int fmt)
	{
		pfc::stringcvt::string_os_from_utf8 os_string_temp(p_name);
		RECT rect = { 0, 0, static_cast<LONG>(p_width_dlu), 0 };
		MapDialogRect(GetParent(p_listview), &rect);
		LVCOLUMN data = {};
		data.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
		data.fmt = fmt;
		data.cx = rect.right;
		data.pszText = const_cast<TCHAR*>(os_string_temp.get_ptr());

		LRESULT ret = uSendMessage(p_listview, LVM_INSERTCOLUMN, p_index, (LPARAM)&data);
		if (ret < 0) return ~0;
		else return (unsigned)ret;
	}
}

namespace tileview_helper {

	void tv_insert_column(HWND hlist, int icol, pfc::string8 heading, int format,
		int width, int subitem) {

		//pfc::stringcvt::string_os_from_utf8 labelOS(heading);

		LV_COLUMN lvcol = {};

		TCHAR outBuffer[MAX_PATH + 1] = {};
		pfc::stringcvt::convert_utf8_to_wide(outBuffer, MAX_PATH,
			heading.get_ptr(), heading.get_length());
		_tcscpy_s(lvcol.pszText, lvcol.cchTextMax, const_cast<TCHAR*>(outBuffer));

		lvcol.cx = width;
		lvcol.fmt = subitem > 1 ? LVCFMT_CENTER : LVCFMT_LEFT;
		lvcol.iSubItem = subitem;
		lvcol.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

		//lvcol.pszText = const_cast<TCHAR*>(labelOS.get_ptr());

		ListView_InsertColumn(hlist, icol, &lvcol);
	}

	void tv_insert_item_with_image(HWND hlist, int iItem, pfc::string8 strItem, int iImage, LPARAM param) {

		//pfc::stringcvt::string_os_from_utf8 labelOS(strItem);
		//int fmtColumns[6] = { LVCFMT_TILE_PLACEMENTMASK, LVCFMT_TILE_PLACEMENTMASK, LVCFMT_TILE_PLACEMENTMASK, LVCFMT_TILE_PLACEMENTMASK, LVCFMT_TILE_PLACEMENTMASK, LVCFMT_TILE_PLACEMENTMASK };
		//int fmtColumns[1] = { LVCFMT_LINE_BREAK/*, LVCFMT_WRAP*/ };
		
		LV_ITEM lvitem = {};
		lvitem.iItem = iItem;
		lvitem.lParam = param;
		lvitem.iImage = iImage;
		lvitem.pszText = LPSTR_TEXTCALLBACK; // const_cast<TCHAR*>(labelOS.get_ptr());
		lvitem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM /*| LVIF_COLFMT*/;

		//lvitem.piColFmt = fmtColumns;

		ListView_InsertItem(hlist, &lvitem);
	}

	//puColums = select columns(lines) and order, cColumns columns(lines) selected
	BOOL tv_set_item_tile_lines(HWND hlist, int iItem, UINT* arrColumns, int nLines) {

		//int fmtColumns[6] = { LVCFMT_TILE_PLACEMENTMASK, LVCFMT_TILE_PLACEMENTMASK, LVCFMT_TILE_PLACEMENTMASK, LVCFMT_TILE_PLACEMENTMASK, LVCFMT_TILE_PLACEMENTMASK, LVCFMT_TILE_PLACEMENTMASK };

		LVTILEINFO lvti = { 0 };
		lvti.cbSize = sizeof(LVTILEINFO);
		lvti.cColumns = nLines;
		lvti.iItem = iItem;
		lvti.puColumns = arrColumns;

		//lvti.piColFmt = /*(UINT *)*/ fmtColumns;// /*PUINT({ LVCFMT_LEFT, LVCFMT_BITMAP_ON_RIGHT,*/ LVCFMT_TILE_PLACEMENTMASK/*})*/;

		return ListView_SetTileInfo(hlist, &lvti);
	}

	//tile view settings...
	//lines (LVTVIM_COLUMNS) for each item
	BOOL tv_tileview_line_count(HWND hlist, int nLines) {

		LVTILEVIEWINFO lvtvwi = { 0 };
		lvtvwi.cbSize = sizeof(LVTILEVIEWINFO);
		lvtvwi.dwMask = LVTVIM_COLUMNS /*| LVTVIM_TILESIZE*/;

		//tvi.dwFlags = LVTVIF_FIXEDSIZE;
		//SIZE tilesize = { 100, 100 };
		//lvtvwi.sizeTile = tilesize;

		lvtvwi.cLines = nLines;

		return ListView_SetTileViewInfo(hlist, &lvtvwi);
	}

	BOOL tv_set_tileview_tile_fixed_width(HWND hlist, int nWidth) {
		LVTILEVIEWINFO lvtvwi = { 0 };
		lvtvwi.cbSize = sizeof(LVTILEVIEWINFO);
		lvtvwi.dwMask = LVTVIM_TILESIZE;

		lvtvwi.dwFlags = LVTVIF_FIXEDWIDTH /*| LVTVIF_EXTENDED*/;
		lvtvwi.sizeTile.cx = nWidth;
		lvtvwi.sizeTile.cy = 0;

		return ListView_SetTileViewInfo(hlist, &lvtvwi);
	}
}

//generate cache thumbnails files and hbitmaps from artwork preview memoryblock
std::pair<HBITMAP, HBITMAP> MemoryBlockToTmpBitmap(std::pair<pfc::string8, pfc::string8> cache_path, MemoryBlock small_art) {

	if (!small_art.get_count()) return std::pair(nullptr, nullptr);

	pfc::string8 temp_file_small(cache_path.first);
	extract_native_path(temp_file_small, temp_file_small);
	pfc::string8 temp_file_mini(cache_path.second);
	extract_native_path(temp_file_mini, temp_file_mini);

	std::filesystem::path os_file_small = std::filesystem::u8path(temp_file_small.c_str());
	std::filesystem::path os_file_mini = std::filesystem::u8path(temp_file_mini.c_str());


	FILE* fd = nullptr;
	if (fopen_s(&fd, os_file_small.string().c_str(), "wb") != 0) {
		pfc::string8 msg("can't open ");
		msg << temp_file_small;
		log_msg(msg);
		return std::pair(nullptr, nullptr);
	}
	else {
		//write small thumb (aprox. 150x150)
		if (fwrite(small_art.get_ptr(), small_art.get_size(), 1, fd) != 1) {
			pfc::string8 msg("error writing ");
			msg << temp_file_small;
			log_msg(msg);
		}
	}
	if (fd != nullptr) {
		fclose(fd);
	}

	Bitmap bmSmall(os_file_small.wstring().c_str());

	HBITMAP hBmSmall, hBmMini;

	//Get hBmSmall (150x150 aprox)
	if (bmSmall.GetHBITMAP(Color(255, 255, 255)/*Color::Black*/, &hBmSmall) == Gdiplus::Ok) {

		Gdiplus::Graphics gmini(/*(Image*)*/&bmSmall);
		Gdiplus::Status gdi_res = gmini.DrawImage(/*(Gdiplus::Image*)*/ &bmSmall, 0, 0);
		PFC_ASSERT(gdi_res == Gdiplus::Ok);

		int inWidth = bmSmall.GetWidth();
		int inHeight = bmSmall.GetHeight();
		int newWidth = 48;
		int newHeight = 48;

		float ScalingFactor;
		if (inWidth >= inHeight)
			ScalingFactor = (float)newWidth / (float)bmSmall.GetWidth();
		else
			ScalingFactor = (float)newHeight / (float)bmSmall.GetHeight();

		/*
			horizontalScalingFactor & horizontalScalingFactor works well,
			except if the original image has a specific resolution,
			for example if it was created by the reading of an image file.
			In this case you have to apply the resolution of the
			original image	to the scaling factors
		*/

		Gdiplus::Bitmap imgMini = Gdiplus::Bitmap((int)newWidth, (int)newHeight);

		Gdiplus::REAL oriResX = bmSmall.GetHorizontalResolution();
		Gdiplus::REAL oriResY = bmSmall.GetVerticalResolution();
		Gdiplus::REAL newResX = imgMini.GetHorizontalResolution();
		Gdiplus::REAL newResY = imgMini.GetVerticalResolution();

#ifdef DEBUG
		float debugx = (float)oriResX / (float)newResX;
		float debugy = (float)oriResY / (float)newResY;
#endif

		float horizontalScalingFactor = ScalingFactor * ((float)oriResX / (float)newResX);
		float verticalScalingFactor = ScalingFactor * ((float)oriResY / (float)newResY);

		Gdiplus::Graphics g(&imgMini);
		g.ScaleTransform(horizontalScalingFactor, verticalScalingFactor);

		gdi_res = g.DrawImage(/*(Gdiplus::Image*)*/ &bmSmall, 0, 0);
		PFC_ASSERT(gdi_res == Gdiplus::Ok);

		FILE* fd = nullptr;
		if (fopen_s(&fd, os_file_mini.string().c_str(), "wb") != 0) {
			pfc::string8 msg("can't open ");
			msg << temp_file_mini;
			log_msg(msg);
			return std::pair(nullptr, nullptr);
		}
		if (fd != nullptr)
			fclose(fd);

		/*
		bmp: {557cf400-1a04-11d3-9a73-0000f81ef32e}
		jpg: {557cf401-1a04-11d3-9a73-0000f81ef32e}
		gif: {557cf402-1a04-11d3-9a73-0000f81ef32e}
		tif: {557cf405-1a04-11d3-9a73-0000f81ef32e}
		png: {557cf406-1a04-11d3-9a73-0000f81ef32e}
		*/

		CLSID pngClsid;
		HRESULT hres = CLSIDFromString(L"{557cf406-1a04-11d3-9a73-0000f81ef32e}", &pngClsid);

		//write mini thumb
		imgMini.Save(os_file_mini.wstring().c_str(), &pngClsid, NULL);

		Bitmap bmMini(os_file_mini.wstring().c_str());

		//Get hBmMini (48x48)
		if (bmMini.GetHBITMAP(Color(255, 255, 255)/*Color::Black*/, &hBmMini) != Gdiplus::Ok) {
			log_msg("GdiPlus error (GetHBITMAP Small preview 48x48)");
		}
	}
	else {
		log_msg("GdiPlus error (GetHBITMAP Small preview)");
	}

	return std::pair(hBmSmall, hBmMini);
}

//generate windows %temp% thumbnails (150x150 and 48x48) bitmaps from local artwork
std::pair<HBITMAP, HBITMAP> GenerateTmpBitmapsFromRealSize(pfc::string8 release_id, size_t pos,
	pfc::string8 source_full_path, std::pair<pfc::string8, pfc::string8>& temp_file_names) {

	std::filesystem::path os_src_full_path = std::filesystem::u8path(source_full_path.c_str());

	Bitmap bmRealSize(os_src_full_path.wstring().c_str());

	int inWidth = bmRealSize.GetWidth();
	int inHeight = bmRealSize.GetHeight();
	int newWidth = 150;
	int newHeight = 150;

	float ScalingFactor;
	if (inWidth >= inHeight)
		ScalingFactor = (float)newWidth / (float)bmRealSize.GetWidth();
	else
		ScalingFactor = (float)newHeight / (float)bmRealSize.GetHeight();

	Gdiplus::Bitmap imgSmall = Gdiplus::Bitmap((int)newWidth, (int)newHeight);

	Gdiplus::REAL oriResX = bmRealSize.GetHorizontalResolution();
	Gdiplus::REAL oriResY = bmRealSize.GetVerticalResolution();
	Gdiplus::REAL newResX = imgSmall.GetHorizontalResolution();
	Gdiplus::REAL newResY = imgSmall.GetVerticalResolution();

#ifdef DEBUG
	float debugx = (float)oriResX / (float)newResX;
	float debugy = (float)oriResY / (float)newResY;
#endif

	float horizontalScalingFactor = ScalingFactor * (float)oriResX / (float)newResX;
	float verticalScalingFactor = ScalingFactor * (float)oriResY / (float)newResY;

	Gdiplus::Graphics g(&imgSmall);
	g.ScaleTransform(horizontalScalingFactor, verticalScalingFactor);

	if (g.DrawImage((Gdiplus::Image*) & bmRealSize, 0, 0) == Gdiplus::Ok) {
		int debug = 0;
	}
	pfc::string8 temp_path, temp_file_name_small, temp_file_name_mini;

	uGetTempPath(temp_path);
	uGetTempFileName(temp_path, "fb2k", 0, temp_file_name_small);

	std::filesystem::path os_tmp = std::filesystem::u8path(temp_file_name_small.c_str());

	/*
	bmp: {557cf400-1a04-11d3-9a73-0000f81ef32e}
	jpg: {557cf401-1a04-11d3-9a73-0000f81ef32e}
	gif: {557cf402-1a04-11d3-9a73-0000f81ef32e}
	tif: {557cf405-1a04-11d3-9a73-0000f81ef32e}
	png: {557cf406-1a04-11d3-9a73-0000f81ef32e}
	*/

	CLSID pngClsid;
	HRESULT hres = CLSIDFromString(L"{557cf406-1a04-11d3-9a73-0000f81ef32e}", &pngClsid);

	//save small
	imgSmall.Save(os_tmp.wstring().c_str(), &pngClsid, NULL);

	Bitmap bmSmall(os_tmp.wstring().c_str());

	HBITMAP hBmSmall, hBmMini;

	if (bmSmall.GetHBITMAP(Color(255, 255, 255)/*Color::Black*/, &hBmSmall) == Gdiplus::Ok) {
		
		Gdiplus::Graphics gmini(/*(Image*)*/&bmSmall);
		Gdiplus::Status gdi_res = gmini.DrawImage(/*(Gdiplus::Image*)*/ &bmSmall, 0, 0);
		PFC_ASSERT(gdi_res == Gdiplus::Ok);
		
		int inWidth = bmSmall.GetWidth();
		int inHeight = bmSmall.GetHeight();
		int newWidth = 48;
		int newHeight = 48;

		float ScalingFactor;
		if (inWidth >= inHeight)
			ScalingFactor = (float)newWidth / (float)bmSmall.GetWidth();
		else
			ScalingFactor = (float)newHeight / (float)bmSmall.GetHeight();

		/*
			horizontalScalingFactor & horizontalScalingFactor works well,
			except if the original image has a specific resolution,
			for example if it was created by the reading of an image file.
			In this case you have to apply the resolution of the
			original image to the scaling factors
		*/

		Gdiplus::Bitmap imgMini = Gdiplus::Bitmap((int)newWidth, (int)newHeight);

		Gdiplus::REAL oriResX = bmSmall.GetHorizontalResolution();
		Gdiplus::REAL oriResY = bmSmall.GetVerticalResolution();
		Gdiplus::REAL newResX = imgMini.GetHorizontalResolution();
		Gdiplus::REAL newResY = imgMini.GetVerticalResolution();

#ifdef DEBUG
		float debugx = (float)oriResX / (float)newResX;
		float debugy = (float)oriResY / (float)newResY;
#endif

		float horizontalScalingFactor = ScalingFactor * (float)oriResX / (float)newResX;//(float)oriResX / (float)newResX;
		float verticalScalingFactor = ScalingFactor * (float)oriResY / (float)newResY;//(float)oriResY / (float)newResY;

		Gdiplus::Graphics g(&imgMini);
		g.ScaleTransform(horizontalScalingFactor, verticalScalingFactor);
		gdi_res = g.DrawImage(/*(Gdiplus::Image*)*/ &bmSmall, 0, 0);
		PFC_ASSERT(gdi_res == Gdiplus::Ok);
		
		uGetTempPath(temp_path);
		uGetTempFileName(temp_path, "fb2k", 0, temp_file_name_mini);
		

		std::filesystem::path tmp_file_min = std::filesystem::u8path(temp_file_name_mini.c_str());

		if (!std::filesystem::exists(tmp_file_min)) {
		
			return std::pair(nullptr, nullptr);
		}

		std::filesystem::path os_tmp_mini = std::filesystem::u8path(temp_file_name_mini.c_str());

		CLSID pngClsid;
		hres = CLSIDFromString(L"{557cf406-1a04-11d3-9a73-0000f81ef32e}", &pngClsid);

		//save mini
		imgMini.Save(os_tmp_mini.wstring().c_str(), &pngClsid, NULL);

		Bitmap bmMini(os_tmp_mini.wstring().c_str());

		//Get hBmMini (48x48)
		//temp file to be deleted on hbitmap cleanup
		if (bmMini.GetHBITMAP(Color(255, 255, 255)/*Color::Black*/, &hBmMini) != Gdiplus::Ok) {
			log_msg("GdiPlus error (GetHBITMAP 48x48 tmp artwork)");
		}
	}
	else {
		log_msg("GdiPlus error (GetHBITMAP 150x150 tmp artwork)");
	}

	temp_file_names.first = temp_file_name_small;
	temp_file_names.second = temp_file_name_mini;

	return std::pair(hBmSmall, hBmMini);
}

MemoryBlock MemoryBlockToPngIcon(MemoryBlock buffer) {
	IStream* pStream = SHCreateMemStream((BYTE*)&buffer[0], buffer.get_count());
	Gdiplus::Bitmap bmFetch(pStream);

	int inWidth = bmFetch.GetWidth();
	int inHeight = bmFetch.GetHeight();
	int newWidth = 32;
	int newHeight = 32;
	float ScalingFactor;
	if (inWidth >= inHeight)
		ScalingFactor = (float)newWidth / (float)bmFetch.GetWidth();
	else
		ScalingFactor = (float)newHeight / (float)bmFetch.GetHeight();

	IStream* poutStream;
	CreateStreamOnHGlobal(NULL, true, &poutStream);
	Gdiplus::Bitmap bmIcon = Gdiplus::Bitmap((int)newWidth, (int)newHeight, PixelFormat24bppRGB);
	Gdiplus::Graphics gmini(&bmIcon);
	gmini.ScaleTransform(ScalingFactor, ScalingFactor);
	Gdiplus::Status status = gmini.DrawImage(&bmFetch, 0, 0);

	CLSID ClsidPNG;
	HRESULT hres = CLSIDFromString(L"{557cf406-1a04-11d3-9a73-0000f81ef32e}", &ClsidPNG);

	LARGE_INTEGER   li;
	li.QuadPart = 0;

	status = bmIcon.Save(poutStream, &ClsidPNG, NULL);
	hres = poutStream->Seek(li, STREAM_SEEK_SET, NULL);
	poutStream->Commit(STGC_DEFAULT);
	STATSTG outstats;
	poutStream->Stat(&outstats, STATFLAG_NONAME);

	ULONG read;
	UINT length = outstats.cbSize.QuadPart;

	BYTE* out_stream_bytes = new BYTE[outstats.cbSize.QuadPart];
	poutStream->Read(out_stream_bytes, outstats.cbSize.QuadPart, &read); //Read entire stream into the array

	MemoryBlock newbuffer; newbuffer.set_size(newWidth * newHeight * 3);
	newbuffer.set_data_fromptr(out_stream_bytes, length);

	poutStream->Release();
	pStream->Release();

	return newbuffer;

}

int static ReadSizeFromFile(pfc::string8 full_path) {

	std::filesystem::path p = std::filesystem::u8path(full_path.get_ptr());
	
	return std::filesystem::exists(p) ? std::filesystem::file_size(p) : -1;
}

std::pair<pfc::string8, pfc::string8> ReadDimSizeFromFile(pfc::string8 path, pfc::string8 file_name) {

	pfc::string8 full_path(path << "\\" << file_name);

	if (int filesize = ReadSizeFromFile(full_path); filesize != -1) {

		pfc::string8 dims, size;
		size = round_file_size_units(filesize);
		pfc::stringcvt::string_os_from_utf8 cvt(full_path);

		Bitmap local_bitmap(cvt.get_ptr(), false);

		UINT w = local_bitmap.GetWidth();
		UINT h = local_bitmap.GetHeight();

		dims << std::to_string(w).c_str() << "x" << std::to_string(h).c_str();

		return std::pair(dims, size);
	
	}
	else {
		return std::pair("", "");
	}
}