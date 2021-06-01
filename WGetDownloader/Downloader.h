#pragma once
#include <DPLib.conf.h>
#include <Generator/CodeGenerator.h>

using __DP_LIB_NAMESPACE__::String;
using __DP_LIB_NAMESPACE__::UInt;
using __DP_LIB_NAMESPACE__::Map;

struct ProxyElement{
	String host;
	UInt port;
	enum class Type {Unknown, Socks4, Socks5, Http};
	Type type = Type::Unknown;

	inline ProxyElement() { type = Type::Unknown; host =""; port = 0; }

	String ToString()const;
	inline friend bool operator==(const ProxyElement & el1, const ProxyElement & el2) {
		return el1.host == el2.host && el1.port == el2.port && el1.type == el2.type;
	}
};

ProxyElement parseProxy(const String & proxy);

namespace __DP_LIB_NAMESPACE__ {
	class Application;
}

class Downloader{
	private:
		ProxyElement _proxy;
		String wget = "wget";
		Map<String, String> headers;
		bool ignoreCheckCert = false;

		__DP_LIB_NAMESPACE__::Application * getBaseApplication() const;
	public:
		Downloader(const ProxyElement & proxy = ProxyElement()):_proxy(proxy) {}
		bool Download(const String & url, const String & saveTo);
		String Download(const String & url);
		inline void SetWget(const String & wg) { wget = wg; }
		inline void SetProxy(const ProxyElement & proxy) { _proxy=proxy; }
		inline String& operator[](const String & name) { return headers[name]; }
		inline void SetIgnoreCheckCert(bool ignore) { ignoreCheckCert = ignore; }

};

DP_SINGLTONE_CLASS(Downloader, DownloaderManeger, Get, GetRef, Create)
