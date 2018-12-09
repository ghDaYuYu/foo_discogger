#include "stdafx.h"

#include "tasks.h"

#include "foo_discogs.h"
#include "tags.h"
#include "find_release_dialog.h"
#include "release_dialog.h"
#include "configuration_dialog.h"
#include "preview_dialog.h"


void foo_discogs_threaded_process_callback::run(threaded_process_status &p_status, abort_callback &p_abort) {
	try {
		safe_run(p_status, p_abort);
	}
	catch (foo_discogs_exception &e) {
		add_error(e, true);
	}
}

void foo_discogs_threaded_process_callback::on_done(HWND p_wnd, bool p_was_aborted) {
	if (p_was_aborted) {
		on_abort(p_wnd);
	}
	else if (!errors.get_count()) {
		try {
			on_success(p_wnd);
		}
		catch (foo_discogs_exception &e) {
			add_error(e, true);
		}
	}
	if (errors.get_count()) {
		bool display = on_error(p_wnd);
		if (display) {
			display_errors();
		}
	}
}

generate_tags_task::generate_tags_task(CPreviewTagsDialog *preview_dialog, TagWriter_ptr tag_writer, bool use_update_tags) :
		tag_writer(tag_writer), preview_dialog(preview_dialog), use_update_tags(use_update_tags) {
	preview_dialog->enable(false);
}

generate_tags_task::generate_tags_task(CReleaseDialog *release_dialog, TagWriter_ptr tag_writer, bool show_preview_dialog, bool use_update_tags) :
		tag_writer(tag_writer), release_dialog(release_dialog), show_preview_dialog(show_preview_dialog), use_update_tags(use_update_tags) {
	release_dialog->enable(false);
}

void generate_tags_task::start() {
	threaded_process::g_run_modeless(
		this,
		threaded_process::flag_show_abort |
		threaded_process::flag_show_delayed |
		threaded_process::flag_show_progress |
		threaded_process::flag_show_item,
		core_api::get_main_window(),
		"Generating tags..."
	);
}

void generate_tags_task::safe_run(threaded_process_status &p_status, abort_callback &p_abort) {
	tag_writer->generate_tags(use_update_tags, p_status, p_abort);
}

void generate_tags_task::on_success(HWND p_wnd) {
	if (preview_dialog) {
		preview_dialog->cb_generate_tags();
	}
	else if (show_preview_dialog) {
		release_dialog->enable(true); 
		release_dialog->hide();
		new CPreviewTagsDialog(core_api::get_main_window(), tag_writer, use_update_tags);
	}
	else {
		g_discogs->release_dialog->destroy_all(); 
		service_ptr_t<write_tags_task> task = new service_impl_t<write_tags_task>(tag_writer);
		task->start();
	}
}

void generate_tags_task::on_abort(HWND p_wnd) {
	on_error(p_wnd);
}

bool generate_tags_task::on_error(HWND p_wnd) {
	if (preview_dialog) {
		preview_dialog->enable(true);
	}
	else {
		release_dialog->enable(true);
		release_dialog->show();
	}
	return true;
}


generate_tags_task_multi::generate_tags_task_multi(pfc::array_t<TagWriter_ptr> tag_writers, bool show_preview_dialog, bool use_update_tags) :
		tag_writers(tag_writers), show_preview_dialog(show_preview_dialog), use_update_tags(use_update_tags) {}

void generate_tags_task_multi::start() {
	threaded_process::g_run_modeless(
		this,
		threaded_process::flag_show_abort |
		threaded_process::flag_show_delayed |
		threaded_process::flag_show_progress |
		threaded_process::flag_show_item,
		core_api::get_main_window(),
		"Generating tags..."
	);
}

void generate_tags_task_multi::safe_run(threaded_process_status &p_status, abort_callback &p_abort) {
	const size_t count = tag_writers.get_count();
	for (size_t i = 0; i < count; i++) {
		p_abort.check(); 
		
		pfc::string8 formatted_release_name;
		metadb_handle_ptr item = tag_writers[i]->finfo_manager->get_item_handle(0);
		item->format_title(nullptr, formatted_release_name, g_discogs->release_name_script, nullptr);
		p_status.set_item(formatted_release_name.get_ptr());
		p_status.set_progress(i + 1, count);

		if (!tag_writers[i]->skip) {
			try {
				tag_writers[i]->generate_tags(use_update_tags, p_status, p_abort);
			}
			catch (foo_discogs_exception &e) {
				pfc::string8 error("release ");
				error << tag_writers[i]->release->id;
				add_error(error, e);
			}
		}
	}
}

void generate_tags_task_multi::on_success(HWND p_wnd) {
	if (show_preview_dialog) {
		new CPreviewTagsDialog(core_api::get_main_window(), tag_writers, use_update_tags);
	}
	else {
		service_ptr_t<write_tags_task_multi> task = new service_impl_t<write_tags_task_multi>(tag_writers);
		task->start();
	}
}

void generate_tags_task_multi::on_abort(HWND p_wnd) {
	on_error(p_wnd);
}

bool generate_tags_task_multi::on_error(HWND p_wnd) {
	return true;
}


void write_tags_task::start() {
	threaded_process::g_run_modeless(
		this,
		threaded_process::flag_show_abort |
		threaded_process::flag_show_delayed |
		threaded_process::flag_show_progress |
		threaded_process::flag_show_item,
		core_api::get_main_window(),
		"Writing tags..."
	);
}

void write_tags_task::safe_run(threaded_process_status &p_status, abort_callback &p_abort) {
	tag_writer->write_tags();
}

void write_tags_task::on_success(HWND p_wnd) {
	// TODO: combine??
	tag_writer->finfo_manager->write_infos();
	if (CONF.save_album_art || CONF.save_artist_art) {
		service_ptr_t<download_art_task> task =
			new service_impl_t<download_art_task>(tag_writer->release->id, CONF.save_album_art, CONF.save_artist_art, tag_writer->finfo_manager->items);
		task->start();
	}
}


void write_tags_task_multi::start() {
	threaded_process::g_run_modeless(
		this,
		threaded_process::flag_show_abort |
		threaded_process::flag_show_delayed |
		threaded_process::flag_show_progress |
		threaded_process::flag_show_item,
		core_api::get_main_window(),
		"Writing tags..."
	);
}

void write_tags_task_multi::safe_run(threaded_process_status &p_status, abort_callback &p_abort) {
	const size_t count = tag_writers.get_count();
	for (size_t i = 0; i < count; i++) {
		if (!tag_writers[i]->skip && tag_writers[i]->changed) {
			tag_writers[i]->write_tags();
		}
	}
}

void write_tags_task_multi::on_success(HWND p_wnd) {
	const size_t count = tag_writers.get_count();
	for (size_t i = 0; i < count; i++) {
		if (!tag_writers[i]->skip && tag_writers[i]->changed) {
			tag_writers[i]->finfo_manager->write_infos();
		}
	}
}

void write_tags_task_multi::on_abort(HWND p_wnd) {
}

bool write_tags_task_multi::on_error(HWND p_wnd) {
	return true;
}


update_art_task::update_art_task(metadb_handle_list items, bool save_album_art, bool save_artist_art) :
		save_album_art(save_album_art), save_artist_art(save_artist_art), items(items), finfo_manager(items) {
	finfo_manager.read_infos();
}

void update_art_task::start() {
	threaded_process::g_run_modeless(
		this,
		threaded_process::flag_show_abort |
		threaded_process::flag_show_delayed |
		threaded_process::flag_show_progress |
		threaded_process::flag_show_item,
		core_api::get_main_window(),
		"Updating album/artist art..."
	);
}

void update_art_task::safe_run(threaded_process_status &p_status, abort_callback &p_abort) {
	std::map<pfc::string8, bool> release_processed;
	std::map<pfc::string8, bool> artist_processed;

	const size_t item_count = finfo_manager.get_item_count();
	pfc::string8 release_id, artist_id;

	for (size_t i = 0; i < item_count && !p_abort.is_aborting(); i++) {
		p_status.set_progress(i + 1, item_count);
		try {
			metadb_handle_ptr item = finfo_manager.get_item_handle(i);
			file_info &finfo = finfo_manager.get_item(i);
			if (save_album_art) {
				try {
					g_discogs->file_info_get_tag(item, finfo, TAG_RELEASE_ID, release_id);

					pfc::string8 formatted_release_name;
					item->format_title(nullptr, formatted_release_name, g_discogs->release_name_script, nullptr);

					if (release_id.get_length()) {
						if (!release_processed[release_id]) {
							release_processed[release_id] = true;
							Release_ptr release = discogs_interface->get_release(release_id.get_ptr(), p_status, p_abort);
							g_discogs->write_album_art(release, item, formatted_release_name.get_ptr(), p_status, p_abort);
						}
					}
					else {
						pfc::string8 error(formatted_release_name);
						error << "missing tag DISCOGS_RELEASE_ID";
						add_error(error);
					}
				}
				catch (network_exception &e) {
					pfc::string8 error("release ");
					error << release_id;
					add_error(error, e, true);
					throw;
				}
				catch (foo_discogs_exception &e) {
					pfc::string8 error("release ");
					error << release_id;
					add_error(error, e);
				}
			}
			if (save_artist_art) {
				try {
					g_discogs->file_info_get_tag(item, finfo, TAG_ARTIST_ID, artist_id);

					// TODO: use artist id in better string instead
					pfc::string8 formatted_release_name;
					item->format_title(nullptr, formatted_release_name, g_discogs->release_name_script, nullptr);

					if (artist_id.get_length()) {
						if (!artist_processed[artist_id]) {
							artist_processed[artist_id] = true;
							Artist_ptr artist = discogs_interface->get_artist(artist_id.get_ptr(), false, p_status, p_abort);

							g_discogs->write_artist_art(artist, item, formatted_release_name.get_ptr(), p_status, p_abort);
						}
					}
					else {
						pfc::string8 error(formatted_release_name);
						error << "missing tag DISCOGS_ARTIST_ID";
						add_error(error);
					}
				}
				catch (network_exception &e) {
					pfc::string8 error("artist ");
					error << artist_id;
					add_error(error, e, true);
					throw;
				}
				catch (foo_discogs_exception &e) {
					pfc::string8 error("artist ");
					error << artist_id;
					add_error(error, e);
				}
			}
		}
		catch (exception_aborted) {
		}
	}
}


download_art_task::download_art_task(pfc::string8 release_id, bool save_album_art, bool save_artist_art, metadb_handle_list items) :
		release_id(release_id), save_album_art(save_album_art), save_artist_art(save_artist_art), items(items) {
}

void download_art_task::start() {
	threaded_process::g_run_modeless(
		this,
		threaded_process::flag_show_abort |
		threaded_process::flag_show_delayed |
		threaded_process::flag_show_progress |
		threaded_process::flag_show_item,
		core_api::get_main_window(),
		"Downloading album/artist art..."
	);
}

void download_art_task::safe_run(threaded_process_status &p_status, abort_callback &p_abort) {
	Release_ptr release = discogs_interface->get_release(release_id.get_ptr(), p_status, p_abort);
	if (save_album_art) {
		pfc::string8 formatted_release_name;
		items[0]->format_title(nullptr, formatted_release_name, g_discogs->release_name_script, nullptr);
		g_discogs->write_album_art(release, items[0], formatted_release_name.get_ptr(), p_status, p_abort);
	}
	if (save_artist_art) {
		pfc::string8 formatted_release_name;
		items[0]->format_title(nullptr, formatted_release_name, g_discogs->release_name_script, nullptr);
		g_discogs->write_artist_art(release, items[0], formatted_release_name.get_ptr(), p_status, p_abort);
	}
}


find_deleted_releases_task::find_deleted_releases_task(metadb_handle_list items) : items(items), finfo_manager(items) {
	finfo_manager.read_infos();
}

void find_deleted_releases_task::start() {
	threaded_process::g_run_modeless(
		this,
		threaded_process::flag_show_abort |
		threaded_process::flag_show_delayed |
		threaded_process::flag_show_progress |
		threaded_process::flag_show_item,
		core_api::get_main_window(),
		"Finding deleted releases..."
	);
}

void find_deleted_releases_task::safe_run(threaded_process_status &p_status, abort_callback &p_abort) {
	const size_t item_count = finfo_manager.get_item_count();

	pfc::string8 release_id;
	pfc::array_t<pfc::string8> finished;
	size_t finished_count = 0;

	for (size_t i = 0; i < item_count && !p_abort.is_aborting(); i++) {
		metadb_handle_ptr item = finfo_manager.get_item_handle(i);
		pfc::string8 formatted_release_name;
		item->format_title(nullptr, formatted_release_name, g_discogs->release_name_script, nullptr);

		p_status.set_item(formatted_release_name.get_ptr());
		p_status.set_progress(i + 1, item_count);

		file_info &finfo = finfo_manager.get_item(i);

		if (!g_discogs->file_info_get_tag(item, finfo, TAG_RELEASE_ID, release_id)) {
			continue;
		}

		bool duplicate = false;
		for (size_t j = 0; j < finished_count; j++) {
			if (STR_EQUAL(finished[j], release_id)) {
				duplicate = true;
				break;
			}
		}
		if (duplicate) {
			continue;
		}

		finished.append_single(release_id);
		finished_count++;

		try {
			discogs_interface->get_release(release_id, p_status, p_abort, true, true);
		}
		catch (http_404_exception) {
			deleted_items.add_item(item);
		}
		catch (network_exception &e) {
			pfc::string8 error("release ");
			error << release_id;
			add_error(error, e, true);
			return;
		}
		catch (foo_discogs_exception &e) {
			pfc::string8 error("release ");
			error << release_id;
			add_error(error, e); //true);
		}
	}
}

void find_deleted_releases_task::on_success(HWND p_wnd) {
	finish();
}

void find_deleted_releases_task::on_abort(HWND p_wnd) {
}

bool find_deleted_releases_task::on_error(HWND p_wnd) {
	finish();
	return true;
}

void find_deleted_releases_task::finish() {
	if (deleted_items.get_count() > 0) {
		static_api_ptr_t<playlist_manager> pm;
		size_t playlist_id = pm->create_playlist("foo_discogs: invalid release ids", ~0, ~0);

		bit_array_true dummy;
		pm->playlist_insert_items(playlist_id, 0, deleted_items, dummy);
		pm->set_active_playlist(playlist_id);
	}
}


find_releases_not_in_collection_task::find_releases_not_in_collection_task(metadb_handle_list items) : items(items), finfo_manager(items) {
	finfo_manager.read_infos();
}

void find_releases_not_in_collection_task::start() {
	threaded_process::g_run_modeless(
		this,
		threaded_process::flag_show_abort |
		threaded_process::flag_show_delayed |
		threaded_process::flag_show_progress |
		threaded_process::flag_show_item,
		core_api::get_main_window(),
		"Finding releases not in collection..."
		);
}

void find_releases_not_in_collection_task::safe_run(threaded_process_status &p_status, abort_callback &p_abort) {
	p_status.set_progress(0, 1);
	
	const pfc::array_t<pfc::string8> &collection = discogs_interface->get_collection(p_status, p_abort);
	const size_t collection_count = collection.get_count();

	p_status.set_item("Checking for missing releases...");
	p_status.set_progress(1, 1);
	
	const size_t item_count = finfo_manager.get_item_count();
	pfc::string8 release_id;

	for (size_t i = 0; i < item_count && !p_abort.is_aborting(); i++) {
		file_info &finfo = finfo_manager.get_item(i);
		metadb_handle_ptr item = finfo_manager.get_item_handle(i);

		if (!g_discogs->file_info_get_tag(item, finfo, TAG_RELEASE_ID, release_id)) {
			continue;
		}

		bool found = false;
		for (size_t j = 0; j < collection_count; j++) {
			if (STR_EQUAL(collection[j], release_id)) {
				found = true;
				break;
			}
		}
		if (!found) {
			missing_items.add_item(item);
		}
	}
}

void find_releases_not_in_collection_task::on_success(HWND p_wnd) {
	finish();
}

void find_releases_not_in_collection_task::on_abort(HWND p_wnd) {
}

bool find_releases_not_in_collection_task::on_error(HWND p_wnd) {
	finish();
	return true;
}

void find_releases_not_in_collection_task::finish() {
	if (missing_items.get_count() > 0) {
		static_api_ptr_t<playlist_manager> pm;
		size_t playlist_id = pm->create_playlist("foo_discogs: not in collection", ~0, ~0);

		bit_array_true dummy;
		pm->playlist_insert_items(playlist_id, 0, missing_items, dummy);
		pm->set_active_playlist(playlist_id);
	}
}


update_tags_task::update_tags_task(metadb_handle_list items, bool use_update_tags) :
		use_update_tags(use_update_tags), finfo_manager(items) {
	finfo_manager.read_infos();
}

void update_tags_task::start() {
	threaded_process::g_run_modeless(
		this,
		threaded_process::flag_show_abort |
		threaded_process::flag_show_delayed |
		threaded_process::flag_show_progress |
		threaded_process::flag_show_item,
		core_api::get_main_window(),
		"Preparing to update tags..."
	);
}

void update_tags_task::safe_run(threaded_process_status &p_status, abort_callback &p_abort) {
	const size_t total_items = finfo_manager.get_item_count();
	std::map<pfc::string8, pfc::array_t<size_t>> metadb_lists;
	pfc::array_t<pfc::string8> releases;

	for (size_t i = 0; i < total_items; i++) {
		metadb_handle_ptr item = finfo_manager.get_item_handle(i);
		file_info &finfo = finfo_manager.get_item(i);
		
		const char *ex_release_id = finfo.meta_get(TAG_RELEASE_ID, 0);
		if (ex_release_id == NULL) {
			continue;
		}

		pfc::string8 release_id(ex_release_id);

		auto it = metadb_lists.find(release_id);
		if (it != metadb_lists.cend()) {
			it->second.append_single(i);
		}
		else {
			metadb_lists[release_id] = pfc::array_t<size_t>();
			metadb_lists[release_id].append_single(i);
			releases.append_single(release_id);
		}
	}

	const size_t count = releases.get_count();
	for (size_t n = 0; n < count; n++) {
		const pfc::string8 &release_id = releases[n];
		const pfc::array_t<size_t> &stuff = metadb_lists[release_id];

		pfc::string8 formatted_release_name;
		metadb_handle_ptr item = finfo_manager.get_item_handle(stuff[0]);
		item->format_title(nullptr, formatted_release_name, g_discogs->release_name_script, nullptr);
		p_status.set_item(formatted_release_name.get_ptr());
		p_status.set_progress(n + 1, count);

		file_info_manager_ptr sub_manager = std::make_shared<file_info_manager>();
		for (size_t i = 0; i < stuff.get_count(); i++) {
			sub_manager->copy_from(finfo_manager, stuff[i]);
		}

		TagWriter_ptr tag_writer = std::make_shared<TagWriter>(sub_manager, discogs_interface->get_release(release_id, p_status, p_abort, false, true));
		tag_writer->match_tracks();
		tag_writers.append_single(tag_writer);
	}
}

void update_tags_task::on_success(HWND p_wnd) {
	new CReleaseDialog(core_api::get_main_window(), tag_writers, use_update_tags); 
	finish();
}

void update_tags_task::on_abort(HWND p_wnd) {
}

bool update_tags_task::on_error(HWND p_wnd) {
	finish();
	return true;
}

void update_tags_task::finish() {
}


void get_artist_process_callback::start(HWND parent) {
	threaded_process::g_run_modeless(this,
		threaded_process::flag_show_item |
		threaded_process::flag_show_abort |
		threaded_process::flag_show_delayed,
		parent,
		"Loading artist releases..."
	);
}

void get_artist_process_callback::safe_run(threaded_process_status &p_status, abort_callback &p_abort) {
	m_artist = discogs_interface->get_artist(m_artist_id, true, p_status, p_abort);
	p_status.set_item("Formatting artist releases...");
}

void get_artist_process_callback::on_success(HWND p_wnd) {
	if (g_discogs->find_release_dialog && m_artist) {
		g_discogs->find_release_dialog->on_get_artist_done(m_artist);
	}
}


void search_artist_process_callback::start(HWND parent) {
	threaded_process::g_run_modeless(this,
		threaded_process::flag_show_item |
		threaded_process::flag_show_abort |
		threaded_process::flag_show_delayed,
		parent,
		"Searching artist..."
	);
}

void search_artist_process_callback::safe_run(threaded_process_status &p_status, abort_callback &p_abort) {
	p_status.set_item("Fetching artist list...");
	discogs_interface->search_artist(m_search, m_artist_exact_matches, m_artist_other_matches, p_status, p_abort);
}

void search_artist_process_callback::on_success(HWND p_wnd) {
	if (g_discogs->find_release_dialog) {
		g_discogs->find_release_dialog->on_search_artist_done(m_artist_exact_matches, m_artist_other_matches);
	}
}


void expand_master_release_process_callback::start(HWND parent) {
	threaded_process::g_run_modeless(
		this,
		threaded_process::flag_show_item |
		threaded_process::flag_show_abort |
		threaded_process::flag_show_delayed,
		parent,
		"Expanding master release..."
		);
}

void expand_master_release_process_callback::safe_run(threaded_process_status &p_status, abort_callback &p_abort) {
	p_status.set_item("Expanding master release...");
	m_master_release->load_releases(p_status, p_abort);
	if (g_discogs->find_release_dialog) {
		g_discogs->find_release_dialog->on_expand_master_release_done(m_master_release, m_pos, p_status, p_abort);
	}
}

void expand_master_release_process_callback::on_success(HWND p_wnd) {
	if (g_discogs->find_release_dialog) {
		g_discogs->find_release_dialog->on_expand_master_release_complete();
	}
}

void expand_master_release_process_callback::on_abort(HWND p_wnd) {
	if (g_discogs->find_release_dialog) {
		g_discogs->find_release_dialog->on_expand_master_release_complete();
	}
}

bool expand_master_release_process_callback::on_error(HWND p_wnd) {
	if (g_discogs->find_release_dialog) {
		g_discogs->find_release_dialog->on_expand_master_release_complete();
	}
	return true;
}



process_release_callback::process_release_callback(CFindReleaseDialog *dialog, const pfc::string8 &release_id, const metadb_handle_list &items) :
		items(items), m_dialog(dialog), m_release_id(release_id) {
	m_dialog->enable(false);
	finfo_manager = std::make_shared<file_info_manager>(items);
	finfo_manager->read_infos();
}

void process_release_callback::start(HWND parent) {
	threaded_process::g_run_modeless(this,
		threaded_process::flag_show_item | 
		threaded_process::flag_show_abort,
		parent,
		"Processing release..."
	);
}

void process_release_callback::safe_run(threaded_process_status &p_status, abort_callback &p_abort) {
	p_status.set_item("Fetching release information...");
	Release_ptr release = discogs_interface->get_release(m_release_id, p_status, p_abort);

	if (release) {
		m_release = release;

		if (release->images.get_size() && CONF.display_art) {
			if (!release->images[0]->url150.get_length()) {
				foo_discogs_exception ex;
				ex << "Image URLs unavailable - Is OAuth working?";
				throw ex;
			}

			p_status.set_item("Fetching small album art...");
			discogs_interface->fetcher->fetch_url(release->images[0]->url150, "", release->small_art, p_abort, false);
		}
	}

	m_dialog->enable(true);
	m_dialog->hide();

	tag_writer = std::make_shared<TagWriter>(finfo_manager, m_release);
	tag_writer->match_tracks();
}

void process_release_callback::on_success(HWND p_wnd) {
	new CReleaseDialog(core_api::get_main_window(), tag_writer, false);
}

void process_release_callback::on_abort(HWND p_wnd) {
	m_dialog->enable(true);
}

bool process_release_callback::on_error(HWND p_wnd) {
	m_dialog->enable(true);
	return true;
}


void test_oauth_process_callback::start(HWND parent) {
	threaded_process::g_run_modal(this,
		threaded_process::flag_show_item |
		threaded_process::flag_show_abort,
		parent,
		"Testing Discogs OAuth support..."
	);
}

void test_oauth_process_callback::safe_run(threaded_process_status &p_status, abort_callback &p_abort) {
	if (test_token.get_length() && test_token_secret.get_length()) {
		p_status.set_item("Testing OAuth identity...");
		discogs_interface->fetcher->test_oauth(test_token, test_token_secret, p_abort);
	}
	else {
		add_error("Error: You must generate OAuth tokens first.");
	}
}

void test_oauth_process_callback::on_success(HWND p_wnd) {
	uMessageBox(nullptr, "OAuth is working!", "Success!", MB_OK);
}


void authorize_oauth_process_callback::start(HWND parent) {
	threaded_process::g_run_modal(this,
		threaded_process::flag_show_item |
		threaded_process::flag_show_abort,
		parent,
		"Requesting Discogs OAuth Authorization Code..."
	);
}

void authorize_oauth_process_callback::safe_run(threaded_process_status &p_status, abort_callback &p_abort) {
	p_status.set_item("Authorizing OAuth...");
	pfc::string8 auth_url = discogs_interface->fetcher->get_oauth_authorize_url(p_abort);
	display_url(auth_url);
}


void generate_oauth_process_callback::start(HWND parent) {
	threaded_process::g_run_modal(this,
		threaded_process::flag_show_item |
		threaded_process::flag_show_abort,
		parent,
		"Generating Discogs OAuth tokens..."
	);
}

void generate_oauth_process_callback::safe_run(threaded_process_status &p_status, abort_callback &p_abort) {
	p_status.set_item("Generating OAuth tokens...");
	token = discogs_interface->fetcher->generate_oauth(code, p_abort);
	if (token == nullptr) {
		add_error("Error: Invalid PIN Code.");
	}
}

void generate_oauth_process_callback::on_success(HWND p_wnd) {
	uSetWindowText(g_discogs->configuration_dialog->token_edit, token->key().c_str());
	uSetWindowText(g_discogs->configuration_dialog->secret_edit, token->secret().c_str());
	delete token;
	token = nullptr;
}
