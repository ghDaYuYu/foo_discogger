#pragma once

#include "exception.h"
#include "utils.h"
#include "conf.h"
#include "jansson.h"
#include "db_fetcher.h"
#include "string_encoded_array.h"
#include "exposetags.h"

#include <map>
#include <functional>

namespace Discogs
{
	class missing_data_exception : public foo_discogs_exception	{};

	extern pfc::string8 remove_number_suffix(const pfc::string8 &src);
	extern pfc::string8 move_the_to_start(const pfc::string8 &src);
	extern pfc::string8 move_the_to_end(const pfc::string8 &src);
	extern pfc::string8 strip_artist_name(const pfc::string8 str);
	extern pfc::string8 format_track_number(int tracknumber);

	class ExpTagsImage : public ExposedTags<ExpTagsImage>
	{
	public:
		pfc::string8 type; //primary or secondary
		pfc::string8 url;
		pfc::string8 url150;
		pfc::string8 width;
		pfc::string8 height;

		string_encoded_array get_type() const {
			return type;
		}
		string_encoded_array get_url() const {
			return url;
		}
		string_encoded_array get_thumbnail_url() const {
			return url150;
		}
		string_encoded_array get_width() const {
			return width;
		}
		string_encoded_array get_height() const {
			return height;
		}

		static ExposedMap<ExpTagsImage> create_tags_map() {
			ExposedMap<ExpTagsImage> m;
			m["TYPE"] = { &ExpTagsImage::get_type, &ExpTagsImage::load };
			m["URL"] = { &ExpTagsImage::get_url, &ExpTagsImage::load };
			m["THUMBNAIL_URL"] = { &ExpTagsImage::get_thumbnail_url, &ExpTagsImage::load };
			m["WIDTH"] = { &ExpTagsImage::get_width, &ExpTagsImage::load };
			m["HEIGHT"] = { &ExpTagsImage::get_height, &ExpTagsImage::load };
			return m;
		}
	};
	typedef std::shared_ptr<ExpTagsImage> Image_ptr;

	class HasImages
	{
	public:

		pfc::array_t<Image_ptr> images;

		~HasImages() {

			for (auto i : images) {

				i.reset();

			}
		}
	};


	class HasRoles
	{
	public:
		pfc::string8 raw_roles;
		pfc::array_t<pfc::string8> roles;
		pfc::array_t<pfc::string8> full_roles;
		pfc::array_t<pfc::string8> cat_roles;

		string_encoded_array get_raw_roles() const {
			return raw_roles;
		}
		string_encoded_array get_roles() const {
			return full_roles;
		}
		string_encoded_array get_cat_roles() const {
			return cat_roles;
		}
		string_encoded_array get_roles_short() const {
			return roles;
		}
	};


	class Release;
	typedef std::shared_ptr<Release> Release_ptr;

	class MasterRelease;
	typedef std::shared_ptr<MasterRelease> MasterRelease_ptr;

	class Artist : public HasImages, public ExposedTags<Artist>
	{
	public:
		pfc::string8 id;
		pfc::string8 name;
		pfc::string8 profile;
		pfc::string8 realname;
		pfc::array_t<MasterRelease_ptr> master_releases;
		pfc::array_t<bool> search_order_master;
		pfc::array_t<pfc::string8> urls;
		pfc::array_t<pfc::string8> aliases;
		pfc::array_t<pfc::string8> ingroups;
		pfc::array_t<pfc::string8> anvs;
		pfc::array_t<pfc::string8> members;
		pfc::array_t<Release_ptr> releases;

		bool loaded_preview = false;
		bool loaded_releases = false;
		bool loaded_releases_offline = false;
		size_t search_role_list_pos = 0;

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

		static ExposedMap<Artist> create_tags_map() {
			ExposedMap<Artist> m;
			m["ID"] = { &Artist::get_id, nullptr };
			m["NAME"] = { &Artist::get_name, &Artist::load };
			m["PROFILE"] = { &Artist::get_profile, &Artist::load };
			m["REAL_NAME"] = { &Artist::get_realname, &Artist::load };
			m["URLS"] = { &Artist::get_urls, &Artist::load };
			m["ALIASES"] = { &Artist::get_aliases, &Artist::load };
			m["GROUPS"] = { &Artist::get_ingroups, &Artist::load };
			m["ALL_NAME_VARIATIONS"] = { &Artist::get_anvs, &Artist::load };
			m["MEMBERS"] = { &Artist::get_members, &Artist::load };
			return m;
		}

		virtual string_encoded_array get_sub_data(pfc::string8 &tag_name, threaded_process_status &p_status, abort_callback &p_abort) override;

		Artist(const char *id) : id(id) {
		}

		void load(threaded_process_status &p_status, abort_callback &p_abort, bool throw_all = false) override;
		void load_releases(threaded_process_status &p_status, abort_callback &p_abort, bool throw_all, db_fetcher* dbfetcher);
	};
	typedef std::shared_ptr<Artist> Artist_ptr;

	// todo: possible to merge artist and release artist, using the lazy loading stuff?

	class ReleaseArtist : public HasRoles, public ExposedTags<ReleaseArtist>
	{
	public:
		Artist_ptr full_artist;
		pfc::string8 id;
		pfc::string8 name;
		pfc::string8 anv;
		pfc::string8 join;

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

		static ExposedMap<ReleaseArtist> create_tags_map() {
			ExposedMap<ReleaseArtist> m;
			m["ID"] = { &ReleaseArtist::get_id, nullptr };
			m["NAME"] = { &ReleaseArtist::get_name, &ReleaseArtist::load };
			m["NAME_VARIATION"] = { &ReleaseArtist::get_anv, &ReleaseArtist::load };
			m["JOIN"] = { &ReleaseArtist::get_join, &ReleaseArtist::load };
			return m;
		}
		//..
		void load(threaded_process_status& p_status, abort_callback& p_abort, bool throw_all = false) override;
		//..
		string_encoded_array get_data(pfc::string8 &tag_name, threaded_process_status &p_status, abort_callback &p_abort) override;

		ReleaseArtist(const char *id, Artist_ptr &full_artist, bool preload = false) : full_artist(full_artist), id(id), preload(preload) {
		}

		bool has_anv() const {
			return anv.get_length() != 0;
		}

	private:
		bool preload = false;
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
		static ExposedMap<ReleaseCredit> create_tags_map() {
			ExposedMap<ReleaseCredit> m;
			m["ROLES_RAW"] = { &ReleaseCredit::get_raw_roles, &ReleaseCredit::load };
			m["ROLES"] = { &ReleaseCredit::get_roles, &ReleaseCredit::load };
			m["SHORT_ROLES"] = { &ReleaseCredit::get_roles_short, &ReleaseCredit::load };
			return m;
		}
		virtual string_encoded_array get_sub_data(pfc::string8 &tag_name, threaded_process_status &p_status, abort_callback &p_abort) override;
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

	typedef std::pair<std::pair<pfc::string8, pfc::string8>, std::pair<pfc::string8, pfc::string8>> rppair;
	typedef std::pair<std::pair<size_t, size_t>, std::pair<size_t, size_t>> rppair_t;
	typedef std::vector <rppair> vppair;

	class HasCatCredits
	{
	public:
		std::map<pfc::string8, vppair> catcredits_gxp;
		std::map<pfc::string8, vppair> catcredits_gxc;

		virtual ~HasCatCredits() {}
	};


	class ReleaseLabel : public ExposedTags<ReleaseLabel>
	{
	public:
		pfc::string8 id;
		pfc::string8 name;
		pfc::string8 catalog;

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

		static ExposedMap<ReleaseLabel> create_tags_map() {
			ExposedMap<ReleaseLabel> m;
			m["ID"] = { &ReleaseLabel::get_id, nullptr };
			m["NAME"] = { &ReleaseLabel::get_name, &ReleaseLabel::load };
			m["CATALOG_NUMBER"] = { &ReleaseLabel::get_catalog, &ReleaseLabel::load };
			return m;
		}
	};
	typedef std::shared_ptr<ReleaseLabel> ReleaseLabel_ptr;


	class ReleaseCompany : public ExposedTags<ReleaseCompany>
	{
	public:
		pfc::string8 id;
		pfc::string8 name;
		pfc::string8 catalog;
		pfc::string8 entity_type_name;

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
		string_encoded_array get_entity_type_name() const {
			return entity_type_name;
		}

		static ExposedMap<ReleaseCompany> create_tags_map() {
			ExposedMap<ReleaseCompany> m;
			m["ID"] = { &ReleaseCompany::get_id, nullptr };
			m["NAME"] = { &ReleaseCompany::get_name, &ReleaseCompany::load };
			m["CATALOG_NUMBER"] = { &ReleaseCompany::get_catalog, &ReleaseCompany::load };
			m["ENTITY_TYPE_NAME"] = { &ReleaseCompany::get_entity_type_name, &ReleaseCompany::load };
			return m;
		}
	};
	typedef std::shared_ptr<ReleaseCompany> ReleaseCompany_ptr;


	class ReleaseSeries : public ExposedTags<ReleaseSeries>
	{
	public:
		pfc::string8 id;
		pfc::string8 name;
		pfc::string8 api_url;
		pfc::string8 number;

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

		static ExposedMap<ReleaseSeries> create_tags_map() {
			ExposedMap<ReleaseSeries> m;
			m["ID"] = { &ReleaseSeries::get_id, nullptr };
			m["NAME"] = { &ReleaseSeries::get_name, &ReleaseSeries::load };
			m["NUMBER"] = { &ReleaseSeries::get_number, &ReleaseSeries::load };
			m["API_URL"] = { &ReleaseSeries::get_api_url, &ReleaseSeries::load };
			return m;
		}
	};
	typedef std::shared_ptr<ReleaseSeries> ReleaseSeries_ptr;


	class ReleaseFormat : public ExposedTags<ReleaseFormat>
	{
	public:

		pfc::string8 qty;
		pfc::array_t<pfc::string8> descriptions;
		pfc::string8 text;

		bool format_incl_vol = false;

		bool is_box() const {
			return name.equals("Box Set") || name.equals("All Media");
		}
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

		void set_name(pfc::string8 aname) {
			name = aname;
			format_incl_vol = name.equals("Vinyl") || name.equals("Cassette");
		}

		static ExposedMap<ReleaseFormat> create_tags_map() {
			ExposedMap<ReleaseFormat> m;
			m["QUANTITY"] = { &ReleaseFormat::get_quantity, &ReleaseFormat::load };
			m["NAME"] = { &ReleaseFormat::get_name, &ReleaseFormat::load };
			m["DESCRIPTIONS"] = { &ReleaseFormat::get_descriptions, &ReleaseFormat::load };
			m["TEXT"] = { &ReleaseFormat::get_text, &ReleaseFormat::load };
			return m;
		}

	private:
		pfc::string8 name;
	};
	typedef std::shared_ptr<ReleaseFormat> ReleaseFormat_ptr;

	// heading

	class ReleaseHeading : public ExposedTags<ReleaseHeading>
	{
	public:
		pfc::string8 position;
		pfc::string8 title;
		pfc::string8 duration;

		pfc::string8 dc_track_first;
		pfc::string8 dc_track_last;
		bool dc_track_closed = false;

		string_encoded_array get_position() const {
			return position;
		}
		string_encoded_array get_title() const {		
			return title;
		}
		string_encoded_array get_duration() const {
			return duration;
		}

		static ExposedMap<ReleaseHeading> create_tags_map() {
			ExposedMap<ReleaseHeading> m;
			m["POSITION"] = { &ReleaseHeading::get_position,  &ReleaseHeading::load };
			m["TITLE"] = { &ReleaseHeading::get_title, &ReleaseHeading::load };
			m["DURATION"] = { &ReleaseHeading::get_duration, &ReleaseHeading::load };
			return m;
		}
	};

	typedef std::shared_ptr<ReleaseHeading> ReleaseHeading_ptr;

	class HasHeadings
	{
	public:
		pfc::array_t<ReleaseHeading_ptr> headings;

		virtual ~HasHeadings() {}
	};

	class ReleaseIndexes : public ExposedTags<ReleaseIndexes>
	{
	public:
		pfc::string8 position;
		pfc::string8 title;
		pfc::string8 duration;

		pfc::string8 dc_track_first;
		pfc::string8 dc_track_last;
		bool dc_track_closed = false;

		string_encoded_array get_position() const {
			return position;
		}
		string_encoded_array get_title() const {
			return title;
		}
		string_encoded_array get_duration() const {
			return duration;
		}

		static ExposedMap<ReleaseIndexes> create_tags_map() {
			ExposedMap<ReleaseIndexes> m;
			m["POSITION"] = { &ReleaseIndexes::get_position,  &ReleaseIndexes::load };
			m["TITLE"] = { &ReleaseIndexes::get_title, &ReleaseIndexes::load };
			m["DURATION"] = { &ReleaseIndexes::get_duration, &ReleaseIndexes::load };
			return m;
		}
	};

	typedef std::shared_ptr<ReleaseIndexes> ReleaseIndexes_ptr;

	class HasIndexes
	{
	public:
		pfc::array_t<ReleaseIndexes_ptr> indexes;

		virtual ~HasIndexes() {}
	};

	class ReleaseTrack : public HasArtists, public HasCredits, public HasCatCredits, public ExposedTags<ReleaseTrack>
	{
	public:
		pfc::string8 title;
		pfc::string8 title_index;
		pfc::string8 title_subtrack;
		pfc::string8 title_heading;
		ReleaseHeading_ptr heading_ptr;
		ReleaseIndexes_ptr index_ptr;
		pfc::string8 discogs_duration_raw;
		pfc::string8 discogs_indextrack_duration_raw;
		int discogs_duration_seconds = 0;
		int discogs_indextrack_duration_seconds = 0;
		int track_number = 0;
		int disc_track_number = 0;
		bool disc_track_side = false;
		pfc::string8 discogs_track_number;
		pfc::array_t<std::shared_ptr<ReleaseTrack>> hidden_tracks;
		int discogs_hidden_duration_seconds = 0;

		string_encoded_array get_track_number() const {
			return track_number;
		}
		string_encoded_array get_disc_track_number() const {
			return disc_track_number;
		}
		string_encoded_array get_disc_track_side() const {
			return disc_track_side;
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
		//--heading_ptr
		ReleaseHeading_ptr get_title_heading_ptr() const {
			return heading_ptr;
		}
		//--
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
			return discogs_duration_seconds + discogs_hidden_duration_seconds;
		}
		string_encoded_array get_discogs_indextrack_duration_raw() const {
			return discogs_indextrack_duration_raw;
		}
		string_encoded_array get_discogs_indextrack_duration_seconds() const {
			return discogs_indextrack_duration_seconds;
		}
		string_encoded_array get_total_hidden_tracks() const {
			return (unsigned)hidden_tracks.get_size();
		}

		static ExposedMap<ReleaseTrack> create_tags_map() {
			ExposedMap<ReleaseTrack> m;
			m["NUMBER"] = { &ReleaseTrack::get_track_number, &ReleaseTrack::load };
			m["DISC_TRACK_NUMBER"] = { &ReleaseTrack::get_disc_track_number, &ReleaseTrack::load };
			m["DISC_TRACK_SIDE"] = { &ReleaseTrack::get_disc_track_side, &ReleaseTrack::load };
			m["DISCOGS_TRACK_NUMBER"] = { &ReleaseTrack::get_discogs_track_number, &ReleaseTrack::load };
			m["TITLE"] = { &ReleaseTrack::get_title, &ReleaseTrack::load };
			m["INDEXTRACK_TITLE"] = { &ReleaseTrack::get_title_index, &ReleaseTrack::load };
			m["HEADING"] = { &ReleaseTrack::get_title_heading, &ReleaseTrack::load };
			m["SUBTRACK_TITLE"] = { &ReleaseTrack::get_title_subtrack, &ReleaseTrack::load };
			m["INDEXTRACK_DURATION_RAW"] = { &ReleaseTrack::get_discogs_indextrack_duration_raw, &ReleaseTrack::load };
			m["INDEXTRACK_DURATION_SECONDS"] = { &ReleaseTrack::get_discogs_indextrack_duration_seconds, &ReleaseTrack::load };
			m["DISCOGS_DURATION_RAW"] = { &ReleaseTrack::get_discogs_duration_raw, &ReleaseTrack::load };
			m["DISCOGS_DURATION_SECONDS"] = { &ReleaseTrack::get_discogs_duration_seconds, &ReleaseTrack::load };
			m["TOTAL_HIDDEN_TRACKS"] = { &ReleaseTrack::get_total_hidden_tracks, &ReleaseTrack::load };
			return m;
		}

		virtual string_encoded_array get_sub_data(pfc::string8 &tag_name, threaded_process_status &p_status, abort_callback &p_abort) override;

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
			rt->heading_ptr = heading_ptr;
			rt->index_ptr = index_ptr;
			rt->discogs_duration_raw = discogs_duration_raw;
			rt->discogs_duration_seconds = discogs_duration_seconds;
			rt->track_number = track_number;
			rt->disc_track_number = disc_track_number; 
			rt->disc_track_side = disc_track_side; 
			rt->discogs_track_number = discogs_track_number;
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

		bool find_std_disc_track(const pfc::string8& sometrack, int* track) {
			for (size_t i = 0; i < tracks.get_size(); i++) {
				if (tracks[i]->discogs_track_number.equals(sometrack)) {
					*track = i;
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
		pfc::string8 format;

		string_encoded_array get_number() const {
			return disc_number;
		}
		string_encoded_array get_total_tracks() const {
			return (unsigned)tracks.get_size();
		}
		string_encoded_array get_format() const {
			return format;
		}

		static ExposedMap<ReleaseDisc> create_tags_map() {
			ExposedMap<ReleaseDisc> m;
			m["NUMBER"] = { &ReleaseDisc::get_number, nullptr };
			m["TOTAL_TRACKS"] = { &ReleaseDisc::get_total_tracks, nullptr };
			m["FORMAT"] = { &ReleaseDisc::get_format, nullptr };
			return m;
		}

		virtual string_encoded_array get_sub_data(pfc::string8 &tag_name, threaded_process_status &p_status, abort_callback &p_abort) override;
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

		size_t get_all_tracks_csv(pfc::string8& alltracks) {
			size_t count = 0;
			for (size_t i = 0; i < discs.get_size(); i++) {
				count += discs[i]->tracks.get_size();
				for (size_t j = 0; j < discs[i]->tracks.get_size(); j++) {
					alltracks << ( i || j  ? "," : "") << discs[i]->tracks[j]->discogs_track_number;
				}
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

		bool find_std_disc_track(const pfc::string8& sometrack, int* disc, int* track) {
			for (size_t d = 0; d < discs.get_size(); d++) {
				if (discs[d]->find_std_disc_track(sometrack, track)) {
					*disc = d;
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
		int total_headings = 0;
		int discogs_total_discs = 0;
	};


	class MasterRelease : public HasImages, public HasArtists, public HasTracklist, public HasHeadings, public HasIndexes, public ExposedTags<MasterRelease>
	{
	public:
		pfc::string8 id;
		pfc::string8 title;
		pfc::string8 release_year;

		Release_ptr main_release;
		pfc::string8 main_release_id;
		pfc::string8 main_release_url;
		pfc::string8 main_release_api_url;

		pfc::string8 most_recent_release_id;
		pfc::string8 most_recent_release_url;

		pfc::string8 versions_api_url;
		pfc::array_t<pfc::string8> genres;
		pfc::array_t<pfc::string8> styles;
		pfc::array_t<pfc::string8> videos;
		pfc::string8 discogs_data_quality;
		pfc::string8 discogs_tracklist_count;
		pfc::array_t<Release_ptr> sub_releases;

		pfc::array_t<pfc::string8> search_roles;
		pfc::string8 search_role;

		bool loaded_preview = false;
		bool loaded_releases = false;

		inline void set_main_release(Release_ptr main);

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
		string_encoded_array get_most_recent_release_id() const {
			return most_recent_release_id;
		}
		string_encoded_array get_most_recent_release_url() const {
			return most_recent_release_url;
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
		string_encoded_array get_search_role() const {
			return search_role;
		}
		string_encoded_array get_search_roles() const {
			return search_roles;
		}

		static ExposedMap<MasterRelease> create_tags_map() {
			ExposedMap<MasterRelease> m;
			m["ID"] = { &MasterRelease::get_id, nullptr };
			m["TITLE"] = { &MasterRelease::get_title, &MasterRelease::load_preview };
			m["YEAR"] = { &MasterRelease::get_release_year, &MasterRelease::load_preview };
			m["MAIN_RELEASE_ID"] = { &MasterRelease::get_main_release_id, &MasterRelease::load_preview };
			m["MAIN_RELEASE_URL"] = { &MasterRelease::get_main_release_url, &MasterRelease::load };
			m["MAIN_RELEASE_API_URL"] = { &MasterRelease::get_main_release_api_url, &MasterRelease::load };
			m["MOST_RECENT_RELEASE_ID"] = { &MasterRelease::get_most_recent_release_id, &MasterRelease::load_preview };
			m["MOST_RECENT_RELEASE_URL"] = { &MasterRelease::get_most_recent_release_url, &MasterRelease::load };
			m["DISCOGS_DATA_QUALITY"] = { &MasterRelease::get_discogs_data_quality, &MasterRelease::load };
			m["GENRES"] = { &MasterRelease::get_genres, &MasterRelease::load };
			m["STYLES"] = { &MasterRelease::get_styles, &MasterRelease::load };
			m["VIDEOS"] = { &MasterRelease::get_videos, &MasterRelease::load };
			m["SEARCH_ROLE"] = { &MasterRelease::get_search_role, &MasterRelease::load_preview };
			m["SEARCH_ROLES"] = { &MasterRelease::get_search_roles, &MasterRelease::load_preview };
			return m;
		}

		virtual string_encoded_array get_sub_data(pfc::string8 &tag_name, threaded_process_status &p_status, abort_callback &p_abort) override;

		bool has_anv() const override {
			return HasArtists::has_anv() || HasDiscs::has_anv();
		}

		inline void load_preview(threaded_process_status &p_status, abort_callback &p_abort, bool throw_all = false) {
			if (loaded || loaded_preview) {
				return;
			}
			// No point just loading the preview, might as well load everything
			load(p_status, p_abort, throw_all);
		}
		void load(threaded_process_status &p_status, abort_callback &p_abort, bool throw_all = false) override;
		void load_releases(threaded_process_status &p_status, abort_callback &p_abort, bool throw_all = false, pfc::string8 offlineArtistId = "", db_fetcher* dbfetcher = nullptr);
	};


	class Release : public HasImages, public HasArtists, public HasCredits, public HasCatCredits, public HasTracklist, public HasHeadings, public HasIndexes, public ExposedTags<Release>
	{
	public:
		pfc::string8 id;
		pfc::string8 title;

		pfc::string8 master_id;
		MasterRelease_ptr master_release = nullptr;
		pfc::array_t<pfc::string8> artist_join_fields;
		pfc::array_t<ReleaseLabel_ptr> labels;
		pfc::array_t<ReleaseCompany_ptr> companies;
		pfc::array_t<pfc::string8> search_major_formats;
		pfc::string8 search_formats;
		pfc::string8 search_labels;
		pfc::string8 search_catno;
		pfc::string8 search_role;
		pfc::array_t<pfc::string8> search_roles;

		pfc::array_t<ReleaseSeries_ptr> series;
		pfc::array_t<ReleaseFormat_ptr> formats;
		pfc::array_t<pfc::string8> genres;
		pfc::array_t<pfc::string8> styles;
		pfc::string8 country;
		pfc::string8 release_date;
		pfc::string8 release_date_raw;
		pfc::string8 release_year;
		pfc::string8 release_month;
		pfc::string8 release_day;
		pfc::string8 discogs_avg_rating;
		pfc::string8 discogs_rating_votes;
		pfc::string8 discogs_my_rating;
		pfc::string8 notes;
		pfc::string8 submitted_by;
		pfc::string8 date_added;
		pfc::string8 date_changed;
		pfc::string8 members_have;
		pfc::string8 members_want;
		pfc::string8 barcode;
		pfc::string8 weight;
		pfc::string8 discogs_status;
		pfc::string8 discogs_data_quality;
		pfc::array_t<pfc::string8> videos;
		MemoryBlock small_art;
		
		bool loaded_preview = false;
		bool loaded_my_rating = false;
		size_t db_total_tracks = 0;

		inline void set_master_release(MasterRelease_ptr master);

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
			return (unsigned)discs.get_size();
		}
		string_encoded_array get_total_tracks() const {
			return (unsigned)get_total_track_count();
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
		string_encoded_array get_my_rating() const {
			return discogs_my_rating;
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
		string_encoded_array get_date_added() const {
			return date_added;
		}
		string_encoded_array get_date_changed() const {
			return date_changed;
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
		string_encoded_array get_search_major_formats() const {
			return search_major_formats;
		}
		string_encoded_array get_search_formats() const {
			return search_formats;
		}
		string_encoded_array get_search_labels() const {
			if (CONF.discard_numeric_suffix) {
				pfc::string8 result = search_labels; 
				result = remove_number_suffix(result);
			}
			return search_labels;
		}
		string_encoded_array get_search_catno() const {
			return search_catno;
		}

		string_encoded_array get_search_role() const {
			return search_role;
		}

		string_encoded_array get_search_roles() const {
			return search_roles;
		}

		static ExposedMap<Release> create_tags_map() {
			ExposedMap<Release> m;
			m["ID"] = { &Release::get_id, nullptr };
			m["TITLE"] = { &Release::get_title, &Release::load_preview };
			m["COUNTRY"] = { &Release::get_country, &Release::load_preview };
			m["DATE"] = { &Release::get_release_date, &Release::load };
			m["DATE_RAW"] = { &Release::get_release_date_raw, &Release::load };
			m["YEAR"] = { &Release::get_release_year, &Release::load_preview };
			m["MONTH"] = { &Release::get_release_month, &Release::load };
			m["DAY"] = { &Release::get_release_day, &Release::load };
			m["NOTES"] = { &Release::get_notes, &Release::load };
			m["BARCODE"] = { &Release::get_barcode, &Release::load };
			m["WEIGHT"] = { &Release::get_weight, &Release::load };
			m["TOTAL_DISCS"] = { &Release::get_total_discs, &Release::load };
			m["TOTAL_TRACKS"] = { &Release::get_total_tracks, &Release::load };
			m["DISCOGS_USERS_HAVE"] = { &Release::get_members_have, &Release::load };
			m["DISCOGS_USERS_WANT"] = { &Release::get_members_want, &Release::load };
			m["DISCOGS_AVG_RATING"] = { &Release::get_avg_rating, &Release::load };
			m["DISCOGS_MY_RATING"] = { &Release::get_my_rating, &Release::load_my_rating };
			m["DISCOGS_RATING_VOTES"] = { &Release::get_rating_votes, &Release::load };
			m["DISCOGS_STATUS"] = { &Release::get_discogs_status, &Release::load };
			m["DISCOGS_DATA_QUALITY"] = { &Release::get_discogs_data_quality, &Release::load };
			m["DISCOGS_SUBMITTED_BY"] = { &Release::get_submitted_by, &Release::load };
			m["DISCOGS_DATE_ADDED"] = { &Release::get_date_added, &Release::load };
			m["DISCOGS_DATE_CHANGED"] = { &Release::get_date_changed, &Release::load };
			m["GENRES"] = { &Release::get_genres, &Release::load };
			m["STYLES"] = { &Release::get_styles, &Release::load };
			m["VIDEOS"] = { &Release::get_videos, &Release::load };
			m["SEARCH_MAJOR_FORMATS"] = { &Release::get_search_major_formats, &Release::load_preview };
			m["SEARCH_FORMATS"] = { &Release::get_search_formats, &Release::load_preview };
			m["SEARCH_LABELS"] = { &Release::get_search_labels, &Release::load_preview };
			m["SEARCH_CATNOS"] = { &Release::get_search_catno, &Release::load_preview };
			m["SEARCH_ROLE"] = { &Release::get_search_role, &Release::load_preview };
			m["SEARCH_ROLES"] = { &Release::get_search_roles, &Release::load_preview };
			return m;
		}

		virtual string_encoded_array get_sub_data(pfc::string8 &tag_name, threaded_process_status &p_status, abort_callback &p_abort) override;

		Release(const char* id) : id(id) {}

		bool has_anv() const override {
			return HasArtists::has_anv() || HasDiscs::has_anv();
		}

		bool has_multiple_artists() const {
			return artists.get_size() > 1;
		}

		inline void load_preview(threaded_process_status &p_status, abort_callback &p_abort, bool throw_all = false) {
			if (loaded || loaded_preview) {
				return;
			}
			// No point just loading the preview, might as well load everything
			load(p_status, p_abort, throw_all);
		}
		void load(threaded_process_status &p_status, abort_callback &p_abort, bool throw_all = false); //) override;
		void load(threaded_process_status &p_status, abort_callback &p_abort, bool throw_all, pfc::string8 offlineArtistId, db_fetcher* dbfetcher = nullptr);
		void load_my_rating(threaded_process_status &p_status, abort_callback &p_abort, bool throw_all = false);
	};

	
	class Identity
	{
	public:
		pfc::string8 user_id;
		pfc::string8 username;
		pfc::string8 consumer_name;
	};

	extern void parseIdentity(json_t *root, Identity *identity);
	extern void parseCollection(json_t *root, pfc::array_t<pfc::string8> &collection);

	extern void parseRelease(Release *release, json_t *root);
	extern void parseReleaseRating(Release *release, json_t *root);
	extern void parseMasterRelease(MasterRelease *master_release, json_t *root);
	extern void parseArtist(Artist *artist, json_t *root);

	extern void parseArtistResults(json_t *root, pfc::array_t<Artist_ptr> &artists);

	extern void parseImages(json_t *array, pfc::array_t<Image_ptr> &images);
	extern Image_ptr parseImage(json_t *element);

	extern void parseArtistReleases(json_t *element, Artist *artist, bool bva_artists);
	extern void parseMasterVersions(json_t *element, MasterRelease *master_release);

	extern ReleaseArtist_ptr parseReleaseArtist(json_t *element, bool preload = false);
	extern void parseReleaseArtists(json_t *element, pfc::array_t<ReleaseArtist_ptr> &artists, bool preload = false);
	extern void parseReleaseCredits(json_t* element, pfc::array_t<ReleaseCredit_ptr> &credits, Release *release);

	extern void DistReleaseTrackCredits(const pfc::array_t<pfc::string8>& arrTracks, ReleaseCredit_ptr &credit, Release *release);

	extern ReleaseLabel_ptr parseReleaseLabel(json_t *element);
	extern void parseReleaseLabels(json_t *element, pfc::array_t<ReleaseLabel_ptr> &release_labels);
	
	extern ReleaseCompany_ptr parseReleaseCompany(json_t *element);
	extern void parseReleaseCompanies(json_t *element, pfc::array_t<ReleaseCompany_ptr> &release_companies);

	extern ReleaseSeries_ptr parseReleaseSeries(json_t *element);
	extern void parseReleaseSeries(json_t *element, pfc::array_t<ReleaseSeries_ptr> &release_series);

	extern ReleaseFormat_ptr parseReleaseFormat(json_t *element);
	extern void parseReleaseFormats(json_t *element, pfc::array_t<ReleaseFormat_ptr> &formats);

	extern void parseAllReleaseTracks(json_t* element, bool isRelease, HasTracklist* has_tracklist, HasArtists* has_artists, HasIndexes* has_indexes);
	extern void parseReleaseTrack(json_t* element, pfc::array_t<ReleaseTrack_ptr>& tracks, /*unsigned& discogs_original_track_number,*/ pfc::string8 heading, ReleaseTrack_ptr* index, HasArtists* has_artists, HasIndexes* has_indexes);
}
