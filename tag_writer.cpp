#include "stdafx.h"
#include "tag_writer.h"
#include "multiformat.h"

TagWriter::TagWriter(file_info_manager_ptr finfo_manager, Release_ptr release) : finfo_manager(finfo_manager), release(release) {
}

const std::vector<int(TagWriter::*)(track_mappings_list_type &)> TagWriter::matching_functions = {
	&TagWriter::order_tracks_by_duration,
	&TagWriter::order_tracks_by_number,
	&TagWriter::order_tracks_by_assumption
};

void TagWriter::match_tracks() {
	match_status = compute_discogs_track_order(track_mappings);
	if (match_status != MATCH_SUCCESS) {
		const size_t citems = finfo_manager->items.get_count();
		size_t ctracks = 0;
		for (size_t i = 0; i < release->discs.get_count(); i++) {
			for (size_t j = 0; j < release->discs[i]->tracks.get_count(); j++) {
				if (ctracks < citems) {					
					track_mapping& tm = track_mappings[ctracks];
					tm.enabled = true;
					tm.discogs_disc = i;
					tm.discogs_track = j;
				    ctracks++;
				}
				else {
					//can not mach more files
					return;
				}
				
			}
		}
		//todo: unknown scenario?
		//match_status = compute_discogs_track_order(track_mappings);
	}
}

int TagWriter::compute_discogs_track_order(track_mappings_list_type &mappings) {
	std::map<int, bool> track_number_exist;

	// TODO: remove?
	// don't try to match tracks if not the same number of tracks
	const size_t count = release->get_total_track_count();
	if (count != finfo_manager->items.get_count()) {
		return MATCH_FAIL;
	}

	mappings.set_size(count);

	track_mappings_list_type temp;
	temp.set_size(count);

	int status;
	bool initialized = false;
	bool assumed = false;

	for (size_t i = 0; i < matching_functions.size(); i++) {
		status = (this->*matching_functions[i])(temp);
		if (status == MATCH_FAIL) {
			return MATCH_FAIL;
		}
		else if (status == MATCH_SUCCESS || status == MATCH_ASSUME) {
			if (!initialized) {
				for (size_t j = 0; j < count; j++) {
					mappings[j] = temp[j];
				}
				initialized = true;
				assumed = status == MATCH_ASSUME;
			}
			else {
				for (size_t j = 0; j < count; j++) {
					if (mappings[j] != temp[j]) {
						return MATCH_FAIL;
					}
				}
				assumed = false;
			}
		}
	}
	if (assumed) {
		return MATCH_ASSUME;
	}
	else if (initialized) {
		return MATCH_SUCCESS;
	}
	return MATCH_FAIL;
}

size_t cost(size_t **m, size_t n) {
	size_t r = 0;
	for (size_t i = 0; i < n; i++) {
		r += m[i][i];
	}
	return r;
}

void copy(size_t **s, size_t **d, size_t n) {
	for (size_t i = 0; i < n; i++) {
		for (size_t j = 0; j < n; j++) {
			d[i][j] = s[i][j];
		}
	}
}

int TagWriter::order_tracks_by_duration(track_mappings_list_type &mappings) {
	if (!CONF.match_tracks_using_duration) {
		return MATCH_NA;
	}
	
	const size_t citems = finfo_manager->items.get_count();

	// first try to match tracks on nearest track length if the release has lengths
	metadb_handle_ptr item;
	
	if (release->has_tracklengths()) {
		// assuming (previously) the same # of files and tracks!
		
		/*
		size_t **distances = new size_t*[count];
		size_t **min_distances = new size_t*[count];
		size_t *index = new size_t[count];
		for (size_t i = 0; i < count; i++) {
			distances[i] = new size_t[count];
			min_distances[i] = new size_t[count];
			index[i] = i;
		}
		
		size_t d = 0;
		size_t t = 0;
		for (size_t i = 0; i < count; i++) {
			while (t >= release->discs[d]->tracks.get_size()) {
				d++;
				t = 0;
			}
			ReleaseDisc_ptr &disc = release->discs[d];
			ReleaseTrack_ptr &track = disc->tracks[t];

			for (size_t j = 0; j < count; j++) {
				item = finfo_manager->items.get_item(j);

				int file_length = (int)round(item->get_length());
				int discogs_length = track->discogs_duration_seconds;
				size_t delta = abs(file_length - discogs_length);
				distances[j][i] = delta;
			}
			t++;
		}
		
		size_t min_cost = cost(distances, count);
		copy(distances, min_distances, count);

		for (size_t i = 0; i < count; i++) {
			pfc::string8 m;
			for (size_t j = 0; j < count; j++) {
				m << distances[i][j] << "\t\t";
			}
			log_msg(m);
		}

		size_t *temp;
		bool found;
		do {
			found = false;
			for (size_t i = 0; i < count; i++) {
				for (size_t j = 0; j < count; j++) {
					if (i == j) {
						continue;
					}
					temp = distances[i];
					distances[i] = distances[j];
					distances[j] = temp;
					if (cost(distances, count) < min_cost) {
						pfc::string8 m;
						m << "swapping " << index[i] << " with " << index[j] << "   " << cost(distances, count) << "<" << min_cost;
						log_msg(m);
						size_t temp2 = index[i];
						index[i] = index[j];
						index[j] = temp2;
						m = "";
						for (size_t k = 0; k < count; k++) {
							m << index[k] << "\t\t";
						}
						log_msg(m);
						min_cost = cost(distances, count);
						copy(distances, min_distances, count);
						log_msg("New min distances:");
						for (size_t k = 0; k < count; k++) {
							pfc::string8 m;
							for (size_t l = 0; l < count; l++) {
								m << distances[k][l] << "\t\t";
							}
							log_msg(m);
						}
						found = true;
						break;
					}
					else {
						temp = distances[i];
						distances[i] = distances[j];
						distances[j] = temp;
					}
				}
				if (found) {
					break;
				}
			}
		} while (found);
		

		for (size_t i = 0; i < count; i++) {
			// TODO: this is broken
			mappings[i].enabled = true;
			PFC_ASSERT(release->find_disc_track(index[i], &mappings[i].discogs_disc, &mappings[i].discogs_track));
			mappings[i].file_index = i;
		}

		for (size_t i = 0; i < count; i++) {
			delete distances[i];
			delete min_distances[i];
		}
		delete distances;
		delete min_distances;
		delete index;
		
		const unsigned MAX_DELTA = count * (count <= 5 ? 120 / (count - 1) : 15);
		if (min_cost < MAX_DELTA) {
			return MATCH_SUCCESS;
		}
		else {
			return MATCH_FAIL;
		}
		*/

		// max delta of 5 seconds per track
		const unsigned MAX_DELTA = citems * 5;
		unsigned total_delta = 0;

		// for every selected item,
		// find the discogs track that most closely matches its duration
		// 

		std::map<int, bool> used_indexes;

		for (size_t i = 0; i < citems; i++) {
			item = finfo_manager->items.get_item(i);
			int min_delta = 0xFFFFFF;
			int bad_min_delta = 0xFFFFFF;
			
			int min_delta_index = 0;
			int min_delta_disc = 0;
			int min_delta_track = 0;

			int cnum = 0;
			for (size_t d = 0; d < release->discs.get_size(); d++) {
				ReleaseDisc_ptr &disc = release->discs[d];
				for (size_t j = 0; j < disc->tracks.get_size(); j++) {
					ReleaseTrack_ptr &track = disc->tracks[j];
					int file_length = (int)round(item->get_length());
					int discogs_length = track->discogs_duration_seconds + track->discogs_hidden_duration_seconds;
					int delta = abs(file_length - discogs_length);
					if (delta < min_delta) {
						if (used_indexes[cnum]) {
							bad_min_delta = delta;
						}
						else {
							min_delta = delta;
							min_delta_index = cnum;
							min_delta_disc = d;
							min_delta_track = j;
						}
					}
					cnum++;
				}
			}
			// give up if the same track is the best match twice
			if (bad_min_delta < min_delta) {
				return MATCH_FAIL;
			}
			total_delta += min_delta;
			if (total_delta > MAX_DELTA) {
				return MATCH_FAIL;
			}
			used_indexes[min_delta_index] = true;
			mappings[i].enabled = true;
			mappings[i].discogs_disc = min_delta_disc;
			mappings[i].discogs_track = min_delta_track;
		}
	}
	else {
		return MATCH_NA;
	}
	return MATCH_SUCCESS;
}

int TagWriter::order_tracks_by_number(track_mappings_list_type &mappings) {
	if (!CONF.match_tracks_using_number) {
		return MATCH_NA;
	}

	const size_t citems = finfo_manager->items.get_count();

	int missing = 0;
	metadb_handle_ptr item;
	
	pfc::array_t<std::pair<int, int>> used;

	file_info_impl finfo;
	for (size_t i = 0; i < citems; i++) {
		item = finfo_manager->items.get_item(i);
		item->get_info(finfo);

		const char * disc_number_s = finfo.meta_get("DISCNUMBER", 0);
		const char * track_number_s = finfo.meta_get("TRACKNUMBER", 0);
		int disc_number, track_number;
		try {
			if (disc_number_s != nullptr) {
				disc_number = std::stoi(disc_number_s);
			}
			else {
				disc_number = 0;
			}
			if (track_number_s != nullptr) {
				track_number = std::stoi(track_number_s);
			}
			else {
				missing++;
				continue;
			}
		}
		catch (std::exception) {
			missing++;
			continue;
		}

		int total_num = 0;
		bool found = false;
		for (size_t d = 0; d < release->discs.get_size() && !found; d++) {
			ReleaseDisc_ptr &disc = release->discs[d];
			for (size_t j = 0; j < disc->tracks.get_size(); j++) {
				total_num++;
				ReleaseTrack_ptr &track = disc->tracks[j];
				bool match;
				match = (!disc_number || disc_number == disc->disc_number) && (track->track_number == track_number || track->disc_track_number == track_number);
				match = match || (!disc_number && track_number == total_num);
				if (match) {
					for (size_t k = 0; k < used.get_count(); k++) {
						if (used[k].first == d && used[k].second == j) {
							return MATCH_FAIL;
						}
					}
					mappings[i].enabled = true;
					mappings[i].discogs_disc = d;
					mappings[i].discogs_track = j;
					found = true;
					used.append_single(std::pair<int, int>(d, j));
					break;
				}
			}
		}
		if (!found) {
			missing++;
		}
	}
	if (missing == citems) {
		return MATCH_NA;
	}
	else if (missing) {
		return MATCH_FAIL;
	}
	/*for (size_t i = 0; i < track_order.get_size(); i++) {
		if (track_order[i] == -1) {
			return MATCH_FAIL;
		}
	}*/
	return MATCH_SUCCESS;
}

int TagWriter::order_tracks_by_assumption(track_mappings_list_type &mappings) {
	if (!CONF.assume_tracks_sorted) {
		return MATCH_NA;
	}
	return MATCH_ASSUME;
}

bool check_multiple_results(pfc::array_t<string_encoded_array> where, string_encoded_array what) {
	for (size_t k = 0; k < where.get_size(); k++) {
		if (where[k].get_cvalue() != what.get_cvalue()) {
			return true;
		}
	}
	return false;
}

void TagWriter::generate_tags(tag_mapping_list_type* alt_mappings, threaded_process_status& p_status, abort_callback& p_abort) {
	
	tag_mapping_list_type* ptags = alt_mappings ? alt_mappings : &TAGS;
	
	tag_results.force_reset();
	will_modify = false;

	pfc::array_t<persistent_store> track_stores;
	for (size_t j = 0; j < track_mappings.get_count(); j++) {
		track_stores.append_single(persistent_store());
	}
	persistent_store prompt_store;

	unsigned long lkey = encode_mr(0, atoi(release->master_id));

	MasterRelease_ptr master = discogs_interface->get_master_release(lkey);

	for (size_t wtags = 0; wtags < ptags->get_size(); wtags++) {
		const tag_mapping_entry& entry = ptags->get_item_ref(wtags);
		if ((entry.enable_write) || (entry.enable_update)) {
			tag_result_ptr result = std::make_shared<tag_result>();

			bool multiple_results = false;
			bool multiple_old_results = false;

			for (size_t wtracks = 0; wtracks < track_mappings.get_count(); wtracks++) {
				const track_mapping& mapping = track_mappings[wtracks];

				bool token_added = false;
				bool meta_changed = false;

				if (!mapping.enabled) {
					continue;
				}

				size_t file_index = mapping.file_index;
				int disc_index = mapping.discogs_disc;
				int trac_index = mapping.discogs_track;

				metadb_handle_ptr item = nullptr;

				try {
					
					//TODO: should have been checked earlier
					if (file_index < finfo_manager->get_item_count()) {

						item = finfo_manager->get_item_handle(file_index);

					}
					else {
						//no more files available (more tracks than files)
						//break track loop and keep processing tags
						break;
					}
				}
				catch (std::exception e) {
					std::string dbexception(e.what());
					continue;
				}
				file_info& info = finfo_manager->get_item(file_index);

				const ReleaseDisc_ptr& disc = release->discs[disc_index];
				const ReleaseTrack_ptr& track = disc->tracks[trac_index];

				titleformat_hook_impl_multiformat hook(p_status, &master, &release, &disc, &track, &info, &track_stores[wtracks], &prompt_store);
				hook.set_files(finfo_manager);

				pfc::string8 str;
				try {
					//if (STR_EQUAL(entry.formatting_script, "")) {
					//	int dbug = 1;
					//}
					entry.formatting_script->run_hook(item->get_location(), &info, &hook, str, nullptr);
					string_encoded_array value(str);
					if (!multiple_results) {
						for (size_t k = 0; k < result->value.get_size(); k++) {
							if (result->value[k].get_cvalue() != value.get_cvalue()) {
								multiple_results = true;
								break;
							}
						}
					}
					result->value.append_single(value);
				}
				catch (foo_discogs_exception& e) {
					foo_discogs_exception ex;
					ex << "Error generating tag " << entry.tag_name << " [" << e.what() << "] for file " << item->get_path();
					throw ex;
				}

				// TODO: read old values separately so they don't have to be re-read multiple times...

				try {

					const size_t old_count = info.meta_get_count_by_name(entry.tag_name);
					string_encoded_array old_value;

					// APPROVING 1/2 (count based)

					if (old_count == 0) {
						
						//note: does not approve replacing empty val with another empty val
						bool approved;
						string_encoded_array newvalue(result->value[result->value.get_count() - 1]);
						
						approved = !newvalue.has_blank() && entry.enable_write;

						//debug - result->result_approved = approved;

						result->r_approved.append_single(approved);
						token_added = true; // approval value (true or false) token added

					}
					else {
						if (old_count == 1) {
							old_value.set_value(info.meta_get(entry.tag_name, 0));
						}
						else {
							for (size_t i = 0; i < old_count; i++) {
								old_value.append_item(info.meta_get(entry.tag_name, i));
							}
						}
					}

					old_value.encode();

					if (!multiple_old_results) {
						for (size_t k = 0; k < result->old_value.get_size(); k++) {
							if (result->old_value[k].get_cvalue() != old_value.get_cvalue()) {
								multiple_old_results = true;
								break;
							}
						}
					}

					result->old_value.append_single(old_value);

					meta_changed = false;
					const int last_val_ndx = result->value.get_count() - 1;
					string_encoded_array newvalue(result->value[last_val_ndx]);
					string_encoded_array oldvalue(result->old_value[last_val_ndx]);
					meta_changed = oldvalue.has_diffs(newvalue);
					result->changed |= meta_changed;
					
					// SUBITEMS APPROVING 2/2 (change based)

					if (!token_added) {
						result->r_approved.append_single(meta_changed && entry.enable_update);
					}
					else {
						PFC_ASSERT(old_count == 0);
						//debug...
						//NOTHING TO DO?
						//result->r_approved[result->r_approved.get_count() - 1] |= (old_count > 1 && meta_changed && entry.enable_update;
					}
					result->result_approved |= result->r_approved[result->r_approved.get_count() - 1];
				}
				catch (std::exception& e) {
					foo_discogs_exception ex;
					ex << "Error reading tag " << entry.tag_name << " [" << e.what() << "] for file " << item->get_path();
					throw ex;
				}
			}
			if (!multiple_results) {
				result->value.set_size_discard(1);
			}
			if (!multiple_old_results) {
				result->old_value.set_size_discard(1);
			}

			will_modify |= result->result_approved;

			result->tag_entry = &entry;
			tag_results.append_single(std::move(result));
		}
	}
}

void TagWriter::write_tags_track_map() {
	
	finfo_manager->invalidate_all();
	bool binvalidate = false;

	for (size_t i = 0; i < track_mappings.get_count(); i++) {
		const track_mapping &mapping = track_mappings[i];

		if (!mapping.enabled) {
			continue;
		}

		size_t file_index = mapping.file_index;
		
		if (file_index >= finfo_manager.get()->get_item_count()) {
			//run out of tracks... skip unmatched files
			break;
		}
		metadb_handle_ptr item = finfo_manager->get_item_handle(file_index);
		file_info &info = finfo_manager->get_item(file_index);

		const size_t count = tag_results.get_size();
		for (size_t j = 0; j < count; j++) {
			const tag_result_ptr &result = tag_results[j];
			string_encoded_array *value;

			bool approved = false;
			if (result->result_approved) {
				approved = result->r_approved[i];
			}
			else {
				//todo: should have result->result_approved been modified earlier by r_usr_approved values?
				if (result->r_usr_modded.get(i)) {
					approved = result->r_usr_approved.get(i);
				}
			}

			if (approved) {

				if (result->value.get_size() == 1) {
					value = &(result->value[0]);
				}
				else {
					value = &(result->value[i]);
				}

				if (value->has_array()) {
					
					pfc::string8 catch_mem = value->get_cvalue();
					check_mem(result->tag_entry, value);

					value->limit_depth(1);
					write_tag(item, info, *(result->tag_entry), value->get_citems());
				}
				else {
					pfc::string8 value_lf;
					value->get_cvalue_lf(value_lf);

					pfc::string8 catch_mem = value_lf;
					check_mem(result->tag_entry, value);

					write_tag(item, info, *(result->tag_entry), value_lf);
				}
				binvalidate = true;
			}
		}

		if (CONF.remove_other_tags) {
			size_t j = 0;
			while (j < info.meta_get_count()) {
				const char * tag_name = info.meta_enum_name(j);
				bool remove = true;
				for (size_t k = 0; k < TAGS.get_size(); k++) {
					auto &tag = TAGS.get_item_ref(k);
					//if (STR_EQUAL(tag_name, tag.tag_name)) {
					if (pfc::stringCompareCaseInsensitive(tag_name, tag.tag_name) == 0) {
						remove = false;
						break;
					}
				}
				if (remove) {
					for (size_t k = 0; k < CONF.remove_exclude_tags.get_size(); k++) {
						//if (STR_EQUAL(tag_name, CONF.remove_exclude_tags[k])) {
						if (pfc::stringCompareCaseInsensitive(tag_name, CONF.remove_exclude_tags[k]) == 0) {
							remove = false;
							break;
						}
					}
				}
				if (remove) {
					info.meta_remove_index(j);
					binvalidate = true;
				}
				else {
					j++;
				}
			}
		}
		if (binvalidate)
			finfo_manager->validate_item(file_index);
	}
}

void TagWriter::write_tags () {

	if (cfg_preview_dialog_track_map)
		write_tags_track_map();
	else
		write_tags_v23();

}

void TagWriter::write_tags_v23() {

	finfo_manager->invalidate_all();
	bool binvalidate = false;

	for (size_t i = 0; i < track_mappings.get_count(); i++) {
		const track_mapping& mapping = track_mappings[i];

		if (!mapping.enabled) {
			continue;
		}

		size_t file_index = mapping.file_index;

		if (file_index > finfo_manager.get()->get_item_count() - 1) {
			//run out of tracks... skip unmatched files
			break;
		}
		metadb_handle_ptr item = finfo_manager->get_item_handle(file_index);
		file_info& info = finfo_manager->get_item(file_index);

		const size_t count = tag_results.get_size();
		for (size_t j = 0; j < count; j++) {
			const tag_result_ptr& result = tag_results[j];

			if (!stricmp_utf8(result->tag_entry->tag_name, "DISCOGS_TRACK_CREDITS")) {
				int debug = 1;
			}

			string_encoded_array* value;

			if (result->result_approved) {

				if (result->value.get_size() == 1) {
					value = &(result->value[0]);
				}
				else {
					value = &(result->value[i]);
				}

				if (value->has_array()) {
					value->limit_depth(1);
					write_tag(item, info, *(result->tag_entry), value->get_citems());
				}
				else {
					pfc::string8 value_lf;
					value->get_cvalue_lf(value_lf);
					write_tag(item, info, *(result->tag_entry), value_lf);
				}
				binvalidate = true;
			}
		}

		if (CONF.remove_other_tags) {
			size_t j = 0;
			while (j < info.meta_get_count()) {
				const char* tag_name = info.meta_enum_name(j);
				bool remove = true;
				for (size_t k = 0; k < TAGS.get_size(); k++) {
					auto& tag = TAGS.get_item_ref(k);
					if (pfc::stringCompareCaseInsensitive(tag_name, tag.tag_name) == 0) {
						remove = false;
						break;
					}
				}
				if (remove) {
					for (size_t k = 0; k < CONF.remove_exclude_tags.get_size(); k++) {
						if (pfc::stringCompareCaseInsensitive(tag_name, CONF.remove_exclude_tags[k]) == 0) {
							remove = false;
							break;
						}
					}
				}
				if (remove) {
					info.meta_remove_index(j);
					binvalidate = true;
				}
				else {
					j++;
				}
			}
		}
		if (binvalidate)
			finfo_manager->validate_item(file_index);
	}
}

void TagWriter::write_tag(metadb_handle_ptr item, file_info &info, const tag_mapping_entry &entry, const pfc::string8 &tag_value) {
	if (tag_value.get_length()) {
		info.meta_set(entry.tag_name, tag_value);
	}
	else {
		info.meta_remove_field(entry.tag_name);
	}
}

void TagWriter::write_tag(metadb_handle_ptr item, file_info &info, const tag_mapping_entry &entry/*, const pfc::array_t<bool> &r_approved*/, const pfc::array_t<string_encoded_array> &tag_values) {
	bool valid = false;
	size_t i = 0;
	size_t meta = 0;
	const size_t count = tag_values.get_size();
	while (i < count) {
		if (tag_values[i].get_pure_cvalue().get_length()) {
			if (!valid) {
				valid = true;
				meta = info.meta_set(entry.tag_name, pfc::lineEndingsToWin(tag_values[i].get_pure_cvalue()));
				//meta = info.meta_set(entry.tag_name, tag_values[i].get_pure_cvalue());
			}
			else {
				//DEBUG
				info.meta_add_value(meta, pfc::lineEndingsToWin(tag_values[i].get_pure_cvalue()));
				//info.meta_add_value(meta, tag_values[i].get_pure_cvalue());
			}
		}
		i++;
	}
	if (!valid) {
		info.meta_remove_field(entry.tag_name);
	}
}
