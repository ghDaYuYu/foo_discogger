#pragma once

#include "jansson.h"
#include "exception.h"


extern void assert_is_object(json_t *element);
extern void assert_is_array(json_t *element);

extern pfc::string8 JSONString(json_t *element);
extern pfc::string8 JSONAttributeString(json_t *element, const char* attribute);
extern pfc::array_t<pfc::string8> JSONAttributeStringArray(json_t *root, const char *attribute);
extern pfc::string8 JSONAttributeObjectAttributeObjectAttributeString(json_t *root, const char *object, const char *object2, const char *attribute);
extern pfc::string8 JSONAttributeObjectAttributeString(json_t *root, const char *object, const char *key);
extern unsigned int JSONAttributeObjectAttributeInt(json_t *root, const char *object, const char *key);
extern pfc::array_t<pfc::string8> JSONAttributeObjectArrayAttributeString(json_t *root, const char *object, const char *attribute);
extern pfc::string8 JSONAttributeObjectArrayAttributeStringWhere(json_t *root, const char *object, const char *attribute, const char *where_attr, const char *where_value);


class parser_exception : public foo_discogs_exception
{
public:
	parser_exception() : foo_discogs_exception("JSON Parser Exception") {}
};


class JSONParser;
typedef pfc::rcptr_t<JSONParser> JSONParser_ptr;


class JSONParser {
public:
	json_t *root = nullptr;
	json_error_t error;

	JSONParser(json_t *object) {
		root = object;
		json_incref(root);
	}

	JSONParser(const pfc::string8 &json) {
		root = json_loads(json.get_ptr(), 0, &error);
		if (!root) {
			parser_exception ex;
			ex << error.text;
			throw ex;
		}
	}

	~JSONParser() {
		if (root) {
			json_decref(root);
		}
	}

	inline void assert_is_object(const char *key) {
		::assert_is_object(json_object_get(root, key));
	}

	inline void assert_is_array(const char *key) {
		::assert_is_object(json_object_get(root, key));
	}

	JSONParser_ptr get(const char *key) {
		return pfc::rcnew_t<JSONParser>(json_object_get(root, key));
	}

	inline pfc::string8 get_string(const char *key) {
		return JSONAttributeString(root, key);
	}

	inline pfc::array_t<pfc::string8> get_string_array(const char *key) {
		return JSONAttributeStringArray(root, key);
	}

	inline pfc::array_t<pfc::string8> get_object_array_string(const char *object, const char *key) {
		return JSONAttributeObjectArrayAttributeString(root, object, key);
	}

	inline unsigned int get_object_int(const char *object, const char *key) {
		return JSONAttributeObjectAttributeInt(root, object, key);
	}

	inline pfc::string8 get_object_string(const char *object, const char *key) {
		return JSONAttributeObjectAttributeString(root, object, key);
	}

	inline pfc::string8 get_object_object_string(const char *object, const char *object2, const char *key) {
		return JSONAttributeObjectAttributeObjectAttributeString(root, object, object2, key);
	}

	inline pfc::string8 get_object_array_string_where(const char *object, const char *key, const char *where_attr, const char *where_value) {
		return JSONAttributeObjectArrayAttributeStringWhere(root, object, key, where_attr, where_value);
	}
};
