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
#include "wwwsrc/WebUI.h"
#include "TCPChecker.h"
#include <Types/SmartPtr.h>
#include "sha1.hpp"
#include <unistd.h>

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
using __DP_LIB_NAMESPACE__::SmartPtr;
using __DP_LIB_NAMESPACE__::OStrStream;



class ShadowSocksMain: public __DP_LIB_NAMESPACE__::ServiceMain {
	private:
		#ifdef DP_QT_GUI
			QApplication * app = nullptr;
			ShadowSocksWindow * gui = nullptr;
		#endif
		bool reopenConsole = true;
		bool loggingEnabled = false;
		ShadowSocksServer * service_server;

		SmartPtr<WebUI> webui;
		struct Host {
			String host;
			unsigned short port;
		};
		Host host_web;
		Host host_service;


		void readHostData();
		String fileHostData() const;
		void writeHostData();


	public:
		ShadowSocksMain():__DP_LIB_NAMESPACE__::ServiceMain("ShadowSocksConsole"){}

		void serviceMain() {
			DP_LOG_INFO << "Start " << getVersion() << " as service";
			if (host_web.port > 0) {
				if (!__DP_LIB_NAMESPACE__::TCPServer::portIsAllow(host_web.host, host_web.port)) {
					this->SetExitCode(2);
					DP_LOG_FATAL << "Can't listen host " << host_web.host << ":" << host_web.port;
					return;
				}
				webui = SmartPtr<WebUI>(new WebUI(host_web.host, host_web.port));
				webui->start();
				DP_LOG_INFO << "Web server started on " << host_web.host << ":" << host_web.port;
			}

			service_server = new ShadowSocksServer();
			ShadowSocksController::Get().SetExitFinc([this]() {
				service_server->Stop();
			});

			if (!__DP_LIB_NAMESPACE__::TCPServer::portIsAllow(host_service.host, host_service.port)) {
				this->SetExitCode(2);
				DP_LOG_FATAL << "Can't listen host " << host_service.host << ":" << host_service.port;
				return;
			}

			DP_LOG_INFO << "Try to start service host " << host_service.host << ":" << host_service.port;
			service_server->Start(host_service.host, host_service.port);
			DP_LOG_INFO << "ShadowSocksConsole service stoped";
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
			this->SetNeedToExit(true);
			DP_LOG_WARNING << "Received close";
			if (isService()) {
				service_server->Stop();
			} else {
				close(0);
			}
			ShadowSocksController::Get().SetExitFinc(std::bind(&ShadowSocksMain::_MainExit0, this));
			ShadowSocksController::Get().MakeExit();
			if (!webui.isNull()) {
				webui->stop();
			}
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
				DP_LOG_WARNING << "Fail set password:" << e.toString() << "\n";
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
			this->fakeService();
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
			__DP_LIB_NAMESPACE__::Path logF(__DP_LIB_NAMESPACE__::ServiceSinglton::Get().GetPathToFile());
			logF=__DP_LIB_NAMESPACE__::Path(logF.GetFolder());
			logF.Append("LOGGING.txt");
			__DP_LIB_NAMESPACE__::log.OpenFile(logF.Get());
			__DP_LIB_NAMESPACE__::log.SetUserLogLevel(__DP_LIB_NAMESPACE__::LogLevel::Trace);
			__DP_LIB_NAMESPACE__::log.SetLibLogLevel(__DP_LIB_NAMESPACE__::LogLevel::DPDebug);
			loggingEnabled = true;
		}
		inline void ForceReadConfig() {
			ShadowSocksController::Get().setForceReadConfigMode();
		}

		virtual void PreStart() override {
			readHostData();
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
			DP_SM_addArgumentHelp0(&ShadowSocksMain::ForceReadConfig, "Disable check checksum in config", "no-check-hashe");
			DP_SM_addArgumentHelp0(&ShadowSocksMain::version, "Show application version", "version");
			DP_SM_addArgumentHelp0(&ShadowSocksMain::writeHostData, "Write file with default service host:port", "write-ports");
			DP_SM_addArgumentHelp0(&ShadowSocksMain::enableLogging, "Enable logging", "log");

			AddHelp(std::cout, [this]() { this->SetNeedToExit(true); }, "help", "-help", "--help", "-h", "--h", "?");
		}
};




void ShadowSocksMain::consoleMain() {
	__DP_LIB_NAMESPACE__::log.SetChannel(nullptr);
	#ifdef DP_WIN
		if (reopenConsole) {
		if (!AllocConsole()) {
			DP_LOG_ERROR << "Fail to create console: " <<  __DP_LIB_NAMESPACE__::GetLastErrorAsString() << "\n";
			//return;
		} else {
			freopen("CONIN$", "r", stdin);
			//freopen("CONOUT$", "w", stderr);
			freopen("CONOUT$", "w", stdout);
		}
		}

	#endif

//	Path p = Path(__DP_LIB_NAMESPACE__::ServiceSinglton::Get().GetPathToFile());
//	p = Path(p.GetFolder());
//	p.Append("__service__.txt");
	//if (p.IsFile()) {
	//if (ShadowSocksServer::IsCanConnect(44444)) {
	if (ShadowSocksServer::IsCanConnect(host_service.host, host_service.port)) {
		ShadowSocksRemoteClient cl;
		cl.Start(host_service.host, host_service.port);
	} else {
		if (host_web.port > 0) {
			webui = SmartPtr<WebUI>(new WebUI(host_web.host, host_web.port));
			webui->start();
		}

		auto looper = makeLooper(std::cout, std::cin);
		looper->Loop();
	}
}

void ShadowSocksMain::readHostData() {
	__DP_LIB_NAMESPACE__::Path file = __DP_LIB_NAMESPACE__::Path(fileHostData());
	if (file.IsFile()) {
		Ifstream in;
		in.open(file.Get());
		SmartParser webhost ("web_host${1:null<string>}=${host:trim<string>}:${port:int}");
		SmartParser cchost ("service_host${1:null<string>}=${host:trim<string>}:${port:int}");
		while (!in.eof()) {
			String line;
			getline(in, line);
			if (webhost.Check(line)) {
				host_web.host = webhost.Get("host");
				host_web.port = parse<unsigned short>(webhost.Get("port"));
			}
			if (cchost.Check(line)) {
				host_service.host = cchost.Get("host");
				host_service.port = parse<unsigned short>(cchost.Get("port"));
			}
		}
	} else {
		host_web.host = "127.0.0.1";
		host_web.port = 0;
		host_service.host = "127.0.0.1";
		host_service.port = 8898;
	}
}

String ShadowSocksMain::fileHostData() const {
	__DP_LIB_NAMESPACE__::Path logF(__DP_LIB_NAMESPACE__::ServiceSinglton::Get().GetPathToFile());
	logF=__DP_LIB_NAMESPACE__::Path(logF.GetFolder());
	logF.Append("PORTS.txt");
	return logF.Get();
}

void ShadowSocksMain::writeHostData() {
	__DP_LIB_NAMESPACE__::Path file = __DP_LIB_NAMESPACE__::Path(fileHostData());
	__DP_LIB_NAMESPACE__::Ofstream out;
	out.open(file.Get());
	out << "web_host=" << host_web.host << ":" << host_web.port << "\n";
	out << "service_host=" << host_service.host << ":" << host_service.port << "\n";
	out.close();
	SetNeedToExit(true);
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

bool ShadowSocksSettings::IsCorrect(_RunParams r) {
	if (r.isNull)
		return false;
	bool inited = false;

	for (const _RunParams & tmp : this->runParams)
		if (tmp.name == r.name)
			return false;

	for (const Tun2SocksConfig & tmp : this->tun2socksConf)
		if (tmp.name == r.tun2SocksName)
			inited = true;

	if (!inited && (r.tun2SocksName.size() > 0))
		return false;
	return true;
}

void ShadowSocksSettings::deleteRunParamsByName(const String & name) {
	for (auto it = this->runParams.begin(); it != this->runParams.end(); it++)
		if (it->name == name) {
			this->runParams.erase(it);
			break;
		}
}

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
	for (const _RunParams & tmp : this->runParams)
		if (tmp.name == t->runParamsName)
			inited = true;
	if (!inited && (t->runParamsName.size() > 0))
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

_RunParams ShadowSocksSettings::findRunParamsbyName(const String & name) const {
	for (const _RunParams & t : runParams)
		if (t.name == name) {
			return t;
		}
	return _RunParams();
}
_RunParams ShadowSocksSettings::findDefaultRunParams()const  {
	const String & name = "DEFAULT";
	for (const _RunParams & t : runParams)
		if (t.name == name) {
			return t;
		}
	return _RunParams();
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

String ShadowSocksSettings::replacePath(const String & path, bool is_dir) const {
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

ShadowSocksClient * ShadowSocksSettings::makeServer(int id, const SSClientFlags & flags) {
	static List<__DP_LIB_NAMESPACE__::Pair<TCPChecker::TCPCheckerLoop *, __DP_LIB_NAMESPACE__::Thread * >> check_list;
	{
		bool deleted = true;
		while (deleted) {
			deleted = false;
			for (auto it = check_list.begin(); it != check_list.end(); it++)
				if ((*it).second->isFinished()){
					deleted = true;
					TCPChecker::TCPCheckerLoop * loop = it->first;
					__DP_LIB_NAMESPACE__::Thread * thread = it->second;
					check_list.erase(it);
					thread->join();
					delete thread;
					delete loop;
					break;
				}
		}
	}
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
	_RunParams run_params = findRunParamsbyName(tk->runParamsName);
	if (run_params.isNull) {
		DP_PRINT_TEXT("Running parametrs for task is not set");
		throw EXCEPTION("Running parametrs for task is not set");
	}

	// Находим настройки для VPN
	if (flags.vpnName.size() > 0)
		run_params.tun2SocksName = flags.vpnName;
	Tun2SocksConfig conf;
	if (flags.runVPN == true) {
		for (const Tun2SocksConfig & c : this->tun2socksConf)
			if (c.name == run_params.tun2SocksName) {
				conf = c;
				break;
			}
	}
	if (run_params.tun2SocksName.size() > 0 && conf.isNull && flags.runVPN == true) {
		DP_PRINT_TEXT("VPN Config is not found.");
		throw EXCEPTION("VPN Config is not found.");
	}

	if (flags.listen_host.size() > 0)
		run_params.localHost = flags.listen_host;


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

	if (flags.multimode == SSClientFlagsBoolStatus::True)
		run_params.multimode = true;
	if (flags.multimode == SSClientFlagsBoolStatus::False)
		run_params.multimode = false;

	if (srvs.size() == 1) {
		run_params.multimode = false;
	}

	// Находим первый "живой" сервер
	TCPChecker::TCPCheckerLoop * checker = new TCPChecker::TCPCheckerLoop();
	_Server * srv_res = nullptr;
	List<_Server*> srv_ress;
	if (! run_params.multimode)
		srv_res = checker->Check((this->enableDeepCheckServer && flags.deepCheck) ? tk : nullptr, srvs, bootstrapDNS, IGNORECHECKSERVER, [this](const String & txt) { return this->replaceVariables(txt); });
	else
		srv_ress = checker->CheckAll((this->enableDeepCheckServer && flags.deepCheck) ? tk : nullptr, srvs, bootstrapDNS, IGNORECHECKSERVER, [this](const String & txt) { return this->replaceVariables(txt); });

	// join-им в отдельном потоке, чтобы не засирать основной поток
	__DP_LIB_NAMESPACE__::Thread * joinChecker = new __DP_LIB_NAMESPACE__::Thread([checker](){
		checker->Join();
	});
	joinChecker->SetName("TCPChecker Join Thread");
	joinChecker->start();
	check_list.push_back(__DP_LIB_NAMESPACE__::Pair<TCPChecker::TCPCheckerLoop *, __DP_LIB_NAMESPACE__::Thread * >(checker, joinChecker));

	if (srv_res == nullptr && srv_ress.size() == 0) {
		throw EXCEPTION("Can't connect to servers");
		return nullptr;
	}

	// Создаем запускаемую таску
	_Task * r = tk->Copy([this](const String & txt) { return this->replaceVariables(txt); });
	// Устанавливаем параметры Socks сервера
	for (_Tun & tun : r->tuns)
		tun.localHost = run_params.localHost;

	// Создаем запускатор
	ShadowSocksClient * res = nullptr;
	if (!conf.isNull && autoDetectTunInterface) {
		conf.defaultRoute = Tun2Socks::DetectDefaultRoute();
		conf.tunName = Tun2Socks::DetectInterfaceName();
		if (conf.defaultRoute.size() == 0)
			throw EXCEPTION("Can't auto detect default route");
		if (conf.tunName.size() == 0)
			throw EXCEPTION("Can't auto detect default TAP interface");
	}
	if (! run_params.multimode)
		res = new ShadowSocksClient(srv_res, r, run_params, conf);
	else {
		res = new ShadowSocksClient(nullptr, r, run_params, conf);
		res->SetMultiModeServers(srv_ress);
	}


	// Устанавливаем глобальные настройки запуска
	res->SetShadowSocksPath(this->replacePath(this->shadowSocksPath));
	res->SetV2RayPluginPath(this->replacePath(this->v2rayPluginPath));
	res->SetTempPath(this->replacePath(this->tempPath, true));
	res->SetUDPTimeout(convertTime(this->udpTimeout));
	Tun2Socks::SetT2SPath(this->replacePath(this->tun2socksPath));
	Tun2Socks::SetD2SPath(this->replacePath(this->dns2socksPath));

	return res;
}

String ShadowSocksSettings::getWGetPath() const {
	#ifdef DP_LIN
		if (fixLinuxWgetPath) {
			Path  p = Path("/usr/local/bin/wget");
			if (p.IsFile())
				return p.Get();
			p = Path("/bin/wget");
			if (p.IsFile())
				return p.Get();
			p = Path("/usr/bin/wget");
			if (p.IsFile())
				return p.Get();
		}
	#endif
	return this->replacePath(this->wgetPath);
}

bool _ShadowSocksController::CheckInstall() {
	DP_LOG_DPDEBUG << "Checking install\n";
	DP_LOG_DPDEBUG << "Check SS\n";
	__DP_LIB_NAMESPACE__::Path p(settings.replacePath(settings.shadowSocksPath));
	if (!p.IsFile())
		return false;
	DP_LOG_DPDEBUG << "Check V2Ray\n";
	p = __DP_LIB_NAMESPACE__::Path(settings.replacePath(settings.v2rayPluginPath));
	if (!p.IsFile())
		return false;
	DP_LOG_DPDEBUG << "Check T2S\n";
	p = __DP_LIB_NAMESPACE__::Path(settings.replacePath(settings.tun2socksPath));
	if (!p.IsFile())
		return false;
	DP_LOG_DPDEBUG << "Check D2S\n";
	p = __DP_LIB_NAMESPACE__::Path(settings.replacePath(settings.dns2socksPath));
	if (!p.IsFile())
		return false;
	#ifdef DP_WIN
		DP_LOG_DPDEBUG << "Check wget\n";
		p = __DP_LIB_NAMESPACE__::Path(settings.getWGetPath());
		if (!p.IsFile())
			return false;
	#endif
	return true;
}

String ShadowSocksSettings::GetSourceCashe() {
	__DP_LIB_NAMESPACE__::Setting setting;
	Set("Checker.Server_Name", this->__checker_server_name);
	Set("Checker.Task_Name", this->__checker_task_name);

	for (_Server* sr: servers) {
		String key = "Servers." + toString(sr->id) + ".";
		Set(key + "ip", sr->check_result.ipAddr);
		SetType(key + "speed" , sr->check_result.speed);
		Set(key + "speed_s", sr->check_result.speed_s);
		SetType(key + "is_run" , sr->check_result.isRun);
		Set(key + "msg", sr->check_result.msg);
	}

	__DP_LIB_NAMESPACE__::OStrStream _out;
	_out << setting;
	return _out.str();
}

void ShadowSocksSettings::LoadCashe(const String & text) {
	__DP_LIB_NAMESPACE__::IStrStream in;
	in.str(text);

	__DP_LIB_NAMESPACE__::Setting setting;
	in * setting;

	//ToDO
	ReadN("Checker.Server_Name", this->__checker_server_name);
	ReadN("Checker.Task_Name", this->__checker_task_name);
	auto serversList = setting.getFolders<List<String>>("Servers");
	for (String id : serversList) {
		String key = "Servers." + id + ".";
		_Server * sr = findServerById(parse<int>(id));
		if (sr == nullptr)
			continue;
		ReadN(key + "ip", sr->check_result.ipAddr);
		ReadNType(key + "speed" , sr->check_result.speed, double);
		ReadN(key + "speed_s", sr->check_result.speed_s);
		ReadNType(key + "is_run" , sr->check_result.isRun, bool);
		ReadN(key + "msg", sr->check_result.msg);
	}
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
		SetType(key + "autostart", tk->autostart);
		Set(key + "RunParams", tk->runParamsName);
		SetType(key + "EnableIPv6", tk->enable_ipv6);
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

	for (const _RunParams & tk : runParams) {
		String key = "RunParams." + toString(tk.name) + ".";
		Set(key + "name", tk.name);
		SetType(key + "localport", tk.localPort);
		Set(key + "localhost", tk.localHost);
		Set(key + "VPN", tk.tun2SocksName);
		SetType(key + "httpport", tk.httpProxy);
		SetType(key + "SystemProxy", tk.systemProxy);
		SetType(key + "multimode", tk.multimode);
	}

	{
		String key = "System.Settings.";
		SetType(key + "autostart", this->autostart);
		Set(key + "ShadowSocksPath", this->shadowSocksPath);
		Set(key + "V2RayPath", this->v2rayPluginPath);
		Set(key + "Tun2Socks", this->tun2socksPath);
		Set(key + "Dns2Socks", this->dns2socksPath);
		Set(key + "WGetPath", this->wgetPath);
		Set(key + "tempPath", this->tempPath);
		SetType(key + "UDPTimeout", this->udpTimeout);
		SetType(key + "ignoreCheck", this->IGNORECHECKSERVER);
		SetType(key + "hideDNS2Socks", this->hideDNS2Socks);
		SetType(key + "fixLinuxWgetPath", this->fixLinuxWgetPath);
		SetType(key + "enableDeepCheckServer", this->enableDeepCheckServer);
		SetType(key + "autoDetectTunInterface", this->autoDetectTunInterface);
		SetType(key + "enableLogging", this->enableLogging);
		Set(key + "BootstrapDNS", this->bootstrapDNS);
		String tmp = ShadowSocksSettings::auto_to_str(this->auto_check_mode);
		Set(key + "AutoCheckingMode", tmp);
		SetType(key + "AutoCheckingIntervalS", this->auto_check_interval_s);
		Set(key + "AutoCheckingIpUrl", this->auto_check_ip_url);
		Set(key + "AutoCheckingDownloadUrl", this->auto_check_download_url);
	}
	{
		static const String control = "System.Settings.ShadowSocksPath";
		SHA1 checksum;
		checksum.update(this->shadowSocksPath);
		if (checksum.isFail())
			Set("Core.Crypt.Hash2", "c950ec0ba1ebbf212b155d39d000d2fbb46eafe4");
		String r = checksum.final();
		Set("Core.Crypt.Hash2", r);
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

void ShadowSocksSettings::Load(const String & text, bool force) {
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
		ReadNType(key + "autostart", tk->autostart, bool);
		ReadNType(key + "EnableIPv6", tk->enable_ipv6, bool);
		ReadN(key + "RunParams", tk->runParamsName);
		if (tk->runParamsName.size() == 0)
			tk->runParamsName = "DEFAULT";
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
	auto runList = setting.getFolders<List<String>>("RunParams");
	for (String id : runList) {
		String key = "RunParams." + id + ".";
		_RunParams tk;

		Read(key + "name", tk.name);
		ReadType(key + "localport", tk.localPort, int);
		Read(key + "localhost", tk.localHost);
		ReadN(key + "VPN", tk.tun2SocksName);
		ReadNType(key + "httpport", tk.httpProxy, int);
		ReadNType(key + "SystemProxy", tk.systemProxy, bool);
		ReadNType(key + "multimode", tk.multimode, bool);
		tk.isNull = false;

		runParams.push_back(tk);
	}
	{
		bool containsDefault =false;
		for (const _RunParams & p : runParams)
			if (p.name == "DEFAULT")
				containsDefault = true;
		if (!containsDefault) {
			_RunParams tk;
			tk.name = "DEFAULT";
			tk.isNull = false;
			tk.httpProxy = -1;
			tk.localHost = "127.0.0.1";
			tk.localPort = 1080;
			tk.multimode = false;
			tk.systemProxy = false;
			tk.tun2SocksName = "";
			runParams.push_back(tk);
		}
	}

	{
		String key = "System.Settings.";
		ReadNType(key + "autostart", this->autostart, bool);
		ReadN(key + "ShadowSocksPath", this->shadowSocksPath);
		ReadN(key + "V2RayPath", this->v2rayPluginPath);
		ReadN(key + "Tun2Socks", this->tun2socksPath);
		ReadN(key + "Dns2Socks", this->dns2socksPath);
		ReadN(key + "WGetPath", this->wgetPath);
		ReadN(key + "tempPath", this->tempPath);
		ReadNType(key + "UDPTimeout", this->udpTimeout, UInt);
		ReadNType(key + "ignoreCheck", this->IGNORECHECKSERVER, bool);
		ReadNType(key + "hideDNS2Socks", this->hideDNS2Socks, bool);
		ReadNType(key + "fixLinuxWgetPath", this->fixLinuxWgetPath, bool);
		ReadNType(key + "autoDetectTunInterface", this->autoDetectTunInterface, bool);
		ReadNType(key + "enableDeepCheckServer", this->enableDeepCheckServer, bool);
		ReadNType(key + "enableLogging", this->enableLogging, bool);
		ReadN(key + "BootstrapDNS", this->bootstrapDNS);
		String tmp = "Off";
		ReadN(key + "AutoCheckingMode", tmp);
		this->auto_check_mode = ShadowSocksSettings::str_to_auto(tmp);
		ReadNType(key + "AutoCheckingIntervalS", this->auto_check_interval_s, UInt);
		ReadN(key + "AutoCheckingIpUrl", this->auto_check_ip_url);
		ReadN(key + "AutoCheckingDownloadUrl", this->auto_check_download_url);
	}

	{
		static const String control = "System.Settings.ShadowSocksPath";
		SHA1 checksum;
		checksum.update(this->shadowSocksPath);
		String hash = "";
		ReadN("Core.Crypt.Hash2", hash);

		if (checksum.isFail())
			if (hash != "c950ec0ba1ebbf212b155d39d000d2fbb46eafe4" && !force)
				throw EXCEPTION("Bad password");
		String r = checksum.final();
		if (r != hash && !force)
			throw EXCEPTION("Bad password");
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
