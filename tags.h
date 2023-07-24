#pragma once

#include "formatting_script.h"
#include "string_encoded_array.h"
#include "utils.h"


typedef struct
{
	char *name;
	char *description;
} data_mapping_entry;


static pfc::string8 TAG_RELEASE_ID("DISCOGS_RELEASE_ID");
static pfc::string8 TAG_ARTIST_ID("DISCOGS_ARTIST_ID");
static pfc::string8 TAG_LABEL_ID("DISCOGS_LABEL_ID");
static pfc::string8 TAG_MASTER_RELEASE_ID("DISCOGS_MASTER_RELEASE_ID");


class tag_mapping_entry
{
public:

	GUID guid_tag = pfc::guid_null;
	pfc::string8 tag_name = "";
	bool is_multival_meta = false;
	bool enable_write = false;
	bool enable_update = false;
	bool freeze_write = false;
	bool freeze_update = false;
	bool freeze_tag_name = false;
	formatting_script formatting_script;

	tag_mapping_entry() {
		//..
	};

	~tag_mapping_entry() {
		//..
	};

	tag_mapping_entry(const char *tn, bool ew, bool eu, bool fw, bool fu, bool ft, const char *fs) :
		tag_name(tn), enable_write(ew), enable_update(eu), freeze_write(fw), freeze_update(fu), freeze_tag_name(ft), formatting_script(fs) {
	
		is_multival_meta = is_multivalue_meta(pfc::string8(tn));
	}

	tag_mapping_entry * clone() {
		tag_mapping_entry *t = new tag_mapping_entry();
		*t = *this;
		t->guid_tag = pfc::createGUID();
		t->is_multival_meta = this->is_multival_meta;
		return t;
	}

	const bool equals(const tag_mapping_entry& a) {
		return pfc::guid_equal(guid_tag, a.guid_tag) &&
			(STR_EQUAL(tag_name, a.tag_name) &&
			enable_write == a.enable_write &&
			enable_update == a.enable_update &&
			freeze_write == a.freeze_write &&
			freeze_update == a.freeze_update &&
			freeze_tag_name == a.freeze_tag_name &&
			STR_EQUAL(formatting_script, a.formatting_script));
	}
};

// == operator

inline bool operator ==(const tag_mapping_entry& a, const tag_mapping_entry& b) {

	return pfc::guid_equal(a.guid_tag, b.guid_tag);
}


FB2K_STREAM_READER_OVERLOAD(tag_mapping_entry) {

	pfc::string8 buffer;
	stream >> buffer;
	
	//todo: rev. fix nulls introduced by uncomplete fix v1.0.19.1
	if (buffer.equals(pfc::print_guid(pfc::guid_null))) {
		buffer = pfc::print_guid(pfc::createGUID());
	}

	// from v0.19
	GUID guid_check = pfc::GUID_from_text(buffer);

	//TODO: rev (quick fix 0.19.1 tag mapping guids)
	auto ol = buffer.get_length();           // 36
	auto tb = buffer.replace_char('-', '-'); //  4
	auto fp = buffer.find_first('-');        //  8
	auto fl = buffer.find_last('-');         // 23
	bool bpattern = (ol == 36 && tb == 4 && fp == 8 && fl == 23);

	if (!bpattern || pfc::guid_equal(guid_check, pfc::guid_null)) {
		value.guid_tag = pfc::createGUID();
		tag_name = buffer;
	}
	else {
		value.guid_tag = guid_check;
		stream >> tag_name;
	}
	stream >> value.enable_write
		>> value.enable_update
		>> value.freeze_write
		>> value.freeze_update
		>> value.freeze_tag_name
		>> formatting_string;
	value.tag_name = tag_name;
	value.formatting_script = formatting_string;
	return stream;
}


FB2K_STREAM_WRITER_OVERLOAD(tag_mapping_entry) {

	bool release_id_mod = STR_EQUAL(TAG_RELEASE_ID, value.tag_name.get_ptr());
	bool mod_write = value.enable_write;
	bool mod_update = value.enable_update;
	if (release_id_mod) {
		mod_write = LOWORD(CONF.alt_write_flags) & (1 << 0);
		mod_update = LOWORD(CONF.alt_write_flags) & (1 << 1);
	}
	pfc::string8 guid_tag = pfc::print_guid(value.guid_tag);
	pfc::string8 tag_name(value.tag_name.get_ptr());
	pfc::string8 formatting_string((const char*)value.formatting_script);
	stream << guid_tag
		<< tag_name
		<< mod_write
		<< mod_update
		<< value.freeze_write
		<< value.freeze_update
		<< value.freeze_tag_name
		<< formatting_string;
	return stream;
}


typedef pfc::list_t<tag_mapping_entry> tag_mapping_list_type;
typedef cfg_objList<tag_mapping_entry> cfg_tag_mapping_list_type;

extern void init_tag_mappings();
extern void init_with_default_tag_mappings();

extern bool awt_get_release_mod_flag(tag_mapping_entry& out);
extern bool awt_set_release_mod_flag(tag_mapping_entry e);
extern int awt_update_mod_flag(bool fromFlag =	true);
extern bool awt_unmatched_flag();
extern void awt_save_normal_mode();

extern pfc::list_t<tag_mapping_entry> * copy_tag_mappings();
extern pfc::list_t<tag_mapping_entry> * copy_default_tag_mappings();
extern pfc::list_t<tag_mapping_entry> * copy_id3_default_tag_mappings(bool onlyms, bool menuctx);

extern void update_loaded_tagmaps_multivalues();
extern void set_cfg_tag_mappings(pfc::list_t<tag_mapping_entry> *mappings);

extern pfc::string8 get_default_tag(const pfc::string8 &name);
extern cfg_tag_mapping_list_type cfg_tag_mappings;

#define TAGS cfg_tag_mappings
