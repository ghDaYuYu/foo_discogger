#pragma once
#include <atomic>
#include "foo_discogs.h"
#include "file_info_manager.h"
#include "error_manager.h"

#define MATCH_NA       -1
#define MATCH_SUCCESS   0
#define MATCH_FAIL      1
#define MATCH_ASSUME    2

struct track_mapping
{
	bool enabled;
	int discogs_disc;
	int discogs_track;
	t_size file_index;

public:

	track_mapping() : enabled(false), discogs_disc(0), discogs_track(0), file_index(0) {};

	bool operator!=(track_mapping &other) {
		return !(other.discogs_disc == discogs_disc &&
			other.discogs_track == discogs_track &&
			other.enabled == enabled &&
			other.file_index == file_index);
	}
};

typedef pfc::array_t<track_mapping> track_mappings_list_type;

class tag_result {

public:

	const tag_mapping_entry *tag_entry;
	pfc::array_t<string_encoded_array> value;
	pfc::array_t<string_encoded_array> old_value;
	pfc::array_t<bool> r_approved;
	bit_array_bittable r_usr_modded;
	bit_array_bittable r_usr_approved;

	bool changed = false;
	bool result_approved = false;
};


typedef std::shared_ptr<tag_result> tag_result_ptr;
typedef pfc::array_t<tag_result_ptr> tag_results_list_type;

class TagWriter : public ErrorManager {

public:

	TagWriter(file_info_manager_ptr finfo_manager, Release_ptr release);

	void write_tags();
	void match_tracks();
	void generate_tags(tag_mapping_list_type* alt_mappings, threaded_process_status& p_status, abort_callback& p_abort);

	Release_ptr GetRelease();
	const t_size GetArtCount(art_src artsrc = art_src::unknown);
	const pfc::array_t<ReleaseArtist_ptr> GetArtists();
	bool Staging_Results();
	void ResetMask();
	bool ResultsReady() { return atm_tag_results_ready; }

private:

	void write_tag(metadb_handle_ptr item, file_info &info, const tag_mapping_entry &entry, const pfc::string8 &tag_value);
	void write_tag(metadb_handle_ptr item, file_info &info, const tag_mapping_entry &entry, const pfc::array_t<string_encoded_array> &tag_values);

	const t_size get_artists_art_count();

	int route_discogs_track_order(track_mappings_list_type &mappings);
	int order_tracks_by_duration(track_mappings_list_type &mappings);
	int order_tracks_by_number(track_mappings_list_type &mappings);
	int order_tracks_by_assumption(track_mappings_list_type &mappings);

public:



	tag_results_list_type tag_results;
	file_info_manager_ptr m_finfo_manager;
	track_mappings_list_type m_track_mappings;

	//selections
	pfc::bit_array_bittable tag_results_mask;
	bool tag_results_mask_force_wu = false;
	PreView tag_results_mask_mode = PreView::Undef;

	int m_match_status = -1;

private:

	Release_ptr release;

	service_ptr_t<titleformat_object> m_track_script;
	const static std::vector<int(TagWriter::*)(track_mappings_list_type &)> m_matching_functions;

	std::atomic<bool> atm_tag_results_ready = false;
};

typedef std::shared_ptr<TagWriter> TagWriter_ptr;
