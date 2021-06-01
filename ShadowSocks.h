#pragma once
#include <DPLib.conf.h>
#include "ShadowSocksMain.h"
#include <Generator/CodeGenerator.h>
#include "Tun2Socks.h"
#include <functional>
#include <mutex>

namespace std {
	class thread;
}

namespace __DP_LIB_NAMESPACE__ {
	class Application;
}

#define DP_PRINT_VAL_1(X, VALUE) \
{\
	__DP_LIB_NAMESPACE__::log << __FILE__ << ":" << __LINE__ << " " << #X << " = '" << VALUE << "'\n";\
}
#define DP_PRINT_VAL_0(X) \
{\
	__DP_LIB_NAMESPACE__::log << __FILE__ << ":" << __LINE__ << " " << #X << " = '" << X << "'\n";\
}
#define DP_PRINT_TEXT(X) \
{\
	__DP_LIB_NAMESPACE__::log << __FILE__ << ":" << __LINE__ << " " << X << "'\n";\
}

struct SSClientFlags{
	bool runVPN = true;
	String vpnName = "";
	unsigned short port = 1;
	String server_name = "";
	unsigned short http_port = 1;
};

class ShadowSocksClient{
	private:
		std::function<void()> _onCrash = nullptr;
		std::mutex _locker;
		bool _is_closed = false;
		_Server * srv;
		_Task * tk;
		Tun2SocksConfig t2s;

		String v2rayPlugin = "D:\\tmp\\plugin.exe";
		String shadowSocks = "D:\\tmp\\demo.exe";
		String polipoPath = "D:\\tmp\\polipo.exe";
		String sysProxyPath = "D:\\tmp\\sysproxy.exe";
		String tempPath = "./";
		String udpTimeout = "1m0s";

		__DP_LIB_NAMESPACE__::Application * _socks = nullptr;
		__DP_LIB_NAMESPACE__::Application * _plugin = nullptr;
		__DP_LIB_NAMESPACE__::Application * _polipo = nullptr;
		std::thread * _sysproxy = nullptr;
		bool _is_exit = false;
		UInt sleepMS = 5000;

		Tun2Socks * tun2socks = nullptr;

	public:
		inline void SetOnCrash(std::function<void()> f) { _onCrash = f; }
		void onCrash();
		~ShadowSocksClient() {
			delete srv;
			delete tk;
		}
		inline const _Task * getTask() const { return tk; }
		inline const _Server * getServer() const { return srv; }

		static bool portIsAllow(const String & host, UInt port);
		ShadowSocksClient(_Server * sr, _Task * t, const Tun2SocksConfig & t2s) :srv(sr), tk(t), t2s(t2s) {}
		void Start(SSClientFlags flags, std::function<void()> _onSuccess);
		void Stop(bool is_async = true);
		void _Stop();
		void SetSystemProxy();
		static UInt findAllowPort(const String & host);
		DP_add_setter_name(String, shadowSocks, ShadowSocksPath)
		DP_add_setter_name(String, v2rayPlugin, V2RayPluginPath)
		DP_add_setter_name(String, polipoPath, PolipoPath)
		DP_add_setter_name(String, sysProxyPath, SysProxyPath)
		DP_add_setter_name(String, tempPath, TempPath)
		DP_add_setter_name(String, udpTimeout, UDPTimeout)

};
