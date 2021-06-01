#include <_Driver/ServiceMain.h>
#include "ShadowSocksMain.h"
#include <_Driver/Path.h>
#include <Parser/Setting.h>
#include <Crypt/Crypt.h>
#include <Converter/Converter.h>
#include <iostream>
#include "ShadowSocks.h"
#include "ConsoleLoop.h"
#include <Log/Log.h>
#include "ShadowSocksController.h"
#include <_Driver/Files.h>
#include <set>
#include "SSClient.h"
#include "SSServer.h"
#include "VERSION.h"
#include "TCPChecker.h"

#ifdef DP_WIN
	#include <windows.h>
#endif

#ifdef DP_QT_GUI
	#include <QApplication>
	#include "ShadowSocksWindow.h"
#endif

using __DP_LIB_NAMESPACE__::Path;
using __DP_LIB_NAMESPACE__::Ifstream;
using __DP_LIB_NAMESPACE__::Vector;
using __DP_LIB_NAMESPACE__::trim;
using __DP_LIB_NAMESPACE__::toString;
using __DP_LIB_NAMESPACE__::SmartParser;



#ifdef DP_WIN
std::string GetLastErrorAsString()
{
	//Get the error message, if any.
	DWORD errorMessageID = ::GetLastError();
	if(errorMessageID == 0)
		return std::string(); //No error message has been recorded

	LPSTR messageBuffer = nullptr;
	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
								 NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

	std::string message(messageBuffer, size);

	//Free the buffer.
	LocalFree(messageBuffer);

	return message;
}
#endif

class ShadowSocksMain: public __DP_LIB_NAMESPACE__::ServiceMain {
	private:
		#ifdef DP_QT_GUI
			QApplication * app = nullptr;
			ShadowSocksWindow * gui = nullptr;
		#endif
		bool reopenConsole = true;
		bool loggingEnabled = false;

	public:
		ShadowSocksMain():__DP_LIB_NAMESPACE__::ServiceMain("ShadowSocksConsole"){}

		void serviceMain() {
			//__DP_LIB_NAMESPACE__::log.OpenFile("LOGGING.txt");

			__DP_LIB_NAMESPACE__::log << getVersion() << "\n";

			__DP_LIB_NAMESPACE__::Ofstream out;
			Path p = Path(__DP_LIB_NAMESPACE__::ServiceSinglton::Get().GetPathToFile());
			p = Path(p.GetFolder());
			p.Append("__service__.txt");
			out.open(p.Get());
			out << "w";
			out.close();
			ShadowSocksServer * srv = new ShadowSocksServer();
			ShadowSocksController::Get().SetExitFinc([srv]() {
				srv->Stop();
			});

			srv->Start();

			__DP_LIB_NAMESPACE__::RemoveFile(p.Get());
		}
		void consoleMain();

		virtual void MainLoop() override {
			ShadowSocksController::Get().SetExitFinc(std::bind(&ShadowSocksMain::MainExit, this));

			if (isService()) {
				serviceMain();
				return;
			}
			#ifdef DP_QT_GUI_0
				int argc = GetArgc();
				app = new QApplication(argc, GetArgv());
				gui = new ShadowSocksWindow();
				gui->show();
				app->exec();
				return;
			#endif
			consoleMain();
		}
		void _MainExit0(){}
		virtual void MainExit() override {
			ShadowSocksController::Get().SetExitFinc(std::bind(&ShadowSocksMain::_MainExit0, this));
			ShadowSocksController::Get().MakeExit();
			#ifdef DP_QT_GUI
				if (app != nullptr) {
					gui->close();
					app->quit();
				}
			#endif
				//std::cin
		}

		inline void autoStart() {
			ShadowSocksController::Get().EnableAutoStart();
		}
		inline void disableStart() {
			ShadowSocksController::Get().DisableAutoStart();
		}
		inline void disableReopenConsole() { reopenConsole = false; }
		inline void setPassword(const String & password) {
			try{
				ShadowSocksController::Get().OpenConfig(password);
			} catch (__DP_LIB_NAMESPACE__::LineException e) {
				__DP_LIB_NAMESPACE__::log << "Fail set password:" << e.toString() << "\n";
			}
		}
		inline void onFailArg() {

		}
		inline void install() {
			#ifdef DP_LINUX
				List<String> args;
				if (loggingEnabled)
					args = {"log", "sexecute"};
				else
					args = {"sexecute"};
				__DP_LIB_NAMESPACE__::ServiceMain::install( args );
			#else
				__DP_LIB_NAMESPACE__::ServiceMain::install();
			#endif
			this->SetNeedToExit(true);
		}
		inline void uninstall() {
			__DP_LIB_NAMESPACE__::ServiceMain::uninstall();
			this->SetNeedToExit(true);
		}
		inline void start() {
			__DP_LIB_NAMESPACE__::ServiceMain::start();
			this->SetNeedToExit(true);
		}
		inline void stop() {
			__DP_LIB_NAMESPACE__::ServiceMain::stop();
			this->SetNeedToExit(true);
		}
		inline void asservice() {
			serviceMain();
			this->SetNeedToExit(true);
		}
		inline String getVersion() {
			__DP_LIB_NAMESPACE__::OStrStream out;
			out << "ShadowSocks Console v" << SS_HEAD_VERSION << "-" << SS_VERSION << " (" << SS_VERSION_HASHE << ")\n";
			out << "Base on DPLib V" << __DP_LIB_NAMESPACE__::VERSION() << "\n";
			return out.str();
		}
		inline void version() {
			std::cout << getVersion();
			this->SetNeedToExit(true);
		}

		inline void enableLogging() {
			__DP_LIB_NAMESPACE__::log.OpenFile("LOGGING.txt");
			loggingEnabled = true;
		}

		virtual void PreStart() override {
			srand(time(nullptr));
			ShadowSocksController::Create(new _ShadowSocksController());
			DP_SM_addArgumentHelp0(&ShadowSocksMain::autoStart, "Auto start tasks", "start", "-e", "--e", "-start", "--start");
			DP_SM_addArgumentHelp0(&ShadowSocksMain::disableStart, "Run without autostart", "savemode", "-s", "--s", "-savemode", "--savemode");
			DP_SM_addArgumentHelp0(&ShadowSocksMain::disableReopenConsole, "Run without start new console (Windows only)", "noconsole", "-noconsole", "--noconsole");
			DP_SM_addArgumentHelp1(String, &ShadowSocksMain::setPassword, &ShadowSocksMain::onFailArg, "Set password for decrypt config", "-p", "--p", "-password", "--password");
			DP_SM_addArgumentHelp0(&ShadowSocksMain::install, "Install as service", "install");
			DP_SM_addArgumentHelp0(&ShadowSocksMain::uninstall, "Uninstall service", "uninstall");
			DP_SM_addArgumentHelp0(&ShadowSocksMain::start, "Start service", "start");
			DP_SM_addArgumentHelp0(&ShadowSocksMain::stop, "Stop service", "stop");
			DP_SM_addArgumentHelp0(&ShadowSocksMain::asservice, "start as service", "sexecute");
			DP_SM_addArgumentHelp0(&ShadowSocksMain::version, "Show application version", "version");
			DP_SM_addArgumentHelp0(&ShadowSocksMain::enableLogging, "Enable logging", "log");

			AddHelp(std::cout, [this]() { this->SetNeedToExit(true); }, "help", "-help", "--help", "-h", "--h", "?");
		}
};

void ShadowSocksMain::consoleMain() {
	#ifdef DP_WIN
		if (reopenConsole) {
		if (!AllocConsole()) {
			__DP_LIB_NAMESPACE__::log << "Fail to create console: " << GetLastErrorAsString() << "\n";
			//return;
		} else {
			freopen("CONIN$", "r", stdin);
			//freopen("CONOUT$", "w", stderr);
			freopen("CONOUT$", "w", stdout);
		}
		}

	#endif

	Path p = Path(__DP_LIB_NAMESPACE__::ServiceSinglton::Get().GetPathToFile());
	p = Path(p.GetFolder());
	p.Append("__service__.txt");
	if (p.IsFile()) {
		ShadowSocksRemoteClient cl;
		cl.Start();
	} else {
		auto looper = makeLooper(std::cout, std::cin);
		looper->Loop();
	}
}


#define Read(X, Var) \
	if ( setting.Conteins(X)) { \
		Var = trim(setting.get(X)); \
	} else { \
		String tmp = "Paramet '"; \
		tmp += X; \
		tmp += "' is not found."; \
		throw EXCEPTION (tmp); \
	}

#define ReadType(X, Var, Type) \
	if ( setting.Conteins(X)) { \
		Var =  __DP_LIB_NAMESPACE__::parse<Type>( trim(setting.get(X))); \
	} else { \
		String tmp = "Paramet '"; \
		tmp += X; \
		tmp += "' is not found."; \
		throw EXCEPTION (tmp); \
	}

#define ReadN(X, Var) \
	if ( setting.Conteins(X)) \
		Var = trim(setting.get(X));

#define ReadNType(X, Var, Type) \
	if ( setting.Conteins(X)) \
		Var = __DP_LIB_NAMESPACE__::parse<Type>(trim(setting.get(X)));

#define Set(X, Var) \
	setting.add(X, Var);

#define SetType(X, Var) \
	setting.add(X, __DP_LIB_NAMESPACE__::toString(Var));

bool ShadowSocksSettings::CheckTask(_Task * t) {
	for (int id : t->servers_id) {
		bool c = false;
		for (_Server * sr : servers)
			if (id == sr->id)
				c = true;
		if (!c)
			return false;
	}
	for (_Task * tmp : this->tasks)
		if (tmp->name == t->name)
			return false;
	bool inited = false;
	for (const Tun2SocksConfig & tmp : this->tun2socksConf)
		if (tmp.name == t->tun2SocksName)
			inited = true;
	if (!inited && (t->tun2SocksName.size() > 0))
		return false;
	return t->Check();
}

bool ShadowSocksSettings::CheckServer(_Server * s) {
	for (_Server * sr: this->servers)
		if (sr->name == s->name)
			return false;
	return s->Check();
}

bool ShadowSocksSettings::CheckT2S(const Tun2SocksConfig & conf) {
	for (const Tun2SocksConfig & t : tun2socksConf)
		if (t.name == conf.name) {
			return false;
		}
	return true;
}

Tun2SocksConfig ShadowSocksSettings::findVPNbyName(const String & name) {
	for (const Tun2SocksConfig & t : tun2socksConf)
		if (t.name == name) {
			return t;
		}
	return Tun2SocksConfig();
}

_Task * ShadowSocksSettings::findTaskByName(const String & name) {
	for (_Task * tmp : this->tasks)
		if (tmp->name == name)
			return tmp;
	return nullptr;
}

_Task * ShadowSocksSettings::findTaskById(int id) {
	for (_Task * tmp : this->tasks)
		if (tmp->id == id)
			return tmp;
	return nullptr;
}

_Server * ShadowSocksSettings::findServerByName(const String & name) {
	for (_Server * sr: this->servers)
		if (sr->name == name)
			return sr;
	return nullptr;
}

_Server * ShadowSocksSettings::findServerById(int id) {
	for (_Server * sr: this->servers)
		if (sr->id == id)
			return sr;
	return nullptr;
}

void ShadowSocksSettings::deleteServerById(int id) {
	for (auto it = this->servers.begin(); it != this->servers.end(); it++)
		if ((*it)->id == id) {
			this->servers.erase(it);
			break;
		}
}

void ShadowSocksSettings::deleteTaskById(int id) {
	for (auto it = this->tasks.begin(); it != this->tasks.end(); it++)
		if ((*it)->id == id) {
			this->tasks.erase(it);
			break;
		}
}

void ShadowSocksSettings::deleteVPNByName(const String & name) {
	for (auto it = this->tun2socksConf.begin(); it != this->tun2socksConf.end(); it++)
		if ((*it).name == name) {
			this->tun2socksConf.erase(it);
			break;
		}
}

#ifdef DP_WIN
bool Is64bitWindows(void) noexcept {
	bool is_64_bit = true;
	if(FALSE == GetSystemWow64DirectoryW(nullptr, 0u)) {
		auto const last_error{::GetLastError()};
		if(ERROR_CALL_NOT_IMPLEMENTED == last_error)
			is_64_bit = false;
	}
	return is_64_bit;
}
#endif

String ShadowSocksSettings::replaceVariables(const String & src) const {
	SmartParser parser(src);
	for (const auto & it : variables)
		parser.SetAll(it.first, it.second);
	return parser.ToString();
}

String ShadowSocksSettings::replacePath(const String & path, bool is_dir) {
	__DP_LIB_NAMESPACE__::SmartParser parser(path);
	Path p = Path(__DP_LIB_NAMESPACE__::ServiceSinglton::Get().GetPathToFile());
	p = Path(p.GetFolder());
	#ifdef DP_WIN
		if (Is64bitWindows())
			p.Append("64");
		else
			p.Append("32");
	#else
		p.Append("modules");
	#endif
	parser.SetAll("INSTALLED", p.Get());
	String res = parser.ToString();
	#ifdef DP_WIN
		if (!is_dir)
			if (!(__DP_LIB_NAMESPACE__::endWithN(res, ".exe") || __DP_LIB_NAMESPACE__::endWithN(res, ".EXE")))
				res = res + ".exe";
	#endif
	return res;
}

int _Server::glob_id = 0;
int _Task::glob_id = 0;

String convertTime(UInt time) {
	String res = toString(time/60);
	if ( res.size() > 0)
		res += "m";
	String m = toString(time%60);
	if (m.size() == 1)
		m = "0" + m;
	if (m.size() > 0)
		res += m + "s";
	return res;
}

TCPChecker::TCPCheckerLoop TCP_CHECKER;

ShadowSocksClient * ShadowSocksSettings::makeServer(int id, const MakeServerFlags & flags) {
	_Task * tk = nullptr;

	// Находим таску
	for (_Task * t : tasks)
		if (t->id == id) {
			tk = t;
			break;
		}
	if (tk == nullptr) {
		DP_PRINT_TEXT("Task is not found.");
		throw EXCEPTION("Task is not found.");
	}

	// Находим настройки для VPN
	if (flags.vpn_name.size() > 0)
		tk->tun2SocksName = flags.vpn_name;
	Tun2SocksConfig conf;
	for (const Tun2SocksConfig & c : this->tun2socksConf)
		if (c.name == tk->tun2SocksName) {
			conf = c;
			break;
		}
	if (tk->tun2SocksName.size() > 0 && conf.isNull) {
		DP_PRINT_TEXT("VPN Config is not found.");
		throw EXCEPTION("VPN Config is not found.");
	}


	conf.hideDNS2Socks = hideDNS2Socks;

	// Находим все сервера, связанные с таской.
	List<_Server*> srvs;
	for (_Server * sr : servers)
		for (int idt : tk->servers_id)
			if (idt == sr->id) {
				if (flags.server_name.size() > 0) {
					if (flags.server_name == sr->name)
						srvs.push_back(sr);
				} else
					srvs.push_back(sr);
			}

	if (srvs.size() == 0) {
		DP_PRINT_TEXT("Servers is not linked");
		throw EXCEPTION("Servers is not linked");
	}

	// Находим первый "живой" сервер
	_Server * srv_res = TCP_CHECKER.Check(srvs, bootstrapDNS, IGNORECHECKSERVER, [this](const String & txt) { return this->replaceVariables(txt); });
	if (srv_res == nullptr) {
		throw EXCEPTION("Can't connect to servers");
		return nullptr;
	}

	// Создаем запускаемую таску
	_Task * r = tk->Copy([this](const String & txt) { return this->replaceVariables(txt); });
	// Устанавливаем параметры Socks сервера
	if (r->localHost.size() < 1)
		r->localHost = this->defaultHost;
	if (r->localPort < 1)
		r->localPort = this->defaultPort;
	for (_Tun & tun : r->tuns)
		tun.localHost = r->localHost.size() < 1 ? defaultHost : r->localHost;

	// Создаем запускатор
	ShadowSocksClient * res = nullptr;
	res = new ShadowSocksClient(srv_res, r, conf);

	// Устанавливаем глобальные настройки запуска
	res->SetShadowSocksPath(this->replacePath(this->shadowSocksPath));
	res->SetV2RayPluginPath(this->replacePath(this->v2rayPluginPath));
	res->SetPolipoPath(this->replacePath(this->polipoPath));
	res->SetSysProxyPath(this->replacePath(this->sysproxyPath));
	res->SetTempPath(this->replacePath(this->tempPath, true));
	res->SetUDPTimeout(convertTime(this->udpTimeout));
	Tun2Socks::SetT2SPath(this->replacePath(this->tun2socksPath));
	Tun2Socks::SetD2SPath(this->replacePath(this->dns2socksPath));

	return res;
}

bool _ShadowSocksController::CheckInstall() {
	__DP_LIB_NAMESPACE__::log << "Checking install\n";
	__DP_LIB_NAMESPACE__::log << "Check SS\n";
	__DP_LIB_NAMESPACE__::Path p(settings.replacePath(settings.shadowSocksPath));
	if (!p.IsFile())
		return false;
	__DP_LIB_NAMESPACE__::log << "Check V2Ray\n";
	p = __DP_LIB_NAMESPACE__::Path(settings.replacePath(settings.v2rayPluginPath));
	if (!p.IsFile())
		return false;
	__DP_LIB_NAMESPACE__::log << "Check T2S\n";
	p = __DP_LIB_NAMESPACE__::Path(settings.replacePath(settings.tun2socksPath));
	if (!p.IsFile())
		return false;
	__DP_LIB_NAMESPACE__::log << "Check D2S\n";
	p = __DP_LIB_NAMESPACE__::Path(settings.replacePath(settings.dns2socksPath));
	if (!p.IsFile())
		return false;
	__DP_LIB_NAMESPACE__::log << "Check polipo\n";
	p = __DP_LIB_NAMESPACE__::Path(settings.replacePath(settings.polipoPath));
	if (!p.IsFile())
		return false;
	#ifdef DP_WIN
		__DP_LIB_NAMESPACE__::log << "Check sysproxy\n";
		p = __DP_LIB_NAMESPACE__::Path(settings.replacePath(settings.sysproxyPath));
		if (!p.IsFile())
			return false;
	#endif
	#ifdef DP_WIN
		__DP_LIB_NAMESPACE__::log << "Check wget\n";
		p = __DP_LIB_NAMESPACE__::Path(settings.replacePath(settings.wgetPath));
		if (!p.IsFile())
			return false;
	#endif
	return true;
}

String ShadowSocksSettings::GetSource() {
	__DP_LIB_NAMESPACE__::Setting setting;

	//ToDo
	for (_Server* sr: servers) {
		String key = "Servers." + toString(sr->id) + ".";
		SetType(key + "port", sr->port);
		Set(key + "name", sr->name);
		Set(key + "host", sr->host);
		{
			_V2RayServer * srv = dynamic_cast<_V2RayServer *>(sr);
			if (srv != nullptr) {
				Set(key + "V2Ray", "true");
				SetType(key + "tls", srv->isTLS);
				Set(key + "mode", srv->mode);
				Set(key + "v2host", srv->v2host);
				Set(key + "v2path", srv->path);
			}
		}
	}

	for (const Tun2SocksConfig & conf : this->tun2socksConf) {
		String key = "Tun2Socks." + conf.name + ".";
		Set(key + "name", conf.name);
		Set(key + "tunName", conf.tunName);
		Set(key + "defaultRoute", conf.defaultRoute);
		SetType(key + "removeDefaultRoute", conf.removeDefaultRoute);
		String ids = "";
		for (const String & id : conf.dns)
			ids = ids + id + ";";
		Set(key + "DNS", ids);
		ids = "";
		for (const String & id : conf.ignoreIP)
			ids = ids + id + ";";
		Set(key + "IgnoreIP", ids);
	}

	for (_Task * tk : tasks) {
		String key = "Tasks." + toString(tk->id) + ".";
		Set(key + "name", tk->name);
		Set(key + "password", tk->password);
		Set(key + "method", tk->method);
		SetType(key + "localport", tk->localPort);
		SetType(key + "autostart", tk->autostart);
		Set(key + "localhost", tk->localHost);
		Set(key + "VPN", tk->tun2SocksName);
		SetType(key + "httpport", tk->httpProxy);
		SetType(key + "SystemProxy", tk->systemProxy);
		String ids = "";
		for (int id : tk->servers_id)
			ids = ids + toString(id) + ";";
		Set(key + "servers", ids);

		int i = 0;
		for (_Tun t : tk->tuns) {
			String kk = key + "tun." + toString(i++) + ".";
			Set(kk + "remoteHost", t.remoteHost);
			SetType(kk + "remotePort", t.remotePort);
			SetType(kk + "localPort", t.localPort);
			if (t.type == TunType::TCP)
				Set(kk + "proto", "tcp");
			if (t.type == TunType::UDP)
				Set(kk + "proto", "udp");
		}

	}

	{
		String key = "System.Settings.";
		Set(key + "localhost", this->defaultHost);
		SetType(key + "localport", this->defaultPort);
		SetType(key + "autostart", this->autostart);
		Set(key + "ShadowSocksPath", this->shadowSocksPath);
		Set(key + "V2RayPath", this->v2rayPluginPath);
		Set(key + "Tun2Socks", this->tun2socksPath);
		Set(key + "Dns2Socks", this->dns2socksPath);
		SetType(key + "HttpProxyPort", this->defaultHttpPort);
		Set(key + "SysProxyPath", this->sysproxyPath);
		Set(key + "WGetPath", this->wgetPath);
		Set(key + "PolipoPath", this->polipoPath);
		Set(key + "tempPath", this->tempPath);
		SetType(key + "UDPTimeout", this->udpTimeout);
		SetType(key + "ignoreCheck", this->IGNORECHECKSERVER);
		SetType(key + "hideDNS2Socks", this->hideDNS2Socks);
		SetType(key + "enableLogging", this->enableLogging);
		Set(key + "BootstrapDNS", this->bootstrapDNS);

	}

	{
		String key = "System.Variables.";
		for (const auto & it : this->variables) {
			Set(key + it.first, it.second);
		}
	}

	__DP_LIB_NAMESPACE__::OStrStream _out;
	_out << setting;
	return _out.str();
}

void ShadowSocksSettings::Load(const String & text) {
	__DP_LIB_NAMESPACE__::IStrStream in;
	in.str(text);

	__DP_LIB_NAMESPACE__::Setting setting;
	in * setting;

	//ToDO
	auto serversList = setting.getFolders<List<String>>("Servers");
	for (String id : serversList) {
		String key = "Servers." + id + ".";
		_Server * sr = new _Server(__DP_LIB_NAMESPACE__::parse<int>(id));
		if (setting.Conteins(key + "V2Ray")) {
			auto srv = new _V2RayServer(sr);
			sr = srv;
			ReadNType(key + "tls", srv->isTLS, bool);
			ReadN(key + "mode", srv->mode);
			ReadN(key + "v2path", srv->path);
			ReadN(key + "v2host", srv->v2host);
		}

		if (sr == nullptr)
			sr = new _Server();
		if (sr->id >= _Server::glob_id)
			_Server::glob_id = sr->id + 1;
		Read(key + "port", sr->port);
		Read(key + "host", sr->host);
		ReadN(key + "name", sr->name);
		servers.push_back(sr);
	}

	auto tun2SocksList = setting.getFolders<List<String>>("Tun2Socks");
	for (String id : tun2SocksList) {
		String key = "Tun2Socks." + id + ".";
		Tun2SocksConfig conf;
		ReadN(key + "name", conf.name);
		ReadN(key + "tunName", conf.tunName);
		ReadN(key + "defaultRoute", conf.defaultRoute);
		ReadNType(key + "removeDefaultRoute", conf.removeDefaultRoute, bool);

		String ids = "";
		ReadN(key + "DNS", ids);
		auto ll = __DP_LIB_NAMESPACE__::split(ids, ';');
		for (String id: ll) {
			if (id.size() > 0)
				conf.dns.push_back(id);
		}
		ids = "";
		ReadN(key + "IgnoreIP", ids);
		ll = __DP_LIB_NAMESPACE__::split(ids, ';');
		for (String id: ll) {
			if (id.size() > 0)
				conf.ignoreIP.push_back(id);
		}
		conf.isNull = false;
		this->tun2socksConf.push_back(conf);
	}

	auto taskList = setting.getFolders<List<String>>("Tasks");
	for (String id : taskList) {
		String key = "Tasks." + id + ".";
		_Task * tk = new _Task(__DP_LIB_NAMESPACE__::parse<int>(id));
		if (tk->id >= _Task::glob_id)
			_Task::glob_id = tk->id + 1;
		ReadN(key + "name", tk->name);
		Read(key + "password", tk->password);
		Read(key + "method", tk->method);
		ReadNType(key + "localport", tk->localPort, int);
		ReadNType(key + "autostart", tk->autostart, bool);
		ReadNType(key + "httpport", tk->httpProxy, int);
		ReadNType(key + "SystemProxy", tk->systemProxy, bool);
		ReadN(key + "localhost", tk->localHost);
		ReadN(key + "VPN", tk->tun2SocksName);
		String ids = "";
		ReadN(key + "servers", ids);
		auto ll = __DP_LIB_NAMESPACE__::split(ids, ';');
		for (String id: ll) {
			if (id.size() > 0)
				tk->servers_id.push_back(__DP_LIB_NAMESPACE__::parse<int>(id));
		}
		auto tunList = setting.getFolders<List<String>>(key + "tun");
		for (String tunKey : tunList) {
			String kk = key + "tun." + tunKey + ".";
			_Tun t;
			Read(kk + "remoteHost", t.remoteHost);
			ReadType(kk + "remotePort", t.remotePort, int);
			ReadType(kk + "localPort", t.localPort, int);
			String proto;
			Read(kk + "proto", proto);
			t.type = TunType::TCP;
			if (proto == "udp")
				t.type = TunType::UDP;
			tk->tuns.push_back(t);
		}


		tasks.push_back(tk);
	}
	{
		String key = "System.Settings.";
		ReadN(key + "localhost", this->defaultHost);
		ReadNType(key + "localport", this->defaultPort, int);
		ReadNType(key + "autostart", this->autostart, bool);
		ReadN(key + "ShadowSocksPath", this->shadowSocksPath);
		ReadN(key + "V2RayPath", this->v2rayPluginPath);
		ReadN(key + "Tun2Socks", this->tun2socksPath);
		ReadN(key + "Dns2Socks", this->dns2socksPath);
		ReadNType(key + "HttpProxyPort", this->defaultHttpPort, int);
		ReadN(key + "SysProxyPath", this->sysproxyPath);
		ReadN(key + "WGetPath", this->wgetPath);
		ReadN(key + "PolipoPath", this->polipoPath);
		ReadN(key + "tempPath", this->tempPath);
		ReadNType(key + "UDPTimeout", this->udpTimeout, UInt);
		ReadNType(key + "ignoreCheck", this->IGNORECHECKSERVER, bool);
		ReadNType(key + "hideDNS2Socks", this->hideDNS2Socks, bool);
		ReadNType(key + "enableLogging", this->enableLogging, bool);
		ReadN(key + "BootstrapDNS", this->bootstrapDNS);
	}

	{

		auto variablesList = setting.getKeys<List<String>>("System.Variables");
		for (String id : variablesList) {
			String key = "System.Variables." + id;
			this->variables[id] = setting.get(key);
		}
	}
}

DP_ADD_MAIN_FUNCTION(new ShadowSocksMain())
