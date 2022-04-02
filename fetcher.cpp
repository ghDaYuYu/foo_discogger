#include "stdafx.h"

#include "fetcher.h"
#include "exception.h"
#include "utils.h"

char USER_AGENT[] = "User-Agent: <<user agent>>";

const double RL_AVG_THRESHOLD = 0.5;
const size_t RL_REMAINING_THRESHOLD = 30;

#include <iomanip>
#include <optional>
#include <ostream>
#include <iostream>

//https://stackoverflow.com/questions/22590821/convert-stdduration-to-human-readable-time
std::ostream& operator<<(std::ostream& os, std::chrono::nanoseconds ns)
{
	using namespace std::chrono;
	using days = duration<int, std::ratio<86400>>;
	auto d = duration_cast<days>(ns);
	ns -= d;
	auto h = duration_cast<hours>(ns);
	ns -= h;
	auto m = duration_cast<minutes>(ns);
	ns -= m;
	auto s = duration_cast<seconds>(ns);
	ns -= s;

	//modded type int to u_uint32 & cast
	std::optional<t_uint32> fs_count;
	switch (os.precision()) {
	case 9: fs_count = (t_uint32)ns.count();
		break;
	case 6: fs_count = (t_uint32)duration_cast<microseconds>(ns).count();
		break;
	case 3: fs_count = (t_uint32)duration_cast<milliseconds>(ns).count();
		break;
	}

	char fill = os.fill('0');
	if (d.count())
		os << d.count() << "d ";
	if (d.count() || h.count())
		os << std::setw(2) << h.count() << ":";
	if (d.count() || h.count() || m.count())
		os << std::setw(d.count() || h.count() ? 2 : 1) << m.count() << ":";
	os << std::setw(d.count() || h.count() || m.count() ? 2 : 1) << s.count();
	if (fs_count.has_value())
		os << "." << std::setw(os.precision()) << fs_count.value();
	if (!d.count() && !h.count() && !m.count())
		os << "s";

	os.fill(fill);
	return os;
}

void Fetcher::fetch_html(const pfc::string8 &url, const pfc::string8 &params, pfc::string8 &html, abort_callback &p_abort, bool use_oauth) {
	pfc::array_t<t_uint8> buffer;
	fetch_url(url, params, buffer, p_abort, use_oauth, "application/xml");
	html = pfc::string8((char*)buffer.get_ptr(), buffer.get_size());
}

void Fetcher::fetch_url(const pfc::string8 &url, const pfc::string8 &params, pfc::array_t<t_uint8> & out, abort_callback &p_abort, bool use_oauth, const pfc::string8 &content_type) {

	pfc::string8 msg_average;
	pfc::string8 status;
	pfc::string8 request_url = "";
	pfc::string8 clean_url(url);

	bool isImageUrl = url.find_first("img.discogs.com") != ~0;
	isImageUrl |= url.find_first("i.discogs.com") != ~0;

	if (params.get_length()) clean_url << "?" << params;
	bool use_api = url.find_first("api") != ~0;
	use_oauth &= use_api;

	//log: url and oauth status
	log_msg(clean_url);
	log_msg(use_oauth ? "Url OAuth enabled" : "Url OAuth disabled");

	if (use_api /*&& m_throttling*/) {
		chrono::steady_clock::time_point time_now = chrono::steady_clock::now();
		chrono::duration<double> time_span = time_now - m_last_fetch;

		m_fetch_wait_rolling_avg = ROLLING_AVG_ALPHA * time_span.count() + (1.0 - ROLLING_AVG_ALPHA) * m_fetch_wait_rolling_avg;
		m_fetch_wait_rolling_avg = (std::min)(m_fetch_wait_rolling_avg, 1.0);

		//record time
		m_last_fetch = time_now;

		//log: average and last delta
		msg_average << "Avg >= " << m_fetch_wait_rolling_avg;
		if (time_span != chrono::steady_clock::duration::zero()) {
			msg_average << ", Lapse: ";
			//double microsecs = time_span.count();
			pfc::string8 msg_delta;
			if (m_throttle_delta) {
				std::stringstream human_read_time;
				human_read_time.precision(2);
				human_read_time << std::fixed << m_throttle_delta;
				msg_delta << " (+ " << human_read_time.str().c_str();
				msg_delta << ")";
			}

			std::stringstream human_read_time;
			human_read_time << std::setprecision(3) << time_span.count();
			msg_average << pfc::string8(human_read_time.str().c_str()) << msg_delta;
		}

		if (!isImageUrl /*&& m_throttling*/)
			log_msg(msg_average);

		//reset when rate remaining reaches max m_value again
		m_ratelimit_cruise &= (m_ratelimit_remaining < m_ratelimit_max); //60
		
		//apply delta/set cruise mode
		if (m_ratelimit_cruise ||
			(m_fetch_wait_rolling_avg < (60/m_ratelimit_max) /*RL_AVG_THRESHOLD*/ /*(0.5)*/ && m_ratelimit_remaining <= m_ratelimit_threshold /*(30)*/)) {

			m_ratelimit_cruise = true;

			chrono::duration<double, std::milli> time_span_milli = time_span;
			double sleep_base = m_ratelimit_remaining < 10 ? 5000 : m_ratelimit_remaining < 20 ? 2000 : 1000;	
			m_throttle_delta = (std::max)(sleep_base - time_span_milli.count(), 0.0);
			
			DWORD dw = static_cast<DWORD>((int)m_throttle_delta);
			Sleep(dw);
		}
		else {
			m_throttle_delta = 0;
		}
	}

	try {
		const http_request::ptr request = client->create_request("GET");

		//OAuth header
		if (use_oauth) {
			request->add_header(oauth_sign_url_header(url, params));			
		}
		 
		//Accept-Encoding header
		request->add_header(dllinflateInit2 != NULL ? "Accept-Encoding: gzip, deflate" : "Accept-Encoding: identity");

		//Accept user User-Agent and header
		request->add_header(USER_AGENT);
		pfc::string8 accept_header;
		accept_header << "Accept: " << content_type;
		request->add_header(accept_header);
	
		int tries = 1; //3x on error loading resource
		

		while (1) {

			try {
				file::ptr f;
				f = request->run_ex(clean_url.get_ptr(), p_abort);

				http_reply::ptr r;
				f->service_query_t(r);
				pfc::string8 status;
				r->get_status(status);

				pfc::string8 rate_header_buffer;
				pfc::string8 msg_ratelimits;

				if (r->get_http_header("X-Discogs-Ratelimit", rate_header_buffer)) {
					m_ratelimit_max = atoi(rate_header_buffer); //currently 60, discoggs may update these rate limits at any time
					m_ratelimit_threshold = m_ratelimit_max - RL_REMAINING_THRESHOLD; //max - 30
					msg_ratelimits << "RateLimit: " << rate_header_buffer;
				}
				if (r->get_http_header("X-Discogs-Ratelimit-Used", rate_header_buffer)) {
					msg_ratelimits << " - Used: " << rate_header_buffer;
				}
				if (r->get_http_header("X-Discogs-Ratelimit-Remaining", rate_header_buffer)) {
					m_ratelimit_remaining = atoi(rate_header_buffer);
					msg_ratelimits << " - Remaining: " << rate_header_buffer;
				}

				//log: throttle and rate limits
				//if (m_throttling) {
					if (m_throttle_delta) {
						std::stringstream human_read_time;
						human_read_time << std::setprecision(3) << m_throttle_delta;
						msg_ratelimits << " - Throttle engaged (" << pfc::string8(human_read_time.str().c_str()) << u8" \u03BCs)";
					}
					else {
						msg_ratelimits << (isImageUrl ? "Not throttling images (img.discogs.com)" : " (Throttle diseng.)");
					}
				//}

				if (msg_ratelimits.length())
					log_msg(msg_ratelimits);
				else {
					m_ratelimit_remaining = m_ratelimit_threshold; //if rate info is empty reset to threshold
				}

				// check status
				if (pfc::string8(status).find_first("200") == ~0) {
					pfc::string8 msg_status;
					msg_status << "HTTP error status: " << status;
					log_msg(msg_status);

					int error_code = std::stoi(substr(status, 9, 3).get_ptr());
					
					switch (error_code) {
					case 404:
						throw http_404_exception();
					case 401: //unauthorized				
						throw http_401_exception();				
					case 429: //too many requests (need to wait for remaining 60 or slow to 1 req/sec)						
						throw http_429_exception();
					default:
						throw http_exception(error_code);
					}
				}

				//Sleep(1000);

				pfc::string8 reply_content_type;
				f->get_content_type(reply_content_type);
				if (reply_content_type.is_empty()) {
					out.set_size(0);
					throw network_exception("Error reading network response.");
				}

				// read stream...
				out.set_size(1024);
				size_t bufferUsed = 0;
				for (;;) {
					p_abort.check();
					size_t delta = out.get_size() - bufferUsed;
					size_t done = f->read(out.get_ptr() + bufferUsed, delta, p_abort);
					bufferUsed += done;
					if (done != delta) {
						break;
					}
					out.set_size(out.get_size() << 1);
				}
				out.set_size(bufferUsed);

				// see if we must unzip buffer
				if (out.get_size() >= 6 && out[0] == 0x1f && out[1] == 0x8b) {
					pfc::array_t<t_uint8> unzipped;
					unzipped.set_size(pfc::decode_little_endian<t_uint32>(out.get_ptr() + out.get_size() - 4));
					uLongf destLen = unzipped.get_size();
					int state = myUncompress(unzipped.get_ptr(), &destLen, out.get_ptr(), out.get_size());
					if (state != Z_OK) {
						throw network_exception("Error unzipping network response.");
					}
					PFC_ASSERT(destLen == unzipped.get_size());
					out = unzipped;
				}
				break;
			}
			catch (http_429_exception) {

				if (tries > 20) throw;

				pfc::string8 error_msg;
				error_msg << "Rate-limited. Retrying: " << tries;
				log_msg(error_msg);
				Sleep(2000 * tries);
			}
			catch (foobar2000_io::exception_io &e) {
				
				if (tries > 5) throw;

				pfc::string8 error_msg;
				error_msg << "Network error";
				if (!error_msg.has_prefix(pfc::string8(e.what())))
					error_msg << ": " << e.what();
				error_msg << ". Retrying: " << tries;
				log_msg(error_msg);
				Sleep(2000 * (tries > 1 ? 2 : 1));
			}
			tries++;
		}
	}
	catch (exception_aborted) {
		throw;
	}
	catch (foo_discogs_exception &e) {
		e << "(url: " << clean_url << ")";
		pfc::string8 error_msg;
		error_msg << "Exception handling: " << clean_url;
		log_msg(error_msg);
		throw;
	}
	catch (const foobar2000_io::exception_io &e) {
		//retries > max retries
		network_exception ex("Network exception: ");
		ex << e.what() << " (url: " << clean_url << ")";
		pfc::string8 error_msg;
		error_msg << "Network exception fetching url: " << clean_url;
		log_msg(error_msg);
		throw ex;
	}
	catch (const std::exception& e) {
		log_msg(e.what());
	}
	catch (...) {
		out.set_size(0);
		network_exception ex("Unknown network exception: ");
		ex << "(url: " << clean_url << ")";
		pfc::string8 error_msg;
		error_msg << "Unknown network exception handling: " << clean_url;
		log_msg(error_msg);
		throw ex;
	}
}

void Fetcher::fetch_html_simple_log(const pfc::string8& url, const pfc::string8& params, pfc::string8& html, abort_callback& p_abort, bool throw_abort, const pfc::string8& content_type) {

	pfc::array_t<t_uint8> buffer;

	pfc::string8 status;
	pfc::string8 request_url = "";
	pfc::string8 clean_url(url);

	if (params.get_length()) clean_url << "?" << params;

	log_msg(clean_url);

	try {
		const http_request::ptr request = client->create_request("GET");

		//HEADERS
		//Accept-Encoding header
		request->add_header(dllinflateInit2 != NULL ? "Accept-Encoding: gzip, deflate" : "Accept-Encoding: identity");

		//Accept user User-Agent and header
		request->add_header(USER_AGENT);
		pfc::string8 accept_header;
		accept_header << "Accept: " << content_type;
		request->add_header(accept_header);

		//END HEADERS

		int tries = 1; //3x on error loading resource
		file::ptr f;

		while (1) {

			try {

				f = request->run_ex(clean_url.get_ptr(), p_abort);

				http_reply::ptr r;
				f->service_query_t(r);
				pfc::string8 status;
				r->get_status(status);

				pfc::string8 debug_keep_alive;
				r->get_http_header("Connection", debug_keep_alive);

				// check status (applies only to request->run_ex)

				if (pfc::string8(status).find_first("200") == ~0) {

					pfc::string8 msg_status;
					msg_status << "HTTP error status: " << status;
					log_msg(msg_status);
				}

				//Sleep(1000);

				pfc::string8 reply_content_type;
				f->get_content_type(reply_content_type);
				if (reply_content_type.is_empty()) {
					buffer.set_size(0);
					log_msg("Error reading network response.");
					break;
				}

				// read stream...
				buffer.set_size(1024);
				size_t bufferUsed = 0;
				for (;;) {
					p_abort.check();
					size_t delta = buffer.get_size() - bufferUsed;
					size_t done = f->read(buffer.get_ptr() + bufferUsed, delta, p_abort);
					bufferUsed += done;
					if (done != delta) {
						break;
					}
					buffer.set_size(buffer.get_size() << 1);
				}
				buffer.set_size(bufferUsed);

				// see if we must unzip buffer
				if (buffer.get_size() >= 6 && buffer[0] == 0x1f && buffer[1] == 0x8b) {
					pfc::array_t<t_uint8> unzipped;
					unzipped.set_size(pfc::decode_little_endian<t_uint32>(buffer.get_ptr() + buffer.get_size() - 4));
					uLongf destLen = unzipped.get_size();
					int state = myUncompress(unzipped.get_ptr(), &destLen, buffer.get_ptr(), buffer.get_size());
					if (state != Z_OK) {
						log_msg("Error unzipping network response.");
						break;
					}
					PFC_ASSERT(destLen == unzipped.get_size());
					buffer = unzipped;
				}

				break;
			}
			catch (foobar2000_io::exception_io& e) {

				if (tries > 5) throw;

				pfc::string8 error_msg;
				error_msg << "Network error";
				if (!error_msg.has_prefix(pfc::string8(e.what())))
					error_msg << ": " << e.what();
				error_msg << ". Retrying: " << tries;
				log_msg(error_msg);
				Sleep(2000 * (tries > 1 ? 2 : 1));
			}
			tries++;
		}
	}
	catch (exception_aborted) {
		if (throw_abort)
			throw;
	}
	catch (foo_discogs_exception& e) {
		e << "(url: " << clean_url << ")";
		pfc::string8 error_msg;
		error_msg << "Exception handling: " << clean_url;
		log_msg(error_msg);
	}
	catch (const foobar2000_io::exception_io& e) {
		//retries > max retries
		network_exception ex("Network exception: ");
		ex << e.what() << " (url: " << clean_url << ")";
		pfc::string8 error_msg = PFC_string_formatter() << "Network exception (simlog fetching url): " << clean_url;
		log_msg(error_msg);
		throw ex;
	}
	catch (const std::exception& e) {
		log_msg(e.what());
	}
	catch (...) {

		// fallback for request->run errors

		buffer.set_size(0);
		network_exception ex("Unknown network exception: ");
		ex << "(url: " << clean_url << ")";
		pfc::string8 error_msg;
		error_msg << "Unknown network exception handling: " << clean_url;
		log_msg(error_msg);
	}

	html = pfc::string8((char*)buffer.get_ptr(), buffer.get_size());
}

void Fetcher::set_oauth(const pfc::string8 &token, const pfc::string8 &token_secret) {
	consumer = new OAuth::Consumer(CONSUMER_KEY, CONSUMER_SECRET);
	oauth_token = token;
	oauth_token_secret = token_secret;
	init_oauth();
}

void Fetcher::init_oauth() {
	if (!oauth_token.get_length() && !oauth_token_secret.get_length()) {
		if (oauth != nullptr) {
			delete oauth;
			oauth = nullptr;
		}
		if (access_token != nullptr) {
			delete access_token;
			access_token = nullptr;
		}
		oauth = new OAuth::Client(consumer);
	}
	else {
		if (access_token != nullptr) {
			delete access_token;
			access_token = nullptr;
		}
		if (oauth != nullptr) {
			delete oauth;
			oauth = nullptr;
		}
		access_token = new OAuth::Token(oauth_token.get_ptr(), oauth_token_secret.get_ptr());
		oauth = new OAuth::Client(consumer, access_token);
	}
}

void Fetcher::update_oauth(const pfc::string8 &token, const pfc::string8 &token_secret) {
	if (token == oauth_token && token_secret == oauth_token_secret) {
		return;
	}
	oauth_token = token;
	oauth_token_secret = token_secret;
	init_oauth();
}

void Fetcher::test_oauth(const pfc::string8 &token, const pfc::string8 &token_secret, abort_callback &p_abort) {
	OAuth::Consumer c = OAuth::Consumer(CONSUMER_KEY, CONSUMER_SECRET);
	OAuth::Token at = OAuth::Token(token.get_ptr(), token_secret.get_ptr());
	OAuth::Client oa = OAuth::Client(&c, &at);

	const char *identity = "https://api.discogs.com/oauth/identity"; 
	pfc::string8 url = "";
	url << identity << "?";
	url << oa.getURLQueryString(OAuth::Http::Get, identity, "").c_str();
	
	pfc::string8 html;
	// throws network_exception on error
	fetch_html(url, "", html, p_abort, false);
}

pfc::string8 Fetcher::oauth_sign_url(const pfc::string8 &url, const pfc::string8 &params) {
	pfc::string8 oauth_params;
	if (params.get_length()) {
		pfc::string8 tmp = url; 
		tmp << "?" << params;
		oauth_params = oauth->getURLQueryString(OAuth::Http::Get, tmp.get_ptr()).c_str();
	}
	else {
		oauth_params = oauth->getURLQueryString(OAuth::Http::Get, url.get_ptr()).c_str();
	}
	pfc::string8 ret(url);
	ret << "?" << oauth_params;
	return ret;
}

pfc::string8 Fetcher::oauth_sign_url_header(const pfc::string8& url, const pfc::string8& params) {
	pfc::string8 oauth_header;
	if (params.get_length()) {
		pfc::string8 tmp = url;
		tmp << "?" << params;	
		oauth_header = oauth->getFormattedHttpHeader(OAuth::Http::Get, tmp.get_ptr()).c_str();
	}
	else {
		oauth_header = oauth->getFormattedHttpHeader(OAuth::Http::Get, url.get_ptr()).c_str();
	}

	return oauth_header;
}

pfc::string8 Fetcher::get_oauth_authorize_url(abort_callback &p_abort) {
	OAuth::Client oa = OAuth::Client(consumer);

	pfc::string8 oauth_query_str = oa.getURLQueryString(OAuth::Http::Get, REQUEST_TOKEN_URL).c_str();

	pfc::string8 html;
	fetch_html(REQUEST_TOKEN_URL, oauth_query_str, html, p_abort, false);

	pfc::string8 msg("OAuth request response: ");
	msg << html;
	log_msg(msg);

	if (request_token != nullptr) {
		delete request_token;
		request_token = nullptr;
	}
	OAuth::Token t = OAuth::Token::extract(html.get_ptr());
	request_token = new OAuth::Token(t.key(), t.secret());

	pfc::string8 auth_url;
	auth_url << AUTHORIZE_URL << "?oauth_token=" << request_token->key().c_str();
	return auth_url;
}

OAuth::Token * Fetcher::generate_oauth(pfc::string8 &code, abort_callback &p_abort) {
	if (request_token == nullptr) {
		return nullptr;
	}
	request_token->setPin(code.get_ptr());

	OAuth::Client oa = OAuth::Client(consumer, request_token);

	pfc::string8 oauth_query_str= oa.getURLQueryString(OAuth::Http::Get, ACCESS_TOKEN_URL, "", true).c_str();

	pfc::string8 html;
	fetch_html(ACCESS_TOKEN_URL, oauth_query_str, html, p_abort, false);

	pfc::string8 msg("OAuth generated response: ");
	msg << html;
	log_msg(msg);

	OAuth::KeyValuePairs access_token_resp_data = OAuth::ParseKeyValuePairs(html.get_ptr());
	OAuth::Token t = OAuth::Token::extract(access_token_resp_data);

	OAuth::Token *generated_token = new OAuth::Token(t.key(), t.secret());
	return generated_token;
}
