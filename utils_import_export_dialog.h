#pragma once

//ej. "Tag Mapping Files (*.tm)\0*.tm\0All Files (*.*)\0*.*\0"
inline BOOL OpenExportDlg(HWND hWndparent, pfc::string8 strfilter, pfc::string8& out) {

	char filename[MAX_PATH];
	OPENFILENAMEA ofn;
	ZeroMemory(&filename, sizeof(filename));
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWndparent;
	ofn.lpstrFilter = strfilter;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = "Save to file...";
	ofn.Flags = OFN_DONTADDTORECENT;

	GetSaveFileNameA(&ofn);

	pfc::string8 strFinalName(filename);
	pfc::string8 tmpNoExt(pfc::string_filename(filename).get_ptr());
	//check also extension & missing name
	tmpNoExt.truncate(tmpNoExt.get_length() - pfc::string_extension(filename).length() + (pfc::string_extension(filename).length() ? 1 /*+1 dot ext length*/ : 0));
	if (!strFinalName.length() || !tmpNoExt.get_length()) {
		return FALSE;
	}

	pfc::string8 outExt;
	pfc::string8 filterExt = ofn.nFilterIndex == 1 ? ".tm" : "";
	outExt << "." << pfc::string_extension(filename);
	if (outExt.get_length() == 1) outExt = "";

	if ((ofn.nFilterIndex == 1) && (stricmp_utf8(outExt, ".tm"))) {
		out = filterExt;
	}
	return TRUE;
}

inline BOOL OpenImportDlg(HWND hWndparent, pfc::string8 strfilter, pfc::string8& out) {

	char filename[MAX_PATH];
	OPENFILENAMEA ofn;
	ZeroMemory(&filename, sizeof(filename));
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWndparent;
	ofn.lpstrFilter = strfilter;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = MAX_PATH - 1;
	ofn.lpstrTitle = "Load from file...";
	ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;

	if (GetOpenFileNameA(&ofn)) {
		//log_msg(filename);
	}

	{
		pfc::string8 tmpFullPath(ofn.lpstrFile);
		if (!tmpFullPath.length()) {
			return FALSE;
		}
		out = tmpFullPath;
	}
	return TRUE;
}