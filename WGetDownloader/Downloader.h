#pragma once
#include <DPLib.conf.h>
#include <Generator/CodeGenerator.h>
#include <Network/Utils.h>
#include <Network/TCPServer.h>

using __DP_LIB_NAMESPACE__::String;
using __DP_LIB_NAMESPACE__::UInt;
using __DP_LIB_NAMESPACE__::Map;
using __DP_LIB_NAMESPACE__::URLElement;

namespace __DP_LIB_NAMESPACE__ {
	class Application;
}

class Node;

class Downloader{
	private:
		URLElement _proxy;
		String wget = "wget";
		Map<String, String> headers;
		bool ignoreCheckCert = false;
		unsigned int timeout_s = 60;
		unsigned short count_try = 3;

		__DP_LIB_NAMESPACE__::Application * getBaseApplication(Node ** proxy, unsigned short & port) const;
	public:
		Downloader(const URLElement & proxy = URLElement()):_proxy(proxy) {}
		bool Download(const String & url, const String & saveTo);
		String Download(const String & url);
		inline void SetWget(const String & wg) { wget = wg; }
		inline void SetProxy(const URLElement & proxy) { _proxy=proxy; }
		inline String& operator[](const String & name) { return headers[name]; }
		inline void SetIgnoreCheckCert(bool ignore) { ignoreCheckCert = ignore; }
		DP_add_getter_setter_name(unsigned int, timeout_s, TimeoutS)
		DP_add_getter_setter_name(unsigned short, count_try, CountTry);

};

unsigned short findAllowPort(const String & host);

DP_SINGLTONE_CLASS(Downloader, DownloaderManeger, Get, GetRef, Create)
