#include "ShadowSocks.h"
#include <Network/TCPClient.h>
#include <Network/TCPServer.h>
#include <thread>
#include <_Driver/ThreadWorker.h>
#include <_Driver/Application.h>
#include <Converter/Converter.h>
#include <thread>
#include <chrono>
#include <_Driver/Path.h>
#include <Types/Exception.h>
#include <_Driver/Files.h>
#include <Network/proxy/Chain.h>
#include <Network/proxy/Connector.h>
#include <Network/proxy/ConnectorDirect.h>
#include <Network/proxy/ConnectorHttpProxy.h>
#include <Network/proxy/ConnectorSocks.h>
#include <Addon/libproxy/ProtoParsers/SocketParserMultiplex.h>
#include <Addon/libproxy/ProtoParsers/SocketParserSocks5.h>
#include <string>

using __DP_LIB_NAMESPACE__::Path;
using __DP_LIB_NAMESPACE__::toString;
using __DP_LIB_NAMESPACE__::Ofstream;



bool ShadowSocksClient::portIsAllow(const String & host, UInt port){
	return __DP_LIB_NAMESPACE__::TCPServer::portIsAllow(host, port);
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

inline String metho_from_go_to_rust(const String & str) {
	if (str == "AEAD_AES_128_GCM")
		return "aes-128-gcm";
	if (str == "AEAD_AES_256_GCM")
		return "aes-256-gcm";
	if (str == "AEAD_CHACHA20_POLY1305")
		return "chacha20-ietf-poly1305";
	throw EXCEPTION("Can't convert " + str + " to shadowsocks-rust format");
}

String convertTime(UInt time);

bool socks5IsWork(const String & host, unsigned short port) {
	char c[3];
	c[0] = 5;
	c[1] = 1;
	c[2] = 0;
	__DP_LIB_NAMESPACE__::TCPClient connect;
	if (!connect.Connect(host, port))
		return false;
	connect.setReadTimeout(1);
	connect.Send(c, 3);

	if (connect.ReadN(c, 2) == 0) {
		connect.Close();
		return false;
	}
	if (c[0] != 5 || c[1] != 0) {
		connect.Close();
		return false;
	}
	connect.Close();
	return true;
}

String ecranire(const String & str) {
	String res = "";
	unsigned int pos = 0;
	res.resize(str.size() * 2);
	for (unsigned int i = 0 ; i < str.size() ; i++) {
		if (str[i] == '/' || str[i] == '"' || str[i] == '\\')
			res[pos++]='\\';
		res[pos++] = str[i];
	}
	res[pos] = 0;
	res.resize(pos);
	return res;
}

void ShadowSocksClient::PreStartChecking(SSClientFlags flags) {
	Path p {shadowSocks};
	if (!p.IsFile())
		throw EXCEPTION("ShadowSocks not found: " + shadowSocks);

	if (flags.port > 5)
		run_params.localPort = flags.port;
	if (flags.http_port > 5)
		run_params.httpProxy = flags.http_port;
	if (flags.sysproxy_s == SSClientFlagsBoolStatus::True)
		run_params.systemProxy = true;
	if (flags.sysproxy_s == SSClientFlagsBoolStatus::False)
		run_params.systemProxy = false;
	_is_exit = false;
	if (!portIsAllow(run_params.localHost, run_params.localPort))
		throw EXCEPTION("Socks5 port is not allow");
	if (run_params.httpProxy > 0 && !portIsAllow(run_params.localHost, run_params.httpProxy))
		throw EXCEPTION("Http port is not allow");
}

void ShadowSocksClient::StartMultiMode(SSClientFlags flags) {
	if (!run_params.multimode)
		return;

	if (this->_multimode_servers.size() == 0)
		throw EXCEPTION("Multiple connection mode need many servers, not none");
	srv = new _Server(-1);
	for (MultiModeServerStruct & data : this->_multimode_servers) {
		if (flags.runVPN && t2s.name.size() > 0)
			t2s.ignoreIP.push_back(data.srv->host);

		srv->name += data.srv->name + ", ";
		_V2RayServer * ser = dynamic_cast<_V2RayServer * >(data.srv);
		if (ser != nullptr) {
			data.host = run_params.localHost;
			data.port = findAllowPort(data.host);
			data.v2ray = new Application(v2rayPlugin);
			Application & app = *data.v2ray;
			app << "-host" << ser->v2host;
			app << "-localAddr" << data.host;
			app << "-localPort" << data.port;
			app << "-loglevel" << "none";
			app << "-mode" << ser->mode;
			app << "-path" << ser->path;
			app << "-remoteAddr" << ser->host;
			if (ser->isTLS)
				app << "-tls";

			app << "-remotePort" << ser->port;
			app.SetOnCloseFunc([this, app]() {
				ExitStatus status;
				status.code = ExitStatusCode::ApplicationError;
				status.str = "One of v2ray crashed";
				this->onCrash(status);
			});
			app.ExecInNewThread();
			app.WaitForStart();
		} else {
			data.port = __DP_LIB_NAMESPACE__::parse<unsigned short>(data.srv->port);
			data.v2ray = nullptr;
			data.host = "";
		}
	}
	srv->host = run_params.localHost;
	unsigned short port = findAllowPort(srv->host);
	srv->port = toString(port);
	_multimode_server_thread = new __DP_LIB_NAMESPACE__::Thread([this, port]() {
		unsigned short pos = 0;
		this->_multimode_server.ThreadListen(this->srv->host, port, [this, &pos](__DP_LIB_NAMESPACE__::TCPServerClient cl){
			const MultiModeServerStruct & data = this->_multimode_servers[pos % this->_multimode_servers.size()];
			try {
				pos = (pos + 1) % this->_multimode_servers.size();
				__DP_LIB_NAMESPACE__::TCPClient target;
				if (data.v2ray == nullptr)
					target.Connect(data.srv->host, data.port);
				else
					target.Connect(data.host, data.port);
				SocketReader buff(&cl);
				tcpLoop(buff, &target);
			} catch (const __DP_LIB_NAMESPACE__::LineException e) {
				DP_LOG_ERROR << "Catch exception: " << e.toString();
			} catch (const std::exception & e) {
				DP_LOG_ERROR << "Catch system exception" << e.what();
			} catch (...) {
				DP_LOG_ERROR << "Catch unknown exception";
			}
		});
	});
	_multimode_server_thread->start();
}

void ShadowSocksClient::GenerateCMDGO(SSClientFlags flags) {
	if (_socks == nullptr)
		_socks = new __DP_LIB_NAMESPACE__::Application(shadowSocks);
	__DP_LIB_NAMESPACE__::Application & s = *_socks;
	_V2RayServer * ser = dynamic_cast<_V2RayServer * >(srv);

	if (ser != nullptr) {
		DP_PRINT_VAL_0(v2rayPlugin);
		String cmd = "mode=" + ser->mode + ";";
		if (ser->isTLS)
			cmd += "tls;";
		cmd += "host=" + ser->v2host + ";path=" + ser->path;
		s << "-plugin" << v2rayPlugin;
		s << "-plugin-opts" << cmd ;
	}

	if (tk->enable_udp) {
		s << "-u";
		s << "-udptimeout" << convertTime(this->udpTimeout);
	}
	s << "-password" << tk->password;
	s << "-c" << (srv->host + ":" + __DP_LIB_NAMESPACE__::toString(srv->port));
	s << "-socks" << (run_params.localHost + ":" + __DP_LIB_NAMESPACE__::toString(run_params.localPort));
	s << "-cipher" << tk->method;

	__DP_LIB_NAMESPACE__::OStrStream tcpTuns;
	__DP_LIB_NAMESPACE__::OStrStream udpTuns;;

	if (!flags.disableTuns) {
		for (_Tun tun : tk->tuns) {
			DP_PRINT_VAL_0(tun.localPort);
			if (tun.type == TunType::TCP)
				tcpTuns << tun.localHost << ":" << tun.localPort << "=" << tun.remoteHost << ":" << tun.remotePort << ",";
			if (tun.type == TunType::UDP)
				udpTuns << tun.localHost << ":" << tun.localPort << "=" << tun.remoteHost << ":" << tun.remotePort << ",";
		}
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

void ShadowSocksClient::StartRustUdp(SSClientFlags flags) {
	if (!(shadowsocks_type == _RunParams::ShadowSocksType::Rust || shadowsocks_type == _RunParams::ShadowSocksType::Android))
		return;
	if (!tk->enable_udp)
		return;
	if (flags.disableTuns)
		return;
	bool generate = false;
	for (const _Tun & tun : tk->tuns)
		if (tun.type == TunType::UDP) {
			generate = true;
			break;
		}

	_socksUdp = new Application(shadowSocks);
	__DP_LIB_NAMESPACE__::Application & ss = *_socksUdp;

	String cmd_config = "";

	if (!generate) {
		ss << "-u";
		ss << "--udp-max-associations" << 512;
		ss << "--udp-timeout" << this->udpTimeout;
		if (tk->enable_ipv6)
			ss << "-6";
		ss << "--password" << tk->password;
		ss << "--server-addr" << (srv->host + ":" + __DP_LIB_NAMESPACE__::toString(srv->port));
		ss << "--local-addr" << (run_params.localHost + ":" + __DP_LIB_NAMESPACE__::toString(run_params.localPort));
		ss << "--encrypt-method" << metho_from_go_to_rust(tk->method);
	} else {
		cmd_config = GenerateConfigRustUDP(flags);
		ss << "--config" << cmd_config;
		if (shadowsocks_type == _RunParams::ShadowSocksType::Android && flags.runVPN && t2s.name.size() > 0 && t2s.dns.size() > 0)
			ss << "--vpn";
	}

	ss.SetOnCloseFunc([this]() {
		ExitStatus status;
		status.code = ExitStatusCode::ApplicationError;
		status.str = "ShadowsSocksUDP unexpected exited";
		this->onCrash(status);
	});
	ss.ExecInNewThread();
	ss.WaitForStart();
}

String ShadowSocksClient::GenerateConfigRustTCP(SSClientFlags flags) {
	return GenerateConfigRust(flags, TunType::TCP);
}
String ShadowSocksClient::GenerateConfigRustUDP(SSClientFlags flags) {
	return GenerateConfigRust(flags, TunType::UDP);
}

String ShadowSocksClient::GenerateConfigRustName(TunType type) {
	// SetCasheDir
	__DP_LIB_NAMESPACE__::Path __p {this->tempPath};
	__p.Append(toString(tk->id) + "-" + toString(srv->id) +  "-ss-conf-" + (type == TunType::TCP ? "tcp.json" : "udp.json"));
	return __p.Get();
}

String ShadowSocksClient::GenerateConfigRust(SSClientFlags flags, TunType type) {
	bool write_conf = !flags.disableTuns && ( shadowsocks_type == _RunParams::ShadowSocksType::Android || tk->tuns.size() > 0 );
	if (!write_conf)
		return "";

	// If need write conf;
	__DP_LIB_NAMESPACE__::Path __p {GenerateConfigRustName(type)};
	__DP_LIB_NAMESPACE__::Ofstream conf;
	conf.open(__p.Get());
	if (conf.fail())
		throw EXCEPTION("Can't to open temp file " + __p.Get() + " to write");
	conf << "{\n";

	String __shift = "    ";
	auto shift_left = [&__shift] () {
		if (__shift.size() > 4)
			__shift = __shift.substr(0, __shift.size() - 4);
		else
			__shift = "";
	};
	auto shift_right = [&__shift] () {
		__shift = __shift + "    ";
	};
	#define ADD_Z(Str) conf << __shift << "\"" << ecranire(Str) << "\": "
	#define ADD_P(Str, Val) ADD_Z(Str) << " \"" << ecranire(Val) << "\",\n";
	#define ADD_I(Str, Val) ADD_Z(Str) << " " << Val << ",\n";

	if (tk->enable_ipv6)
		ADD_Z("ipv6_first") << "true,\n";

	_V2RayServer * ser = dynamic_cast<_V2RayServer * >(srv);

	ADD_P("server", srv->host);
	ADD_I("server_port", srv->port);
	ADD_P("password", tk->password);
	ADD_P("method", metho_from_go_to_rust(tk->method));
	if (type == TunType::UDP) {
		ADD_I("udp_max_associations", 512);
		ADD_I("udp_timeout", this->udpTimeout);
	}
	if (ser != nullptr && type == TunType::TCP) {
		if (shadowsocks_type == _RunParams::ShadowSocksType::Android)
			ADD_Z("plugin_args") << "[\"-V\"],\n";
		ADD_P("plugin", v2rayPlugin);
		String cmd = "mode=" + ser->mode + ";";
		if (ser->isTLS)
			cmd += "tls;";
		cmd += "host=" + ser->v2host + ";path=" + ser->path;

		ADD_P("plugin_opts", cmd);
	}

	String dn = "";
	if (shadowsocks_type == _RunParams::ShadowSocksType::Android ) {
		if (flags.runVPN && t2s.name.size() > 0 && t2s.dns.size() > 0 && type == TunType::TCP)
			dn = *t2s.dns.begin();
		if (dn.size() > 0)
			ADD_P("dns", "unix://local_dns_path");
	}
	ADD_Z("locals") << "[{\n";
	shift_right();
	shift_right();
	ADD_P("local_address", run_params.localHost);
	ADD_I("local_port", run_params.localPort);
	ADD_Z("mode") << " \"" << ( type == TunType::TCP ? "tcp_only" : "udp_only" ) << "\"\n";
	shift_left();
	conf << __shift << "}";

	if (dn.size() > 0) {
		conf << ", {\n";
		shift_right();
		ADD_P("local_address", run_params.localHost);
		ADD_I("local_port", 35081);
		ADD_P("local_dns_address", "local_dns_path");
		ADD_P("remote_dns_address", dn);
		ADD_I("remove_dns_port", 53);
		ADD_Z("protocol") << " \"" << "dns" << "\"\n";
		conf << __shift << "}";
		shift_left();
	}
	for (const _Tun & tun : tk->tuns) {
		if (tun.type != type)
			continue;
		conf << ", {\n";
		shift_right();
		ADD_P("protocol", "tunnel");
		ADD_P("local_address", tun.localHost);
		ADD_I("local_port", tun.localPort);
		ADD_P("forward_address", tun.remoteHost);
		ADD_I("forward_port", tun.remotePort);
		ADD_Z("mode") << " \"" << ( type == TunType::TCP ? "tcp_only" : "udp_only" ) << "\"\n";
		shift_left();
		conf << __shift << "}";

	}
	shift_left();
	conf << "\n" << __shift << "]\n";
	conf << "}";
	conf.flush();
	conf.close();
	if (conf.fail())
		throw EXCEPTION("Can't to write temp file " + __p.Get() + " to write");
	return __p.Get();
}


String ShadowSocksClient::GenerateCMDRust(SSClientFlags flags) {
	if (_socks == nullptr)
		_socks = new __DP_LIB_NAMESPACE__::Application(shadowSocks);
	__DP_LIB_NAMESPACE__::Application & s = *_socks;

	String configPath = GenerateConfigRustTCP(flags);
	if (configPath.size() == 0) {
		_V2RayServer * ser = dynamic_cast<_V2RayServer * >(srv);
		if (ser != nullptr) {
			DP_PRINT_VAL_0(v2rayPlugin);
			String cmd = "mode=" + ser->mode + ";";
			if (ser->isTLS)
				cmd += "tls;";
			cmd += "host=" + ser->v2host + ";path=" + ser->path;
			s << "--plugin" << v2rayPlugin ;
			s << "--plugin-opts" << cmd;
		}

		if (tk->enable_udp) {
			s << "-U";
			s << "--udp-timeout" << this->udpTimeout;
		}
		if (tk->enable_ipv6)
			s << "-6";
		s << "--password" << tk->password;
		s << "--server-addr" << (srv->host + ":" + __DP_LIB_NAMESPACE__::toString(srv->port));
		s << "--local-addr" << (run_params.localHost + ":" + __DP_LIB_NAMESPACE__::toString(run_params.localPort));
		s << "--encrypt-method" << metho_from_go_to_rust(tk->method);
	} else {
		s << "--config" << configPath;
		if (shadowsocks_type == _RunParams::ShadowSocksType::Android && flags.runVPN && t2s.name.size() > 0 && t2s.dns.size() > 0)
			s << "--vpn";
		StartRustUdp(flags);
	}
	return configPath;
}

bool isMethodLocal(const String & method) {
	return method == "Socks5" || method == "Http" || method == "Direct";
}

#ifdef DP_WIN
bool Is64bitWindows(void) noexcept;
#endif

bool ShadowSocksClient::waitForStart() {
	int count = 15;
	#ifdef DP_WIN
		if (!Is64bitWindows())
			// Wait 6 second.
			// On Win32 start process may be very long time
			count = 120;
	#endif
	while (! socks5IsWork(run_params.localHost, run_params.localPort)) {
		count--;
		if (count <=0) {
			_locker.unlock();
			this->Stop();
			return false;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
		if (!isMethodLocal(tk->method) && _socks->isFinished()) {
			ExitStatus status;
			status.code = ExitStatusCode::ApplicationError;
			status.str = "ShadowSocks unexpected closed with status: " + toString(_socks->ResultCode());
			_locker.unlock();
			this->onCrash(status);
			return false;
		}
	}
	return true;
}

bool ShadowSocksClient::startTun2Socks(SSClientFlags flags, OnShadowSocksRunned _onSuccess) {
	if (!(flags.runVPN && t2s.name.size() > 0))
		return true;

	DP_PRINT_TEXT("Try start VPN mode");
	tun2socks = new Tun2Socks();
	tun2socks->config = t2s;
	#ifdef DP_ANDROID
		if (!isMethodLocal(tk->method))
			tun2socks->config.isDNS2Socks = false;
	#endif
	tun2socks->proxyPort = run_params.localPort;
	tun2socks->proxyServer = run_params.localHost;
	tun2socks->enable_udp = tk->enable_udp;
	if (!run_params.multimode)
		tun2socks->config.ignoreIP.push_back(srv->host);
	tun2socks->SetUDPTimeout(convertTime(this->udpTimeout));
	try{
		tun2socks->Start([this, _onSuccess] () {
            if (isMethodLocal(tk->method) || (_socks != nullptr && !_socks->isFinished())) {
				this->status = ShadowSocksClientStatus::Started;
				_onSuccess(tk->name);
			}
		}, [this](const String & , const ExitStatus & status) {
			_locker.try_lock();
			_locker.unlock();
			this->onCrash(status);
		});
	} catch (__DP_LIB_NAMESPACE__::Exception & e) {
		ExitStatus status;
		status.code = ExitStatusCode::ApplicationError;
		status.str = e.toString();
		_locker.unlock();
		this->onCrash(status);
		return false;
	}

	tun2socks->WaitForStart();
	is_run_vpn = true;
	return true;
}

bool ShadowSocksClient::startHttpProxy(SSClientFlags flags, OnShadowSocksRunned _onSuccess) {
	if (!(run_params.httpProxy > 0 && !(flags.runVPN && t2s.name.size() > 0)))
		return true;
	DP_PRINT_TEXT("Try start HTTP-Proxy");

	http_connector_node = new __DP_LIB_NAMESPACE__::ProxyNode(new __DP_LIB_NAMESPACE__::ProxyConnectorDirect());
	http_connector_node = new __DP_LIB_NAMESPACE__::ProxyNode(new __DP_LIB_NAMESPACE__::ProxyConnectorSocks(run_params.localHost,  run_params.localPort), http_connector_node);
	auto makeConnect = [this] (const std::string & host, unsigned short port) {
		return http_connector_node->makeConnect(host, port);
	};
	String host_pr = run_params.localHost;
	int port_pr = run_params.httpProxy;
	_http_server_thread = new __DP_LIB_NAMESPACE__::Thread([this, host_pr, port_pr, makeConnect]() {
		this->_http_server.ThreadListen(host_pr, port_pr, [makeConnect](__DP_LIB_NAMESPACE__::TCPServerClient cl){
			SocketParserMultiplex parser;
			SocketReader buff(&cl);
			parser.tryParse(&buff, makeConnect);
		});
	});
	_http_server_thread->SetName("HttpProxy=>SocksProxy main loop");
	_http_server_thread->addOnFinish([this]() {
		ExitStatus status;
		status.code = ExitStatusCode::NetworkError;
		status.str = "Main thread of Proxy Converter exited";
		this->onCrash(status);
	});
	_http_server_thread->start();
	status = ShadowSocksClientStatus::Started;
	_onSuccess(tk->name);
	return true;
}

bool ShadowSocksClient::startSocks5() {
	DP_PRINT_TEXT("Try start Socks5-Proxy");

	auto pos = tk->password.find(':');
	String login = "";
	String password = "";
	if (pos != tk->password.npos) {
		login = tk->password.substr(0, pos);
		password = tk->password.substr(pos+1);
	}

	socks_connector_node = new __DP_LIB_NAMESPACE__::ProxyNode(new __DP_LIB_NAMESPACE__::ProxyConnectorDirect());
	if (tk->method == "Socks5")
		socks_connector_node = new __DP_LIB_NAMESPACE__::ProxyNode(new __DP_LIB_NAMESPACE__::ProxyConnectorSocks(srv->host,  __DP_LIB_NAMESPACE__::parse<unsigned short>(srv->port), login, password), socks_connector_node);
	if (tk->method == "Http")
		socks_connector_node = new __DP_LIB_NAMESPACE__::ProxyNode(new __DP_LIB_NAMESPACE__::ProxyConnectorHttpProxy(srv->host,  __DP_LIB_NAMESPACE__::parse<unsigned short>(srv->port), login, password), socks_connector_node);
	auto makeConnect = [this] (const std::string & host, unsigned short port) {
		return socks_connector_node->makeConnect(host, port);
	};
	String host_pr = run_params.localHost;
	int port_pr = run_params.localPort;
	_socks5_server_thread = new __DP_LIB_NAMESPACE__::Thread([this, host_pr, port_pr, makeConnect]() {
		this->_socks5_server.ThreadListen(host_pr, port_pr, [makeConnect](__DP_LIB_NAMESPACE__::TCPServerClient cl){
			cl.setReadTimeout(60);
			SocketParserSocks5 parser;
			SocketReader buff(&cl);
			parser.tryParse(&buff, makeConnect);
		});
	});
	_socks5_server_thread->SetName("Socks5Proxy main loop");
	_socks5_server_thread->addOnFinish([this]() {
		ExitStatus status;
		status.code = ExitStatusCode::NetworkError;
		status.str = "Main thread of Proxy exited";
		this->onCrash(status);
	});
	_socks5_server_thread->start();
	return true;
}

void ShadowSocksClient::Start(SSClientFlags flags, OnShadowSocksRunned _onSuccess) {
	status = ShadowSocksClientStatus::Running;
	PreStartChecking(flags);
	StartMultiMode(flags);

	_locker.lock();
	String configPath = "";
	try {
		if (isMethodLocal(tk->method)) {
			startSocks5();
		} else {
			if (shadowsocks_type == _RunParams::ShadowSocksType::GO)
				GenerateCMDGO(flags);
			if (shadowsocks_type == _RunParams::ShadowSocksType::Rust || shadowsocks_type == _RunParams::ShadowSocksType::Android) {
				configPath = GenerateCMDRust(flags);
			}
		}
	} catch (...) {
		_locker.unlock();
		throw;
	}
#define RUTURN_FUNC { _locker.unlock(); return; }
	if (_socks != nullptr) {
		__DP_LIB_NAMESPACE__::Application & s = *_socks;

		s.SetOnCloseFunc([this]() {
			ExitStatus status;
			status.code = ExitStatusCode::ApplicationError;
			status.str = "ShadowsSocks unexpected exited";
			this->onCrash(status);
		});
		s.SetReadedFunc([](const String & str) {
			DP_LOG_TRACE << "SS: " << str;
		});

		s.ExecInNewThread();
		s.WaitForStart();
	}

	if (!waitForStart())
		return;
	{
		__DP_LIB_NAMESPACE__::Path __p {configPath};
		if (__p.IsFile())
			__DP_LIB_NAMESPACE__::RemoveFile(__p.Get());

		{
			__DP_LIB_NAMESPACE__::Path p {GenerateConfigRustName(TunType::UDP)};
			if (p.IsFile())
				__DP_LIB_NAMESPACE__::RemoveFile(p.Get());
		}
	}

	if (!startTun2Socks(flags, _onSuccess))
		return;

	if (!startHttpProxy(flags, _onSuccess))
		return;
	if (tun2socks == nullptr && _http_server_thread == nullptr && (_socks5_server_thread != nullptr || (_socks != nullptr && !_socks->isFinished()))) {
		status = ShadowSocksClientStatus::Started;
		_onSuccess(tk->name);
	}
	if (run_params.systemProxy && run_params.httpProxy > 0) {
		#ifdef DP_WIN
			DP_PRINT_TEXT("Try start sys proxy");
			_sysproxy = new __DP_LIB_NAMESPACE__::Thread(&ShadowSocksClient::SetSystemProxy, this);
			_sysproxy->SetName("Sys proxy loop for task " + tk->name);
			_sysproxy->start();
		#else
			DP_LOG_ERROR << "Sys proxy not support on Linux";
		#endif
	}
	RUTURN_FUNC;
}

void ShadowSocksClient::onCrash(const ExitStatus & status) {
    if (tk == nullptr)
        return;
	DP_LOG_TRACE << tk->name << " ShadowSocksClient::onCrash";
	_locker.lock();
	if (_is_exit) {
		_locker.unlock();
		return;
	}
	_locker.unlock();

    String name = tk->name;
	// Нужно асинхронно. Эту функцию вызвал один из потоков Application. А он сам не в состоянии join выполнить.
	Stop(status);
}

extern "C" int setNoProxy();
extern "C" int setProxy(wchar_t * _host, unsigned short length);

void ShadowSocksClient::SetSystemProxy() {
	String proxy = run_params.localHost + ":" + __DP_LIB_NAMESPACE__::toString(run_params.httpProxy);
	std::wstring prox(proxy.begin(), proxy.end());
	wchar_t * data = new wchar_t[prox.size() + 1];
	wcsncpy(data, prox.c_str(), prox.size());
	data[prox.size()] = 0;
	DP_LOG_WARNING << "Set global proxy " <<proxy;
	while (!_is_exit) {
		wcsncpy(data, prox.c_str(), prox.size());
		data[prox.size()] = 0;

		int r = setProxy(data, prox.size());
		if (r != 0) {
			auto l = DP_LOG_ERROR;
			l << "Fail to set global proxy: ";
			if (r == 1)
				l << "invalid format";
			if ( r == 2)
				l << "no permission";
			if (r == 3)
				l << "sys call fail";
			if (r == 4)
				l << "no memory";
			if (r == 5)
				l << "invalid option count";
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(sleepMS));
	}
	delete [] data;
}

void ShadowSocksClient::_Stop(const ExitStatus & status) {
	if (tk == nullptr)
		return;
	String name = tk->name;
	_Stop();
	if (_onCrash != nullptr)
		_onCrash(name, status);
}

void ShadowSocksClient::_Stop() {
    if (status == ShadowSocksClientStatus::DoStop || status == ShadowSocksClientStatus::Stoped)
        return;
	DP_LOG_DEBUG << "ShadowSocksClient::_Stop";
	status = ShadowSocksClientStatus::DoStop;
	_is_exit = true;
	if (tun2socks != nullptr) {
		tun2socks->DisableCrashFunc();
		tun2socks->Stop();
		delete tun2socks;
		tun2socks = nullptr;
	}
	if (_socks != nullptr) {
		_socks->SetOnCloseFunc(nullptr);
        _socks->Kill();
		delete _socks;
		_socks = nullptr;
	}
	if (_socksUdp != nullptr) {
		_socksUdp->SetOnCloseFunc(nullptr);
		_socksUdp->Kill();
		delete _socksUdp;
		_socksUdp = nullptr;
	}
	if (run_params.systemProxy && run_params.httpProxy > 0 && _sysproxy != nullptr) {
		_sysproxy->join();
		delete _sysproxy;
		_sysproxy = nullptr;
		setNoProxy();
	}
	if (_multimode_server_thread != nullptr) {
		_multimode_server.exit();
		for (MultiModeServerStruct & data : _multimode_servers) {
			if (data.v2ray != nullptr) {
				data.v2ray->Kill();
				delete data.v2ray;
				delete data.srv;
			}
		}
		_multimode_servers.clear();
		_multimode_server_thread->join();
		delete _multimode_server_thread;
		_multimode_server_thread = nullptr;

	}
	if (_http_server_thread != nullptr) {
		this->_http_server.exit();
		delete http_connector_node;
		_http_server_thread->join();
		delete _http_server_thread;
		_http_server_thread = nullptr;
	}
	if (_socks5_server_thread != nullptr) {
		this->_socks5_server.exit();
		delete socks_connector_node;
		_socks5_server_thread->join();
		delete _socks5_server_thread;
		_socks5_server_thread = nullptr;
	}
	delete this->srv;
	this->srv = nullptr;
	delete this->tk;
	this->tk = nullptr;
	status = ShadowSocksClientStatus::Stoped;
}

void ShadowSocksClient::Stop(bool is_async) {
	if (is_async) {
		__DP_LIB_NAMESPACE__::Thread * th = new __DP_LIB_NAMESPACE__::Thread(&ShadowSocksClient::_Stop, this);
		th->SetName("ShadowSocksClient::Stop impl");
		th->start();
	} else
		_Stop();
}

void ShadowSocksClient::Stop(const ExitStatus & status) {
	__DP_LIB_NAMESPACE__::Thread * th = new __DP_LIB_NAMESPACE__::Thread([this, status]() {
		this->_Stop(status);
	});
	th->SetName("ShadowSocksClient::Stop impl");
	th->start();
}
