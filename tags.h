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
	pfc::string8 tag_name = "";
	bool is_multival_meta = false;
	bool enable_write = false;
	bool enable_update = false;
	bool freeze_write = false;
	bool freeze_update = false;
	bool freeze_tag_name = false;
	formatting_script formatting_script;

	tag_mapping_entry() {};

	tag_mapping_entry(const char *tn, bool ew, bool eu, bool fw, bool fu, bool ft, const char *fs) :
		tag_name(tn), enable_write(ew), enable_update(eu), freeze_write(fw), freeze_update(fu), freeze_tag_name(ft), formatting_script(fs) {
	
		is_multival_meta = is_multivalue_meta(pfc::string8(tn));
	}

	tag_mapping_entry * clone() {
		tag_mapping_entry *t = new tag_mapping_entry();
		*t = *this;
		t->is_multival_meta = this->is_multival_meta;
		return t;
	}

	const bool equals(const tag_mapping_entry& a) {
		return (STR_EQUAL(tag_name, a.tag_name) &&
			enable_write == a.enable_write &&
			enable_update == a.enable_update &&
			freeze_write == a.freeze_write &&
			freeze_update == a.freeze_update &&
			freeze_tag_name == a.freeze_tag_name &&
			STR_EQUAL(formatting_script, a.formatting_script));
	}
};


extern bool operator ==(const tag_mapping_entry &a, const tag_mapping_entry &b);


FB2K_STREAM_READER_OVERLOAD(tag_mapping_entry) {
	pfc::string8 tag_name, formatting_string;
	stream >> tag_name
		>> value.enable_write
		>> value.enable_update
		>> value.freeze_write
		>> value.freeze_update
		>> value.freeze_tag_name
		>> formatting_string;
	value.tag_name = tag_name;
	value.formatting_script = formatting_string;

	bool release_id_mod = STR_EQUAL(TAG_RELEASE_ID, value.tag_name.get_ptr());
	if (release_id_mod) {
		if (!value.enable_write) {
			value.enable_write = true;
		}		
	}

	return stream;
}


FB2K_STREAM_WRITER_OVERLOAD(tag_mapping_entry) {
	bool release_id_mod = STR_EQUAL(TAG_RELEASE_ID, value.tag_name.get_ptr());
	if (release_id_mod) {
		if (!value.enable_write) {
			release_id_mod = true;
		}
		else
		{
			release_id_mod = value.enable_write;
		}
	}
	pfc::string8 tag_name(value.tag_name.get_ptr());
	pfc::string8 formatting_string((const char*)value.formatting_script);
	stream << tag_name
		<< release_id_mod
		<< value.enable_update
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
extern pfc::list_t<tag_mapping_entry> * copy_tag_mappings();
extern pfc::list_t<tag_mapping_entry> * copy_default_tag_mappings();
extern void update_loaded_tagmaps_multivalues();
extern void set_cfg_tag_mappings(pfc::list_t<tag_mapping_entry> *mappings);

extern pfc::string8 get_default_tag(const pfc::string8 &name);
extern cfg_tag_mapping_list_type cfg_tag_mappings;

#define TAGS cfg_tag_mappings
