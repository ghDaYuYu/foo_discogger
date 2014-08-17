#pragma once

#include "error_manager.h"


class file_info_manager;


class read_tags_completion : public completion_notify_dummy, public ErrorManager
{
private:
	const file_info_manager &helper;

public:
	void on_completion(unsigned p_code) override;

	read_tags_completion(const file_info_manager &helper) : helper(helper) {}
};


class write_tags_completion : public completion_notify_dummy, public ErrorManager
{
public:
	void on_completion(unsigned p_code) override;
};



class file_info_impl_wrapper
{
public:
	file_info_impl info;
	bool mask;
};
typedef std::shared_ptr<file_info_impl_wrapper> file_info_impl_wrapper_ptr;


class file_info_manager
{
public:
	metadb_handle_list items;
	pfc::array_t<file_info_impl_wrapper_ptr> info_wrappers;

	file_info_manager() {}
	file_info_manager(metadb_handle_list items) : items(items) {
		const size_t count = items.get_count();
		for (size_t i = 0; i < count; i++) {
			file_info_impl_wrapper_ptr info_wrapper = std::make_shared<file_info_impl_wrapper>();
			info_wrapper->mask = false;
			info_wrappers.append_single(std::move(info_wrapper));
		}
	};

	// TODO: replace with async version
	bool read_infos();
	void read_infos_new();

	void write_infos();

	void copy_from(file_info_manager &other, size_t index) {
		items.add_item(other.items[index]);
		info_wrappers.append_single(other.info_wrappers[index]);
	}
	
	inline  size_t get_item_count() const {
		return items.get_count();
	}

	inline bool is_item_valid(size_t p_index) const {
		return info_wrappers[p_index]->mask;
	}

	inline file_info_impl & get_item(size_t p_index) {
		return info_wrappers[p_index]->info;
	}

	inline metadb_handle_ptr get_item_handle(size_t p_index) const {
		return items[p_index];
	}

	inline void validate_item(size_t p_index) {
		info_wrappers[p_index]->mask = true;
	}

	inline void invalidate_item(size_t p_index) {
		info_wrappers[p_index]->mask = false;
	}

	inline void invalidate_all() {
		const size_t count = info_wrappers.get_count();
		for (size_t p_index = 0; p_index < count; p_index++) {
			info_wrappers[p_index]->mask = false;
		}
	}
};
typedef std::shared_ptr<file_info_manager> file_info_manager_ptr;
