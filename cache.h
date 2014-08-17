#pragma once


template<typename key_t, typename value_t>

class lru_cache
{
public:
	typedef typename std::pair<key_t, value_t> key_value_pair_t;
	typedef typename std::list<key_value_pair_t>::iterator list_iterator_t;

	lru_cache(size_t max_size) :
		_max_size(max_size) {
	}

	void put(const key_t& key, const value_t& value) {
		auto it = _cache_items_map.find(key);
		if (it != _cache_items_map.end()) {
			_cache_items_list.erase(it->second);
			_cache_items_map.erase(it);
		}

		_cache_items_list.push_front(key_value_pair_t(key, value));
		_cache_items_map[key] = _cache_items_list.begin();

		if (_cache_items_map.size() > _max_size) {
			auto last = _cache_items_list.end();
			last--;
			_cache_items_map.erase(last->first);
			_cache_items_list.pop_back();
		}
	}

	const value_t& get(const key_t& key) {
		auto it = _cache_items_map.find(key);
		if (it == _cache_items_map.end()) {
			throw ("cache error");
		}
		else {
			_cache_items_list.splice(_cache_items_list.begin(), _cache_items_list, it->second);
			return it->second->second;
		}
	}

	bool exists(const key_t& key) const {
		return _cache_items_map.find(key) != _cache_items_map.end();
	}

	size_t size() const {
		return _cache_items_map.size();
	}

	void clear() {
		_cache_items_list.clear();
		_cache_items_map.clear();
	}

	void set_max_size(size_t max_size) {
		_max_size = max_size;
		while (_cache_items_map.size() > _max_size) {
			auto last = _cache_items_list.end();
			last--;
			_cache_items_map.erase(last->first);
			_cache_items_list.pop_back();
		}
	}

private:
	std::list<key_value_pair_t> _cache_items_list;
	std::unordered_map<key_t, list_iterator_t> _cache_items_map;
	size_t _max_size;
};


namespace std
{
	template <>
	struct hash<pfc::string8> : public unary_function<pfc::string8, size_t>
	{
		size_t operator()(const pfc::string8& value) const {
			const char *str = value.get_ptr();
			unsigned long hash = 5381;
			int c;
			while (c = *str++) {
				hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
			}
			return hash;
		}
	};

	template <>
	struct equal_to<pfc::string8> : public unary_function<pfc::string8, bool>
	{
		bool operator()(const pfc::string8& x, const pfc::string8& y) const {
			return STR_EQUAL(x, y);
		}
	};
}