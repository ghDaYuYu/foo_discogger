#pragma once

#include "string_encoded_array.h"

class ExposesTags
{
public:
	bool loaded = false;
	virtual void load(threaded_process_status &p_status, abort_callback &p_abort, bool throw_all = false) { loaded = true; }
	virtual string_encoded_array get_data(pfc::string8 &tag_name, threaded_process_status &p_status, abort_callback &p_abort) = 0;
	virtual ~ExposesTags() {}
};
typedef std::shared_ptr<ExposesTags> ExposesTags_ptr;


template <class T>
class ExposedTags : public virtual ExposesTags
{
protected:
	const static std::map<const char*, std::pair<string_encoded_array(T::*)()const, void(T::*)(threaded_process_status&, abort_callback&, bool)>, cmp_str> exposed_tags;

public:
	static std::map<const char*, std::pair<string_encoded_array(ExposedTags::*)()const, void(ExposedTags::*)(threaded_process_status&, abort_callback&, bool)>, cmp_str> create_default_tags_map() {
		std::map<const char*, std::pair<string_encoded_array(ExposedTags::*)()const, void(ExposedTags::*)(threaded_process_status&, abort_callback&, bool)>, cmp_str> m;
		return m;
	}
	
	virtual string_encoded_array get_data(pfc::string8 &tag_name, threaded_process_status &p_status, abort_callback &p_abort) override;
	virtual string_encoded_array get_sub_data(pfc::string8 &tag_name, threaded_process_status &p_status, abort_callback &p_abort);

	inline bool has_data(pfc::string8 &tag_name) {
		return exposed_tags.count(tag_name.get_ptr()) != 0;
	}
};

template <class T>
using ExposedPair = std::pair<string_encoded_array(T::*)()const, void(T::*)(threaded_process_status&, abort_callback&, bool)>;

template <class T>
using ExposedMap = std::map<const char*, std::pair<string_encoded_array(T::*)()const, void(T::*)(threaded_process_status&, abort_callback&, bool)>, cmp_str>;



template <class T>
string_encoded_array ExposedTags<T>::get_data(pfc::string8 &tag_name, threaded_process_status &p_status, abort_callback &p_abort) {
	auto &it = exposed_tags.find(tag_name.get_ptr());
	if (it != exposed_tags.cend()) {
		auto pair = it->second;
		auto x = std::bind(pair.first, ((T*)this));
		auto load_check = pair.second;
		if (load_check != nullptr) {
			auto load_func = std::bind(pair.second, ((T*)this), std::ref(p_status), std::ref(p_abort), false);
			load_func();
		}
		
		pfc::string8 out = x();
		return out;
	}
	else {
		return get_sub_data(tag_name, p_status, p_abort);
	}
}


template <class T>
string_encoded_array ExposedTags<T>::get_sub_data(pfc::string8 &tag_name, threaded_process_status &p_status, abort_callback &p_abort) {
	Discogs::missing_data_exception ex;
	ex << "Unknown tag: " << tag_name;
	throw ex;
}
