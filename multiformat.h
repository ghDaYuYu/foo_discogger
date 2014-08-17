#pragma once

#include "string_encoded_array.h"
#include "discogs.h"
#include "exception.h"
#include "file_info_manager.h"


using namespace Discogs;


class persistent_store
{
private:
	std::map<const char*, const char*, cmp_str> store;

public:
	void put(const pfc::string8 &name, const pfc::string8 &value) {
		char *n = new char[name.get_length() + 1];
		char *v = new char[value.get_length() + 1];
		strcpy(n, name.get_ptr());
		strcpy(v, value.get_ptr());
		store[n] = v;
	}

	const char * get(const char *name) const {
		auto it = store.find(name);
		if (it != store.cend()) {
			return it->second;
		}
		else {
			foo_discogs_exception ex;
			ex << "Invalid $pget, unknown: " << name;
			throw ex;
		}
	}

	~persistent_store() {
		for (auto &it : store) {
			delete it.first;
			delete it.second;
		}
	}
};

/**********
TODO:
- Eliminate "update tags" and allow "write tags" for multiple releases at once, or not....
**********/


/*
class titleformat_hook_impl_discogs : public titleformat_hook
{
private:
	const Release_ptr &release = nullptrptr;
	const ReleaseDisc_ptr &release_disc = nullptrptr;
	const ReleaseTrack_ptr &release_track = nullptrptr;
	const MasterRelease_ptr &master_release = nullptrptr;
	const Artist_ptr &artist = nullptrptr;

	std::map<const char*, pfc::string8, cmp_str> custom_map;
};
*/


class titleformat_hook_impl_multiformat : public titleformat_hook
{
private:
	// TODO: figure out something better....
	const Release_ptr * release = nullptr;
	const ReleaseDisc_ptr * release_disc = nullptr;
	const ReleaseTrack_ptr * release_track = nullptr;
	const MasterRelease_ptr * master_release = nullptr;
	const Artist_ptr * artist = nullptr;
	const Image_ptr * image = nullptr;
	file_info_manager_ptr files = nullptr;

	const file_info *m_finfo = nullptr;
	persistent_store *m_store = nullptr;
	persistent_store *prompt_store = nullptr;
	std::map<const char*, string_encoded_array, cmp_str> custom_map;

public:
	titleformat_hook_impl_multiformat(const Release_ptr *release = nullptr, const ReleaseDisc_ptr *disc = nullptr, const ReleaseTrack_ptr *track = nullptr) : 
		release(release), release_disc(disc), release_track(track) {}
	titleformat_hook_impl_multiformat(const Artist_ptr *artist) : artist(artist) {}
	titleformat_hook_impl_multiformat(const MasterRelease_ptr *master_release, const Release_ptr *release = nullptr,
		const ReleaseDisc_ptr *release_disc = nullptr, const ReleaseTrack_ptr *release_track = nullptr,
		const file_info *info = nullptr, persistent_store *pstore = nullptr, persistent_store *prompt_store = nullptr) :
		release(release), release_disc(release_disc), release_track(release_track), master_release(master_release), artist(artist), m_finfo(info), m_store(pstore), prompt_store(prompt_store) {};

	void set_custom(const char *s, string_encoded_array v) {
		custom_map[s] = v;
	}

	void set_image(const Image_ptr *i) {
		image = i;
	}

	void set_track(const ReleaseTrack_ptr *t) {
		release_track = t;
	}

	void set_disc(const ReleaseDisc_ptr *d) {
		release_disc = d;
	}

	void set_release(const Release_ptr *r) {
		release = r;
	}

	void set_master(const MasterRelease_ptr *m) {
		master_release = m;
	}

	void set_files(file_info_manager_ptr f) {
		files = f;
	}

	virtual bool process_field(titleformat_text_out * p_out, const char * p_name, size_t p_name_length, bool & p_found_flag) override;
	virtual bool process_function(titleformat_text_out * p_out, const char * p_name, size_t p_name_length, titleformat_hook_function_params * p_params, bool & p_found_flag) override;
};
