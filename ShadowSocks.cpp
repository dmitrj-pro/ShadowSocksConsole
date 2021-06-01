#include "ShadowSocks.h"
#include "TCPServer.h"
#include "TCPClient.h"
#include <thread>
#include <_Driver/Application.h>
#include <Converter/Converter.h>
#include <thread>
#include <chrono>
#include <_Driver/Path.h>
#include <Types/Exception.h>

using std::thread;
using __DP_LIB_NAMESPACE__::Path;
using __DP_LIB_NAMESPACE__::toString;
using __DP_LIB_NAMESPACE__::Ofstream;



bool ShadowSocksClient::portIsAllow(const String & host, UInt port){
	DP::NET::Server::Server _portCheckerServer;
	return _portCheckerServer.portIsAllow(host, port);
}

UInt ShadowSocksClient::findAllowPort(const String & host) {
	while (true) {
		UInt v = rand() % 65025 + 1024;
		if (portIsAllow(host, v))
			return v;
	}
	return 0;
}

String ReadAllFile(const String & file);

void ShadowSocksClient::Start(SSClientFlags flags, std::function<void()> _onSuccess) {
	if (flags.port > 5)
		tk->localPort = flags.port;
	if (flags.http_port > 5)
		tk->httpProxy = flags.http_port;
	_is_exit = false;
	if (!portIsAllow(tk->localHost, tk->localPort))
		throw EXCEPTION("Socks5 port is not allow");
	if (tk->httpProxy > 0 && !portIsAllow(tk->localHost, tk->httpProxy))
		throw EXCEPTION("Http port is not allow");
	_locker.lock();
	_V2RayServer * ser = dynamic_cast<_V2RayServer * >(srv);
	String host = srv->host;
	String port = srv->port;

	_socks = new __DP_LIB_NAMESPACE__::Application(shadowSocks);
	__DP_LIB_NAMESPACE__::Application & s = *_socks;

	if (ser != nullptr) {
		DP_PRINT_VAL_0(v2rayPlugin);
		s << "-plugin" << ("\"" + v2rayPlugin + "\"");
		String cmd = "mode=" + ser->mode + ";";
		if (ser->isTLS)
			cmd += "tls;";
		cmd += "host=" + ser->v2host + ";path=" + ser->path;
		s << "-plugin-opts" << ("\"" + cmd + "\"");

		/*
		_plugin = new __DP_LIB_NAMESPACE__::Application(v2rayPlugin);
		__DP_LIB_NAMESPACE__::Application & plugin = *_plugin;
		plugin << "-localAddr" << "127.0.0.1";
		port = findAllowPort(host);
		plugin << "-localPort" << port;
		plugin << "-remoteAddr" << ser->host;
		plugin << "-remotePort" << ser->port;
		plugin << "-mode" << ser->mode;
		if (ser->isTLS)
			plugin << "-tls";
		plugin << "-host" << ser->v2host;
		host = "127.0.0.1";
		plugin.SetOnCloseFunc(std::bind(&ShadowSocksClient::onCrash, this));
		plugin.ExecInNewThread();
		plugin.WaitForStart();*/
	}
	// Включить тунелирование UDP
	s << "-u";
	s << "-udptimeout" << this->udpTimeout;
	s << "-password" << tk->password;
	s << "-c" << (host + ":" + __DP_LIB_NAMESPACE__::toString(port));
	s << "-socks" << (tk->localHost + ":" + __DP_LIB_NAMESPACE__::toString(tk->localPort));
	s << "-cipher" << tk->method;

	{
		__DP_LIB_NAMESPACE__::OStrStream tcpTuns;
		__DP_LIB_NAMESPACE__::OStrStream udpTuns;;

		for (_Tun tun : tk->tuns) {
			DP_PRINT_VAL_0(tun.localPort);
			if (tun.type == TunType::TCP)
				tcpTuns << tun.localHost << ":" << tun.localPort << "=" << tun.remoteHost << ":" << tun.remotePort << ",";
			if (tun.type == TunType::UDP)
				udpTuns << tun.localHost << ":" << tun.localPort << "=" << tun.remoteHost << ":" << tun.remotePort << ",";
		}
		String tcp = tcpTuns.str();
		String udp = udpTuns.str();
		if (tcp.size() > 0) {
			tcp = tcp.substr(0, tcp.size()-1);
			s << "-tcptun" << tcp;
		}
		if (udp.size() > 0) {
			udp = udp.substr(0, udp.size() - 1);
			s << "-udptun" << udp;
		}
	}
	s.SetOnCloseFunc(std::bind(&ShadowSocksClient::onCrash, this));

	s.ExecInNewThread();
	s.WaitForStart();
	if (flags.runVPN && t2s.name.size() > 0) {
		DP_PRINT_TEXT("Try start VPN mode");
		tun2socks = new Tun2Socks();
		tun2socks->config = t2s;
		tun2socks->proxyPort = tk->localPort;
		tun2socks->proxyServer = tk->localHost;
		tun2socks->config.ignoreIP.push_back(srv->host);
		tun2socks->SetUDPTimeout(this->udpTimeout);
		tun2socks->Start([this, _onSuccess] () {
			if (!_socks->isFinished())
				_onSuccess();
		},std::bind(&ShadowSocksClient::onCrash, this));
		tun2socks->WaitForStart();
	}
	if (tk->httpProxy > 0) {
		DP_PRINT_TEXT("Try start HTTP-Proxy");
		Path p = Path(tempPath);
		p.Append(srv->host + toString(tk->httpProxy) +  ".conf");
		DP_PRINT_VAL_0(p.Get());
		Ofstream out;
		out.open(p.Get());
		if (out.fail()) {
			Stop();
			throw EXCEPTION("Fail to write temp file");
		}
		out << "proxyAddress = \"" << tk->localHost << "\"\n";
		out << "proxyPort = " << tk->httpProxy << "\n";
		out << "allowedClients = \"0.0.0.0/0\"\n";
		out << "allowedPorts = 1-65535\n";
		out << "proxyName = \"ss.dpapp.proxy\"\n";
		out << "cacheIsShared = false\n";
		out << "socksParentProxy = \"" << tk->localHost << ":" << tk->localPort << "\"\n";
		out << "socksProxyType = socks5\n";
		out << "localDocumentRoot = \"\"\n";
		out << "disableVia=false\n";
		out << "censoredHeaders = from, accept-language\n";
		out << "censorReferer = no\n";
		out << "tunnelAllowedPorts = 1-65535\n";
		out.flush();
		out.close();

		__DP_LIB_NAMESPACE__::log << "Polipo config: \n" << ReadAllFile(p.Get()) << "\n";

		_polipo = new Application(polipoPath);
		(*_polipo) << "-c" << p.Get();
		_polipo->SetOnCloseFunc(std::bind(&ShadowSocksClient::onCrash, this));
		_polipo->ExecInNewThread();
		_polipo->WaitForStart();
		if ((!_socks->isFinished()) && (!_polipo->isFinished()))
			_onSuccess();
	}
	if (tun2socks == nullptr && _polipo == nullptr && (!_socks->isFinished())) {
		_onSuccess();
	}
	if (tk->systemProxy && tk->httpProxy > 0) {
		DP_PRINT_TEXT("Try start sys proxy");
		_sysproxy = new std::thread(&ShadowSocksClient::SetSystemProxy, this);
	}
	_locker.unlock();
}

void ShadowSocksClient::onCrash() {
	_locker.lock();
	if (_is_exit) {
		_locker.unlock();
		return;
	}

	if (tun2socks != nullptr)
		tun2socks->DisableCrashFunc();
	if (_polipo != nullptr)
		_polipo->SetOnCloseFunc(nullptr);
	if (_socks != nullptr)
		_socks->SetOnCloseFunc(nullptr);
	if (_plugin != nullptr)
		_plugin->SetOnCloseFunc(nullptr);
	_locker.unlock();

	Stop();
	if (_onCrash != nullptr)
		_onCrash();
}

void ShadowSocksClient::SetSystemProxy() {
	while (!_is_exit) {
		Application sysprox(sysProxyPath);
		sysprox << "global" << (tk->localHost + ":" + __DP_LIB_NAMESPACE__::toString(tk->httpProxy)) << "localhost;127.*";
		sysprox.Exec();
		std::this_thread::sleep_for(std::chrono::milliseconds(sleepMS));
	}
}

void ShadowSocksClient::_Stop() {
	_is_exit = true;
	if (tun2socks != nullptr) {
		tun2socks->Stop();
		//delete tun2socks;
		tun2socks = nullptr;
	}
	if (tk->systemProxy && tk->httpProxy > 0) {
		_sysproxy->join();
		delete _sysproxy;
		_sysproxy = nullptr;
		Application app(sysProxyPath);
		app << "off";
		app.Exec();
	}
	if (_polipo != nullptr) {
		_polipo ->KillNoJoin();
		_polipo = nullptr;
	}
	if (_socks != nullptr) {
		_socks->KillNoJoin();
		_socks = nullptr;
	}
	if (_plugin != nullptr) {
		_plugin->KillNoJoin();
		_plugin = nullptr;
	}
}

void ShadowSocksClient::Stop(bool is_async) {
	//_Stop();
	if (is_async) {
		std::thread * th = new std::thread(&ShadowSocksClient::_Stop, this);
		std::this_thread::sleep_for(std::chrono::seconds(1));
	} else
		_Stop();
	//th->join();
	//ToDo
	//delete th;
}
