#pragma once
#include <errno.h>
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
	};

	enum GetFrom {
		ArtistReleases,
		Versions,
		Artist,
		Release,
	};

	static bool can_read(CConf conf) { return conf.cache_offline_cache_flag & CacheFlags::OC_READ; }
	static bool can_read() { return CONF.cache_offline_cache_flag & CacheFlags::OC_READ; }
	static bool can_write() { return CONF.cache_offline_cache_flag & CacheFlags::OC_WRITE; }
	static bool can_ovr() { return CONF.cache_offline_cache_flag & CacheFlags::OC_OVERWRITE; }

	static pfc::string8 GetOfflinePath(pfc::string8 id, bool native, GetFrom gfFrom, pfc::string8 secId) {

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

		pfc::string8 path(core_api::pathInProfile(OC_NAME));
		path << "\\thumbnails\\";
		path << (artSrc == art_src::alb ? "releases" :	artSrc == art_src::art ? "artists" : "unknown") << "\\" << id;
		
		return path;
	}

	static pfc::array_t<pfc::string8> get_thumbnail_cache_path_filenames(pfc::string8 id, art_src artSrc, size_t iImageList, size_t ndx = pfc_infinite) {

		pfc::array_t<pfc::string8> folders;
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
			pfc::string8 path_native(folders[walk]);
			if (path_native.has_prefix(pages_path_prefix)) {
				extract_native_path(path_native, path_native);
				pagepaths.append_single(path_native);
			}
		}
		return pagepaths;
	}

	static pfc::array_t<pfc::string8> GetFS_IdFilePaths(pfc::string8 id, GetFrom getFrom, pfc::string8 secid) {

		PFC_ASSERT(getFrom == GetFrom::Artist || getFrom == GetFrom::Release || getFrom == GetFrom::ArtistReleases || getFrom == GetFrom::Versions);
		PFC_ASSERT((getFrom != GetFrom::Release && getFrom != GetFrom::Versions) || (!STR_EQUAL(id, secid) && secid.get_length()));

		pfc::array_t<pfc::string8> entries;
		pfc::array_t<pfc::string8> paths;

		pfc::string8 rel_path = GetOfflinePath(id, false, getFrom, secid);

		std::filesystem::path os_path = std::filesystem::u8path(rel_path.c_str());

		abort_callback_dummy dummy_abort;

		try {
			listFiles(os_path.string().c_str(), entries, dummy_abort);
		}
		catch (...) {
			return {};
		}
		if (!entries.get_size())
			return {};

		return entries;
	}

	//marks folder job: 'loading.' or 'TaskReg.txt' (done param value)

	bool static MarkDownload(pfc::string8 fcontent, pfc::string8 path, bool done) {
		
		bool bok = false;
		
		if (!done) {
			
			//delete 'not done' existing folder

			std::filesystem::path os_path = std::filesystem::u8path(path.c_str());
			
			try {
				std::filesystem::remove_all(os_path);
			}
			catch (...) {
				return false;
			}

			//..

			//mark as pending

			std::error_code ec;
			bool bfolder_created = std::filesystem::create_directories(os_path, ec);

			if (!ec.value() && bfolder_created) {

				os_path = std::filesystem::u8path((PFC_string_formatter() << path << "\\" << MARK_LOADING_NAME).c_str());

				int flags = 0;
				FILE* fo_abplus;
				errno = 0;

				if ((fo_abplus = fopen(os_path.string().c_str(), "ab+")) != NULL)
				{
					int w = fwrite(fcontent.get_ptr(), fcontent.get_length(), 1, fo_abplus);
					bok = (w == fcontent.get_length());

					return (fclose(fo_abplus) == 0) && bok;
				}
			}

			bok = false;
		}
		else {

			//done

			pfc::string8 oldname(path); oldname << "\\" << MARK_LOADING_NAME;
			pfc::string8 newname(path); newname << "\\" << MARK_CHECK_NAME;
			std::filesystem::path os_old = std::filesystem::u8path(oldname.c_str());
			std::filesystem::path os_new = std::filesystem::u8path(newname.c_str());

			std::error_code ec;
			std::filesystem::rename(os_old, os_new, ec);
			bok = !ec.value();
		}

		return bok;
	}


	bool static CheckDownload(pfc::string8 path) {
		
		pfc::string8 path_loading;
		pfc::string8 path_taskreg;
		path_loading << path << "\\" << MARK_LOADING_NAME;
		path_taskreg << path << "\\" << MARK_CHECK_NAME;

		bool stat_result_loading = false;
		bool stat_result_checked = false;
		int src_length_loading = -1;
		int src_length_checked = -1;

		std::filesystem::path p_loading = std::filesystem::u8path(path_loading.get_ptr());
		if (std::filesystem::exists(p_loading)) {
			stat_result_loading = true;
			src_length_loading = std::filesystem::file_size(p_loading);
		}
		
		std::filesystem::path p_taskreg = std::filesystem::u8path(path_taskreg.get_ptr());
		if (std::filesystem::exists(p_taskreg)) {
			stat_result_checked = true;
			src_length_checked = std::filesystem::file_size(p_taskreg);
		}

		return (stat_result_checked || src_length_checked == 0) && (!stat_result_loading);
	}

	// checks preconditions and done

	static bool is_data_avail(pfc::string8 id, pfc::string8 secid, GetFrom gfFrom, pfc::string8& out_relative_path) {

		// pre
		if (atoi(id) == pfc_infinite || !id.get_length() || !is_number(id.c_str())) return false;
		if ((gfFrom == GetFrom::Release) || (gfFrom == GetFrom::Versions)) {
			if (atoi(secid) == pfc_infinite || !secid.get_length() || !is_number(secid.c_str())) return false;
		}
		//..

		out_relative_path = GetOfflinePath(id, true, gfFrom, secid);
		bool bres = Get_FS_Id_Folder_Entries(id, gfFrom, secid).get_count();

		bres &= CheckDownload(out_relative_path);

		return bres;
	}

	//todo: tidy up

	//create offline folders for paged content and artwork
	//returns true if folder ready
	bool static CreateOfflinePath_PAGES(pfc::string8 id, art_src artSrc, size_t subpage, GetFrom getFrom, pfc::string8 secid, bool thumbs = false) {

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
		else {
			rel_path = GetOfflinePagesPath(id, subpage, true, getFrom, secid);
		}
		
		std::filesystem::path os_path = std::filesystem::u8path(rel_path.c_str());
		
		try {

			std::error_code ec;

			//it should + marked as loading
			bool bfolder_exists = std::filesystem::exists(os_path, ec);
			if (!bfolder_exists && !ec.value()) {
				bfolder_exists = std::filesystem::create_directories(os_path, ec);
			}

			return bfolder_exists;
		}
		catch (...) {
			return false;
		}
	}

	// create offline path for artist and release id

	bool static CreateOfflinePath_ID(pfc::string8 id, GetFrom getFrom, pfc::string8 secid = "") {

		PFC_ASSERT(getFrom == GetFrom::Artist || getFrom == GetFrom::Release);
		PFC_ASSERT(getFrom != GetFrom::Release || (!STR_EQUAL(id, secid) && secid.get_length()));

		pfc::string8 rel_path = GetOfflinePath(id, true, getFrom, secid);

		std::filesystem::path os_path = std::filesystem::u8path(rel_path.c_str());
		
		try {
			return std::filesystem::create_directories(os_path);
		}
		catch (...) {
			return false;
		}
	}

	template<typename key_t, typename value_t>
	class ol_cache
	{
	public:
		typedef typename std::pair<key_t, value_t> key_value_pair_t;
		typedef typename std::list<key_value_pair_t>::iterator list_iterator_t;


		// constructor

		ol_cache() : m_offlinepath("") {

			//..

		}


		/*
			serves:
			get_all_pages_offline_cache
			get_artist_offline_cache
			get_release_offline_cache
		*/


		json_t* FRead_JSON(const char* offlinepath = nullptr) const {

			pfc::string8 path = offlinepath == nullptr ? m_offlinepath : offlinepath;
	
			std::filesystem::path os_ol = std::filesystem::u8path(path.c_str());

			int srclen = -1;
			
			if (std::filesystem::exists(os_ol)) {

				srclen = std::filesystem::file_size(os_ol);
			}

			json_t* root = json_array();

			if (srclen > 0) {

				FILE* f_In;
				errno = 0;
				json_error_t json_error = {};

				if ((f_In = fopen(os_ol.string().c_str(), "r")) != NULL)
				{
					// json load

					root = json_loadf(f_In, 0, &json_error);

					//..

					fclose(f_In);
					f_In = NULL;
				}

				if (errno || strlen(json_error.text) || root == nullptr) {

					log_msg(PFC_string_formatter() << "removing non-valid offline cache file: " << path);
						
					try {

						pfc::string8 debug;
						debug << os_ol.remove_filename().c_str();
						std::filesystem::remove_all(os_ol.remove_filename());
					}
					catch (...) {
						//
					}
				}
			}
			return root;
		}


		/*
			serves:
			offline_cache_save
		*/

		bool FDump_JSON(pfc::string8 path, json_t* root) {

			const std::filesystem::path os_full = std::filesystem::u8path(path.c_str());
			const std::filesystem::path os_parent = os_full.parent_path();

			std::error_code ec;

			//folder should already be there, marked as loading
			bool bfolder_exists = std::filesystem::exists(os_parent, ec);
			if (!bfolder_exists && !ec.value()) {
				bfolder_exists = std::filesystem::create_directories(os_parent, ec);
			}
			
			if (bfolder_exists) {
			
				int flags = 0;
				FILE* fo_abplus;
				errno = 0;

				if ((fo_abplus = fopen(os_full.string().c_str(), "ab+")) != NULL)
				{
					int result = json_dumpf(root, fo_abplus, flags);
					return (fclose(fo_abplus) == 0 && result == 0);
				}		
			}

			return false;

		}

	private:

		pfc::string8 m_offlinepath;

	};
}