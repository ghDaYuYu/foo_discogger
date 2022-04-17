#include "stdafx.h"
#include "ol_cache.h"
namespace Offline {
	
	bool CheckOfflineFolder_ID(pfc::string8 id, GetFrom getFrom, pfc::string8 secid) {

		PFC_ASSERT(getFrom == GetFrom::Artist || getFrom == GetFrom::Release || getFrom == GetFrom::ArtistReleases || getFrom == GetFrom::Versions);
		PFC_ASSERT((getFrom != GetFrom::Release && getFrom != GetFrom::Versions) || (!STR_EQUAL(id, secid) && secid.get_length()));

		pfc::string8 native_path = GetOfflinePath(id, true, getFrom, secid);
		std::filesystem::path fs_path = std::filesystem::u8path(native_path.c_str());

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
			if (fs::exists(fs_path) && fs::is_directory(fs_path))
			{
				std::filesystem::recursive_directory_iterator dirpos{ fs_path };

				for (auto entry : dirpos) {
					if (entry.is_regular_file()) req_files--;
					else if (entry.is_directory())  req_dirs--;
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
}