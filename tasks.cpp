#include "stdafx.h"

#include "foo_discogs.h"
#include "tags.h"
#include "art_download_attribs.h"
#include "ol_cache.h"
#include "db_utils.h"
#include "db_fetcher.h"
#include "configuration_dialog.h"

#ifdef CAT_UI
#include "uui_element_dialog.h"
#endif // CAT_UI

#include "tasks.h"

void foo_discogs_threaded_process_callback::run(threaded_process_status &p_status, abort_callback &p_abort) {
	try {
		safe_run(p_status, p_abort);
	}
	catch (foo_discogs_exception &e) {
		add_error(e, true);
	}
	catch (...) {		
		foo_discogs_exception ex;
		ex << "Unknown error running task.";
		add_error(ex, true);
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
		m_tag_writer(tag_writer), m_preview_dialog(preview_dialog), m_show_preview_dialog(false), m_alt_mappings(nullptr), m_use_update_tags(use_update_tags) {
	preview_dialog->enable(false, true);
}

generate_tags_task::generate_tags_task(CTrackMatchingDialog *track_matching_dialog, TagWriter_ptr tag_writer, bool show_preview_dialog, bool use_update_tags) :
		m_tag_writer(tag_writer), m_track_matching_dialog(track_matching_dialog), m_show_preview_dialog(show_preview_dialog), m_alt_mappings(nullptr), m_use_update_tags(use_update_tags) {
	track_matching_dialog->enable(false);
}
generate_tags_task::generate_tags_task(CTagCreditDialog* credits_dialog, TagWriter_ptr tag_writer, tag_mapping_list_type* alt_mappings) :
		m_tag_writer(tag_writer), m_alt_mappings(alt_mappings), m_credits_dialog(credits_dialog), m_show_preview_dialog(false), m_use_update_tags(false) {
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
	m_tag_writer->generate_tags(m_use_update_tags, m_alt_mappings, p_status, p_abort);
}

void generate_tags_task::on_success(HWND p_wnd) {
	if (m_preview_dialog) {
		if (IsWindow(m_preview_dialog->m_hWnd)) {
			m_preview_dialog->cb_generate_tags();
		}
	}
#ifdef CAT_CRED
	else if (m_credits_dialog) {
		if (IsWindow(m_credits_dialog->m_hWnd)) {
			m_credits_dialog->cb_generate_tags();
		}
	}
#endif // CAT_CRED

	else if (m_show_preview_dialog) {
		m_track_matching_dialog->enable(true);
		fb2k::newDialog <CPreviewTagsDialog>(core_api::get_main_window(), m_tag_writer, m_use_update_tags);
		m_track_matching_dialog->hide();
	}
	else {
		CTrackMatchingDialog* dlg = reinterpret_cast<CTrackMatchingDialog*>(g_discogs->track_matching_dialog);
		dlg->destroy_all();
		service_ptr_t<write_tags_task> task = new service_impl_t<write_tags_task>(m_tag_writer);
		task->start();
	}
}

void generate_tags_task::on_abort(HWND p_wnd) {
	on_error(p_wnd);
}

void generate_tags_task::on_error(HWND p_wnd) {
	if (m_preview_dialog) {
		m_preview_dialog->enable(true, true);
	}
	else {
		m_track_matching_dialog->enable(true);
		m_track_matching_dialog->show();
	}
}

void generate_tags_task_multi::on_success(HWND p_wnd) {
	if (show_preview_dialog) {
		fb2k::newDialog <CPreviewTagsDialog>(core_api::get_main_window(), tag_writers, use_update_tags);
	}
	else {
		service_ptr_t<write_tags_task_multi> task = new service_impl_t<write_tags_task_multi>(tag_writers);
		task->start();
	}
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
	m_tag_writer->write_tags();
}

void write_tags_task::on_success(HWND p_wnd) {

	m_tag_writer->finfo_manager->write_infos();

	bool bskip_art = ((LOWORD(CONF.album_art_skip_default_cust) & ART_CUST_SKIP_DEF_FLAG) == ART_CUST_SKIP_DEF_FLAG)
		|| ((HIWORD(CONF.album_art_skip_default_cust) & ART_CUST_SKIP_DEF_FLAG) == ART_CUST_SKIP_DEF_FLAG);
	bool bconf_art_save = (CONF.save_album_art || CONF.save_artist_art || CONF.embed_album_art || CONF.embed_artist_art);
	bool bcustom_art_save = !(CONFARTWORK == uartwork(CONF));

	if (!bskip_art && (bconf_art_save || bcustom_art_save)) {
		service_ptr_t<download_art_task> task =
			new service_impl_t<download_art_task>(m_tag_writer->release->id, m_tag_writer->finfo_manager->items);
		task->start();
	}
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
					
					art_download_attribs ada;
					ada.write_it = save_artist_art;
					ada.embed_it = CONF.embed_artist_art;
					ada.overwrite_it = CONF.artist_art_overwrite;
					ada.fetch_all_it = CONF.artist_art_fetch_all;
					ada.to_path_only = false;

					if (save_album_art) {
						pfc::array_t<GUID> this_item_guid; this_item_guid.add_item(CONFARTGUIDS[i]);
						g_discogs->save_album_art(release, item,
							ada, pfc::array_t<GUID>(), bit_array_bittable(album_art_ids::num_types()),
							done_release_files, p_status, p_abort);
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

						art_download_attribs ada;
						ada.write_it = save_artist_art;
						ada.embed_it = CONF.embed_artist_art;
						ada.overwrite_it = CONF.artist_art_overwrite;
						ada.fetch_all_it = CONF.artist_art_fetch_all;
						ada.to_path_only = false;

						bit_array_bittable dummy_saved_mask(release->images.get_count() + release->artists[0]->full_artist->images.get_count());
						g_discogs->save_artist_art(artist, release, item, ada, pfc::array_t<GUID>(), dummy_saved_mask, done_artist_files, p_status, p_abort);
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
	bcust_album_save_or_embed |= CONFARTWORK.ucfg_album_ovr != uartconf.ucfg_album_ovr;
	bcust_album_save_or_embed |= CONFARTWORK.ucfg_album_embed != uartconf.ucfg_album_embed;

	bool bcust_artist_save_or_embed = CONFARTWORK.ucfg_art_save_to_dir != uartconf.ucfg_art_save_to_dir;
	bcust_artist_save_or_embed |= CONFARTWORK.ucfg_art_ovr != uartconf.ucfg_art_ovr;
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
		size_t playlist_id = pm->create_playlist("foo_discogger: invalid release ids", ~0, ~0);

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
		size_t playlist_id = pm->create_playlist("foo_discogger: not in collection", ~0, ~0);

		bit_array_true dummy;
		pm->playlist_insert_items(playlist_id, 0, missing_items, dummy);
		pm->set_active_playlist(playlist_id);
	}
}


void get_artist_process_callback::start(HWND parent) {
	pfc::string8 msg;
	msg << "Loading artist id " << m_artist_id;

	if ((CONF.db_dc_flag & (DBFlags::DB_SEARCH)) && get_dbfetcher())
		msg << " releases from local database...";
	else
		msg << " online releases...";

	threaded_process::g_run_modeless(this,
		threaded_process::flag_show_item |
		threaded_process::flag_show_abort |
		threaded_process::flag_show_delayed,
		parent,
		msg
	);
}

void get_artist_process_callback::safe_run(threaded_process_status &p_status, abort_callback &p_abort) {

	bool bload_releases = m_updsrc != updRelSrc::ArtistProfile;

#ifdef DB_DC
	//note: DBFlags::DB_DWN_ARTWORK checked inside artist.load()

	if (DBFlags(CONF.db_dc_flag).IsReady() && get_dbfetcher()) {

		start_cancel_listener_thread(&p_abort);

		m_artist = discogs_db_interface->get_artist(m_artist_id, bload_releases, p_status, p_abort, false, true, get_dbfetcher() /*get_dbfetcher().get()*/);

		set_db_finished();

	}
	else {
		m_artist = discogs_interface->get_artist(m_artist_id, bload_releases, p_status, p_abort);
	}
#else
	m_artist = discogs_interface->get_artist(m_artist_id, bload_releases, p_status, p_abort);
#endif // DB_DC

	p_status.set_item("Formatting artist releases...");
}

void get_artist_process_callback::on_success(HWND p_wnd) {
	if (m_artist) {
		CFindReleaseDialog* find_dlg = reinterpret_cast<CFindReleaseDialog*>(g_discogs->find_release_dialog);
		//artist history
		rppair row = std::pair(std::pair("", ""), std::pair(m_artist->id, m_artist->name));
		if (bool added = find_dlg->add_history_row(1, std::move(row)); added) {
			sqldb db;
			size_t db_parsed_credit_id = db.add_artist_history(m_artist, "get_artist_process_callback", row);
		}
		
		find_dlg->on_get_artist_done(m_updsrc, m_artist);
	}
}


search_artist_process_callback::search_artist_process_callback(const char* search, const int db_dc_flags) : m_search(search), m_db_dc_flags(db_dc_flags) {
	//
}

void search_artist_process_callback::start(HWND parent) {
	threaded_process::g_run_modeless(this,
		threaded_process::flag_show_item |
		threaded_process::flag_show_abort /*|
		threaded_process::flag_show_delayed*/,
		parent,
		"Searching artist..."
	);
}

void search_artist_process_callback::safe_run(threaded_process_status &p_status, abort_callback &p_abort) {
	
	pfc::string8 msg;

#ifdef DB_DC

	if (DBFlags(m_db_dc_flags).IsReady() && get_dbfetcher()) {
		vppair results;
		start_cancel_listener_thread(&p_abort);
		msg << "Fetching artist list from local database...";
		discogs_db_interface->search_artist(m_search, m_db_dc_flags, m_artist_exact_matches, m_artist_other_matches, p_status, p_abort, get_dbfetcher());
		set_db_finished();
	}
	else
	{
		msg << "Fetching online artist list...";
		discogs_interface->search_artist(m_search, m_artist_exact_matches, m_artist_other_matches, p_status, p_abort);
	}
#else
	msg << "Fetching online artist list...";
	discogs_interface->search_artist(m_search, m_artist_exact_matches, m_artist_other_matches, p_status, p_abort);
#endif // DB_DC

	p_status.set_item(msg);	
}

void search_artist_process_callback::on_success(HWND p_wnd) {

	CFindReleaseDialog* find_dlg = g_discogs->find_release_dialog;
	if (m_artist_exact_matches.size()) {
		//artist history
		rppair row = std::pair(std::pair("", ""), std::pair(m_artist_exact_matches[0]->id, m_artist_exact_matches[0]->name));
		if (bool added = find_dlg->add_history_row(1, std::move(row)); added) {
			sqldb db;
			size_t db_parsed_credit_id = db.add_artist_history(m_artist_exact_matches[0], "search_artist_process_callback", row);
		}
	}

	find_dlg->on_search_artist_done(m_artist_exact_matches, m_artist_other_matches);
}

void search_artist_process_callback::on_abort(HWND p_wnd) {
    //
}

void search_artist_process_callback::on_error(HWND p_wnd) {
	//
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

#ifdef DB_DC
	if (DBFlags(CONF.db_dc_flag).IsReady() && get_dbfetcher())
		start_cancel_listener_thread(&p_abort);

	m_master_release->load_releases(p_status, p_abort, false, m_offlineArtist_id, get_dbfetcher());

	if (DBFlags(CONF.db_dc_flag).IsReady() && get_dbfetcher())
		set_db_finished();
#else
	m_master_release->load_releases(p_status, p_abort, false, m_offlineArtist_id, get_dbfetcher());
#endif // DB_DC


	CFindReleaseDialog* find_dlg = g_discogs->find_release_dialog;
	find_dlg->on_expand_master_release_done(m_master_release, m_pos, p_status, p_abort);
}

void expand_master_release_process_callback::on_success(HWND p_wnd) {

#ifdef DEBUG_TREE
	log_msg(PFC_string_formatter() << " expand_master_release_process_callback::on_success");
#endif

	CFindReleaseDialog* find_dlg = g_discogs->find_release_dialog;
	find_dlg->on_expand_master_release_complete();
}

void expand_master_release_process_callback::on_abort(HWND p_wnd) {
	CFindReleaseDialog* find_dlg = reinterpret_cast<CFindReleaseDialog*>(g_discogs->find_release_dialog);
	find_dlg->on_expand_master_release_complete();
}

void expand_master_release_process_callback::on_error(HWND p_wnd) {
	CFindReleaseDialog* find_dlg = reinterpret_cast<CFindReleaseDialog*>(g_discogs->find_release_dialog);
	find_dlg->on_expand_master_release_complete();
}


process_release_callback::process_release_callback(CFindReleaseDialog *dialog, const pfc::string8 &release_id,
	const pfc::string8& offline_artist_id, pfc::string8 inno, const metadb_handle_list &items) :
		m_dialog(dialog), m_release_id(release_id),
		m_offline_artist_id(offline_artist_id), m_inno(inno), m_items(items)
{
	m_dialog->enable(false);
	m_finfo_manager = std::make_shared<file_info_manager>(items);
	m_finfo_manager->read_infos();
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

	m_tag_writer = std::make_shared<TagWriter>(m_finfo_manager, p_release);
	m_tag_writer->match_tracks();
}

void process_release_callback::on_success(HWND p_wnd) {

	//release history
	rppair row = std::pair(std::pair(tag_writer->release->id, tag_writer->release->title),
		std::pair(tag_writer->release->artists[0]->full_artist->id, tag_writer->release->artists[0]->full_artist->name));
	if (bool added = m_dialog->add_history_row(0, std::move(row)); added) {
		sqldb db;
		size_t db_parsed_credit_id = db.add_release_history(tag_writer->release, "proccess_release_callback", row);
	}

	fb2k::newDialog<CTrackMatchingDialog>(core_api::get_main_window(), tag_writer, false);
}

void process_release_callback::on_abort(HWND p_wnd) {
	m_dialog->enable(true);
	m_dialog->show();
}

void process_release_callback::on_error(HWND p_wnd) {
	m_dialog->enable(true);
	m_dialog->show();
}

process_artwork_preview_callback::process_artwork_preview_callback(CTrackMatchingDialog* dialog, const Release_ptr& release, const size_t img_ndx, const bool bartist, bool onlycache) :
	m_dialog(dialog), m_release(release), m_img_ndx(img_ndx), m_bartist(bartist), m_onlycache(onlycache) {
	m_musicbrainz_mibs = {};
}

void process_artwork_preview_callback::start(HWND parent) {
	threaded_process::g_run_modeless(this,
		threaded_process::flag_show_item |
		threaded_process::flag_show_delayed |
		threaded_process::flag_show_abort,
		parent,
		"Processing track matching artwork preview..."
	);
}

void process_artwork_preview_callback::safe_run(threaded_process_status& p_status, abort_callback& p_abort) {
	p_status.set_item("Fetching artwork preview information...");
	
	if (m_release) {
		if (!(CONF.skip_mng_flag & SkipMng::SKIP_BRAINZ_ID_FETCH) && (m_img_ndx == 0 && !m_bartist)) {
			do {
				try {
					p_status.set_item("Fetching MusicBrainz ids...");
					pfc::string8 html;
					pfc::string8 mib_request_url;
					//RELEASE MIB
					mib_request_url = "https://www.musicbrainz.org/ws/2/url?inc=release-rels&fmt=json&resource=https://www.discogs.com/release/";
					mib_request_url << m_release->id;
					discogs_interface->fetcher->fetch_html_simple_log(mib_request_url, "", html, p_abort);
					
					try {
						JSONParser jp(html);
						if (json_is_object(jp.root)) {
							json_t* s = json_object_get(jp.root, "relations");
							json_t* ar_s = json_array_get(s, 0);
							json_t* t = json_object_get(ar_s, "release");
							m_musicbrainz_mibs.release = JSONAttributeString(t, "id");
						}
					}
					catch (...) {
						log_msg("Error parsing release MBid");
					}

					if (m_release->master_id.get_length()) {
						//RELEASE-GROUP MIB
						mib_request_url = "https://www.musicbrainz.org/ws/2/url?inc=release-group-rels&fmt=json&resource=https://www.discogs.com/master/";
						mib_request_url << m_release->master_id;
						discogs_interface->fetcher->fetch_html_simple_log(mib_request_url, "", html, p_abort);

						try {
							JSONParser jp(html);
							if (json_is_object(jp.root)) {
								json_t* s = json_object_get(jp.root, "relations");
								json_t* ar_s = json_array_get(s, 0);
								json_t* t = json_object_get(ar_s, "release_group");
								m_musicbrainz_mibs.release_group = JSONAttributeString(t, "id");
							}
						}
						catch (...) {
							log_msg("Error parsing release_group MBid");
						}
					}
					if (m_release->artists[0]->full_artist->id.get_length()) {
						//ARTIST MIB
						mib_request_url = "https://www.musicbrainz.org/ws/2/url?inc=artist-rels&fmt=json&resource=https://www.discogs.com/artist/";
						mib_request_url << m_release->artists[0]->full_artist->id;
						discogs_interface->fetcher->fetch_html_simple_log(mib_request_url, "", html, p_abort);
						
						try {
							JSONParser jp(html);
							if (json_is_object(jp.root)) {
								json_t* s = json_object_get(jp.root, "relations");
								json_t* ar_s = json_array_get(s, 0);
								json_t* t = json_object_get(ar_s, "artist");
								m_musicbrainz_mibs.artist = JSONAttributeString(t, "id");
							}
						}
						catch (...) {
							log_msg("Error parsing artist MBid");
						}
					}

					if (m_musicbrainz_mibs.release.get_length()) {
						//CHECK COVERART
						mib_request_url = "https://musicbrainz.org/release/";
						mib_request_url << m_musicbrainz_mibs.release << "/cover-art";
						discogs_interface->fetcher->fetch_html_simple_log(mib_request_url, "", html, p_abort);
						m_musicbrainz_mibs.hascoverart = html.get_length() && html.find_first("Cover Art (0)", 0) == pfc_infinite;
					}
				}
				catch (network_exception) {					
					break;
				}
				catch (exception_aborted) {
					break;
				}
				catch (...) {
					log_msg("Unhandled error parsing MBid");
					throw;
				}

			} while (false);
		}

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
	m_dialog->process_artwork_preview_done(m_img_ndx, m_bartist, m_small_art, m_musicbrainz_mibs);
}

void process_artwork_preview_callback::on_abort(HWND p_wnd) {
	//
}

void process_artwork_preview_callback::on_error(HWND p_wnd) {
	//
}


process_file_artwork_preview_callback::process_file_artwork_preview_callback(CTrackMatchingDialog* dialog, const Release_ptr& release, metadb_handle_list items, const size_t img_ndx, const bool bartist) :
	m_dialog(dialog), m_release(release), m_img_ndx(img_ndx), m_items(items), m_bartist(bartist) {
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

			pfc::string8 full_path(directory);
			full_path << "\\" << file_name;

			char converted[MAX_PATH - 1];	
			pfc::stringcvt::string_os_from_utf8 cvt(full_path);
#pragma warning(suppress:4996)
			wcstombs(converted, cvt.get_ptr(), MAX_PATH - 1);

			FILE* fd = nullptr;
			if (fopen_s(&fd, converted, "rb") != 0) {
				pfc::string8 msg("can't open ");
				msg << full_path;
				log_msg(msg);
				return;
			}
			fclose(fd);

			struct stat stat_buf;
			int stat_result = stat(converted, &stat_buf);
			//returns -1 or size
			int filesize = (stat_result == -1 ? -1 : stat_buf.st_size);
			if (filesize == -1)
				return;

			pfc::stringcvt::string_os_from_utf8 cvt_bitmap(full_path);
			Gdiplus::Bitmap local_bitmap(cvt_bitmap.get_ptr(), false);
			m_small_art = GenerateTmpBitmapsFromRealSize(m_release->id, m_img_ndx, full_path, m_temp_file_names);
			return;
		}
		catch (foo_discogs_exception& e) {
			add_error("Fetching local files artwork preview small album art", e, false);
		}
	}
}

void process_file_artwork_preview_callback::on_success(HWND p_wnd) {
	//not checking dlg (m_hWnd is the process HWND parent)
	m_dialog->process_file_artwork_preview_done(m_img_ndx, m_bartist, m_small_art, m_temp_file_names);
	BOOL bres = DeleteObject(m_small_art.first);
	bres = DeleteObject(m_small_art.second);
}

void process_file_artwork_preview_callback::on_abort(HWND p_wnd) {
	//
}

void process_file_artwork_preview_callback::on_error(HWND p_wnd) {
	//
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
	//not checking dlg (m_hWnd is the process HWND parent)
	CConfigurationDialog* dlg = reinterpret_cast<CConfigurationDialog*>(g_discogs->configuration_dialog);
	dlg->show_oauth_msg("OAuth is working!", false);
	::SetFocus(g_discogs->configuration_dialog->m_hWnd);
}

void test_oauth_process_callback::on_error(HWND p_wnd) {
	CConfigurationDialog* dlg = reinterpret_cast<CConfigurationDialog*>(g_discogs->configuration_dialog);
	dlg->show_oauth_msg("OAuth failed.", true);
	::SetFocus(g_discogs->configuration_dialog->m_hWnd);
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
