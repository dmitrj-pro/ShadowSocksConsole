#pragma once
#include <DPLib.conf.h>
//#include <Parser/SmartParser.h>
#include "ShadowSocksMain.h"
#include "ShadowSocksController.h"
#include <Converter/Converter.h>
#include <_Driver/Path.h>
#include "TCPClient.h"
#include "libSimpleDNS/dns.h"
#include <iomanip>
#include <_Driver/ServiceMain.h>
#include "VERSION.h"
#include <_Driver/Files.h>
#include "WGetDownloader/Downloader.h"

namespace __DP_LIB_NAMESPACE__ {
	bool startWithN(const String & str, const String & in);
	bool startWith(const String & str, const String & in);

	bool endWith(const String & str, const String & end);
	bool endWithN(const String & str, const String & end);
}

using __DP_LIB_NAMESPACE__::Map;
using __DP_LIB_NAMESPACE__::String;
using __DP_LIB_NAMESPACE__::Vector;
using __DP_LIB_NAMESPACE__::parse;
using __DP_LIB_NAMESPACE__::toString;
using __DP_LIB_NAMESPACE__::Path;

using __DP_LIB_NAMESPACE__::startWithN;

template <typename OutStream, typename InStream>
class ParamsReaderTemplate {
	private:
		Vector<String> params;
		int pos = 0;
		OutStream & cout;
		InStream & cin;
	public:
		ParamsReaderTemplate(OutStream & o, InStream & i) : cout (o), cin (i) {}
		inline void AddParams(const String & p) { params.push_back(p); }
		inline String read(const String & name) {
			if (params.size() <= pos) {
				cout << name << ": ";
				String res;
				getline(cin, res);
				return res;
			} else {
				return params[pos++];
			}
		}
		inline String read(const String & name, const String & oldVal) {
			if (params.size() <= pos) {
				cout << name << " (" << oldVal << ")" ": ";
				String res;
				getline(cin, res);
				return res;
			} else {
				return params[pos++];
			}
		}
		inline UInt readUInt(const String & name) {
			String tmp = read(name);
			return parse<UInt>(tmp);
		}
		inline bool readbool(const String & name) {
			String tmp = read(name);
			return parse<bool>(tmp);
		}
		inline UInt size() const { return params.size(); }
		inline bool isEmpty() const { return pos >= params.size(); }

};

#define HELP_COND cmd == "-h" || cmd == "--help" || cmd == "--h" || cmd == "-help" || cmd == "help"

template <typename OutStream, typename InStream>
class ConsoleLooper{
	private:
		class StructLogout : public __DP_LIB_NAMESPACE__::BaseException {
			public:
				StructLogout() : __DP_LIB_NAMESPACE__::BaseException() {}
		};
		using ParamsReader = ParamsReaderTemplate<OutStream, InStream>;
		typedef void (ConsoleLooper::*ConsoleFunc) (ParamsReader &arg);
		Map<String, ConsoleFunc> funcs;
		OutStream & cout;
		InStream & cin;
		_ShadowSocksController & ctrl;
		bool is_exit = false;

		void AddServer(ParamsReader &arg);
		void AddTask(ParamsReader &arg);
		void Add(ParamsReader & arg);

		void CheckPort(ParamsReader & arg);
		void Check(ParamsReader & arg);

		void ListTun(ParamsReader & arg);
		void ListTasks(ParamsReader & arg);
		void ListServers(ParamsReader & arg);
		void ListVPNMode(ParamsReader & arg);
		void ListVariables(ParamsReader & arg);
		void ListRunning(ParamsReader & arg);
		void List(ParamsReader & arg);

		void EditServer(ParamsReader & arg);
		void EditTask(ParamsReader & arg);
		void EditSettings(ParamsReader & arg);
		void EditVPN(ParamsReader & arg);
		void Edit(ParamsReader & arg);

		void GetSource(ParamsReader &);

		void FindFreePorts(ParamsReader &);
		void Find(ParamsReader &);

		void Connect(ParamsReader &);
		void CheckSpeedTest(ParamsReader &, bool enable_speed);
		void Resolve(ParamsReader &);


		void ServerDisconnect(ParamsReader &);

		void Enable(ParamsReader &);
		void Disable(ParamsReader &);

		void StartTask(ParamsReader & arg);
		void StopTask(ParamsReader & arg);

		void TunAdd(ParamsReader & arg);
		void Tun(ParamsReader & arg);

		void VPNAdd(ParamsReader & arg);
		void VPN(ParamsReader & arg);
		void VPNUpdate(ParamsReader & arg);

		void DeleteTask(ParamsReader & arg);
		void DeleteServer(ParamsReader & arg);
		void DeleteTun(ParamsReader & arg);
		void Delete(ParamsReader & arg);

		void SetConfig(ParamsReader & arg);
		void SetVariable(ParamsReader & arg);
		void Set(ParamsReader & arg);
		void S(ParamsReader & arg);

		void ExportConfig(ParamsReader & arg);
		void Export(ParamsReader & arg);

		void SaveConfig(ParamsReader & arg);
		void Save(ParamsReader & arg);

		inline void MakeExit(ParamsReader & arg) { is_exit = true; }

		void Help(ParamsReader & arg);

		void VERSION(ParamsReader & arg);
	public:
		ConsoleLooper(OutStream & o, InStream & i) : cout (o), cin(i), ctrl(ShadowSocksController::Get()) {}
		void Load();
		void Loop();
		void ManualCMD(const String & cmd);
		inline void MakeExit() { is_exit = true; ctrl.MakeExit(); }
		bool SetPassword(const String & password);

};

template <typename OutStream, typename InStream>
ConsoleLooper<OutStream, InStream> * makeLooper(OutStream & out, InStream & in) {
	auto res = new ConsoleLooper<OutStream, InStream>(out, in);
	res->Load();
	return res;
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::Help(ParamsReader & arg) {
	cout << "Supported comands:\n";
	cout << "\tadd - Add element (Task or Server)\n";
	cout << "\tcheck - Check condition\n";
	cout << "\tlist - List of elements\n";
	cout << "\tquit (or exit) - Close comand line interface\n";
	cout << "\tEXIT - Close application (If user remote client)\n";
	cout << "\tedit - Edit paramtrs\n";
	cout << "\tenable - Enable autostart task\n";
	cout << "\tdisable - Disable autostart task\n";
	cout << "\tstart - Start task\n";
	cout << "\tstop - Stop task\n";
	cout << "\ttun - Work with tun for task\n";
	cout << "\tfind - Find\n";
	cout << "\tvpn - Modify VPN mode\n";
	cout << "\tdelete - Delete element\n";
	cout << "\tset - Set env\n";
	cout << "\tsave - Save\n";
	cout << "\ts - edit parametr from config\n";
	cout << "\texport - export to other format\n";
	cout << "\tconnect - check connect to server\n";
	cout << "\tresolve - resolve DNS name server\n";
	cout << "\tversion - show version of client\n";

}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::Load() {
	funcs["add"] = &ConsoleLooper::Add;
	funcs["check"] = &ConsoleLooper::Check;
	funcs["list"] = &ConsoleLooper::List;
	funcs["help"] = &ConsoleLooper::Help;
	funcs["quit"] = &ConsoleLooper::MakeExit;
	funcs["exit"] = &ConsoleLooper::MakeExit;
	funcs["edit"] = &ConsoleLooper::Edit;
	funcs["SOURCE"] = &ConsoleLooper::GetSource;
	funcs["find"] = &ConsoleLooper::Find;
	funcs["enable"] = &ConsoleLooper::Enable;
	funcs["disable"] = &ConsoleLooper::Disable;
	funcs["stop"] = &ConsoleLooper::StopTask;
	funcs["start"] = &ConsoleLooper::StartTask;
	funcs["tun"] = &ConsoleLooper::Tun;
	funcs["vpn"] = &ConsoleLooper::VPN;
	funcs["delete"] = &ConsoleLooper::Delete;
	funcs["set"] = &ConsoleLooper::Set;
	funcs["save"] = &ConsoleLooper::Save;
	funcs["export"] = &ConsoleLooper::Export;
	funcs["connect"] = &ConsoleLooper::Connect;
	funcs["resolve"] = &ConsoleLooper::Resolve;
	funcs["s"] = &ConsoleLooper::S;
	funcs["version"] = &ConsoleLooper::VERSION;
	funcs["disconnect"] = &ConsoleLooper::ServerDisconnect;
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::VERSION(ParamsReader & arg) {
	cout << "ShadowSocks Console v" << SS_HEAD_VERSION << "-" << SS_VERSION << " (" << SS_VERSION_HASHE << ")\n";
	cout << "Base on DPLib V" << __DP_LIB_NAMESPACE__::VERSION() << "\n";
}



template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::ServerDisconnect(ParamsReader & arg) {
	throw StructLogout();
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::S(ParamsReader & arg) {
	String cmd = arg.read("Name");
	if (HELP_COND) {
		cout << "s ${Type}[${Name}]>${Parametr} = ${Value}\nWhere:\n\tType - vpn, task, server, settings\n\tName-Name of VPN/Task/Server If settings thes any word\n";
		cout << "\tValue - New Value\n\n";
		cout << "\tParametr - vpn:\n";
		cout << "\t\troute:String\n\t\tname:String\n\t\tremove_route:Bool\n\t\tenable_dns2socks:Bool\n\t\tdns:Array<String>\n\t\tignore:Array<String>\n\n";
		cout << "\tParametr - server:\n";
		cout << "\t\tname:String\n\t\thost:String\n\t\tport:Int (May be string if use variable)\n\t\ttls:Bool\n\t\tmode:String\n\t\tv2host:String\n\t\tv2path:String\n\n";
		cout << "\tParametr - task\n";
		cout << "\t\tname:String\n\t\tpassword:String\n\t\tmethod:String(aes128, aes256, chacha20_poly1305)\n\t\tservers:Array<String>\n\t\tvpn:String(default=none)\n\t\thttp_port:int\n\t\tsys_proxy:Bool\n\t\tsocks_port:int\n\t\tlocahost:String\n\n";
		cout << "\tParametr - settings\n";
		cout << "\t\tport:UInt\n\t\tss_path:String\n\t\tv2ray_path:String\n\t\tautostart:Bool\n\t\ttun2socks_path:String\n\t\tdns2socks_path:String\n\t\tsysproxy_path:String\n\t\tpolipo_path:String\n\t\ttmp_path:String\n\t\tbootstrap_dns:String\n\t\tudp_timeout:UInt\n\t\tignore_check:Bool\n\t\thide_dns2socks:Bool\n\t\tenable_logging:Bool\n";

		return;
	}
	LiteralReader * _reader = nullptr;
	String type = "";
	String name = "";
	String parametr = "";
	String value = "";
	unsigned short pos = 0;
	__DP_LIB_NAMESPACE__::List<char> delimers{'[', ']', '=', '>', '<', '+', ' ', '\n', '\r' };
	while (true) {
		if (_reader != nullptr) {
			delete _reader;
			_reader = nullptr;
		}
		if (cmd.size() != 0) {
			_reader = new LiteralReader(cmd, delimers);
			cmd = "";
		} else
			_reader = new LiteralReader(arg.read("next"), delimers);
		while (!_reader->isEnd()) {
			auto readed = _reader->read();
			if (readed.size() == 0)
				continue;
			String val = *(readed.begin());
			if (pos == 0)
				type = val;
			if (pos == 1)
				name = val;
			if (pos == 2)
				parametr = val;
			if (pos == 3)
				value = val;
			pos++;
		}
		if (pos >= 4) {
			if (type == "vpn") {
				Tun2SocksConfig conf = ctrl.getConfig().findVPNbyName(name);
				if (conf.isNull) {
					cout << "VPN is not found\n";
					return;
				}
				cout << "Edit vpn '" << name << "': ";
				#define MiCro_s(TXT, variable) if (parametr == TXT) { conf.variable = value; cout << #variable << " = " << value <<"\n"; }
				#define MiCro_t(TXT, variable, type) if (parametr == TXT) { conf.variable = parse<type>(value); cout << #variable << " = " << conf.variable <<"\n"; }
				MiCro_s("route", defaultRoute)
				MiCro_s("name", tunName)
				MiCro_t("remove_route", removeDefaultRoute, bool)
				MiCro_t("enable_dns2socks", isDNS2Socks, bool)
				delimers.push_back(',');
				if (parametr == "dns") {
					conf.dns.clear();

					cout << "DNS = ";

					LiteralReader reade(value, delimers);
					while (!reade.isEnd()) {
						auto val = reade.read();
						if (val.size() != 1)
							continue;
						String dns = *(val.begin());
						conf.dns.push_back(dns);
						cout << dns << ", ";
					}
					cout << "\n";
				}
				if (parametr == "ignore") {
					conf.ignoreIP.clear();
					LiteralReader reade(value, delimers);
					cout << "ignore = ";
					while (!reade.isEnd()) {
						auto val = reade.read();
						if (val.size() != 1)
							continue;
						String dns = *(val.begin());
						conf.ignoreIP.push_back(dns);
						cout << dns << ", ";
					}
					cout << "\n";
				}

				auto res = ctrl.getConfig().findVPNbyName(name);
				ctrl.getConfig().deleteVPNByName(name);
				if (!ctrl.getConfig().CheckT2S(conf)) {
					ctrl.getConfig().tun2socksConf.push_back(res);
					cout << "Fail to add VPN mode\n";
					return;
				}
				ctrl.getConfig().tun2socksConf.push_back(conf);
			}
			if (type == "task") {
				_Task * _sr = ctrl.getConfig().findTaskByName(name);
				if (_sr == nullptr) {
					cout << "Task is not found";
					return;
				}

				cout << "Edit task '" << name << "': ";
				#define MiCro2_s(TXT, variable) if (parametr == TXT) { cf->variable = value; cout << #variable << " = " << value <<"\n"; }
				#define MiCro2_t(TXT, variable, type) if (parametr == TXT)  { cf->variable = parse<type>(value); cout << #variable << " = " << cf->variable <<"\n"; }

				_Task * cf = _sr->Copy([this](const String & txt) { return txt; });
				MiCro2_s("name", name);
				MiCro2_s("password", password);
				if (parametr == "method") {
					cf->method = value;
					String mt = ctrl.getConfig().replaceVariables(cf->method);
					if (mt == "chacha20_poly1305") cf->method = "AEAD_CHACHA20_POLY1305";
					if (mt == "aes128") cf->method = "AEAD_AES_128_GCM";
					if (mt == "aes256") cf->method = "AEAD_AES_256_GCM";
					cout << "method = " << cf->method << "\n";
				}
				delimers.push_back(',');
				if (parametr == "servers") {
					cf->servers_id.clear();

					LiteralReader reade(value, delimers);
					cout << "servers = ";

					while (!reade.isEnd()) {
						auto val = reade.read();
						if (val.size() != 1)
							continue;
						String server_name = *(val.begin());
						_Server * tt = ctrl.getConfig().findServerByName(server_name);
						if (tt == nullptr) {
							cout << "Unknow server.";
							continue;
						}
						cf->servers_id.push_back(tt->id);
						cout << tt->id << ", ";
					}
					cout << "\n";
				}
				if (parametr == "vpn") {
					if (value != "none")
						cf->tun2SocksName = value;
					else
						cf->tun2SocksName = "";
					cout << "tun2SocksName = " << cf->tun2SocksName << "\n";
				}
				if (cf->tun2SocksName.size() == 0) {
					MiCro2_t("http_port", httpProxy, int);
					if (cf->httpProxy > 0) {
						MiCro2_t("sys_proxy", systemProxy, bool);
					} else
						cf->systemProxy = false;
				} else {
					cf->httpProxy = -1;
					cf->systemProxy = false;
				}
				MiCro2_t("socks_port", localPort, int);
				MiCro2_s("localhost", localHost);
				ctrl.getConfig().deleteTaskById(cf->id);
				if (!ctrl.getConfig().CheckTask(cf)) {
					ctrl.getConfig().tasks.push_back(_sr);
					cout << "Bad task";
					delete cf;
					return;
				}
				ctrl.getConfig().tasks.push_back(cf);
				delete _sr;
			}
			if (type == "server") {
				_Server * _sr = ctrl.getConfig().findServerByName(name);
				if (_sr == nullptr) {
					cout << "Server is not found";
					return;
				}
				_Server * cf = _sr->Copy([] (const String& val) { return val; });
				cout << "Edit server '" << name << "': ";

				#define SaveSRV2()\
					{	\
						ctrl.getConfig().deleteServerById(_sr->id); \
						if (ctrl.getConfig().CheckServer(cf)) { \
							delete _sr; \
							_sr = nullptr; \
							ctrl.getConfig().servers.push_back(cf); \
						} else {\
							ctrl.getConfig().servers.push_back(_sr); \
							delete cf; \
							cout << "Server is not correct. Try later\n"; \
						} \
					}


				MiCro2_s("name", name);
				MiCro2_s("host", host);
				MiCro2_s("port", port);
				if (cf->port.size() > 1) {
					if (startWithN(cf->port, "${") && __DP_LIB_NAMESPACE__::endWithN(cf->port, "}")) {
					} else {
						if (!isNumber(cf->port)) {
							cout << "Port is invalid";
							return;
						}
					}
				}
				if (dynamic_cast<_V2RayServer * >(cf) != nullptr) {
					_V2RayServer * t = dynamic_cast<_V2RayServer *>(cf);
					#define MiCro3_s(TXT, variable) if (parametr == TXT) t->variable = value;
					#define MiCro3_t(TXT, variable, type) if (parametr == TXT) t->variable = parse<type>(value);
					MiCro3_t("tls", isTLS, bool);
					MiCro3_s("mode", mode);
					MiCro3_s("v2host", v2host);
					MiCro3_s("v2path", path);

				}
				SaveSRV2();
			}
			if (type == "settings") {
				cout << "Edit settings ";
				auto & conf = ctrl.getConfig();
				MiCro_t("port", defaultPort, UInt);
				MiCro_s("ss_path", shadowSocksPath);
				MiCro_s("v2ray_path", v2rayPluginPath);
				MiCro_t("autostart", autostart, bool);
				MiCro_s("tun2socks_path", tun2socksPath);
				MiCro_s("dns2socks_path", dns2socksPath);
				MiCro_s("sysproxy_path", sysproxyPath);
				MiCro_s("polipo_path", polipoPath);
				MiCro_s("tmp_path", tempPath);
				MiCro_s("bootstrap_dns", bootstrapDNS);
				MiCro_t("udp_timeout", udpTimeout, UInt);
				MiCro_t("ignore_check", IGNORECHECKSERVER, bool);
				MiCro_t("hide_dns2socks", hideDNS2Socks, bool);
				MiCro_t("enable_logging", enableLogging, bool);
			}
			break;
		}
	}
	#undef MiCro_s
	#undef MiCro_t
	#undef MiCro2_s
	#undef MiCro2_t
	#undef MiCro3_s
	#undef MiCro3_t
	ctrl.SaveConfig();
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::EditServer(ParamsReader & arg) {
	String cmd = arg.read("Name");
	if (HELP_COND) {
		cout << "edit server ${Name} ${NewName} ${NewHost} ${NewPort:int} ${NewPlugin} ${Plugin opts:array}\n";
		cout << "Supported plugin:\n\tnone\n\tv2ray\n";
		return;
	}
	_Server * _sr = ctrl.getConfig().findServerByName(cmd);
	if (_sr == nullptr) {
		cout << "Server is not found";
		return;
	}
	_Server * sr = _sr->Copy([] (const String& val) { return val; });

	#define SaveSRV2()\
		{	\
			ctrl.getConfig().deleteServerById(_sr->id); \
			if (ctrl.getConfig().CheckServer(sr)) { \
				delete _sr; \
				_sr = nullptr; \
				ctrl.getConfig().servers.push_back(sr); \
				ctrl.SaveConfig(); \
			} else {\
				ctrl.getConfig().servers.push_back(_sr); \
				delete sr; \
				cout << "Server is not correct. Try later\n"; \
			} \
		}
	String tx = arg.read("Name", sr->name);
	if (tx.size() > 1) sr->name = tx;
	tx = arg.read("Host", sr->host);
	if (tx.size() > 1) sr->host = tx;
	tx = arg.read("Port", toString(sr->port));
	if (tx.size() > 1) {
		if (startWithN(tx, "${") && __DP_LIB_NAMESPACE__::endWithN(tx, "}")) {
			sr->port = tx;
		} else {
			if (!isNumber(tx)) {
				cout << "Port is invalid";
				return;
			}
			sr->port = tx;
		}
	}
	if (arg.size() == 3) {
		SaveSRV2();
		return;
	}
	tx = arg.read("Plugin");
	if (tx == "none") {
		SaveSRV2();
		return;
	}
	if (tx == "v2ray") {
		_V2RayServer * t;
		if (dynamic_cast<_V2RayServer * >(sr) != nullptr)
			t = dynamic_cast<_V2RayServer *>(sr);
		else
			sr = t = new _V2RayServer(sr);
		tx = arg.read("isTLS", toString(t->isTLS));
		if (tx.size() > 0) t->isTLS = parse<bool>(tx);
		tx = arg.read("Mode", t->mode);
		if (tx.size() > 1) t->mode = tx;
		tx = arg.read("V2Ray Host", t->v2host);
		if (tx.size() > 1) t->v2host = tx;
		tx = arg.read("V2Ray Path", t->path);
		if (tx.size() > 1) t->path = tx;
		SaveSRV2(); //ToDo
		return;
	}
	cout << "Unknow plugin. Ignore new server\n";
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::EditTask(ParamsReader & arg) {
	String cmd = arg.read("Name");
	if (HELP_COND) {
		cout << "edit task ${Name} ${NewName} ${Password} ${Method} ${Servers:array<int>}\n";
		return;
	}
	_Task * _sr = ctrl.getConfig().findTaskByName(cmd);
	if (_sr == nullptr) {
		cout << "Task is not found";
		return;
	}
	_Task * tk = _sr->Copy([this](const String & txt) { return txt; });
	String tx = arg.read("New Name", tk->name);
	if (tx.size() > 1) tk->name = tx;
	tx = arg.read("Password");
	if (tx.size() > 1) tk->password = tx;
	tx = arg.read("Method", tk->method);
	if (tx.size() > 1) {
		tk->method = tx;
		String mt = ctrl.getConfig().replaceVariables(tk->method);
		if (mt == "chacha20_poly1305") tk->method = "AEAD_CHACHA20_POLY1305";
		if (mt == "aes128") tk->method = "AEAD_AES_128_GCM";
		if (mt == "aes256") tk->method = "AEAD_AES_256_GCM";
	}

	int del = false;
	while (true) {
		tx = arg.read("Server name");
		if (tx.size() > 0) {
			if (!del) {
				tk->servers_id.clear();
				del = true;
			}
			_Server * tt = ctrl.getConfig().findServerByName(tx);
			if (tt == nullptr) {
				cout << "Unknow server.";
				continue;
			}
			tk->servers_id.push_back(tt->id);
		} else
			break;
	}
	tx = arg.read("VPN Mode Name");
	if (tx.size() > 0 ) {
		if (tx != "none")
			tk->tun2SocksName = tx;
		else
			tk->tun2SocksName = "";
	}
	if (tk->tun2SocksName.size() == 0) {
		tx = arg.read("Http Proxy Port", toString(tk->httpProxy));
		if (tx.size() > 0) tk->httpProxy = parse<int>(tx);
		if (tk->httpProxy > 0) {
			tx = arg.read("System proxy? (0/1)", toString(tk->systemProxy));
			if (tx.size() > 0) tk->systemProxy = parse<int>(tx);
		} else {
			tk->systemProxy = false;
		}
	} else {
		tk->httpProxy = -1;
		tk->systemProxy = false;
	}
	tx = arg.read("Socks Port (-1 - use default)", toString(tk->localPort));
	if (tx.size() > 1) tk->localPort = parse<int>(tx);
	tx = arg.read("Socks local host (none or '' - use default)", tk->localHost);
	if (tx.size() > 1) {
		if (tx == "none")
			tk->localHost = "";
		else
			tk->localHost = tx;
	}

	ctrl.getConfig().deleteTaskById(tk->id);
	if (!ctrl.getConfig().CheckTask(tk)) {
		ctrl.getConfig().tasks.push_back(_sr);
		cout << "Bad task";
		delete tk;
		return;
	}
	ctrl.getConfig().tasks.push_back(tk);
	delete _sr;
	ctrl.SaveConfig();
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::GetSource(ParamsReader &) {
	cout << ctrl.GetSourceConfig() << "\n";
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::EditSettings(ParamsReader & arg) {
	auto & conf = ctrl.getConfig();
	String cmd = arg.read("Default Host", conf.defaultHost);
	if (HELP_COND) {
		cout << "edit settings";//
		return;
	}
	if (cmd.size() > 1) conf.defaultHost = cmd;
	cmd = arg.read("Default Port", toString(conf.defaultPort));
	if (cmd.size() > 1) conf.defaultPort = parse<UInt>(cmd);
	cmd = arg.read("ShadowSocks Path", conf.shadowSocksPath);
	if (cmd.size() > 1 ) conf.shadowSocksPath = cmd;
	cmd = arg.read("V2Ray Path", conf.v2rayPluginPath);
	if (cmd.size() > 1) conf.v2rayPluginPath = cmd;
	cmd = arg.read("AutoStart", toString(conf.autostart));
	if (cmd.size() > 0) conf.autostart = parse<bool>(cmd);
	cmd = arg.read("Tun2Socks", toString(conf.tun2socksPath));
	if (cmd.size() > 0) conf.tun2socksPath = cmd;
	cmd = arg.read("Dns2Socks", toString(conf.dns2socksPath));
	if (cmd.size() > 0) conf.dns2socksPath = cmd;
	cmd = arg.read("HttpProxyPort", toString(conf.defaultHttpPort));
	if (cmd.size() > 0) conf.defaultHttpPort = parse<int>(cmd);
	cmd = arg.read("SysProxyPath", conf.sysproxyPath);
	if (cmd.size() > 1) conf.sysproxyPath = cmd;
	cmd = arg.read("PolipoPath", conf.polipoPath);
	if (cmd.size() > 1) conf.polipoPath = cmd;

	cmd = arg.read("WGetPath", conf.wgetPath);
	if (cmd.size() > 1) conf.wgetPath = cmd;

	cmd = arg.read("TempPath", conf.tempPath);
	if (cmd.size() > 1) conf.tempPath = cmd;

	cmd = arg.read("Bootstrap DNS", conf.bootstrapDNS);
	if (cmd.size() > 1) conf.bootstrapDNS = cmd;

	cmd = arg.read("Udp Timeout (s)", toString(conf.udpTimeout));
	if (cmd.size() > 0) conf.udpTimeout = parse<UInt>(cmd);

	cmd = arg.read("Ignore check result (s)", toString(conf.IGNORECHECKSERVER));
	if (cmd.size() > 0) conf.IGNORECHECKSERVER = parse<bool>(cmd);

	cmd = arg.read("Hide DNS2Socks", toString(conf.hideDNS2Socks));
	if (cmd.size() > 0) conf.hideDNS2Socks = parse<bool>(cmd);

	cmd = arg.read("Enable logging file", toString(conf.enableLogging));
	if (cmd.size() > 0) conf.enableLogging = parse<bool>(cmd);

	ctrl.SaveConfig();
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::Edit(ParamsReader & arg) {
	String cmd = arg.read("type");
	if (HELP_COND) {
		cout << "Supported type:\n\ttask\n\tserver\n\tsettings\n\tvpn";
		return;
	}
	if (cmd == "task") {
		EditTask(arg);
		return;
	}
	if (cmd == "server") {
		EditServer(arg);
		return;
	}
	if (cmd == "settings") {
		EditSettings(arg);
		return;
	}
	if (cmd == "vpn") {
		EditVPN(arg);
		return;
	}
	cout << "Unknow command '" << cmd << "'\nUse edit --help\n";
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::Connect(ParamsReader & arg) {
	String cmd = arg.read("ip");
	if (HELP_COND) {
		cout << "connect ${ip} ${port}";
		return;
	}

	if (TCPClient::IsCanConnect(cmd, parse<unsigned int>(arg.read("port")), ctrl.getConfig().bootstrapDNS))
		cout << "Connect success\n";
	else
		cout << "Connect fail\n";

}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::Resolve(ParamsReader &arg) {
	String cmd = arg.read("host");
	if (HELP_COND) {
		cout << "resolve ${host}";
		return;
	}
	auto list = resolveDNS(cmd, this->ctrl.getConfig().bootstrapDNS);
	for (const String & ip : list)
		cout << ip << "\n";
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::FindFreePorts(ParamsReader & arg) {
	String cmd = arg.read("Host");
	if (HELP_COND) {
		cout << "find freeports ${host} ${count}\n";
		return;
	}
	int count = 5;
	if (arg.size() == 3)
		count = arg.readUInt("Count");
	cout << "Free port: ";
	for (int i = 0; i < count; i++)
		 cout << ShadowSocksClient::findAllowPort(cmd) << ", ";
	cout << "\n";
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::Find(ParamsReader &arg) {
	String cmd = arg.read("type");
	if (HELP_COND) {
		cout << "Supported type:\n\tfreeports\n";
		return;
	}
	if (cmd == "freeports") {
		FindFreePorts(arg);
		return;
	}
	cout << "Unknow comand. Use find --help\n";
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::Enable(ParamsReader &arg) {
	String cmd = arg.read("Task");
	if (HELP_COND) {
		cout << "Enable autostart for task\n. If want enable autostart task use enable autostart\n";
		return;
	}
	if (cmd == "autostart") {
		this->ctrl.getConfig().autostart = true;
		ctrl.SaveConfig();
		return;
	}
	_Task * t = ctrl.getConfig().findTaskByName(cmd);
	if (t == nullptr) {
		cout << "Unknow task\n";
		return;
	}
	t->autostart = true;
	ctrl.SaveConfig();
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::Disable(ParamsReader &arg) {
	String cmd = arg.read("Task");
	if (HELP_COND) {
		cout << "Disable autostart for task\n. If want disable autostart task use disable autostart\n";
		return;
	}
	if (cmd == "autostart") {
		this->ctrl.getConfig().autostart = false;
		ctrl.SaveConfig();
		return;
	}
	_Task * t = ctrl.getConfig().findTaskByName(cmd);
	if (t == nullptr) {
		cout << "Unknow task\n";
		return;
	}
	t->autostart = false;
	ctrl.SaveConfig();
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::EditVPN(ParamsReader & arg) {
	String cmd = arg.read("Name");
	if (HELP_COND) {
		cout << "edit vpn ${Name} ${NewName} ${Default Route IP} ${Tun Name} ${Remove default route: bool} ${isDNS2Socks:bool} ${DNS:array} ${Ignore IP}\n";
		return;
	}

	Tun2SocksConfig conf = ctrl.getConfig().findVPNbyName(cmd);
	if (conf.isNull) {
		cout << "VPN is not found\n";
		return;
	}

	//String tx = arg.read("New Name", conf.name);
	//if (tx.size() > 1) conf.name = tx;
	String tx = arg.read("Default route", conf.defaultRoute);
	if (tx.size() > 1) conf.defaultRoute = tx;
	tx = arg.read("Tun interface name", conf.tunName);
	if (tx.size() > 1) conf.tunName = tx;
	tx = arg.read("Remove default route (0/1)", toString(conf.removeDefaultRoute));
	if (tx.size() > 0) conf.removeDefaultRoute = parse<bool>(tx);
	tx = arg.read("Enable DNS2Socks", toString(conf.isDNS2Socks));
	if (tx.size() > 0) conf.isDNS2Socks = parse<bool>(tx);

	int del = false;
	while (true) {
		tx = arg.read("DNS");
		if (tx.size() > 0) {
			if (!del) {
				conf.dns.clear();
				del = true;
			}
			conf.dns.push_back(tx);
		} else
			break;
	}
	del = false;
	while (true) {
		tx = arg.read("Ignore IP");
		if (tx.size() > 0) {
			if (!del) {
				conf.ignoreIP.clear();
				del = true;
			}
			conf.ignoreIP.push_back(tx);
		} else
			break;
	}


	auto res = ctrl.getConfig().findVPNbyName(cmd);
	ctrl.getConfig().deleteVPNByName(cmd);
	if (!ctrl.getConfig().CheckT2S(conf)) {
		ctrl.getConfig().tun2socksConf.push_back(res);
		cout << "Fail to add VPN mode\n";
		return;
	}
	ctrl.getConfig().tun2socksConf.push_back(conf);
	ctrl.SaveConfig();
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::VPNAdd(ParamsReader & arg) {
	String cmd = arg.read("Name");
	if (HELP_COND) {
		cout << "vpn add ${Name} ${Default Route IP} ${Tun Name} ${Remove default route: bool} ${isDNS2Socks:bool} ${DNS:array} ${Ignore IP}\n";
		return;
	}
	Tun2SocksConfig conf;
	conf.name = cmd;
	conf.defaultRoute = arg.read("Default route (Example 192.168.0.1)");
	conf.tunName = arg.read("Tun interface name");
	conf.removeDefaultRoute = arg.readbool("Remove default route (0/1)");
	conf.isDNS2Socks = arg.readbool("Enable DNS2Socks");
	cmd = arg.read("DNS");
	while (cmd.size() > 1) {
		conf.dns.push_back(cmd);
		cmd = arg.read("DNS");
	}
	cmd = arg.read("Ignore IP");
	while (cmd.size() > 1) {
		conf.ignoreIP.push_back(cmd);
		cmd = arg.read("Ignore IP");
	}
	if (!ctrl.getConfig().CheckT2S(conf)) {
		cout << "Fail to add VPN mode\n";
		return;
	}
	conf.isNull = false;
	ctrl.getConfig().tun2socksConf.push_back(conf);
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::VPN(ParamsReader & arg) {
	String cmd = arg.read("Comand");
	if (HELP_COND) {
		cout << "Use VPN Mode.\nSupported comand:\n\tadd\nupdate - Update interface for VPN\n";
		return;
	}
	if (cmd == "add") {
		VPNAdd(arg);
		return;
	}
	if (cmd == "update") {
		VPNUpdate(arg);
		return;
	}
	cout << "Unknow comand. Use vpn --help\n";
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::VPNUpdate(ParamsReader & arg) {
	String cmd = arg.read("Name");
	if (HELP_COND) {
		cout << "vpn update ${Name} ${response:y/n}\n";
		return;
	}
	Tun2SocksConfig conf = ctrl.getConfig().findVPNbyName(cmd);
	if (conf.isNull) {
		cout << "VPN is not found\n";
		return;
	}

	String route = Tun2Socks::DetectDefaultRoute();
	cout << "Detected default route: " << route << "\n";
	String tapInterface = Tun2Socks::DetectInterfaceName();
	cout << "Detected tun interface name: " << tapInterface << "\n";
	String result = arg.read("Save (y/n)");
	if (result == "y") {
		conf.tunName = tapInterface;
		conf.defaultRoute = route;
		ctrl.getConfig().deleteVPNByName(cmd);
		ctrl.getConfig().tun2socksConf.push_back(conf);
		ctrl.SaveConfig();
	}

}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::CheckSpeedTest(ParamsReader & arg, bool enable_speed) {
	const auto & tasks = ctrl.getConfig().tasks;
	struct ResultCheck{
		String ipAddr;

		double speed_100;
		double speed_500;

		bool isRun = false;
		bool checked = false;
		String msg = "";
		UInt socks_port;
		UInt http_port;
	};

	bool isFast = false;

	while (!arg.isEmpty()) {
		String flag = arg.read("");
		if (flag == "FAST") {
			isFast = true;
		}
	}

	Map<String, ResultCheck * > data;
	cout << "Start run all servers\n";

	__DP_LIB_NAMESPACE__::List<UInt> socks5_ports;
	__DP_LIB_NAMESPACE__::List<UInt> http_ports;
	{
		UInt socks5_port = 12000;
		UInt http_port = 16000;

		for (const _Task * task : tasks)
			for (int id : task->servers_id) {
				while (!ShadowSocksClient::portIsAllow(ctrl.getConfig().defaultHost, socks5_port))
					socks5_port += 1;
				socks5_ports.push_back(socks5_port);
				socks5_port++;

				while (!ShadowSocksClient::portIsAllow(ctrl.getConfig().defaultHost, http_port))
					http_port += 1;
				http_ports.push_back(http_port);
				http_port++;
			}
	}
	// Ждем пока освободятся порты.
	std::this_thread::sleep_for(std::chrono::milliseconds(500));



	for (const _Task * task : tasks) {
		for (int id : task->servers_id) {
			_Server * server = ctrl.getConfig().findServerById(id);
			ResultCheck * res = new ResultCheck();
			SSClientFlags flags;

			UInt socks5_port = *socks5_ports.begin();
			UInt http_port = *http_ports.begin();
			socks5_ports.pop_front();
			http_ports.pop_front();

			flags.port = socks5_port;
			flags.http_port = http_port;
			res->socks_port = flags.port;
			res->http_port = flags.http_port;
			flags.runVPN = false;
			flags.vpnName = "";
			flags.server_name = server->name;
			auto onCrash = [this, res]() {
				res->checked = true;
				res->isRun = false;
				res->msg = "Fail to run";
			};

			data[server->name] = res;

			try{
				ctrl.StartByName(task->name, [res]() {
					res->isRun = true;
				}, onCrash, flags);
			} catch (__DP_LIB_NAMESPACE__::LineException e) {
				res->isRun = false;
				res->checked = true;
				res->msg = e.toString();
			}
		}
	}
	cout << "All servers started. Wait for start success\n";
	std::this_thread::sleep_for(std::chrono::milliseconds(2000));
	{
		for (auto& it : data) {
			if (!it.second->isRun)
				continue;
			ProxyElement proxy;
			proxy.host = ctrl.getConfig().defaultHost;
			proxy.port = it.second->http_port;
			proxy.type = ProxyElement::Type::Http;
			Downloader dwn(proxy);
			dwn.SetWget(ctrl.getConfig().getWGetPath());
			dwn.SetIgnoreCheckCert(true);
			String url = "http://ipconfig.cc";
			try{
				it.second->ipAddr = dwn.Download(url);
			} catch (__DP_LIB_NAMESPACE__::LineException e) {
				it.second->isRun = false;
				it.second->checked = true;
				it.second->msg = e.toString();
				continue;
			}

			it.second->checked = true;
			it.second->isRun = it.second->ipAddr.size() > 3;

			if (enable_speed && it.second->isRun) {
				Path p (ctrl.getConfig().tempPath);
				p.Append("speed.zip");
				auto t1 = std::chrono::high_resolution_clock::now();
				try{
					dwn.Download("http://speedtest.wdc01.softlayer.com/downloads/test100.zip", p.Get());
				} catch (__DP_LIB_NAMESPACE__::LineException e) {
					it.second->isRun = false;
					it.second->checked = true;
					it.second->msg = e.toString();
					continue;
				}
				auto t2 = std::chrono::high_resolution_clock::now();
				__DP_LIB_NAMESPACE__::RemoveFile(p.Get());

				auto time_s = std::chrono::duration_cast<std::chrono::seconds> (t2 - t1).count();
				auto time_ms = std::chrono::duration_cast<std::chrono::milliseconds> (t2 - t1).count();
				if (time_s == 0) {
					if (time_ms == 0)
						it.second->speed_100 = 0;
					else
						it.second->speed_100 = (100.0 * 1000) / time_ms;
				} else
					it.second->speed_100 = (100.0) / time_s;

			}
		}
	}

	{
		for (auto& it : data) {
			if (!it.second->isRun)
				continue;
			ProxyElement proxy;
			proxy.host = ctrl.getConfig().defaultHost;
			proxy.port = it.second->http_port;
			proxy.type = ProxyElement::Type::Http;
			Downloader dwn(proxy);
			dwn.SetWget(ctrl.getConfig().getWGetPath());
			dwn.SetIgnoreCheckCert(true);

			if (enable_speed && !isFast) {
				Path p (ctrl.getConfig().tempPath);
				p.Append("speed.zip");
				auto t1 = std::chrono::high_resolution_clock::now();
				try{
					dwn.Download("http://speedtest.wdc01.softlayer.com/downloads/test500.zip", p.Get());
				} catch (__DP_LIB_NAMESPACE__::LineException e) {
					it.second->isRun = false;
					it.second->checked = true;
					it.second->msg = e.toString();
					continue;
				}
				auto t2 = std::chrono::high_resolution_clock::now();
				__DP_LIB_NAMESPACE__::RemoveFile(p.Get());

				auto time_s = std::chrono::duration_cast<std::chrono::seconds> (t2 - t1).count();
				auto time_ms = std::chrono::duration_cast<std::chrono::milliseconds> (t2 - t1).count();
				if (time_s == 0) {
					if (time_ms == 0)
						it.second->speed_500 = 0;
					else
						it.second->speed_500 = (500.0 * 1000) / time_ms;
				} else
					it.second->speed_500 = (500.0) / time_s;
			}
		}
	}


	{
		if (enable_speed)
			cout << "Server\tStatus\tip\tSpeed 100\tSpeed 500\tmsg\n";
		else
			cout << "Server\tStatus\tip\tmsg\n";
		for (auto& it : data) {
			ctrl.StopByName(it.first);
			if (enable_speed)
				cout << it.first << "\t" << (it.second->isRun ? "OK" : "FAIL") << "\t" << it.second->ipAddr << "\t" << it.second->speed_100 << "\t" << it.second->speed_500 << "\t" << it.second->msg << "\n";
			else
				cout << it.first << "\t" << (it.second->isRun ? "OK" : "FAIL") << "\t" << it.second->ipAddr << "\t" << it.second->msg << "\n";
			delete it.second;
		}
	}
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::StartTask(ParamsReader & arg){
	String cmd = arg.read("Task");
	if (HELP_COND) {
		cout << "Start task\n";
		cout << "Use: start ${TaskName} ${check:null<string(NOCHECK)>} ${check:null<string(NOVPN)>} ${check:null<string(SPORT=${port})>} ${check:null<string(SERVER=${servername})>}\n";
		return;
	}
	_Task * t = ctrl.getConfig().findTaskByName(cmd);
	if (t == nullptr) {
		cout << "Unknow task\n";
		return;
	}
	bool nocheck = false;
	bool prev = ctrl.getConfig().IGNORECHECKSERVER;
	SSClientFlags flags;
	while (!arg.isEmpty()) {
		String flag = arg.read("");
		if (flag == "NOCHECK") {
			nocheck = true;
			auto & cnf = ctrl.getConfig();
			cnf.IGNORECHECKSERVER = true;
		}
		if (flag == "NOVPN") {
			flags.runVPN = false;
		}
		if (__DP_LIB_NAMESPACE__::startWith(flag, "VPN=")) {
			String vpn = flag.substr(String("VPN=").size());
			flags.runVPN = true;
			flags.vpnName = vpn;
		}
		if (__DP_LIB_NAMESPACE__::startWith(flag, "SPORT=")) {
			String port = flag.substr(String("SPORT=").size());
			flags.port = parse<unsigned short>(port);
		}
		if (__DP_LIB_NAMESPACE__::startWith(flag, "SERVER=")) {
			String port = flag.substr(String("SERVER=").size());
			flags.server_name = port;
		}

	}
	auto funcCrash = [this, t, cmd] () {
		cout << "Fail to start task " << t->name << "\nCheck it\n";
		ctrl.StopByName(cmd);
	};

	ctrl.StartByName(cmd, [this, t] () {
		cout << "Task " << t->name << " succesfully started\n";
	}, funcCrash, flags);
	if (nocheck) {
		auto & cnf = ctrl.getConfig();
		cnf.IGNORECHECKSERVER = prev;
	}
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::StopTask(ParamsReader & arg) {
	String cmd = arg.read("Task");
	if (HELP_COND) {
		cout << "Stop task\n";
		return;
	}
	_Task * t = ctrl.getConfig().findTaskByName(cmd);
	if (t == nullptr) {
		cout << "Unknow task\n";
		return;
	}
	ctrl.StopByName(cmd);
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::TunAdd(ParamsReader & arg) {
	String cmd = arg.read("Task");
	if (HELP_COND) {
		cout << "Add tun for tasks\n";
		return;
	}
	_Task * t = ctrl.getConfig().findTaskByName(cmd);
	if (t == nullptr) {
		cout << "Unknow task\n";
		return;
	}
	cmd = arg.read("Protocol (tcp/udp)");
	_Tun tun;
	bool inited = false;

	if (cmd == "tcp" || cmd == "TCP") {
		tun.type = TunType::TCP;
		inited = true;
	}
	if (cmd == "udp" || cmd == "UDP") {
		inited = true;
		tun.type = TunType::UDP;
	}
	if (!inited) {
		cout << "Unknow protocol";
		return;
	}
	tun.localPort = arg.readUInt("Local Port");
	tun.remoteHost = arg.read("Remote Host");
	tun.remotePort = arg.readUInt("Remote Port");
	t->tuns.push_back(tun);
	ctrl.SaveConfig();
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::Tun(ParamsReader & arg) {
	String cmd = arg.read("comand");
	if (HELP_COND) {
		cout << "Supported comand:\n\tadd\n";
		return;
	}
	if (cmd == "add") {
		TunAdd(arg);
		return;
	}
	cout << "Unknow comand. Use tun --help\n";
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::ManualCMD(const String & _cmd) {
	String cmd = _cmd + "\n";
	ParamsReader args = ParamsReader(cout, cin);
	String c = "";
	int prev = 0;
	for (int i = 0; i < cmd.size(); i++) {
		if (cmd[i] == ' ' || cmd[i] == '\t' || cmd[i] == '\n') {
			String t = cmd.substr(prev, i-prev);
			prev = i+1;
			if (c.size() == 0)
				c = t;
			else
				args.AddParams(t);

		}
	}
	__DP_LIB_NAMESPACE__::log << "Command '" << c << "'\n";
	if (__DP_LIB_NAMESPACE__::ConteinsKey(funcs, c)) {
		auto func = funcs[c];
		(this->*func)(args);
	} else {
		__DP_LIB_NAMESPACE__::log << "Unknow comand '" << c << "'\n";
		throw EXCEPTION("Unknow comand '" + c + "'");
	}
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::Loop() {
	if (! ctrl.isCreated() || (ctrl.isCreated() && ctrl.isEncrypted())) {
		while (true) {
			if (ctrl.isOpened())
				break;
			cout << "Write password:";
			cout.flush();
			String password;
			getline(cin, password);
			password = __DP_LIB_NAMESPACE__::trim(password);
			__DP_LIB_NAMESPACE__::log << "Input password: '" << password << "'\n";

			if (SetPassword(password)) {
				__DP_LIB_NAMESPACE__::log << "Config parsed\n";
				break;
			}
		}
	} else {
		ctrl.OpenConfig();
		__DP_LIB_NAMESPACE__::log << "WARNING: Config is not encrypted\n";
	}
	if (ctrl.getConfig().enableLogging)
		__DP_LIB_NAMESPACE__::log.OpenFile("LOGGING.txt");

	if (ctrl.CheckInstall()) {
		auto funcCrash = [this] () {
			__DP_LIB_NAMESPACE__::log << "Fail to auto start tasks.\n";
			cout << "Fail to auto start tasks." << "\nCheck its\n";
		};
		ctrl.AutoStart(funcCrash);
	} else {
		__DP_LIB_NAMESPACE__::log << "Fail install. Try exit and enter new password. Or reinstall application\n";
		cout << "Fail install. Try exit and enter new password. Or reinstall application\n";
	}

	is_exit = false;
	cout << "Write help to get help\n";
	__DP_LIB_NAMESPACE__::log << "Started command loop\n";
	while (!is_exit && (!__DP_LIB_NAMESPACE__::ServiceSinglton::Get().NeedToExit()) && (!cin.eof())) {
		cout << "> ";
		cout.flush();
		String cmd;
		getline(cin, cmd);
		__DP_LIB_NAMESPACE__::log << "Input command: '" << cmd << "'\n";
		try {
			ManualCMD(cmd);
		} catch (StructLogout e) {
			__DP_LIB_NAMESPACE__::log << "User Logout\n";
			break;
		} catch (__DP_LIB_NAMESPACE__::LineException e) {
			cout << e.toString() << "\nUse help\n";
		}
	}
	if (is_exit)
		MakeExit();
}

template <typename OutStream, typename InStream>
bool ConsoleLooper<OutStream, InStream>::SetPassword(const String & password) {
	try {
		if (ctrl.isCreated()) {
			ctrl.OpenConfig(password);
		} else {
			if (password.size() > 0)
				ctrl.SetPassword(password);
			ctrl.SaveConfig();
		}
	} catch (__DP_LIB_NAMESPACE__::BaseException e) {
		return false;
	}
	return true;
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::AddServer(ParamsReader &arg) {
	String cmd = arg.read("Name");
	if (HELP_COND) {
		cout << "add server ${Name} ${Host} ${Port:int} ${Plugin} ${Plugin opts:array}\n";
		cout << "Supported plugin:\n\tnone\n\tv2ray\n";
		return;
	}
	_Server * sr = new _Server();
	sr->name = cmd;
	sr->host = arg.read("Host");

	cmd = arg.read("Port");
	if (cmd.size() > 1) {
		if (startWithN(cmd, "${") && __DP_LIB_NAMESPACE__::endWithN(cmd, "}")) {
			sr->port = cmd;
		} else {
			if (!isNumber(cmd)) {
				cout << "Port is invalid";
				return;
			}
			sr->port = cmd;
		}
	}
	#define SaveSRV()\
		{	\
			if (ctrl.getConfig().CheckServer(sr)) { \
				ctrl.getConfig().servers.push_back(sr); \
				ctrl.SaveConfig(); \
			} else {\
				cout << "Server is not correct. Try later\n"; \
			} \
		}

	if (arg.size() == 4) {
		SaveSRV();
		return;
	}
	String plugin = arg.read("Plugin");
	if (plugin == "none") {
		SaveSRV();
		return;
	}
	if (plugin == "v2ray") {
		_V2RayServer * t;
		sr = t = new _V2RayServer(sr);
		t->isTLS = arg.readbool("isTLS");
		t->mode = arg.read("Mode");
		t->v2host = arg.read("V2Ray host");
		t->path = arg.read("V2Ray path");
		SaveSRV();
		return;
	}
	cout << "Unknow plugin. Ignore new server\n";
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::AddTask(ParamsReader &arg) {
	String cmd = arg.read("Name");
	if (HELP_COND) {
		cout << "add task ${Name} ${Password} ${Method} ${Server:List<Int> = null}\n";
		cout << "Supported plugin:\n\tnone\n\tv2ray\n";
		return;
	}

	_Task * tk = new _Task();
	tk->name = cmd;
	tk->password =arg.read("Password");
	tk->method = arg.read("Method");
	String mt = ctrl.getConfig().replaceVariables(tk->method);
	if (mt == "chacha20_poly1305") tk->method = "AEAD_CHACHA20_POLY1305";
	if (mt == "aes128") tk->method = "AEAD_AES_128_GCM";
	if (mt == "aes256") tk->method = "AEAD_AES_256_GCM";

	while (!arg.isEmpty()) {
		String c = arg.read("Server");
		_Server * sr = ctrl.getConfig().findServerByName(c);
		if (sr == nullptr) {
			cout << "Unknow server " << c;
			return;
		}
		tk->servers_id.push_back(sr->id);
	}
	if (!ctrl.getConfig().CheckTask(tk)) {
		cout << "Fail to check task. See help\n";
		return;
	}
	ctrl.getConfig().tasks.push_back(tk);
	ctrl.SaveConfig();
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::CheckPort(ParamsReader &arg) {
	String host = arg.read("Host");
	while (true) {
		String p = arg.read("Port");
		if (p.size() == 0)
			break;
		cout << "Port allow: " << ShadowSocksClient::portIsAllow(host, parse<UInt>(p)) << "\n";
	}
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::Check(ParamsReader &arg) {
	String cmd = arg.read("Comand");
	if (HELP_COND) {
		cout << "check port ${host} ${port}\n";
		cout << "check servers\n";
		cout << "check speed\n";
		return;
	}
	if (cmd == "port")
		CheckPort(arg);
	if (cmd == "servers")
		CheckSpeedTest(arg, false);
	if (cmd == "speed")
		CheckSpeedTest(arg, true);
}


template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::Add(ParamsReader &arg) {
	String cmd = arg.read("type");
	if (HELP_COND) {
		cout << "Supported type:\n\ttask\n\tserver\n";
		return;
	}
	bool inited = false;
	if (cmd == "task") {
		AddTask(arg); inited = true; }
	if (cmd == "server") {
		AddServer(arg);inited = true; }
	if (!inited) {
		cout << "Unknow comand" << cmd<<"\n";
	}
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::DeleteTask(ParamsReader & arg) {
	String cmd = arg.read("Name");
	if (HELP_COND) {
		cout << "delete task\n";
		return;
	}
	_Task * _sr = ctrl.getConfig().findTaskByName(cmd);
	if (_sr == nullptr) {
		cout << "Task is not found";
		return;
	}
	String r = arg.read("You shure? (Y/n):");
	if (r[0] == 'Y') {
		ctrl.getConfig().deleteTaskById(_sr->id);
		ctrl.SaveConfig();
		return;
	}
	cout << "Abort\n";
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::DeleteServer(ParamsReader & arg) {
	String cmd = arg.read("Name");
	if (HELP_COND) {
		cout << "delete server ${Name}\n";
		return;
	}
	_Server * _sr = ctrl.getConfig().findServerByName(cmd);
	if (_sr == nullptr) {
		cout << "Server is not found";
		return;
	}
	for (_Task * t : ctrl.getConfig().tasks)
		for (int srId : t->servers_id)
			if (srId == _sr->id) {
				cout << "Can't remove this server. It use in task " << t->name << "\n";
				return;
			}
	String r = arg.read("You shure? (Y/n):");
	if (r[0] == 'Y') {
		ctrl.getConfig().deleteServerById(_sr->id);
		ctrl.SaveConfig();
		return;
	}
	cout << "Abort\n";
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::DeleteTun(ParamsReader & arg) {
	String cmd = arg.read("Name");
	if (HELP_COND) {
		cout << "delete tun ${task_id} ${proto} ${port}\n";
		return;
	}
	_Task * _sr = ctrl.getConfig().findTaskByName(cmd);
	if (_sr == nullptr) {
		cout << "Task is not found";
		return;
	}
	cmd = arg.read("Protocol (tcp/udp)");
	TunType tt;
	bool inited = false;

	if (cmd == "tcp" || cmd == "TCP") {
		tt = TunType::TCP;
		inited = true;
	}
	if (cmd == "udp" || cmd == "UDP") {
		inited = true;
		tt = TunType::UDP;
	}
	if (!inited) {
		cout << "Unknow protocol";
		return;
	}
	int lp = arg.readUInt("Local Port");
	inited = false;
	for (auto it = _sr->tuns.begin(); it != _sr->tuns.end(); it++)
		if (it->type == tt && it->localPort == lp) {
			_sr->tuns.erase(it);
			inited = true;
			ctrl.SaveConfig();
			break;
		}
	if (!inited) {
		cout << "Can't find current tun\n";
	}

}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::Delete(ParamsReader & arg) {
	String cmd = arg.read("type");
	if (HELP_COND) {
		cout << "Support type:\n\ttask\n\tserver\n\ttun\n";
		return;
	}
	if (cmd == "tun") {
		DeleteTun(arg);
		return;
	}
	if (cmd == "server") {
		DeleteServer(arg);
		return;
	}
	if (cmd == "task") {
		DeleteTask(arg);
		return;
	}
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::SetConfig(ParamsReader & arg) {
	String cmd = arg.read("Config File");
	if (HELP_COND) {
		cout << "set config ${filename}\n";
		return;
	}
	Path tmp (cmd);
	if (! tmp.IsFile()) {
		cout << "File is not exists.";
		return;
	}
	String all = ReadAllFile(cmd);
	ctrl.SaveConfig(all);
	exit(0);
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::SetVariable(ParamsReader & arg) {
	String cmd = arg.read("Name");
	if (HELP_COND) {
		cout << "set variable ${name} ${value}\n";
		return;
	}
	String value = arg.read("Value");
	ShadowSocksSettings & conf = ctrl.getConfig();
	conf.variables[cmd] = value;
	ctrl.SaveConfig();
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::Set(ParamsReader & arg) {
	String cmd = arg.read("type");
	if (HELP_COND) {
		cout << "Support:\n\tconfig ${filename}\nvariable name value\n";
		return;
	}
	if (cmd == "config")
		SetConfig(arg);
	if (cmd == "variable")
		SetVariable(arg);
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::ExportConfig(ParamsReader & arg) {
	String cmd = arg.read("Config File");
	if (HELP_COND) {
		cout << "save config to json file ${filename}\n";
		return;
	}
	Path tmp (cmd);
	if (tmp.IsFile()) {
		cout << "File is exists.";
		return;
	}
	bool isMobile = arg.readbool("Is Mobile? (0/1)");
	String v2ray = "v2ray-plugin";
	String dns = "94.140.14.14"; //"8.8.8.8";
	bool is_resolve = arg.readbool("Resolve DNS? (0/1)");

	if (isMobile) {
		String oo  = arg.read("DNS", dns);
		if (oo.size() > 0)
			dns = oo;
	} else {
		v2ray = arg.read("V2Ray path (Ignore if it don't use");
	}

	__DP_LIB_NAMESPACE__::Ofstream out;
	out.open(tmp.Get());
	if (!isMobile) {
		out << "{\n";
		out << "\t\"version\": \"4.1.10.0\",\n";
		out << "\t\"configs\":\n";
	}
	out << "\t[\n";
	int num = 0;
	ShadowSocksSettings & config = ctrl.getConfig();
	for (_Task * t: ctrl.getConfig().tasks) {
		num += 1;
		int srv_num = 0;
		for (int id : t->servers_id) {
			srv_num ++;
			for (_Server * sr : ctrl.getConfig().servers)
				if (sr->id == id) {
					__DP_LIB_NAMESPACE__::List<String> ip_addrs;
					String _host = config.replaceVariables(sr->host);
					if (isIPv4(_host) || (!is_resolve)) {
						ip_addrs.push_back(_host);
					} else {
						ip_addrs = resolveDNS(_host, config.bootstrapDNS);
						if (ip_addrs.size() == 0) {
							cout << "Fail to resolve host" << _host << "\nSkeep it\n";
							ip_addrs.push_back (_host);
						}
					}
					int i = -1;
					for (const String & ip : ip_addrs) {
						i++;
						out << "\t{\n";
						out << "\t\t\"server\": \"" << ip << "\",\n";
						out << "\t\t\"server_port\": " << config.replaceVariables(sr->port) << ",\n";
						out << "\t\t\"password\": \"" << config.replaceVariables(t->password) << "\",\n";
						out << "\t\t\"method\": \"";

						String mt = config.replaceVariables(t->method);
						if (mt == "AEAD_CHACHA20_POLY1305")
							out << "chacha20-ietf-poly1305";
						if (mt == "AEAD_AES_256_GCM")
							out << "aes-256-gcm";
						if (mt == "AEAD_AES_128_GCM")
							out << "aes-128-gcm";
						out << "\",\n";
						_V2RayServer * srv = dynamic_cast<_V2RayServer *> (sr);
						out << "\t\t\"plugin\": \"";
						if (srv != nullptr) {
							out << v2ray;
						}
						out << "\",\n";
						out << "\t\t\"plugin_opts\": \"";
						if (srv != nullptr) {
							if (srv->isTLS)
								out << "tls;";
							out << "mode=" << config.replaceVariables(srv->mode) << ";host=" << config.replaceVariables(srv->v2host) << ";path=" << config.replaceVariables(srv->path);
						}
						out << "\",\n";
						if (!isMobile) {
							out << "\t\t\"plugin_args\": \"\",\n";
							out << "\t\t\"timeout\": " << 10 << ",\n";
						}
						out << "\t\t\"remarks\": \"" << sr->name << (ip_addrs.size() > 1 ? "_" + toString(i) : "") << "\"" << (isMobile ? "," : "") << "\n";
						if (isMobile) {
							String _dns = dns;
							if (t->tun2SocksName.size() > 1) {
								const Tun2SocksConfig tun = config.findVPNbyName(t->tun2SocksName);
								if (tun.dns.size() > 0)
									_dns = *(tun.dns.begin());

							}

							out << "\t\t\"route\": \"all\",\n";
							out << "\t\t\"remote_dns\": \"" << _dns << "\",\n";
							out << "\t\t\"ipv6\": false,\n";
							out << "\t\t\"metered\": false,\n";
							out << "\t\t\"proxy_apps\": {\n";
							out << "\t\t\t\"enabled\": false\n";
							out << "\t\t},\n";
							out << "\t\t\"udpdns\": false\n";
						}
						out << "\t}";
						if (!((ctrl.getConfig().tasks.size() == num) && (srv_num == t->servers_id.size()) && (i == (ip_addrs.size() - 1)) ) ) {
							out <<",\n";
						} else {
							out << "\n";
						}
					}


				}
		}
	}
	out << "\t]";
	if (!isMobile) {
		out << ",\n";
		out << "\t\"strategy\": null,\n";
		out << "\t\"index\": 0,\n";
		out << "\t\"global\": true,\n";
		out << "\t\"enabled\": false,\n";
		out << "\t\"shareOverLan\": false,\n";
		out << "\t\"isDefault\": false,\n";
		out << "\t\"isIPv6Enabled\": false,\n";
		out << "\t\"localPort\": 1080,\n";
		out << "\t\"portableMode\": true,\n";
		out << "\t\"showPluginOutput\": false,\n";
		out << "\t\"pacUrl\": null,\n";
		out << "\t\"gfwListUrl\": null,\n\t";
		out << R"("useOnlinePac": false,
		"secureLocalPac": true,
		"availabilityStatistics": false,
		"autoCheckUpdate": true,
		"checkPreRelease": false,
		"isVerboseLogging": false,
		"logViewer": {
		  "topMost": false,
		  "wrapText": false,
		  "toolbarShown": false,
		  "Font": "Consolas, 8pt",
		  "BackgroundColor": "Black",
		  "TextColor": "White"
		},
		"proxy": {
		  "useProxy": false,
		  "proxyType": 1,
		  "proxyServer": "localhost",
		  "proxyPort": 8080,
		  "proxyTimeout": 3,
		  "useAuth": false,
		  "authUser": "",
		  "authPwd": ""
		},
		"hotkey": {
		  "SwitchSystemProxy": "",
		  "SwitchSystemProxyMode": "",
		  "SwitchAllowLan": "",
		  "ShowLogs": "",
		  "ServerMoveUp": "",
		  "ServerMoveDown": "",
		  "RegHotkeysAtStartup": false
		}
	  })";
	}
	out.close();

}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::Export(ParamsReader & arg) {
	String cmd = arg.read("type");
	if (HELP_COND) {
		cout << "Support:\n\tconfig ${filename}\n";
		return;
	}
	if (cmd == "config")
		ExportConfig(arg);
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::SaveConfig(ParamsReader & arg) {
	String cmd = arg.read("Config File");
	if (HELP_COND) {
		cout << "save config ${filename}\n";
		return;
	}
	Path tmp (cmd);
	if (tmp.IsFile()) {
		cout << "File is exists.";
		return;
	}
	__DP_LIB_NAMESPACE__::Ofstream out;
	out.open(cmd);
	out << ctrl.getConfig().GetSource();
	out.close();
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::Save(ParamsReader & arg) {
	String cmd = arg.read("type");
	if (HELP_COND) {
		cout << "Support:\n\tconfig ${filename}\n";
		return;
	}
	if (cmd == "config")
		SaveConfig(arg);
}

#define ec(NAME, VAL) cout << NAME << " \t=\t" << VAL << "\n";

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::ListTasks(ParamsReader & arg) {
	const auto & srvs = ctrl.getConfig().tasks;
	for (const _Task * sr : srvs) {
		ec("id", sr->id);
		ec("name", sr->name);
		ec("method", sr->method);
		ec("local host", (sr->localHost.size() < 1 ? ctrl.getConfig().defaultHost : sr->localHost));
		ec("local port", (sr->localPort< 1 ? ctrl.getConfig().defaultPort : sr->localPort));
		ec("vpn", sr->tun2SocksName)
		cout << "Servers: ";
		for (int id : sr->servers_id)
			cout << ctrl.getConfig().findServerById(id)->name << " (" << id << ") , ";
		cout << "\n------------------------------------------\n";
	}
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::ListServers(ParamsReader & arg) {
	cout
		 << std::setw(5) << "ID"
		 << std::setw(15) << "Name"
		 << std::setw(30) << "Host:Port"
		 << std::setw(10) << "Plugin"
		 << std::setw(10) << "Mode"
		 << std::setw(7) << "isTls"
		 << std::setw(30) << "Host/Path\n";

	const auto & srvs = ctrl.getConfig().servers;
	for (const _Server * sr : srvs) {
		cout
			 << std::setw(5) << sr->id
			 << std::setw(15) << sr->name
			 << std::setw(30) << (sr->host + ":" + sr->port);
		{
			const _V2RayServer * srv = dynamic_cast<const _V2RayServer *> (sr);
			if (srv != nullptr) {
				cout
					 << std::setw(10) << "V2Ray"
					 << std::setw(10) << srv->mode
					 << std::setw(7) << srv->isTLS
					 << std::setw(30) << (srv->v2host + srv->path);
			}
		}
		cout << "\n";
		/*ec("id", sr->id);
		ec("name", sr->name);
		ec("host", sr->host);
		ec("port", sr->port);
		{
			const _V2RayServer * srv = dynamic_cast<const _V2RayServer *> (sr);
			if (srv != nullptr) {
				ec("plugin", "V2Ray");
				ec("mode", srv->mode);
				ec("host", srv->v2host);
				ec("path", srv->path);
				ec("isTLS", srv->isTLS);
			}
		}*/
		cout << "------------------------------------------\n";
	}
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::ListTun(ParamsReader & arg) {
	String cmd = arg.read("Task");
	if (HELP_COND) {
		cout << "Show list of tuns for task";
		return;
	}
	_Task * t = ctrl.getConfig().findTaskByName(cmd);
	if (t == nullptr) {
		cout << "Unknow task\n";
		return;
	}
	for (const _Tun & tun : t->tuns) {
		cout << tun.localPort << "(" << (tun.type == TunType::TCP ? "tcp" : "udp") << ") => " << tun.remoteHost << ":" << tun.remotePort << "\n";
	}
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::ListVPNMode(ParamsReader & arg) {
	cout
		 << std::setw(10) << "Name"
		 << std::setw(15) << "Tun Name"
		 << std::setw(20) << "Default Route"
		 << std::setw(5) << "Remove"
		 << std::setw(10) << "DNS"
		 << std::setw(17) << "Ignore IP\n";

	for (const Tun2SocksConfig & conf : ctrl.getConfig().tun2socksConf) {
		cout
			 << std::setw(10) << conf.name
			 << std::setw(15) << conf.tunName
			 << std::setw(20) << conf.defaultRoute
			 << std::setw(5) << conf.removeDefaultRoute << "\n";
			 //<< std::setw(17) << "DN
			 //<< std::setw(17)
		for (const String & d: conf.dns)
			cout << std::setw(60) << d << "\n";
		for (const String & ip : conf.ignoreIP)
			cout << std::setw(77) << ip << "\n";
	}
}

#undef ec

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::ListRunning(ParamsReader & arg) {
	auto list = ctrl.getRunning();
	for (const auto & it : list) {
		const _Task * task = it.second->getTask();
		const _Server * server = it.second->getServer();
		cout << task->name << "\tStart on " << task->localHost << ":" << task->localPort << " => " << server->host << ":" << server->port << "\n";
	}
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::List(ParamsReader & arg) {
	String cmd = arg.read("type");
	if (HELP_COND) {
		cout << "Supported type:\n\ttasks\n\tservers\n\ttuns\n\tvpn\n\tvariables\nrun\n";
		return;
	}
	if (cmd == "servers")
		ListServers(arg);
	if (cmd == "tasks")
		ListTasks(arg);
	if (cmd == "tuns")
		ListTun(arg);
	if (cmd == "vpn")
		ListVPNMode(arg);
	if (cmd == "variables")
		ListVariables(arg);
	if (cmd == "run")
		ListRunning(arg);
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::ListVariables(ParamsReader & arg) {
	const auto & cn = ctrl.getConfig().variables;
	for (const auto & it : cn)
		cout << it.first << "\t-\t" << it.second << "\n";
}

/*
template <typename IStream, typename OStream>
void ConsoleLoop(IStream & in, OStream & out) {
	if ((!this->noStart ) && (this->autostart || settings.autostart)) {
		makeStartServers();
	}

	bool whileCond = true;
	auto quitFunc = [&whileCond](const Vector<String> &) {
		whileCond = false;
	};

	auto setFunc = [this] (const Vector<String> & arg) {
		size_t argpos = 0;
		String name;
		readVal("Name", name);
		if (name == "port")
			readValType("Port", settings.defaultPort, int);
		if (name == "host")
			readVal("Host", settings.defaultHost);
		if (name == "ss")
			readVal("ShadowSocksPath", settings.shadowSocksPath);
		if (name == "v2ray")
			readVal("V2Ray Path", settings.v2rayPluginPath);
		if (name == "autostart")
			readValType("Autostart", settings.autostart, bool);
		this->SaveConfig();
	};
	auto setPortFunc = [this] (const Vector<String> &arg) {
		size_t argpos = 0;
		int id;
		readValType("Task ID", id, int);
		int port;
		readValType("Port", port, int);
		for (_Task * tk : settings.tasks)
			if (tk->id == id) {
				tk->localPort = port;
				this->SaveConfig();
			}
	};
	auto setHostFunc = [this] (const Vector<String> &arg) {
		size_t argpos = 0;
		int id;
		readValType("Task ID", id, int);
		String host = "";
		readVal("Host", host);
		for (_Task * tk : settings.tasks)
			if (tk->id == id) {
				tk->localHost = host;
				this->SaveConfig();
			}
	};



#undef ec
}
*/
