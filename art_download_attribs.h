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