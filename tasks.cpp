#include "stdafx.h"

#include "tasks.h"

#include "foo_discogs.h"
#include "tags.h"
#include "art_download_attribs.h"
#include "ol_cache.h"

#include "configuration_dialog.h"


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
	else {
		if (!fatal_error) {
			try {
				on_success(p_wnd);
			}
			catch (foo_discogs_exception &e) {
				add_error(e, true);
			}
		}
		if (errors.get_count()) {
			display_errors();
			if (fatal_error) {
				on_error(p_wnd);
			}
		}
	}
}

generate_tags_task::generate_tags_task(CPreviewTagsDialog *preview_dialog, TagWriter_ptr tag_writer, bool use_update_tags) :
		tag_writer(tag_writer), preview_dialog(preview_dialog), show_preview_dialog(NULL), use_update_tags(use_update_tags) {
	preview_dialog->enable(false, true);
}

generate_tags_task::generate_tags_task(CTrackMatchingDialog *track_matching_dialog, TagWriter_ptr tag_writer, bool show_preview_dialog, bool use_update_tags) :
		tag_writer(tag_writer), track_matching_dialog(track_matching_dialog), show_preview_dialog(show_preview_dialog), use_update_tags(use_update_tags) {
	track_matching_dialog->enable(false);
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
		track_matching_dialog->enable(true);
		track_matching_dialog->hide();
		fb2k::newDialog <CPreviewTagsDialog>(core_api::get_main_window(), tag_writer, use_update_tags);
	}
	else {
		CTrackMatchingDialog* dlg = reinterpret_cast<CTrackMatchingDialog*>(g_discogs->track_matching_dialog);
		dlg->destroy_all();
		service_ptr_t<write_tags_task> task = new service_impl_t<write_tags_task>(tag_writer);
		task->start();
	}
}

void generate_tags_task::on_abort(HWND p_wnd) {
	on_error(p_wnd);
}

void generate_tags_task::on_error(HWND p_wnd) {
	if (preview_dialog) {
		preview_dialog->enable(true, true);
	}
	else {
		track_matching_dialog->enable(true);
		track_matching_dialog->show();
	}
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

		if (!tag_writers[i]->skip) {
			pfc::string8 formatted_release_name;
			metadb_handle_ptr item = tag_writers[i]->finfo_manager->get_item_handle(0);
			item->format_title(nullptr, formatted_release_name, g_discogs->release_name_script, nullptr);
			p_status.set_item(formatted_release_name.get_ptr());
			p_status.set_progress(i + 1, count);

			try {
				tag_writers[i]->generate_tags(use_update_tags, p_status, p_abort);
			}
			catch (foo_discogs_exception &e) {
				pfc::string8 error("release ");
				error << tag_writers[i]->release->id;
				add_error(error, e);
			}
		}
		else if (!STR_EQUAL(tag_writers[i]->error, "")) {
			add_error(tag_writers[i]->error);
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

void generate_tags_task_multi::on_error(HWND p_wnd) {
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
	if (CONF.save_album_art || CONF.save_artist_art || CONF.embed_album_art || CONF.embed_artist_art) {
		service_ptr_t<download_art_task> task =
			new service_impl_t<download_art_task>(tag_writer->release->id, tag_writer->finfo_manager->items);
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
		if (!tag_writers[i]->skip && tag_writers[i]->will_modify) {
			tag_writers[i]->write_tags();
		}
	}
}

void write_tags_task_multi::on_success(HWND p_wnd) {
	const size_t count = tag_writers.get_count();
	file_info_manager_ptr super_manager = std::make_shared<file_info_manager>();
	for (size_t i = 0; i < count; i++) {
		if (!tag_writers[i]->skip && tag_writers[i]->will_modify) {
			for (size_t j = 0; j < tag_writers[i]->finfo_manager->get_item_count(); j++) {
				super_manager->copy_from(*(tag_writers[i]->finfo_manager), j);
			}
		}
	}
	super_manager->write_infos();
}

void write_tags_task_multi::on_abort(HWND p_wnd) {
}

void write_tags_task_multi::on_error(HWND p_wnd) {
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
	std::map<pfc::string8, bool> artist_processed;

	const size_t item_count = finfo_manager.get_item_count();
	pfc::string8 release_id, artist_id;

	pfc::array_t<pfc::string8> done_release_files;
	pfc::array_t<pfc::string8> done_artist_files;

	for (size_t i = 0; i < item_count && !p_abort.is_aborting(); i++) {
		p_status.set_progress(i + 1, item_count);
		try {
			metadb_handle_ptr item = finfo_manager.get_item_handle(i);
			file_info &finfo = finfo_manager.get_item(i);
			
			Release_ptr release = nullptr;
			try {
				g_discogs->file_info_get_tag(item, finfo, TAG_RELEASE_ID, release_id);

				if (release_id.get_length()) {
					release = discogs_interface->get_release(release_id.get_ptr(), p_status, p_abort);
					if (save_album_art) {
						g_discogs->save_album_art(release, item, done_release_files, p_status, p_abort);
					}
				}
				else {
					pfc::string8 formatted_release_name;
					item->format_title(nullptr, formatted_release_name, g_discogs->release_name_script, nullptr);
					formatted_release_name << "missing tag DISCOGS_RELEASE_ID";
					add_error(formatted_release_name);
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

			if (save_artist_art) {
				try {
					g_discogs->file_info_get_tag(item, finfo, TAG_ARTIST_ID, artist_id);

					// TODO: use artist id in better string instead
					pfc::string8 formatted_release_name;
					item->format_title(nullptr, formatted_release_name, g_discogs->release_name_script, nullptr);

					if (artist_id.get_length()) {
						Artist_ptr artist = discogs_interface->get_artist(artist_id.get_ptr(), false, p_status, p_abort);
						g_discogs->save_artist_art(artist, release, item, done_artist_files, p_status, p_abort);
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


download_art_task::download_art_task(pfc::string8 release_id, metadb_handle_list items) :
		release_id(release_id), items(items) {
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

	uartwork uartconf = uartwork(CONF);
	bool bconf_album_save_or_embed = CONF.save_album_art || CONF.embed_album_art;
	bool bconf_artist_save_or_embed = CONF.save_artist_art || CONF.embed_artist_art;

	bool bcust_album_save_or_embed = CONFARTWORK.ucfg_album_save_to_dir != uartconf.ucfg_album_save_to_dir;
	bcust_album_save_or_embed |= CONFARTWORK.ucfg_album_embed != uartconf.ucfg_album_embed;

	bool bcust_artist_save_or_embed = CONFARTWORK.ucfg_art_save_to_dir != uartconf.ucfg_art_save_to_dir;
	bcust_artist_save_or_embed |= CONFARTWORK.ucfg_art_embed != uartconf.ucfg_art_embed;

	if (bconf_album_save_or_embed || bcust_album_save_or_embed) {
		pfc::array_t<pfc::string8> done_files;
		for (size_t i = 0; i < items.get_count(); i++) {
			bool bsave = false;
			if (bconf_album_save_or_embed && !bcust_album_save_or_embed) {
				art_download_attribs cfg_ada(CONF.save_album_art, CONF.embed_album_art, CONF.album_art_overwrite, CONF.album_art_fetch_all, false, {});			
				bit_array_bittable dummy_saved_mask(release->images.get_count() + release->artists[0]->full_artist->images.get_count());
				g_discogs->save_album_art(release, items[i], cfg_ada, pfc::array_t<GUID>(), dummy_saved_mask, done_files, p_status, p_abort);
			}
			else {
				bool bitem_write = CONFARTWORK.ucfg_art_save_to_dir != 0;
				bool bitem_embed = CONFARTWORK.ucfg_art_ovr != 0;
				bool bitem_overwrite = CONFARTWORK.ucfg_art_ovr != 0;
				bool bitem_fetch_all = true; //CONFARTWORK.getflag(af::art_sa, i);
				if (bitem_write || bitem_embed) {					
					art_download_attribs ada_mod(bitem_write, bitem_embed, bitem_overwrite, true /*enter image loop*/, false, {});
					bit_array_bittable dummy_saved_mask(release->images.get_count() + release->artists[0]->full_artist->images.get_count());
					g_discogs->save_album_art(release, items[i], ada_mod, pfc::array_t<GUID>(), dummy_saved_mask, done_files, p_status, p_abort);
				}
			}
		}
	}
	if (bconf_artist_save_or_embed || bcust_artist_save_or_embed) {
		pfc::array_t<pfc::string8> done_files;
		for (size_t i = 0; i < items.get_count(); i++) {
			bool bsave = false;
			if (bconf_artist_save_or_embed && !bcust_artist_save_or_embed) {
				art_download_attribs cfg_ada(CONF.save_artist_art, CONF.embed_artist_art, CONF.artist_art_overwrite, CONF.artist_art_fetch_all, false, {});
				bit_array_bittable dummy_saved_mask(release->images.get_count() + release->artists[0]->full_artist->images.get_count());
				g_discogs->save_artist_art(release, items[i], cfg_ada, pfc::array_t<GUID>(), dummy_saved_mask, done_files, p_status, p_abort);
			}
			else {
				bool bitem_write = CONFARTWORK.ucfg_art_save_to_dir != 0;
				bool bitem_embed = CONFARTWORK.ucfg_art_ovr != 0;
				bool bitem_overwrite = CONFARTWORK.ucfg_art_ovr != 0;
				bool bitem_fetch_all = true; //CONFARTWORK.getflag(af::art_sa, i);
				if (bitem_write || bitem_embed) {
					art_download_attribs ada_mod(bitem_write, bitem_embed, bitem_overwrite, true /*enter image loop*/, false, {});
					bit_array_bittable dummy_saved_mask(release->images.get_count() + release->artists[0]->full_artist->images.get_count());
					g_discogs->save_artist_art(release, items[i], ada_mod, pfc::array_t<GUID>(), dummy_saved_mask, done_files, p_status, p_abort);
				}
			}
		}
	}
}

download_art_paths_task::download_art_paths_task(CTrackMatchingDialog* m_dialog, pfc::string8 release_id, 
	metadb_handle_list items, pfc::array_t<GUID> album_art_ids, bool to_path_only) :
	m_dialog(m_dialog), m_release_id(release_id), m_items(items),
	m_album_art_ids(album_art_ids), m_to_path_only(to_path_only) {

}

void download_art_paths_task::start() {
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

void download_art_paths_task::safe_run(threaded_process_status& p_status, abort_callback& p_abort) {
	Release_ptr release = discogs_interface->get_release(m_release_id.get_ptr(), p_status, p_abort);


	bit_array_bittable saved_mask(m_album_art_ids.size());


	uartwork uartconf = uartwork(CONF);
	bool bconf_album_save_or_embed = CONF.save_album_art || CONF.embed_album_art;
	bool bconf_artist_save_or_embed = CONF.save_artist_art || CONF.embed_artist_art;

	bool bcust_album_save_or_embed = CONFARTWORK.ucfg_album_save_to_dir != uartconf.ucfg_album_save_to_dir;
	bcust_album_save_or_embed |= CONFARTWORK.ucfg_album_embed != uartconf.ucfg_album_embed;

	bool bcust_artist_save_or_embed = CONFARTWORK.ucfg_art_save_to_dir != uartconf.ucfg_art_save_to_dir;
	bcust_artist_save_or_embed |= CONFARTWORK.ucfg_art_embed != uartconf.ucfg_art_embed;

	if (bconf_album_save_or_embed || bcust_album_save_or_embed) {
		pfc::array_t<pfc::string8> done_files;
		for (size_t i = 0; i < m_items.get_count(); i++) {
			bool bsave = false;
			if (bconf_album_save_or_embed && !bcust_album_save_or_embed) {
				art_download_attribs cfg_ada(CONF.save_album_art, CONF.embed_album_art, CONF.album_art_overwrite, CONF.album_art_fetch_all, true, {});
				cfg_ada.to_path_only = m_to_path_only;
				
				g_discogs->save_album_art(release, m_items[i], cfg_ada, m_album_art_ids, saved_mask, done_files, p_status, p_abort);
				
				for (size_t walk = 0; walk < done_files.size(); walk++) {
					m_vres.emplace_back(std::pair(done_files[walk].get_ptr(), saved_mask));
				}
			}
			else {
				bool bitem_write = CONFARTWORK.ucfg_album_save_to_dir != 0;
				bool bitem_embed = CONFARTWORK.ucfg_album_embed != 0;
				bool bitem_overwrite = CONFARTWORK.ucfg_album_ovr != 0;
				bool bitem_fetch_all = true; //CONFARTWORK.getflag(af::art_sa, i);
				if (bitem_write || bitem_embed) {
					art_download_attribs ada_mod(bitem_write, bitem_embed, bitem_overwrite, true /*enter image loop*/, true, {});
				
					ada_mod.to_path_only = m_to_path_only;
					
					g_discogs->save_album_art(release, m_items[i],
						ada_mod, m_album_art_ids, saved_mask,
						done_files, p_status, p_abort);

					for (size_t walk = 0; walk < done_files.size(); walk++) {
						m_vres.emplace_back(std::pair(done_files[walk].get_ptr(), saved_mask));
					}
				}
			}
		}
	}
	if (bconf_artist_save_or_embed || bcust_artist_save_or_embed) {
		pfc::array_t<pfc::string8> done_files;
		for (size_t i = 0; i < m_items.get_count(); i++) {
			bool bsave = false;
			if (bconf_artist_save_or_embed && !bcust_artist_save_or_embed) {
				art_download_attribs cfg_ada(CONF.save_artist_art, CONF.embed_artist_art, CONF.artist_art_overwrite, CONF.artist_art_fetch_all, false, {});
				
				g_discogs->save_artist_art(release, m_items[i],
					cfg_ada, m_album_art_ids, saved_mask,
					done_files, p_status, p_abort);

				for (size_t walk = 0; walk < done_files.size(); walk++) {
					m_vres.emplace_back(std::pair(done_files[walk].get_ptr(), saved_mask));
				}
			}
			else {
				bool bitem_write = CONFARTWORK.ucfg_art_save_to_dir != 0;
				bool bitem_embed = CONFARTWORK.ucfg_art_embed != 0;
				bool bitem_overwrite = CONFARTWORK.ucfg_art_ovr != 0;
				bool bitem_fetch_all = true; //CONFARTWORK.getflag(af::art_sa, i);
				if (bitem_write || bitem_embed) {
					art_download_attribs ada_mod(bitem_write, bitem_embed, bitem_overwrite, true, false, {});

					g_discogs->save_artist_art(release, m_items[i],
						ada_mod, m_album_art_ids, saved_mask,
						done_files, p_status, p_abort);

					for (size_t walk = 0; walk < done_files.size(); walk++) {
						m_vres.emplace_back(std::pair(done_files[walk].get_ptr(), saved_mask));
					}
				}
			}
		}
	}
}

void download_art_paths_task::on_success(HWND p_wnd) {

	std::shared_ptr<std::vector<std::pair<pfc::string8, bit_array_bittable>>> vpaths = std::make_shared<std::vector<std::pair<pfc::string8, bit_array_bittable>>>(m_vres);

	if (IsWindow(m_dialog->m_hWnd))
		m_dialog->process_download_art_paths_done(m_release_id, vpaths/*srv*/, m_album_art_ids);
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

void find_deleted_releases_task::on_error(HWND p_wnd) {
	finish();
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

void find_releases_not_in_collection_task::on_error(HWND p_wnd) {
	finish();
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

		try {
			TagWriter_ptr tag_writer = std::make_shared<TagWriter>(sub_manager, discogs_interface->get_release(release_id, p_status, p_abort, false, true));
			tag_writer->match_tracks();
			tag_writers.append_single(tag_writer);
		}
		catch (http_404_exception) {
			pfc::string8 error;
			error << "Skipping 404 release: " << release_id;
			TagWriter_ptr tag_writer = std::make_shared<TagWriter>(sub_manager, error);
			tag_writers.append_single(tag_writer);
		}
	}
}

void update_tags_task::on_success(HWND p_wnd) {
	fb2k::newDialog<CTrackMatchingDialog>(core_api::get_main_window(), tag_writers, use_update_tags);
	finish();
}

void update_tags_task::on_abort(HWND p_wnd) {
}

void update_tags_task::on_error(HWND p_wnd) {
	finish();
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
		CFindReleaseDialog* find_dlg = reinterpret_cast<CFindReleaseDialog*>(g_discogs->find_release_dialog);
		find_dlg->on_get_artist_done(m_updsrc, m_artist);
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
		CFindReleaseDialog* find_dlg = reinterpret_cast<CFindReleaseDialog*>(g_discogs->find_release_dialog);
		find_dlg->on_search_artist_done(m_artist_exact_matches, m_artist_other_matches);
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
		CFindReleaseDialog* find_dlg = reinterpret_cast<CFindReleaseDialog*>(g_discogs->find_release_dialog);
		find_dlg->on_expand_master_release_done(m_master_release, m_pos, p_status, p_abort);
	}
}

void expand_master_release_process_callback::on_success(HWND p_wnd) {
	if (g_discogs->find_release_dialog) {
		CFindReleaseDialog* find_dlg = reinterpret_cast<CFindReleaseDialog*>(g_discogs->find_release_dialog);
		find_dlg->on_expand_master_release_complete();
	}
}

void expand_master_release_process_callback::on_abort(HWND p_wnd) {
	if (g_discogs->find_release_dialog) {
		CFindReleaseDialog* find_dlg = reinterpret_cast<CFindReleaseDialog*>(g_discogs->find_release_dialog);
		find_dlg->on_expand_master_release_complete();
	}
}

void expand_master_release_process_callback::on_error(HWND p_wnd) {
	if (g_discogs->find_release_dialog) {
		CFindReleaseDialog* find_dlg = reinterpret_cast<CFindReleaseDialog*>(g_discogs->find_release_dialog);
		find_dlg->on_expand_master_release_complete();
	}
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
		if (release->images.get_size() && (CONF.save_album_art || CONF.embed_album_art)) {
			if (!release->images[0]->url150.get_length()) {
				foo_discogs_exception ex;
				ex << "Image URLs unavailable - Is OAuth working?";
				throw ex;
			}

			bool has_thumb;
			try {
				has_thumb = discogs_interface->get_thumbnail_from_cache(release, false, 0, release->small_art,
					p_status, p_abort);
			}
			catch (foo_discogs_exception& e) {
				has_thumb = false;
				add_error("Reading artwork preview small album art cache", e, false);
			}

			if (!has_thumb) {
				p_status.set_item("Fetching small album art...");
				try {
					discogs_interface->fetcher->fetch_url(release->images[0]->url150, "", release->small_art, p_abort, false);
				}
				catch (foo_discogs_exception& e) {
					add_error("Fetching small album art", e, false);
				}
			}
			
		}
	}

	m_dialog->enable(true);
	m_dialog->hide();

	tag_writer = std::make_shared<TagWriter>(finfo_manager, release);
	tag_writer->match_tracks();
}

void process_release_callback::on_success(HWND p_wnd) {
	fb2k::newDialog<CTrackMatchingDialog>(core_api::get_main_window(), tag_writer, false);
}

void process_release_callback::on_abort(HWND p_wnd) {
	m_dialog->enable(true);
}

void process_release_callback::on_error(HWND p_wnd) {
	m_dialog->enable(true);
}

process_artwork_preview_callback::process_artwork_preview_callback(CTrackMatchingDialog* dialog, const Release_ptr& release, const size_t img_ndx, const bool bartist, bool onlycache) :
	m_dialog(dialog), m_release(release), m_img_ndx(img_ndx), m_bartist(bartist), m_onlycache(onlycache) {
}

void process_artwork_preview_callback::start(HWND parent) {
	threaded_process::g_run_modeless(this,
		threaded_process::flag_show_item |
		threaded_process::flag_show_abort,
		parent,
		"Processing track matching artwork preview..."
	);
}

void process_artwork_preview_callback::safe_run(threaded_process_status& p_status, abort_callback& p_abort) {
	p_status.set_item("Fetching artwork preview information...");
	
	if (m_release) {
		pfc::string8 id;
		pfc::string8 url;

		bool has_thumb;
		try {
			has_thumb = discogs_interface->get_thumbnail_from_cache(m_release, m_bartist, m_img_ndx, m_small_art,
				p_status, p_abort);
		}
		catch (foo_discogs_exception& e) {
			has_thumb = false;
			add_error("Reading artwork preview small album art cache", e, false);
		}

		if (!has_thumb) {

			if (!m_onlycache) {
				p_status.set_item("Fetching artwork preview small album art...");
				try {
					pfc::array_t<Image_ptr> images;
					size_t ndx;
					if (m_bartist) {
						images = m_release->artists[0]->full_artist->images;

					}
					else {
						images = m_release->images;
					}
					discogs_interface->fetcher->fetch_url(images[m_img_ndx]->url150, "", m_small_art, p_abort, false);
				}
				catch (foo_discogs_exception& e) {
					add_error("Fetching artwork preview small album art", e, false);
				}			
			}
		}
	}
}

void process_artwork_preview_callback::on_success(HWND p_wnd) {

	m_dialog->process_artwork_preview_done(m_img_ndx, m_bartist, m_small_art);

}

void process_artwork_preview_callback::on_abort(HWND p_wnd) {
	//m_dialog->enable(true);
}

void process_artwork_preview_callback::on_error(HWND p_wnd) {
	//m_dialog->enable(true);
}


process_file_artwork_preview_callback::process_file_artwork_preview_callback(CTrackMatchingDialog* dialog, const Release_ptr& release, metadb_handle_list items, const size_t img_ndx, const bool bartist) :
	m_dialog(dialog), m_release(release), m_img_ndx(img_ndx), m_items(items), m_bartist(bartist) {
	//m_dialog->enable(false);
}

void process_file_artwork_preview_callback::start(HWND parent) {
	threaded_process::g_run_modeless(this,
		threaded_process::flag_show_item |
		threaded_process::flag_show_abort,
		parent,
		"Processing local files track matching artwork preview..."
	);
}

void process_file_artwork_preview_callback::safe_run(threaded_process_status& p_status, abort_callback& p_abort) {
	p_status.set_item("Fetching local files artwork preview information...");

	if (m_release) {

		p_status.set_item("Fetching local files artwork preview small album art...");
		try {

	    	bool bexists = true;
			pfc::string8 directory;
			file_info_impl info;
			titleformat_hook_impl_multiformat hook(p_status, &m_release);
			
			CONF.album_art_directory_string->run_hook(m_items[0]->get_location(), &info, &hook, directory, nullptr);
			// hardcode "nullptr" as don't write anything.  ???
			if (STR_EQUAL(directory, "nullptr")) {
				bexists = false;
			}
			
			pfc::string8 path(directory);

			pfc::string8 file_name = m_img_ndx < album_art_ids::num_types() ? album_art_ids::query_name(m_img_ndx) : "";
			pfc::chain_list_v2_t<pfc::string8> splitfile;
			pfc::splitStringByChar(splitfile, file_name.get_ptr(), ' ');
			if (splitfile.get_count()) file_name = *splitfile.first();
			file_name << ".jpg";

			//pfc::string8 file_name("front.jpg");

			pfc::string8 full_path(directory);
			full_path << "\\" << file_name;

			char converted[MAX_PATH - 1];	
			pfc::stringcvt::string_os_from_utf8 cvt(full_path);
			wcstombs(converted, cvt.get_ptr(), MAX_PATH - 1);

			FILE* fd = nullptr;
			if (fopen_s(&fd, converted, "rb") != 0) {
				pfc::string8 msg("can't open ");
				msg << full_path;
				log_msg(msg);
				return;
			}
			fclose(fd);

			//(checksize)

			struct stat stat_buf;
			int stat_result = stat(converted, &stat_buf);
			//return -1 or size
			int src_length = stat_buf.st_size;

			int filesize = (stat_result == -1 ? -1 : src_length);

			if (filesize == -1)
				return;

			pfc::stringcvt::string_os_from_utf8 cvt_bitmap(full_path);

			Gdiplus::Bitmap local_bitmap(cvt_bitmap.get_ptr(), false);

			UINT w = local_bitmap.GetWidth();
			UINT h = local_bitmap.GetHeight();

			m_small_art = GenerateTmpBitmapsFromRealSize(m_release->id, m_img_ndx, full_path, m_temp_file_names);

			return;

			}
			catch (foo_discogs_exception& e) {
				add_error("Fetching local files artwork preview small album art", e, false);
			}
		}

		//m_dialog->enable(true);
		//m_dialog->hide();

	
}

void process_file_artwork_preview_callback::on_success(HWND p_wnd) {

	m_dialog->process_file_artwork_preview_done(m_img_ndx, m_bartist, m_small_art, m_temp_file_names);
	BOOL bres = DeleteObject(m_small_art.first);
	bres = DeleteObject(m_small_art.second);
}

void process_file_artwork_preview_callback::on_abort(HWND p_wnd) {
	//m_dialog->enable(true);
}

void process_file_artwork_preview_callback::on_error(HWND p_wnd) {
	//m_dialog->enable(true);
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
	if (!g_discogs->configuration_dialog) {
		static_api_ptr_t<ui_control>()->show_preferences(guid_pref_page);
	}
	CConfigurationDialog* dlg = reinterpret_cast<CConfigurationDialog*>(g_discogs->configuration_dialog);
	if (dlg) {
		dlg->show_oauth_msg(
			"OAuth is working!\nSuccess!", false);
		::SetFocus(g_discogs->configuration_dialog->m_hWnd);
	}
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
	CConfigurationDialog* dlg = reinterpret_cast<CConfigurationDialog*>(g_discogs->configuration_dialog);
	
	uSetWindowText(dlg->token_edit, token->key().c_str());
	uSetWindowText(dlg->secret_edit, token->secret().c_str());
	delete token;
	token = nullptr;
}
