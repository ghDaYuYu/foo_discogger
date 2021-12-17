#pragma once

#include "foo_discogs.h"
#include "file_info_manager.h"
#include "error_manager.h"

#define MATCH_NA		-1
#define MATCH_SUCCESS	0
#define MATCH_FAIL		1
#define MATCH_ASSUME	2

struct track_mapping
{
	bool enabled;
	int discogs_disc;
	int discogs_track;
	t_size file_index;

public:
	bool operator!=(track_mapping &other) {
		return !(other.discogs_disc == discogs_disc && other.discogs_track == discogs_track && other.enabled == enabled && other.file_index == file_index);
	}
};

typedef pfc::array_t<track_mapping> track_mappings_list_type;

class tag_result
{
public:
	const tag_mapping_entry *tag_entry;
	pfc::array_t<string_encoded_array> value;
	pfc::array_t<string_encoded_array> old_value;
	pfc::array_t<bool> r_approved;

	bool changed = false;
	bool result_approved = false;
};

typedef std::shared_ptr<tag_result> tag_result_ptr;
typedef pfc::array_t<tag_result_ptr> tag_results_list_type;

class TagWriter : public ErrorManager
{
public:
	file_info_manager_ptr finfo_manager;

	track_mappings_list_type track_mappings;
	int match_status = -1;
	
	/* Tag results are per [enabled] tag. They contain an array of result per track_mapping. */
	tag_results_list_type tag_results;

	Release_ptr release;

	TagWriter(file_info_manager_ptr finfo_manager, Release_ptr release);
	TagWriter(file_info_manager_ptr finfo_manager, pfc::string8 p_error) : finfo_manager(finfo_manager), release(nullptr), error(p_error) {
		skip = true;
		force_skip = true;
	}

	void generate_tags(bool use_update_tags, tag_mapping_list_type* alt_mappings, threaded_process_status& p_status, abort_callback& p_abort);
	const t_size get_art_count() { return release->images.get_count() + release->artists[0]->full_artist->images.get_count();}

	void write_tags();
	void write_tags_track_map();
	void write_tags_v23();

	void match_tracks();

	bool will_modify = false;
	bool skip = false;
	bool force_skip = false;

	pfc::string8 error = "";

private:

	void write_tag(metadb_handle_ptr item, file_info &info, const tag_mapping_entry &entry, const pfc::string8 &tag_value);
	void write_tag(metadb_handle_ptr item, file_info &info, const tag_mapping_entry &entry, const pfc::array_t<string_encoded_array> &tag_values);

	int compute_discogs_track_order(track_mappings_list_type &mappings);
	int order_tracks_by_duration(track_mappings_list_type &mappings);
	int order_tracks_by_number(track_mappings_list_type &mappings);
	int order_tracks_by_assumption(track_mappings_list_type &mappings);

	const static std::vector<int(TagWriter::*)(track_mappings_list_type &)> matching_functions;
};
typedef std::shared_ptr<TagWriter> TagWriter_ptr;
