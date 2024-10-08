#include "stdafx.h"

#include <GdiPlus.h>
#include "CGdiPlusBitmap.h"
#pragma comment(lib, "gdiplus.lib")

#include "discogs_interface.h"
#include "ol_cache.h"
#include "track_matching_utils.h"

using namespace Gdiplus;

inline bool GdipErrorMsg(int code, const pfc::string8 path, pfc::string8& outmsg) {
	bool bret = 0;
	outmsg << " | ";
	switch (code) {
	case Gdiplus::Ok:                         outmsg << "Ok";                           bret = 1;
	case Gdiplus::GenericError:               outmsg << "Generic Error";                bret = 0;
	case Gdiplus::InvalidParameter:           outmsg << "Invalid Parameter";            bret = 0;
	case Gdiplus::OutOfMemory:                outmsg << "Out Of Memory";                bret = 0;
	case Gdiplus::ObjectBusy:                 outmsg << "Object Busy";                  bret = 0;
	case Gdiplus::InsufficientBuffer:         outmsg << "Insufficient Buffer";          bret = 0;
	case Gdiplus::NotImplemented:             outmsg << "Not Implemented";              bret = 0;
	case Gdiplus::Win32Error:                 outmsg << "Win32 Error";                  bret = 0;
	case Gdiplus::Aborted:                    outmsg << "Aborted";                      bret = 0;
	case Gdiplus::FileNotFound:               outmsg << "File Not Found";               bret = 0;
	case Gdiplus::ValueOverflow:              outmsg << "Value Overflow";               bret = 0;
	case Gdiplus::AccessDenied:               outmsg << "Access Denied";                bret = 0;
	case Gdiplus::UnknownImageFormat:         outmsg << "Unknown Image Format";         bret = 0;
	case Gdiplus::FontFamilyNotFound:         outmsg << "Font Family Not Found";        bret = 0;
	case Gdiplus::FontStyleNotFound:          outmsg << "Font Style Not Found";         bret = 0;
	case Gdiplus::NotTrueTypeFont:            outmsg << "Not TruType Font";             bret = 0;
	case Gdiplus::UnsupportedGdiplusVersion:  outmsg << "Unsupported Gdiplus Version";  bret = 0;
	case Gdiplus::GdiplusNotInitialized:      outmsg << "Gdiplus Not Initialized";      bret = 0;
	case Gdiplus::PropertyNotFound:           outmsg << "Property Not Found";           bret = 0;
	case Gdiplus::PropertyNotSupported:       outmsg << "Property Not Supported";       bret = 0;
	default:                                  outmsg << "Unknown Error";                bret = 0;
	}

	outmsg << " | ";
	outmsg << path;

	return bret;
}

multi_uartwork::multi_uartwork(const CConf& conf, Discogs::Release_ptr release) {

	if (!release) {
		return;
	}

	size_t calbum_art = release->images.get_count();
	size_t cartist_art = 0;

	for (auto wra : release->artists) {
		if (wra->full_artist) {
			cartist_art += wra->full_artist->images.get_count();
		}
	}

	size_t ctotal_art = calbum_art + cartist_art;

	// album attributes

	if (calbum_art) {

		if (conf.save_album_art) {

			setflag(af::alb_sd, 0, true);

			//ovr
			if (conf.album_art_overwrite)

				setflag(af::alb_ovr, 0, true);
		}

		if (conf.embed_album_art) {

			setflag(af::alb_emb, 0, true);
		}

		if (conf.album_art_fetch_all && conf.save_album_art) {

			setbitflag_range(af::alb_sd, true, 0, calbum_art);

			//ovr
			if (conf.album_art_overwrite)
				setbitflag_range(af::alb_ovr, true, 0, calbum_art);
		}
	}

	// artist attributes

	if (cartist_art) {

		if (conf.save_artist_art) {
			Discogs::Artist_ptr artist;
			size_t walk_artists_ndx = 0;
			size_t ndx;
			while (walk_artists_ndx < cartist_art) {
				
				if (discogs_interface->img_artists_ndx_to_artist(release, walk_artists_ndx, artist, ndx)) {
					if (ndx == 0) {
						setflag(af::art_sd, walk_artists_ndx, true);
						walk_artists_ndx += artist->images.get_count() -1;
					}
				}
				walk_artists_ndx++;
			}

			//ovr
			if (conf.artist_art_overwrite)
				setflag(af::art_ovr, 0, true);
		}

		if (conf.embed_artist_art) {

			setflag(af::art_emb, 0, true);
		}

		if (conf.artist_art_fetch_all && conf.save_artist_art) {

			setbitflag_range(af::art_sd, true, 0, cartist_art);

			//ovr
			if (conf.artist_art_overwrite)
				setbitflag_range(af::art_ovr, true, 0, cartist_art);
		}
	}

	init = true;
}

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

std::vector<int> build_woas_libppui(CListControlOwnerData* ui_list, double tile_multi) {

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
		if (it_woa == vw_res.begin()) {
			hwoa = (double)hwoa * tile_multi;
		}
		//store woa
		*it_woa = MAKELPARAM(lwoa, hwoa);
	}
	return vw_res;
}

bool isPrimary(pfc::string8 type) {
	return STR_EQUAL(type, "primary") || STR_EQUAL(type, "secondary+");
}

//discogs artwork list, fixes missing primary item
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

//generate cache thumbnails files and hbitmaps from artwork preview memoryblock
imgpairs MemoryBlockToTmpBitmap(std::pair<pfc::string8, pfc::string8> n8_cache_paths, /*size_t pos,*/ MemoryBlock small_art) {

	if (!small_art.get_count()) return imgpairs{ {nullptr, nullptr}, {nullptr, nullptr } };

	pfc::string8 temp_file_small(n8_cache_paths.first);
	pfc::string8 temp_file_mini(n8_cache_paths.second);

	std::filesystem::path os_file_small = std::filesystem::u8path(temp_file_small.c_str());
	std::filesystem::path os_file_mini = std::filesystem::u8path(temp_file_mini.c_str());

	auto jf = _wopen(os_file_small.wstring().c_str(), _O_CREAT | _O_TRUNC | _O_RDWR | _O_BINARY, _S_IWRITE);
	if (jf == -1) {
		pfc::string8 msg("can't open ");
		msg << temp_file_small;
		log_msg(msg);
		return imgpairs{ {nullptr, nullptr}, {nullptr, nullptr } };
	}
	else {
		//write small thumb (aprox. 150x150)
		auto w = _write(jf, small_art.get_ptr(), small_art.get_size());
		if (w != small_art.get_size()) {
			_close(jf);
			pfc::string8 msg("error writing ");
			msg << temp_file_small;
			log_msg(msg);
			return imgpairs{ {nullptr, nullptr}, {nullptr, nullptr } };
		}
		_close(jf);
	}

	Bitmap bmSmall(os_file_small.wstring().c_str());

	HBITMAP hBmSmall, hBmMini;
	HICON hIconSmall, hIconMini;

	//Get hBmSmall (150x150 aprox)
	if (bmSmall.GetHBITMAP(Color(255, 255, 255)/*Color::Black*/, &hBmSmall) == Gdiplus::Ok) {
		bmSmall.GetHICON(&hIconSmall);

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
		Gdiplus::Bitmap imgMini = Gdiplus::Bitmap((int)newWidth, (int)newHeight);

		Gdiplus::REAL oriResX = bmSmall.GetHorizontalResolution();
		Gdiplus::REAL oriResY = bmSmall.GetVerticalResolution();
		Gdiplus::REAL newResX = imgMini.GetHorizontalResolution();
		Gdiplus::REAL newResY = imgMini.GetVerticalResolution();


		float horizontalScalingFactor = ScalingFactor * ((float)oriResX / (float)newResX);
		float verticalScalingFactor = ScalingFactor * ((float)oriResY / (float)newResY);

		Gdiplus::Graphics g(&imgMini);
		g.ScaleTransform(horizontalScalingFactor, verticalScalingFactor);

		gdi_res = g.DrawImage(/*(Gdiplus::Image*)*/ &bmSmall, 0, 0);
		
		PFC_ASSERT(gdi_res == Gdiplus::Ok);

		auto jf = _wopen(os_file_mini.wstring().c_str(), _O_CREAT | _O_TRUNC | _O_RDWR | _O_BINARY, _S_IWRITE);

		if (jf == -1) {
			pfc::string8 msg("can't open ");
			msg << temp_file_mini;
			log_msg(msg);
			return imgpairs{ {nullptr, nullptr}, {nullptr, nullptr } };
		}
		else {
			_close(jf);
		}

		CLSID pngClsid;
		HRESULT hres = CLSIDFromString(L"{557cf406-1a04-11d3-9a73-0000f81ef32e}", &pngClsid);

		//write mini thumb
		hres = imgMini.Save(os_file_mini.wstring().c_str(), &pngClsid, NULL);

		if (hres != Gdiplus::Status::Ok) {
			pfc::string8 err = "Error saving small preview tmp bitmap: ";
			GdipErrorMsg(hres, os_file_mini.u8string().c_str(), err);
			log_msg(err);
			return imgpairs{ {nullptr, nullptr}, {nullptr, nullptr } };
		}

		Bitmap bmMini(os_file_mini.wstring().c_str());

		//Get hBmMini (48x48)
		hres = bmMini.GetHBITMAP(Color(255, 255, 255)/*Color::Black*/, &hBmMini);
		if (hres != Gdiplus::Ok) {
			pfc::string8 err = "GdiPlus error (GetHBITMAP Small preview 48x48)";
			GdipErrorMsg(hres, os_file_mini.u8string().c_str(), err);
			log_msg(err);
			return imgpairs{ {nullptr, nullptr}, {nullptr, nullptr } };
		}
		else {
			bmMini.GetHICON(&hIconMini);
			DeleteObject(hBmMini);
		}

		DeleteObject(hBmSmall);
	}
	else {
		log_msg("GdiPlus error (GetHBITMAP Small preview)");
	}

	return std::pair(std::pair(hIconSmall, nullptr), std::pair(hIconMini, nullptr));
}
imgpairs GenerateTmpBitmapsFromRealSize(pfc::string8 release_id, size_t pos,
	pfc::string8 source_full_path, std::pair<pfc::string8, pfc::string8>& temp_file_names) {

	std::filesystem::path os_src_full_path = std::filesystem::u8path(source_full_path.c_str());

	Bitmap bmRealSize(os_src_full_path.wstring().c_str());

	Gdiplus::Status status = bmRealSize.GetLastStatus();
	if (status != Gdiplus::Status::Ok) {
		pfc::string8 err = "Error opening real size bitmap";
		GdipErrorMsg(status, os_src_full_path.u8string().c_str(), err);
		log_msg(err);
		return imgpairs{ {nullptr, nullptr}, {nullptr, nullptr } };
	}

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


	float horizontalScalingFactor = ScalingFactor * (float)oriResX / (float)newResX;
	float verticalScalingFactor = ScalingFactor * (float)oriResY / (float)newResY;

	Gdiplus::Graphics g(&imgSmall);
	g.ScaleTransform(horizontalScalingFactor, verticalScalingFactor);

	if (g.DrawImage((Gdiplus::Image*) & bmRealSize, 0, 0) == Gdiplus::Ok) {
		//..
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
	hres = imgSmall.Save(os_tmp.wstring().c_str(), &pngClsid, NULL);

	if (!SUCCEEDED(hres)) {
		pfc::string8 err = "Error saving temp small png icon";
		GdipErrorMsg(hres, os_tmp.u8string().c_str(), err);
		log_msg(err);
		return imgpairs{ {nullptr, nullptr}, {nullptr, nullptr } };
	}

	Bitmap bmSmall(os_tmp.wstring().c_str());

	status = bmSmall.GetLastStatus();
	if (status != Gdiplus::Status::Ok) {
		pfc::string8 err = "Error opening small size temp path";
		GdipErrorMsg(status, os_tmp.u8string().c_str(), err);
		log_msg(err);
		return imgpairs{ {nullptr, nullptr}, {nullptr, nullptr } };
	}

	HBITMAP hBmSmall, hBmMini;
	HICON hIconSmall, hIconMini;

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


		float horizontalScalingFactor = ScalingFactor * (float)oriResX / (float)newResX;
		float verticalScalingFactor = ScalingFactor * (float)oriResY / (float)newResY;

		Gdiplus::Graphics g(&imgMini);
		g.ScaleTransform(horizontalScalingFactor, verticalScalingFactor);
		gdi_res = g.DrawImage(/*(Gdiplus::Image*)*/ &bmSmall, 0, 0);
		bmSmall.GetHICON(&hIconSmall);
		PFC_ASSERT(gdi_res == Gdiplus::Ok);
		
		uGetTempPath(temp_path);
		uGetTempFileName(temp_path, "fb2k", 0, temp_file_name_mini);
		

		std::filesystem::path tmp_file_min = std::filesystem::u8path(temp_file_name_mini.c_str());

		if (!std::filesystem::exists(tmp_file_min)) {

			return std::pair(std::pair(hIconSmall, nullptr), std::pair(nullptr, nullptr));
		}
		std::filesystem::path os_tmp_mini = std::filesystem::u8path(temp_file_name_mini.c_str());

		CLSID pngClsid;
		hres = CLSIDFromString(L"{557cf406-1a04-11d3-9a73-0000f81ef32e}", &pngClsid);

		//save mini
		hres = imgMini.Save(os_tmp_mini.wstring().c_str(), &pngClsid, NULL);
		if (!SUCCEEDED(hres)) {
			pfc::string8 err = "Error saving minimal png icon";
			GdipErrorMsg(hres, os_tmp_mini.u8string().c_str(), err);
			log_msg(err);
			return imgpairs{ {hIconSmall, nullptr}, {nullptr, nullptr } };
		}

		Bitmap bmMini(os_tmp_mini.wstring().c_str());

		Gdiplus::Status status = bmMini.GetLastStatus();
		if (status != Gdiplus::Status::Ok) {
			pfc::string8 err = "Error opening minimal size temp path";
			GdipErrorMsg(status, os_tmp_mini.u8string().c_str(), err);
			log_msg(err);
			return imgpairs{ {hIconSmall, nullptr}, {nullptr, nullptr } };
		}

		hres = bmMini.GetHBITMAP(Color(255, 255, 255)/*Color::Black*/, &hBmMini);
		if (!SUCCEEDED(hres)) {
			pfc::string8 err = "GdiPlus error (GetHBITMAP 48x48 tmp artwork)";
			GdipErrorMsg(hres, os_tmp_mini.u8string().c_str(), err);
			log_msg(err);
			return imgpairs{ {hIconSmall, nullptr}, {nullptr, nullptr } };
		}
		else {
			bmMini.GetHICON(&hIconMini);
			DeleteObject(hBmMini);
		}
		DeleteObject(hBmSmall);
	}
	else {
		log_msg("GdiPlus error (GetHBITMAP 150x150 tmp artwork)");
	}
	return std::pair(std::pair(hIconSmall, nullptr), std::pair(hIconMini, nullptr));
}

MemoryBlock MemoryBlockToPngIcon(MemoryBlock buffer) {
	IStream* pStream = SHCreateMemStream((BYTE*)&buffer[0], buffer.get_count());

	if (!pStream) {
		log_msg("Bad memory stream allocation for PNG Icon");
		return {};
	}

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

	HRESULT hr = S_FALSE;
	IStream* poutStream;
	hr = CreateStreamOnHGlobal(NULL, true, &poutStream);
	if (!SUCCEEDED(hr)) {
		log_msg("Bad global stream allocation for PNG Icon");
		poutStream->Release();
		pStream->Release();
		return {};
	}

	Gdiplus::Bitmap bmIcon = Gdiplus::Bitmap((int)newWidth, (int)newHeight, PixelFormat24bppRGB);
	Gdiplus::Graphics gmini(&bmIcon);
	gmini.ScaleTransform(ScalingFactor, ScalingFactor);
	Gdiplus::Status status = gmini.DrawImage(&bmFetch, 0, 0);

	CLSID ClsidPNG;
	HRESULT hres = CLSIDFromString(L"{557cf406-1a04-11d3-9a73-0000f81ef32e}", &ClsidPNG);

	LARGE_INTEGER   li;
	li.QuadPart = 0;

	status = bmIcon.Save(poutStream, &ClsidPNG, NULL);
	if (status != Gdiplus::Status::Ok) {
		pfc::string8 err = "Bad result saving to PNG Icon stream";
		GdipErrorMsg(hres, "memory", err);
		log_msg(err);

		poutStream->Release();
		pStream->Release();

		return {};
	}
	
	hres = poutStream->Seek(li, STREAM_SEEK_SET, NULL);
	poutStream->Commit(STGC_DEFAULT);
	
	STATSTG outstats;
	poutStream->Stat(&outstats, STATFLAG_NONAME);

	ULONG read;
	UINT length = outstats.cbSize.QuadPart;

	MemoryBlock newbuffer = {};
	BYTE* out_stream_bytes = new BYTE[outstats.cbSize.QuadPart];
	hr = poutStream->Read(out_stream_bytes, outstats.cbSize.QuadPart, &read); //Read entire stream into the array
	if (SUCCEEDED(hr)) {
		newbuffer.set_size(newWidth * newHeight * 3);
		newbuffer.set_data_fromptr(out_stream_bytes, length);
	}
	else {
		log_msg("Bad result reading from PNG Icon stream");
	}
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
