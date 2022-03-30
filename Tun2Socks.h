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


class WordReader{
	private:
		String text;
		unsigned int pos = 0;
		List<char> delimers;
	public:
		WordReader(const String & text, const List<char> & delimers) : text(text), pos(0), delimers(delimers) {}
		inline bool isEnd() const { return pos >= text.size(); }
		String read() {
			while (text[pos] == ' ' || text[pos] == '\t' || text[pos] == '\n')
				pos ++;
			String res = "";
			char c = text[pos];
			if (__DP_LIB_NAMESPACE__::ConteinsElement(delimers, c)) {
				pos++;
				//res += c;
				return res;
			}
			while (!(text[pos] == ' ' || text[pos] == '\t' || text[pos] == '\n')) {
				if (__DP_LIB_NAMESPACE__::ConteinsElement(delimers, c)) {
					break;
				}
				res += c;
				pos++;
				if (isEnd())
					break;
				c = text[pos];
			}
			return res;
		}
};

class LiteralReader{
	private:
		WordReader reader;
		static List<char> delimers;
	public:
		LiteralReader(const String & str) : reader(WordReader(str, delimers)) {}
		LiteralReader(const String & str, const List<char> & delimer) : reader(WordReader(str, delimer)) {}
		inline bool isEnd() const { return reader.isEnd(); }
		List<String> read() {
			List<String> res;
			bool started = false;
			while (true) {
				if (reader.isEnd())
					return List<String>();
				String l = reader.read();
				if (l.size() == 0)
					continue;
				if (started && l == "]") {
					started = false;
					break;
				}
				if (l == "[") {
					started = true;
					continue;
				}
				if (started && l == ",") {
					continue;
				}
				res.push_back(l);
				if (!started)
					break;
			}
			return res;
		}
		static List<List<String>> readAllLiterals(const String & str) {
			LiteralReader reader(str);
			List<List<String>> res;
			while (!reader.isEnd()) {
				auto tmp = reader.read();
				if (tmp.size() == 0)
					return res;
				res.push_back(tmp);
			}
			return res;
		}
};
