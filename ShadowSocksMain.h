#pragma once
#include <DPLib.conf.h>
#include <Log/Log.h>
#include <functional>

using __DP_LIB_NAMESPACE__::String;
using __DP_LIB_NAMESPACE__::UInt;
using __DP_LIB_NAMESPACE__::List;
using __DP_LIB_NAMESPACE__::Map;

inline void ReadFuncProc(const String & str) {
	__DP_LIB_NAMESPACE__::log << str << "\n";
}

inline bool isNumber(const String & text) {
	for (int i = 0; i < text.size(); i++)
		if (!(text[i] >= '0' && text[i] <= '9'))
			return false;
	return true;
}

struct _Server {
	String host = "";
	String port = "80";
	String name = "";
	const int id = 0;
	static int glob_id;
	virtual ~_Server(){}
	_Server():id(glob_id) { glob_id++; }
	_Server(int id):id(id) {}
	virtual bool Check() { return true; }

	virtual _Server * Copy(_Server * res, std::function<String(const String&)> replacer) const {
		res->host = replacer(host);
		res->port = replacer(port);
		res->name = name;
		return res;
	}

	virtual _Server * Copy(std::function<String(const String&)> replacer) const {
		_Server * res = new _Server(id);
		this->Copy(res, replacer);
		return res;
	}

};

struct _V2RayServer:public _Server {
	bool isTLS = false;
	String mode = "websocket";
	String v2host = "yandex.ru";
	String path = "/";
	_V2RayServer():_Server(){}
	_V2RayServer(int id) : _Server(id) {}
	_V2RayServer(const _Server * srv) : _Server(*srv) {}
	virtual bool Check() override {
		List<String> modes{"websocket", "quic"};
		int i = 0;
		for (String m: modes)
			if (m == mode)
				i++;
		if (i == 0)
			return false;
		return true;
	}
	virtual _Server * Copy(std::function<String(const String&)> replacer) const override {
		_V2RayServer * res = new _V2RayServer(id);
		Copy(res, replacer);
		return res;
	}
	virtual _Server * Copy(_Server * _res, std::function<String(const String&)> replacer) const override {
		_Server::Copy(_res, replacer);
		_V2RayServer * res = dynamic_cast<_V2RayServer*>(_res);
		res->isTLS = isTLS;
		res->mode = replacer(mode);
		res->v2host = replacer(v2host);
		res->path = replacer(path);
		return res;
	}
};

enum class TunType{TCP, UDP};

struct _Tun {
	TunType type;
	String localHost = "127.0.0.1";
	UInt localPort = 0;
	String remoteHost = "";
	UInt remotePort = 0;
};

struct _Task{
	String name = "";
	List<int> servers_id;
	String password = "";
	String method = "";
	List<_Tun> tuns;
	static int glob_id;
	const int id;
	int localPort = -1;
	String localHost = "";
	bool autostart = false;
	int httpProxy = -1;
	bool systemProxy = false;

	String tun2SocksName = "";

	_Task():id(glob_id) {glob_id++;}
	_Task(int id):id(id) {}
	virtual ~_Task(){};

	bool Check() const {
		List<String> modes{"AEAD_AES_128_GCM", "AEAD_AES_256_GCM", "AEAD_CHACHA20_POLY1305"};
		int i = 0;
		for (String m: modes)
			if (m == method)
				i++;
		if (i == 0)
			return false;
		return true;
	}
	virtual _Task * Copy(std::function<String(const String&)> replacer) const {
		_Task * res = new _Task(id);
		Copy(res, replacer);
		return res;
	}
	virtual _Task * Copy(_Task * res, std::function<String(const String&)> replacer) const {
		res->name = name;
		res->password = replacer(password);
		res->method = replacer(method);
		res->localHost = replacer(localHost);
		res->localPort = localPort;
		res->autostart = autostart;
		res->httpProxy = httpProxy;
		res->systemProxy = systemProxy;
		res->tun2SocksName = tun2SocksName;
		for (int server: servers_id)
			res->servers_id.push_back(server);
		for (_Tun tun : tuns) {
			_Tun tt = tun;
			tt.remoteHost = replacer(tt.remoteHost);
			res->tuns.push_back(tt);
		}
		return res;
	}
};

struct Tun2SocksConfig{
	List<String> dns;
	String tunName = "";
	List<String> ignoreIP;
	String defaultRoute = "";
	bool removeDefaultRoute = true;
	String name = "";
	bool isDNS2Socks = true;
	bool hideDNS2Socks = true;
	bool isNull = true;
};

class ShadowSocksClient;

struct MakeServerFlags{
	String server_name = "";
	String vpn_name = "";
};

struct ShadowSocksSettings{
	List<_Server*> servers;
	List<_Task * > tasks;
	int defaultPort = 1080;
	int defaultHttpPort = 8080;
	String defaultHost = "127.0.0.1";
	bool autostart = false;
	String shadowSocksPath = "${INSTALLED}/ss";
	String v2rayPluginPath = "${INSTALLED}/v2ray";
	String tun2socksPath = "${INSTALLED}/tun2socks";
	String dns2socksPath = "${INSTALLED}/dns2socks";
	String polipoPath = "${INSTALLED}/polipo";
	String sysproxyPath = "${INSTALLED}/sysproxy";
	String wgetPath = "${INSTALLED}/wget";
	String tempPath = "${INSTALLED}";
	bool enableLogging = false;
	bool hideDNS2Socks = true;
	Map<String, String> variables;
	UInt udpTimeout = 600;
	List<Tun2SocksConfig> tun2socksConf;
	bool IGNORECHECKSERVER = false;
	String bootstrapDNS = "8.8.8.8";


	bool CheckTask(_Task * t);
	bool CheckServer(_Server * s);
	//ToDo
	bool CheckT2S(const Tun2SocksConfig & conf);
	_Task * findTaskByName(const String & name);
	_Task * findTaskById(int id);
	_Server * findServerByName(const String & name);
	_Server * findServerById(int id);
	Tun2SocksConfig findVPNbyName(const String & name);
	void deleteServerById(int id);
	void deleteTaskById(int id);
	void deleteVPNByName(const String & name);
	String replacePath(const String & path, bool is_dir = false);
	String replaceVariables(const String & src) const;
	inline String getWGetPath() { return this->replacePath(this->wgetPath); }

	String GetSource();
	void Load(const String & text);

	ShadowSocksClient * makeServer(int id, const MakeServerFlags & flags);
};

