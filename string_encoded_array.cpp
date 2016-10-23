#include "stdafx.h"

#include "string_encoded_array.h"
#include "utils.h"



inline void PRINTDOUBLE(pfc::string8 &v, double d) {
	std::ostringstream s;
	s << d;
	v << pfc::string8(s.str().c_str());
}


string_encoded_array::string_encoded_array() {
}

string_encoded_array::string_encoded_array(const char c) {
	set_value(c);
}

string_encoded_array::string_encoded_array(const pfc::string8_ex &str) {
	set_value(str);
}

string_encoded_array::string_encoded_array(const pfc::string8 &str) {
	set_value(str);
}

string_encoded_array::string_encoded_array(const pfc::array_t_ex<pfc::string8> &arr) {
	set_value(arr);
}

string_encoded_array::string_encoded_array(const pfc::array_t<pfc::string8> &arr) {
	set_value(arr);
}

void string_encoded_array::set_value(const pfc::string8_ex &str) {
	set_value((const char*)str);
}

void string_encoded_array::set_value(const pfc::string8 &str) {
	set_value((const char*)str);
}

void string_encoded_array::set_value(const char c) {
	value.force_reset();
	value.add_char(c);
	m_depth = 0;
}

void string_encoded_array::set_value(const char *str) {
	value = str;
	sub_array.force_reset();
	const size_t length = strlen(str);
	if (str[0] == LIST_START && str[length - 1] == LIST_END) {
		pfc::string8 item = "";
		int depth = 1;
		int max_depth = 1;
		for (size_t pos = 1; pos < length - 1; pos++) {
			if (str[pos] == LIST_START) {
				if (depth == 0) {
					string_encoding_exception ex;
					ex << "Tried decoding corrupt string: embedded start";
					throw ex;
				}
				depth++;
				max_depth = max(max_depth, depth);
			}
			else if (str[pos] == LIST_END) {
				depth--;
				if (depth < 0) {
					string_encoding_exception ex;
					ex << "Tried decoding corrupt string: depth < 0";
					throw ex;
				}
			}
			else if (str[pos] == LIST_SEP && depth == 1) {
				sub_array.append_single(string_encoded_array(item));
				item.reset();
				continue;
			}
			item.add_byte(str[pos]);
		}
		if (depth != 1) {
			string_encoding_exception ex;
			ex << "Tried decoding corrupt string: depth > 0";
			throw ex;
		}
		sub_array.append_single(string_encoded_array(item));
		m_depth = max_depth;
	}
	else {
		for (size_t i = 0; i < length; i++) {
			if (str[i] == LIST_START || str[i] == LIST_END || str[i] == LIST_SEP) {
				string_encoding_exception ex;
				ex << "Corrupted array string: missing start byte";
				throw ex;
			}
		}
		m_depth = 0;
	}
}


void string_encoded_array::set_value(const pfc::array_t_ex<pfc::string8> &arr) {
	arr.assert_initialized();
	set_value(static_cast<const pfc::array_t<pfc::string8>>(arr));
}

void string_encoded_array::set_value(const pfc::array_t<pfc::string8> &arr) {
	dirty = true;
	const size_t count = arr.get_size();
	sub_array.set_size(count);
	size_t max_depth = 0;
	for (size_t i = 0; i < count; i++) {
		string_encoded_array add(arr[i]);
		sub_array[i] = add;
		max_depth = max(max_depth, add.get_depth());
	}
	m_depth = max_depth + 1;
}

void string_encoded_array::append_item(const string_encoded_array &v) {
	if (m_depth == 0) {
		m_depth = 1;
		sub_array.append_single(value);
	}
	if (m_depth == ~0) {
		m_depth = v.m_depth + 1;
	}
	else if (v.get_depth() + 1 != m_depth) {
		string_encoding_exception ex;
		ex << "Append array item of depth " << v.m_depth << ", required " << m_depth - 1;
		throw ex;
	}
	sub_array.append_single(v);
	dirty = true;
}

void string_encoded_array::append_item_val(const string_encoded_array v) {
	if (m_depth == 0) {
		m_depth = 1;
		sub_array.append_single(value);
	}
	if (m_depth == ~0) {
		m_depth = v.m_depth + 1;
	}
	else if (v.m_depth + 1 != m_depth) {
		string_encoding_exception ex;
		ex << "Append array item of depth " << v.m_depth << ", required " << m_depth - 1;
		throw ex;
	}
	sub_array.append_single(v);
	dirty = true;
}

void string_encoded_array::set_item(size_t i, string_encoded_array &v) {
	PFC_ASSERT(has_array());
	if (m_depth == ~0) {
		m_depth = v.m_depth + 1;
	}
	else if (v.m_depth + 1 != m_depth) {
		string_encoding_exception ex;
		ex << "Set array item of depth " << v.m_depth << ", required " << m_depth - 1;
		throw ex;
	}
	sub_array[i] = v;
	dirty = true;
}

void string_encoded_array::encode() {
	if (!dirty) {
		return;
	}
	if (has_array()) {
		value.force_reset();
		value.add_byte(LIST_START);
		const size_t count = sub_array.get_size();
		for (size_t i = 0; i < count; i++) {
			sub_array[i].encode();
			value.add_string(sub_array[i].get_cvalue());
			if (i != count - 1) {
				value.add_byte(LIST_SEP);
			}
		}
		value.add_byte(LIST_END);
		dirty = false;
	}
}

bool string_encoded_array::limit_depth(size_t depth) {
	if (m_depth > depth) {
		bool changed = false;
		if (depth == 0) {
			join();
			sub_array.force_reset();
			dirty = false;
			changed = true;
		}
		else {
			for (size_t i = 0; i < sub_array.get_size(); i++) {
				bool _changed = sub_array[i].limit_depth(depth - 1);
				changed |= _changed;
			}
			if (changed) {
				dirty = true;
			}
		}
		m_depth = depth;
		return changed;
	}
	return false;
}

bool string_encoded_array::expand_depth(size_t depth) {
	if (depth != 0) {
		bool changed = false;
		if (get_width() == 0) {
			string_encoded_array x("");
			sub_array.append_single(x);
			changed = true;
			m_depth = 1;
		}
		if (m_depth) {
			for (size_t i = 0; i < sub_array.get_size(); i++) {
				bool _changed = sub_array[i].expand_depth(depth - 1);
				changed |= _changed;
			}
			if (changed) {
				dirty = true;
			}
		}
		m_depth = depth;
		return changed;
	}
	return false;
}

void string_encoded_array::split(const pfc::string8 &delim) {
	pfc::array_t<pfc::string8> tokens;
	tokenize(value, delim, tokens, false);
	for (size_t i = 0; i < tokens.get_size(); i++) {
		append_item(tokens[i]);
	}
	dirty = true;
	m_depth = 1;
}

void string_encoded_array::increase_width(size_t length) {
	sub_array.set_size(length);
	for (size_t i = 1; i < length; i++) {
		sub_array[i] = sub_array[0];
	}
}

bool string_encoded_array::branch_execute(bool(string_encoded_array::*func)(), size_t depth) {
	if (m_depth > depth) {
		const size_t count = get_width();
		bool _changed = false;
		for (size_t i = 0; i < count; i++) {
			bool changed = sub_array[i].branch_execute(func, depth);
			if (sub_array[i].has_array()) {
				sub_array[i].dirty = changed;
			}
			else {
				sub_array[i].dirty = false;
			}
			if (changed) {
				m_depth = sub_array[i].m_depth + 1;
			}
			_changed |= changed;
		}
		dirty |= _changed;
		return _changed;
	}
	else if (m_depth == depth) {
		bool _dirty = (this->*func)();
		if (!has_array()) {
			dirty = false;
		}
		else {
			dirty = _dirty;
		}
		return _dirty;
	}
	else {
		return false;
	}
}

bool string_encoded_array::branch_execute(bool(string_encoded_array::*func)(const string_encoded_array&), const string_encoded_array &other1, size_t depth, size_t other_depth) {
	const bool array1 = other1.has_array();
	if (m_depth > depth) {
		const size_t count = get_width();
		if (other_depth > other1.m_depth) {
			array_param_too_shallow(2, other1.m_depth, other_depth);
		}
		if (array1 && other1.m_depth > other_depth && other1.get_width() != count) {
			array_param_wrong_width(2, other1.m_depth, other1.get_width(), count);
		}
		const string_encoded_array *p1 = &other1;
		bool _changed = false;
		for (size_t i = 0; i < count; i++) {
			if (array1 && other1.m_depth > other_depth) {
				p1 = &other1.get_citem(i);
			}
			bool changed = sub_array[i].branch_execute(func, *p1, depth, other_depth);
			if (sub_array[i].has_array()) {
				sub_array[i].dirty = changed;
			}
			else {
				sub_array[i].dirty = false;
			}
			if (changed) {
				m_depth = sub_array[i].m_depth + 1;
			}
			_changed |= changed;
		}
		dirty |= _changed;
		return _changed;
	}
	else if (m_depth == depth) {
		bool _dirty = (this->*func)(other1);
		if (!has_array()) {
			dirty = false;
		}
		else {
			dirty = _dirty;
		}
		return _dirty;
	}
	else {
		return false;
	}
}

bool string_encoded_array::branch_execute(bool(string_encoded_array::*func)(const string_encoded_array&, const string_encoded_array&), const string_encoded_array &other1, const string_encoded_array &other2, size_t depth, size_t other_depth) {
	const bool array1 = other1.has_array();
	const bool array2 = other2.has_array();
	if (m_depth > depth) {
		const size_t count = get_width();
		
		if (other_depth > other1.m_depth) {
			array_param_too_shallow(2, other1.m_depth, other_depth);
		}
		if (other_depth > other2.m_depth) {
			array_param_too_shallow(3, other1.m_depth, other_depth);
		}
		if (array1 && other1.m_depth > other_depth && other1.get_width() != count) {
			array_param_wrong_width(2, other1.m_depth, other1.get_width(), count);
		}
		else if (array2 && other2.m_depth > other_depth && other2.get_width() != count) {
			array_param_wrong_width(3, other2.m_depth, other2.get_width(), count);
		}
		const string_encoded_array *p1 = &other1;
		const string_encoded_array *p2 = &other2;
		bool _changed = false;
		for (size_t i = 0; i < count; i++) {
			if (array1 && other1.m_depth > other_depth) {
				p1 = &other1.get_citem(i);
			}
			if (array2 && other2.m_depth > other_depth) {
				p2 = &other2.get_citem(i);
			}
			bool changed = sub_array[i].branch_execute(func, *p1, *p2, depth);
			if (sub_array[i].has_array()) {
				sub_array[i].dirty = changed;
			}
			else {
				sub_array[i].dirty = false;
			}
			changed |= sub_array[i].dirty;
			if (changed) {
				m_depth = sub_array[i].m_depth + 1;
			}
			_changed |= changed;
		}
		dirty |= _changed;
		return _changed;
	}
	else if (m_depth == depth) {
		bool changed = (this->*func)(other1, other2);
		if (!has_array()) {
			dirty = false;
		}
		else {
			dirty = changed;
		}
		return changed;
	}
	else {
		return false;
	}
}

// TODO: this unique is at depth D, make one at depth 1 as well...

void string_encoded_array::unique() {
	if (has_array()) {
		encode();
		size_t i = 1;
		size_t count = get_width();
		while (i < count) {
			bool duplicate = false;
			for (size_t j = 0; j < i; j++) {
				if (STR_EQUAL(sub_array[j].value, sub_array[i].value)) {
					duplicate = true;
					break;
				}
			}
			if (duplicate) {
				for (size_t j = i + 1; j < sub_array.get_size(); j++) {
					sub_array[j - 1] = sub_array[j];
				}
				sub_array.set_size(sub_array.get_size() - 1);
				count--;
				dirty = true;
				continue;
			}
			i++;
		}
	}
}

void string_encoded_array::flatten() {
	if (has_array()) {
		const size_t count = get_width();
		pfc::array_t<string_encoded_array> new_sub_array;
		for (size_t i = 0; i < count; i++) {
			if (sub_array[i].has_array()) {
				dirty = true;
				sub_array[i].flatten();
				for (size_t j = 0; j < sub_array[i].get_width(); j++) {
					new_sub_array.append_single(sub_array[i].get_item(j));
				}
			}
			else {
				new_sub_array.append_single(sub_array[i]);
			}
		}
		sub_array = new_sub_array;
	}
}

void string_encoded_array::force_array() {
	if (m_depth == 0) {
		sub_array.append_single(string_encoded_array(value));
		m_depth = 1;
		dirty = true;
	}
	else if (m_depth == ~0) {
		m_depth = 1;
		dirty = true;
	}
}

bool string_encoded_array::_join(const string_encoded_array &delim) {
	PFC_ASSERT(m_depth == 1);
	if (delim.has_array() && delim.m_depth != m_depth) {
		array_param_too_deep(2);
	}
	value.force_reset();
	bool first = true;
	const string_encoded_array *pd = &delim;
	const size_t count = sub_array.get_size();
	for (size_t i = 0; i < count; i++) {
		pfc::string8 sub_item = sub_array[i].value;
		if (!sub_item.get_length()) {  // filter out empty strings when force squashing array
			continue;
		}
		if (!first) {
			if (delim.has_array()) {
				pd = &delim.get_citem(delim.get_width() % i);
			}
			value.add_string(pd->value);
		}
		first = false;
		value.add_string(sub_item);
	}
	sub_array.force_reset();
	m_depth = 0;
	return true;
}

bool string_encoded_array::_joinnames(const string_encoded_array &delim) {
	PFC_ASSERT(m_depth == 1);
	if (delim.has_array() && delim.m_depth != m_depth) {
		array_param_too_deep(2);
	}
	value.force_reset();
	bool first = true;
	const string_encoded_array *pd = &delim;
	const size_t count = sub_array.get_size();
	for (size_t i = 0; i < count; i++) {
		pfc::string8 sub_item = sub_array[i].value;
		if (!sub_item.get_length()) {  // filter out empty strings when force squashing array
			continue;
		}
		if (!first) {
			if (delim.has_array()) {
				size_t j = i - 1;
				if (j >= delim.get_width()) {
					j = j - delim.get_width();
				}
				pd = &delim.get_citem(j);
			}
			if (!STR_EQUAL(pd->value, ",")) {
				value.add_char(' ');
			}
			value.add_string(pd->value);
			value.add_char(' ');
		}
		first = false;
		value.add_string(sub_item);
	}
	sub_array.force_reset();
	m_depth = 0;
	return true;
}

bool string_encoded_array::_zip(const string_encoded_array &other) {
	PFC_ASSERT(m_depth == 0);
	if (other.m_depth != m_depth) {
		array_param_too_deep(2);
	}
	// don't zip when left side is empty string.
	if (value.get_length() && other.value.get_length()) {
		value << other.value;
		return true;
	}
	return false;
}

bool string_encoded_array::_zip2(const string_encoded_array &other) {
	PFC_ASSERT(m_depth == 0);
	if (other.m_depth != m_depth) {
		array_param_too_deep(2);
	}
	value << other.value;
	return true;
}

bool string_encoded_array::_replace(const string_encoded_array &find, const string_encoded_array &with) {
	PFC_ASSERT(m_depth == 0);
	if (find.m_depth != m_depth) {
		array_param_too_deep(2);
	}
	else if (with.m_depth != m_depth) {
		array_param_too_deep(3);
	}
	return value.replace_string(find.value, with.value) != 0;
}

bool string_encoded_array::_trim() {
	PFC_ASSERT(m_depth == 0);
	pfc::string8 old = value;
	value = ::trim(value);
	return !STR_EQUAL(value, old);
}

bool string_encoded_array::_wrap(const string_encoded_array &other) {
	PFC_ASSERT(m_depth == 0);
	if (other.m_depth != m_depth) {
		array_param_too_deep(2);
	}
	pfc::string8 temp;
	temp << other.get_pure_cvalue();
	temp << get_cvalue();
	temp << other.get_pure_cvalue();
	value = temp;
	return true;
}

bool string_encoded_array::_pad(const string_encoded_array &max_len, const string_encoded_array &padchar) {
	PFC_ASSERT(m_depth == 0);
	if (max_len.m_depth != m_depth) {
		array_param_too_deep(2);
	}
	if (padchar.m_depth != m_depth) {
		array_param_too_deep(3);
	}
	const size_t max_length = max_len.get_numeric_value();
	const size_t count = value.get_length();
	if (count < max_length) {
		const char pad_char = padchar.value[0];
		for (size_t i = 0; i < max_length - count; i++) {
			value.add_char(pad_char);
		}
		return true;
	}
	return false;
}

bool string_encoded_array::_pad_right(const string_encoded_array &max_len, const string_encoded_array &padchar) {
	PFC_ASSERT(m_depth == 0);
	if (max_len.m_depth != m_depth) {
		array_param_too_deep(2);
	}
	if (padchar.m_depth != m_depth) {
		array_param_too_deep(3);
	}
	const size_t max_length = max_len.get_numeric_value();
	pfc::string8 temp;
	const size_t count = value.get_length();
	const char pad_char = padchar.value[0];
	if (count < max_length) {
		for (size_t i = 0; i < max_length - count; i++) {
			temp.add_char(pad_char);
		}
		temp << value;
		value = temp;
		return true;
	}
	return false;
}

bool string_encoded_array::_strcmp(const string_encoded_array &other) {
	PFC_ASSERT(m_depth == 0);
	if (other.m_depth != m_depth) {
		array_param_too_deep(2);
	}
	if (STR_EQUAL(value, other.value)) {
		value = "1";
	}
	else {
		value = "";
	}
	return true;
}

bool string_encoded_array::_istrcmp(const string_encoded_array &other) {
	PFC_ASSERT(m_depth == 0);
	if (other.m_depth != m_depth) {
		array_param_too_deep(2);
	}
	if (pfc::stricmp_ascii(value, other.value) == 0) {
		value = "1";
	}
	else {
		value = "";
	}
	return true;
}

bool string_encoded_array::_strstr(const string_encoded_array &other) {
	PFC_ASSERT(m_depth == 0);
	if (other.m_depth != m_depth) {
		array_param_too_deep(2);
	}
	if (value.find_first(other.value) != pfc::infinite_size) {
		value = "1";
	}
	else {
		value = "";
	}
	return true;
}

bool string_encoded_array::_any() {
	PFC_ASSERT(m_depth == 1);
	bool result = false;
	const size_t count = get_width();
	for (size_t i = 0; i < count; i++) {
		if (sub_array[i].value.get_length() && !(sub_array[i].value.get_length() == 1 && sub_array[i].value[0] == '0')) {
			result = true;
			break;
		}
	}
	sub_array.force_reset();
	if (result) {
		value = "1";
	}
	else {
		value = "";
	}
	m_depth = 0;
	return true;
}

bool string_encoded_array::_all() {
	PFC_ASSERT(m_depth == 1);
	bool result = true;
	const size_t count = get_width();
	for (size_t i = 0; i < count; i++) {
		if (!(sub_array[i].value.get_length() && !(sub_array[i].value.get_length() == 1 && sub_array[i].value[0] == '0'))) {
			result = false;
			break;
		}
	}
	sub_array.force_reset();
	if (result) {
		value = "1";
	}
	else {
		value = "";
	}
	m_depth = 0;
	return true;
}

bool string_encoded_array::_count() {
	PFC_ASSERT(m_depth == 1);
	unsigned result = 0;
	const size_t count = get_width();
	for (size_t i = 0; i < count; i++) {
		if (sub_array[i].value.get_length() && !(sub_array[i].value.get_length() == 1 && sub_array[i].value[0] == '0')) {
			result++;
		}
	}
	sub_array.force_reset();
	if (result) {
		value = "";
		value << result;
	}
	else {
		value = "";
	}
	m_depth = 0;
	return true;
}

bool string_encoded_array::_multi_if(const string_encoded_array &yes, const string_encoded_array &no) {
	PFC_ASSERT(m_depth == 0);
	/*if (yes.m_depth != m_depth) {
		array_param_too_deep(2);
	}
	else if (no.m_depth != m_depth) {
		array_param_too_deep(3);
	}*/
	if (value.get_length() && !(value.get_length() == 1 && value[0] == '0')) {
		value = yes.value;
	}
	else {
		value = no.value;
	}
	return true;
}

bool string_encoded_array::_multi_or(const string_encoded_array &other) {
	PFC_ASSERT(m_depth == 0);
	/*if (other.m_depth != m_depth) {
		array_param_too_deep(2);
	}*/
	if (value.get_length() && !(value.get_length() == 1 && value[0] == '0')) {
	}
	else if (other.value.get_length() && !(other.value.get_length() == 1 && other.value[0] == '0')) {
		value = other.value;
	}
	else {
		value = "";
	}
	return true;
}

bool string_encoded_array::_multi_and(const string_encoded_array &other) {
	PFC_ASSERT(m_depth == 0);
	/*if (other.m_depth != m_depth) {
	array_param_too_deep(2);
	}*/
	if (value.get_length() && !(value.get_length() == 1 && value[0] == '0') &&
		other.value.get_length() && !(other.value.get_length() == 1 && other.value[0] == '0')) {
		value = "1";
	}
	else {
		value = "";
	}
	return true;
}

bool string_encoded_array::_multi_not() {
	PFC_ASSERT(m_depth == 0);
	if (value.get_length() && !(value.get_length() == 1 && value[0] == '0')) {
		value = "";
	}
	else {
		value = "1";
	}
	return true;
}

bool string_encoded_array::_extend(const string_encoded_array &other) {
	//PFC_ASSERT(m_depth == 1);
	if (other.m_depth != m_depth) {
		array_param_too_deep(2);
	}
	const size_t count = get_width();
	const size_t new_count = other.get_width();
	sub_array.set_size(count + new_count);
	for (size_t i = 0; i < new_count; i++) {
		sub_array[count + i] = other.get_citem(i);
	}
	return count != 0;
}

bool string_encoded_array::_append(const string_encoded_array &other) {
	//PFC_ASSERT(m_depth == 1);
	if (other.m_depth != m_depth - 1) {
		array_param_too_deep(2);
	}
	sub_array.append_single(other);
	return true;
}

bool string_encoded_array::_contains(const string_encoded_array &other) {
	PFC_ASSERT(m_depth == 1);
	const size_t count = get_width(); 
	if (other.m_depth == 0) {
		bool result = false;
		for (size_t i = 0; i < count; i++) {
			if (STR_EQUAL(sub_array[i], other.value)) {
				result = true;
				break;
			}
		}
		set_value(result ? "1" : "");
		m_depth = 0;
	}
	else {
		if (other.m_depth != 1) {
			array_param_too_deep(2);
		}
		const size_t other_count = other.get_width();
		pfc::array_t<bool> results;
		results.set_size(other_count);
		for (size_t i = 0; i < other_count; i++) {
			results[i] = false;
			for (size_t j = 0; j < count; j++) {
				if (STR_EQUAL(sub_array[j], other.sub_array[i])) {
					results[i] = true;
					break;
				}
			}
		}
		sub_array.force_reset();
		for (size_t i = 0; i < other_count; i++) {
			sub_array.append_single(pfc::string8(results[i] ? "1" : "0"));
		}
	}
	return true;
}

bool string_encoded_array::_filter(const string_encoded_array &other) {
	PFC_ASSERT(m_depth == 1);
	if (other.m_depth != 0) {
		array_param_too_deep(2);
	}
	size_t count = get_width();
	bool changed = false;
	if (other.m_depth == 0) {
		for (size_t i = 0; i < count; i++) {
			if (STR_EQUAL(sub_array[i], other.value)) {
				for (size_t j = i + 1; j < count; j++) {
					sub_array[j - 1] = sub_array[j];
				}
				count--;
				sub_array.set_size(count);
				changed = true;
			}
		}
	}
	/*else {
		if (other.m_depth != 1) {
			array_param_too_deep(2);
		}
		const size_t other_count = other.get_width();
		for (size_t i = 0; i < other_count; i++) {
			size_t inner_count = count;
			for (size_t j = 0; j < inner_count; j++) {
				if (STR_EQUAL(sub_array[j], other.sub_array[i])) {
					for (size_t k = j + 1; k < inner_count; k++) {
						sub_array[k - 1] = sub_array[k];
					}
					inner_count--;
					sub_array.set_size(count);
					changed = true;
				}
			}
		}
	}*/
	return changed;
}

/*
bool string_encoded_array::_reduce() {
	PFC_ASSERT(m_depth == 2);
	pfc::array_t<string_encoded_array> next;
	const size_t count = get_width();
	for (size_t i = 0; i < count; i++) {
		const size_t inner_count = sub_array[i].get_width();
		for (size_t j = 0; j < inner_count; j++) {
			next.append_single(sub_array[i][j]);
		}
	}
	sub_array = next;
	m_depth = 1;
	return true;
}
*/

bool string_encoded_array::_multi_len() {
	PFC_ASSERT(m_depth == 0);
	size_t length = value.get_length();
	value.force_reset();
	value << length;
	return true;
}

bool string_encoded_array::_multi_longest(const string_encoded_array& other) {
	PFC_ASSERT(m_depth == 0);
	if (other.m_depth) {
		array_param_too_deep(2);
	}
	if (value.get_length() < other.value.get_length()) {
		value = other.value;
		return true;
	}
	else {
		return false;
	}
}

// Same as left?
bool string_encoded_array::_multi_cut(const string_encoded_array& other) {
	PFC_ASSERT(m_depth == 0);
	if (other.m_depth) {
		array_param_too_deep(2);
	}
	size_t len = value.get_length();
	size_t maxlen = other.get_numeric_value();
	if (maxlen >= len) {
		return false;
	}
	value = substr(value, 0, maxlen);
	return true;
}

bool string_encoded_array::_multi_left(const string_encoded_array& other) {
	PFC_ASSERT(m_depth == 0);
	if (other.m_depth) {
		array_param_too_deep(2);
	}
	size_t len = value.get_length();
	size_t ask = other.get_numeric_value();
	if (ask >= len) {
		return false;
	}
	value = substr(value, 0, ask);
	return true;
}

bool string_encoded_array::_multi_right(const string_encoded_array& other) {
	PFC_ASSERT(m_depth == 0);
	if (other.m_depth) {
		array_param_too_deep(2);
	}
	size_t len = value.get_length();
	size_t ask = other.get_numeric_value();
	if (ask >= len) {
		return false;
	}
	value = substr(value, len - ask);
	return true;
}

bool string_encoded_array::_multi_greater(const string_encoded_array& other) {
	PFC_ASSERT(m_depth == 0);
	if (other.m_depth) {
		array_param_too_deep(2);
	}
	int me_num = get_numeric_value();
	int other_num = other.get_numeric_value();
	if (me_num > other_num) {
		value = "1";
	}
	else {
		value = "";
	}
	return true;
}

bool string_encoded_array::_multi_longer(const string_encoded_array& other) {
	PFC_ASSERT(m_depth == 0);
	if (other.m_depth) {
		array_param_too_deep(2);
	}
	if (value.get_length() < other.value.get_length()) {
		value = "1";
	}
	else {
		value = "";
	}
	return true;
}

bool string_encoded_array::_multi_add(const string_encoded_array& other) {
	PFC_ASSERT(m_depth == 0);
	if (other.m_depth) {
		array_param_too_deep(2);
	}
	double me_num = get_numeric_double_value();
	double other_num = other.get_numeric_double_value();
	value.force_reset();
	PRINTDOUBLE(value, me_num + other_num);
	return true;
}

bool string_encoded_array::_multi_sub(const string_encoded_array& other) {
	PFC_ASSERT(m_depth == 0);
	if (other.m_depth) {
		array_param_too_deep(2);
	}
	double me_num = get_numeric_double_value();
	double other_num = other.get_numeric_double_value();
	value.force_reset();
	PRINTDOUBLE(value, me_num - other_num);
	return true;
}

bool string_encoded_array::_multi_mul(const string_encoded_array& other) {
	PFC_ASSERT(m_depth == 0);
	if (other.m_depth) {
		array_param_too_deep(2);
	}
	double me_num = get_numeric_double_value();
	double other_num = other.get_numeric_double_value();
	value.force_reset();
	PRINTDOUBLE(value, me_num * other_num);
	return true;
}

bool string_encoded_array::_multi_div(const string_encoded_array& other) {
	PFC_ASSERT(m_depth == 0);
	if (other.m_depth) {
		array_param_too_deep(2);
	}
	int me_num = get_numeric_value();
	int other_num = other.get_numeric_value();
	value.force_reset();
	value << (value, me_num / other_num);
	return true;
}

bool string_encoded_array::_multi_divd(const string_encoded_array& other) {
	PFC_ASSERT(m_depth == 0);
	if (other.m_depth) {
		array_param_too_deep(2);
	}
	double me_num = get_numeric_double_value();
	double other_num = other.get_numeric_double_value();
	value.force_reset();
	PRINTDOUBLE(value, me_num / other_num);
	return true;
}

bool string_encoded_array::_multi_mod(const string_encoded_array& other) {
	PFC_ASSERT(m_depth == 0);
	if (other.m_depth) {
		array_param_too_deep(2);
	}
	double me_num = get_numeric_double_value();
	double other_num = other.get_numeric_double_value();
	value.force_reset();
	PRINTDOUBLE(value, fmod(me_num,other_num));
	return true;
}

bool string_encoded_array::_multi_round(const string_encoded_array& other) {
	PFC_ASSERT(m_depth == 0);
	if (other.m_depth) {
		array_param_too_deep(2);
	}
	double me_num = get_numeric_double_value();
	if (me_num == 0) {
		return false;
	}
	int sigdigs = other.get_numeric_value();
	unsigned long long int multiplier = pow(10, sigdigs);
	long double out_num = me_num * multiplier;
	if (me_num > 0) {
		out_num += 0.5;
	}
	else {
		out_num -= 0.5;
	}
	out_num = floor(out_num);
	out_num = out_num / multiplier;
	value.force_reset();
	PRINTDOUBLE(value, out_num);
	return true;
}

bool string_encoded_array::_multi_min(const string_encoded_array& other) {
	PFC_ASSERT(m_depth == 0);
	if (other.m_depth) {
		array_param_too_deep(2);
	}
	double me_num = get_numeric_double_value();
	double other_num = other.get_numeric_double_value();
	if (me_num <= other_num) {
		return false;
	}
	else {
		value.force_reset();
		PRINTDOUBLE(value, other_num);
		return true;
	}
}

bool string_encoded_array::_multi_max(const string_encoded_array& other) {
	PFC_ASSERT(m_depth == 0);
	if (other.m_depth) {
		array_param_too_deep(2);
	}
	double me_num = get_numeric_double_value();
	double other_num = other.get_numeric_double_value();
	if (me_num >= other_num) {
		return false;
	}
	else {
		value.force_reset();
		PRINTDOUBLE(value, other_num);
		return true;
	}
}

bool string_encoded_array::_sum() {
	PFC_ASSERT(m_depth == 1);
	double result = 0;
	const size_t count = get_width();
	for (size_t i = 0; i < count; i++) {
		result += sub_array[i].get_numeric_double_value();
	}
	sub_array.force_reset();
	value = "";
	PRINTDOUBLE(value, result);
	m_depth = 0;
	return true;
}

pfc::string8 string_encoded_array::print() const {
	if (has_array()) {
		string_encoded_array x(value);
		x.limit_depth(1);
		const pfc::string8 delim = "; ";
		x.join(delim);
		return x.get_pure_cvalue();
	}
	else {
		return get_pure_cvalue();
	}
}

pfc::string8 string_encoded_array::print_raw() const {
	pfc::string8 result;
	if (m_depth == 0) {
		for (size_t i = 0; i < value.get_length(); i++) {
			if (value[i] == LIST_SEP_PRETTY || value[i] == LIST_START_PRETTY || value[i] == LIST_END_PRETTY) {
				result.add_char('\\');
			}
			result.add_char(value[i]);
		}
	}
	else {
		result.add_char(LIST_START_PRETTY);
		const size_t count = get_width();
		for (size_t i = 0; i < count; i++) {
			auto & part = sub_array[i].print_raw();
			//if (part.get_length()) {
				result << part;
				if (i != count - 1) {
					result.add_char(LIST_SEP_PRETTY);
				}
			//}
		}
		result.add_char(LIST_END_PRETTY);
	}
	return result;
}
