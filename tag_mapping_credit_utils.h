#pragma once

#include "discogs.h"

#define CAT_CREDIT_TAG_PREFIX "RELEASE_CAT_CREDITS_"
#define DEF_CAT_CREDIT_TAG "RELEASE_CAT_CREDITS_GXP_0_ALL"

#define GROUP_BY_GXP "GXP"
#define GROUP_BY_GXC "GXC"
#define DEF_GROUP_BY "GXP"

#define SRC_ALL "ALL"
#define SRC_RELEASE "RELEASE"
#define SRC_TRACKS "TRACKS"
#define DEF_SRC "ALL"

#define DEF_CATS "0"
#define DEF_NAME_PREFIX "TMPL_"

using namespace Discogs;

class credit_tag_nfo {

	pfc::string8 tag_name;
	pfc::string8 sub_tag_name;
	std::vector<pfc::string8>sub_tag_split;
	pfc::string8 groupby = GROUP_BY_GXP;
	pfc::string8 cat = "0";
	pfc::string8 src = "ALL";
	std::vector<pfc::string8> vsplit;

	size_t srcpos = pfc_infinite;
	bool checked = false;

public:

	enum { HEADER = 0, SUBHEADER, CREDIT };

	credit_tag_nfo() {

		tag_name = "";
		vsplit = {};
		sub_tag_name = "";
		sub_tag_split = {};

		groupby = DEF_GROUP_BY;
		src = DEF_SRC;
		cat = "";

		srcpos = pfc_infinite;
		checked = false;

		stdf_change_notifier = nullptr;
	};

	pfc::string8 rebuild_tag_name() {

		pfc::string8 tag = CAT_CREDIT_TAG_PREFIX;

		tag << groupby << "_";
		pass_v_to_cat();
		tag << cat << "_";
		tag << src;

		tag_name = tag;
		return tag;
	}

	pfc::string8 rebuild_tag_name(std::vector<pfc::string8> v) {
		pfc::string8 catperm = pass_v_to_cat(v);
		pfc::string8 parsed_cat = parse_cat(catperm, 0);
		pass_cat_to_v(parsed_cat);
		pass_v_to_cat();
		rebuild_tag_name();
		return tag_name;
	}

	void SetNotifier(std::function<bool(HWND, bool)>update_notifier) {
		stdf_change_notifier = update_notifier;
	}

	std::function<bool(HWND, bool)>stdf_change_notifier = nullptr;

	pfc::string8 get_src() { return src; }
	pfc::string8 get_groupby() { return groupby; }
	std::vector<pfc::string8> get_vsplit() { return vsplit; }
	
	const pfc::string8 get_cat() { return cat; }
	const pfc::string8 get_tagname() { return tag_name; }
	//const pfc::string8 get_tagname_ff() { pfc::string8 ff("%"); ff << tag_name << "%"; return ff; }	
	const std::vector<pfc::string8> get_sub_tag_split() { return sub_tag_split; }
	bool get_checked() { return checked; }

	bool isGXC();
	bool isGXP();
	bool isAll();
	bool isRelease();
	bool isTracks();

	void init(pfc::string8 tag_name);
	void set_groupby(pfc::string8 agrp) { groupby = agrp; }
	void set_src(pfc::string8 asrc) { src = asrc; }
	void set_sub_tag_split(pfc::string8 asub_tag_split);

	void pass_v_to_cat();
	pfc::string8 pass_v_to_cat(std::vector<pfc::string8> vfrom);
	void pass_cat_to_v(pfc::string8 parsed_cat, bool append = false);
	pfc::string8 parse_cat(pfc::string8 testformat, size_t level);

	void build_vwhere_cats(pfc::string8& out);

	pfc::string8 get_default_name(size_t ndx) {
		pfc::string8 def_str;
		def_str << DEF_NAME_PREFIX;
		def_str << pfc::toString(ndx).get_ptr();
		return def_str;
	}

	pfc::string8 get_default_titleformat() {
		pfc::string8 def_str;
		def_str << CAT_CREDIT_TAG_PREFIX;	// "RELEASE_CAT_CREDITS_"
		def_str << DEF_GROUP_BY << "_";		// "GXP"	(group per artist)
		def_str << DEF_CATS << "_";			// "0"		(all credits)
		def_str << DEF_SRC;					// "ALL"	(release + tracks)
		return def_str;
	}

	void SetTagCredit(size_t type, pfc::string8 id, bool state);
	void RemoveTagCredit(size_t index, bool disable);

	size_t GetElemType(size_t index);
	void ChangeNotify(HWND wnd, bool updatedata) { stdf_change_notifier(wnd, updatedata); }
};

extern bool is_number(const std::string& s);
//extern int cmp(const void* key, const void* value);
extern size_t grp_array_position(const std::string& value/*, const std::vector<std::string>& array*/);
extern size_t split(pfc::string8 str, pfc::string8 token, size_t index, std::vector<pfc::string8>& out);
// 1-7+223 (drums) splits into std::pair("1-7","223")
// 1-7	 to std::pair("1-7","")
// +223	 to std::pair("", "223") returning size_t 223 or pfc_infinite if +token not found
extern bool vfind(pfc::string8 str, std::vector<pfc::string8> v);

extern pfc::string8 sanitize_track_commas(const pfc::string8& tracks);
extern pfc::string8 disc_tracks_to_range(const pfc::string8& tracks, Release* release, bool stdnames = false);
extern pfc::string8 tracks_to_range(const pfc::string8& tracks);

//extern void init(pfc::string8 tag_name, credit_tag_nfo& test_ctag);
//extern size_t plus_split(pfc::string8 str, std::pair<pfc::string8, pfc::string8>& out);