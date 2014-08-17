#pragma once

#include "../../pfc/pfc.h"


namespace pfc {

	class uninitialized_exception : public std::exception
	{};


	template<template<typename> class t_alloc>
	class string8_t_ex : public pfc::string8_t<t_alloc>
	{
	private:
		using t_self = typename pfc::string8_t<t_alloc>::t_self;
	public:
		bool initialized = false;
		inline const t_self & operator= (const char * src) {
			set_string_(src); return *this;
		}
		inline const t_self & operator+= (const char * src) {
			add_string_(src); return *this;
		}
		inline const t_self & operator= (const string_base & src) {
			initialized = true;
			set_string(src); return *this;
		}
		inline const t_self & operator+= (const string_base & src) {
			initialized = true;
			add_string(src); return *this;
		}
		inline const t_self & operator= (const t_self & src) {
			initialized = true;
			set_string(src); return *this;
		}
		inline const t_self & operator= (const string8_t_ex & src) {
			initialized = src.initialized;
			if (initialized) {
				set_string(src);
			}
			return *this;
		}
		inline const t_self & operator+= (const t_self & src) {
			initialized = true;
			add_string(src); return *this;
		}

		inline const t_self & operator= (string_part_ref src) {
			initialized = true;
			set_string(src); return *this;
		}
		inline const t_self & operator+= (string_part_ref src) {
			initialized = true;
			add_string(src); return *this;
		}
		
		inline operator const char * () const {
			if (!initialized) {
				throw uninitialized_exception();
			}
			return _get_ptr();
		}

		string8_t_ex() {}
		string8_t_ex(const char * p_string) {
			initialized = true;
			set_string_(p_string);
		}
		string8_t_ex(const string8_t_ex & p_string) {
			initialized = p_string.initialized;
			if (initialized) {
				set_string_(p_string);
			}
		}
		string8_t_ex(const char * p_string, size_t p_length) {
			initialized = true;
			set_string(p_string, p_length);
		}
		string8_t_ex(const t_self & p_string) {
			initialized = true;
			set_string(p_string);
		}
		string8_t_ex(const string_base & p_string) {
			initialized = true;
			set_string(p_string);
		}
		string8_t_ex(string_part_ref ref) {
			initialized = true;
			set_string(ref);
		}

		const char * get_ptr() const override {
			return _get_ptr();
		}

		void force_reset() {
			initialized = false;
			pfc::string8_t<t_alloc>::force_reset();
		}
	};

	typedef string8_t_ex<pfc::alloc_standard> string8_ex;


	template<typename _t_item, template<typename> class t_alloc = pfc::alloc_standard>
	class array_t_ex : public pfc::array_t<_t_item, t_alloc>
	{
	public:
		using t_item = typename pfc::array_t<_t_item, t_alloc>::t_item;
	private:
		using t_self = typename pfc::array_t<_t_item, t_alloc>::t_self;
	public:
		bool initialized = false;
		array_t_ex() {}
		array_t_ex(const t_self & p_source) {
			initialized = true;
			copy_array_t(*this, p_source);
		}
		template<typename t_source>
		array_t_ex(const t_source & p_source) {
			initialized = true; 
			copy_array_t(*this, p_source);
		}
		const t_self & operator=(const t_self & p_source) {
			initialized = true;
			copy_array_t(*this, p_source); return *this;
		}
		template<typename t_source>
		const t_self & operator=(const t_source & p_source) {
			initialized = true; 
			copy_array_t(*this, p_source); return *this;
		}

		array_t_ex(t_self && p_source) {
			initialized = true; 
			move_from(p_source);
		}
		const t_self & operator=(t_self && p_source) {
			initialized = true; 
			move_from(p_source); return *this;
		}
		void force_reset() {
			initialized = false;
			pfc::array_t<_t_item, t_alloc>::force_reset();
		}

		template<typename t_array>
		void append(const t_array & p_source) {
			initialized = true;
			pfc::array_t<_t_item, t_alloc>::append(p_source);
		}

		template<typename t_insert>
		void insert_multi(const t_insert & value, size_t base, size_t count) {
			initialized = true;
			pfc::array_t<_t_item, t_alloc>::insert_multi(value, base, count);
		}
		template<typename t_append> void append_multi(const t_append & value, size_t count) {
			initialized = true; 
			pfc::array_t<_t_item, t_alloc>::insert_multi(value, ~0, count);
		}

		//! Warning: buffer pointer must not point to buffer allocated by this array (fixme).
		template<typename t_append>
		void append_fromptr(const t_append * p_buffer, size_t p_count) {
			initialized = true; 
			pfc::array_t<_t_item, t_alloc>::append_fromptr(p_buffer, p_count);
		}

		template<typename t_append>
		void append_single_val(t_append item) {
			initialized = true; 
			pfc::array_t<_t_item, t_alloc>::append_single_val(item);
		}

		template<typename t_append>
		void append_single(const t_append & p_item) {
			initialized = true; 
			pfc::array_t<_t_item, t_alloc>::append_single(p_item);
		}

		void set_size(size_t p_size) {
			initialized = true;
			pfc::array_t<_t_item, t_alloc>::set_size(p_size);
		}

		void assert_initialized() const {
			if (!initialized) {
				throw uninitialized_exception();
			}
		}
	};
}
