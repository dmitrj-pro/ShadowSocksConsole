#include "Downloader.h"
#include <_Driver/Application.h>
#include <Log/Log.h>
#include <Converter/Converter.h>

using __DP_LIB_NAMESPACE__::Application;
using __DP_LIB_NAMESPACE__::log;
using __DP_LIB_NAMESPACE__::toString;
using __DP_LIB_NAMESPACE__::parse;

DP_SINGLTONE_CLASS_CPP(Downloader, DownloaderManeger)



Application * Downloader::getBaseApplication() const {
	Application * _app = new Application(wget);
	Application & app = *_app;

	if (_proxy.type != ProxyElement::Type::Unknown) {
		log << "Proxy is set\n";
		if (_proxy.type != ProxyElement::Type::Http) {
			throw EXCEPTION("Downloader support http proxy only");
		}
		String proxy = _proxy.host + ":" + toString(_proxy.port);
		app << "-e" << "use_proxy=yes" << "-e" << ("http_proxy=" + proxy) << "-e" << ("https_proxy=" + proxy);
	}
	for (const auto & it : headers) {
		String head = "--header=\"" + it.first + ": " + it.second + "\"";
		app << head;
	}
	app << "--tries=3" << "--connect-timeout=300";
	if (ignoreCheckCert)
		app << "--no-check-certificate";

	return _app;
}

bool Downloader::Download(const String & url, const String & saveTo) {
	Application * app = getBaseApplication();
	(*app) << "--output-document" << saveTo;
	(*app) << url;
	app->SetReadedFunc([](const String & str) {
		//log << str << "\n";
	});
	app->Exec();
	bool res = app->ResultCode() == 0;
	delete app;
	return res;
}

String Downloader::Download(const String & url) {
	Application * app = getBaseApplication();
	(*app) << "-qO-";
	(*app) << url;
	String res = app->ExecAll();
	if (app->ResultCode() != 0) {
		delete app;
		throw EXCEPTION("Fail to download: " + res);
	}
	delete app;
	return res;
}

bool parse_domain(const String & _domain, ProxyElement::Type & protocol, String & host, String & path, unsigned int & port, bool & is_ssl) {
	String domain = _domain;
	String prev = "";
	// 0 - protocol
	// 1 - domain
	// 2 - port
	// 3 - path
	int current_mode = 0;
	bool is_ok = true;
	// parse
	// ${protocol}://${domain}:${port}/${path}
	// ${domain}:${port}/${path}
	// ${protocol}://${domain}/${path}
	//Tests:
	// https://localhost.com:80443/update
	// http://localhost.com:8080/update
	// https://localhost.com/update
	// http://localhost.com/update
	// https://localhost.com
	// http://localhost.com
	// localhost.com:443/update
	// localhost.com:80/update
	// localhost.com:80
	// localhost.com:443
	// localhost/update
	bool ssl_set = false;
	for (int i = 0; i < domain.size(); i++) {
		if (domain[i] == ':') {
			if (current_mode == 0) {
				if (prev == "http" || prev == "HTTP" || prev == "https" || prev == "HTTPS") {
					protocol = ProxyElement::Type::Http;
					is_ssl = (prev == "https" || prev == "HTTPS") ? true : false;
					port = 3128;
					current_mode ++;
					prev = "";
					// skeep //
					i+=2;
					ssl_set = true;
					continue;
				}
				if (prev == "socks4" || prev == "SOCKS4") {
					is_ssl = true;
					protocol = ProxyElement::Type::Socks4;
					port = 1085;
					current_mode ++;
					prev = "";
					ssl_set = true;
					// skeep //
					i+=2;
					continue;
				}
				if (prev == "socks" || prev == "SOCKS" || prev == "socks5" || prev == "SOCKS5") {
					is_ssl = true;
					protocol = ProxyElement::Type::Socks5;
					port = 1085;
					current_mode ++;
					prev = "";
					ssl_set = true;
					// skeep //
					i+=2;
					continue;
				}
				// get domain.ru:port/
				host = prev;
				prev = "";
				current_mode = 2;
				continue;
			}
			if (current_mode == 1) {
				host = prev;
				prev = "";
				current_mode++;
				continue;
			}
		}
		if (domain[i] == '/') {
			if (current_mode==0) {
				host = prev;
				prev="";
				current_mode = 3;
				i-=1;
				continue;
			}
			//Port skyped
			if (current_mode == 1) {
				host = prev;
				prev = "";
				current_mode+=2;
				i-=1;
				continue;
			}
			if (current_mode == 2) {
				int _port = parse<int>(prev);
				if (!is_ok)
					break;
				port = _port;
				i-=1;
				current_mode ++;
				if (!ssl_set) {
					if (port == 80)
						is_ssl = false;
					else
						is_ssl = true;
				}
				prev="";
				continue;
			}
		}
		prev += domain[i];
	}
	// http://localhost:80/update
	if (current_mode == 3) {
		path = prev;
	}
	// http://localhost:80
	if (current_mode == 2) {
		path = "/";
		int _port = parse<int>(prev);
		if (is_ok)
			port=_port;
		if (!ssl_set) {
			if (port == 80)
				is_ssl = false;
			else
				is_ssl = true;
		}
	}
	// http://localhost
	if (current_mode == 1) {
		path = "/";
		host=prev;
	}
	// localhost
	if (current_mode == 0) {
		path = "/";
		is_ssl = true;
		port=443;
		host = prev;
	}
	return is_ok;
}

ProxyElement parseProxy(const String & proxy) {
	if (proxy.size() == 0)
		return ProxyElement();
	bool is_ssl;
	String path;
	ProxyElement elem;
	if (parse_domain(proxy, elem.type, elem.host, path, elem.port, is_ssl) && elem.type != ProxyElement::Type::Unknown) {
		return elem;
	} else {
		log << "Proxy is not parsed\n";
		return ProxyElement();
	}
}
