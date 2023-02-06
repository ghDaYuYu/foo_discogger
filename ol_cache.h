#pragma once
#include <errno.h>
#include <filesystem>
#include "jansson/jansson.h"
#include "utils.h"
#include "utils_path.h"
#include "track_matching_utils.h"

namespace Offline {

	namespace fs = std::filesystem;

	const pfc::string8 MARK_LOADING_NAME = "loading";
	const pfc::string8 MARK_CHECK_NAME = "taskreg.txt";

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

	static bool can_read(const foo_conf& conf) { return conf.cache_offline_cache_flag & CacheFlags::OC_READ; }
	static bool can_read() { return CONF.cache_offline_cache_flag & CacheFlags::OC_READ; }
	static bool can_write() { return CONF.cache_offline_cache_flag & CacheFlags::OC_WRITE; }
	static bool can_ovr() { return CONF.cache_offline_cache_flag & CacheFlags::OC_OVERWRITE; }

	static pfc::string8 get_offline_path(pfc::string8 id, GetFrom gfFrom, pfc::string8 secId, bool native) {

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

	static pfc::string8 get_thumbnail_cache_path(pfc::string8 id, art_src artSrc, bool native) {
		pfc::string8 path =  profile_path(OC_NAME, true);
		path << "\\thumbnails\\";
		path << (artSrc == art_src::alb ? "releases" :	artSrc == art_src::art ? "artists" : "unknown") << "\\" << id;
		
		return path;
	}

	static pfc::array_t<pfc::string8> get_thumbnail_cache_path_filenames(pfc::string8 id, art_src artSrc, size_t iImageList, bool native, size_t ndx) {

		pfc::array_t<pfc::string8> folders;
		pfc::array_t<pfc::string8> filenames;

		pfc::string8 n8_rel_path = get_thumbnail_cache_path(id, artSrc, native);
		pfc::string8 n8_full_path(n8_rel_path);
		n8_full_path << "\\thumb_" << (iImageList == LVSIL_NORMAL ? "150x150" : "48x48") << "_";
		
		if (ndx == pfc_infinite) {
			filenames.append_single(n8_full_path);
			return filenames;
		}
		n8_full_path << ndx << THUMB_EXTENSION;
		
		abort_callback_dummy dummy_abort;
		try {
			listFiles(n8_rel_path, folders, dummy_abort);
		}
		catch (...) {
			return filenames;
		}

		for (t_size walk = 0; walk < folders.get_size(); ++walk) {
			pfc::string8 tmp(folders[walk]);
			extract_native_path(tmp, tmp);
			if (stricmp_utf8(tmp, n8_full_path) == 0) {
				if (native) 
					filenames.append_single(tmp);
				else 
					filenames.append_single(folders[walk]);
				if (ndx != ~0) break;
			}
		}
		return filenames;
	}

	static pfc::string8 get_offline_pages_path(pfc::string8 id, size_t page, GetFrom getFrom, pfc::string8 secid, bool native) {
		
		PFC_ASSERT(getFrom == GetFrom::ArtistReleases || getFrom == GetFrom::Versions);
		PFC_ASSERT(getFrom != GetFrom::Versions || (!STR_EQUAL(id, secid) && secid.get_length()));
		
		pfc::string8 page_path(get_offline_path(id, getFrom, secid, native));
		page_path << "\\page-";
		if (page != pfc_infinite)
			page_path << page;
		
		return page_path;
	}
	static bool check_offline_entity_folder(pfc::string8 id, GetFrom getFrom, pfc::string8 secid) {

		PFC_ASSERT(getFrom == GetFrom::Artist || getFrom == GetFrom::Release || getFrom == GetFrom::ArtistReleases || getFrom == GetFrom::Versions);
		PFC_ASSERT((getFrom != GetFrom::Release && getFrom != GetFrom::Versions) || (!STR_EQUAL(id, secid) && secid.get_length()));

		pfc::string8 n8_path = get_offline_path(id, getFrom, secid, true);
		fs::path os_path = fs::u8path(n8_path.c_str());

		bool debug_path_exists = false;
		bool req_check = false;
		size_t req_files;
		size_t req_dirs;

		if (getFrom == GetFrom::ArtistReleases || getFrom == GetFrom::Versions) {
			req_files = 1;
			req_dirs = 1;
		}
		else {
			req_files = 2;
			req_dirs = 0;
		}

		try
		{
			if (fs::exists(os_path) && fs::is_directory(os_path))
			{
				fs::recursive_directory_iterator dirpos{ os_path };

				for (auto walk_dir : dirpos) {

					if (walk_dir.is_regular_file()) {
						req_files--;
					}
					else if (walk_dir.is_directory()) {
						req_dirs--;
					}

					req_check = req_files < 1 && req_dirs < 1;

					if (req_check) break;
				}
			}
		}
		catch (const fs::filesystem_error& err)
		{
			return false;
		}
		catch (const std::exception& ex)
		{
			return false;
		}
		return req_check;
	}

	//sets folder job: 'loading.' or 'TaskReg.txt' (done param value)

	bool static stamp_download(pfc::string8 fcontent, pfc::string8 path, bool done) {
		
		bool bok = false;
		
		if (!done) {

			//delete if exists

			fs::path os_path = fs::u8path(path.c_str());
			
			try {
				fs::remove_all(os_path);
			}
			catch (...) {
				return false;
			}

			//..

			//pending

			std::error_code ec;
			bool bfolder_created = fs::create_directories(os_path, ec);

			if (!ec.value() && bfolder_created) {

				os_path = fs::u8path((PFC_string_formatter() << path << "\\" << MARK_LOADING_NAME).c_str());

				int flags = 0; // JSON_ENCODE_ANY | JSON_INDENT(1);
				FILE* file;
				errno = 0;
				//ab+ : append; open or create binary file for update, writing at eof
				if ((file = fopen(os_path.string().c_str(), "ab+")) != NULL)
				{
					int w = fwrite(fcontent.get_ptr(), fcontent.get_length(), 1, file);
					bok = (w == fcontent.get_length());

					// close and return status

					return (fclose(file) == 0) && bok;
				}
			}

			bok = false;
		}
		else {

			//done

			pfc::string8 oldname(path); oldname << "\\" << MARK_LOADING_NAME;
			pfc::string8 newname(path); newname << "\\" << MARK_CHECK_NAME;
			fs::path os_old = fs::u8path(oldname.c_str());
			fs::path os_new = fs::u8path(newname.c_str());

			std::error_code ec;
			fs::rename(os_old, os_new, ec);
			bok = !ec.value();
		}

		return bok;
	}


	bool static check_download(pfc::string8 path) {
		
		pfc::string8 path_loading;
		pfc::string8 path_taskreg;
		path_loading << path << "\\" << MARK_LOADING_NAME;
		path_taskreg << path << "\\" << MARK_CHECK_NAME;

		bool stat_result_loading = false;
		bool stat_result_checked = false;
		int src_length_loading = -1;
		int src_length_checked = -1;

		fs::path p_loading = fs::u8path(path_loading.get_ptr());
		if (fs::exists(p_loading)) {
			stat_result_loading = true;
			src_length_loading = fs::file_size(p_loading);
		}
		
		fs::path p_taskreg = fs::u8path(path_taskreg.get_ptr());
		if (fs::exists(p_taskreg)) {
			stat_result_checked = true;
			src_length_checked = fs::file_size(p_taskreg);
		}

		return (stat_result_checked || src_length_checked == 0) && (!stat_result_loading);
	}

	// checks precondition and stamp

	static bool is_data_avail(pfc::string8 id, pfc::string8 secid, GetFrom gfFrom, pfc::string8& out_relative_path, bool native = true) {

		// precondition
		if (atoi(id) == pfc_infinite || !id.get_length() || !is_number(id.c_str())) return false;
		if ((gfFrom == GetFrom::Release) || (gfFrom == GetFrom::Versions)) {
			if (atoi(secid) == pfc_infinite || !secid.get_length() || !is_number(secid.c_str())) return false;
		}
		//..

		out_relative_path = get_offline_path(id, gfFrom, secid, native);
		bool bres = check_offline_entity_folder(id, gfFrom, secid);

		bres &= check_download(out_relative_path);

		return bres;
	}

	//create offline folders for paged json content/artwork
	//returns true if folder is available after check/creation
	bool static create_offline_subpage_folder(pfc::string8 id, art_src artSrc, size_t subpage, GetFrom getFrom, pfc::string8 secid, bool thumbs = false) {

		PFC_ASSERT(getFrom == GetFrom::ArtistReleases || getFrom == GetFrom::Versions);
		PFC_ASSERT(getFrom != GetFrom::Versions || (!STR_EQUAL(id, secid) && secid.get_length()));

		pfc::string8 n8_rel_path;

		if (subpage == pfc_infinite) {		
			if (!thumbs)
				n8_rel_path = get_offline_path(id, getFrom, secid, true);
			else {
				n8_rel_path = get_thumbnail_cache_path(id, artSrc, true);				
			}
		}
		else {
			n8_rel_path = get_offline_pages_path(id, subpage, getFrom, secid, true);
		}
		
		fs::path os_path = fs::u8path(n8_rel_path.c_str());
		
		try {

			std::error_code ec;

			//folder should already be there, marked as loading
			bool bfolder_exists = fs::exists(os_path, ec);

			if (!bfolder_exists && !ec.value()) {
				bfolder_exists = fs::create_directories(os_path, ec);
			}

			return bfolder_exists && !ec.value();
		}
		catch (...) {
			return false;
		}
	}

	// create offline path for artist and release ids

	bool static create_offline_entity_folder(pfc::string8 id, GetFrom getFrom, pfc::string8 secid = "") {

		PFC_ASSERT(getFrom == GetFrom::Artist || getFrom == GetFrom::Release);
		PFC_ASSERT(getFrom != GetFrom::Release || (/*!STR_EQUAL(id, secid) &&*/ secid.get_length()));

		pfc::string8 n8_rel_path = get_offline_path(id, getFrom, secid, true);

		fs::path os_path = fs::u8path(n8_rel_path.c_str());
		
		try {
			std::error_code ec;
			bool bdone = fs::create_directories(os_path, ec);
			return !ec.value();
		}
		catch (...) {
			return false;
		}
	}
	class ol_cache
	{

	public:

		// constructor

		ol_cache() {

			//..

		}
		json_t* Read_JSON(const char* offlinepath = nullptr) const {

			pfc::string8 path = offlinepath;
			pfc::string8 n8_cache_prefix = profile_path(OC_NAME, true);

			log_msg(path.replace(n8_cache_prefix, "(cache)"));

	
			fs::path os_ol = fs::u8path(path.c_str());

			int srclen = -1;
			
			if (fs::exists(os_ol)) {

				srclen = fs::file_size(os_ol);
			}

			json_t* root = json_array();

			if (srclen > 0) {

				FILE* file;
				errno = 0;
				json_error_t json_error = {};

				if ((file = fopen(os_ol.string().c_str(), "r")) != NULL)
				{
					// json load

					root = json_loadf(file, 0, &json_error);

					//..

					fclose(file);
					file = NULL;
				}

				if (errno || strlen(json_error.text) || root == nullptr) {

					log_msg(PFC_string_formatter() << "removing non-valid offline cache file: " << path);
						
					try {

						pfc::string8 debug;
						debug << os_ol.remove_filename().c_str();
						fs::remove_all(os_ol.remove_filename());
					}
					catch (...) {
						//
					}
				}
			}
			return root;
		}

		// serves offline_cache_save

		bool Dump_JSON(pfc::string8 path, json_t* root) {

			const fs::path os_full = fs::u8path(path.c_str());
			const fs::path os_parent = os_full.parent_path();

			std::error_code ec;

			//it should exist with loading stamp
			bool bfolder_exists = fs::exists(os_parent, ec);
			
			if (!bfolder_exists && !ec.value()) {

				bfolder_exists = fs::create_directories(os_parent, ec);
			}
			
			if (bfolder_exists) {
				int flags = 0; 
				FILE* file;
				errno = 0;

				if ((file = fopen(os_full.string().c_str(), "ab+")) != NULL)
				{
					int result = json_dumpf(root, file, flags);
					return (fclose(file) == 0 && result == 0);
				}
				else {
					//..
				}
			}
			return false;
		}

	private:

		//..
	};
}