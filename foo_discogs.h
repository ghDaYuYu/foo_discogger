#pragma once

#pragma warning(disable:4996)

#include "version.h"
#include "discogs_interface.h"
#include "tags.h"
#include "error_manager.h"
#include "string_encoded_array.h"
#include "art_download_attribs.h"

using namespace Discogs;

const UINT WM_CUSTOM_ANV_CHANGED = WM_USER + 100;

class CFindReleaseDialog;
class CFindReleaseArtistDialog;
class CTrackMatchingDialog;
class CPreviewTagsDialog;
class CPreviewLeadingTagDialog;
class CTagMappingDialog;
class CTagCreditDialog;
class CConfigurationDialog;
class CUpdateArtDialog;
class CUpdateTagsDialog;
class contextmenu_discogs;
class process_release_callback;
class get_artist_process_callback;
class get_various_artists_process_callback;
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

	HICON icon = nullptr;

	CFindReleaseDialog *find_release_dialog = nullptr;
	CFindReleaseArtistDialog* find_release_artist_dialog = nullptr;
	CTrackMatchingDialog *track_matching_dialog = nullptr;
	CPreviewTagsDialog *preview_tags_dialog = nullptr;
	CPreviewLeadingTagDialog*preview_modal_tag_dialog = nullptr;
	CTagMappingDialog *tag_mappings_dialog = nullptr;
	CTagCreditDialog *tag_credit_dialog = nullptr;
	CConfigurationDialog *configuration_dialog = nullptr;

	size_t locked_operation = 0;
	size_t write_tag_locked_operation = 0;
	std::vector<pfc::string8> vArtistLoadReleasesTasks;

	void save_album_art(Release_ptr &release, metadb_handle_ptr item, art_download_attribs ada, pfc::array_t<GUID> album_art_ids, bit_array_bittable &saved_mask, pfc::array_t<pfc::string8> &done_files, std::map<pfc::string8, MemoryBlock> &done_fetches, threaded_process_status &p_status, abort_callback &p_abort);
	void save_artist_art(Release_ptr &release, metadb_handle_ptr item, art_download_attribs ada, pfc::array_t<GUID> album_art_ids, bit_array_bittable& saved_mask, pfc::array_t<pfc::string8> &done_files, std::map<pfc::string8, MemoryBlock>& done_fetches, threaded_process_status &p_status, abort_callback &p_abort);
	void save_artist_art(pfc::array_t<Artist_ptr> &artists, Release_ptr &release, metadb_handle_ptr item, art_download_attribs ada, pfc::array_t<GUID> album_art_ids, bit_array_bittable& saved_mask, pfc::array_t<pfc::string8> &done_files, std::map<pfc::string8, MemoryBlock>& done_fetches, threaded_process_status &p_status, abort_callback &p_abort);

	void fetch_image(MemoryBlock &buffer, Image_ptr &image, abort_callback &p_abort);
	void write_image(MemoryBlock &buffer, const pfc::string8 &full_path, abort_callback &p_abort);
	void embed_image(MemoryBlock &buffer, metadb_handle_ptr item, GUID embed_guid, abort_callback &p_abort);

	bool gave_oauth_warning = false;

	foo_discogs();
	~foo_discogs();

	service_ptr_t<titleformat_object> release_name_script;

	service_ptr_t<titleformat_object> dc_artist_id_script;

	service_ptr_t<titleformat_object> dc_release_id_script;

	void item_display_web_page(const metadb_handle_ptr item, discog_web_page web_page);
	bool item_has_tag(const metadb_handle_ptr item, const char *tag, const char *backup_tag=nullptr);
	bool some_item_has_tag(const metadb_handle_list items, const char *tag);

	bool file_info_get_tag(const metadb_handle_ptr item, file_info &finfo, const char* tag, pfc::string8 &value, const char *backup_tag=nullptr);
	const ReleaseDisc_ptr& get_discogs_disc(const Release_ptr &release, size_t pos, size_t &track_num);
};

inline bool g_os_is_wine = false;
inline foo_discogs* g_discogs = nullptr;

inline HICON g_hIcon_quian;
inline HBITMAP g_hIcon_rec;
inline HFONT g_hFont;

enum class PreView :int { Normal = 0, Diff, Original, Dbg, Undef, default = 0 };
inline pfc::string8 preview_to_label(PreView pv) { return pv == PreView::Normal ? "Results" : pv == PreView::Diff ? "Difference" :
	pv == PreView::Original ? "Original" : pv == PreView::Dbg ? "Debug" : "Undefine"; }

enum class updRelSrc { Artist, Releases, Filter, ArtistList, ArtistListCheckExact, ArtistProfile, ArtistSearch, Undef, UndefFast };

class cupdRelSrc {

public:

	cupdRelSrc(updRelSrc src) : src(src), extended(false), oninit(false) {};

	friend bool operator == (const cupdRelSrc& lhs, const cupdRelSrc& rhs);
	friend bool operator == (const cupdRelSrc& lhs, const updRelSrc& rhs);
	// implicit conversion
	operator updRelSrc() const { return src; }

	bool extended;
	bool oninit;

private:

	updRelSrc src;

};

inline bool operator==(const cupdRelSrc& lhs, const cupdRelSrc& rhs) { return lhs.src == rhs.src; }
inline bool operator==(const cupdRelSrc& lhs, const updRelSrc& rhs) { return lhs.src == rhs; }
inline bool operator!=(const cupdRelSrc& lhs, const cupdRelSrc& rhs) { return !(lhs == rhs); }
inline bool operator!=(const cupdRelSrc& lhs, const updRelSrc& rhs) { return !(lhs == rhs); }
