#include "stdafx.h"
#include "foo_discogs.h"
#include "multiformat.h"
#include "tag_writer.h"

TagWriter::TagWriter(file_info_manager_ptr finfo_manager, Release_ptr release) : m_finfo_manager(finfo_manager), release(release), tag_results_mask_mode(PreView::Undef) {

	pfc::string8 buffer;
	buffer << "[%" << "track" << "%]";
	static_api_ptr_t<titleformat_compiler>()->compile_force(m_track_script, buffer.c_str());
}

Release_ptr TagWriter::GetRelease() {
	return release;
}

const t_size TagWriter::GetArtCount(art_src artsrc) {
	if (!release) {
		return 0;
	}
	if (artsrc == art_src::alb) return release->images.get_count();
	else return get_artists_art_count();
}

const pfc::array_t<ReleaseArtist_ptr> TagWriter::GetArtists() {
	if (!release) {
		return pfc::array_t<ReleaseArtist_ptr>{};
	}
	else {
		return release->artists;
	}
}

const t_size TagWriter::get_artists_art_count() {
	if (!release) {
		return 0;
	}
	size_t res = release->images.get_count();
	for (auto wra : release->artists) {
		res += wra->full_artist->images.get_count();
	}
	return res;
}

bool TagWriter::Staging_Results() {
	for (auto wr : tag_results) {
		if (wr->result_approved) {
			return true;
		}
	}
	for (auto wr : tag_results) {
		bool usr_app = wr->r_usr_approved.find_first(true, 0, wr->r_usr_approved.size() < wr->r_usr_approved.size());
		if (usr_app) {
			return true;
		}
	}

	return false;
}

void TagWriter::ResetMask() {
	tag_results_mask = {};
	tag_results_mask_force_wu = false;
	tag_results_mask_mode = PreView::Undef;
}

const std::vector<int(TagWriter::*)(track_mappings_list_type &)> TagWriter::m_matching_functions = {
	&TagWriter::order_tracks_by_duration,
	&TagWriter::order_tracks_by_number,
	&TagWriter::order_tracks_by_assumption
};

void TagWriter::match_tracks() {
	m_match_status = route_discogs_track_order(m_track_mappings);
	if (m_match_status != MATCH_SUCCESS) {
		const size_t citems = m_finfo_manager->items.get_count();
		size_t ctracks = 0;
		for (size_t i = 0; i < release->discs.get_count(); i++) {
			for (size_t j = 0; j < release->discs[i]->tracks.get_count(); j++) {
				if (ctracks < citems) {					
					track_mapping& tm = m_track_mappings[ctracks];
					tm.enabled = true;
					tm.discogs_disc = i;
					tm.discogs_track = j;

					ctracks++;
				}
				else {
					//run out of files to match
					return;
				}
				
			}
		}
	}
}

int TagWriter::route_discogs_track_order(track_mappings_list_type &mappings) {
	std::map<int, bool> track_number_exist;

	const size_t citems = m_finfo_manager->items.get_count();
	const size_t ctracks = release ? release->get_total_track_count() : 0;
	const size_t cmappings = ctracks;
	mappings.force_reset();
	mappings.resize(cmappings);
	size_t cf_ndx = 0;
	for (size_t i = 0; i < (release ? release->discs.get_count() : 0); i++) {
		for (size_t j = 0; j < (release ? release->discs[i]->tracks.get_count() : 0); j++) {
			track_mapping& tm = mappings[cf_ndx];
			tm.file_index = cf_ndx < citems ? cf_ndx : -1;
			tm.enabled = false;			
			tm.discogs_disc = i; //zero based
			tm.discogs_track = j;
			cf_ndx++;
		}		
	}
	if (ctracks != m_finfo_manager->items.get_count()) {
		return MATCH_FAIL;
	}

	track_mappings_list_type temp;
	temp.set_size(ctracks);
	for (size_t i = 0; i < ctracks; i++) {
		temp[i].file_index = i;		
	}

	int status;
	bool initialized = false;
	bool assumed = false;

	for (size_t i = 0; i < m_matching_functions.size(); i++) {
		status = (this->*m_matching_functions[i])(temp);
		if (status == MATCH_FAIL) {
			return MATCH_FAIL;
		}
		else if (status == MATCH_SUCCESS || status == MATCH_ASSUME) {
			if (!initialized) {
				for (size_t j = 0; j < ctracks; j++) {
					mappings[j].discogs_disc = temp[j].discogs_disc;
					mappings[j].discogs_track = temp[j].discogs_track;
					mappings[j].enabled = temp[j].enabled;
				}
				initialized = true;
				assumed = status == MATCH_ASSUME;
			}
			else {
				for (size_t j = 0; j < ctracks; j++) {
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
	
	const size_t citems = m_finfo_manager->items.get_count();
	// first try to match tracks on nearest track length if the release has lengths
	metadb_handle_ptr item;
	
	if (release && release->has_tracklengths()) {
		// max delta of 5 seconds per track
		const unsigned MAX_DELTA = citems * 5;
		unsigned total_delta = 0;

		// for every selected item,
		// find the discogs track that most closely matches its duration
		// 

		std::map<int, bool> used_indexes;

		for (size_t i = 0; i < citems; i++) {
			item = m_finfo_manager->items.get_item(i);
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

	const size_t citems = m_finfo_manager->items.get_count();
	int missing = 0;
	metadb_handle_ptr item;
	
	pfc::array_t<std::pair<size_t, size_t>> used;

	file_info_impl finfo;
	for (size_t i = 0; i < citems; i++) {
		item = m_finfo_manager->items.get_item(i);
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
					used.append_single(std::pair<size_t, size_t>(d, j));
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

	tag_mapping_list_type* ptags;
	if (alt_mappings) {
		ptags = alt_mappings;
	}
	else {
		ptags = &TAGS;
	} 

	atm_tag_results_ready = false;

	auto masksize = tag_results_mask.size();

	// CAN NOT PROPERLY CALCUALTE WHICH RESULTS TO UPDATE
	// IN MASKED MODE SO FALSE FOR NOW:

	masksize = false;

	auto mask_left = 0;
	size_t mask_count_enabled = 0;

	std::vector<GUID> vmaskguids;

	if (tag_results.get_count() && masksize) {
		size_t fpos = 0;
		while ((fpos = tag_results_mask.find_first(true, fpos, masksize)) < masksize) {
			auto dbg = tag_results[fpos];
			vmaskguids.emplace_back(dbg->tag_entry->guid_tag);
			++fpos;
		}
		mask_left = vmaskguids.size();
		mask_count_enabled = 0;
	}
	else {
		tag_results.force_reset();
		//will_modify = false;
	}

	pfc::array_t<persistent_store> track_stores;
	for (size_t j = 0; j < m_track_mappings.get_count(); j++) {
		track_stores.append_single(persistent_store());
	}

	persistent_store prompt_store;

	size_t lkey = encode_mr(0, release ? atoi(release->master_id): 0);
	MasterRelease_ptr master = discogs_interface->get_master_release(lkey);

	for (size_t wtags = 0; wtags < ptags->get_size(); wtags++) {

		const tag_mapping_entry& entry = ptags->get_item_ref(wtags);

		if ((entry.enable_write) || (entry.enable_update)) {


			if (masksize) {
				
				++mask_count_enabled;

				if (!mask_left) {
					break;
				}

				bool bskip = false;

				GUID guid_wtag = entry.guid_tag;

				auto found_it = std::find_if(vmaskguids.begin(), vmaskguids.end(), [&](const GUID& elem) {
					return pfc::guid_equal(elem, guid_wtag);
					});

				//todo: protected to mask
				if (found_it == vmaskguids.end()) {
					continue;
				}

				--mask_left;
			}

			tag_result_ptr result = std::make_shared<tag_result>();

			bool multiple_results = false;
			bool multiple_old_results = false;

			for (size_t wtracks = 0; wtracks < m_track_mappings.get_count(); wtracks++) {
				const track_mapping& mapping = m_track_mappings[wtracks];

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
					if (file_index < m_finfo_manager->get_item_count()) {

						item = m_finfo_manager->get_item_handle(file_index);

					}
					else {
						break;
					}
				}
				catch (std::exception e) {
					std::string dbexception(e.what());
					continue;
				}
				file_info& info = m_finfo_manager->get_item(file_index);

				const ReleaseDisc_ptr& disc = release->discs[disc_index];
				const ReleaseTrack_ptr& track = disc->tracks[trac_index];

				titleformat_hook_impl_multiformat hook(p_status, &master, &release, &disc, &track, &info, &track_stores[wtracks], &prompt_store);
				hook.set_files(m_finfo_manager);

				pfc::string8 str;
				try {
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
				catch (http_404_exception) {
					log_msg("error runing...");
				}
				catch (foo_discogs_exception& e) {
					atm_tag_results_ready = true;
					foo_discogs_exception ex;
					ex << "Error generating tag " << entry.tag_name << " [" << e.what() << "] for file " << item->get_path();
					throw ex;
				}
				try {

					const size_t old_count = info.meta_get_count_by_name(entry.tag_name);
					string_encoded_array old_value;

					// APPROVING 1/2 (count based)

					if (old_count == 0) {
						
						//note: does not approve replacing empty val with another empty val
						bool approved;
						string_encoded_array newvalue(result->value[result->value.get_count() - 1]);
						
						approved = !newvalue.has_blank() && entry.enable_write;
						result->r_approved.append_single(approved);
						token_added = true;

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
					}
					result->result_approved |= result->r_approved[result->r_approved.get_count() - 1];
				}
				catch (std::exception& e) {
					atm_tag_results_ready = true;
					foo_discogs_exception ex;
					ex << "Error reading tag " << entry.tag_name << " [" << e.what() << "] for file " << item->get_path();
					throw ex;
				}
			} //tracks

			if (!multiple_results) {
				result->value.set_size_discard(1);
			}
			if (!multiple_old_results) {
				result->old_value.set_size_discard(1);
			}

			result->tag_entry = &entry;

			if (masksize) {
				tag_results[mask_count_enabled] = std::move(result);
			}
			else {
				tag_results.append_single(std::move(result));
			}
		} //write or update enabled
	} //walk tracks

	atm_tag_results_ready = true;
}


void check_mem(const tag_mapping_entry* tag_entry, const string_encoded_array* value) {
	if (STR_EQUAL(tag_entry->tag_name, TAG_ARTIST_ID) ||
		STR_EQUAL(tag_entry->tag_name, TAG_RELEASE_ID)) {
		
		bool berror = false;
		std::string buffer;
		if (value->has_array()) {
			for (size_t i = 0; i < value->get_citems().get_count(); i++) {
				buffer = value->get_item(i);
				if (berror = !is_number(buffer)) {
					break;
				}
			}
		}
		else {
			buffer = value->print().c_str();
			berror = !is_number(buffer);
		}
		if (berror) {
			foo_discogs_exception e("foo_discogger rejected an attempt to write invalid tag values");
			log_msg(e.what());
			throw e;
		}
	}
}

struct write_tag_nfo {

	metadb_handle_ptr item = nullptr;
	file_info* info;
	tag_mapping_entry entry;
	pfc::string8 tag_value;
	pfc::array_t<string_encoded_array> tag_values_array;
};

void process_result(const metadb_handle_ptr item, file_info* info, size_t ndx_track_mappings, const tag_result_ptr& result, bool bhasmask, bool bforce_wu, bool bmasked, bool &bvalidate, write_tag_nfo &out_write_info) {


	string_encoded_array* value;

	bool approved = false;
	if (result->result_approved) {

		approved = result->r_approved[ndx_track_mappings];
		if (!approved) {

			if (result->r_usr_modded.get(ndx_track_mappings)) {
				approved = result->r_usr_approved.get(ndx_track_mappings);
			}
			/*else*/
			if (bhasmask && bforce_wu) {

				auto old_val = result->old_value.get_count() > ndx_track_mappings ? result->old_value[ndx_track_mappings] : result->old_value[0].print();
				auto new_val = result->value.get_count() > ndx_track_mappings ? result->value[ndx_track_mappings] : result->value[0].print();
				bool beq = old_val.print().equals(new_val.print());

				if (!beq) {

					approved = bmasked;
				}
			}
		}
	}
	else {

		bool release_id_mod = STR_EQUAL(TAG_RELEASE_ID, result->tag_entry->tag_name.get_ptr());
		release_id_mod &= !CONF.awt_alt_mode();

		if (release_id_mod) {
			if (!(result->tag_entry->enable_write)) {
				approved = true;
			}
		}
		else {
			//todo: rev maybe remove turn else into two sequencial ifs
			if (result->r_usr_modded.get(ndx_track_mappings)) {
				approved = result->r_usr_approved.get(ndx_track_mappings);
			}
			/*else*/
			if (bhasmask && bforce_wu) {

				auto old_val = result->old_value.get_count() > ndx_track_mappings ? result->old_value[ndx_track_mappings] : result->old_value[0].print();
				auto new_val = result->value.get_count() > ndx_track_mappings ? result->value[ndx_track_mappings] : result->value[0].print();
				bool beq = old_val.print().equals(new_val.print());

				if (!beq) {

					approved = bmasked;
				}
			}
		}
	}
	if (approved) {
		if (result->value.get_count() > ndx_track_mappings) {



			value = &(result->value[ndx_track_mappings]);
		}
		else {

			value = &(result->value[0]);
		}

		if (value->has_array()) {

			pfc::string8 catch_mem = value->get_cvalue();
			check_mem(result->tag_entry, value);

			value->limit_depth(1);
			
			//WRITE array
			out_write_info.item = item;
			out_write_info.info = info;
			out_write_info.entry = *(result->tag_entry);
			out_write_info.tag_values_array = value->get_citems();

		}
		else {
			pfc::string8 value_lf;
			value->get_cvalue_lf(value_lf);

			pfc::string8 catch_mem = value_lf;
			check_mem(result->tag_entry, value);
			
			//WRITE single
			out_write_info.item = item;
			out_write_info.info = info;
			out_write_info.entry = *(result->tag_entry);
			out_write_info.tag_value = value_lf;
		}
		bvalidate = true;
	}
}

void TagWriter::write_tags() {
	
	m_finfo_manager->invalidate_all();

	bit_array_bittable info_man_validate_mask(bit_array_false(), m_finfo_manager->items.get_count());
	
	bool bhasmask = tag_results_mask.size();

	//tracks
	for (size_t i = 0; i < m_track_mappings.get_count(); i++) {

		const track_mapping &mapping = m_track_mappings[i];

		if (!mapping.enabled) {
			continue;
		}

		size_t file_index = mapping.file_index;

		if (file_index >= m_finfo_manager.get()->get_item_count()) {
			//run out of tracks... skip unmatched files
			break;
		}
		metadb_handle_ptr item = m_finfo_manager->get_item_handle(file_index);
		file_info &info = m_finfo_manager->get_item(file_index);


		if (!tag_results_mask.size()) {
		
			// bulk
			
			// walk track results
			for (size_t j = 0; j < tag_results.get_size(); j++) {

				const tag_result_ptr& result = tag_results[j];
				//todo:
				bool release_id_mod = STR_EQUAL(TAG_RELEASE_ID, result->tag_entry->tag_name.get_ptr());
				release_id_mod &= !CONF.awt_alt_mode();

				if (bhasmask && !tag_results_mask.get(j) && !release_id_mod /*&& (!(result->tag_entry->freeze_tag_name && result->tag_entry->enable_write))*/)
				{
					continue;
				}

				bool bvalidate = false;
				write_tag_nfo out_write_info = { 0 };
				process_result(item, &info, i /*ndx_track_mappings*/, result, bhasmask, tag_results_mask_force_wu, false, bvalidate, out_write_info);

				if (out_write_info.item.get_ptr()) {
					if (out_write_info.tag_values_array.get_count()) {
						write_tag(out_write_info.item, *(out_write_info.info), out_write_info.entry, out_write_info.tag_values_array);
					}
					else {
						write_tag(out_write_info.item, *(out_write_info.info), out_write_info.entry, out_write_info.tag_value);
					}
					info_man_validate_mask.set(file_index, bvalidate);
				}

			} // walked results in track

		}

		else {
		
			//selection
		
			size_t mask_size = tag_results_mask.size();
			size_t fpos = 0;

			while ((fpos = tag_results_mask.find_first(true, fpos, mask_size)) < mask_size) {
			
				const tag_result_ptr& result = tag_results[fpos];
				//todo:
				bool release_id_mod = STR_EQUAL(TAG_RELEASE_ID, result->tag_entry->tag_name.get_ptr());
				release_id_mod &= !CONF.awt_alt_mode();

				bool bvalidate = false;
				write_tag_nfo out_write_info = { 0 };
				process_result(item, &info, i , result, bhasmask, tag_results_mask_force_wu, mask_size, bvalidate, out_write_info);
				
				if (out_write_info.item.get_ptr()) {
					if (out_write_info.tag_values_array.get_count()) {
						write_tag(out_write_info.item, *(out_write_info.info), out_write_info.entry, out_write_info.tag_values_array);
					}
					else {
						write_tag(out_write_info.item, *(out_write_info.info), out_write_info.entry, out_write_info.tag_value);
					}
					info_man_validate_mask.set(file_index, bvalidate);
				}

				fpos++;
			}
		
		}

		// keep on walking this track...
		// remove other tags

		if (CONF.remove_other_tags) {
			const size_t jcount = info.meta_get_count();
			bit_array_bittable delete_mask(bit_array_true(), jcount);

			//loop metas

			for (size_t j = 0; j < jcount; j++) {
				
				const char * tag_name = info.meta_enum_name(j);
				
				//do not remove current map tags
				for (size_t k = 0; k < TAGS.get_size(); k++) {
					const auto &tag = TAGS.get_item_ref(k);
					bool match = !pfc::stringCompareCaseInsensitive(tag_name, tag.tag_name);
					if (match) {
						delete_mask.set(j, false);
						break;
					}
				}

				//do not remove from exclusion list
				if (delete_mask.get(j)) {
					for (size_t k = 0; k < CONF.remove_exclude_tags.get_size(); k++) {
						bool match = !pfc::stringCompareCaseInsensitive(tag_name, CONF.remove_exclude_tags[k]);
						if (match) {
							delete_mask.set(j, false);
							break;
						}
					}
				}
			} //end loop metas

			info.meta_remove_mask(delete_mask);
			bool bvalidate = info_man_validate_mask.get(file_index);
			bvalidate |= jcount != info.meta_get_count();
			info_man_validate_mask.set(file_index, bvalidate);

		} // end remove other tags

		if (info_man_validate_mask.get(file_index)) {
			m_finfo_manager->validate_item(file_index);
		}

	}//end tracks loop
}

// single vals

void TagWriter::write_tag(metadb_handle_ptr item, file_info &info, const tag_mapping_entry &entry, const pfc::string8 &tag_value) {

	bool blog = CONF.tag_save_flags & TAGSAVE_LOG_FLAG;
	pfc::string8 logmsg, logtrack;

	//log
	if (blog) {
		pfc::string8 tmptrack;
		item->format_title(nullptr, tmptrack, m_track_script, nullptr);
		if (tmptrack.get_length()) {
			logtrack << (PFC_string_formatter() << "#" << tmptrack);
		}
		else {
			logtrack = "";
		}
		logmsg << logtrack << "T < single-val: " << entry.tag_name << " < " << (tag_value.get_length()? tag_value : "(empty)");
		log_msg(logmsg); logmsg = "";
	} //..log

	size_t cexist = info.meta_get_count_by_name(entry.tag_name);

	if (cexist > 1 && tag_value.get_length()) {

		//log
		if (blog) {
			logmsg << logtrack << "Td < clearing " << cexist << " previous " << entry.tag_name << " entries";
			log_msg(logmsg); logmsg = "";
		} //..log

		info.meta_remove_field(entry.tag_name);
		cexist = false;
	}

	if (tag_value.get_length()) {
		//log
		if (blog) {
			logmsg << logtrack << "Tw < set " << entry.tag_name << " = " << (tag_value.get_length() ? tag_value : "(empty)");
			log_msg(logmsg); logmsg = "";		
		}	//..log
		try {
			auto res = info.meta_set(entry.tag_name, tag_value);
		}
		catch (...) {
			throw;
		}

	}
	else {
		//log
		if (blog) {
			logmsg << logtrack << "Td < Removing field " << entry.tag_name;
			log_msg(logmsg); logmsg = "";
		}	//..log

		info.meta_remove_field(entry.tag_name);
	}
}

// multi-val

void TagWriter::write_tag(metadb_handle_ptr item, file_info &info, const tag_mapping_entry &entry, const pfc::array_t<string_encoded_array> &tag_values) {

	bool blog = CONF.tag_save_flags & TAGSAVE_LOG_FLAG;
	pfc::string logmsg, logtrack;

	bool bauto_add_mfield = CONF.tag_save_flags & TAGSAVE_AUTO_MULTIV_FLAG;
	bool bauto_fresh_mfield = false;
	pfc::string8 forced_flat;
	if (!entry.is_multival_meta) {

		if (bauto_add_mfield) {

			if (is_multivalue_meta(entry.tag_name)) {
				bauto_fresh_mfield = true;
			}
			else {
				//log
				if (blog) {
					logmsg << "T < auto-insert multi-value tag: " << entry.tag_name;
					log_msg(logmsg); logmsg = "";
				}//..log

				CONF.multivalue_fields << ";" << entry.tag_name;
				CONF.save(CConf::cfgFilter::CONF, CONF, CFG_MULTIVALUE_FIELDS);
			}
		}
		else {

			bool bneedsep = false;
			for (size_t ws = 0; ws < tag_values.get_size(); ws++) {

				pfc::string8 str_ws = ltrim(tag_values[ws].get_pure_cvalue());

				if (str_ws.get_length()) {
					if (bneedsep) forced_flat << ";";
					forced_flat << ltrim(tag_values[ws].get_pure_cvalue());
					bneedsep = true;
				}
			}
		}
	}

	//log
	if (blog) {
		pfc::string8 tmptrack;
		item->format_title(nullptr, tmptrack, m_track_script, nullptr);
		if (tmptrack.get_length()) {
			logtrack << (PFC_string_formatter() << "#" << tmptrack);
		}
		else {
			logtrack = "";
		}
		pfc::string8 dbgflat;
		for (size_t ws = 0; ws < tag_values.get_size(); ws++) {
			if (ws) dbgflat << " * ";
			dbgflat << tag_values[ws].get_pure_cvalue();
		}
		logmsg << logtrack << "T < multi-value (array " << tag_values.get_size() << ") < " << (!entry.is_multival_meta && !bauto_fresh_mfield ? "not " : "") << "multi-val "
			<< entry.tag_name << " < " << (tag_values.get_size() ? dbgflat : "(empty)");
		log_msg(logmsg); logmsg = "";
	}//..log

	size_t cexist = info.meta_get_count_by_name(entry.tag_name);
	
	size_t meta = 0;
	const size_t cvalues = tag_values.get_size();

	if (cexist) {
		meta = info.meta_find(entry.tag_name);
	}

	std::vector<pfc::string8> valready_there;
	bool already_there = cexist && cexist == cvalues;
	bool bforce_flat = (tag_values.get_size() > 1 && !entry.is_multival_meta) && !bauto_add_mfield && !bauto_fresh_mfield;

	if (already_there && !bforce_flat) {
		valready_there.resize(cexist);
		for (size_t i = 0; i < cexist; i++) {
			pfc::string8 tv = info.meta_get(entry.tag_name, i);
			valready_there.at(i) = tv;
			pfc::string8 this_val = pfc::lineEndingsToWin(tag_values[i].get_pure_cvalue());
			if (!tv.equals(this_val)) {
				already_there &= false;
				break;
			}
		}
	}

	bool Valid_Meta_Touched = false;
	size_t writes = 0;
	size_t i = 0;

	while (i < (bforce_flat ? 1 : cvalues)) {
		if (bforce_flat || tag_values[i].get_pure_cvalue().get_length()) {

			if (!already_there) {

				if (!Valid_Meta_Touched || tag_values.get_size() == 1) {

					if (cexist) {

						// PRE-WRITE DELETE
						 
						//log
						if (blog) {
							logmsg << logtrack << "TD < pre-write clear " << entry.tag_name;
							log_msg(logmsg); logmsg = "";
						}//..log

						info.meta_remove_field(entry.tag_name);
						cexist = false;
					}

					// SET

					const pfc::string8 setval = bforce_flat ? forced_flat : pfc::lineEndingsToWin(tag_values[i].get_pure_cvalue());

					//log
					if (blog) {
						pfc::string8 dbgflat(setval);
						logmsg << logtrack << "TW < sets " << entry.tag_name << " = " << (setval.get_length() ? setval : "(empty)");
						log_msg(logmsg); logmsg = "";
					}//..log

					meta = info.meta_set(entry.tag_name, setval);
					Valid_Meta_Touched = true;
					++writes;
				}
				else {

					// ADD

					Valid_Meta_Touched = true;

					//log
					if (blog) {
						pfc::string8 dbgflat = tag_values[i].get_pure_cvalue();
						logmsg << logtrack << "TW < adds " << entry.tag_name << " + " << (dbgflat.get_length() ? dbgflat : "(empty)");
						log_msg(logmsg); logmsg = "";
					}//..log

					info.meta_add_value(meta, pfc::lineEndingsToWin(tag_values[i].get_pure_cvalue()));
					++writes;
				}
			}
			else {
				//log
				if (i == 0 && blog) {
					logmsg << logtrack << "TD < skipping " << entry.tag_name << " (value already exits)";
					log_msg(logmsg); logmsg = "";
				}//..log
			}
		}
		i++;
	}
	if (cexist && !writes && !already_there) {

		//log
		if (blog) {
			logmsg << logtrack << "TD < Removing field " << entry.tag_name;
			log_msg(logmsg); logmsg = "";
		}//..log

		info.meta_remove_field(entry.tag_name);
	}
}
