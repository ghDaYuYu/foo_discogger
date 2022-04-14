#include "stdafx.h"

#include "jansson/jansson.h"
#include "foo_discogs.h"
#include "utils.h"
#include "json_helpers.h"
#include "tag_mapping_credit_utils.h"

#include "discogs.h"

using namespace Discogs;

const ExposedMap<ReleaseLabel> ExposedTags<ReleaseLabel>::exposed_tags = ReleaseLabel::create_tags_map();
const ExposedMap<ReleaseCompany> ExposedTags<ReleaseCompany>::exposed_tags = ReleaseCompany::create_tags_map();
const ExposedMap<ReleaseSeries> ExposedTags<ReleaseSeries>::exposed_tags = ReleaseSeries::create_tags_map();
const ExposedMap<ReleaseTrack> ExposedTags<ReleaseTrack>::exposed_tags = ReleaseTrack::create_tags_map();
const ExposedMap<Release> ExposedTags<Release>::exposed_tags = Release::create_tags_map();
const ExposedMap<MasterRelease> ExposedTags<MasterRelease>::exposed_tags = MasterRelease::create_tags_map();
const ExposedMap<Artist> ExposedTags<Artist>::exposed_tags = Artist::create_tags_map();
const ExposedMap<ReleaseArtist> ExposedTags<ReleaseArtist>::exposed_tags = ReleaseArtist::create_tags_map();
const ExposedMap<ReleaseCredit> ExposedTags<ReleaseCredit>::exposed_tags = ReleaseCredit::create_tags_map();
#ifdef CAT_CRED
const ExposedMap<ReleaseCatCredit> ExposedTags<ReleaseCatCredit>::exposed_tags = ReleaseCatCredit::create_tags_map();
#endif // CAT_CRED
const ExposedMap<ReleaseFormat> ExposedTags<ReleaseFormat>::exposed_tags = ReleaseFormat::create_tags_map();
const ExposedMap<ReleaseDisc> ExposedTags<ReleaseDisc>::exposed_tags = ReleaseDisc::create_tags_map();
const ExposedMap<ExpTagsImage> ExposedTags<ExpTagsImage>::exposed_tags = ExpTagsImage::create_tags_map();


string_encoded_array Discogs::ReleaseArtist::get_data(pfc::string8 &tag_name, threaded_process_status &p_status, abort_callback &p_abort) {
	try {
		return ExposedTags<ReleaseArtist>::get_data(tag_name, p_status, p_abort);
	}
	catch (missing_data_exception) {
		pfc::string8 xxx = full_artist->name;
		return full_artist->get_data(tag_name, p_status, p_abort);
	}
}


string_encoded_array Discogs::Artist::get_sub_data(pfc::string8 &tag_name, threaded_process_status &p_status, abort_callback &p_abort) {
	pfc::string8 sub_tag_name;
	string_encoded_array result;
	if (STR_EQUALN(tag_name, "IMAGES_", 7)) {
		result.force_array(); 
		sub_tag_name = substr(tag_name, 7);
		load(p_status, p_abort);
		for (size_t i = 0; i < images.get_size(); i++) {
			result.append_item_val(images[i]->get_data(sub_tag_name, p_status, p_abort));
		}
	}
	else if (STR_EQUALN(tag_name, "RELEASES_", 9)) {
		result.force_array();
		sub_tag_name = substr(tag_name, 9);
		load_releases(p_status, p_abort, false, nullptr);
		for (size_t i = 0; i < releases.get_size(); i++) {
			result.append_item_val(releases[i]->get_data(sub_tag_name, p_status, p_abort));
		}
	}
	else {
		return ExposedTags<Artist>::get_sub_data(tag_name, p_status, p_abort);
	}
	result.encode();
	return result;
}


string_encoded_array Discogs::ReleaseCredit::get_sub_data(pfc::string8 &tag_name, threaded_process_status &p_status, abort_callback &p_abort) {
	pfc::string8 sub_tag_name;
	string_encoded_array result;
	if (strncmp(tag_name.get_ptr(), "ARTISTS_", 8) == 0) {
		result.force_array(); 
		sub_tag_name = pfc::string8(tag_name.get_ptr() + 8);
		for (size_t i = 0; i < artists.get_size(); i++) {
			result.append_item_val(artists[i]->get_data(sub_tag_name, p_status, p_abort));
		}
	}
	else {
		return ExposedTags<ReleaseCredit>::get_sub_data(tag_name, p_status, p_abort);
	}
	result.encode();
	return result;
}

#ifdef CAT_CRED

string_encoded_array Discogs::ReleaseCatCredit::get_sub_data(pfc::string8& tag_name, threaded_process_status& p_status, abort_callback& p_abort) {
	pfc::string8 sub_tag_name;
	string_encoded_array result;
	if (strncmp(tag_name.get_ptr(), "ARTISTS_", 8) == 0) {
		result.force_array();
		sub_tag_name = pfc::string8(tag_name.get_ptr() + 8);
		for (size_t i = 0; i < artists.get_size(); i++) {
			result.append_item_val(artists[i]->get_data(sub_tag_name, p_status, p_abort));
		}
	}
	else {
		return ExposedTags<ReleaseCatCredit>::get_sub_data(tag_name, p_status, p_abort);
	}
	result.encode();
	return result;
}

#endif // CAT_CRED


string_encoded_array Discogs::ReleaseTrack::get_sub_data(pfc::string8 &tag_name, threaded_process_status &p_status, abort_callback &p_abort) {
	pfc::string8 sub_tag_name;
	string_encoded_array result;
	if (strncmp(tag_name.get_ptr(), "ARTISTS_", 8) == 0) {
		result.force_array();
		sub_tag_name = pfc::string8(tag_name.get_ptr() + 8);
		for (size_t i = 0; i < artists.get_size(); i++) {
			result.append_item_val(artists[i]->get_data(sub_tag_name, p_status, p_abort));
		}
	}
	else if (strncmp(tag_name.get_ptr(), "CREDITS_", 8) == 0) {
		result.force_array(); 
		sub_tag_name = pfc::string8(tag_name.get_ptr() + 8);
		for (size_t i = 0; i < credits.get_size(); i++) {
			result.append_item_val(credits[i]->get_data(sub_tag_name, p_status, p_abort));
		}
	}
	else if (strncmp(tag_name.get_ptr(), "HIDDEN_TRACKS_", 14) == 0) {
		result.force_array(); 
		sub_tag_name = pfc::string8(tag_name.get_ptr() + 14);
		for (size_t i = 0; i < hidden_tracks.get_size(); i++) {
			result.append_item_val(hidden_tracks[i]->get_data(sub_tag_name, p_status, p_abort));
		}
	}
	else {
		return ExposedTags<ReleaseTrack>::get_sub_data(tag_name, p_status, p_abort);
	}
	result.encode();
	return result;
}


string_encoded_array Discogs::ReleaseDisc::get_sub_data(pfc::string8 &tag_name, threaded_process_status &p_status, abort_callback &p_abort) {
	pfc::string8 sub_tag_name;
	string_encoded_array result;
	if (STR_EQUALN(tag_name, "TRACKS_", 7)) {
		result.force_array();
		sub_tag_name = substr(tag_name, 7);
		for (size_t i = 0; i < tracks.get_size(); i++) {
			result.append_item_val(tracks[i]->get_data(sub_tag_name, p_status, p_abort));
		}
	}
	else if (format != nullptr && STR_EQUALN(tag_name, "FORMAT_", 7)) {
		sub_tag_name = substr(tag_name, 7);
		return format->get_data(sub_tag_name, p_status, p_abort);
	}
	else {
		return ExposedTags<ReleaseDisc>::get_sub_data(tag_name, p_status, p_abort);
	}
	result.encode();
	return result;
}

string_encoded_array Discogs::Release::get_sub_data(pfc::string8 &tag_name, threaded_process_status &p_status, abort_callback &p_abort) {
	pfc::string8 sub_tag_name;
	string_encoded_array result;
	if (STR_EQUALN(tag_name, "TRACKS_", 7)) {
		result.force_array(); 
		load(p_status, p_abort);
		for (size_t i = 0; i < discs.get_size(); i++) {
			if (i == 0) {
				result = discs[i]->get_data(tag_name, p_status, p_abort);
			}
			else {
				result.shallow_extend(discs[i]->get_data(tag_name, p_status, p_abort));
			}
		}
	}
	else if (STR_EQUALN(tag_name, "DISCS_", 6)) {
		result.force_array(); 
		sub_tag_name = substr(tag_name, 6);
		load(p_status, p_abort); 
		for (size_t i = 0; i < discs.get_size(); i++) {
			result.append_item_val(discs[i]->get_data(sub_tag_name, p_status, p_abort));
		}
	}
	else if (STR_EQUALN(tag_name, "ARTISTS_", 8)) {
		result.force_array(); 
		sub_tag_name = substr(tag_name, 8);
		load(p_status, p_abort); 
		for (size_t i = 0; i < artists.get_size(); i++) {
			result.append_item_val(artists[i]->get_data(sub_tag_name, p_status, p_abort));
		}
	}
	else if (STR_EQUALN(tag_name, "LABELS_", 7)) {
		result.force_array(); 
		sub_tag_name = substr(tag_name, 7);
		load(p_status, p_abort); 
		for (size_t i = 0; i < labels.get_size(); i++) {
			result.append_item_val(labels[i]->get_data(sub_tag_name, p_status, p_abort));
		}
	}
	else if (STR_EQUALN(tag_name, "COMPANIES_", 10)) {
		result.force_array();
		sub_tag_name = substr(tag_name, 10);
		load(p_status, p_abort);
		for (size_t i = 0; i < companies.get_size(); i++) {
			result.append_item_val(companies[i]->get_data(sub_tag_name, p_status, p_abort));
		}
	}
	else if (STR_EQUALN(tag_name, "SERIES_", 7)) {
		result.force_array(); 
		sub_tag_name = substr(tag_name, 7);
		load(p_status, p_abort); 
		for (size_t i = 0; i < series.get_size(); i++) {
			result.append_item_val(series[i]->get_data(sub_tag_name, p_status, p_abort));
		}
	}
	else if (STR_EQUALN(tag_name, "FORMATS_", 8)) {
		result.force_array(); 
		sub_tag_name = substr(tag_name, 8);
		load(p_status, p_abort); 
		for (size_t i = 0; i < formats.get_size(); i++) {
			result.append_item_val(formats[i]->get_data(sub_tag_name, p_status, p_abort));
		}
	}
	else if (STR_EQUALN(tag_name, "CREDITS_", 8)) {
		result.force_array(); 
		sub_tag_name = substr(tag_name, 8);
		load(p_status, p_abort);
		for (size_t i = 0; i < credits.get_size(); i++) {
			result.append_item_val(credits[i]->get_data(sub_tag_name, p_status, p_abort));
		}
	}
	else if (STR_EQUALN(tag_name, "CAT_CREDITS_", 12)) {

		pfc::string8 init_tagname;
		init_tagname << "RELEASE_" << tag_name;
		credit_tag_nfo credit_nfo(init_tagname);
		
		pfc::string8 inno;
		credit_nfo.build_vwhere_cats(inno);

		size_t srcpos = grp_array_position(credit_nfo.get_src().get_ptr());

		if (!credit_nfo.get_checked() || srcpos == pfc_infinite) return result;
		
		result.force_array();
		load(p_status, p_abort);

		std::map<pfc::string8, vppair>* map_res = credit_nfo.isGXP() ? &catcredits_gxp : &catcredits_gxc;
		auto catcredits = map_res->find(init_tagname/*inno*/);

		if (catcredits != map_res->end()) {
			for (auto walkcat : catcredits->second) {

				bool bfiltermatch = true;
				if (srcpos != 0) { 
					if (walkcat.second.second.has_prefix("SUBVAL")) {
						if (srcpos == 1) {
							walkcat.second.second = "SUBVAL";
							walkcat.second.first = substr(walkcat.second.first, 0, walkcat.second.first.find_first(",", 0));
						}
						else
							if (srcpos == 2) {
								if (STR_EQUAL(walkcat.second.second, "SUBVAL")) {
									bfiltermatch = false; //reject 'only tracks'
								}
								else
								if (credit_nfo.isGXP()) {
									walkcat.second.second = substr(walkcat.second.second, pfc::string8("SUBVAL,").get_length(), walkcat.second.second.get_length() - pfc::string8("SUBVAL,").get_length());
									size_t commapos = walkcat.second.first.find_first(",", 0);
									walkcat.second.first = substr(walkcat.second.first, commapos, walkcat.second.first.get_length() - commapos - 1);
								}
							}
					}
					else {
						if (srcpos == 1) bfiltermatch = false; //reject 'only release'
					}
				}
				if (!bfiltermatch) { continue; }

				string_encoded_array se_array;
				pfc::string8 row(walkcat.first.second);
				if (credit_nfo.isGXP() && !STR_EQUAL(walkcat.second.second, "SUBVAL")) {
					walkcat.second.second.replace_string("SUBVAL,", "", 0);
					row << " on track " << disc_tracks_to_range(walkcat.second.second, this);
				}
				row << " - " << walkcat.second.first;
				se_array.append_item(row);
				result.append_item_val(se_array);
			}		
		}
	}
	else if (STR_EQUALN(tag_name, "IMAGES_", 7)) {
		result.force_array(); 
		sub_tag_name = substr(tag_name, 7);
		load(p_status, p_abort);
		for (size_t i = 0; i < images.get_size(); i++) {
			result.append_item_val(images[i]->get_data(sub_tag_name, p_status, p_abort));
		}
	}
	else if (STR_EQUALN(tag_name, "MASTER_RELEASE_", 15)) {
		sub_tag_name = substr(tag_name, 15);
		load_preview(p_status, p_abort);
		if (master_release) {
			result = master_release->get_data(sub_tag_name, p_status, p_abort);
		}
	}
	else {
		return ExposedTags<Release>::get_sub_data(tag_name, p_status, p_abort);
	}
	result.encode();
	return result;
}


string_encoded_array Discogs::MasterRelease::get_sub_data(pfc::string8 &tag_name, threaded_process_status &p_status, abort_callback &p_abort) {
	pfc::string8 sub_tag_name;
	string_encoded_array result;
	if (STR_EQUALN(tag_name, "TRACKS_", 7)) {
		result.force_array(); 
		load(p_status, p_abort);
		for (size_t i = 0; i < discs.get_size(); i++) {
			if (i == 0) {
				result = discs[i]->get_data(tag_name, p_status, p_abort);
			}
			else {
				result.shallow_extend(discs[i]->get_data(tag_name, p_status, p_abort));
			}
		}
	}
	else if (STR_EQUALN(tag_name, "DISCS_", 6)) {
		result.force_array(); 
		sub_tag_name = substr(tag_name, 6);
		load(p_status, p_abort); 
		for (size_t i = 0; i < discs.get_size(); i++) {
			result.append_item_val(discs[i]->get_data(sub_tag_name, p_status, p_abort));
		}
	}
	else if (STR_EQUALN(tag_name, "ARTISTS_", 8)) {
		result.force_array(); 
		sub_tag_name = substr(tag_name, 8);
		load(p_status, p_abort); 
		for (size_t i = 0; i < artists.get_size(); i++) {
			result.append_item_val(artists[i]->get_data(sub_tag_name, p_status, p_abort));
		}
	}
	else if (STR_EQUALN(tag_name, "IMAGES_", 7)) {
		result.force_array(); 
		sub_tag_name = substr(tag_name, 7);
		load(p_status, p_abort); 
		for (size_t i = 0; i < images.get_size(); i++) {
			result.append_item_val(images[i]->get_data(sub_tag_name, p_status, p_abort));
		}
	}
	else if (STR_EQUALN(tag_name, "MAIN_RELEASE_", 13)) {
		sub_tag_name = substr(tag_name, 13);
		load_preview(p_status, p_abort);
		if (main_release) {
			result = main_release->get_data(sub_tag_name, p_status, p_abort);
		}
	}
	else if (STR_EQUALN(tag_name, "RELEASES_", 9)) {
		result.force_array(); 
		sub_tag_name = substr(tag_name, 9);
		load_releases(p_status, p_abort);
		for (size_t i = 0; i < sub_releases.get_size(); i++) {
			result.append_item_val(sub_releases[i]->get_data(sub_tag_name, p_status, p_abort));
		}
	}
	else {
		return ExposedTags<MasterRelease>::get_sub_data(tag_name, p_status, p_abort);
	}
	result.encode();
	return result;
}


inline void Discogs::MasterRelease::set_main_release(Release_ptr main) {
	main_release = main;
	if (main) {
		main_release_id = main->id;
	}
}


inline void Discogs::Release::set_master_release(MasterRelease_ptr master) {
	master_release = master;
	if (master) {
		master_id = master->id;
	}
}


pfc::string8 Discogs::remove_number_suffix(const pfc::string8 &src) {
	pfc::string8 dst = src;
	unsigned src_size = src.get_length();
	if (src_size >= 5 && src[src_size - 1] == ')' && (src[src_size - 3] == '(' || src[src_size - 4] == '(')) {
		if (src[src_size - 3] == '(' && atoi(substr(dst, src_size - 2, 1).get_ptr()) != 0) {
			dst = substr(dst, 0, src_size - 4);
		}
		else if (atoi(substr(dst, src_size - 3, 2).get_ptr()) != 0) {
			dst = substr(dst, 0, src_size - 5);
		}
	}
	return dst;
}

pfc::string8 Discogs::move_the_to_end(const pfc::string8 &src) {
	if (src.get_length() < 5) {
		return src;
	}
	pfc::string8 tmp = src;
	pfc::string8 start = substr(tmp, 0, 4);
	if (STR_EQUAL(start, "The ") || STR_EQUAL(start, "the ")) {
		tmp = substr(tmp, 4, src.get_length());
		tmp << ", The";
	}
	return tmp;
}

pfc::string8 Discogs::move_the_to_start(const pfc::string8 &src) {
	if (src.get_length() < 5) {
		return src;
	}
	pfc::string8 tmp = src;
	pfc::string8 end = substr(tmp, src.get_length() - 5, 5);
	if (STR_EQUAL(end, ", The") || STR_EQUAL(end, ", the")) {
		pfc::string8 ret("The ");
		ret << substr(tmp, 0, src.length() - 5);
		return ret;
	}
	return src;
}

pfc::string8 Discogs::strip_artist_name(const pfc::string8 str) {
	pfc::string8 result = lowercase(str);
	result = rtrim(result);
	if (result.length() > 0) {
		if (result[result.length() - 1] == ')') {
			result = substr(result, 0, result.find_last('('));
			result = rtrim(result);
		}
		if (result.length() > 5 && STR_EQUAL(substr(result, result.length() - 5, 5), ", the")) {
			result = substr(result, 0, result.length() - 5);
			result = rtrim(result);
		}
		if (result.length() > 4 && STR_EQUAL(substr(result, result.length() - 4, 4), "the ")) {
			result = substr(result, 4);
			result = rtrim(result);
		}
	}
	return result;
}

pfc::string8 Discogs::format_track_number(int tracknumber) {
	char buf[4];
	sprintf_s(buf, "%d", tracknumber);
	return buf;
}

ReleaseArtist_ptr Discogs::parseReleaseArtist(json_t *element) {
	assert_is_object(element);
	pfc::string8 artist_id = JSONAttributeString(element, "id");
	auto full_artist = discogs_interface->get_artist(artist_id);
	ReleaseArtist_ptr artist(new ReleaseArtist(artist_id, full_artist));
	artist->loaded = true;
	pfc::string8 name = JSONAttributeString(element, "name");
	pfc::string8 anv = JSONAttributeString(element, "anv");
	artist->name = name;
	artist->anv = anv;
	artist->raw_roles = JSONAttributeString(element, "role");
	tokenize(artist->raw_roles, ",", artist->full_roles, true);
	for (size_t i = 0; i < artist->full_roles.get_size(); i++) {
		size_t pos = artist->full_roles[i].find_first('[');
		if (pos != pfc::infinite_size) {
			artist->roles.append_single(trim(substr(artist->full_roles[i], 0, pos)));
		}
		else {
			artist->roles.append_single(artist->full_roles[i]);
		}
	}
	artist->join = JSONAttributeString(element, "join");
	return std::move(artist);
}

void Discogs::parseReleaseArtists(json_t *element, pfc::array_t<ReleaseArtist_ptr> &artists) {
	assert_is_array(element);
	for (size_t i = 0; i < json_array_size(element); i++) {
		json_t *artist = json_array_get(element, i);
		artists.append_single(parseReleaseArtist(artist));
	}
}

void Discogs::parseReleaseCredits(json_t* element, pfc::array_t<ReleaseCredit_ptr> &credits, Release *release) {
	assert_is_array(element);
	pfc::string8 last_role;
	for (size_t i = 0; i < json_array_size(element); i++) {
		json_t *j = json_array_get(element, i);
		ReleaseArtist_ptr artist = parseReleaseArtist(j);
		const pfc::string8 &tracks = JSONAttributeString(j, "tracks");

		if (tracks.get_length()) {

			ReleaseCredit_ptr credit(new ReleaseCredit());
			credit->raw_roles = artist->raw_roles;
			credit->roles = artist->roles;
			credit->full_roles = artist->full_roles;
			credit->artists.append_single(artist);

			try {
				addReleaseTrackCredits(sanitize_track_commas(tracks), credit, release);
			}
			catch (parser_exception& e) {
				pfc::string8 error;
				error << ("(skipped) ") << e.what() << "\n";
				popup_message::g_show(error.get_ptr(), "Error(s)", popup_message::icon_error);
			}
		}
		else {

			if (!artist->raw_roles.get_length() || (credits.get_size() && STR_EQUAL(artist->raw_roles, credits[credits.get_size() - 1]->raw_roles))) {
				credits[credits.get_size() - 1]->artists.append_single(artist);
			}
			else {

				ReleaseCredit_ptr credit(new ReleaseCredit());
				credit->raw_roles = artist->raw_roles;
				credit->roles = artist->roles;
				credit->full_roles = artist->full_roles;
				credit->artists.append_single(artist);
				credits.append_single(credit);
			}
		}
	}
}

//todo: cleanup/fix alphanumeric notation
unsigned long long encode_track_position(const pfc::string8 &pos) {
	// first 1-8 characters encoded
	unsigned long long result = 0;
	unsigned char temp = 0;
	bool use_temp = false;
	size_t count = 0;
	const size_t length = pos.get_length();
	for (size_t i = 0; count < 8 && i < length; i++) {
		if (pos[i] >= 48 && pos[i] <= 57) {
			temp = temp * 10 + (pos[i] - 48);
			use_temp = true;
		}
		else {
			if (use_temp) {
				result |= temp;
				use_temp = false;
				temp = 0;
				count++;
				if (count < 7) {
					result <<= 8;
				}
			}
			result |= pos[i];
			count++;
			if (count < 7) {
				result <<= 8;
			}
		}
	}
	if (use_temp) {
		result |= temp;
		count++;
		if (count < 7) {
			result <<= 8;
		}
	}
	if (count < 7) {
		result <<= (count * 8);
	}
	return result;
}

//todo: fix/clean up not working for alphanumeric notations
bool track_in_range(const pfc::string8 &pos, const pfc::string8 &start, const pfc::string8 &end) {
	unsigned long long dpos = encode_track_position(pos);
	unsigned long long dstart = encode_track_position(start);
	unsigned long long dend = encode_track_position(end);
	return dpos >= dstart && dpos <= dend;
}

pfc::array_t<pfc::string8> blah() {
	pfc::array_t<pfc::string8> v;
	v.append_single(pfc::string8(","));
	v.append_single(pfc::string8(";"));
	v.append_single(pfc::string8(" and "));
	return v;
}
const pfc::array_t<pfc::string8> CREDIT_DELIMS = blah();

void Discogs::addReleaseTrackCredits(const pfc::string8 &tracks, ReleaseCredit_ptr &credit, Release *release) {
	pfc::array_t<pfc::string8> parts;
	size_t count = tokenize_multi(tracks, CREDIT_DELIMS, parts, true);
	for (size_t j = 0; j < count; j++) {
		pfc::array_t<pfc::string8> inner_parts;
		size_t inner_count = tokenize(parts[j], " to ", inner_parts, true);

		if (inner_count < 2) {
			int tmp_disc, tmp_track;
			if (release->find_std_disc_track(inner_parts[0], &tmp_disc, &tmp_track)) {
				if (tmp_disc < release->discs.get_size() && tmp_track < release->discs[tmp_disc]->tracks.get_size())
					release->discs[tmp_disc]->tracks[tmp_track]->credits.append_single(credit);
			}
		}
		else if (inner_count == 2) {

			std::vector<std::pair<size_t, size_t>> inner_std_parts;
			int tmp_disc, tmp_track;
			if (release->find_std_disc_track(inner_parts[0], &tmp_disc, &tmp_track))
				inner_std_parts.emplace_back(tmp_disc, tmp_track);
			if (release->find_std_disc_track(inner_parts[1], &tmp_disc, &tmp_track))
				inner_std_parts.emplace_back(tmp_disc, tmp_track);

			if (inner_std_parts.size() == 2) {
				
				pfc::string8 range_botton = PFC_string_formatter() << inner_std_parts[0].first << "." << inner_std_parts[0].second;
				pfc::string8 range_top = PFC_string_formatter() << inner_std_parts[1].first << "." << inner_std_parts[1].second;

				//check bounds
				if (inner_std_parts[0].first < release->discs.get_size() &&
					inner_std_parts[1].first < release->discs.get_size() &&
					inner_std_parts[0].second < release->discs[inner_std_parts[0].first]->tracks.get_size() &&
					inner_std_parts[1].second < release->discs[inner_std_parts[1].first]->tracks.get_size()) {

					for (size_t d = inner_std_parts[0].first; d <= inner_std_parts[1].first; d++) {
						for (size_t t = 0; t < release->discs[d]->tracks.get_size(); t++) {
							pfc::string8 pos = release->discs[d]->tracks[t]->discogs_track_number;
							if (release->find_std_disc_track(pos, &tmp_disc, &tmp_track)) {
								pos = PFC_string_formatter() << tmp_disc << "." << tmp_track;
								//todo: fix/cleanup to support release track notation
								if (track_in_range(pos, range_botton, range_top)) {
									release->discs[d]->tracks[t]->credits.append_single(credit);
								}
							}
						}
					}
				}
			}
		}
		else {
			parser_exception ex;
			ex << "Error parsing release credits.";
			throw ex;
		}
	}
}

ReleaseLabel_ptr Discogs::parseReleaseLabel(json_t *element) {
	ReleaseLabel_ptr label(new ReleaseLabel());
	label->name = JSONAttributeString(element, "name");
	label->id = JSONAttributeString(element, "id");
	label->catalog = JSONAttributeString(element, "catno");
	return std::move(label);
}

void Discogs::parseReleaseLabels(json_t *element, pfc::array_t<ReleaseLabel_ptr> &release_labels) {
	if (json_is_array(element)) {
		for (size_t i = 0; i < json_array_size(element); i++) {
			release_labels.append_single(parseReleaseLabel(json_array_get(element, i)));
		}
	}
}

ReleaseCompany_ptr Discogs::parseReleaseCompany(json_t *element) {
	ReleaseCompany_ptr company(new ReleaseCompany());
	company->name = JSONAttributeString(element, "name");
	company->id = JSONAttributeString(element, "id");
	company->catalog = JSONAttributeString(element, "catno");
	company->entity_type_name = JSONAttributeString(element, "entity_type_name");
	return std::move(company);
}

void Discogs::parseReleaseCompanies(json_t *element, pfc::array_t<ReleaseCompany_ptr> &release_companies) {
	if (json_is_array(element)) {
		for (size_t i = 0; i < json_array_size(element); i++) {
			release_companies.append_single(parseReleaseCompany(json_array_get(element, i)));
		}
	}
}

ReleaseSeries_ptr Discogs::parseReleaseSeries(json_t *element) {
	ReleaseSeries_ptr series(new ReleaseSeries());
	series->name = JSONAttributeString(element, "name");
	series->id = JSONAttributeString(element, "id");
	series->api_url = JSONAttributeString(element, "resource_url");
	series->number = JSONAttributeString(element, "catno");
	return std::move(series);
}

void Discogs::parseReleaseSeries(json_t *element, pfc::array_t<ReleaseSeries_ptr> &release_series) {
	if (json_is_array(element)) {
		for (size_t i = 0; i < json_array_size(element); i++) {
			release_series.append_single(parseReleaseSeries(json_array_get(element, i)));
		}
	}
}

Image_ptr Discogs::parseImage(json_t *element) {
	Image_ptr image(new ExpTagsImage());

	//auto key = json_object_iter_key(json_object_iter(element));
	//void* iter = json_object_iter((json_t*)element);

	// Should we use thumb instead of url150?
	image->type = JSONAttributeString(element, "type");
	image->url = JSONAttributeString(element, "resource_url");
	image->url150 = JSONAttributeString(element, "uri150");
	image->width = JSONAttributeString(element, "width");
	image->height = JSONAttributeString(element, "height");
	return std::move(image);
}

void Discogs::parseImages(json_t *array, pfc::array_t<Image_ptr> &images) {
	pfc::array_t<Image_ptr> temp;
	if (json_is_array(array)) {
		for (size_t i = 0; i < json_array_size(array); i++) {
			json_t *img = json_array_get(array, i);
			if (json_is_object(img)) {
				Image_ptr image = parseImage(img);
				if (STR_EQUAL(image->type, "primary")) {
					images.append_single(std::move(image));
				}
				else {
					temp.append_single(std::move(image));
				}
			}
		}
	}
	images.append(temp);
}

ReleaseFormat_ptr Discogs::parseReleaseFormat(json_t *element) {
	ReleaseFormat_ptr format(new ReleaseFormat());
	format->name = JSONAttributeString(element, "name");
	format->qty = JSONAttributeString(element, "qty");
	format->descriptions = JSONAttributeStringArray(element, "descriptions");
	format->text = JSONAttributeString(element, "text");
	return format;
}

void Discogs::parseReleaseFormats(json_t *element, pfc::array_t<ReleaseFormat_ptr> &formats) {
	if (json_is_array(element)) {
		for (size_t i = 0; i < json_array_size(element); i++) {
			json_t *format = json_array_get(element, i);
			formats.append_single(parseReleaseFormat(format));
		}
	}
}

bool remove_chars(char c) {
	return !((c >= '0' && c <= '9') || c == '-');
}

void detect_hidden(pfc::string8 &s, pfc::string8 &pre, pfc::string8 &hidden) {
	// after first numeric, immediate switch to non-numeric, then ends
	// after period, immediate switch to number, then ends
	int len = s.length();
	int pos = pfc::infinite_size;
	int dot = s.find_last(".");

	// eg. 1.a, C2.a, B.1
	if (dot != pfc::infinite_size) {
		bool numeric = false;
		bool alpha = false;
		while (pos < len) {
			pos++;
			if (!numeric && !alpha) {
				if (s[pos] > 0 && s[pos] < 256 && isdigit(s[pos])) {
					numeric = true;
				}
				else if (s[pos] > 0 && s[pos] < 256 && isalpha(s[pos])) {
					alpha = true;
				}
			}
			else {
				if (s[pos] < 0 || s[pos] > 255 || ((numeric && !isdigit(s[pos])) || (alpha && !isalpha(s[pos])))) {
					break;
				}
			}
		}
	}
	// eg. 6a
	else if (s[len - 2] > 0 && s[len - 2] < 256 && isdigit(s[len - 2]) && s[len - 1] > 0 && s[len - 1] < 256 && isalpha(s[len - 1])) {
		pos = len - 1;
	}

	if (pos != pfc::infinite_size && pos < len) {
		hidden = substr(s, pos);
		pre = substr(s, 0, pos);
	}
}

void parseTrackPosition(ReleaseTrack_ptr &track, pfc::string8 &pre, pfc::string8 &post, pfc::string8 &hidden, bool &alpha, int &range,
		bool multi_format, int next_track_number, bool fix_dots) {
	// pre = before the - or before the number
	// post = numeric portion after the - or the last numeric portion or ???
	pfc::string8 position = track->discogs_track_number;
	if (fix_dots) {
		size_t pos = position.find_first('.');
		position.set_char(pos, '-');
	}
	range = 0;
	int dash = position.find_last("-");
	//if (multi_format && dash == pfc::infinite_size) {
	//	dash = position.find_last(".");
	//}
	if (multi_format && dash != pfc::infinite_size) {
		alpha = false;
		pre = substr(position, 0, dash);
		post = substr(position, dash + 1);
	}
	else if (dash != pfc::infinite_size && pfc::string_is_numeric(substr(position, 0, dash)) &&
		pfc::string_is_numeric(substr(position, dash + 1)) && next_track_number == std::stoi(substr(position, 0, dash).get_ptr())) {
		alpha = false;
		pre = substr(position, 0, dash);
		post = substr(position, dash + 1);
		range = std::stoi(substr(position, dash + 1).get_ptr()) - std::stoi(substr(position, 0, dash).get_ptr());
	}
	else {
		alpha = (position.get_length() != 0 && (position[0] < -1 || position[0] > 255 || isalpha(position[0]))); // unicode...
		pre = "";
		post = position;
	}
	hidden = "";
	if (!CONF.parse_hidden_as_regular) {
		detect_hidden(post, post, hidden);
	}
}

void parseTrackPositions(pfc::array_t<ReleaseTrack_ptr> &intermediate_tracks, HasTracklist *release, bool fix_dots = false) {
	// alpha -> number = new CD (after Vinyl)
	// number -> alpha = new Vinyl (after CD)
	// position_pre changes after a group = new item

	pfc::string8 pre, last_pre = "";
	pfc::string8 post, last_post = "";
	pfc::string8 hidden = "";
	bool alpha, last_alpha = false;
	bool last_hidden = false;
	int range;

	unsigned int disc_number = 1;
	int track_number = 1;
	int disc_track_number = 1;
	unsigned int format_quantity = release->discogs_total_discs;
	bool multi_format = format_quantity != 1;

	ReleaseDisc_ptr disc(new ReleaseDisc());
	disc->disc_number = disc_number;

	for (size_t i = 0; i < intermediate_tracks.get_size(); i++) {
		ReleaseTrack_ptr &track = intermediate_tracks[i];
		parseTrackPosition(track, pre, post, hidden, alpha, range, multi_format, track_number, fix_dots);
		if (track_number != 1 && ((last_pre != pre && (disc_number < format_quantity || last_alpha == alpha)) || (multi_format && disc_number < format_quantity && last_alpha != alpha))) {
			if (range <= 0) {
				release->discs.append_single(disc);
				disc = std::make_shared<ReleaseDisc>();
				disc->disc_number = ++disc_number;
				disc_track_number = 1;
			}
		}
		track->track_number = track_number;
		track->disc_track_number = disc_track_number;

		if (range > 0) {
			for (int j = 0; j < range; j++) {
				disc->tracks.append_single(ReleaseTrack_ptr(track->clone()));
				//release->tracks.append_single(ReleaseTrack_ptr(track->clone()));
				track_number++;
				disc_track_number++;
				track->track_number = track_number;
				track->disc_track_number = disc_track_number;
			}
			pre = "";
		}

		if (hidden.get_length() == 0 || !last_hidden || last_post != post || 
				disc->tracks.get_size() == 0) { // only use first partial track ie. 10a, 10b
			disc->tracks.append_single(std::move(track));
			track_number++;
			disc_track_number++;
		}
		else {
			disc->tracks[disc->tracks.get_size() - 1]->discogs_hidden_duration_seconds += track->discogs_duration_seconds;
			if (!STR_EQUAL(track->discogs_track_number, "(silence)")) {
				disc->tracks[disc->tracks.get_size() - 1]->hidden_tracks.append_single(std::move(track));
			}
		}

		if (range <= 0) {  // not sure what this is for
			last_pre = pre;
		}
		last_post = post;
		last_alpha = alpha;
		last_hidden = hidden.get_length() != 0;
	}
	if (disc->tracks.get_size()) {
		release->discs.append_single(disc);
	}

	// renumber if we didn't get them all and number of headings matches number of formats
	if (disc_number != format_quantity && format_quantity) {
		if (release->total_headings == format_quantity && !STR_EQUAL(release->discs[0]->tracks[0]->title_heading, "")) {
			pfc::string8 last_heading = "";
			track_number = 1;
			disc_track_number = 0;
			disc_number = 1;
			pfc::array_t<ReleaseTrack_ptr> all_tracks;

			size_t disc_n;
			for (disc_n = 0; disc_n < release->discs.get_size(); disc_n++) {
				for (size_t j = 0; j < release->discs[disc_n]->tracks.get_size(); j++) {
					all_tracks.append_single(release->discs[disc_n]->tracks[j]);
				}
				release->discs[disc_n]->tracks.force_reset();
			}
			release->discs.set_size(format_quantity);
			for (; (int)disc_n < format_quantity; disc_n++) {
				release->discs[disc_n] = std::make_shared<ReleaseDisc>();
				release->discs[disc_n]->disc_number = disc_n + 1;
			}

			for (size_t i = 0; i < all_tracks.get_size(); i++) {
				ReleaseTrack_ptr &track = all_tracks[i];
				track_number++;
				if (disc_track_number == 0) {
					last_heading = track->title_heading;
				}
				if (last_heading != track->title_heading) {
					disc_number++;
					if (disc_number > release->discs.get_size()) {
						throw foo_discogs_exception("Error parsing discs/tracks");
					}
					disc_track_number = 0;
					last_heading = track->title_heading;
				}
				disc_track_number++;
				track->track_number = track_number;
				track->disc_track_number = disc_track_number;
				release->discs[disc_number - 1]->tracks.append_single(track);
				//track->disc_number = disc_number;
				//for (size_t h = 0; h < track->hidden_tracks.get_size(); h++) {
				//	ReleaseTrack_ptr &hidden = track->hidden_tracks[i];
				//	hidden->track_number = track_number;
				//	hidden->disc_track_number = disc_track_number;
					//hidden->disc_number = disc_number;
				//}
			}
		}
		/*else {
			parser_exception ex;
			ex << "Unable to parse Discogs tracklist.";
			throw ex;
		}*/
	}

	// use index track duration for single tracks
	for (size_t i = 0; i < disc->tracks.get_size(); i++) {
		auto track = disc->tracks[i];
		if (!track->discogs_duration_seconds && track->discogs_indextrack_duration_seconds) {
			if (i + 1 > disc->tracks.get_size() - 1 || disc->tracks[i + 1]->discogs_indextrack_duration_seconds != track->discogs_indextrack_duration_seconds) {
				track->discogs_duration_seconds = track->discogs_indextrack_duration_seconds;
				track->discogs_duration_raw = track->discogs_indextrack_duration_raw;
			}
		}
	}
}

void Discogs::parseReleaseTrack(json_t *element, pfc::array_t<ReleaseTrack_ptr> &tracks, unsigned &discogs_original_track_number, pfc::string8 heading, ReleaseTrack_ptr *index, HasArtists *has_artists) {
	assert_is_object(element);

	ReleaseTrack_ptr track(new ReleaseTrack());

	track->title_heading = heading;

	track->title = JSONAttributeString(element, "title");
	pfc::string8 duration = JSONAttributeString(element, "duration");

	if (index != nullptr) {
		track->title_index = (*index)->title;
		track->title_subtrack = track->title;
		pfc::string8 temp = (*index)->title;
		temp << " (" << track->title << ")";
		track->title = temp;

		track->credits = (*index)->credits;

		track->discogs_indextrack_duration_raw = (*index)->discogs_duration_raw;
		track->discogs_indextrack_duration_seconds = (*index)->discogs_duration_seconds;
	}
	else {
		track->title_index = "";
		track->title_subtrack = "";
	}

	track->discogs_track_number = JSONAttributeString(element, "position");

	if (CONF.skip_video_tracks) {
		pfc::string8 lower_position = lowercase(track->discogs_track_number);
		if (lower_position.find_first("video") != pfc::infinite_size || lower_position.find_first("dvd") != pfc::infinite_size) {
			return;
		}
	}

	json_t *artists = json_object_get(element, "artists");
	if (artists) {
		parseReleaseArtists(artists, track->artists);
	}
	else if (has_artists) {
		for (size_t i = 0; i < has_artists->artists.get_count(); i++) {
			track->artists.append_single_val(has_artists->artists[i]);
		}
	}

	json_t *extraartists = json_object_get(element, "extraartists");
	if (extraartists) {
			parseReleaseCredits(extraartists, track->credits, nullptr);
	}

	int duration_seconds;
	if (duration.get_length() != 0) {
		size_t pos = min(duration.find_first(':'), duration.find_first('.'));
		size_t pos2 = min(duration.find_first(':', pos + 1), duration.find_first('.', pos + 1));
		if (pos2 != pfc::infinite_size) {
			pfc::string8 hour = substr(duration, 0, pos);
			pfc::string8 min = substr(duration, pos + 1, pos2);
			pfc::string8 sec = substr(duration, pos2 + 1);
			duration_seconds = (3600 * atoi(hour.get_ptr()) + 60 * atoi(min.get_ptr()) + atoi(sec.get_ptr()));
		}
		else if (pos != pfc::infinite_size) {
			pfc::string8 min = substr(duration, 0, pos);
			pfc::string8 sec = substr(duration, pos + 1);
			duration_seconds = (60 * atoi(min.get_ptr()) + atoi(sec.get_ptr()));
		}
		else {
			duration_seconds = 0;
			pfc::string8 msg("Error parsing track duration: ");
			msg << duration;
			log_msg(msg);
		}
		track->discogs_duration_raw = duration;
		track->discogs_duration_seconds = duration_seconds;
	}

	pfc::string8 type = JSONAttributeString(element, "type_");
	if (STR_EQUAL(type, "track")) {
		if (track->discogs_track_number.get_length() != 0 || track->title.get_length() != 0) {
			tracks.append_single(move(track));
		}
	}
	else if (STR_EQUAL(type, "index")) {
		json_t *sub_tracks = json_object_get(element, "sub_tracks");
		if (json_is_array(sub_tracks)) {
			for (size_t i = 0; i < json_array_size(sub_tracks); i++) {
				json_t *sub_track = json_array_get(sub_tracks, i);
				//recursion
				parseReleaseTrack(sub_track, tracks, discogs_original_track_number, heading, &track, has_artists);
			}
		}
	}
}

void Discogs::parseReleaseTracks(json_t *element, HasTracklist *has_tracklist, HasArtists *has_artists) {
	assert_is_array(element);

	pfc::array_t<ReleaseTrack_ptr> intermediate_tracks;

	pfc::string8 heading = "";
	has_tracklist->total_headings = 0;
	unsigned discogs_original_track_count = 0;

	for (size_t i = 0; i < json_array_size(element); i++) {
		json_t *tr = json_array_get(element, i);
		pfc::string8 type = JSONAttributeString(tr, "type_");
		if (STR_EQUAL(type, "heading")) {
			heading = JSONAttributeString(tr, "title");
			has_tracklist->total_headings++;
		}
		else {
			parseReleaseTrack(tr, intermediate_tracks, discogs_original_track_count, heading, nullptr, has_artists);
		}
	}
	has_tracklist->discogs_tracklist_count = discogs_original_track_count;
	has_tracklist->discs.force_reset();
	bool fix_dots = false;
	/*for (size_t i = 0; i < intermediate_tracks.get_count(); i++) {
		if (intermediate_tracks[i]->discogs_track_number.find_first('.') == pfc::infinite_size) {
			fix_dots = false;
			break;
		}
	}*/
	parseTrackPositions(intermediate_tracks, has_tracklist, fix_dots);
}

pfc::string8 getYearFromReleased(pfc::string8 released) {
	int dash = released.find_first("-");
	if (dash != pfc::infinite_size) {
		released = substr(released, 0, dash);
	}
	if (released.get_length() == 4) {
		return released;
	}
	return "";
}

pfc::string8 remove_dot_2spaces(pfc::string8 in) {
	pfc::string8_fastalloc buffer;
	uReplaceString(buffer, in, pfc_infinite, ".  ", 3, ". ", 2, false);
	return buffer;
}

void Discogs::parseRelease(Release *release, json_t *root) {
	assert_is_object(root);

	bool db_isready = DBFlags(CONF.db_dc_flag).IsReady();
	bool db_want_artwork = DBFlags(CONF.db_dc_flag).WantArtwork();

	release->id = JSONAttributeString(root, "id");
	MasterRelease_ptr master = discogs_interface->get_master_release(JSONAttributeString(root, "master_id"));
	release->set_master_release(master);
	release->title = JSONAttributeString(root, "title");
	release->country = JSONAttributeString(root, "country");
	release->genres = JSONAttributeStringArray(root, "genres");
	release->styles = JSONAttributeStringArray(root, "styles");

	release->barcode = JSONAttributeObjectArrayAttributeStringWhere(root, "identifiers", "value", "type", "Barcode");
	
	pfc::array_t<pfc::string8> tokens;
	release->release_date_raw = JSONAttributeString(root, "released");
	int num_tokens = tokenize(release->release_date_raw, "-", tokens, false);

	release->release_date = "";
	try {
		if (num_tokens) {
			if (num_tokens == 1) {
				release->release_year = tokens[0];
			}
			else if (num_tokens == 3) {
				release->release_year = tokens[0];
				if (!STR_EQUAL(tokens[1], "00")) {
					release->release_month = tokens[1];
					release->release_date << MONTH_NAMES.at(tokens[1].get_ptr()) << " ";
				}
				if (!STR_EQUAL(tokens[2], "00")) {
					release->release_day = tokens[2];
					release->release_date << tokens[2] << " ";
				}
			}
			release->release_date << tokens[0];
		}
	}
	catch (std::out_of_range) {
		throw foo_discogs_exception("Error parsing release date.");
	}

	json_t *labels = json_object_get(root, "labels");
	parseReleaseLabels(labels, release->labels);

	json_t *formats = json_object_get(root, "formats");
	parseReleaseFormats(formats, release->formats);

	json_t *artists = json_object_get(root, "artists");
	parseReleaseArtists(artists, release->artists);

	if (!release->artists.get_size()) {
		parser_exception ex;
		ex << " Release has no artists!";
		throw ex;
	}

	json_t *tracklist = json_object_get(root, "tracklist");
	parseReleaseTracks(tracklist, release, release);

	json_t *extraartists = json_object_get(root, "extraartists");
	parseReleaseCredits(extraartists, release->credits, release);

	release->notes = remove_dot_2spaces(JSONAttributeString(root, "notes"));

	json_t* images = json_object_get(root, "images");
	parseImages(images, release->images);

	//todo: db support
	release->videos = JSONAttributeObjectArrayAttributeString(root, "videos", "uri");

	//todo: db support
	json_t* companies = json_object_get(root, "companies");
	parseReleaseCompanies(companies, release->companies);

	//todo: db support
	json_t* series = json_object_get(root, "series");
	parseReleaseSeries(series, release->series);

	//todo: db support
	pfc::string8 fq(JSONAttributeString(root, "format_quantity").get_ptr());
	release->discogs_total_discs = std::stoi(fq.get_length()? fq.get_ptr() : "0");
	//todo: db support
	release->weight = JSONAttributeString(root, "estimated_weight");
		
	json_t* community = json_object_get(root, "community");
	if (json_is_object(community)) {
		release->discogs_status = JSONAttributeString(community, "status");
		release->members_have = JSONAttributeString(community, "have");
		release->members_want = JSONAttributeString(community, "want");
		json_t* rating = json_object_get(community, "rating");
		if (json_is_object(rating)) {
			release->discogs_avg_rating = JSONAttributeString(rating, "average");
			release->discogs_rating_votes = JSONAttributeString(rating, "count");
		}
		json_t* submitter = json_object_get(community, "submitter");
		if (json_is_object(submitter)) {
			release->submitted_by = JSONAttributeString(submitter, "username");
		}
		release->discogs_data_quality = JSONAttributeString(community, "data_quality");
	}
}

void Discogs::parseReleaseRating(Release *release, json_t *root) {
	release->discogs_my_rating = JSONAttributeString(root, "rating");
}

void Discogs::parseMasterRelease(MasterRelease *master_release, json_t *root) {
	assert_is_object(root);

	master_release->id = JSONAttributeString(root, "id");
	Release_ptr main_release = discogs_interface->get_release(encode_mr(0, master_release->id),JSONAttributeString(root, "main_release"));
	master_release->set_main_release(main_release);
	master_release->main_release_api_url = JSONAttributeString(root, "main_release_url");
	master_release->main_release_url = JSONAttributeString(root, "uri");
	master_release->release_year = JSONAttributeString(root, "year");
	master_release->versions_api_url = JSONAttributeString(root, "versions_url");
	master_release->title = JSONAttributeString(root, "title");
	master_release->discogs_data_quality = JSONAttributeString(root, "data_quality");
	master_release->genres = JSONAttributeStringArray(root, "genres");
	master_release->styles = JSONAttributeStringArray(root, "styles");
	master_release->videos = JSONAttributeObjectArrayAttributeString(root, "videos", "uri");
	master_release->search_role = JSONAttributeString(root, "role");
	master_release->search_roles.add_item(JSONAttributeString(root, "role"));

	json_t *images = json_object_get(root, "images");
	parseImages(images, master_release->images);

	json_t *artists = json_object_get(root, "artists");
	parseReleaseArtists(artists, master_release->artists);

	if (!master_release->artists.get_size()) {
		parser_exception ex;
		ex << " Master release has no artists!";
		throw ex;
	}

	json_t *tracklist = json_object_get(root, "tracklist");
	parseReleaseTracks(tracklist, master_release, master_release);
}

void Discogs::parseArtistReleases(json_t *root, Artist *artist) {
	try {
		//artist = nullptr; root = nullptr;
		json_t* releases = json_object_get(root, "releases");
		if (json_is_array(releases)) {

			for (size_t i = 0; i < json_array_size(releases); i++) {
				json_t* rel = json_array_get(releases, i);

				if (json_is_object(rel)) {
					
					pfc::string8 type = JSONAttributeString(rel, "type");

					if (STR_EQUAL(type, "master")) {

						// MASTERS

						pfc::string8 master_id = JSONAttributeString(rel, "id");

						unsigned long ulcoded = encode_mr(artist->search_role_list_pos, atoi(master_id));
						MasterRelease_ptr release = discogs_interface->get_master_release(ulcoded/*JSONAttributeString(rel, "id")*/);


						if (!release->loaded_preview) {

							release->title = JSONAttributeString(rel, "title");
							release->release_year = JSONAttributeString(rel, "year");
							if (release->release_year.get_length() == 0) {
								release->release_year = getYearFromReleased(JSONAttributeString(rel, "released"));
							}
							unsigned long lkey = encode_mr(artist->search_role_list_pos, JSONAttributeString(rel, "main_release"));
							Release_ptr main_release = discogs_interface->get_release(lkey);
							release->set_main_release(main_release);


							release->search_role = JSONAttributeString(rel, "role");
							release->search_roles.add_item(JSONAttributeString(rel, "role"));		
						}

						bool duplicate = false;
						for (size_t r = 0; r < artist->master_releases.get_size(); r++) {
							if (release->id == artist->master_releases[r]->id) {
								if (!release->search_role.equals("Main")) {
									release->search_role = JSONAttributeString(rel, "role");
								}
								release->search_roles.add_item(JSONAttributeString(rel, "role"));
								duplicate = true;
								break;
							}
						}
						if (!duplicate) {
							release->loaded_preview = true;
							artist->master_releases.append_single(std::move(release));
							artist->search_order_master.append_single(true);
						}
					}
					else {

						// NON-MASTER RELASE & RELEASES

						unsigned long lkey = encode_mr(artist->search_role_list_pos, JSONAttributeString(rel, "id"));
						Release_ptr release = discogs_interface->get_release(lkey, decode_mr(lkey).second);

						void* iter = json_object_iter((json_t*)rel);

						if (!release->loaded_preview) {

							release->title = JSONAttributeString(rel, "title");
							release->search_labels = JSONAttributeString(rel, "label");

							pfc::string8 all_formats = JSONAttributeString(rel, "format");
							pfc::array_t<pfc::string8> flist;
							tokenize(all_formats, ",", flist, false);
							tokenize(flist[0], ",", release->search_major_formats, false);
							t_size formats_cpos = flist[0].get_length();
							release->search_formats = ltrim(substr(all_formats, formats_cpos + 1, all_formats.get_length() - formats_cpos));
							release->search_catno = JSONAttributeString(rel, "catno");
							release->release_year = JSONAttributeString(rel, "year");
							if (release->release_year.get_length() == 0) {
								release->release_year = getYearFromReleased(JSONAttributeString(rel, "released"));
							}
							release->country = JSONAttributeString(rel, "country");
							release->search_role = JSONAttributeString(rel, "role");
							release->search_roles.add_item(JSONAttributeString(rel, "role"));
						}
						bool duplicate = false;
						Release_ptr prev;

						for (size_t r = 0; r < artist->releases.get_size(); r++) {
							if (release->id == artist->releases[r]->id) {
								prev = artist->releases[r];

								if (!artist->releases[r]->id.equals("Main")) {
									release->search_role = JSONAttributeString(rel, "role");
								}
								release->search_roles.add_item(JSONAttributeString(rel, "role"));
								duplicate = true;
								break;
							}
						}

						if (!duplicate) {

							release->loaded_preview = true;
							artist->releases.append_single(std::move(release));
							artist->search_order_master.append_single(false);

						}
					}
				}
			}
		}
	}
	catch (...) {
		foo_discogs_exception ex;
		ex << "Unknown error parsing artist releases.";
		throw ex;
	}
}

void Discogs::parseMasterVersions(json_t *root, MasterRelease *master_release) {

	json_t *versions = json_object_get(root, "versions");

	if (json_is_array(versions)) {
		for (size_t i = 0; i < json_array_size(versions); i++) {
			json_t *rel = json_array_get(versions, i);
			
			if (json_is_object(rel)) {
				pfc::string8 release_id = JSONAttributeString(rel, "id");
				unsigned long lkey = encode_mr(0, atol(release_id));
				Release_ptr release = discogs_interface->get_release(lkey);
				//mr from any artist
				lkey = encode_mr(0, atol(master_release->id));
				//..
				MasterRelease_ptr master = discogs_interface->get_master_release(lkey/*master_release->id*/);

				if (!release->loaded_preview) {
					release->set_master_release(master);
					release->title = JSONAttributeString(rel, "title");
					release->search_labels = JSONAttributeString(rel, "label");
					release->search_major_formats = JSONAttributeStringArray(rel, "major_formats");
					release->search_formats = JSONAttributeString(rel, "format");
					release->search_catno = JSONAttributeString(rel, "catno");
					release->release_year = JSONAttributeString(rel, "year");
					release->db_total_tracks = atoi(JSONAttributeString(rel, "db_total_tracks"));
					if (release->release_year.get_length() == 0) {
						release->release_year = getYearFromReleased(JSONAttributeString(rel, "released"));
					}
					release->country = JSONAttributeString(rel, "country");
				}

				bool duplicate = false;
				for (size_t r = 0; r < master_release->sub_releases.get_size(); r++) {
					Release_ptr sub_release = master_release->sub_releases[r];
					if (master_release->sub_releases[r]->id == release->id) {
						duplicate = true;
						break;
					}
				}
				if (!duplicate) {
					release->loaded_preview = true;
					master_release->sub_releases.append_single(std::move(release));
				}
			}
		}
	}
}

void Discogs::parseArtist(Artist *artist, json_t *root) {

	assert_is_object(root);

	artist->profile = JSONAttributeString(root, "profile");
	artist->name = JSONAttributeString(root, "name");

	artist->id = JSONAttributeString(root, "id");

	artist->anvs = JSONAttributeStringArray(root, "namevariations");
	artist->urls = JSONAttributeStringArray(root, "urls");

	artist->realname = JSONAttributeString(root, "realname");

	json_t *images = json_object_get(root, "images");
	parseImages(images, artist->images);

	artist->aliases = JSONAttributeObjectArrayAttributeString(root, "aliases", "name");
	artist->ingroups = JSONAttributeObjectArrayAttributeString(root, "groups", "name");
	artist->members = JSONAttributeObjectArrayAttributeString(root, "members", "name");
}

void Discogs::parseArtistResults(json_t *root, pfc::array_t<Artist_ptr> &artists) {
	assert_is_object(root);
	json_t *results = json_object_get(root, "results");
	if (json_is_array(results)) {
		for (size_t i = 0; i < json_array_size(results); i++) {
			json_t *result = json_array_get(results, i);
			if (json_is_object(result)) {
				Artist_ptr artist = discogs_interface->get_artist(JSONAttributeString(result, "id"));
				artist->name = JSONAttributeString(result, "title");
				artists.append_single(std::move(artist));
			}
		}
	}
}


void initialize_null_artist(Artist *artist) {
	artist->profile = "";
	artist->anvs = pfc::array_t<pfc::string8>();
	artist->urls = pfc::array_t<pfc::string8>();
	artist->realname = "";
	artist->aliases = pfc::array_t<pfc::string8>();
	artist->ingroups = pfc::array_t<pfc::string8>();
	artist->members = pfc::array_t<pfc::string8>();
	artist->loaded = true;
}

//todo: check integration with discogs_db_interface::get_artist
void Discogs::Artist::load(threaded_process_status &p_status, abort_callback &p_abort, bool throw_all) {
	
	if (loaded) {
		return;
	}

	pfc::string8 rel_path;

	bool db_isready = DBFlags(CONF.db_dc_flag).IsReady() && !(DBFlags(CONF.db_dc_flag).WantArtwork());

	bool offline_can_read = ol::can_read();
	bool offline_can_write = ol::can_write();
	bool offline_can_overwrite = ol::can_ovr();

	bool offline_avail_data = ol::is_data_avail(id, "", ol::GetFrom::Artist, rel_path);
	bool btransient = !(offline_can_read && offline_avail_data) || offline_can_overwrite;

	btransient &= !db_isready;

	try {
		if (STR_EQUAL(id, "355") || STR_EQUAL(id, "Unknown Artist")) {
			name = "Unknown Artist";
			initialize_null_artist(this);
			return;
		}
		else if (STR_EQUAL(id, "118760") || STR_EQUAL(id, "No Artist")) {
			name = "No Artist";
			initialize_null_artist(this);
			return;
		}
		else if (STR_EQUAL(id, "194") || STR_EQUAL(id, "Various")) {
			name = "Various";
			initialize_null_artist(this);
			return;
		}
		
		pfc::string8 msg("Loading artist ");
		msg << id << "...";
		p_status.set_item(msg);

		pfc::string8 json;
		pfc::string8 url;

		if (btransient) {
			url << "https://api.discogs.com/artists/" << id;
			discogs_interface->fetcher->fetch_html(url, "", json, p_abort);	
		}
		else {
			if (offline_avail_data) {
				discogs_interface->get_artist_offline_cache(id, json, p_abort, "Fetching offline cache artist releases...", p_status);

				if (json.get_length()) {
					offline_can_write = false;
				}
				else {
					//todo: recover from an invalid cache
					//delete cache and reload?
					pfc::string8 msg;
					msg << "Error loading disk cache, deleting invalid cache (artist_id:" << id << ") ...";
					log_msg(msg);
					discogs_interface->delete_artist_cache(id);
					url << "https://api.discogs.com/artists/" << id;
					discogs_interface->fetcher->fetch_html(url, "", json, p_abort);
				}
			}
#ifdef DB_DC
			else {
				discogs_db_interface->get_artist_DB(id, json, p_abort, "Fetching artist from backup db...", p_status, nullptr);
				//cache this if enabled
				btransient = true;
			}

#endif // DB_DC

			//we have just read from offline, do not write in next step!!!
		}

#ifdef _DEBUG
		log_msg(json);
#endif

		if (!json.get_length()) {
			foo_discogs_exception ex("Truncated results error.");
			throw ex;
		}


		// parse json and artist

		JSONParser jp(json);
		parseArtist(this, jp.root);

		//..

		loaded = true;
		bool bCacheSaved;

		if (btransient && offline_can_write) {
			try {

				//create page-n folder
				ol::CreateOfflinePath_ID(id, ol::GetFrom::Artist);

				//PENDING
				bool bmark_loading = ol::MarkDownload("", rel_path, false/*done*/);

				pfc::string8 page_path = ol::GetOfflinePath(id, true, ol::GetFrom::Artist, "");
				page_path << "\\root.json";

				bCacheSaved = bmark_loading && discogs_interface->offline_cache_save(page_path, jp.root);
			}
			catch (...) {
				//todo: error writting to disk
			}
		}

		if (btransient && offline_can_write) {
			if (bCacheSaved) {
				try {

					//DONE
					bool bmark_done = ol::MarkDownload("", rel_path, true/*done*/);
				}
				catch (...) {
					//todo: failed to created file
				}
			}
		}
	}
	catch (foo_discogs_exception &e) {
		if (throw_all) {
			throw;
		}

		pfc::string8 error("Error loading artist ");
		error << id << ": " << e.what();
		throw foo_discogs_exception(error);
	}
}

void Discogs::Release::load(threaded_process_status& p_status, abort_callback& p_abort, bool throw_all) {
	if (loaded) {
		return;
	}
	try {
		pfc::string8 msg("Loading release ");
		msg << id << "...";
		p_status.set_item(msg);

		pfc::string8 json;
		pfc::string8 url;
		url << "https://api.discogs.com/releases/" << id;
		discogs_interface->fetcher->fetch_html(url, "", json, p_abort);

		JSONParser jp(json);
		parseRelease(this, jp.root);
		loaded = true;
	}
	catch (foo_discogs_exception& e) {
		if (throw_all) {
			throw;
		}
		pfc::string8 error("Error loading release ");
		error << id << ": " << e.what();
		throw foo_discogs_exception(error);
	}
}
//todo: exec paths to (both) release::load
void Discogs::Release::load(threaded_process_status &p_status, abort_callback &p_abort, bool throw_all, pfc::string8 offlineArtistId, db_fetcher* dbfetcher) {
	
	if (loaded) {
		return;
	}

	pfc::string8 artist_id = offlineArtistId;
	pfc::string8 release_id = id;
	pfc::string8 rel_path;

	bool offline_can_read = ol::can_read();
	bool offline_can_write = ol::can_write();
	bool offline_can_ovr = ol::can_ovr();

	bool offline_avail_data = ol::is_data_avail(artist_id, release_id, ol::GetFrom::Release, rel_path);

	bool btransient = !(offline_can_read && offline_avail_data) || offline_can_ovr;

	bool db_isready = DBFlags(CONF.db_dc_flag).IsReady();
	bool db_wantartwork = DBFlags(CONF.db_dc_flag).WantArtwork();
	bool db_skip = !db_isready || db_wantartwork || !dbfetcher;

	btransient &= db_skip;

	try {

		pfc::string8 json;

		if (btransient) {

			pfc::string8 msg;
			msg << "Loading online releases\\" << id;
			p_status.set_item(msg);

			pfc::string8 url;
			url << "https://api.discogs.com/releases/" << id;
			discogs_interface->fetcher->fetch_html(url, "", json, p_abort);
		
		}
		else {

			//parseRelease_DB(this, p_status, p_abort);
#ifdef DB_DC
			if (!db_skip)
				discogs_db_interface->get_release_db(id, json, "", p_abort, "Gettting release from local db",p_status, dbfetcher);
			else 

#endif // DB_DC
				discogs_interface->get_release_offline_cache(artist_id, release_id, json, p_abort, "Fetching offline release...", p_status);

			//we have just read from offline, do not write in next step!!!
			offline_can_write = false;
		}

#ifdef DEBUG
		log_msg(json);
#endif

		//parse json and release

		JSONParser jp(json);
		parseRelease(this, jp.root);

		//..

		loaded = true;

		bool bCacheSaved;
		if (btransient && offline_can_write) {
			try {
				//create page-n folder
				ol::CreateOfflinePath_ID(artists[0]->id, ol::GetFrom::Release, id);
				//PENDING
				bool bmark_loading = ol::MarkDownload("", rel_path, false/*done*/);

				pfc::string8 page_path = ol::GetOfflinePath(artists[0]->id, true, ol::GetFrom::Release, id);
				page_path << "\\root.json";

				bCacheSaved = bmark_loading && discogs_interface->offline_cache_save(page_path, jp.root);
			}
			catch (...) {
				//todo: error writting to disk				
			}
		}

		if (btransient && offline_can_write) {

			if (bCacheSaved) {
				try {
					//DONE
					bool bmark_done = ol::MarkDownload("", rel_path, true/*done*/);
				}
				catch (...) {
					//todo: error writting to disk
				}
			}
		}
	}
	catch (foo_discogs_exception &e) {
		if (throw_all) {
			throw;
		}
		pfc::string8 error("Error loading release ");
		error << id << ": " << e.what();
		throw foo_discogs_exception(error);
	}
}

void Discogs::Release::load_my_rating(threaded_process_status &p_status, abort_callback &p_abort, bool throw_all) {
	if (loaded_my_rating) {
		return;
	}
	try {
		pfc::string8 msg("Loading release ");
		msg << id << " personal rating...";
		p_status.set_item(msg);

		pfc::string8 json;
		pfc::string8 url;
		url << "https://api.discogs.com/releases/" << id << "/rating/" << discogs_interface->get_username(p_status, p_abort);
		discogs_interface->fetcher->fetch_html(url, "", json, p_abort);

		JSONParser jp(json);
		parseReleaseRating(this, jp.root);
		loaded_my_rating = true;
	}
	catch (foo_discogs_exception &e) {
		if (throw_all) {
			throw;
		}
		pfc::string8 error("Error loading release ");
		error << id << ": " << e.what();
		throw foo_discogs_exception(error);
	}
}

void Discogs::MasterRelease::load(threaded_process_status &p_status, abort_callback &p_abort, bool throw_all) {
	if (loaded || !id.get_length()) {
		return;
	}
	try {
		pfc::string8 json;
		pfc::string8 url;
		url << "https://api.discogs.com/masters/" << id;
		discogs_interface->fetcher->fetch_html(url, "", json, p_abort);

		JSONParser jp(json);
		parseMasterRelease(this, jp.root);
		loaded = true;
	}
	catch (foo_discogs_exception &e) {
		if (throw_all) {
			throw;
		}
		pfc::string8 error("Error loading master release ");
		error << id << ": " << e.what();
		throw foo_discogs_exception(error);
	}
}


void Discogs::Artist::load_releases(threaded_process_status &p_status, abort_callback &p_abort, bool throw_all, db_fetcher* dbfetcher) {
	if (loaded_releases) {
		return;
	}

	pfc::string8 rel_path;

	bool offline_can_read = ol::can_read();
	bool offline_can_write = ol::can_write();
	bool offline_can_overwrite = ol::can_ovr();
	bool offline_avail_data = ol::is_data_avail(id, "", ol::GetFrom::ArtistReleases, rel_path);

	bool btransient = !(offline_can_read && offline_avail_data) || offline_can_overwrite;
	
	bool db_ready = DBFlags(CONF.db_dc_flag).IsReady();
	btransient &= !db_ready;

	try {
		pfc::string8 url;
		pfc::array_t<JSONParser_ptr> pages;
		if (btransient) {
			url << "https://api.discogs.com/artists/" << id << "/releases";
			pages = discogs_interface->get_all_pages(url, "", p_abort, "Fetching artist releases...", p_status);
		}
		else {

			bool db_ready = DBFlags(CONF.db_dc_flag).IsReady();
#ifdef DB_DC
			if (db_ready)

				pages = discogs_db_interface->releases_get_all_pages(id, id, "", p_abort, "Fetching local database artist releases...", p_status, dbfetcher);

			else
#endif				
				pages = discogs_interface->get_all_pages_offline_cache(ol::GetFrom::ArtistReleases, id, id, "", p_abort, "Fetching offline artist releases...", p_status);


			//we have just read from offline, do not write in next step!!!
			offline_can_write = false;
		}
		loaded_releases_offline = !btransient;

		const size_t count = pages.get_count();

		bool bCacheSaved = true;
		bool bmark_pending = false;

		for (size_t i = 0; i < count; i++) {

			//parse artist releases

			parseArtistReleases(pages[i]->root, this);

			//..

			if (btransient && offline_can_write) {

				try {

					if (i == 0) {
						//create release folder
						bool bfolder_ready = ol::CreateOfflinePath_PAGES(id, art_src::unknown, pfc_infinite, ol::GetFrom::ArtistReleases, "");

						if (bfolder_ready) {
						
							pfc::string8 markcontent = JSONString(json_object_get(pages[i]->root, "items"));

							//mark pending job
							bmark_pending = ol::MarkDownload(markcontent, rel_path, false/*done*/);
						}
					}
					
					//create page-n folder
					if (bmark_pending) {
					
						bool bfolder_ready = ol::CreateOfflinePath_PAGES(id, art_src::unknown, i, ol::GetFrom::ArtistReleases, "");

						if (bfolder_ready) {
						
							pfc::string8 page_path = ol::GetOfflinePagesPath(id, i, true, ol::GetFrom::ArtistReleases, "");
							page_path << "\\root.json";
					
							bCacheSaved &= discogs_interface->offline_cache_save(page_path, pages[i]->root);
						}					
					}
					else {
						bCacheSaved = false;
					}
				}
				catch (...) {
					//todo: error writting to disk
				}
			}
            
            //debug
			//json_t* releases = json_object_get(pages[i]->root, "releases");
			//if (json_is_array(releases)) {
			//	for (size_t i = 0; i < json_array_size(releases); i++) {
			//		json_t* rel = json_array_get(releases, i);
			//		if (json_is_object(rel)) {
			//			pfc::string8 type = JSONAttributeString(rel, "type");
			//			if (STR_EQUAL(type, "master")) {
			//				pfc::string8 id = JSONAttributeString(rel, "id");
			//				//void* iter = json_object_iter((json_t*)rel);
			//			}
			//		}
			//	}
			//}
			
			//todo: move to prev condition block

			if (btransient && offline_can_write) {

				if ((i == count - 1) && (bCacheSaved)) {
					
					try {

						bool bmark_done = ol::MarkDownload("", rel_path, true/*done*/);
					}
					catch (...) {
						//todo: error writting to disk
					}
				}
			}
		}

		loaded_releases = true;
	}
	catch (foo_discogs_exception &e) {
		if (throw_all) {
			throw;
		}
		pfc::string8 error("Error loading artist releases (Artist id: ");
		error << id << ") " << e.what();
		throw foo_discogs_exception(error);
	}
	catch (...) {
		foo_discogs_exception ex;
		ex << "Unknown error loading releases.";
		throw ex;
	}
}

void Discogs::MasterRelease::load_releases(threaded_process_status &p_status, abort_callback &p_abort, bool throw_all, pfc::string8 offlineArtistId, db_fetcher* dbfetcher) {
	
	if (loaded_releases || !id.get_length()) {
		return;
	}

	pfc::string8 artist_id = offlineArtistId;
	pfc::string8 master_id = id;
	ol::GetFrom gfVersions = ol::GetFrom::Versions;

	pfc::string8 rel_path;

	bool offline_can_read = ol::can_read();
	bool offline_can_write = ol::can_write();
	bool offline_can_overwrite = ol::can_ovr();

	bool offline_avail_data = ol::is_data_avail(artist_id, master_id, gfVersions, rel_path);
	bool btransient = !(offline_can_read && offline_avail_data) || offline_can_overwrite;
	
	bool db_isready = DBFlags(CONF.db_dc_flag).IsReady();
	btransient &= !db_isready;

	try {

		pfc::string8 url;
		pfc::array_t<JSONParser_ptr> pages;

		if (btransient) {
			url << "https://api.discogs.com/masters/" << id << "/versions";
			pages = discogs_interface->get_all_pages(url, "", p_abort, "Fetching master release versions...", p_status);
		}
		else {
#ifdef DB_DC
			if (db_isready)
				pages = discogs_db_interface->versions_get_all_pages(artist_id, master_id, "", p_abort, "Fetching local database master releases versions ...", p_status, dbfetcher);
			else
#endif
				pages = discogs_interface->get_all_pages_offline_cache(gfVersions, artist_id, master_id, "", p_abort, "Fetching offline master releases versions ...", p_status);

			//we have just read from offline, do not write in next step
			offline_can_write = false;
		}

		const size_t cPages = pages.get_count();

		bool bCacheSaved = true;
		bool bmark_pending = false;

		for (size_t i = 0; i < cPages; i++) {

			//parse master versions

			parseMasterVersions(pages[i]->root, this);

			//..

			if (btransient && offline_can_write) {

				try {

					if (i == 0) {
						//create release folder
						bool bfolder_ready = ol::CreateOfflinePath_PAGES(artist_id, art_src::unknown, pfc_infinite, gfVersions, master_id);

						if (bfolder_ready) {
						
							pfc::string8 markcontent = JSONString(json_object_get(pages[i]->root, "items"));

							//mark as pending
							bmark_pending = ol::MarkDownload(markcontent, rel_path, false);
						}
					}

					if (bmark_pending) {

						//create page-n folder
						bool bfolder_ready = ol::CreateOfflinePath_PAGES(artist_id, art_src::unknown, i, gfVersions, master_id);

						if (bfolder_ready) {
						
							pfc::string8 page_path = ol::GetOfflinePagesPath(artist_id, i, true, gfVersions, master_id);
							page_path << "\\root.json";
					
							bCacheSaved &= discogs_interface->offline_cache_save(page_path, pages[i]->root);
						}
					}
				}
				catch (...) {
					//todo: error writting to disk
				}

				if (btransient && offline_can_write) {
					if ((i == cPages - 1) && (bCacheSaved)) {
						try {

							//mark as done...
							bool bmark_done = ol::MarkDownload("", rel_path, true);
						}
						catch (...) {
							//todo: failed to created file
						}
					}
				}
			}
		}
		loaded_releases = true;
	}
	catch (foo_discogs_exception &e) {
		if (throw_all) {
			throw;
		}
		pfc::string8 error("Error loading master release versions");
		error << id << ": " << e.what();
		throw foo_discogs_exception(error);
	}
}

void Discogs::parseIdentity(json_t *root, Identity *identity) {
	
	assert_is_object(root);

	identity->user_id = JSONAttributeString(root, "id");
	identity->username = JSONAttributeString(root, "username");
	identity->consumer_name = JSONAttributeString(root, "consumer_name");

	pfc::string8 resource_url = JSONAttributeString(root, "resource_url");
}

void Discogs::parseCollection(json_t *root, pfc::array_t<pfc::string8> &collection) {
	
	assert_is_object(root);

	json_t *releases = json_object_get(root, "releases");
	assert_is_array(releases);

	for (size_t i = 0; i < json_array_size(releases); i++) {
		json_t *release = json_array_get(releases, i);
		if (json_is_object(release)) {
			json_t *info = json_object_get(release, "basic_information");
			if (json_is_object(info)) {
				collection.append_single(JSONAttributeString(info, "id"));
			}
		}
	}
}

/*
void Discogs::parseProfile(json_t* root, Profile* profile) {
	
	assert_is_object(root);

	profile->id = JSONAttributeString(root, "id");
	profile->username = JSONAttributeString(root, "username");
	profile->email = JSONAttributeString(root, "email");

	//pfc::string8 resource_url = JSONAttributeString(root, "resource_url");
	//int dbug = resource_url.length();
}
*/