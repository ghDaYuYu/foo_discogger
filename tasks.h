#pragma once
#include "sqlite3.h"
#include "db_fetcher.h"

#include "discogs.h"
#include "file_info_manager.h"
#include "find_release_dialog.h"
#include "error_manager.h"
#include "tag_writer.h"

#include "track_matching_dialog.h"
#include "preview_dialog.h"
#include "preview_leading_tag_dialog.h"


using namespace Discogs;

class foo_discogs_threaded_process_callback : public threaded_process_callback, public ErrorManager
{
public:
	db_fetcher* get_dbfetcher() { return nullptr; }

protected:
	// implement task here. catches foo_discogs exceptions.
	virtual void safe_run(threaded_process_status &p_status, abort_callback &p_abort) = 0;
	
	// success callback
	virtual void on_success(HWND p_wnd) {};

	// aborted callback
	virtual void on_abort(HWND p_wnd) {};

	// error callback. display errors if return true
	virtual void on_error(HWND p_wnd) {}

private:
	void run(threaded_process_status &p_status, abort_callback &p_abort) override final;
	void on_done(HWND p_wnd, bool p_was_aborted) override final;
};

class locked_task_exception : public foo_discogs_exception
{
public:
	locked_task_exception() : foo_discogs_exception("Locked Task Exception") {}
	locked_task_exception(const char* msg) : foo_discogs_exception(msg) {}
};

class foo_discogs_locked_threaded_process_callback : public foo_discogs_threaded_process_callback
{
public:
	foo_discogs_locked_threaded_process_callback() {
		if (g_discogs->locked_operation) {
			locked_task_exception e("Operation locked");
			throw e;
		}
		g_discogs->locked_operation++;
	}

	~foo_discogs_locked_threaded_process_callback() {
		g_discogs->locked_operation--;
	}
};

class foo_discogs_write_tag_locked_threaded_process_callback : public foo_discogs_threaded_process_callback
{
public:
	foo_discogs_write_tag_locked_threaded_process_callback() {
		if (g_discogs->write_tag_locked_operation) {
			locked_task_exception e("Write tag peration locked");
			throw e;
		}
		g_discogs->write_tag_locked_operation++;
	}

	~foo_discogs_write_tag_locked_threaded_process_callback() {
		g_discogs->write_tag_locked_operation--;
	}
};

class generate_tags_task : public foo_discogs_locked_threaded_process_callback
{
public:

	generate_tags_task(CPreviewTagsDialog *preview_dialog, TagWriter_ptr tag_writer);
	generate_tags_task(CTrackMatchingDialog *release_dialog, TagWriter_ptr tag_writer, bool show_preview_dialog);
	generate_tags_task(CTagCreditDialog* credits_dialog, TagWriter_ptr tag_writer, tag_mapping_list_type* alt_mappings);
	void start();

private:

	CPreviewTagsDialog *m_preview_dialog = nullptr;
	CTrackMatchingDialog *m_track_matching_dialog = nullptr;
	CTagCreditDialog *m_credits_dialog = nullptr;

	bool m_show_preview_dialog;
	
	TagWriter_ptr m_tag_writer;
	tag_mapping_list_type* m_alt_mappings;

	void safe_run(threaded_process_status &p_status, abort_callback &p_abort) override;
	void on_success(HWND p_wnd) override;
	void on_abort(HWND p_wnd) override;
	void on_error(HWND p_wnd) override;
};


class write_tags_task : public foo_discogs_write_tag_locked_threaded_process_callback
{
public:
	write_tags_task(TagWriter_ptr tag_writer) : m_tag_writer(tag_writer) {}
	void start();

private:
	TagWriter_ptr m_tag_writer;

	void safe_run(threaded_process_status &p_status, abort_callback &p_abort) override;
	void on_success(HWND p_wnd) override;
};


class download_art_task : public foo_discogs_threaded_process_callback
{
public:
	download_art_task(pfc::string8 release_id, metadb_handle_list items, bool file_match);
	void start();

private:
	pfc::string8 release_id;
	metadb_handle_list items;
	bool m_file_match;

	void safe_run(threaded_process_status &p_status, abort_callback &p_abort) override;
};


//album_art_ids (front, back...) based art download
class download_art_paths_task : public foo_discogs_threaded_process_callback
{
public:
	download_art_paths_task(CTrackMatchingDialog* m_dialog, pfc::string8 release_id, metadb_handle_list items, pfc::array_t<GUID> album_art_ids, bool to_path_only, bool file_match);
	void start();

private:
	CTrackMatchingDialog* m_dialog;
	pfc::array_t<GUID> m_album_art_ids;
	pfc::string8 m_release_id;
	metadb_handle_list m_items;
	bool m_to_path_only; //debug sim
	bool m_file_match;

	std::vector<std::pair<pfc::string8, bit_array_bittable>> m_vres;
	void safe_run(threaded_process_status& p_status, abort_callback& p_abort) override;
	void on_success(HWND p_wnd) override;
};


class find_deleted_releases_task : public foo_discogs_locked_threaded_process_callback
{
public:
	find_deleted_releases_task(metadb_handle_list items);
	void start();

private:
	file_info_manager m_finfo_manager;
	metadb_handle_list m_items;
	metadb_handle_list m_deleted_items;
	
	void safe_run(threaded_process_status &p_status, abort_callback &p_abort) override;
	void on_success(HWND p_wnd) override;
	void on_abort(HWND p_wnd) override; 
	void on_error(HWND p_wnd) override;

	void finish();
};


class find_releases_not_in_collection_task : public foo_discogs_locked_threaded_process_callback
{
public:
	find_releases_not_in_collection_task(metadb_handle_list items);
	void start();

private:
	file_info_manager m_finfo_manager;
	metadb_handle_list items;
	metadb_handle_list missing_items;

	void safe_run(threaded_process_status &p_status, abort_callback &p_abort) override;
	void on_success(HWND p_wnd) override;
	void on_abort(HWND p_wnd) override;
	void on_error(HWND p_wnd) override;

	void finish();
};


class get_artist_process_callback : public foo_discogs_threaded_process_callback

{
public:
	get_artist_process_callback(cupdRelSrc cupdsrc, const char *artist_id)
		: m_cupdsrc(cupdsrc), m_artist_id(artist_id) {}
	void start(HWND parent);

private:
	pfc::string8 m_artist_id;
	Artist_ptr m_artist;
	cupdRelSrc m_cupdsrc;

	bool check_in();
	void check_out();

	void safe_run(threaded_process_status &p_status, abort_callback &p_abort) override;
	void on_success(HWND p_wnd) override;
	void on_error(HWND p_wnd) override;
	void on_abort(HWND p_wnd) override;
};


class get_various_artists_process_callback : public foo_discogs_threaded_process_callback

{
public:
	get_various_artists_process_callback(cupdRelSrc cupdsrc, const std::vector<size_t> & artist_ids)
		: m_cupdsrc(cupdsrc), m_artist_ids(artist_ids) {}
	void start(HWND parent);

private:
	const std::vector<size_t> & m_artist_ids;
	pfc::array_t<Artist_ptr> m_artists;
	Release_ptr m_release;
	cupdRelSrc m_cupdsrc;

	void safe_run(threaded_process_status& p_status, abort_callback& p_abort) override;
	void on_success(HWND p_wnd) override;
	void on_error(HWND p_wnd) override;
};


class search_artist_process_callback : public foo_discogs_locked_threaded_process_callback
{
public:
	search_artist_process_callback(const char* search, const int db_dc_flags);
	void start(HWND parent);

private:
	pfc::string8 m_search;
	int m_db_dc_flags;

	pfc::array_t<Artist_ptr> m_artist_exact_matches;
	pfc::array_t<Artist_ptr> m_artist_other_matches;

	void safe_run(threaded_process_status &p_status, abort_callback &p_abort) override;
	void on_success(HWND p_wnd) override;
	void on_abort(HWND p_wnd) override;
	void on_error(HWND p_wnd) override;

};


class expand_master_release_process_callback : public foo_discogs_threaded_process_callback
{
public:
	expand_master_release_process_callback(const MasterRelease_ptr& master_release, const int pos, pfc::string8 offlineArtistId) : m_master_release(master_release), m_pos(pos), m_offlineArtist_id(offlineArtistId) {}
	void start(HWND parent);

private:
	MasterRelease_ptr m_master_release;
	int m_pos;
	pfc::string8 m_offlineArtist_id;

	void safe_run(threaded_process_status& p_status, abort_callback& p_abort) override;
	void on_success(HWND p_wnd) override;
	void on_abort(HWND p_wnd) override;
	void on_error(HWND p_wnd) override;
};


class process_release_callback : public foo_discogs_threaded_process_callback
{
public:
	process_release_callback(CFindReleaseDialog *dialog, const pfc::string8 &release_id, const pfc::string8& offline_artist_id, pfc::string8 inno, const metadb_handle_list &items);
	void start(HWND parent);

private:
	TagWriter_ptr m_tag_writer;
	file_info_manager_ptr m_finfo_manager;
	metadb_handle_list m_items;

	CFindReleaseDialog *m_dialog;
	pfc::string8 m_release_id;
	pfc::string8 m_offline_artist_id;
	pfc::string8 m_inno;
	Release_ptr m_release;

	void safe_run(threaded_process_status &p_status, abort_callback &p_abort) override;
	void on_success(HWND p_wnd) override;
	void on_abort(HWND p_wnd) override;
	void on_error(HWND p_wnd) override;
};


class process_artwork_preview_callback : public foo_discogs_threaded_process_callback
{
public:
	process_artwork_preview_callback(CTrackMatchingDialog*dialog, const Release_ptr &release, const size_t img_ndx, const bool bartist, const bool onlycache, const bool get_mibs = false);
	void start(HWND parent);

private:
	MemoryBlock m_small_art;
	CTrackMatchingDialog* m_dialog;
	Release_ptr m_release;
	size_t m_img_ndx;
	bool m_bartist;
	bool m_onlycache;
	bool m_get_mibs;

	musicbrainz_info m_musicbrainz_mibs;

	void safe_run(threaded_process_status& p_status, abort_callback& p_abort) override;
	void on_success(HWND p_wnd) override;
	void on_abort(HWND p_wnd) override;
	void on_error(HWND p_wnd) override;
};


class process_file_artwork_preview_callback : public foo_discogs_threaded_process_callback
{
public:
	process_file_artwork_preview_callback(CTrackMatchingDialog* dialog, const Release_ptr& m_release, const metadb_handle_list items, const size_t img_ndx, const bool bartist);
	void start(HWND parent);

private:
	imgpairs m_small_art;
	std::pair<pfc::string8, pfc::string8> m_temp_file_names;
	CTrackMatchingDialog* m_dialog;
	pfc::string8 m_release_id;
	Release_ptr m_release;
	metadb_handle_list m_items;
	size_t m_img_ndx;
	bool m_bartist;

	void safe_run(threaded_process_status& p_status, abort_callback& p_abort) override;
	void on_success(HWND p_wnd) override;
	void on_abort(HWND p_wnd) override;
	void on_error(HWND p_wnd) override;
};


class test_oauth_process_callback : public foo_discogs_threaded_process_callback
{
public:
	test_oauth_process_callback(const pfc::string8 &token, const pfc::string8 &token_secret) : test_token(token), test_token_secret(token_secret) {}
	void start(HWND parent);

private:
	const pfc::string8 &test_token;
	const pfc::string8 &test_token_secret;

	void safe_run(threaded_process_status &p_status, abort_callback &p_abort) override;
	void on_success(HWND p_wnd) override;
	void on_error(HWND p_wnd) override;
};


class authorize_oauth_process_callback : public foo_discogs_threaded_process_callback
{
public:
	authorize_oauth_process_callback() {}
	void start(HWND parent);

private:
	void safe_run(threaded_process_status &p_status, abort_callback &p_abort) override;
};


class generate_oauth_process_callback : public foo_discogs_threaded_process_callback
{
public:
	generate_oauth_process_callback(pfc::string8 pin_code) : code(pin_code), token(nullptr) {}
	void start(HWND parent);

private:
	pfc::string8 code;
	OAuth::Token *token;

	void safe_run(threaded_process_status &p_status, abort_callback &p_abort) override;
	void on_success(HWND p_wnd) override;
};
