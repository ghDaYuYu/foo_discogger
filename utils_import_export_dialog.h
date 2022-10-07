#pragma once

//ej. filter : "Tag Mapping Files (*.tm)\0*.tm\0All Files (*.*)\0*.*\0"
inline BOOL OpenExportDlg(HWND hWndparent, const TCHAR * filter, std::wstring& out) {

	TCHAR lpstrFile[MAX_PATH] = _T("");
	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWndparent;
	ofn.lpstrFilter = filter;
	ofn.lpstrFile = lpstrFile;
	ofn.lpstrDefExt = _T("tm");
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = L"Save to file...";
	ofn.Flags = OFN_DONTADDTORECENT;

	if (GetSaveFileName(&ofn)) {
		std::wstring wfilename(&lpstrFile[0]);
		if (!wfilename.size()) {
			return FALSE;
		}

		char fullpath[MAX_PATH] = "";
		pfc::stringcvt::convert_wide_to_utf8(fullpath, MAX_PATH, wfilename.c_str(), MAX_PATH);

		pfc::string8 filename(pfc::string_filename(fullpath));
		
		//only extension?
		filename.truncate(filename.get_length() - pfc::string_extension(fullpath).length() > filename.get_length() ?
			filename.get_length() - pfc::string_extension(fullpath).length() : filename.get_length());
		
		if (!pfc::string8(fullpath).length() || !filename.get_length()) {

			return FALSE;
		}

		//no extension?
		pfc::string8 dotext;
		dotext << "." << pfc::string_extension(fullpath);
		if (dotext.get_length() == 1) dotext = "";

		auto filterExt = ofn.nFilterIndex == 1 ? L".tm" : L"";
		out = wfilename.c_str();
		if ((ofn.nFilterIndex == 1) && (stricmp_utf8(dotext, ".tm"))) {
			out.append(filterExt);
		}
		out = wfilename.c_str();
		return TRUE;
	}
	return FALSE;
}

inline BOOL OpenImportDlg(HWND hWndparent, const TCHAR * filter, std::wstring& out) {

	TCHAR lpstrFile[MAX_PATH] = L"";
	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWndparent;
	ofn.lpstrFilter = filter;
	ofn.lpstrFile = lpstrFile;
	ofn.nMaxFile = MAX_PATH - 1;
	ofn.lpstrTitle = L"Load from file...";
	ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&ofn)) {
		std::wstring wfilename(&lpstrFile[0]);		
		if (!wfilename.size()) {
			return FALSE;
		}
		out = wfilename.c_str();
		return TRUE;
	}
	return FALSE;
}