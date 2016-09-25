#pragma once

#include "exception.h"
#include "../../pfc/pfc.h"
#include "pfc_helpers.h"


#define LIST_START	1
#define LIST_SEP	2
#define LIST_END	3

#define LIST_START_PRETTY	91
#define LIST_SEP_PRETTY	44
#define LIST_END_PRETTY	93

#define DEFAULT_JOIN_DELIM		", "


class string_encoding_exception : public foo_discogs_exception
{};


class string_encoded_array
{
private:
	pfc::array_t<string_encoded_array> sub_array;
	pfc::string8 value;
	bool dirty = false;
	size_t m_depth = ~0;

	bool branch_execute(bool(string_encoded_array::*)(), size_t depth = 0);
	bool branch_execute(bool(string_encoded_array::*)(const string_encoded_array&), const string_encoded_array &other1, size_t depth = 0, size_t other_depth = 0);
	bool branch_execute(bool(string_encoded_array::*)(const string_encoded_array&, const string_encoded_array&), const string_encoded_array &other1, const string_encoded_array &other2, size_t depth = 0, size_t other_depth = 0);

	bool _join(const string_encoded_array&);
	bool _joinnames(const string_encoded_array&);
	bool _zip(const string_encoded_array&);
	bool _zip2(const string_encoded_array&);
	bool _replace(const string_encoded_array&, const string_encoded_array&);
	bool _trim();
	bool _wrap(const string_encoded_array&);
	bool _pad(const string_encoded_array&, const string_encoded_array&);
	bool _pad_right(const string_encoded_array&, const string_encoded_array&);
	bool _strcmp(const string_encoded_array&);
	bool _istrcmp(const string_encoded_array&);
	bool _strstr(const string_encoded_array&);
	bool _any();
	bool _all();
	bool _sum();
	bool _multi_if(const string_encoded_array&, const string_encoded_array&);
	bool _multi_or(const string_encoded_array&);
	bool _multi_and(const string_encoded_array&);
	bool _multi_not();
	bool _extend(const string_encoded_array&);
	bool _append(const string_encoded_array&);
	bool _contains(const string_encoded_array&);
	bool _filter(const string_encoded_array&);
	//bool _reduce();
	bool _multi_len();
	bool _multi_longest(const string_encoded_array&);
	bool _multi_cut(const string_encoded_array&);
	bool _multi_left(const string_encoded_array&);
	bool _multi_right(const string_encoded_array&);
	bool _multi_greater(const string_encoded_array&);
	bool _multi_longer(const string_encoded_array&);
	bool _multi_add(const string_encoded_array&);
	bool _multi_sub(const string_encoded_array&);
	bool _multi_mul(const string_encoded_array&);
	bool _multi_div(const string_encoded_array&);
	bool _multi_mod(const string_encoded_array&);
	bool _multi_min(const string_encoded_array&);
	bool _multi_max(const string_encoded_array&);

	void array_param_too_deep(size_t pos) {
		string_encoding_exception ex;
		ex << "Array parameter (" << pos << ") is too deep.";
		throw ex;
	}

	void array_param_too_shallow(size_t pos, size_t depth, size_t required) {
		string_encoding_exception ex;
		ex << "Array parameter (" << pos << ") is too shallow. (" << depth << " need " << required << ")";
		throw ex;
	}

	void array_param_wrong_width(size_t pos, size_t depth, size_t width, size_t required) {
		string_encoding_exception ex;
		ex << "Array parameter (" << pos << ") wrong width at depth " << depth << ". (" << width << " need " << required << ")";
		throw ex;
	}

public:
	string_encoded_array();
	string_encoded_array(const char c);
	string_encoded_array(const pfc::string8_ex &str); 
	string_encoded_array(const pfc::string8 &str);
	string_encoded_array(const pfc::array_t_ex<pfc::string8> &arr);
	string_encoded_array(const pfc::array_t<pfc::string8> &arr);
	string_encoded_array::string_encoded_array(int i) : m_depth(0) {
		value << i;
	}
	string_encoded_array::string_encoded_array(unsigned i) : m_depth(0) {
		value << i;
	}
	string_encoded_array::string_encoded_array(const char *i) : m_depth(0) {
		value << i;
	}

	operator const char* () {
		if (dirty) {
			encode();
		}
		return value.get_ptr();
	}

	operator const char* () const {
		PFC_ASSERT(!dirty);
		return value.get_ptr();
	}

	inline pfc::string8 get_value() {
		if (dirty) {
			encode();
		}
		return value;
	}
	inline const pfc::string8& get_cvalue() {
		if (dirty) {
			encode();
		}
		return value;
	}
	inline const pfc::string8& get_pure_cvalue() const {
		PFC_ASSERT(!dirty);
		return value;
	}

	inline string_encoded_array get_item(size_t i) const {
		PFC_ASSERT(has_array() && i < sub_array.get_size());
		return sub_array[i];
	}
	inline const string_encoded_array& get_citem(size_t i) const {
		PFC_ASSERT(has_array() && i < sub_array.get_size());
		return sub_array[i];
	}
	inline string_encoded_array& get_item_unsafe(size_t i) {
		PFC_ASSERT(has_array() && i < sub_array.get_size());
		return sub_array[i];
	}
	inline const pfc::array_t<string_encoded_array>& get_citems() const {
		return sub_array;
	}

	inline int get_numeric_value() const {
		if (!has_array() && pfc::string_is_numeric(value)) {
			return std::stoi(value.get_ptr());
		}
		string_encoding_exception ex;
		ex << "Invalid numeric parameter: " << value;
		throw ex;
	}

	void set_value(const char c);
	void set_value(const char *str);
	void set_value(const pfc::string8_ex &str); 
	void set_value(const pfc::string8 &str);
	void set_value(const pfc::array_t_ex<pfc::string8> &arr); 
	void set_value(const pfc::array_t<pfc::string8> &arr);

	void append_item(const string_encoded_array &v);
	void append_item_val(const string_encoded_array v);
	void set_item(size_t i, string_encoded_array &v);

	// encodes array into value string
	void encode();

	inline bool has_array() const {
		return m_depth > 0 && m_depth != ~0;
		//return sub_array.get_size() != 0;
	}

	inline size_t get_width() const {
		return sub_array.get_size();
	}

	inline size_t get_depth() const {
		return m_depth;
	}

	inline void force_reset() {
		sub_array.force_reset();
		value = "";
		m_depth = ~0;
		dirty = false;
	}

	// limits depth of array by squashing it with ", " delimiter
	bool limit_depth(size_t depth);
	
	// expands depth to fill in empty values
	bool expand_depth(size_t depth);

	void split(const pfc::string8 &delim = DEFAULT_JOIN_DELIM);
	void increase_width(size_t length);
	
	void unique();
	void flatten();
	void force_array();

	inline void join(const pfc::string8 &delim = DEFAULT_JOIN_DELIM) {
		branch_execute(&string_encoded_array::_join, delim, 1);
	}
	inline void join(const string_encoded_array &other) {
		branch_execute(&string_encoded_array::_join, other, 1);
	}
	inline void joinnames(const string_encoded_array &other) {
		branch_execute(&string_encoded_array::_joinnames, other, 1);
	}
	inline void zip(const string_encoded_array &other) {
		branch_execute(&string_encoded_array::_zip, other, 0);
	}
	inline void zip2(const string_encoded_array &other) {
		branch_execute(&string_encoded_array::_zip2, other, 0);
	}
	inline void replace(const string_encoded_array &find, const string_encoded_array &with) {
		branch_execute(&string_encoded_array::_replace, find, with, 0);
	}
	inline void trim() {
		branch_execute(&string_encoded_array::_trim, 0);
	}
	inline void wrap(const string_encoded_array &other) {
		branch_execute(&string_encoded_array::_wrap, other);
	}
	inline void pad(const string_encoded_array &max_len, const string_encoded_array &padchar = ' ') {
		branch_execute(&string_encoded_array::_pad, max_len, padchar);
	}
	inline void pad_right(const string_encoded_array &max_len, const string_encoded_array &padchar = ' ') {
		branch_execute(&string_encoded_array::_pad_right, max_len, padchar);
	}
	inline void strcmp(const string_encoded_array &other) {
		branch_execute(&string_encoded_array::_strcmp, other, 0);
	}
	inline void istrcmp(const string_encoded_array &other) {
		branch_execute(&string_encoded_array::_istrcmp, other, 0);
	}
	inline void strstr(const string_encoded_array &other) {
		branch_execute(&string_encoded_array::_strstr, other, 0);
	}
	inline void any() {
		branch_execute(&string_encoded_array::_any, 1);
	}
	inline void all() {
		branch_execute(&string_encoded_array::_all, 1);
	}
	inline void sum() {
		branch_execute(&string_encoded_array::_sum, 1);
	}
	inline void multi_if(const string_encoded_array &yes, const string_encoded_array &no) {
		branch_execute(&string_encoded_array::_multi_if, yes, no);
	}
	inline void multi_or(const string_encoded_array &other) {
		branch_execute(&string_encoded_array::_multi_or, other);
	}
	inline void multi_and(const string_encoded_array &other) {
		branch_execute(&string_encoded_array::_multi_and, other);
	}
	inline void multi_not() {
		branch_execute(&string_encoded_array::_multi_not);
	}
	inline void extend(const string_encoded_array &other) {
		branch_execute(&string_encoded_array::_extend, other, 1);
	}
	inline void append(const string_encoded_array &other) {
		branch_execute(&string_encoded_array::_append, other, 1);
	}
	inline void shallow_extend(const string_encoded_array &other) {
		branch_execute(&string_encoded_array::_extend, other, m_depth, m_depth);
	}
	inline void shallow_append(const string_encoded_array &other) {
		branch_execute(&string_encoded_array::_append, other, m_depth, m_depth);
	}
	inline void contains(const string_encoded_array &other) {
		branch_execute(&string_encoded_array::_contains, other, 1, 1);
	}
	inline void filter(const string_encoded_array &other) {
		branch_execute(&string_encoded_array::_filter, other, 1, 1);
	}
	/*inline void reduce() {
		branch_execute(&string_encoded_array::_reduce, 2);
	}*/
	inline void multi_len() {
		branch_execute(&string_encoded_array::_multi_len);
	}
	inline void multi_longest(const string_encoded_array &other) {
		branch_execute(&string_encoded_array::_multi_longest, other);
	}
	inline void multi_cut(const string_encoded_array &other) {
		branch_execute(&string_encoded_array::_multi_cut, other);
	}
	inline void multi_left(const string_encoded_array &other) {
		branch_execute(&string_encoded_array::_multi_left, other);
	}
	inline void multi_right(const string_encoded_array &other) {
		branch_execute(&string_encoded_array::_multi_right, other);
	}
	inline void multi_greater(const string_encoded_array &other) {
		branch_execute(&string_encoded_array::_multi_greater, other);
	}
	inline void multi_longer(const string_encoded_array &other) {
		branch_execute(&string_encoded_array::_multi_longer, other);
	}
	inline void multi_add(const string_encoded_array &other) {
		branch_execute(&string_encoded_array::_multi_add, other);
	}
	inline void multi_sub(const string_encoded_array &other) {
		branch_execute(&string_encoded_array::_multi_sub, other);
	}
	inline void multi_mul(const string_encoded_array &other) {
		branch_execute(&string_encoded_array::_multi_mul, other);
	}
	inline void multi_div(const string_encoded_array &other) {
		branch_execute(&string_encoded_array::_multi_div, other);
	}
	inline void multi_mod(const string_encoded_array &other) {
		branch_execute(&string_encoded_array::_multi_mod, other);
	}
	inline void multi_min(const string_encoded_array &other) {
		branch_execute(&string_encoded_array::_multi_min, other);
	}
	inline void multi_max(const string_encoded_array &other) {
		branch_execute(&string_encoded_array::_multi_max, other);
	}

	pfc::string8 print() const;
	pfc::string8 print_raw() const;
};
