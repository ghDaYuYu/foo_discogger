#include "stdafx.h"

#include "json_helpers.h"
#include "entities.h"


void assert_is_object(json_t *element) {
	if (!json_is_object(element)) {
		parser_exception ex;
		ex << "Object assertion failed.";
		throw ex;
	}
}

void assert_is_array(json_t *element) {
	if (!json_is_array(element)) {
		parser_exception ex;
		ex << "Array assertion failed.";
		throw ex;
	}
}

pfc::string8 JSONString(json_t *element) {
	pfc::string8 s = "";
	if (json_is_string(element)) {
		s = decode_html_entities_utf8(json_string_value(element));
	}
	return s;
}

pfc::string8 JSONAttributeString(json_t *element, const char* attribute) {
	pfc::string8 s = "";
	if (json_is_object(element)) {
		json_t *attr = json_object_get(element, attribute);
		if (json_is_string(attr)) {
			s = JSONString(attr);
		}
		// this allows us to read "id" attributes into strings
		else if (json_is_integer(attr)) {
			s << json_integer_value(attr);
		}
		// and ratings
		else if (json_is_number(attr)) {
			s << json_number_value(attr);
			if (s.find_first('.') != pfc::infinite_size) {
				s = rtrim(s, "0");
			}
			if (s[s.length() - 1] == '.') {
				s += "0";
			}
		}
	}
	return s;
}

/*
pfc::string8_ex JSONAttributeStringEx(json_t *element, const char* attribute) {
	pfc::string8_ex s;

	if (json_is_object(element)) {

		json_t *attr = json_object_get(element, attribute);

		if (json_is_string(attr)) {
			const char *tmp = JSONString(attr);
			if (strlen(tmp)) {
				s = JSONString(attr);
			}
		}
		// this allows us to read "id" attributes into strings
		else if (json_is_integer(attr)) {
			s << json_integer_value(attr);
		}
		// and ratings
		else if (json_is_number(attr)) {
			s << json_number_value(attr);
			if (s.find_first('.') != pfc::infinite_size) {
				s = rtrim(s, "0");
			}
			if (s[s.length() - 1] == '.') {
				s += "0";
			}
		}
	}
	return s;
}
*/

pfc::array_t<pfc::string8> JSONAttributeStringArray(json_t *root, const char* attribute) {
	pfc::array_t<pfc::string8> v;
	json_t *array = json_object_get(root, attribute);
	if (json_is_array(array)) {
		for (unsigned int i = 0; i < json_array_size(array); i++) {
			json_t *s = json_array_get(array, i);
			pfc::string8 ss = JSONString(s);
			if (ss.get_length() != 0) {
				v.append_single(ss);
			}
		}
	}
	return v;
}

pfc::string8 JSONAttributeObjectAttributeString(json_t *root, const char *object, const char *key) {
	json_t *obj = json_object_get(root, object);
	if (json_is_object(obj)) {
		json_t *s = json_object_get(obj, key);
		return JSONString(s);
	}
	return "";
}

unsigned int JSONAttributeObjectAttributeInt(json_t *root, const char *object, const char *key) {
	json_t *obj = json_object_get(root, object);
	if (json_is_object(obj)) {
		json_t *s = json_object_get(obj, key);
		if (json_is_integer(s)) {
			return json_integer_value(s);
		}
	}
	return pfc::infinite_size;
}

pfc::string8 JSONAttributeObjectAttributeObjectAttributeString(json_t *root, const char *object, const char *object2, const char *attribute) {
	json_t *obj = json_object_get(root, object);
	if (json_is_object(obj)) {
		json_t *obj2 = json_object_get(obj, object2);
		if (json_is_object(obj2)) {
			json_t *s = json_object_get(obj2, attribute);
			return JSONString(s);
		}
	}
	return "";
}

pfc::array_t<pfc::string8> JSONAttributeObjectArrayAttributeString(json_t *root, const char* object, const char* attribute) {
	pfc::array_t<pfc::string8> v;
	json_t *array = json_object_get(root, object);
	if (json_is_array(array)) {
		for (unsigned int i = 0; i < json_array_size(array); i++) {
			json_t *obj = json_array_get(array, i);
			if (json_is_object(obj)) {
				json_t *s = json_object_get(obj, attribute);
				pfc::string8 ss = JSONString(s);
				if (ss.get_length() != 0) {
					v.append_single(ss);
				}
			}
		}
	}
	return v;
}

pfc::string8 JSONAttributeObjectArrayAttributeStringWhere(json_t *root, const char* object, const char* attribute, const char* where_attr, const char* where_value) {
	json_t *array = json_object_get(root, object);
	if (json_is_array(array)) {
		for (unsigned int i = 0; i < json_array_size(array); i++) {
			json_t *obj = json_array_get(array, i);
			if (json_is_object(obj)) {
				json_t *s = json_object_get(obj, where_attr);
				if (STR_EQUAL(JSONString(s), where_value)) {
					return JSONString(json_object_get(obj, attribute));
				}
			}
		}
	}
	return "";
}
