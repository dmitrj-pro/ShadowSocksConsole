#pragma once
#include <DPLib.conf.h>
#include "ShadowSocksMain.h"
#include <Generator/CodeGenerator.h>
#include "Tun2Socks.h"
#include <functional>
#include <Network/TCPServer.h>
#include <mutex>

namespace __DP_LIB_NAMESPACE__ {
	class Application;
	struct Thread;
}

#define DP_PRINT_VAL_1(X, VALUE) \
{\
	DP_LOG_DEBUG << #X << " = '" << VALUE << "'\n";\
}
#define DP_PRINT_VAL_0(X) \
{\
	DP_LOG_DEBUG << #X << " = '" << X << "'\n";\
}
#define DP_PRINT_TEXT(X) \
{\
	DP_LOG_DEBUG << X << "'\n";\
}

class Node;

enum class ShadowSocksClientStatus { None, Running, Started, DoStop, Stoped};

class ShadowSocksClient{
	private:
		OnShadowSocksError _onCrash = nullptr;
		std::mutex _locker;
		bool _is_closed = false;
		_Server * srv = nullptr;
		_Task * tk = nullptr;
		_RunParams run_params;
		Tun2SocksConfig t2s;
		_RunParams::ShadowSocksType shadowsocks_type = _RunParams::ShadowSocksType::None;

		String v2rayPlugin = "D:\\tmp\\plugin.exe";
		String shadowSocks = "D:\\tmp\\demo.exe";
		String tempPath = "./";
		unsigned int udpTimeout;

		__DP_LIB_NAMESPACE__::Application * _socks = nullptr;
		__DP_LIB_NAMESPACE__::Application * _socksUdp = nullptr;
		__DP_LIB_NAMESPACE__::Thread * _http_server_thread = nullptr;
		__DP_LIB_NAMESPACE__::TCPServer _http_server;
		Node * http_connector_node = nullptr;
		__DP_LIB_NAMESPACE__::Thread * _sysproxy = nullptr;
		bool _is_exit = false;
		UInt sleepMS = 5000;
		bool is_run_vpn = false;

		__DP_LIB_NAMESPACE__::Thread * _multimode_server_thread = nullptr;
		__DP_LIB_NAMESPACE__::TCPServer _multimode_server;
		struct MultiModeServerStruct{
			_Server * srv = nullptr;
			__DP_LIB_NAMESPACE__::Application * v2ray = nullptr;
			String host = "127.0.0.1";
			unsigned short port = 0;
		};

		__DP_LIB_NAMESPACE__::Vector<MultiModeServerStruct> _multimode_servers;


		Tun2Socks * tun2socks = nullptr;

		ShadowSocksClientStatus status = ShadowSocksClientStatus::None;

		void PreStartChecking(SSClientFlags flags);
		void StartMultiMode(SSClientFlags flags);
		void GenerateCMDGO(SSClientFlags flags);
		String GenerateConfigRust(SSClientFlags flags, TunType type);
		String GenerateConfigRustName(TunType type);
		String GenerateConfigRustTCP(SSClientFlags flags);
		String GenerateConfigRustUDP(SSClientFlags flags);
		void StartRustUdp(SSClientFlags flags);
		// Return path to config
		String GenerateCMDRust(SSClientFlags flags);
		// return true if started
		bool waitForStart();
		// return true if started
		bool startTun2Socks(SSClientFlags flags, OnShadowSocksRunned _onSuccess);
		bool startHttpProxy(SSClientFlags flags, OnShadowSocksRunned _onSuccess);

	public:
		inline void SetOnCrash(OnShadowSocksError f) { _onCrash = f; }
		void onCrash(const ExitStatus & status);
		~ShadowSocksClient() {
			delete srv;
			delete tk;
		}
		inline const _Task * getTask() const { return tk; }
		inline const _Server * getServer() const { return srv; }
		inline const _RunParams& getRunParams() const { return run_params; }
		inline bool vpnRun() const { return is_run_vpn; }

		static bool portIsAllow(const String & host, UInt port);
		ShadowSocksClient(_Server * sr, _Task * t, const _RunParams & run_params, const Tun2SocksConfig & t2s) :srv(sr), tk(t), run_params(run_params), t2s(t2s) {}
		void Start(SSClientFlags flags, OnShadowSocksRunned _onSuccess);
		void Stop(bool is_async = true);
		void Stop(const ExitStatus & status);
		void _Stop();
		void _Stop(const ExitStatus & status);
		void SetSystemProxy();
		inline void SetMultiModeServers(__DP_LIB_NAMESPACE__::List<_Server * > srvs) {
			_multimode_servers.reserve(srvs.size());
			for (_Server * s : srvs) {
				MultiModeServerStruct data;
				data.srv = s;
				_multimode_servers.push_back(data);
			}
		}
		static UInt findAllowPort(const String & host);
		DP_add_setter_name(String, shadowSocks, ShadowSocksPath)
		DP_add_setter_name(_RunParams::ShadowSocksType, shadowsocks_type, ShadowSocksType)
		DP_add_setter_name(String, v2rayPlugin, V2RayPluginPath)
		DP_add_setter_name(String, tempPath, TempPath)
		DP_add_setter_name(unsigned int, udpTimeout, UDPTimeout)
		DP_add_getter_name(ShadowSocksClientStatus, status, Status)

};
