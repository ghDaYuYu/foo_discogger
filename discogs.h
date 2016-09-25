#pragma once

#include "exception.h"
#include "utils.h"
#include "conf.h"
#include "jansson.h"
#include "string_encoded_array.h"
#include "pfc_helpers.h"

#include <map>
#include <functional>


namespace Discogs
{
	class missing_data_exception : public foo_discogs_exception
	{};

	class DiscogsInterface;

	extern pfc::string8 remove_number_suffix(const pfc::string8 &src);
	extern pfc::string8 move_the_to_start(const pfc::string8 &src);
	extern pfc::string8 move_the_to_end(const pfc::string8 &src);
	extern pfc::string8 strip_artist_name(const pfc::string8 str);
	extern pfc::string8 format_track_number(int tracknumber);


	class ExposesTags
	{
	public:
		bool loaded = false;
		static DiscogsInterface *discogs;

		virtual string_encoded_array get_data(pfc::string8 &tag_name, abort_callback &p_abort) = 0;

		virtual ~ExposesTags() {}
	};
	typedef std::shared_ptr<ExposesTags> ExposesTags_ptr;


	template <typename T>
	class ExposedTags : public virtual ExposesTags
	{
	protected:
		const static std::map<const char*, string_encoded_array(T::*)()const, cmp_str> exposed_tags;

	public:
		static std::map<const char*, string_encoded_array(ExposedTags::*)()const, cmp_str> create_default_tags_map() {
			std::map<const char*, string_encoded_array(ExposedTags::*)()const, cmp_str> m;
			return m;
		}

		inline bool has_data(pfc::string8 &tag_name) {
			return exposed_tags.count(tag_name.get_ptr()) != 0;
		}
		
		virtual string_encoded_array get_data(pfc::string8 &tag_name, abort_callback &p_abort) override {
			auto &it = exposed_tags.find(tag_name.get_ptr());
			if (it != exposed_tags.cend()) {
				auto x = std::bind(it->second, ((T*)this));
				pfc::string8 out;
				try {
					out = x();
				}
				catch (pfc::uninitialized_exception) {
					if (!this->loaded) {
						g_discogs->discogs->load(((T*)this), p_abort);
						try {
							out = x();
						}
						catch (pfc::uninitialized_exception) {
							out = "";
						}
					}
					else {
						out = "";
					}
				}
				return out;
			}
			else {
				return get_sub_data(tag_name, p_abort);
			}
		}

		virtual string_encoded_array get_sub_data(pfc::string8 &tag_name, abort_callback &p_abort) {
			missing_data_exception ex;
			ex << "Unknown tag: " << tag_name;
			throw ex;
		}
	};


	class Image : public ExposedTags<Image>
	{
	public:
		pfc::string8_ex type; // "primary" or "secondary"
		pfc::string8_ex url;
		pfc::string8_ex url150;

		string_encoded_array get_type() const {
			return type;
		}

		string_encoded_array get_url() const {
			return url;
		}

		string_encoded_array get_thumbnail_url() const {
			return url150;
		}

		static std::map<const char*, string_encoded_array(Image::*)()const, cmp_str> create_tags_map() {
			std::map<const char*, string_encoded_array(Image::*)()const, cmp_str> m;
			m["TYPE"] = &Image::get_type;
			m["URL"] = &Image::get_url;
			m["THUMBNAIL_URL"] = &Image::get_thumbnail_url;
			return m;
		}
	};
	typedef std::shared_ptr<Image> Image_ptr;
	

	class HasImages
	{
	public:
		pfc::array_t<Image_ptr> images;
	};


	class Release;
	typedef std::shared_ptr<Release> Release_ptr;

	class MasterRelease;
	typedef std::shared_ptr<MasterRelease> MasterRelease_ptr;


	class Artist : public HasImages, public ExposedTags<Artist>
	{
	public:
		pfc::string8_ex id;
		pfc::string8_ex name;
		pfc::string8_ex profile;
		pfc::string8_ex realname;
		pfc::array_t<Release_ptr> releases;
		pfc::array_t<MasterRelease_ptr> master_releases;
		pfc::array_t<bool> search_order_master;
		pfc::array_t_ex<pfc::string8> urls;
		pfc::array_t_ex<pfc::string8> aliases;
		pfc::array_t_ex<pfc::string8> ingroups;
		pfc::array_t_ex<pfc::string8> anvs;
		pfc::array_t_ex<pfc::string8> members;
		bool loaded_releases = false;

		string_encoded_array get_id() const {
			return id;
		}
		string_encoded_array get_name() const {
			pfc::string8 result = name;
			if (CONF.discard_numeric_suffix) {
				result = remove_number_suffix(result);
			}
			if (CONF.move_the_at_beginning) {
				result = move_the_to_start(result);
			}
			return result;
		}
		string_encoded_array get_profile() const {
			return profile;
		}
		string_encoded_array get_realname() const {
			return realname;
		}
		string_encoded_array get_urls() const {
			return urls;
		}
		string_encoded_array get_aliases() const {
			return aliases;
		}
		string_encoded_array get_ingroups() const {
			return ingroups;
		}
		string_encoded_array get_anvs() const {
			return anvs;
		}
		string_encoded_array get_members() const {
			return members;
		}

		static std::map<const char*, string_encoded_array(Artist::*)()const, cmp_str> create_tags_map() {
			std::map<const char*, string_encoded_array(Artist::*)()const, cmp_str> m;
			m["ID"] = &Artist::get_id;
			m["NAME"] = &Artist::get_name;
			m["PROFILE"] = &Artist::get_profile;
			m["REAL_NAME"] = &Artist::get_realname;
			m["URLS"] = &Artist::get_urls;
			m["ALIASES"] = &Artist::get_aliases;
			m["GROUPS"] = &Artist::get_ingroups;
			m["ALL_NAME_VARIATIONS"] = &Artist::get_anvs;
			m["MEMBERS"] = &Artist::get_members;
			return m;
		}

		virtual string_encoded_array get_sub_data(pfc::string8 &tag_name, abort_callback &p_abort) override {
			pfc::string8 sub_tag_name;
			string_encoded_array result;
			if (STR_EQUALN(tag_name, "IMAGES_", 7)) {
				sub_tag_name = substr(tag_name, 7);
				for (size_t i = 0; i < images.get_size(); i++) {
					result.append_item_val(images[i]->get_data(sub_tag_name, p_abort));
				}
			}
			else {
				return ExposedTags<Artist>::get_sub_data(tag_name, p_abort);
			}
			result.encode();
			return result;
		}

		Artist(const char *id) : id(id) {
		}
	};
	typedef std::shared_ptr<Artist> Artist_ptr;


	class HasRoles
	{
	public:
		pfc::string8_ex raw_roles;
		pfc::array_t_ex<pfc::string8> roles;
		pfc::array_t_ex<pfc::string8> full_roles;

		string_encoded_array get_raw_roles() const {
			return raw_roles;
		}
		string_encoded_array get_roles() const {
			return full_roles;
		}
		string_encoded_array get_roles_short() const {
			return roles;
		}
	};


	class ReleaseArtist : public HasRoles, public ExposedTags<ReleaseArtist>
	{
	public:
		Artist_ptr full_artist;
		pfc::string8_ex id;
		pfc::string8_ex name;
		pfc::string8_ex anv;
		pfc::string8_ex join;

		string_encoded_array get_id() const {
			return id;
		}
		string_encoded_array get_name() const {
			pfc::string8 result;
			if (!CONF.replace_ANVs && anv.get_length()) {
				result = anv;
			}
			else {
				result = name;
			}
			if (CONF.discard_numeric_suffix) {
				result = remove_number_suffix(result);
			}
			if (CONF.move_the_at_beginning) {
				result = move_the_to_start(result);
			}
			return result;
		}
		string_encoded_array get_anv() const {
			return anv;
		}
		string_encoded_array get_join() const {
			return join;
		}

		static std::map<const char*, string_encoded_array(ReleaseArtist::*)()const, cmp_str> create_tags_map() {
			std::map<const char*, string_encoded_array(ReleaseArtist::*)()const, cmp_str> m;
			m["ID"] = &ReleaseArtist::get_id;
			m["NAME"] = &ReleaseArtist::get_name;
			m["NAME_VARIATION"] = &ReleaseArtist::get_anv;
			m["JOIN"] = &ReleaseArtist::get_join;
			return m;
		}

		string_encoded_array get_data(pfc::string8 &tag_name, abort_callback &p_abort) override {
			try {
				return ExposedTags<ReleaseArtist>::get_data(tag_name, p_abort);
			}
			catch (missing_data_exception) {
				pfc::string8 xxx = full_artist->name;
				return full_artist->get_data(tag_name, p_abort);
			}
		}

		ReleaseArtist(const char *id, Artist_ptr &full_artist) : full_artist(full_artist), id(id) {
		}

		bool has_anv() const {
			return anv.get_length() != 0;
		}
	};
	typedef std::shared_ptr<ReleaseArtist> ReleaseArtist_ptr;


	class HasArtists
	{
	public:
		pfc::array_t<ReleaseArtist_ptr> artists;

		virtual bool has_anv() const {
			for (size_t i = 0; i < artists.get_size(); i++) {
				if (artists[i]->has_anv()) {
					return true;
				}
			}
			return false;
		}

		virtual ~HasArtists() {}
	};


	class ReleaseCredit : public HasRoles, public HasArtists, public ExposedTags<ReleaseCredit>
	{
	public:
		static std::map<const char*, string_encoded_array(ReleaseCredit::*)()const, cmp_str> create_tags_map() {
			std::map<const char*, string_encoded_array(ReleaseCredit::*)()const, cmp_str> m;
			m["ROLES_RAW"] = &ReleaseCredit::get_raw_roles;
			m["ROLES"] = &ReleaseCredit::get_roles;
			m["SHORT_ROLES"] = &ReleaseCredit::get_roles_short;
			return m;
		}

		virtual string_encoded_array get_sub_data(pfc::string8 &tag_name, abort_callback &p_abort) override {
			pfc::string8 sub_tag_name;
			string_encoded_array result;
			if (strncmp(tag_name.get_ptr(), "ARTISTS_", 8) == 0) {
				sub_tag_name = pfc::string8(tag_name.get_ptr() + 8);
				for (size_t i = 0; i < artists.get_size(); i++) {
					result.append_item_val(artists[i]->get_data(sub_tag_name, p_abort));
				}
			}
			else {
				return ExposedTags<ReleaseCredit>::get_sub_data(tag_name, p_abort);
			}
			result.encode();
			return result;
		}
	};
	typedef std::shared_ptr<ReleaseCredit> ReleaseCredit_ptr;


	class HasCredits
	{
	public:
		pfc::array_t<ReleaseCredit_ptr> credits;

		virtual bool has_anv() const {
			for (size_t i = 0; i < credits.get_size(); i++) {
				if (credits[i]->has_anv()) {
					return true;
				}
			}
			return false;
		}

		virtual ~HasCredits() {}
	};


	class ReleaseLabel : public ExposedTags<ReleaseLabel>
	{
	public:
		pfc::string8_ex id;
		pfc::string8_ex name;
		pfc::string8_ex catalog;

		string_encoded_array get_id() const {
			return id;
		}
		string_encoded_array get_name() const {
			pfc::string8 result = name;
			if (CONF.discard_numeric_suffix) {
				result = remove_number_suffix(result);
			}
			return result;
		}
		string_encoded_array get_catalog() const {
			return catalog;
		}

		static std::map<const char*, string_encoded_array(ReleaseLabel::*)()const, cmp_str>  create_tags_map() {
			std::map<const char*, string_encoded_array(ReleaseLabel::*)()const, cmp_str> m;
			m["ID"] = &ReleaseLabel::get_id;
			m["NAME"] = &ReleaseLabel::get_name;
			m["CATALOG_NUMBER"] = &ReleaseLabel::get_catalog;
			return m;
		}
	};
	typedef std::shared_ptr<ReleaseLabel> ReleaseLabel_ptr;


	class ReleaseSeries : public ExposedTags<ReleaseSeries>
	{
	public:
		pfc::string8_ex id;
		pfc::string8_ex name;
		pfc::string8_ex api_url;
		pfc::string8_ex number;

		string_encoded_array get_id() const {
			return id;
		}
		string_encoded_array get_name() const {
			pfc::string8 result = name;
			if (CONF.discard_numeric_suffix) {
				result = remove_number_suffix(result);
			}
			return result;
		}
		string_encoded_array get_number() const {
			return number;
		}
		string_encoded_array get_api_url() const {
			return api_url;
		}

		static std::map<const char*, string_encoded_array(ReleaseSeries::*)()const, cmp_str> create_tags_map() {
			std::map<const char*, string_encoded_array(ReleaseSeries::*)()const, cmp_str> m;
			m["ID"] = &ReleaseSeries::get_id;
			m["NAME"] = &ReleaseSeries::get_name;
			m["NUMBER"] = &ReleaseSeries::get_number;
			m["API_URL"] = &ReleaseSeries::get_api_url;
			return m;
		}
	};
	typedef std::shared_ptr<ReleaseSeries> ReleaseSeries_ptr;


	class ReleaseFormat : public ExposedTags<ReleaseFormat>
	{
	public:
		pfc::string8_ex name; 
		pfc::string8_ex qty;
		pfc::array_t_ex<pfc::string8> descriptions;
		pfc::string8_ex text;

		string_encoded_array get_quantity() const {
			return qty;
		}
		string_encoded_array get_name() const {
			return name;
		}
		string_encoded_array get_descriptions() const {
			return string_encoded_array(descriptions);
		}
		string_encoded_array get_text() const {
			return text;
		}

		static std::map<const char*, string_encoded_array(ReleaseFormat::*)()const, cmp_str> create_tags_map() {
			std::map<const char*, string_encoded_array(ReleaseFormat::*)()const, cmp_str> m;
			m["QUANTITY"] = &ReleaseFormat::get_quantity;
			m["NAME"] = &ReleaseFormat::get_name;
			m["DESCRIPTIONS"] = &ReleaseFormat::get_descriptions;
			m["TEXT"] = &ReleaseFormat::get_text;
			return m;
		}
	};
	typedef std::shared_ptr<ReleaseFormat> ReleaseFormat_ptr;


	class ReleaseTrack : public HasArtists, public HasCredits, public ExposedTags<ReleaseTrack>
	{
	public:
		pfc::string8_ex title;
		pfc::string8_ex title_index;
		pfc::string8_ex title_subtrack;
		pfc::string8_ex title_heading;
		pfc::string8_ex discogs_duration_raw;
		pfc::string8_ex discogs_indextrack_duration_raw;
		int discogs_duration_seconds = 0;
		int discogs_indextrack_duration_seconds = 0;
		int track_number;
		int disc_track_number;
		pfc::string8_ex discogs_track_number;
		pfc::array_t<std::shared_ptr<ReleaseTrack>> hidden_tracks;

		string_encoded_array get_track_number() const {
			return track_number;
		}
		string_encoded_array get_disc_track_number() const {
			return disc_track_number;
		}
		string_encoded_array get_discogs_track_number() const {
			return discogs_track_number;
		}
		string_encoded_array get_title() const {
			return title;
		}
		string_encoded_array get_title_heading() const {
			return title_heading;
		}
		string_encoded_array get_title_index() const {
			return title_index;
		}
		string_encoded_array get_title_subtrack() const {
			return title_subtrack;
		}
		string_encoded_array get_discogs_duration_raw() const {
			return discogs_duration_raw;
		}
		string_encoded_array get_discogs_duration_seconds() const {
			return discogs_duration_seconds;
		}
		string_encoded_array get_discogs_indextrack_duration_raw() const {
			return discogs_indextrack_duration_raw;
		}
		string_encoded_array get_discogs_indextrack_duration_seconds() const {
			return discogs_indextrack_duration_seconds;
		}
		string_encoded_array get_total_hidden_tracks() const {
			return hidden_tracks.get_size();
		}

		static std::map<const char*, string_encoded_array(ReleaseTrack::*)()const, cmp_str> create_tags_map() {
			std::map<const char*, string_encoded_array(ReleaseTrack::*)()const, cmp_str> m;
			m["NUMBER"] = &ReleaseTrack::get_track_number;
			m["DISC_TRACK_NUMBER"] = &ReleaseTrack::get_disc_track_number;
			m["DISCOGS_TRACK_NUMBER"] = &ReleaseTrack::get_discogs_track_number;
			m["TITLE"] = &ReleaseTrack::get_title;
			m["INDEXTRACK_TITLE"] = &ReleaseTrack::get_title_index;
			m["HEADING"] = &ReleaseTrack::get_title_heading;
			m["SUBTRACK_TITLE"] = &ReleaseTrack::get_title_subtrack;
			m["INDEXTRACK_DURATION_RAW"] = &ReleaseTrack::get_discogs_indextrack_duration_raw;
			m["INDEXTRACK_DURATION_SECONDS"] = &ReleaseTrack::get_discogs_indextrack_duration_seconds;
			m["DISCOGS_DURATION_RAW"] = &ReleaseTrack::get_discogs_duration_raw;
			m["DISCOGS_DURATION_SECONDS"] = &ReleaseTrack::get_discogs_duration_seconds;
			m["TOTAL_HIDDEN_TRACKS"] = &ReleaseTrack::get_total_hidden_tracks;
			return m;
		}

		virtual string_encoded_array get_sub_data(pfc::string8 &tag_name, abort_callback &p_abort) override {
			pfc::string8 sub_tag_name;
			string_encoded_array result;
			if (strncmp(tag_name.get_ptr(), "ARTISTS_", 8) == 0) {
				sub_tag_name = pfc::string8(tag_name.get_ptr() + 8);
				for (size_t i = 0; i < artists.get_size(); i++) {
					result.append_item_val(artists[i]->get_data(sub_tag_name, p_abort));
				}
			}
			else if (strncmp(tag_name.get_ptr(), "CREDITS_", 8) == 0) {
				sub_tag_name = pfc::string8(tag_name.get_ptr() + 8);
				for (size_t i = 0; i < credits.get_size(); i++) {
					result.append_item_val(credits[i]->get_data(sub_tag_name, p_abort));
				}
			}
			else if (strncmp(tag_name.get_ptr(), "HIDDEN_TRACKS_", 14) == 0) {
				sub_tag_name = pfc::string8(tag_name.get_ptr() + 14);
				for (size_t i = 0; i < hidden_tracks.get_size(); i++) {
					result.append_item_val(hidden_tracks[i]->get_data(sub_tag_name, p_abort));
				}
			}
			else {
				return ExposedTags<ReleaseTrack>::get_sub_data(tag_name, p_abort);
			}
			result.encode();
			return result;
		}

		ReleaseTrack* clone() {
			ReleaseTrack *rt = new ReleaseTrack();
			// WARNING: We are copying pointers to artists, Artist
			rt->track_number = track_number;
			rt->disc_track_number = disc_track_number; 
			rt->discogs_track_number = discogs_track_number;
			rt->title = title;
			rt->title_index = title_index;
			rt->title_subtrack = title_subtrack;
			rt->title_heading = title_heading;
			rt->discogs_duration_raw = discogs_duration_raw;
			rt->discogs_duration_seconds = discogs_duration_seconds;
			rt->artists = artists;
			rt->credits = credits;
			return rt;
		}
		
		bool has_anv() const override {
			if (HasArtists::has_anv()) {
				return true;
			}
			for (size_t i = 0; i < hidden_tracks.get_size(); i++) {
				if (hidden_tracks[i]->has_anv()) {
					return true;
				}
			}
			return false;
		}
	};
	typedef std::shared_ptr<ReleaseTrack> ReleaseTrack_ptr;


	class HasTracks
	{
	public:
		pfc::array_t<ReleaseTrack_ptr> tracks;

		inline bool has_anv() const {
			for (size_t i = 0; i < tracks.get_size(); i++) {
				if (tracks[i]->has_anv()) {
					return true;
				}
			}
			return false;
		}

		bool has_tracklengths() const {
			for (size_t i = 0; i < tracks.get_size(); i++) {
				if (tracks[i]->discogs_duration_seconds) {
					return true;
				}
			}
			return false;
		}

		bool has_track_with_artist() const {
			for (size_t i = 0; i < tracks.get_size(); i++) {
				if (tracks[i]->artists.get_size()) {
					return true;
				}
			}
			return false;
		}
	};


	class ReleaseDisc : public HasTracks, public ExposedTags<ReleaseDisc>
	{
	public:
		int disc_number;
		// TODO: disc index title
		ReleaseFormat_ptr format;  // TODO: assumed to be initialized

		string_encoded_array get_number() const {
			return disc_number;
		}
		string_encoded_array get_total_tracks() const {
			return tracks.get_size();
		}

		static std::map<const char*, string_encoded_array(ReleaseDisc::*)()const, cmp_str> create_tags_map() {
			std::map<const char*, string_encoded_array(ReleaseDisc::*)()const, cmp_str> m;
			m["NUMBER"] = &ReleaseDisc::get_number;
			m["TOTAL_TRACKS"] = &ReleaseDisc::get_total_tracks;
			return m;
		}

		virtual string_encoded_array get_sub_data(pfc::string8 &tag_name, abort_callback &p_abort) override {
			pfc::string8 sub_tag_name;
			string_encoded_array result;
			if (STR_EQUALN(tag_name, "TRACKS_", 7)) {
				sub_tag_name = substr(tag_name, 7);
				for (size_t i = 0; i < tracks.get_size(); i++) {
					result.append_item_val(tracks[i]->get_data(sub_tag_name, p_abort));
				}
			}
			else if (STR_EQUALN(tag_name, "FORMAT_", 7)) {
				sub_tag_name = substr(tag_name, 7);
				return format->get_data(sub_tag_name, p_abort);
			}
			else {
				return ExposedTags<ReleaseDisc>::get_sub_data(tag_name, p_abort);
			}
			result.encode();
			return result;
		}
	};
	typedef std::shared_ptr<ReleaseDisc> ReleaseDisc_ptr;


	class HasDiscs
	{
	public:
		pfc::array_t<ReleaseDisc_ptr> discs;

		virtual inline bool has_anv() const {
			for (size_t i = 0; i < discs.get_size(); i++) {
				if (discs[i]->has_anv()) {
					return true;
				}
			}
			return false;
		}

		bool has_tracklengths() const {
			for (size_t i = 0; i < discs.get_size(); i++) {
				if (discs[i]->has_tracklengths()) {
					return true;
				}
			}
			return false;
		}

		bool has_track_with_artist() const {
			for (size_t i = 0; i < discs.get_size(); i++) {
				if (discs[i]->has_track_with_artist()) {
					return true;
				}
			}
			return false;
		}

		size_t get_total_track_count() const {
			size_t count = 0;
			for (size_t i = 0; i < discs.get_size(); i++) {
				count += discs[i]->tracks.get_size();
			}
			return count;
		}

		bool find_disc_track(size_t i, int *disc, int *track) {
			for (size_t d = 0; d < discs.get_size(); d++) {
				if (discs[d]->tracks.get_size() <= i) {
					i -= discs[d]->tracks.get_size();
				}
				else {
					(*disc) = d;
					(*track) = i;
					return true;
				}
			}
			return false;
		}

		virtual ~HasDiscs() {}
	};


	class HasTracklist : public HasDiscs
	{
	public:
		int discogs_tracklist_count = 0;
		int total_headings;
		int discogs_total_discs = 0;
	};


	class MasterRelease : public HasImages, public HasArtists, public HasTracklist, public ExposedTags<MasterRelease>
	{
	public:
		pfc::string8_ex id;
		pfc::string8_ex title;
		pfc::string8_ex release_year;
		pfc::string8_ex main_release_id; 
		pfc::string8_ex main_release_url;
		pfc::string8_ex main_release_api_url;
		pfc::string8_ex versions_api_url;
		pfc::array_t_ex<pfc::string8> genres;
		pfc::array_t_ex<pfc::string8> styles;
		pfc::array_t_ex<pfc::string8> videos;
		pfc::string8_ex discogs_data_quality;
		pfc::string8_ex discogs_tracklist_count;
		// TODO: expose these...
		pfc::array_t<Release_ptr> sub_releases;

		MasterRelease() {}

		MasterRelease(const char *id) : id(id) {
		}

		string_encoded_array get_id() const {
			return id;
		}
		string_encoded_array get_title() const {
			return title;
		}
		string_encoded_array get_release_year() const {
			return release_year;
		}
		string_encoded_array get_main_release_id() const {
			return main_release_id;
		}
		string_encoded_array get_main_release_url() const {
			return main_release_url;
		}
		string_encoded_array get_main_release_api_url() const {
			return main_release_api_url;
		}
		string_encoded_array get_discogs_data_quality() const {
			return discogs_data_quality;
		}
		string_encoded_array get_videos() const {
			return videos;
		}
		string_encoded_array get_styles() const {
			return styles;
		}
		string_encoded_array get_genres() const {
			return genres;
		}

		static std::map<const char*, string_encoded_array(MasterRelease::*)()const, cmp_str> create_tags_map() {
			std::map<const char*, string_encoded_array(MasterRelease::*)()const, cmp_str> m;
			m["ID"] = &MasterRelease::get_id;
			m["TITLE"] = &MasterRelease::get_title;
			m["YEAR"] = &MasterRelease::get_release_year;
			m["MAIN_RELEASE_ID"] = &MasterRelease::get_main_release_id;
			m["MAIN_RELEASE_URL"] = &MasterRelease::get_main_release_url;
			m["MAIN_RELEASE_API_URL"] = &MasterRelease::get_main_release_api_url;
			m["DISCOGS_DATA_QUALITY"] = &MasterRelease::get_discogs_data_quality;
			m["GENRES"] = &MasterRelease::get_genres;
			m["STYLES"] = &MasterRelease::get_styles;
			m["VIDEOS"] = &MasterRelease::get_videos;
			return m;
		}

		virtual string_encoded_array get_sub_data(pfc::string8 &tag_name, abort_callback &p_abort) override {
			pfc::string8 sub_tag_name;
			string_encoded_array result;
			if (STR_EQUALN(tag_name, "TRACKS_", 7)) {
				for (size_t i = 0; i < discs.get_size(); i++) {
					if (i == 0) {
						result = discs[i]->get_data(tag_name, p_abort);
					}
					else {
						result.shallow_extend(discs[i]->get_data(tag_name, p_abort));
					}
				}
			}
			else if (STR_EQUALN(tag_name, "DISCS_", 6)) {
				sub_tag_name = substr(tag_name, 6);
				for (size_t i = 0; i < discs.get_size(); i++) {
					result.append_item_val(discs[i]->get_data(sub_tag_name, p_abort));
				}
			}
			else if (STR_EQUALN(tag_name, "ARTISTS_", 8)) {
				sub_tag_name = substr(tag_name, 8);
				for (size_t i = 0; i < artists.get_size(); i++) {
					result.append_item_val(artists[i]->get_data(sub_tag_name, p_abort));
				}
			}
			else if (STR_EQUALN(tag_name, "IMAGES_", 7)) {
				sub_tag_name = substr(tag_name, 7);
				for (size_t i = 0; i < images.get_size(); i++) {
					result.append_item_val(images[i]->get_data(sub_tag_name, p_abort));
				}
			}
			else {
				return ExposedTags<MasterRelease>::get_sub_data(tag_name, p_abort);
			}
			result.encode();
			return result;
		}

		bool has_anv() const override {
			return HasArtists::has_anv() || HasDiscs::has_anv();
		}
	};
	


	class Release : public HasImages, public HasArtists, public HasCredits, public HasTracklist, public ExposedTags<Release>
	{
	public:
		pfc::string8_ex id;
		pfc::string8_ex master_id;
		pfc::string8_ex title;

		pfc::array_t_ex<pfc::string8> artist_join_fields;
		pfc::array_t<ReleaseLabel_ptr> labels;
		pfc::string8_ex search_formats;
		pfc::string8_ex search_labels;
		pfc::string8_ex search_catno;
		pfc::array_t<ReleaseSeries_ptr> series;
		pfc::array_t<ReleaseFormat_ptr> formats;
		pfc::array_t_ex<pfc::string8> genres;
		pfc::array_t_ex<pfc::string8> styles;
		pfc::string8_ex country;
		pfc::string8_ex release_date;
		pfc::string8_ex release_date_raw;
		pfc::string8_ex release_year;
		pfc::string8_ex release_month;
		pfc::string8_ex release_day;
		// TODO: add my rating
		pfc::string8_ex discogs_avg_rating;
		pfc::string8_ex discogs_rating_votes;
		pfc::string8_ex notes;
		pfc::string8_ex submitted_by;
		pfc::string8_ex members_have;
		pfc::string8_ex members_want;
		pfc::string8_ex barcode;
		pfc::string8_ex weight;
		pfc::string8_ex discogs_status;
		pfc::string8_ex discogs_data_quality;
		pfc::array_t_ex<pfc::string8> videos;
		MemoryBlock small_art;

		string_encoded_array get_id() const {
			return id;
		}
		string_encoded_array get_title() const {
			return title;
		}
		string_encoded_array get_country() const {
			return country;
		}
		string_encoded_array get_release_date() const {
			return release_date;
		}
		string_encoded_array get_release_date_raw() const {
			return release_date_raw;
		}
		string_encoded_array get_release_year() const {
			return release_year;
		}
		string_encoded_array get_release_month() const {
			return release_month;
		}
		string_encoded_array get_release_day() const {
			return release_day;
		}
		string_encoded_array get_notes() const {
			return notes;
		}
		string_encoded_array get_barcode() const {
			return barcode;
		}
		string_encoded_array get_weight() const {
			return weight;
		}
		string_encoded_array get_total_discs() const {
			return discs.get_size();
		}
		string_encoded_array get_total_tracks() const {
			return get_total_track_count();
		}
		string_encoded_array get_members_have() const {
			return members_have;
		}
		string_encoded_array get_members_want() const {
			return members_want;
		}
		string_encoded_array get_avg_rating() const {
			return discogs_avg_rating;
		}
		string_encoded_array get_rating_votes() const {
			return discogs_rating_votes;
		}
		string_encoded_array get_discogs_status() const {
			return discogs_status;
		}
		string_encoded_array get_discogs_data_quality() const {
			return discogs_data_quality;
		}
		string_encoded_array get_submitted_by() const {
			return submitted_by;
		}
		string_encoded_array get_discogs_total_discs() const {
			return discogs_total_discs;
		}
		string_encoded_array get_genres() const {
			return genres;
		}
		string_encoded_array get_styles() const {
			return styles;
		}
		string_encoded_array get_videos() const {
			return videos;
		}
		string_encoded_array get_search_formats() const {
			return search_formats;
		}
		string_encoded_array get_search_labels() const {
			return search_labels;
		}
		string_encoded_array get_search_catno() const {
			return search_catno;
		}

		static std::map<const char*, string_encoded_array(Release::*)()const, cmp_str> create_tags_map() {
			std::map<const char*, string_encoded_array(Release::*)()const, cmp_str> m;
			m["ID"] = &Release::get_id;
			m["TITLE"] = &Release::get_title;
			m["COUNTRY"] = &Release::get_country;
			m["DATE"] = &Release::get_release_date;
			m["DATE_RAW"] = &Release::get_release_date_raw;
			m["YEAR"] = &Release::get_release_year;
			m["MONTH"] = &Release::get_release_month;
			m["DAY"] = &Release::get_release_day;
			m["NOTES"] = &Release::get_notes;
			m["BARCODE"] = &Release::get_barcode;
			m["WEIGHT"] = &Release::get_weight;
			m["TOTAL_DISCS"] = &Release::get_total_discs;
			m["TOTAL_TRACKS"] = &Release::get_total_tracks;
			m["DISCOGS_USERS_HAVE"] = &Release::get_members_have;
			m["DISCOGS_USERS_WANT"] = &Release::get_members_want;
			m["DISCOGS_AVG_RATING"] = &Release::get_avg_rating;
			m["DISCOGS_RATING_VOTES"] = &Release::get_rating_votes;
			m["DISCOGS_STATUS"] = &Release::get_discogs_status;
			m["DISCOGS_DATA_QUALITY"] = &Release::get_discogs_data_quality;
			m["DISCOGS_SUBMITTED_BY"] = &Release::get_submitted_by;
			//m["DISCOGS_TOTAL_DISCS"] = &Release::get_discogs_total_discs;  -- technically format_quantity...? :-s
			m["GENRES"] = &Release::get_genres;
			m["STYLES"] = &Release::get_styles;
			m["VIDEOS"] = &Release::get_videos;
			m["SEARCH_FORMATS"] = &Release::get_search_formats;
			m["SEARCH_LABELS"] = &Release::get_search_labels;
			m["SEARCH_CATNOS"] = &Release::get_search_catno;
			return m;
		}

		virtual string_encoded_array get_sub_data(pfc::string8 &tag_name, abort_callback &p_abort) override {
			pfc::string8 sub_tag_name;
			string_encoded_array result;
			if (STR_EQUALN(tag_name, "TRACKS_", 7)) {
				for (size_t i = 0; i < discs.get_size(); i++) {
					if (i == 0) {
						result = discs[i]->get_data(tag_name, p_abort);
					}
					else {
						result.shallow_extend(discs[i]->get_data(tag_name, p_abort));
					}
				}
			}
			else if (STR_EQUALN(tag_name, "DISCS_", 6)) {
				sub_tag_name = substr(tag_name, 6);
				for (size_t i = 0; i < discs.get_size(); i++) {
					result.append_item_val(discs[i]->get_data(sub_tag_name, p_abort));
				}
			}
			else if (STR_EQUALN(tag_name, "ARTISTS_", 8)) {
				sub_tag_name = substr(tag_name, 8);
				for (size_t i = 0; i < artists.get_size(); i++) {
					result.append_item_val(artists[i]->get_data(sub_tag_name, p_abort));
				}
			}
			else if (STR_EQUALN(tag_name, "LABELS_", 7)) {
				sub_tag_name = substr(tag_name, 7);
				for (size_t i = 0; i < labels.get_size(); i++) {
					result.append_item_val(labels[i]->get_data(sub_tag_name, p_abort));
				}
			}
			else if (STR_EQUALN(tag_name, "SERIES_", 7)) {
				sub_tag_name = substr(tag_name, 7);
				for (size_t i = 0; i < series.get_size(); i++) {
					result.append_item_val(series[i]->get_data(sub_tag_name, p_abort));
				}
			}
			else if (STR_EQUALN(tag_name, "FORMATS_", 8)) {
				sub_tag_name = substr(tag_name, 8);
				for (size_t i = 0; i < formats.get_size(); i++) {
					result.append_item_val(formats[i]->get_data(sub_tag_name, p_abort));
				}
			}
			else if (STR_EQUALN(tag_name, "CREDITS_", 8)) {
				sub_tag_name = substr(tag_name, 8);
				for (size_t i = 0; i < credits.get_size(); i++) {
					result.append_item_val(credits[i]->get_data(sub_tag_name, p_abort));
				}
			}
			else if (STR_EQUALN(tag_name, "IMAGES_", 7)) {
				sub_tag_name = substr(tag_name, 7);
				for (size_t i = 0; i < images.get_size(); i++) {
					result.append_item_val(images[i]->get_data(sub_tag_name, p_abort));
				}
			}
			else {
				return ExposedTags<Release>::get_sub_data(tag_name, p_abort);
			}
			result.encode();
			return result;
		}

		Release(const char* id) : id(id) {}

		bool has_anv() const override {
			return HasArtists::has_anv() || HasDiscs::has_anv();
		}

		bool has_multiple_artists() const {
			return artists.get_size() > 1;
		}
	};

	
	class Identity
	{
	public:
		pfc::string8 user_id;
		pfc::string8 username;
		pfc::string8 consumer_name;
	};


	void appendCredit(pfc::string8 &dst, const pfc::string8 &src);

	extern void parseIdentity(json_t *root, Identity *identity);
	extern void parseCollection(json_t *root, pfc::array_t<pfc::string8> &collection);

	extern void parseRelease(Release *release, json_t *root);
	extern void parseMasterRelease(MasterRelease *master_release, json_t *root);
	extern void parseArtist(Artist *artist, json_t *root);

	extern void parseArtistResults(json_t *root, pfc::array_t<Artist_ptr> &artists);

	extern void parseImages(json_t *array, pfc::array_t<Image_ptr> &images);
	extern Image_ptr parseImage(json_t *element);

	extern void parseArtistReleases(json_t *element, Artist *artist);
	extern void parseMasterVersions(json_t *element, MasterRelease_ptr &master_release);

	extern ReleaseArtist_ptr parseReleaseArtist(json_t *element);
	extern void parseReleaseArtists(json_t *element, pfc::array_t<ReleaseArtist_ptr> &artists);

	extern ReleaseCredit_ptr parseReleaseCredit(json_t *element);
	extern void parseReleaseCredits(json_t *element, pfc::array_t<ReleaseCredit_ptr> &credits, Release *release);
	extern void addReleaseTrackCredits(const pfc::string8 &tracks, ReleaseCredit_ptr &credit, Release *release);

	extern ReleaseLabel_ptr parseReleaseLabel(json_t *element);
	extern void parseReleaseLabels(json_t *element, pfc::array_t<ReleaseLabel_ptr> &release_labels);

	extern ReleaseSeries_ptr parseReleaseSeries(json_t *element);
	extern void parseReleaseSeries(json_t *element, pfc::array_t<ReleaseSeries_ptr> &release_series);

	extern ReleaseFormat_ptr parseReleaseFormat(json_t *element);
	extern void parseReleaseFormats(json_t *element, pfc::array_t<ReleaseFormat_ptr> &formats);

	extern void parseReleaseTrack(json_t *element, pfc::array_t<ReleaseTrack_ptr> &tracks, unsigned &discogs_original_track_number, pfc::string8 heading = "", ReleaseTrack_ptr *index = nullptr, HasArtists *has_artists = nullptr);
	extern void parseReleaseTracks(json_t *element, HasTracklist *has_tracklist, HasArtists *has_artists);

	extern int get_track_count(json_t *element);
}

#include "foo_discogs.h"
