#include "Downloader.h"
#include <_Driver/Application.h>
#include <Log/Log.h>
#include <Converter/Converter.h>
#include "libproxy/Chain.h"
#include "libproxy/Connectors/Connector.h"
#include "libproxy/Connectors/ConnectorSocks.h"
#include <Network/Utils.h>
#include "libproxy/Connectors/ConnectorDirect.h"
#include <Network/TCPServer.h>
#include <_Driver/ThreadWorker.h>
#include "libproxy/ProtoParsers/SocketParserMultiplex.h"

using __DP_LIB_NAMESPACE__::Application;
using __DP_LIB_NAMESPACE__::log;
using __DP_LIB_NAMESPACE__::toString;
using __DP_LIB_NAMESPACE__::parse;

DP_SINGLTONE_CLASS_CPP(Downloader, DownloaderManeger)

unsigned short findAllowPort(const String & host) {
	while (true) {
		unsigned short v = rand() % 65025 + 1024;
		if (__DP_LIB_NAMESPACE__::TCPServer::portIsAllow(host, v))
			return v;
	}
	return 0;
}

Application * Downloader::getBaseApplication(Node ** proxy, unsigned short & port) const {
	Application * _app = new Application(wget);
	Application & app = *_app;

	if (_proxy.type != URLElement::Type::Unknown) {
		DP_LOG_DEBUG << "Proxy is set\n";
		if (_proxy.type == URLElement::Type::Socks5) {
			(*proxy) = new Node(new ConnectorDirect());
			(*proxy) = new Node(new ConnectorSocks(_proxy.host,  _proxy.port), (*proxy));
			port = findAllowPort("127.0.0.1");
			String proxy = String("127.0.0.1") + ":" + toString(port);
			app << "-e" << "use_proxy=yes" << "-e" << ("http_proxy=" + proxy) << "-e" << ("https_proxy=" + proxy);
		}
		if (_proxy.type == URLElement::Type::Http) {
			String proxy = _proxy.host + ":" + toString(_proxy.port);
			app << "-e" << "use_proxy=yes" << "-e" << ("http_proxy=" + proxy) << "-e" << ("https_proxy=" + proxy);
		}
		if (_proxy.type != URLElement::Type::Http && _proxy.type != URLElement::Type::Socks5) {
			throw EXCEPTION("Downloader support http and socks5 proxy only");
		}

	}
	for (const auto & it : headers) {
		String head = "--header=\"" + it.first + ": " + it.second + "\"";
		app << head;
	}
	app << ("--tries=" + toString(this->count_try)) << ("--timeout=" + toString(timeout_s));
	if (ignoreCheckCert)
		app << "--no-check-certificate";

	return _app;
}

bool Downloader::Download(const String & url, const String & saveTo) {
	Node * n;
	unsigned short proxy_port = 0;
	Application * app = getBaseApplication(&n, proxy_port);

	__DP_LIB_NAMESPACE__::TCPServer server;
	__DP_LIB_NAMESPACE__::Thread server_thread;

	if (proxy_port > 0) {
		DP_LOG_DEBUG << "Try download " << url << " (" << saveTo <<") throught proxy http://127.0.0.1:" << proxy_port << " => socks5://" << _proxy.host << ":" << _proxy.port;
		auto makeConnect = [n] (const std::string & host, unsigned short port) {
			return n->makeConnect(host, port);
		};
		server_thread=__DP_LIB_NAMESPACE__::Thread([makeConnect, &server, proxy_port](){
			server.ThreadListen("127.0.0.1", proxy_port, [makeConnect](__DP_LIB_NAMESPACE__::TCPServerClient cl){
				SocketParserMultiplex parser;
				SocketReader buff(&cl);
				parser.tryParse(&buff, makeConnect);
			});
		});
		server_thread.SetName("ProxyServer for WGet");
		server_thread.start();
	} else
		DP_LOG_DEBUG << "Try direct download " << url << " (" << saveTo <<")";

	(*app) << "--output-document" << saveTo;
	(*app) << url;
	app->SetReadedFunc([](const String & ) {

	});
	app->Exec();
	bool res = app->ResultCode() == 0;
	delete app;

	if (proxy_port > 0) {
		server.exit();
		server_thread.join();
	}

	return res;
}

String Downloader::Download(const String & url) {
	Node * n;
	unsigned short proxy_port = 0;

	Application * app = getBaseApplication(&n, proxy_port);

	__DP_LIB_NAMESPACE__::TCPServer server;
	__DP_LIB_NAMESPACE__::Thread server_thread;

	if (proxy_port > 0) {
		DP_LOG_DEBUG << "Try get " << url << " throught proxy http://127.0.0.1:" << proxy_port << " => socks5://" << _proxy.host << ":" << _proxy.port;
		auto makeConnect = [n] (const std::string & host, unsigned short port) {
			return n->makeConnect(host, port);
		};
		server_thread=__DP_LIB_NAMESPACE__::Thread([makeConnect, &server, proxy_port](){
			server.ThreadListen("127.0.0.1", proxy_port, [makeConnect](__DP_LIB_NAMESPACE__::TCPServerClient cl){
				SocketParserMultiplex parser;
				SocketReader buff(&cl);
				parser.tryParse(&buff, makeConnect);
			});
		});
		server_thread.SetName("ProxyServer for WGet");
		server_thread.start();
	} else
		DP_LOG_DEBUG << "Try direct get " << url;

	(*app) << "-qO-";
	(*app) << url;
	String res = app->ExecAll();
	if (app->ResultCode() != 0) {
		delete app;
		throw EXCEPTION("Fail to download: " + res);
	}
	delete app;

	if (proxy_port > 0) {
		server.exit();
		server_thread.join();
	}
	return res;
}
