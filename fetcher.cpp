#include "stdafx.h"

#include "fetcher.h"
#include "exception.h"
#include "utils.h"


char USER_AGENT[] = "User-Agent: foo_discogs/" FOO_DISCOGS_VERSION;


void Fetcher::fetch_html(const pfc::string8 &url, const pfc::string8 &params, pfc::string8 &html, abort_callback &p_abort, bool use_oauth) {
	pfc::array_t<t_uint8> buffer;
	fetch_url(url, params, buffer, p_abort, use_oauth, "application/xml");
	html = pfc::string8((char *)buffer.get_ptr(), buffer.get_size());
}

void Fetcher::fetch_url(const pfc::string8 &url, const pfc::string8 &params, pfc::array_t<t_uint8> & out, abort_callback &p_abort, bool use_oauth, const pfc::string8 &content_type) {
	pfc::string8 req_url = "";

	bool use_api = true;
	if (use_oauth && url.find_first("api") == pfc::infinite_size) {
		log_msg("Disabling OAuth for URL");
		use_oauth = false;
		use_api = false;
	}

	if (use_oauth) {
		req_url << oauth_sign_url(url, params);
	}
	else if (params.get_length()) {
		req_url << url << "?" << params;
	}
	else {
		req_url << url;
	}

	log_msg(req_url);

	if (use_api) {
		//std::clock_t fetch_wait = (std::clock_t)((double)(clock() - last_fetch + 1))/CLOCKS_PER_SEC;
		//fetch_wait_rolling_avg = (ROLLING_AVG_ALPHA * fetch_wait) + (1.0 - ROLLING_AVG_ALPHA) * fetch_wait_rolling_avg;
		/*//pfc::string8 msg = "AVG: " + to_pfc::string8(fetch_wait_rolling_avg) + " WAIT: " + to_pfc::string8(fetch_wait);
		//log_msg(msg.c_str());
		//if (fetch_wait_rolling_avg < 1) {
		//	log_msg("Network throttling...");
		//	Sleep(1000 - (fetch_wait_rolling_avg * 1000));
		//}*/
	}

	try {
		const http_request::ptr request = client->create_request("GET");
		//request->add_header("Accept-Encoding: gzip");
		if (dllinflateInit2 != NULL) {
			request->add_header("Accept-Encoding: gzip, deflate");
		}
		else {
			request->add_header("Accept-Encoding: identity");
		}
		request->add_header(USER_AGENT);
		pfc::string8 accept_header("Accept: ");
		accept_header << (content_type);
		request->add_header(accept_header);

		// try to load resource 3x on error
		int tries = 1;
		file::ptr f;
		while (1) {
			try {
				f = request->run_ex(req_url.get_ptr(), p_abort);
				
				http_reply::ptr r;
				f->service_query_t(r);

				pfc::string8 status;
				r->get_status(status);

				pfc::string8 header;
				pfc::string8 xratelimits(" RateLimit: ");
				if (r->get_http_header("X-Discogs-Ratelimit", header)) {
					xratelimits << header;
				}
				if (r->get_http_header("X-Discogs-Ratelimit-Used", header)) {
					xratelimits << " - Used: " << header;
				}
				if (r->get_http_header("X-Discogs-Ratelimit-Remaining", header)) {
					xratelimits << " - Remaining: " << header;
				}
				if (xratelimits.length())
					log_msg(xratelimits);

				if (pfc::string8(status).find_first("200") == pfc::infinite_size) {
					pfc::string8 msg("HTTP error status: ");
					msg << status;
					log_msg(msg);

					// TODO: handle rate limiting
					pfc::string8 header;
					/*if (r->get_http_header("X-RateLimit-Type", header)) {
					log_msg(pfc::string8("  X-RateLimit-Type: ") << header);
					}
					if (r->get_http_header("X-RateLimit-Limit", header)) {
					log_msg(pfc::string8("  X-RateLimit-Limit: ") << header);
					}
					if (r->get_http_header("X-RateLimit-Remaining", header)) {
					log_msg(pfc::string8("  X-RateLimit-Remaining: ") << header);
					}
					if (r->get_http_header("X-RateLimit-Reset", header)) {
					log_msg(pfc::string8("  X-RateLimit-Reset: ") << header);
					}*/

					int code = std::stoi(substr(status, 9, 3).get_ptr());
					switch (code) {
					case 404:
						throw http_404_exception();

					case 401:
						throw http_401_exception();

					case 429:
						throw http_429_exception();

					default:
						throw http_exception(code);
					}
				}

				pfc::string8 reply_content_type;
				f->get_content_type(reply_content_type);
				if (reply_content_type.is_empty()) {
					out.set_size(0);
					throw network_exception("Error reading network response.");
				}

				/* Weird stuff here.... Read 1024 bytes at a time.... */
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

				if (use_api) {
					//last_fetch = std::clock();
				}

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
				if (tries > 20) {
					throw;
				}
				pfc::string8 msg;
				msg << "Rate-limited. Retrying: " << tries;
				log_msg(msg);
				Sleep(2000 * tries);
			}
			catch (foobar2000_io::exception_io &e) {
				if (tries > 5) {
					throw;
				}
				pfc::string8 error;
				error << "Networking Error: " << pfc::string8(e.what()) << " - Retrying: " << tries;
				log_msg(error);
				Sleep(2000);
			}
			tries++;
		}
	}
	catch (exception_aborted) {
		throw;
	}
	catch (foo_discogs_exception &e) {
		e << "(url: " << url << ")";
		pfc::string8 msg("Exception handling: ");
		msg << req_url;
		log_msg(msg);
		throw;
	}
	catch (foobar2000_io::exception_io &e) {
		network_exception ex("Network exception");
		ex << e.what() << " (url: " << url << ")";
		pfc::string8 msg("Network exception handling: ");
		msg << req_url;
		log_msg(msg);
		throw ex;
	}
	catch (...) {
		out.set_size(0);
		network_exception ex("Unknown network exception.");
		ex << "(url: " << url << ")";
		pfc::string8 msg("Unknown network exception handling: ");
		msg << req_url;
		log_msg(msg);
		throw ex;
	}
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
