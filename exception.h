#pragma once

#include <exception>
#include <sstream>
#include <string>


class foo_discogs_exception : public std::exception
{
protected:
	mutable std::string _mwhat;
	mutable std::ostringstream *_mstream;

public:
	foo_discogs_exception() : _mstream(nullptr) {}

	foo_discogs_exception(const char* msg) : _mwhat(msg), _mstream(nullptr) {}

	foo_discogs_exception(const foo_discogs_exception &that) {
		if (that._mstream != nullptr) {
			_mwhat = that._mstream->str();
		}
		else {
			_mwhat = that._mwhat;
		}
		_mstream = nullptr;
	}

	~foo_discogs_exception() {
		if (_mstream != nullptr) {
			delete _mstream;
		}
	}

	virtual const char *what() const override {
		if (_mstream != nullptr && _mstream->str().size()) {
			_mwhat = _mstream->str();
			_mstream->str("");
		}
		return _mwhat.c_str();
	}

	template<typename T>
	foo_discogs_exception& operator<<(const T& t) {
		if (_mstream == nullptr) {
			_mstream = new std::ostringstream();
			(*_mstream) << _mwhat;
		}
		(*_mstream) << t;
		return (*this);
	}
};
