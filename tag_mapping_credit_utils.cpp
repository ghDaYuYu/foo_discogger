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

	return false;
	//return is_number(str.get_ptr()) && std::find(v.begin(), v.end(), str) != v.end();
}

pfc::string8 min_range_to_tracks(pfc::string8 tracks) {

	std::vector<pfc::string8> v;
	split(tracks, " to ", 0, v);
	if (v.size() > 1) {
		bool bnums = is_number(v[0].c_str()) && is_number(v[1].c_str());
		if (bnums && (atoi(v[1]) == atoi(v[0]) + 1))
			return PFC_string_formatter() << v[0] << ", " << v[1];
	}
	return tracks;
}

pfc::string8 tracks_to_range(pfc::string8 tracks) {
	
	pfc::string8 ret;
	std::vector<pfc::string8> v;
	std::vector<pfc::string8> vo;
	split(tracks, ",", 0, v);

#ifdef DEBUG
	if (v.size() > 2)
		v = v;
#endif

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
	for (auto x : vo) {
		ret << separator << min_range_to_tracks(x);
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
			//remove non numbers if is not credit
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
		tag_name = DEFAULT_CAT_CREDIT_TAG;

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

	//group (all, release, track) position in array
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
				out << "( credit_id == ";
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
		pfc::string8 nofilter("( catid = 0 )");
		size_t dbug_strstr = pfc::strstr_ex(out.get_ptr(), out.get_length(), nofilter, nofilter.get_length());
		if (STR_EQUAL(trim(out), nofilter) /*|| (pfc::strstr_ex(out.get_ptr(), out.get_length(), nofilter, nofilter.get_length()) != pfc_infinite)*/) {
			out = "";
		}
	}
}
