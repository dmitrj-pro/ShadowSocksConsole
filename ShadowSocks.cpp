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
#include "WGetDownloader/libproxy/Chain.h"
#include "WGetDownloader/libproxy/Connectors/Connector.h"
#include "WGetDownloader/libproxy/Connectors/ConnectorSocks.h"
#include "WGetDownloader/libproxy/Connectors/ConnectorDirect.h"
#include "WGetDownloader/libproxy/ProtoParsers/SocketParserMultiplex.h"
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

void ShadowSocksClient::Start(SSClientFlags flags, OnShadowSocksRunned _onSuccess) {
	status = ShadowSocksClientStatus::Running;
	{
		Path p {shadowSocks};
		if (!p.IsFile())
			throw EXCEPTION("ShadowSocks not found: " + shadowSocks);
	};

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

	if (run_params.multimode) {
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
					TCPClient target;
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

	_locker.lock();
	_V2RayServer * ser = dynamic_cast<_V2RayServer * >(srv);
	String host = srv->host;
	String port = srv->port;

	_socks = new __DP_LIB_NAMESPACE__::Application(shadowSocks);
	__DP_LIB_NAMESPACE__::Application & s = *_socks;

	if (ser != nullptr) {
		DP_PRINT_VAL_0(v2rayPlugin);
		String cmd = "mode=" + ser->mode + ";";
		if (ser->isTLS)
			cmd += "tls;";
		cmd += "host=" + ser->v2host + ";path=" + ser->path;
		if (shadowsocks_type == _RunParams::ShadowSocksType::GO) {
			s << "-plugin" << ("\"" + v2rayPlugin + "\"");
			s << "-plugin-opts" << ("\"" + cmd + "\"");
		} else {
			s << "--plugin" << ("\"" + v2rayPlugin + "\"");
			s << "--plugin-opts" << ("\"" + cmd + "\"");
		}
	}
	// Включить тунелирование UDP
	if (shadowsocks_type == _RunParams::ShadowSocksType::GO) {
		s << "-u";
		s << "-udptimeout" << convertTime(this->udpTimeout);
		s << "-password" << tk->password;
		s << "-c" << (host + ":" + __DP_LIB_NAMESPACE__::toString(port));
		s << "-socks" << (run_params.localHost + ":" + __DP_LIB_NAMESPACE__::toString(run_params.localPort));
		s << "-cipher" << tk->method;
	} else {
		s << "-U";
		if (tk->enable_ipv6)
			s << "-6";
		s << "--udp-timeout" << this->udpTimeout;
		s << "--password" << tk->password;
		s << "--server-addr" << (host + ":" + __DP_LIB_NAMESPACE__::toString(port));
		s << "--local-addr" << (run_params.localHost + ":" + __DP_LIB_NAMESPACE__::toString(run_params.localPort));
		s << "--encrypt-method" << metho_from_go_to_rust(tk->method);

	}

	{
		__DP_LIB_NAMESPACE__::OStrStream tcpTuns;
		__DP_LIB_NAMESPACE__::OStrStream udpTuns;;

		for (_Tun tun : tk->tuns) {
			if (shadowsocks_type == _RunParams::ShadowSocksType::Rust)
				throw EXCEPTION("Tunnel mode is not supported for shadowsocks-rust");
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
	s.SetOnCloseFunc([this]() {
		ExitStatus status;
		status.code = ExitStatusCode::ApplicationError;
		status.str = "ShadowsSocks unexpected exited";
		this->onCrash(status);
	});

	s.ExecInNewThread();
	s.WaitForStart();
	if (flags.runVPN && t2s.name.size() > 0) {
		DP_PRINT_TEXT("Try start VPN mode");
		tun2socks = new Tun2Socks();
		tun2socks->config = t2s;
		tun2socks->proxyPort = run_params.localPort;
		tun2socks->proxyServer = run_params.localHost;
		if (!run_params.multimode)
			tun2socks->config.ignoreIP.push_back(srv->host);
		tun2socks->SetUDPTimeout(convertTime(this->udpTimeout));
		tun2socks->Start([this, _onSuccess] () {
			if (!_socks->isFinished()) {
				this->status = ShadowSocksClientStatus::Started;
				_onSuccess(tk->name);
			}
		}, [this](const String & , const ExitStatus & status) {
			this->onCrash(status);
		});
		tun2socks->WaitForStart();
		is_run_vpn = true;
	}
	if (run_params.httpProxy > 0 && !(flags.runVPN && t2s.name.size() > 0)) {
		DP_PRINT_TEXT("Try start HTTP-Proxy");

		http_connector_node = new Node(new ConnectorDirect());
		http_connector_node = new Node(new ConnectorSocks(run_params.localHost,  run_params.localPort), http_connector_node);
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
	}
	if (tun2socks == nullptr && _http_server_thread == nullptr && (!_socks->isFinished())) {
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
	_locker.unlock();
}

void ShadowSocksClient::onCrash(const ExitStatus & status) {
    if (tk == nullptr)
        return;
	_locker.lock();
	if (_is_exit) {
		_locker.unlock();
		return;
	}

	if (tun2socks != nullptr)
		tun2socks->DisableCrashFunc();
	if (_socks != nullptr)
		_socks->SetOnCloseFunc(nullptr);
	_locker.unlock();

    String name = tk->name;
    Stop(false);
	if (_onCrash != nullptr)
        _onCrash(name, status);
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

void ShadowSocksClient::_Stop() {
    if (status == ShadowSocksClientStatus::DoStop || status == ShadowSocksClientStatus::Stoped)
        return;
	status = ShadowSocksClientStatus::DoStop;
	_is_exit = true;
	if (tun2socks != nullptr) {
		tun2socks->Stop();
		delete tun2socks;
		tun2socks = nullptr;
	}
	if (_socks != nullptr) {
        _socks->Kill();
		delete _socks;
		_socks = nullptr;
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
		while (http_connector_node->getNext() != nullptr) {
			auto it = http_connector_node->getNext();
			delete http_connector_node->getConnector();
			delete http_connector_node;
			http_connector_node = it;
		}
		_http_server_thread->join();
		delete _http_server_thread;
		_http_server_thread = nullptr;
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
