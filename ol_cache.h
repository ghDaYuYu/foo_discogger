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

		PFC_ASSERT(getFrom == GetFrom::Artist || getFrom == GetFrom::Release);
		PFC_ASSERT(getFrom != GetFrom::Release || (!STR_EQUAL(id, secid) && secid.get_length()));

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

		pfc::string8 path_native(folders[0]);
		extract_native_path(path_native, path_native);
		paths.append_single(path_native);
		return paths;
	}

	bool static MarkDownload(pfc::string8 fcontent, pfc::string8 path, bool done) {
		
		bool bok = false;
		
		if (!done) {
			
			//delete prev RegTask
			pfc::string8 taskreg_path;
			taskreg_path << path << "\\" << MARK_CHECK_NAME;

			std::filesystem::path os_path = std::filesystem::u8path(path.c_str());
			
			try {
				std::filesystem::remove_all(os_path);
			}
			catch (...) {
				return false;
			}
			//..

			os_path = std::filesystem::u8path((PFC_string_formatter() << path << "\\" << MARK_LOADING_NAME).c_str());
            
            FILE* f = fopen(os_path.string().c_str(), "ab+");
            if (f) {
                bok = (fwrite(fcontent.get_ptr(), fcontent.get_length(), 1, f) == 1);					
            }

            bok &= (f && (fclose(f) == 0));
        }
        else {
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

	static bool is_data_avail(pfc::string8 id, pfc::string8 secid, GetFrom gfFrom, pfc::string8& out_relative_path) {
		out_relative_path = GetOfflinePath(/*artist_id*/id, /*native*/ true, /*ol::GetFrom::Release*/gfFrom, /*release_id*/secid);
		return (GetFS_IdFilePaths(id, gfFrom, secid).get_count()) && CheckDownload(out_relative_path);
	}

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
		else {
			rel_path = GetOfflinePagesPath(id, subpage, true, getFrom, secid);
		}
		
		std::filesystem::path os_path = std::filesystem::u8path(rel_path.c_str());
		
		try {
			return std::filesystem::create_directories(os_path);
		}
		catch (...) {
			return false;
		}
	}

	bool static CreateFS_Offline_IdPath(pfc::string8 id, GetFrom getFrom, pfc::string8 secid = "") {

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

		ol_cache(size_t max_size) :
			_max_size(max_size) {
		}

		json_t* FReadAll(const char* offlinepath = nullptr) const {
        
            pfc::string8 path = offlinepath == nullptr ? m_offlinepath : offlinepath;
            log_msg(pfc::string8("reading offline:") << path);
            
            std::filesystem::path os_ol = std::filesystem::u8path(path.c_str());
    
            int srclen = -1;
            if (std::filesystem::exists(os_ol))
                srclen = std::filesystem::file_size(os_ol);

            //load or reset array
            json_t* root = json_array();
            FILE* fOut;

            if (srclen > 0) {
                fOut = fopen(os_ol.string().c_str(), "r");
                json_error_t error = {};
                root = json_loadf(fOut, 0, &error);
                fclose(fOut); fOut = NULL;
                if (strlen(error.text)) {

                    log_msg(PFC_string_formatter() << "deleting invalid offline cache file: " << path);
                    
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

        bool FDumpToFolder(pfc::string8 path, json_t* root) {
    
            std::filesystem::path os_path = std::filesystem::u8path(path.c_str());
    
            int flags = 0; // JSON_ENCODE_ANY | JSON_INDENT(1);
            FILE* fOut = fopen(os_path.string().c_str(), "ab+");
            if (!fOut)
                return false;

            int result = json_dumpf(root, fOut, flags);
            return (fclose(fOut) == 0 && result == 0);
        }

	private:
		std::list<key_value_pair_t> _cache_items_list;
		std::unordered_map<key_t, list_iterator_t> _cache_items_map;

		pfc::string8 m_offlinepath;
		size_t _max_size;
	};
}
