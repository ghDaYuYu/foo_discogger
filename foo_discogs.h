#pragma once

#pragma warning(disable:4996)

#include "discogs_interface.h"
#include "tags.h"
#include "error_manager.h"
#include "string_encoded_array.h"
#include "art_download_attribs.h"

using namespace Discogs;

const UINT WM_CUSTOM_ANV_CHANGED = WM_USER + 100;

class CFindReleaseDialog;
class CTrackMatchingDialog;
class CPreviewTagsDialog;
class CNewTagMappingsDialog;
class CConfigurationDialog;
class contextmenu_discogs;
class CUpdateArtDialog;
class CUpdateTagsDialog;
class process_release_callback;
class get_artist_process_callback;
class search_artist_process_callback;
class edit_complete;


class foo_discogs : public ErrorManager
{
public:
	enum discog_web_page
	{
		ARTIST_PAGE,
		ARTIST_ART_PAGE,
		RELEASE_PAGE,
		MASTER_RELEASE_PAGE,
		LABEL_PAGE,
		ALBUM_ART_PAGE,
	};

	CFindReleaseDialog *find_release_dialog = nullptr;
	CTrackMatchingDialog *track_matching_dialog = nullptr;
	CPreviewTagsDialog *preview_tags_dialog = nullptr;
	CNewTagMappingsDialog *tag_mappings_dialog = nullptr;
	CConfigurationDialog *configuration_dialog = nullptr;
	CUpdateArtDialog *update_art_dialog = nullptr;
	CUpdateTagsDialog *update_tags_dialog = nullptr;
	size_t locked_operation = 0;

	void write_album_art(Release_ptr &release,
		metadb_handle_ptr item,
		const char *item_text,
		threaded_process_status &p_status,
		abort_callback &p_abort);

	void save_album_art(Release_ptr &release, metadb_handle_ptr item, art_download_attribs ada, pfc::array_t<GUID> album_art_ids, bit_array_bittable &saved_mask, pfc::array_t<pfc::string8> &done_files, threaded_process_status &p_status, abort_callback &p_abort);
	void save_artist_art(Release_ptr &release, metadb_handle_ptr item, art_download_attribs ada, pfc::array_t<GUID> album_art_ids, bit_array_bittable& saved_mask, pfc::array_t<pfc::string8> &done_files, threaded_process_status &p_status, abort_callback &p_abort);
	void save_artist_art(Artist_ptr &artist, Release_ptr &release, metadb_handle_ptr item, art_download_attribs ada, pfc::array_t<GUID> album_art_ids, bit_array_bittable& saved_mask, pfc::array_t<pfc::string8> &done_files, threaded_process_status &p_status, abort_callback &p_abort);

	void fetch_image(MemoryBlock &buffer, Image_ptr &image, abort_callback &p_abort);
	void write_image(MemoryBlock &buffer, const pfc::string8 &full_path, abort_callback &p_abort);
	void embed_image(MemoryBlock &buffer, metadb_handle_ptr item, GUID embed_guid, abort_callback &p_abort);

	bool gave_oauth_warning = false;

	foo_discogs();
	~foo_discogs();

	service_ptr_t<titleformat_object> release_name_script;

	void item_display_web_page(const metadb_handle_ptr item, discog_web_page web_page);
	bool item_has_tag(const metadb_handle_ptr item, const char *tag, const char *backup_tag=nullptr);
	bool some_item_has_tag(const metadb_handle_list items, const char *tag);

	bool file_info_get_tag(const metadb_handle_ptr item, file_info &finfo, const char* tag, pfc::string8 &value, const char *backup_tag=nullptr);

	const ReleaseDisc_ptr& file_info_get_discogs_disc(file_info &finfo, const metadb_handle_ptr item, const Release_ptr &release, size_t &track_num);
	const ReleaseDisc_ptr& get_discogs_disc(const Release_ptr &release, size_t pos, size_t &track_num);
};

extern foo_discogs *g_discogs;
