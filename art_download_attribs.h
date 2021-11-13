#pragma once
#include <vector>
#include <pfc/pfc.h>

struct art_download_attribs
{
	bool write_it;
	bool embed_it;
	bool overwrite_it;
	bool fetch_all_it;
	bool to_path_only;

	std::vector<pfc::string8> vpaths;

	art_download_attribs()
		: write_it(false), embed_it(false), overwrite_it(false),
		fetch_all_it(false), to_path_only(false), vpaths() {};

	art_download_attribs(bool write_it, bool embed_it, bool overwrite_it,
		bool fetch_all_it, bool to_parth_only, std::vector<pfc::string8> vpaths)
		: write_it(write_it), embed_it(embed_it), overwrite_it(overwrite_it),
		fetch_all_it(fetch_all_it), to_path_only(to_parth_only), vpaths(vpaths) {};
};

namespace template_art_ids {
//! inlay card
static const GUID inlay_card = { 0xfc8c7ba2, 0x4396, 0x43ed, { 0x9d, 0x55, 0x5, 0xeb, 0xda, 0x52, 0x81, 0xec } };
//! booklet
static const GUID booklet = { 0xd8e6dcd8, 0x5610, 0x4920, { 0xa8, 0x9e, 0x35, 0x85, 0x8f, 0x83, 0x5b, 0x26 } };
//! page folder
static const GUID page_folder = { 0x9f9262ca, 0xef84, 0x49f2, { 0xab, 0x84, 0x5b, 0xec, 0xa4, 0x8e, 0x4d, 0x5f } };

size_t num_types();
GUID query_type(size_t);
// returns lowercase name
const char* query_name(size_t);
const char* name_of(const GUID&);
// returns Capitalized name
const char* query_capitalized_name(size_t);
const char* capitalized_name_of(const GUID&);
};

