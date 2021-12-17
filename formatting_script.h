#pragma once


class formatting_script
{
private:
	pfc::string8 string;
	mutable service_ptr_t<titleformat_object> script;
	mutable bool compiled = false;

public:
	formatting_script() {}

	formatting_script(const char * s) {
		string = s;
	}

	formatting_script & operator=(const char * s) {
		string = s;
		compiled = false;
		return *this;
	}
	formatting_script & operator=(pfc::string8 s) {
		string = s;
		compiled = false;
		return *this;
	}

	inline operator const pfc::string8 & () const {
		return string;
	}

	inline operator const char * () const {
		return string.get_ptr();
	}

	inline const char * get_ptr() {
		return string.get_ptr();
	}

	const service_ptr_t<titleformat_object> & operator->() const {
		if (script == NULL || !compiled) {		
			static_api_ptr_t<titleformat_compiler>()->compile_safe_ex(script, string);
			compiled = true;
		}
		return script;
	}

	const service_ptr_t<titleformat_object> & get_script() const {
		if (script == NULL || !compiled) {	
			static_api_ptr_t<titleformat_compiler>()->compile_safe_ex(script, string);
			compiled = true;
		}
		return script;
	}

	formatting_script(const formatting_script &other) {
		string = other.string;
		compiled = other.compiled;
	}
};
