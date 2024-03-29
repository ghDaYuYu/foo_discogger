#pragma once

#include "exception.h"
#include "liboauthcpp\liboauthcpp.h"
#include <chrono>

#define CONSUMER_KEY "<<key>>"
#define CONSUMER_SECRET "<<consumer_secret>>"
#define REQUEST_TOKEN_URL "https://api.discogs.com/oauth/request_token"
#define AUTHORIZE_URL "https://www.discogs.com/oauth/authorize"
#define ACCESS_TOKEN_URL "https://api.discogs.com/oauth/access_token"

#define ROLLING_AVG_ALPHA (0.1/5)

namespace chrono = std::chrono;

class network_exception : public foo_discogs_exception
{
public:
	network_exception() : foo_discogs_exception("Network Exception") {}
	network_exception(const char* msg) : foo_discogs_exception(msg) {}
};


class http_exception : public network_exception
{
protected:
	const int status;
public:
	http_exception() : status(0) {}
	http_exception(int s) : network_exception("HTTP Error"), status(s) {
		(*this) << " (" << s << ")";
	}
	http_exception(int s, const char* msg) : network_exception(msg), status(s) {}
	int get_status() const {
		return status;
	}
};


class http_404_exception : public http_exception
{
public:
	http_404_exception() : http_exception(404, "Page Deleted or Missing (404)") {}
};


class http_401_exception : public http_exception
{
public:
	http_401_exception() : http_exception(401, "Authorization Failed (401) [Is OAuth working?]") {}
};


class http_429_exception : public http_exception
{
public:
	http_429_exception() : http_exception(429, "Too Many Requests (429) [Discogs API rate-limit reached.]") {}
};
class Fetcher
{
private:
	// OAuth stuff
	OAuth::Consumer *consumer;
	OAuth::Client *oauth;
	OAuth::Token *access_token;
	pfc::string8 oauth_token;
	pfc::string8 oauth_token_secret;
	OAuth::Token *request_token;

	chrono::steady_clock::time_point m_last_fetch;
	double m_fetch_wait_rolling_avg;
	size_t m_ratelimit_max;
	size_t m_ratelimit_remaining;
	size_t m_ratelimit_threshold;
	bool m_ratelimit_cruise;
	double m_throttle_delta;
	size_t m_current_minute;

	http_client::ptr client;

public:
	Fetcher() {
		consumer = nullptr;
		oauth = nullptr;
		access_token = nullptr;
		request_token = nullptr;

		m_last_fetch = chrono::steady_clock::now();
		m_fetch_wait_rolling_avg = 1;
		m_ratelimit_max = 60;
		m_ratelimit_threshold = 30;
		m_ratelimit_remaining = 60;
		m_ratelimit_cruise = false;
		m_throttle_delta = 0.0;

		time_t currentTime;
		struct tm* localTime;
		time(&currentTime);
		localTime = localtime(&currentTime);
		
		m_current_minute = localTime->tm_min;
		
		int n = 0;
		while (!service_enum_t<http_client>().first(client)) {
			log_msg("Waiting a little for http_client...");
			Sleep(200);
			if (++n == 19) {
				foo_discogs_exception ex("Timed out waiting for http_client.");
				throw ex;
			}
		}
		log_msg("http_client service ready");
	}
	
	~Fetcher() {
		delete consumer;
		delete oauth;
		delete access_token;
		delete request_token;
	}

	void fetch_html(const pfc::string8 &url,
				const pfc::string8 &params,
				pfc::string8 &html,
				abort_callback &p_abort,
				bool use_oauth = true);
	
	void fetch_url(const pfc::string8 &url,
				const pfc::string8 &params,
				pfc::array_t<t_uint8> & out,
				abort_callback &p_abort,
				bool use_oauth = true,
				const pfc::string8 &content_type = "application/xml,text/html,image/jpeg");

	void fetch_html_simple_log(const pfc::string8& url,
		const pfc::string8& params,
		pfc::string8& out,
		abort_callback& p_abort,
		bool throw_abort = true,
		const pfc::string8& content_type = "application/xml,text/html" /*, image / jpeg"*/);

	// OAuth stuff
	void set_oauth(const pfc::string8 &token, const pfc::string8 &token_secret);
	void init_oauth();
	void update_oauth(const pfc::string8 &token, const pfc::string8 &token_secret);
	void test_oauth(const pfc::string8 &token, const pfc::string8 &token_secret, abort_callback &p_abort);
	pfc::string8 oauth_sign_url(const pfc::string8 &url, const pfc::string8 &params);
	pfc::string8 oauth_sign_url_header(const pfc::string8& url, const pfc::string8& params);
	pfc::string8 get_oauth_authorize_url(abort_callback &p_abort);
	OAuth::Token * generate_oauth(pfc::string8 &code, abort_callback &p_abort);
};
