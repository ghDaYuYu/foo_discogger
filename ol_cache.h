#pragma once
#include <filesystem>
#include "jansson/jansson.h"

#include "track_matching_utils.h"

namespace Offline {
	const pfc::string8 MARK_LOADING_NAME = "loading";
	const pfc::string8 MARK_CHECK_NAME = "taskreg.txt";
	const pfc::string8 OC_NAME = "foo_discogger-cache";

	enum CacheFlags {
		OC_READ = 1 << 0,
		OC_WRITE = 1 << 1,
		OC_OVERWRITE = 1 << 2,
		OC_3 = 1 << 3,
		OC_4 = 1 << 4,
		OC_5 = 1 << 5,
		OC_6 = 1 << 6,
		OC_7 = 1 << 7
	};

	enum GetFrom {
		ArtistReleases,
		Versions,
		Artist,
		Release,
	};

	static bool can_read(new_conf conf) { return conf.cache_use_offline_cache & CacheFlags::OC_READ; }
	static bool can_read() { return CONF.cache_use_offline_cache & CacheFlags::OC_READ; }
	static bool can_write() { return CONF.cache_use_offline_cache & CacheFlags::OC_WRITE; }
	static bool can_ovr() { return CONF.cache_use_offline_cache & CacheFlags::OC_OVERWRITE; }
	static pfc::string8 GetOfflinePath(pfc::string8 id, bool native, GetFrom gfFrom, pfc::string8 secId) {

		const char* dll_name = core_api::get_my_file_name();
		pfc::string8 foo_path(core_api::pathInProfile("user-components\\") << dll_name);
		
		pfc::string8 ol_path(core_api::pathInProfile(OC_NAME));

		switch(gfFrom) {
		case GetFrom::Artist:
			ol_path << "\\artists\\" << id << "\\artist";
			break;
		case GetFrom::ArtistReleases:
			ol_path << "\\artists\\" << id << "\\releases";
			break;
		case GetFrom::Release:
			ol_path << "\\artists\\" << id << "\\release\\" << secId;
			break;
		case GetFrom::Versions:
			ol_path << "\\artists\\" << id << "\\masters\\" << secId << "\\versions";
			break;
		default:
			PFC_ASSERT(false);
		}

		if (native) 			
			extract_native_path(ol_path, ol_path);

		return ol_path;
	}

	static pfc::string8 get_thumbnail_cache_path(pfc::string8 id, art_src artSrc) {

		const char* dll_name = core_api::get_my_file_name();
		pfc::string8 foo_path(core_api::pathInProfile("configuration\\"));
		
		pfc::string8 release_path(core_api::pathInProfile(OC_NAME));
		release_path << "\\thumbnails\\";
		release_path << (artSrc == art_src::alb ? "releases" :	artSrc == art_src::art ? "artists" : "unknown") << "\\" << id;
		
		return release_path;
	}

	static pfc::array_t<pfc::string8> get_thumbnail_cache_path_filenames(pfc::string8 id, art_src artSrc, size_t iImageList, size_t ndx = pfc_infinite) {
		pfc::string8 path_native;
		pfc::stringcvt::string_wide_from_utf8 wpath;

		pfc::array_t<pfc::string8> folders;
		pfc::array_t<pfc::string8> directories;
		pfc::array_t<pfc::string8> filenames;

		pfc::string8 rel_path = get_thumbnail_cache_path(id, artSrc);
		pfc::string8 full_path(rel_path);
		full_path << "\\thumb_" << (iImageList == LVSIL_NORMAL ? "150x150" : "48x48") << "_";
		
		if (ndx == pfc_infinite) {
			filenames.append_single(full_path);
			return filenames;
		}
		full_path << ndx << THUMB_EXTENSION;
		
		abort_callback_dummy dummy_abort;
		try {
			listFiles(rel_path, folders, dummy_abort);
		}
		catch (...) {
			return filenames;
		}

		for (t_size walk = 0; walk < folders.get_size(); ++walk) {
			pfc::string8 tmp(folders[walk]);

			if (stricmp_utf8(tmp, full_path) == 0)
				filenames.append_single(tmp);
		}
		return filenames;
	}


	static pfc::string8 GetOfflinePagesPath(pfc::string8 id, size_t page, bool native, GetFrom getFrom, pfc::string8 secid) {
		
		PFC_ASSERT(getFrom == GetFrom::ArtistReleases || getFrom == GetFrom::Versions);
		PFC_ASSERT(getFrom != GetFrom::Versions || (!STR_EQUAL(id, secid) && secid.get_length()));
		
		pfc::string8 page_path(GetOfflinePath(id, native, getFrom, secid));
		page_path << "\\page-";
		if (page != pfc_infinite)
			page_path << page;
		
		return page_path;
	}

	static pfc::array_t<pfc::string8> GetFSPagesFilePaths(pfc::string8 id, GetFrom getFrom, pfc::string8 secid) {

		PFC_ASSERT(getFrom == GetFrom::ArtistReleases || getFrom == GetFrom::Versions);
		PFC_ASSERT(getFrom != GetFrom::Versions || (!STR_EQUAL(id, secid) && secid.get_length()));

		pfc::string8 path_native;
		pfc::stringcvt::string_wide_from_utf8 wpath;
		pfc::array_t<pfc::string8> folders;
		pfc::array_t<pfc::string8> pagepaths;

		pfc::string8 rel_path = GetOfflinePath(id, false, getFrom, secid);
		
		abort_callback_dummy dummy_abort;
		try {
			listDirectories(rel_path, folders, dummy_abort);
		}
		catch (...) {
			return pagepaths;
		}

		pfc::string8 pages_path_prefix = GetOfflinePagesPath(id, pfc_infinite, false, getFrom, secid);
		for (t_size walk = 0; walk < folders.get_size(); ++walk) {
			pfc::string8 tmp(folders[walk]);
			if (tmp.has_prefix(pages_path_prefix)) {
				extract_native_path(tmp, path_native);
				pagepaths.append_single(path_native);
			}
		}
		return pagepaths;
	}

	static pfc::array_t<pfc::string8> GetFS_IdFilePaths(pfc::string8 id, GetFrom getFrom, pfc::string8 secid) {

		PFC_ASSERT(getFrom == GetFrom::Artist || getFrom == GetFrom::Release);
		PFC_ASSERT(getFrom != GetFrom::Release || (!STR_EQUAL(id, secid) && secid.get_length()));

		pfc::string8 path_native;
		pfc::stringcvt::string_wide_from_utf8 wpath;

		pfc::array_t<pfc::string8> folders;
		pfc::array_t<pfc::string8> paths;

		pfc::string8 rel_path = GetOfflinePath(id, false, getFrom, secid);

		abort_callback_dummy dummy_abort;
		try {
			listFiles(rel_path, folders, dummy_abort);
		}
		catch (...) {
			return paths;
		}
		if (!folders.get_size())
			return paths;

		pfc::string8 tmp(folders[0]);
		extract_native_path(tmp, path_native);
		paths.append_single(path_native);
		return paths;
	}

	bool static MarkDownload(pfc::string8 fcontent, pfc::string8 path, bool done) {
		bool bok = false;
		if (!done) {
			std::filesystem::path fspath;
			char converted[MAX_PATH - 1];
			pfc::string8 taskreg_path;
			taskreg_path << path << "\\" << MARK_CHECK_NAME;
			pfc::stringcvt::string_os_from_utf8 cvt(taskreg_path);
			wcstombs(converted, cvt.get_ptr(), MAX_PATH - 1);		
			try {
				std::filesystem::remove_all(converted);
			}
			catch (...) {
				return false;
			}
			//..

			FILE* f = fopen(path << "\\" << MARK_LOADING_NAME, "ab+");
			if (f) {
				if (fwrite(fcontent.get_ptr(), fcontent.get_length(), 1, f) != 1)
					bok |= false;
			}

			bok |= f && (fclose(f) == 0);
		}
		else {
			pfc::string8 oldname(path); oldname << "\\" << MARK_LOADING_NAME;
			pfc::string8 newname(path); newname << "\\" << MARK_CHECK_NAME;
			bok |= (rename(oldname, newname) == 0);
		}

		return bok;
	}

	bool static CheckDownload(pfc::string8 path) {
		
		pfc::string8 path_taskreg;
		pfc::string8 path_loading;
		path_taskreg << path << "\\" << MARK_CHECK_NAME;
		path_loading << path << "\\" << MARK_LOADING_NAME;

		//check codepage
		char converted[MAX_PATH - 1];
		pfc::stringcvt::string_os_from_utf8 cvt(path_taskreg);
		wcstombs(converted, cvt.get_ptr(), MAX_PATH - 1);

		struct stat stat_buf;
		int stat_result = stat(converted, &stat_buf);
		int src_length = stat_buf.st_size;

		cvt = path_loading.get_ptr();
		wcstombs(converted, cvt.get_ptr(), MAX_PATH - 1);

		int stat_result_loading = stat(converted, &stat_buf);
		int src_length_loading = stat_buf.st_size;

		return (stat_result == 0 || src_length == 0) && (stat_result_loading == -1);
	}

	static bool is_data_avail(pfc::string8 id, pfc::string8 secid, GetFrom gfFrom, pfc::string8& out_relative_path) {
		out_relative_path = GetOfflinePath(/*artist_id*/id, /*native*/ true, /*ol::GetFrom::Release*/gfFrom, /*release_id*/secid);
		return (GetFS_IdFilePaths(id, gfFrom, secid).get_ptr() != nullptr) && CheckDownload(out_relative_path);
	};

	bool static CreateOfflinePagesPath(pfc::string8 id, art_src artSrc, size_t subpage, GetFrom getFrom, pfc::string8 secid, bool thumbs = false) {

		PFC_ASSERT(getFrom == GetFrom::ArtistReleases || getFrom == GetFrom::Versions);
		PFC_ASSERT(getFrom != GetFrom::Versions || (!STR_EQUAL(id, secid) && secid.get_length()));

		pfc::string8 rel_path;
		if (subpage == pfc_infinite) {		
			if (!thumbs)
				rel_path = GetOfflinePath(id, true, getFrom, secid);
			else {
				rel_path = get_thumbnail_cache_path(id, artSrc);
				extract_native_path(rel_path, rel_path);
			}
		}
		else
			rel_path = GetOfflinePagesPath(id, subpage, true, getFrom, secid);

		pfc::stringcvt::string_wide_from_utf8 wpath;
		std::filesystem::path fspath;
		wpath = rel_path.get_ptr();
		fspath = wpath.get_ptr();

		try {
			std::filesystem::create_directories(fspath);
		}
		catch (...) {
			return false;
		}

		return true;
	}

	bool static CreateFS_Offline_IdPath(pfc::string8 id, GetFrom getFrom, pfc::string8 secid = "") {

		PFC_ASSERT(getFrom == GetFrom::Artist || getFrom == GetFrom::Release);
		PFC_ASSERT(getFrom != GetFrom::Release || (!STR_EQUAL(id, secid) && secid.get_length()));

		pfc::string8 rel_path;
		rel_path = GetOfflinePath(id, true, getFrom, secid);
		pfc::stringcvt::string_wide_from_utf8 wpath;
		std::filesystem::path fspath;
		wpath = rel_path.get_ptr();
		fspath = wpath.get_ptr();

		try {
			std::filesystem::create_directories(fspath);
		}
		catch (...) {
			return false;
		}

		return true;
	}

	template<typename key_t, typename value_t>
	class ol_cache
	{
	public:
		typedef typename std::pair<key_t, value_t> key_value_pair_t;
		typedef typename std::list<key_value_pair_t>::iterator list_iterator_t;

		ol_cache(size_t max_size) :
			_max_size(max_size) {
		}

		void FDumpf(json_t* root) {
			bool bfileempty = true;
			const char* strfile_out = m_offlinepath;
			struct stat stat_buf;
			int stat_result = stat(strfile_out, &stat_buf);
			int src_length = stat_buf.st_size;
			bfileempty = (stat_result == -1 || src_length == 0);
			bool badded = true;
			int flags = 0;
			FILE* fOut = fopen(strfile_out, stat_result == -1 ? "wb+" : "ab+");
			int result = json_dumpf(root, fOut, flags);
			badded = (fclose(fOut) == 0 && result == 0);
		}

		bool FDumpToFolder(pfc::string8 path, json_t* root) {
			int flags = 0;
			FILE* fOut = fopen(path.get_ptr(), "ab+");
			if (!fOut)
				return false;

			int result = json_dumpf(root, fOut, flags);
			return (fclose(fOut) == 0 && result == 0);
		}

		json_t* FReadAll(const char* offlinepath = nullptr) const {
			struct stat stat_buf;
			pfc::string8 path = offlinepath == nullptr ? m_offlinepath : offlinepath;
			log_msg(pfc::string8("reading offline cache page:") << path);
			
			int rc_stat = stat(path.get_ptr(), &stat_buf);
			int srclen = stat_buf.st_size;

			json_t* root = json_array();
			FILE* fOut;

			if (rc_stat != -1) {
				if (srclen > 0) {
					fOut = fopen(path.get_ptr(), "r");
					json_error_t error = {};
					root = json_loadf(fOut, 0, &error);
					fclose(fOut); fOut = NULL;
					if (sizeof(error.text)) {
						log_msg(PFC_string_formatter() << "deleting invalid offline cache file: " << path);
						std::filesystem::path fspath = path.get_ptr();
						try {
							pfc::string8 debug;
							debug << fspath.remove_filename().c_str();
							std::filesystem::remove_all(fspath.remove_filename());
						}
						catch (...) {
							//
						}
					}
				}
			}
			return root;
		}
	private:
		
		pfc::string8 m_offlinepath;
	};
}

