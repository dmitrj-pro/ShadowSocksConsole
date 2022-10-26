#pragma once
#include <DPLib.conf.h>
//#include <Parser/SmartParser.h>
#include "ShadowSocksMain.h"
#include "ShadowSocksController.h"
#include <Converter/Converter.h>
#include <_Driver/Path.h>
#include <Network/TCPClient.h>
#include <Network/Utils.h>
#include <iomanip>
#include <_Driver/ServiceMain.h>
#include "VERSION.h"
#include <_Driver/Files.h>
#include <Addon/WGetDownloader.h>
#include <Network/Utils.h>
#include <Addon/LiteralReader.h>

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
using __DP_LIB_NAMESPACE__::trim;
using __DP_LIB_NAMESPACE__::toString;
using __DP_LIB_NAMESPACE__::Path;

using __DP_LIB_NAMESPACE__::startWithN;

template <typename OutStream, typename InStream>
class ParamsReaderTemplate {
	private:
		Vector<String> params;
		unsigned int pos = 0;
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

unsigned long fileSize(const String & file);

#define HELP_COND cmd == "-h" || cmd == "--help" || cmd == "--h" || cmd == "-help" || cmd == "help"

template <typename OutStream, typename InStream>
class ConsoleLooper : public ShadowSocksControllerUpdateStatus{
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
		void ListRun(ParamsReader & arg);
		void ListServers(ParamsReader & arg);
		void ListVPNMode(ParamsReader & arg);
		void ListVariables(ParamsReader & arg);
		void ListRunning(ParamsReader & arg);
		void List(ParamsReader & arg);

		void EditServer(ParamsReader & arg);
		void EditTask(ParamsReader & arg);
		void EditRun(ParamsReader & arg);
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

		void ShowSettings(ParamsReader & arg);
		void ShowServer(ParamsReader & arg);
		void ShowVPN(ParamsReader & arg);
		void ShowTask(ParamsReader & arg);
		void ShowRun(ParamsReader & arg);
		void Show(ParamsReader & arg);

		inline void MakeExit(ParamsReader &) { is_exit = true; }

		void Help(ParamsReader & arg);

		void VERSION(ParamsReader & arg);
	public:
		ConsoleLooper(OutStream & o, InStream & i) : cout (o), cin(i), ctrl(ShadowSocksController::Get()) {}
		void Load();
		void Loop();
		void ManualCMD(const String & cmd);
		inline void MakeExit() { is_exit = true; ctrl.MakeExit(); }
		bool SetPassword(const String & password);

		virtual void UpdateServerStatus(const String & server, const String & msg) override {
			_Server * srv = ShadowSocksController::Get().getConfig().findServerByName(server);
			if (srv == nullptr)
				return;
			cout << "New status server " << srv->name << " " << ( srv->check_result.isRun ?
																	  ( "ip " + srv->check_result.ipAddr +
																			(srv->check_result.speed_s.size() > 0 ?
																				 ( " speed "  + srv->check_result.speed_s )
																			   :
																				 ""))
																	: msg) << "\n";
		}
		virtual void UpdateTaskStatus(const String & server, const String & msg) override {
			cout << "New status task " << server << " " << msg << "\n";
		}

};

template <typename OutStream, typename InStream>
ConsoleLooper<OutStream, InStream> * makeLooper(OutStream & out, InStream & in) {
	auto res = new ConsoleLooper<OutStream, InStream>(out, in);
	res->Load();
	return res;
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::Help(ParamsReader &) {
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
	funcs["show"] = &ConsoleLooper::Show;
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::VERSION(ParamsReader &) {
	cout << "ShadowSocks Console v" << SS_HEAD_VERSION << "-" << SS_VERSION << " (" << SS_VERSION_HASHE << ")\n";
	cout << "Base on DPLib V" << __DP_LIB_NAMESPACE__::VERSION() << "\n";
}



template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::ServerDisconnect(ParamsReader &) {
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
			cout << "\t\tname:String\n"
				 << "\t\tenable_ipv6:Bool\n"
				 << "\t\tpassword:String\n"
				 << "\t\tmethod:String(aes128, aes256, chacha20_poly1305)\n"
				 << "\t\tservers:Array<String>\n"
				 << "\t\trun:String(default=DEFAULT)\n"
				 << "\n";
		cout << "\tParametr - run";
			cout << "\t\tvpn:String(default=none)\n"
				 << "\t\thttp_port:int\n"
				 << "\t\tsys_proxy:Bool\n"
				 << "\t\tsocks_port:int\n"
				 << "\t\tlocahost:String\n\n";
		cout << "\tParametr - settings\n";
			cout << "\t\tport:UInt\n"
				 << "\t\thport:UInt\n"
				 << "\t\tlisten_host:String\n"
				 << "\t\tautostart:Bool\n"
				 << "\t\tss_path:String\n"
				 << "\t\tv2ray_path:String\n"
				 << "\t\ttun2socks_path:String\n"
				 << "\t\tdns2socks_path:String\n"
				 << "\t\twget_path:String\n"
				 << "\t\ttmp_path:String\n"
				 << "\t\tenable_logging:Bool\n"
				 << "\t\thide_dns2socks:Bool\n"
				 << "\t\tudp_timeout:UInt\n"
				 << "\t\tignore_check:Bool\n"
				 << "\t\tbootstrap_dns:String\n"
				 << "\t\tauto_check_servers: Off, Ip, Speed\n"
				 << "\t\tauto_check_interval_s: UInt\n"
				 << "\t\tauto_check_server_ip_url:String\n"
				 << "\t\tauto_check_download_url:String\n"
				 ;
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
			_reader = new LiteralReader(cmd, delimers, false);
			cmd = "";
		} else
			_reader = new LiteralReader(arg.read("next"), delimers, false);
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

					LiteralReader reade(value, delimers, false);
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
					LiteralReader reade(value, delimers, false);
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
			if (type == "run") {
				_RunParams conf = ctrl.getConfig().findRunParamsbyName(name);

				if (parametr == "vpn") {
					if (value != "none")
						conf.tun2SocksName = value;
					else
						conf.tun2SocksName = "";
					cout << "tun2SocksName = " << conf.tun2SocksName << "\n";
				}
				if (conf.tun2SocksName.size() == 0) {
					MiCro_t("http_port", httpProxy, int);
					if (conf.httpProxy > 0) {
						MiCro_t("sys_proxy", systemProxy, bool);
					} else
						conf.systemProxy = false;
				} else {
					conf.httpProxy = -1;
					conf.systemProxy = false;
				}
				MiCro_t("socks_port", localPort, int);
				MiCro_s("localhost", localHost);
				MiCro_t("multimode", multimode, bool);

				_RunParams conf2 = ctrl.getConfig().findRunParamsbyName(name);
				ctrl.getConfig().deleteRunParamsByName(conf.name);
				if (!ctrl.getConfig().IsCorrect(conf)) {
					ctrl.getConfig().runParams.push_back(conf2);
				} else
					ctrl.getConfig().runParams.push_back(conf);
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

					LiteralReader reade(value, delimers, false);
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
				if (parametr == "run") {
					if (value != "none")
						cf->runParamsName = value;
					else
						cf->runParamsName = "DEFAULT";
					cout << "runParams = " << cf->runParamsName << "\n";
				}
				MiCro2_t("enable_ipv6", enable_ipv6, bool);
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
				#undef SaveSRV2
			}
			if (type == "settings") {
				cout << "Edit settings ";
				auto & conf = ctrl.getConfig();

				if (parametr == "autostart") {
					conf.autostart = str_to_AutoStartMode(value);
					cout << "autostart" << " = " << AutoStartMode_to_str(conf.autostart) <<"\n";
				}
				MiCro_t("countRestartAutostarted", countRestartAutostarted, unsigned int);
				MiCro_s("ss_path", shadowSocksPath);
				MiCro_s("v2ray_path", v2rayPluginPath);
				MiCro_s("tun2socks_path", tun2socksPath);

				MiCro_s("wget_path", wgetPath);

				MiCro_t("enable_logging", enableLogging, bool);
				MiCro_t("udp_timeout", udpTimeout, UInt);
				if (parametr == "checkServerMode") {
					conf.checkServerMode = str_to_ServerCheckingMode(value);
					cout << "autostart" << " = " << ServerCheckingMode_to_str(conf.checkServerMode) <<"\n";
				}
				MiCro_s("bootstrap_dns", bootstrapDNS);

				if (parametr == "auto_check_servers") {
					conf.auto_check_mode = str_to_AutoCheckingMode(value);
					cout << "auto_check_mode" << " = " << AutoCheckingMode_to_str(conf.auto_check_mode) <<"\n";
				}
				MiCro_t("auto_check_interval_s", auto_check_interval_s, unsigned int);
				MiCro_s("auto_check_server_ip_url", auto_check_ip_url);
				MiCro_s("auto_check_download_url", auto_check_download_url);




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
	tx = arg.read("Group", sr->group);
	if (tx.size() > 1) sr->group = tx;
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
void ConsoleLooper<OutStream, InStream>::EditRun(ParamsReader & arg) {
	String cmd = arg.read("Name");
	if (HELP_COND) {
		cout << "edit task ${Name} ${NewName} ${Password} ${Method} ${Servers:array<int>}\n";
		return;
	}
	_RunParams params = ctrl.getConfig().findRunParamsbyName(cmd);
	if (params.isNull) {
		cout << "Run params is not found";
		return;
	}

	String tx = arg.read("VPN Mode Name");
	if (tx.size() > 0 ) {
		if (tx != "none")
			params.tun2SocksName = tx;
		else
			params.tun2SocksName = "";
	}
	if (params.tun2SocksName.size() == 0) {
		tx = arg.read("Http Proxy Port", toString(params.httpProxy));
		if (tx.size() > 0) params.httpProxy = parse<int>(tx);
		if (params.httpProxy > 0) {
			tx = arg.read("System proxy? (0/1)", toString(params.systemProxy));
			if (tx.size() > 0) params.systemProxy = parse<bool>(tx);
		} else {
			params.systemProxy = false;
		}
	} else {
		params.httpProxy = -1;
		params.systemProxy = false;
	}
	tx = arg.read("Enable multi connections? (0/1)", toString(params.multimode));
	if (tx.size() > 0) params.multimode = parse<bool>(tx);

	tx = arg.read("Socks Port (-1 - use default)", toString(params.localPort));
	if (tx.size() > 1) params.localPort = parse<int>(tx);
	tx = arg.read("Socks local host (none or '' - use default)", params.localHost);
	if (tx.size() > 1) {
		if (tx == "none")
			params.localHost = "";
		else
			params.localHost = tx;
	}

	_RunParams conf2 = ctrl.getConfig().findRunParamsbyName(params.name);
	ctrl.getConfig().deleteRunParamsByName(params.name);
	if (!ctrl.getConfig().IsCorrect(params)) {
		ctrl.getConfig().runParams.push_back(conf2);
	} else
		ctrl.getConfig().runParams.push_back(params);

	ctrl.SaveConfig();
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
	tx = arg.read("Group", tk->group);
	if (tx.size() > 1) tk->group = tx;
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
	tx = arg.read("Run Parametr Name");
	if (tx.size() > 0 ) {
		if (tx != "none")
			tk->runParamsName = tx;
		else
			tk->runParamsName = "DEFAULT";
	}
	tx = arg.read("Enable IPv6 (for mobile)? (0/1)", toString(tk->enable_ipv6));
	if (tx.size() > 0) tk->enable_ipv6 = parse<bool>(tx);

	tx = arg.read("Enable UDP Support", toString(tk->enable_udp));
	if (tx.size() > 0) tk->enable_udp = parse<bool>(tx);

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
void ConsoleLooper<OutStream, InStream>::GetSource(ParamsReader & arg) {
	if (!arg.isEmpty()) {
		String cmd = arg.read("config");
		if (HELP_COND) {
			cout << "SOURCE boot\n";//
			cout << "SOURCE patch\n";//
			cout << "SOURCE\n";
			return;
		}
		if (cmd == "boot") {
			Path p {getCacheDirectory()};
			p.Append("boot.conf");
			if (!p.IsFile())
				return;
			String password = p.Get() + SS_VERSION_HASHE;
			cout << ctrl.GetSourceConfig(p.Get(), password);
			return;
		}
		if (cmd == "patch") {
			Path p {ctrl.GetConfigPath() + ".patch"};
			if (!p.IsFile())
				return;
			cout << ctrl.GetSourceConfig(p.Get(), ctrl.GetPassword());
			return;
		}

	}
	cout << ctrl.GetSourceConfig() << "\n";
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::ShowSettings(ParamsReader &) {
	auto & conf = ctrl.getConfig();
	#define SHOWSETTINGS(NAME, VAL) cout << std::setw(20) << NAME << std::setw(20) << VAL << "\n"
	SHOWSETTINGS("Name", "Value");
	cout << "----------------------------\n";
	SHOWSETTINGS("Enable autostart", AutoStartMode_to_str(conf.autostart));
	SHOWSETTINGS("Count of try restart task on autostart", conf.countRestartAutostarted);
	SHOWSETTINGS("ShadowSocks Path", conf.shadowSocksPath);
	SHOWSETTINGS("ShadowSocksRust Path", conf.shadowSocksPathRust);
	SHOWSETTINGS("V2Ray Path", conf.v2rayPluginPath);
	SHOWSETTINGS("Tun2Socks path", conf.tun2socksPath);
	SHOWSETTINGS("WGet path", conf.wgetPath);
	SHOWSETTINGS("Enable logging file", conf.enableLogging);
	SHOWSETTINGS("Use system wget (Linux)", conf.fixLinuxWgetPath);
	SHOWSETTINGS("Auto detect Tap interface", conf.autoDetectTunInterface);
	SHOWSETTINGS("Udp Timeout (s)", conf.udpTimeout);
	SHOWSETTINGS("Web session timeout (m)", conf.web_session_timeout_m);
	SHOWSETTINGS("Web enable log page", conf.enable_log_page);
	SHOWSETTINGS("Web enable export page", conf.enable_export_page);
	SHOWSETTINGS("Web enable import page", conf.enable_import_page);
	SHOWSETTINGS("Web enable utils page", conf.enable_utils_page);
	SHOWSETTINGS("Web enable exit page", conf.enable_exit_page);
	SHOWSETTINGS("Servers checking mode", ServerCheckingMode_to_str(conf.checkServerMode));
	SHOWSETTINGS("Bootstrap DNS", conf.bootstrapDNS);
	SHOWSETTINGS("Auto check servers", AutoCheckingMode_to_str(conf.auto_check_mode));
	SHOWSETTINGS("Auto check server interval (s)", conf.auto_check_interval_s);
	SHOWSETTINGS("Auto check server ip url", conf.auto_check_ip_url);
	SHOWSETTINGS("Auto check server download url", conf.auto_check_download_url);

	#undef SHOWSETTINGS
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::EditSettings(ParamsReader & arg) {
	auto & conf = ctrl.getConfig();
	String cmd = arg.read("ShadowSocks Path", conf.shadowSocksPath);
	if (HELP_COND) {
		cout << "edit settings";//
		return;
	}
	if (cmd.size() > 1 ) conf.shadowSocksPath = cmd;

	cmd = arg.read("ShadowSocksRust Path", conf.shadowSocksPathRust);
	if (cmd.size() > 1) conf.shadowSocksPathRust = cmd;

	cmd = arg.read("V2Ray Path", conf.v2rayPluginPath);
	if (cmd.size() > 1) conf.v2rayPluginPath = cmd;

	cmd = arg.read("AutoStart", AutoStartMode_to_str(conf.autostart));
	if (cmd.size() > 0) conf.autostart = str_to_AutoStartMode(cmd);

	cmd = arg.read("countRestartAutostarted", toString(conf.countRestartAutostarted));
	if (cmd.size() > 0) conf.countRestartAutostarted = parse<UInt>(cmd);

	cmd = arg.read("Tun2Socks", toString(conf.tun2socksPath));
	if (cmd.size() > 0) conf.tun2socksPath = cmd;

	cmd = arg.read("WGetPath", conf.wgetPath);
	if (cmd.size() > 1) conf.wgetPath = cmd;

	cmd = arg.read("Bootstrap DNS", conf.bootstrapDNS);
	if (cmd.size() > 1) conf.bootstrapDNS = cmd;
	if (conf.bootstrapDNS.size() > 4)
		__DP_LIB_NAMESPACE__::global_config.setDNS(conf.bootstrapDNS);

	cmd = arg.read("Web session timeout (m)", toString(conf.web_session_timeout_m));
	if (cmd.size() > 0) conf.web_session_timeout_m = parse<UInt>(cmd);


	cmd = arg.read("Web enable log page", toString(conf.enable_log_page));
	if (cmd.size() > 0) conf.enable_log_page = parse<bool>(cmd);

	cmd = arg.read("Web enable import page", toString(conf.enable_import_page));
	if (cmd.size() > 0) conf.enable_import_page = parse<bool>(cmd);

	cmd = arg.read("Web enable export page", toString(conf.enable_export_page));
	if (cmd.size() > 0) conf.enable_export_page = parse<bool>(cmd);

	cmd = arg.read("Web enable utils page", toString(conf.enable_utils_page));
	if (cmd.size() > 0) conf.enable_utils_page = parse<bool>(cmd);

	cmd = arg.read("Web enable exit page", toString(conf.enable_exit_page));
	if (cmd.size() > 0) conf.enable_exit_page = parse<bool>(cmd);

	cmd = arg.read("Udp Timeout (s)", toString(conf.udpTimeout));
	if (cmd.size() > 0) conf.udpTimeout = parse<UInt>(cmd);

	cmd = arg.read("Servers checking mode (off, tcp, deep)", ServerCheckingMode_to_str(conf.checkServerMode));
	if (cmd.size() > 0) conf.checkServerMode = str_to_ServerCheckingMode(cmd);

	cmd = arg.read("Use system wget (Linux)", toString(conf.fixLinuxWgetPath));
	if (cmd.size() > 0) conf.fixLinuxWgetPath = parse<bool>(cmd);

	cmd = arg.read("Auto detect Tap interface", toString(conf.autoDetectTunInterface));
	if (cmd.size() > 0) conf.autoDetectTunInterface = parse<bool>(cmd);

	cmd = arg.read("Auto check servers (Off, Ip, Speed)", AutoCheckingMode_to_str(conf.auto_check_mode));
	if (cmd.size() > 0) conf.auto_check_mode = str_to_AutoCheckingMode(cmd);

	cmd = arg.read("Auto check server interval (s)", toString(conf.auto_check_interval_s));
	if (cmd.size() > 0) conf.auto_check_interval_s = parse<unsigned int>(cmd);

	cmd = arg.read("Auto check server ip url", conf.auto_check_ip_url);
	if (cmd.size() > 1) conf.auto_check_ip_url = cmd;

	cmd = arg.read("Auto check server download url", conf.auto_check_download_url);
	if (cmd.size() > 1) conf.auto_check_download_url = cmd;

	cmd = arg.read("Enable logging file", toString(conf.enableLogging));
	if (cmd.size() > 0) {
		conf.enableLogging = parse<bool>(cmd);
		if (conf.enableLogging) {
			ctrl.OpenLogFile();
		}
	}

	ctrl.SaveConfig();
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::Edit(ParamsReader & arg) {
	String cmd = arg.read("type");
	if (HELP_COND) {
		cout << "Supported type:\n\ttask\n\tserver\n\tsettings\n\tvpn\n\trun";
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
	if (cmd == "run") {
		EditRun(arg);
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

	if (__DP_LIB_NAMESPACE__::TCPClient::IsCanConnect(cmd, parse<unsigned int>(arg.read("port"))))
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
	auto list = __DP_LIB_NAMESPACE__::resolveDomainList(cmd);
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
		this->ctrl.getConfig().autostart = AutoStartMode::On;
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
		this->ctrl.getConfig().autostart = AutoStartMode::Off;
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
	tx = arg.read("Push default routing (0/1)", toString(conf.enableDefaultRouting));
	if (tx.size() > 0) conf.enableDefaultRouting = parse<bool>(tx);
	if (ShadowSocksSettings::enablePreStartStopScripts) {
		tx = arg.read("Run command after start VPN", conf.postStartCommand);
		if (tx.size() > 1) conf.postStartCommand = tx;
		tx = arg.read("Run command before stop VPN", conf.preStopCommand);
		if (tx.size() > 1) conf.preStopCommand = tx;
	}

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
	conf.enableDefaultRouting = arg.readbool("Push default routing (0/1)");
	conf.postStartCommand = arg.read("Run command after start VPN");
	conf.preStopCommand = arg.read("Run command before stop VPN");
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

void calcSizeAndText(double origin, double & res, String & res_s);

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::CheckSpeedTest(ParamsReader & arg, bool enable_speed) {
	_ShadowSocksController::CheckLoopStruct args = ctrl.makeCheckStruct();
	args._server_name = "";
	args._task_name = "";
	args.auto_check_interval_s = 0;
	args.auto_check_mode = enable_speed ? AutoCheckingMode::Speed : AutoCheckingMode::Ip;


	while (!arg.isEmpty()) {
		String cmd = arg.read("");
		if (cmd == "MYIP") {
			cout << "Set check ip link to api.my-ip.io\n";
			args.checkIpUrl = "https://api4.my-ip.io/ip";
		}
		if (startWithN(cmd, "LINK=")) {
			args.downloadUrl = cmd.substr(String("LINK=").size());
			cout << "Set speed test link: " << args.downloadUrl << "\n";
		}
		if (startWithN(cmd, "CHECKIP=")) {
			args.checkIpUrl = cmd.substr(String("CHECKIP=").size());
			cout << "Set check ip link: " << args.checkIpUrl << "\n";
		}

		if (HELP_COND) {
			cout << "check speed ${FAST=FAST} LINK=${link:url}\n";
			cout << "LINK - set test speed link. If it use enable FAST\n";
			cout << "CHECKIP - set link to check ip. Default ipconfig.cc\n";
			cout << "MYIP - set link to check ip api4.my-ip.io\n\n";

			cout << "Check speed with https://speed.hetzner.de/100MB.bin and http://ipv4.download.thinkbroadband.com/512MB.zip\n";
			return;
		}
	}

	ctrl.check_loop(args);

	{
		if (enable_speed)
			cout
					<< std::setw (10) << "Server"
					<< std::setw(8) << "Status"
					<< std::setw(20) << "IP"
					<< std::setw(15) << "Speed 100"
					<< "    msg\n";
		else
			cout
					<< std::setw (10) << "Server"
					<< std::setw(8) << "Status"
					<< std::setw(20) << "IP"
					<< "    msg\n";
		for (_Server* it : ctrl.getConfig().servers) {
			if (enable_speed)
				cout
						<< std::setw (10) << it->name
						<< std::setw(8) << (it->check_result.isRun ? "OK" : "FAIL")
						<< std::setw(20) << it->check_result.ipAddr
						<< std::setw(15) << (toString(it->check_result.speed) + " " + it->check_result.speed_s)
						<< "    " << it->check_result.msg << "\n";
			else
				cout
						<< std::setw (10) << it->name
						<< std::setw(8) << (it->check_result.isRun ? "OK" : "FAIL")
						<< std::setw(20) << it->check_result.ipAddr
						<< "    " << it->check_result.msg << "\n";
		}
	}
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::StartTask(ParamsReader & arg){
	String cmd = arg.read("Task");
	if (HELP_COND) {
		cout << "Start task\n";
		cout << "Use: start ${TaskName} ${check:null<string(NOCHECK)>} ${novpn:null<string(NOVPN)>} ${socks_port:null<string(SPORT=${port})>} ${server:null<string(SERVER=${servername})>} ${http_port:null<string(HPORT=${http port})>} ${enable_sysproxy:null<string(SYSPROXY)>}\n";
		return;
	}
	_Task * t = ctrl.getConfig().findTaskByName(cmd);
	if (t == nullptr) {
		cout << "Unknow task\n";
		return;
	}
	SSClientFlags flags;
	flags.checkServerMode = ctrl.getConfig().checkServerMode;
	while (!arg.isEmpty()) {
		String flag = arg.read("");
		if (flag == "NOCHECK")
			flags.checkServerMode = ServerCheckingMode::Off;
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
		if (__DP_LIB_NAMESPACE__::startWith(flag, "HPORT=")) {
			String port = flag.substr(String("HPORT=").size());
			flags.http_port = parse<unsigned short>(port);
		}
		if (__DP_LIB_NAMESPACE__::startWith(flag, "SYSPROXY")) {
			flags.sysproxy_s = SSClientFlagsBoolStatus::True;
		}

	}
	auto funcCrash = [this, cmd] (const String & name, const ExitStatus & status) {
		cout << "Fail to start task " << name << ": " << status.str << "\n";
		ctrl.StopByName(cmd);
	};

	ctrl.StartByName(cmd, [this] ( const String & name) {
		cout << "Task " << name << " succesfully started\n";
	}, funcCrash, flags);
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
	for (unsigned int i = 0; i < cmd.size(); i++) {
		if (cmd[i] == ' ' || cmd[i] == '\t' || cmd[i] == '\n') {
			String t = cmd.substr(prev, i-prev);
			prev = i+1;
			if (c.size() == 0)
				c = t;
			else
				args.AddParams(t);

		}
	}
	DP_LOG_DEBUG << "Command '" << c <<"'";
	if (__DP_LIB_NAMESPACE__::ConteinsKey(funcs, c)) {
		auto func = funcs[c];
		(this->*func)(args);
	} else {
		DP_LOG_WARNING << "Unknow comand '" << c << "'";
		throw EXCEPTION("Unknow comand '" + c + "'");
	}
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::Loop() {
	unsigned int _prev_servers = ctrl.getConfig().servers.size();
	if (! ctrl.isCreated() || (ctrl.isCreated() && ctrl.isEncrypted())) {
		while (!cin.eof()) {
			cout << "Write password:";
			cout.flush();
			String password;
			getline(cin, password);
			if (__DP_LIB_NAMESPACE__::ServiceSinglton::Get().NeedToExit())
				return;
			password = __DP_LIB_NAMESPACE__::trim(password);
			DP_LOG_TRACE << "Input password: '" << password << "'\n";

			if (SetPassword(password)) {
				DP_LOG_INFO << "Config parsed\n";
				break;
			}
		}
	} else {
		ctrl.OpenConfig();
		DP_LOG_WARNING << "Config is not encrypted\n";
	}
	if (cin.eof() || __DP_LIB_NAMESPACE__::ServiceSinglton::Get().NeedToExit())
		return;

	if (ctrl.CheckInstall()) {
		auto funcCrash = [this] (const String & name, const ExitStatus & status) {
			__DP_LIB_NAMESPACE__::OStrStream error;
			error << "Fail to auto start task " << name << ": " << status.str;
			DP_LOG_FATAL << error.str();
			cout << error.str() << "\n";
		};
		if (_prev_servers != ctrl.getConfig().servers.size())
			ctrl.AutoStart(funcCrash);
		ShadowSocksController::Get().startCheckerThread();
	} else {
		DP_LOG_FATAL << "Fail install. Try exit and enter new password. Or reinstall application";
		cout << "Fail install. Try exit and enter new password. Or reinstall application\n";
	}

	is_exit = false;
	cout << "Write help to get help\n";
	DP_LOG_DEBUG << "Started command loop";
	ShadowSocksController::Get().connectNotify(this);
	while (!is_exit && (!__DP_LIB_NAMESPACE__::ServiceSinglton::Get().NeedToExit()) && (!cin.eof())) {
		cout << "> ";
		cout.flush();
		String cmd;
		getline(cin, cmd);
		DP_LOG_DEBUG << "Input command: '" << cmd << "'\n";
		try {
			ManualCMD(cmd);
		} catch (StructLogout e) {
			DP_LOG_INFO << "User Logout\n";
			break;
		} catch (__DP_LIB_NAMESPACE__::LineException e) {
			cout << e.message() << "\nUse help\n";
		}
	}
	ShadowSocksController::Get().disconnectNotify(this);
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
	sr->group = arg.read("Group");
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
	tk->group = arg.read("Group");
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
	else if (cmd == "servers")
		CheckSpeedTest(arg, false);
	else if (cmd == "speed")
		CheckSpeedTest(arg, true);
	else
		cout << "Unknow command";
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
	unsigned int lp = arg.readUInt("Local Port");
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
	if (!tmp.IsAbsolute()) {
		Path p = Path(getWritebleDirectory());
		p.Append(tmp.Get());
		tmp = p;
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
	ctrl.ExportConfig(out, isMobile, is_resolve, v2ray);
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
void ConsoleLooper<OutStream, InStream>::Show(ParamsReader & arg) {
	String cmd = arg.read("type");
	if (HELP_COND) {
		cout << "Support:\n\tsettings - Show current settings\n";
		cout << "\ttask\n";
		cout << "\tserver\n";
		cout << "\tvpn\n";
		cout << "\trun\n";
		return;
	}
	if (cmd == "task") {
		ShowTask(arg);
		return;
	}
	if (cmd == "server") {
		ShowServer(arg);
		return;
	}
	if (cmd == "vpn") {
		ShowVPN(arg);
		return;
	}
	if (cmd == "settings") {
		ShowSettings(arg);
		return;
	}
	if (cmd == "run") {
		ShowRun(arg);
		return;
	}
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
void ConsoleLooper<OutStream, InStream>::ListTasks(ParamsReader &) {
	const auto & srvs = ctrl.getConfig().tasks;
	for (const _Task * sr : srvs) {
		ec("id", sr->id);
		ec("name", sr->name);
		ec("group", sr->group);
		ec("method", sr->method);
		ec("RunParams", sr->runParamsName)
		cout << "Servers: ";
		for (int id : sr->servers_id)
			cout << ctrl.getConfig().findServerById(id)->name << " (" << id << ") , ";
		cout << "\n------------------------------------------\n";
	}
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::ListRun(ParamsReader &) {
	const auto & srvs = ctrl.getConfig().runParams;
	for (const _RunParams & sr : srvs) {
		ec("name", sr.name);
		ec("local host", sr.localHost);
		ec("Enable multi connections:", sr.multimode);
		ec("local port", sr.localPort);
		ec("local http port", sr.httpProxy);
		ec("enable sys proxy", sr.systemProxy);
		ec("vpn", sr.tun2SocksName)
		cout << "\n------------------------------------------\n";
	}
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::ShowTask(ParamsReader & arg) {
	String cmd = arg.read("Name");
	if (HELP_COND) {
		cout << "show task ${Name}\n";
		return;
	}
	_Task * sr = ctrl.getConfig().findTaskByName(cmd);
	if (sr == nullptr) {
		cout << "Task is not found";
		return;
	}

	{
		ec("id", sr->id);
		ec("name", sr->name);
		ec("group", sr->group);
		ec("method", sr->method);
		ec("password", sr->password);
		ec("Enable IPv6", (sr->enable_ipv6 ? "True" : "False"));
		ec("Run Params", sr->runParamsName)
		cout << "Servers: ";
		for (int id : sr->servers_id)
			cout << ctrl.getConfig().findServerById(id)->name << " (" << id << ") , ";
		cout << "\n------------------------------------------\n";
	}
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::ShowRun(ParamsReader & arg) {
	String cmd = arg.read("Name");
	if (HELP_COND) {
		cout << "show run ${Name}\n";
		return;
	}
	_RunParams p = ctrl.getConfig().findRunParamsbyName(cmd);
	if (p.isNull) {
		cout << "Run Parametrs is not found";
		return;
	}

	{
		ec("name", p.name);
		ec("local host", p.localHost);
		ec("local port", p.localPort);
		ec("Http Proxy Port", p.httpProxy);
		ec("System proxy", (p.systemProxy ? "True" : "False"));
		ec("vpn", p.tun2SocksName)
		cout << "\n------------------------------------------\n";
	}
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::ListServers(ParamsReader &) {
	cout
		 << std::setw(5) << "ID"
		 << std::setw(15) << "Name"
		 << std::setw(15) << "Group"
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
			 << std::setw(15) << sr->group
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
void ConsoleLooper<OutStream, InStream>::ShowServer(ParamsReader & arg) {
	String cmd = arg.read("name");
	if (HELP_COND) {
		cout << "show server ${NAME}\n";
		return;
	}
	_Server * sr = ctrl.getConfig().findServerByName(cmd);
	if (sr == nullptr) {
		cout << "Server is not found";
		return;
	}

	cout
		 << std::setw(5) << "ID"
		 << std::setw(15) << "Name"
		 << std::setw(15) << "Group"
		 << std::setw(30) << "Host:Port"
		 << std::setw(10) << "Plugin"
		 << std::setw(10) << "Mode"
		 << std::setw(7) << "isTls"
		 << std::setw(30) << "Host/Path\n";
	{
		cout
			 << std::setw(5) << sr->id
			 << std::setw(15) << sr->name
			 << std::setw(15) << sr->group
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
void ConsoleLooper<OutStream, InStream>::ListVPNMode(ParamsReader &) {
	cout
		 << std::setw(10) << "Name"
		 << std::setw(15) << "Tun Name"
		 << std::setw(20) << "Default Route"
		 << std::setw(5) << "Remove"
		 << std::setw(5) << "is default"
		 << std::setw(10) << "DNS"
		 << std::setw(17) << "Ignore IP\n";

	for (const Tun2SocksConfig & conf : ctrl.getConfig().tun2socksConf) {
		cout
			 << std::setw(10) << conf.name
			 << std::setw(15) << conf.tunName
			 << std::setw(20) << conf.defaultRoute
			 << std::setw(5) << conf.removeDefaultRoute << std::setw(5) << conf.enableDefaultRouting << "\n";
			 //<< std::setw(17) << "DN
			 //<< std::setw(17)
		for (const String & d: conf.dns)
			cout << std::setw(60) << d << "\n";
		for (const String & ip : conf.ignoreIP)
			cout << std::setw(77) << ip << "\n";
	}
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::ShowVPN(ParamsReader & arg) {
	String cmd = arg.read("Name");
	if (HELP_COND) {
		cout << "show vpn ${Name}\n";
		return;
	}

	Tun2SocksConfig conf = ctrl.getConfig().findVPNbyName(cmd);
	if (conf.isNull) {
		cout << "VPN is not found\n";
		return;
	}

	cout
		 << std::setw(10) << "Name"
		 << std::setw(15) << "Tun Name"
		 << std::setw(20) << "Default Route"
		 << std::setw(5) << "Remove"
		 << std::setw(5) << "is default"
		 << std::setw(10) << "DNS"
		 << std::setw(17) << "Ignore IP\n";

	{
		cout
			 << std::setw(10) << conf.name
			 << std::setw(15) << conf.tunName
			 << std::setw(20) << conf.defaultRoute
			 << std::setw(5) << conf.removeDefaultRoute << std::setw(5) << conf.enableDefaultRouting << "\n";
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
void ConsoleLooper<OutStream, InStream>::ListRunning(ParamsReader &) {
	auto list = ctrl.getRunning();
	for (const auto & it : list) {
		const _Task * task = it.second->getTask();
		const _Server * server = it.second->getServer();
		const _RunParams & p = it.second->getRunParams();
		cout << task->name << "\tStart on " << p.localHost << ":" << p.localPort << " => " << server->host << ":" << server->port << "\n";
	}
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::List(ParamsReader & arg) {
	String cmd = arg.read("type");
	if (HELP_COND) {
		cout << "Supported type:\n\ttasks\n\tservers\n\ttuns\n\tvpn\n\tvariables\nrun\nruns\n";
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
		ListRun(arg);
	if (cmd == "runs")
		ListRunning(arg);
}

template <typename OutStream, typename InStream>
void ConsoleLooper<OutStream, InStream>::ListVariables(ParamsReader &) {
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
