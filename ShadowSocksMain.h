#pragma once
#include <DPLib.conf.h>
#include <Log/Log.h>
#include <functional>

using __DP_LIB_NAMESPACE__::String;
using __DP_LIB_NAMESPACE__::UInt;
using __DP_LIB_NAMESPACE__::List;
using __DP_LIB_NAMESPACE__::Map;

inline void ReadFuncProc(const String & str) {
	DP_LOG_DEBUG << str << "\n";
}

inline bool isNumber(const String & text) {
	for (unsigned int i = 0; i < text.size(); i++)
		if (!(text[i] >= '0' && text[i] <= '9'))
			return false;
	return true;
}

enum class ExitStatusCode { None, Normal, Warning, NetworkError, ApplicationError, UnknowError };

struct ExitStatus {
	ExitStatusCode code;
	String str;
};

typedef std::function<void(const String & , const ExitStatus &) > OnShadowSocksExit;
typedef std::function<void(const String & ) > OnShadowSocksRunned;
typedef std::function<void(const String & , const ExitStatus &) > OnShadowSocksError;


struct _Server {
	struct ResultCheck{
		String ipAddr;

		double speed;
		String speed_s;

		bool isRun = false;
		String msg = "";
	};

	ResultCheck check_result;

	String host = "";
	String port = "80";
	String name = "";
	String group = "";
	const int id = 0;
	static int glob_id;
	virtual ~_Server(){}
	_Server():id(glob_id) { glob_id++; }
	_Server(int id):id(id) {}
	virtual bool Check() { return true; }

	virtual _Server * Copy(_Server * res, std::function<String(const String&)> replacer) const {
		res->host = replacer(host);
		res->port = replacer(port);
		res->check_result = check_result;
		res->name = name;
		res->group = group;
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

struct _RunParams {
	int localPort = -1;
	String localHost = "";
	int httpProxy = -1;
	bool systemProxy = false;
	String tun2SocksName = "";
	String name = "DEFAULT";
	bool multimode = false;
	bool isNull = true;
	enum class ShadowSocksType {None, GO, Rust, Android};
	ShadowSocksType shadowsocks_type = ShadowSocksType::None;


	virtual _RunParams * Copy(std::function<String(const String&)> replacer) const {
		_RunParams * res = new _RunParams();
		Copy(res, replacer);
		return res;
	}
	virtual _RunParams * Copy(_RunParams * res, std::function<String(const String&)> replacer) const {
		res->name = name;
		res->localHost = replacer(localHost);
		res->localPort = localPort;
		res->httpProxy = httpProxy;
		res->systemProxy = systemProxy;
		res->multimode = multimode;
		res->tun2SocksName = tun2SocksName;
		res->shadowsocks_type = shadowsocks_type;
		return res;
	}
};
inline _RunParams::ShadowSocksType parseSSType(const String & s) {
	if (s == "go")
		return  _RunParams::ShadowSocksType::GO;
	if (s == "rust")
		return _RunParams::ShadowSocksType::Rust;
	if (s == "android")
		return _RunParams::ShadowSocksType::Android;
	if (s == "none")
		return _RunParams::ShadowSocksType::None;
	return _RunParams::ShadowSocksType::GO;
}
inline String SSTtypetoString(_RunParams::ShadowSocksType t){
	if (t == _RunParams::ShadowSocksType::GO)
		return "go";
	if (t == _RunParams::ShadowSocksType::Rust)
		return "rust";
	if (t == _RunParams::ShadowSocksType::Android)
		return "android";
	if (t == _RunParams::ShadowSocksType::None)
		return "none";
	return "none";
}

struct _Task{
	String name = "";
	String group = "";
	List<int> servers_id;
	String password = "";
	String method = "";
	List<_Tun> tuns;
	static int glob_id;
	const int id;

	bool autostart = false;
	bool enable_udp = false;
	bool enable_ipv6 = false;

	String runParamsName = "DEFAULT";

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
		res->group = group;
		res->password = replacer(password);
		res->method = replacer(method);
		res->autostart = autostart;
		res->enable_ipv6 = enable_ipv6;
		res->enable_udp = enable_udp;
		res->runParamsName = runParamsName;
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
	bool enableDefaultRouting = true;
	String postStartCommand = "";
	String preStopCommand = "";
	String name = "";
	bool isDNS2Socks = true;
	bool hideDNS2Socks = true;
	bool isNull = true;
};

class ShadowSocksClient;

enum SSClientFlagsBoolStatus { None, False, True};
enum class ServerCheckingMode { Off, TCPCheck, DeepFast, DeepCheck};
struct SSClientFlags{
	bool runVPN = true;
	String vpnName = "";
	unsigned short port = 1;
	String server_name = "";
	unsigned short http_port = 0;
	SSClientFlagsBoolStatus sysproxy_s = SSClientFlagsBoolStatus::None;
	bool deepCheck = true;
	String listen_host = "";
	bool disableTuns = false;
	ServerCheckingMode checkServerMode = ServerCheckingMode::TCPCheck;
	SSClientFlagsBoolStatus multimode = SSClientFlagsBoolStatus::None;
	_RunParams::ShadowSocksType type = _RunParams::ShadowSocksType::None;
};

// OnCoreStart - Нужно запустить задания при запуске ядра
// BootedCore - Загружен нешифрованный конфиг, а основной не загружен. (идет запуск на уровне ядра)
enum class AutoStartMode { Off, On, OnCoreStart};
inline String AutoStartMode_to_str(AutoStartMode v) {
	switch (v) {
		case AutoStartMode::Off: return "off";
		case AutoStartMode::On: return "on";
		case AutoStartMode::OnCoreStart: return "core";
		default: return "off";
	}
}
inline AutoStartMode str_to_AutoStartMode(const String & val) {
	if (val == "off") return AutoStartMode::Off;
	if (val == "on") return AutoStartMode::On;
	if (val == "core") return AutoStartMode::OnCoreStart;
	return AutoStartMode::Off;
}

enum class AutoCheckingMode { Off, Passiv, Work, Ip, Speed };
inline String AutoCheckingMode_to_str(AutoCheckingMode m) {
	switch (m) {
		case AutoCheckingMode::Off: return "Off";
		case AutoCheckingMode::Ip: return "Ip";
		case AutoCheckingMode::Work: return "Work";
		case AutoCheckingMode::Passiv: return "Passiv";
		case AutoCheckingMode::Speed: return "Speed";
		default: return "Off";
	}
}
inline AutoCheckingMode str_to_AutoCheckingMode(const String & m) {
	if (m == "Off") return AutoCheckingMode::Off;
	if ( m == "Ip" ) return AutoCheckingMode::Ip;
	if ( m == "Work") return AutoCheckingMode::Work;
	if ( m == "Passiv" ) return AutoCheckingMode::Passiv;
	if ( m == "Speed" ) return AutoCheckingMode::Speed;
	return AutoCheckingMode::Off;
}

inline String ServerCheckingMode_to_str(ServerCheckingMode v) {
	switch (v) {
		case ServerCheckingMode::Off: return "off";
		case ServerCheckingMode::TCPCheck: return "tcp";
		case ServerCheckingMode::DeepFast: return "deepfast";
		case ServerCheckingMode::DeepCheck: return "deep";
		default: return "off";
	}
}
inline ServerCheckingMode str_to_ServerCheckingMode(const String & val) {
	if (val == "off") return ServerCheckingMode::Off;
	if (val == "deepfast") return ServerCheckingMode::DeepFast;
	if (val == "tcp") return ServerCheckingMode::TCPCheck;
	if (val == "deep") return ServerCheckingMode::DeepCheck;
	return ServerCheckingMode::TCPCheck;
}


struct ShadowSocksSettings{
	List<_Server*> servers;
	List<_Task * > tasks;
	List<Tun2SocksConfig> tun2socksConf;
	List<_RunParams> runParams;

	AutoStartMode autostart = AutoStartMode::Off;
	UInt countRestartAutostarted = 3;
	UInt web_session_timeout_m = 60;

	String shadowSocksPath = "${INSTALLED}/ss";
	String shadowSocksPathRust = "${INSTALLED}/ss-rust";
	_RunParams::ShadowSocksType shadowSocksType = _RunParams::ShadowSocksType::GO;
	String v2rayPluginPath = "${INSTALLED}/v2ray";
	String tun2socksPath = "${INSTALLED}/tun2socks";
	String dns2socksPath = "${INSTALLED}/dns2socks";
	String wgetPath = "${INSTALLED}/wget";
	String tempPath = "${INSTALLED}";
	bool fixLinuxWgetPath = true;

	ServerCheckingMode checkServerMode = ServerCheckingMode::TCPCheck;
	bool autoDetectTunInterface = false;
	static bool enablePreStartStopScripts;
	bool enableLogging = false;
	bool hideDNS2Socks = true;
	Map<String, String> variables;
	UInt udpTimeout = 600;

	String bootstrapDNS = "8.8.8.8";

	AutoCheckingMode auto_check_mode = AutoCheckingMode::Off;
	unsigned int auto_check_interval_s = 10;
	String auto_check_ip_url = "https://api4.my-ip.io/ip";
	String auto_check_download_url = "https://speed.hetzner.de/100MB.bin";

	String __checker_server_name = "";
	String __checker_task_name = "";



	bool CheckTask(_Task * t);
	bool CheckServer(_Server * s);
	//ToDo
	bool CheckT2S(const Tun2SocksConfig & conf);
	_Task * findTaskByName(const String & name);
	_Task * findTaskById(int id);
	_Server * findServerByName(const String & name);
	_Server * findServerById(int id);
	Tun2SocksConfig findVPNbyName(const String & name);

	_RunParams findRunParamsbyName(const String & name)const ;
	_RunParams findDefaultRunParams()const ;
	bool IsCorrect(_RunParams r);
	void deleteRunParamsByName(const String & name);

	void deleteServerById(int id);
	void deleteTaskById(int id);
	void deleteVPNByName(const String & name);
	String replacePath(const String & path, bool is_dir = false) const;
	String replaceVariables(const String & src) const;
	String getWGetPath() const;

	String GetSource();
	String GetSourceCashe();
	void LoadCashe(const String & text);
	void Load(const String & text);

	ShadowSocksClient * makeServer(int id, const SSClientFlags & flags);
};


String getWritebleDirectory();

String getExecutableDirectory();
