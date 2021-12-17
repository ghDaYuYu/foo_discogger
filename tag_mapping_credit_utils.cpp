#include "stdafx.h"

#include "tag_mapping_credit_utils.h"

std::string toupper(const std::string str) { std::string tmpstr = str; for (auto& c : tmpstr) c = toupper(c); return tmpstr; }
std::string toupper(const char* str) { std::string tmpstr(str); for (auto& c : tmpstr) c = toupper(c); return tmpstr; }

bool is_number(const std::string& s)
{
	return !s.empty() && std::find_if(s.begin(),
		s.end(), [](unsigned char c) { return !std::isdigit(c); }) == s.end();
}

size_t grp_array_position(const std::string& value) 
{
	const std::vector<std::string> array{ SRC_ALL, SRC_RELEASE, SRC_TRACKS };
	size_t pos = std::distance(array.begin(), std::find(array.begin(), array.end(), value));
	return pos == array.size() ? pfc_infinite : pos;
}

size_t split(pfc::string8 str, pfc::string8 token, size_t index, std::vector<pfc::string8>& out) {
	size_t last_index = index;

	index = str.find_first(token, index);
	if (index != pfc_infinite) {
		//if (str.length())
			out.push_back(pfc::string_part(str + last_index, index - last_index));
		//recursion
		return split(str, token, index + token.length(), out);
	}
	else {
		pfc::string8 notoken;
		if ((notoken = substr(str, last_index, str.get_length())).get_length())
			out.push_back(notoken);
		return pfc_infinite; //end
	}
}

bool vfind(pfc::string8 str, std::vector<pfc::string8> v) {

	std::vector<pfc::string8> vsplit_str;
	split(str, "-", 0, vsplit_str);

	if (vsplit_str.size() == 2) {
		if (std::find(v.begin(), v.end(), vsplit_str.at(0)) != v.end())
			//subcat contained in demanded cat
			return true;
	}

	if (std::find(v.begin(), v.end(), str) != v.end()) {
		//subcat equals to demanded subcat
		return true;
	}
	//return is_number(str.get_ptr()) && std::find(v_tracks.begin(), v_tracks.end(), str) != v_tracks.end();
	return false;
}

//sanitize tokenize_multi(...) input
//ej. missing commas in "A1 to A6 B1 to B3, B5"
//https://www.discogs.com/The-Cribs-Mens-Needs-Womens-Needs-Whatever/release/6963878

pfc::string8 sanitize_track_commas(const pfc::string8& tracks) {

	pfc::string8 res;

	//split
	std::vector<pfc::string8>v;
	split(tracks, " ", 0, v);
	for (int i = 0; i < v.size(); i++) {
		if (i < v.size() - 2) {
			if (v.at(i).equals("to")) {
				if (v.at(i + 1).find_first(',', 0) == pfc_infinite) {
					auto c = std::find(v.begin() + i, v.end(), ",");
					if (c == v.end() || (c - v.begin() + i > 1)) {
						v.at(i + 1) = PFC_string_formatter() << v.at(i + 1) << ",";
					}					
				}
				v.at(i) = " to ";
			}
		}
	}

	//rebuild
	std::vector<pfc::string8>::iterator iter_forward = v.begin();
	for (auto w : v) {
		iter_forward++;
		size_t comma_pos = w.find_first(',', 0);
		if (comma_pos != pfc_infinite) {
			res << w << " ";
			continue;
		}
		if (w.equals(" to ")) {		
			res << w;
			continue;
		}
		res << w << (iter_forward != v.end() && !iter_forward->equals(" to ") ? ", " : "");
	}
	return res;
}

//todo: implement credits spanning multiple vinyls (A4 to C3)

//test 1: credit tracks spanning multiple discs
//		bass gxp (+827) https://www.discogs.com/release/796538-Sonny-Criss-The-Complete-Imperial-Sessions
//		RELEASE_CAT_CREDITS_GXP_+827&+374&+392&+445_ALL
//		Bass: Bill Woodson (tracks: 1-1 to 1-12), Buddy Clark (tracks: 2-7 to 2-16), Leroy Vinnegar (tracks: 1-13 to 2-6)
//
//test 2: malformed (missing commas credit tracks)
//		engineer (+133) https://www.discogs.com/release/1131383-The-Cribs-Mens-Needs-Womens-Needs-Whatever
//		Engineer: David Corcos (tracks: A1 to A6 B1 to B3, B5), Jim Keller (tracks: B4, B6)
//
//test 3: credit track format mismatch. discogs release tracks listed 1, 2, 3 - discogs credits A1 to A6...
//		https://www.discogs.com/release/6963878-The-Cribs-Mens-Needs-Womens-Needs-Whatever
//
//test 4: track notation incluing minus sign (LP-A1 to LP-B3) 
//		https://www.discogs.com/release/2556904
//
//note:	func input tracks parameter is 1 based disk.track (1.1, 1.3, 2.1...) list, no ranges
//		serves release get_sub_data()

pfc::string8 disc_tracks_to_range(const pfc::string8& tracks, Release* release, bool stdnames) {
	
	pfc::string8 ret;

	//split comma separated input tracks (1.1, 2.4...) -> {1.1, 2.4...}
	std::vector<pfc::string8> v_tracks;
	split(tracks, ",", 0, v_tracks);

//#ifdef DEBUG	
//	if (tracks.find_first("1.13", 0) != pfc_infinite) {		
//		int debug = 1;
//	}
//#endif

	std::vector <std::pair<pfc::string8, pfc::string8>> v_disk_tracks;
	for (auto w : v_tracks) {

		//split single track {{disc, track}}
		std::vector<pfc::string8> v_one_track;
		split(w, ".", 0, v_one_track);

		size_t media = atoi(v_one_track.at(0));
		if (media > v_disk_tracks.size()) {
			//new element {{}, {"2", "1"}}
			v_disk_tracks.resize(media);
			auto& disctrack = v_disk_tracks.at(media - 1);
			disctrack = std::pair(pfc::toString(media).c_str(), v_one_track.at(1));
		}
		else {
			//append track to v_disk_tracks element {{}, {"2", "1,4"}}
			auto& disctrack = v_disk_tracks.at(media - 1);
			pfc::string8 left = pfc::toString(media).c_str();
			disctrack = std::pair(left, (PFC_string_formatter() << disctrack.second << "," << v_one_track.at(1)).c_str());
		}
	}

	//define ranges and (optional) translate to release track position notation (1.1 to 1.3 -> A1 to A3)

	pfc::string8 separator = "";
	for (auto w : v_disk_tracks) {

		if (!w.second.length()) continue; //skipping empty element {{empty, empty}, {"2", "1, 2, 3, 5, 8, 9, 10"}}

		pfc::string8 tmpstr = tracks_to_range(w.second); // outputs "1 to 3, 5, 8 to 10"

		std::vector<pfc::string8>v_cmp_tracks;
		split(tmpstr, ",", 0, v_cmp_tracks);

		//restore disc prefix or release track number notation

		for (auto& wct : v_cmp_tracks) {

			size_t pos = wct.find_first(" to ", 0);

			if (pos != pfc_infinite) {
				if (stdnames) {
					tmpstr = PFC_string_formatter()
						<< w.first << "." << substr(wct, 0, pos)
						<< " to "
						<< w.first << "." << substr(wct, pos + pfc::string8(" to ").length(), pfc_infinite);
				}
				else {
					size_t ndx_left = atoi(substr(wct, 0, pos)) - 1;
					size_t ndx_right = atoi(substr(wct, pos + pfc::string8(" to ").length(), pfc_infinite)) - 1;

					tmpstr = PFC_string_formatter()
						<< release->discs[atoi(w.first) - 1]->tracks[ndx_left]->discogs_track_number
						<< " to "
						<< release->discs[atoi(w.first) - 1]->tracks[ndx_right]->discogs_track_number;
				}
			}
			else {
				if (stdnames) {
					tmpstr = PFC_string_formatter() << w.first << "." <<
						tmpstr.replace_string(", ", PFC_string_formatter() << ", " << w.second);
				}
				else {
					tmpstr = release->discs[atoi(w.first) - 1]->tracks[atoi(wct) - 1]->discogs_track_number;
				}
			}

			ret << separator << tmpstr;
			separator = ", ";
		}
	}

	// A1 to B3, B5
	// 1.1 to 1.9, 1.11 (option)
	return ret;
}

pfc::string8 min_range_to_tracks(const pfc::string8& tracks) {

	std::vector<pfc::string8> v;
	split(tracks, " to ", 0, v);
	if (v.size() > 1) {
		bool bnums = is_number(v[0].c_str()) && is_number(v[1].c_str());
		if (bnums && (atoi(v[1]) == atoi(v[0]) + 1))
			return PFC_string_formatter() << v[0] << ", " << v[1];
	}
	return tracks;
}

pfc::string8 tracks_to_range(const pfc::string8& tracks) {
	
	pfc::string8 ret;
	std::vector<pfc::string8> v;
	std::vector<pfc::string8> vo;
	split(tracks, ",", 0, v);

	size_t beg = 0;
	vo.push_back(v[0]);
	for (size_t n = 1; n < v.size(); n ++) {
		bool bnums = is_number(v[n - 1].c_str()) && is_number(v[n].c_str());
		if (!bnums || (atoi(v[n]) != atoi(v[n - 1]) + 1)) {
			vo.push_back(v[n]);
			beg = n;
		}
		else {
			vo[vo.size() - 1] = PFC_string_formatter() << v[beg] << " to " << v[n];			
		}
	}
	pfc::string8 separator = "";
	for (auto w : vo) {
		ret << separator << min_range_to_tracks(w);
		separator = ", ";
	}
	return ret;
}

bool credit_tag_nfo::isGXP() {
	return (STR_EQUAL(groupby, GROUP_BY_GXP));
}
bool credit_tag_nfo::isGXC() {
	return (STR_EQUAL(groupby, GROUP_BY_GXC));
}
bool credit_tag_nfo::isAll() {
	return (STR_EQUAL(src, SRC_ALL));
}
bool credit_tag_nfo::isRelease() {
	return (STR_EQUAL(src, SRC_RELEASE));
}
bool credit_tag_nfo::isTracks() {
	return (STR_EQUAL(src, SRC_TRACKS));
}


bool catmatch(std::vector<pfc::string8> vsplit, pfc::string8 cat) {

	bool bcatmatch = (vsplit.size() == 0) || vfind(cat, vsplit);
	return bcatmatch;
}

void credit_tag_nfo::set_sub_tag_split(pfc::string8 asub_tag_split) {
	split(asub_tag_split.c_str(), "_", 0, sub_tag_split);
}

void credit_tag_nfo::pass_v_to_cat() {
	if (vsplit.size()) {
		cat = "";
		for (auto elem : vsplit) {
			cat << elem << "&";
		}
		if (cat.ends_with('&')) cat = substr(cat, 0, cat.get_length() - 1);
	}
	else
	{
		cat = "0";
	}
}

void credit_tag_nfo::pass_cat_to_v(pfc::string8 parsed_cat, bool append) {
	if (!append) vsplit.clear();		
	split(parsed_cat, "&", 0, vsplit);
}

pfc::string8 credit_tag_nfo::pass_v_to_cat(std::vector<pfc::string8> vfrom) {
	pfc::string8 non_parsed;
	for (auto& walk_cat : vfrom)
		non_parsed << walk_cat << "&";

	if (non_parsed.ends_with('&')) 
		non_parsed = substr(non_parsed, 0, non_parsed.get_length() - 1);
	return non_parsed;
}

pfc::string8 credit_tag_nfo::parse_cat(pfc::string8 unparsed_cat, size_t level) {
	//only two level cats (category-subcategory-credit -> 1-7-223 -> Instruments-Other Musical-Drums)
	if (level > 2) return !is_number(unparsed_cat.c_str())? "" : unparsed_cat;
		
	pfc::string8 pre_parsed_cat, parsed_cat;
	std::vector<pfc::string8> tmp_vsplit;
	split(unparsed_cat, "&", 0, tmp_vsplit);

	for (auto& walk_cat : tmp_vsplit) {
		std::vector<pfc::string8> tmp_vsplit_sub;
		split(walk_cat, "-", 0, tmp_vsplit_sub);
		if (tmp_vsplit_sub.size() > 1) {
			//contains subcategory (recursion)
			//left side
			pre_parsed_cat << tmp_vsplit_sub.at(0);
			//right side
			size_t leftpart_len = tmp_vsplit_sub.at(0).get_length();
			pfc::string8 rightpart = parse_cat(substr(walk_cat, leftpart_len + pfc::string("&").length()), level + 1);
			pre_parsed_cat << (rightpart.get_length() ? "-" : "") << rightpart ;
		}
		else {
			//remove non numbers if not credit
			if (walk_cat.has_prefix("+"))
				pre_parsed_cat << (is_number(substr(walk_cat, 1, walk_cat.get_length() -1).c_str()) ? walk_cat : "");
			else
				pre_parsed_cat << (is_number(walk_cat.c_str()) ? walk_cat : "");
		}
		pre_parsed_cat << (level == 0 ? "&" : "");
	}

	//build cat
	if (level == 0) {
		tmp_vsplit.clear();
		split(pre_parsed_cat, "&", 0, tmp_vsplit);
		size_t pos = 0;
		size_t parsed = 0;
		for (auto elem : tmp_vsplit) {
			auto find_it = std::find(tmp_vsplit.begin(), tmp_vsplit.end(), elem);
			//also remove duplicates
			size_t first_pos = std::distance(tmp_vsplit.begin(), find_it);
			if (elem.get_length() && first_pos == pos) {
				parsed_cat << elem << "&";
				++parsed;
			}
			++pos;
		}
		if (parsed_cat.ends_with('&')) parsed_cat = substr(parsed_cat, 0, parsed_cat.get_length() - 1);

		//return level 0 recursion
		return parsed_cat;
	}
	else {
		//recursion
		return pre_parsed_cat;
	}
}

void credit_tag_nfo::init(pfc::string8 tagname) {

	groupby = DEF_GROUP_BY;
	cat = "0";
	sub_tag_split.clear();
	src = DEF_SRC;

	vsplit.clear();

	bool needtag = false;

	if (strncmp(pfc::stringToUpper(tagname), CAT_CREDIT_TAG_PREFIX, 20) == 0)
		tag_name = pfc::stringToUpper(tagname);
	else
		tag_name = DEF_CAT_CREDIT_TAG;

	sub_tag_name = substr(tag_name, 20);

	//split by underscore
	split(sub_tag_name.c_str(), "_", 0, sub_tag_split);

	//check &= sub_tag_split.size() >= 1 && sub_tag_split.size() <= 3;
		 
	//groupby, cat, src
	if (sub_tag_split.size() > 0)
		groupby.set_string(sub_tag_split[0].c_str());
	if (sub_tag_split.size() > 1)
		cat.set_string(sub_tag_split[1].c_str());
	if (sub_tag_split.size() > 2)
		src.set_string(sub_tag_split[2].c_str());

	//basic parse on group (src)
	checked = STR_EQUAL(groupby, GROUP_BY_GXC) || STR_EQUAL(groupby, GROUP_BY_GXP);

	//group (all, release, v_one_track) position in array
	checked = grp_array_position(src.get_ptr()) != pfc_infinite;


	//fill cat vector from parsed cats '&'
	pfc::string8 parsed_cat = parse_cat(cat, 0);
	pass_cat_to_v(parsed_cat);
	pass_v_to_cat();

	if ((vsplit.size() == 1 && STR_EQUAL(vsplit.at(0), "0")) || (vsplit.size() == 0)) {
		vsplit.clear();
		cat = "0";
	} 

	srcpos = grp_array_position(src.get_ptr());
	checked &= srcpos != pfc_infinite;
		
	//REMOVE prefix
	//set_sub_tag_split(substr(tagname, 20));
}

void credit_tag_nfo::SetTagCredit(size_t type, pfc::string8 id, bool state) {

	pfc::string8 elem;
	elem << (type == credit_tag_nfo::CREDIT ? "+" : "") << id;

	pass_cat_to_v(cat);
	auto it = std::find(vsplit.begin(), vsplit.end(), elem);
	if (!state) {
		if (it != vsplit.end()) 
		{
			vsplit.erase(it);
		}
	}
	else {
		if (it == vsplit.end()) {				
				
			vsplit.emplace_back(elem);
		}			
	}
	//pass_v_to_cat();
	rebuild_tag_name();

	stdf_change_notifier(nullptr, true);
}

void credit_tag_nfo::RemoveTagCredit(size_t index, bool notify) {

	vsplit.erase(vsplit.begin() + index);

	rebuild_tag_name();
	if (notify) stdf_change_notifier(nullptr, false);
}

size_t credit_tag_nfo::GetElemType(size_t index) {
	size_t ires = pfc_infinite;
	pfc::string8 elem = vsplit[index];
	if (elem.has_prefix("+")) {
		ires = CREDIT;
	}
	else {
		std::vector<pfc::string8> v;
		split(elem, "-", 0, v);
		if (v.size() == 2)
			ires = SUBHEADER;
		else
			ires = HEADER;
		
	}

	return ires;
}

void credit_tag_nfo::build_vwhere_cats(pfc::string8& out) {

	// instruments -> stringed instr. or vocals
	// and ((catid = 1 and subcatid = 4) or credit_id = 178)

	if (vsplit.size()) {
		pfc::string8 postfix = "or ";
		out = "and (";
		for (auto elem : vsplit) {
			if (elem.has_prefix("+")) {
				out << "( credit_id = ";
				out << substr(elem, 1, elem.get_length() - 1) << " ";
			}
			else {
				std::vector<pfc::string8> velem;
				split(elem, "-", 0, velem);
				if (velem.size() == 2) {
					out << "( catid = ";
					out << velem.at(0);
					out << " and subcatid = ";
					out << velem.at(1) << " ";
				}	
				else {
					out << "( catid = ";
					out << elem << " ";
				}
			}
			out << ") " << postfix;
		}

		//check/remove extra postfix
		if (STR_EQUAL(substr(out, out.get_length() - pfc::string8(postfix).get_length(), out.get_length()), postfix)) {
			out = substr(out, 0, out.get_length() - pfc::string8(postfix).get_length());
		}
		//close enclosing bracket
		out << ") ";

		//(cat = 0) -> no filter
		pfc::string8 nofilter("and ( catid = 0 )");
		size_t dbug_strstr = pfc::strstr_ex(out.get_ptr(), out.get_length(), nofilter, nofilter.get_length());
		if (STR_EQUAL(trim(out), nofilter) /*|| (pfc::strstr_ex(out.get_ptr(), out.get_length(), nofilter, nofilter.get_length()) != pfc_infinite)*/) {
			out = "";
		}
	}
	else {
		out = "";
	}
}
