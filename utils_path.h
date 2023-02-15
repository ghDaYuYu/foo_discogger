#pragma once

static pfc::string8 profile_path(pfc::string8 folder, bool native = true) {
	pfc::string8 path;
	path = core_api::pathInProfile(folder);
	if (native)
		extract_native_path(path, path);
	return path;
}

static pfc::string8 dll_db_filename() {
	pfc::string8 str = core_api::get_my_file_name();
	str << ".dll.db";
	return str;
}

static pfc::string8 profile_usr_components_path(bool native = true) {
	pfc::string8 path;
#ifdef _WIN64
	path << (core_api::pathInProfile("user-components-x64\\") << core_api::get_my_file_name());
#else
	path << (core_api::pathInProfile("user-components\\") << core_api::get_my_file_name());
#endif
	if (native)
		extract_native_path(path, path);
	return path;
}

static pfc::string8 full_dll_db_name(bool native = true) {
	pfc::string8 db_path;
	db_path << core_api::pathInProfile("configuration\\") << dll_db_filename();
	if (native)
		extract_native_path(db_path, db_path);
	return db_path.c_str();
}
