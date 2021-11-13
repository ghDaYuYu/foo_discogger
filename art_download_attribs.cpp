#include "art_download_attribs.h"

namespace {

	static const GUID guids[] = {
		template_art_ids::inlay_card,
		template_art_ids::booklet,
		template_art_ids::page_folder,
	};
	static const char* const names[] = {
		"inlay card",
		"booklet",
		"page_folder",
	};
	static const char* const names2[] = {
		"Inlay Card",
		"Booklet",
		"Page Folder",		
	};
}

size_t template_art_ids::num_types() {
	PFC_STATIC_ASSERT(PFC_TABSIZE(guids) == PFC_TABSIZE(names));
	PFC_STATIC_ASSERT(PFC_TABSIZE(guids) == PFC_TABSIZE(names2));
	return PFC_TABSIZE(guids);
}

GUID template_art_ids::query_type(size_t idx) {
	PFC_ASSERT(idx < PFC_TABSIZE(guids));
	return guids[idx];
}

const char* template_art_ids::query_name(size_t idx) {
	PFC_ASSERT(idx < PFC_TABSIZE(names));
	return names[idx];
}

const char* template_art_ids::name_of(const GUID& id) {
	for (size_t w = 0; w < num_types(); ++w) {
		if (query_type(w) == id) return query_name(w);
	}
	return nullptr;
}

const char* template_art_ids::query_capitalized_name(size_t idx) {
	PFC_ASSERT(idx < PFC_TABSIZE(names2));
	return names2[idx];
}

const char* template_art_ids::capitalized_name_of(const GUID& id) {
	for (size_t w = 0; w < num_types(); ++w) {
		if (query_type(w) == id) return query_capitalized_name(w);
	}
	return nullptr;
}
