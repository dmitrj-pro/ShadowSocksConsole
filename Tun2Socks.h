#pragma once
#include <DPLib.conf.h>
#include <_Driver/Application.h>
#include <_Driver/ThreadWorker.h>
#include "ShadowSocksMain.h"
#include <Converter/Converter.h>
#include <Generator/CodeGenerator.h>

using __DP_LIB_NAMESPACE__::Application;
using __DP_LIB_NAMESPACE__::String;
using __DP_LIB_NAMESPACE__::List;
using __DP_LIB_NAMESPACE__::UInt;

struct Tun2Socks {
	public:
		Tun2SocksConfig config;

		UInt sleepMS = 5000;
		String proxyServer;
		UInt proxyPort;
		String udpTimeout = "1m0s";
		bool enable_udp = true;

		bool & _is_exit;
		__DP_LIB_NAMESPACE__::Thread * deleteDefault = nullptr;

		#ifdef DP_WIN
			String DetectTunAdapterIF(const String & name);
			void WaitForIFInited(const String & name);
		#endif
		#ifdef DP_LIN
			String iface_name = "";
			String iface_gw = "";
		#endif
		static String DetectDefaultRoute();
		static String DetectInterfaceName();

	private:
		static String appPath;
		Application * run = nullptr;

		static String tun2socksPath;
		Application * tun2socks = nullptr;

	public:
		Tun2Socks():_is_exit(*(new bool(false))) {}
		inline static void SetT2SPath(const String & p) { appPath = p; }
		inline static void SetD2SPath(const String & p) { tun2socksPath = p; }
		void Start(std::function<void()> _onSuccess, OnShadowSocksError _onCrash);
		void Stop();
		void DisableCrashFunc();
		void ThreadLoop();
		void WaitForStart();
		DP_add_setter_name(String, udpTimeout, UDPTimeout)

};


