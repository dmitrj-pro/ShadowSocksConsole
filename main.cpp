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
#include <Parser/SmartParser.h>
#include <_Driver/ThreadWorker.h>
#include <Addon/sha1.hpp>
#include <unistd.h>

#ifdef DP_ANDROID
	#include <jni.h>
#endif

#ifdef DP_WIN
	#include <windows.h>
#endif


using __DP_LIB_NAMESPACE__::Path;
using __DP_LIB_NAMESPACE__::Ifstream;
using __DP_LIB_NAMESPACE__::Vector;
using __DP_LIB_NAMESPACE__::trim;
using __DP_LIB_NAMESPACE__::toString;
using __DP_LIB_NAMESPACE__::SmartParser;
using __DP_LIB_NAMESPACE__::SmartPtr;
using __DP_LIB_NAMESPACE__::OStrStream;

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


#ifdef DP_ANDROID
	extern bool __signal_vpn_started;
#endif

class ShadowSocksMain: public __DP_LIB_NAMESPACE__::ServiceMain {
	private:
		bool reopenConsole = true;
		bool loggingEnabled = false;
		ShadowSocksServer * service_server = nullptr;

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

		String witable_directory = "";
		String executable_directory = "";
		String cache_directory = "";


	public:
		ShadowSocksMain():__DP_LIB_NAMESPACE__::ServiceMain("ShadowSocksConsole"){}

		inline String getWritableDir() const { return witable_directory; }
		inline String getExecutableDir() const { return executable_directory; }
		inline String getCacheDir() const { return cache_directory; }

		void serviceMain() {
			DP_LOG_INFO << "Start " << getVersion() << " as service";
			ShadowSocksController::Get().StartOnBoot();
			if (host_web.port > 0) {
				if (!__DP_LIB_NAMESPACE__::TCPServer::portIsAllow(host_web.host, host_web.port)) {
					this->SetExitCode(2);
					DP_LOG_FATAL << "Can't listen host " << host_web.host << ":" << host_web.port;
					return;
				}
				ShadowSocksController::Get().SetExitFinc([this]() {
					webui->stop();
				});
				webui = SmartPtr<WebUI>(new WebUI(host_web.host, host_web.port));
				#ifdef DP_ANDROID
					host_service.port = 10;
				#endif
				if (host_service.port > 0)
					webui->start();
				else
					webui->startMainLoop();
			}
			#ifdef DP_ANDROID
				host_service.port = 0;
			#endif
			if (host_service.port > 0) {
				service_server = new ShadowSocksServer();
				ShadowSocksController::Get().SetExitFinc([this]() {
					if (service_server != nullptr)
						service_server->Stop();
					if (host_web.port > 0 && !webui.isNull())
						webui->stop();
				});

				if (!__DP_LIB_NAMESPACE__::TCPServer::portIsAllow(host_service.host, host_service.port)) {
					this->SetExitCode(2);
					DP_LOG_FATAL << "Can't listen host " << host_service.host << ":" << host_service.port;
					return;
				}

				DP_LOG_INFO << "Try to start service host " << host_service.host << ":" << host_service.port;
				service_server->Start(host_service.host, host_service.port);
			}
			#ifdef DP_ANDROID
				while (!std::cin.eof()) {
					String line = "";
					getline(std::cin, line);
					if (__DP_LIB_NAMESPACE__::startWithN(line, "SERVICE_STARTED")) {
						__signal_vpn_started = true;
						continue;
					}
					if (__DP_LIB_NAMESPACE__::startWithN(line, "KILL")) {
						exit(0);
						continue;
					}
					
					if (__DP_LIB_NAMESPACE__::startWithN(line, "EXIT")) {
						DP_LOG_ERROR << "Received exit from main application";
						ShadowSocksController::Get().MakeExit();
						DP_LOG_ERROR << "ShadowSocksController stoped";
						break;
					}
				}
			#endif
			DP_LOG_INFO << "ShadowSocksConsole service stoped";
		}
		void consoleMain();

		virtual void MainLoop() override {
			ShadowSocksController::Get().SetExitFinc(std::bind(&ShadowSocksMain::MainExit, this));

			if (isService()) {
				serviceMain();
				return;
			}
			consoleMain();
		}
		void _MainExit0(){}
		virtual void MainExit() override {
			this->SetNeedToExit(true);
			DP_LOG_WARNING << "Received close";
			ShadowSocksController::Get().SetExitFinc(std::bind(&ShadowSocksMain::_MainExit0, this));
			if (service_server != nullptr) {
				service_server->Stop();
			} else {
				close(0);
			}
			if (!webui.isNull())
				webui->stop();
			ShadowSocksController::Get().MakeExit();

				//std::cin
		}

		inline void autoStart() {
			ShadowSocksController::Get().EnableAutoStart();
		}
		inline void disableStart() {
			ShadowSocksController::Get().DisableAutoStart();
		}
		inline void disableReopenConsole() { reopenConsole = false; }
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
			out << "ShadowSocksConsole v" << SS_HEAD_VERSION << "-" << SS_VERSION << " (" << SS_VERSION_HASHE << "). ";
			out << "Base on DPLib V" << __DP_LIB_NAMESPACE__::VERSION();
			return out.str();
		}
		inline void version() {
			std::cout << getVersion();
			this->SetNeedToExit(true);
		}

		inline void enableLogging() {
			ShadowSocksController::Get().OpenLogFile();
			loggingEnabled = true;
		}

		inline void setWriteDirectory(const String & path) {
			DP_LOG_ERROR << "Set main directory " << path;
			this->witable_directory = path;
			this->cache_directory = this->witable_directory;
			readHostData();

			Path p { path};
			#ifdef DP_WIN
				if (Is64bitWindows())
					p.Append("64");
				else
					p.Append("32");
			#endif
			#ifdef DP_LINUX
				p.Append("modules");
			#endif
			DP_LOG_ERROR << "Set executable directory " << path;
			this->executable_directory = p.Get();
		}
		inline void setExecuteDirectory(const String & path) {
			DP_LOG_ERROR << "Set executable directory " << path;
			this->executable_directory = path;
		}
		inline void setCacheDirectory(const String & path) {
			DP_LOG_ERROR << "Set executable directory " << path;
			this->cache_directory = path;
		}

		inline void onError() {
			this->SetNeedToExit(true);
		}

		virtual void PreStart() override {
			__DP_LIB_NAMESPACE__::ThreadWorkerManager::get()->setMaximuFreeWorkers(2);
			srand(time(nullptr));
			{
				Path p = Path(__DP_LIB_NAMESPACE__::ServiceSinglton::Get().GetPathToFile());
				p = Path(p.GetFolder());
				this->witable_directory = p.Get();
				this->cache_directory = this->witable_directory;

				#ifdef DP_WIN
					if (Is64bitWindows())
						p.Append("64");
					else
						p.Append("32");
				#endif
				#ifdef DP_LINUX
					p.Append("modules");
				#endif
				this->executable_directory = p.Get();
			}
			readHostData();

			ShadowSocksController::Create(new _ShadowSocksController());
			DP_SM_addArgumentHelp0(&ShadowSocksMain::autoStart, "Auto start tasks", "-e", "--e");
			DP_SM_addArgumentHelp0(&ShadowSocksMain::disableStart, "Run without autostart", "savemode", "-s", "--s", "-savemode", "--savemode");
			DP_SM_addArgumentHelp0(&ShadowSocksMain::disableReopenConsole, "Run without start new console (Windows only)", "noconsole", "-noconsole", "--noconsole");
			DP_SM_addArgumentHelp0(&ShadowSocksMain::install, "Install as service", "install");
			DP_SM_addArgumentHelp0(&ShadowSocksMain::uninstall, "Uninstall service", "uninstall");
			DP_SM_addArgumentHelp0(&ShadowSocksMain::start, "Start service", "start");
			DP_SM_addArgumentHelp0(&ShadowSocksMain::stop, "Stop service", "stop");
			DP_SM_addArgumentHelp0(&ShadowSocksMain::asservice, "start as service", "sexecute");
			DP_SM_addArgumentHelp0(&ShadowSocksMain::version, "Show application version", "version");
			DP_SM_addArgumentHelp0(&ShadowSocksMain::writeHostData, "Write file with default service host:port", "write-ports");
			DP_SM_addArgumentHelp0(&ShadowSocksMain::enableLogging, "Enable logging", "log");
			DP_SM_addArgumentHelp1(String, &ShadowSocksMain::setWriteDirectory, &ShadowSocksMain::onError, "Set main directory with config", "--set-main-dir");
			DP_SM_addArgumentHelp1(String, &ShadowSocksMain::setExecuteDirectory, &ShadowSocksMain::onError, "Set directory with binary files", "--set-depend-dir");
			DP_SM_addArgumentHelp1(String, &ShadowSocksMain::setCacheDirectory, &ShadowSocksMain::onError, "Set directory with cache files", "--set-cache-dir");

			AddHelp(std::cout, [this]() { this->SetNeedToExit(true); }, "help", "-help", "--help", "-h", "--h", "?");
		}
};

String getWritebleDirectory() {
	ShadowSocksMain * m = dynamic_cast<ShadowSocksMain*>(__DP_LIB_NAMESPACE__::ServiceSinglton::GetRef());
	if (m == nullptr) {
		Path p = Path(__DP_LIB_NAMESPACE__::ServiceSinglton::Get().GetPathToFile());
		p = Path(p.GetFolder());
		return p.Get();
	}
	return m->getWritableDir();
}

String getExecutableDirectory() {
	ShadowSocksMain * m = dynamic_cast<ShadowSocksMain*>(__DP_LIB_NAMESPACE__::ServiceSinglton::GetRef());
	if (m == nullptr) {
		Path p = Path(__DP_LIB_NAMESPACE__::ServiceSinglton::Get().GetPathToFile());
		p = Path(p.GetFolder());
		return p.Get();
	}
	return m->getExecutableDir();
}

String getCacheDirectory() {
	ShadowSocksMain * m = dynamic_cast<ShadowSocksMain*>(__DP_LIB_NAMESPACE__::ServiceSinglton::GetRef());
	if (m == nullptr) {
		Path p = Path(__DP_LIB_NAMESPACE__::ServiceSinglton::Get().GetPathToFile());
		p = Path(p.GetFolder());
		return p.Get();
	}
	return m->getCacheDir();
}



void ShadowSocksMain::consoleMain() {
	__DP_LIB_NAMESPACE__::global_config.log.DetachChannel(__DP_LIB_NAMESPACE__::global_config.log_default_channel);
	#ifdef DP_WIN
		if (reopenConsole) {
		if (!AllocConsole()) {
			DP_LOG_ERROR << "Fail to create console: " <<  __DP_LIB_NAMESPACE__::GetLastErrorAsString() << "\n";
		} else {
			freopen("CONIN$", "r", stdin);
			//freopen("CONOUT$", "w", stderr);
			freopen("CONOUT$", "w", stdout);
		}
		}

	#endif

	if (ShadowSocksServer::IsCanConnect(host_service.host, host_service.port)) {
		ShadowSocksRemoteClient cl;
		cl.Start(host_service.host, host_service.port);
	} else {
		if (host_web.port > 0) {
			webui = SmartPtr<WebUI>(new WebUI(host_web.host, host_web.port));
			webui->start();
		}
		ShadowSocksController::Get().StartOnBoot();

		auto looper = makeLooper(std::cout, std::cin);
		looper->Loop();
	}
}

bool ShadowSocksSettings::enablePreStartStopScripts = false;
bool ShadowSocksSettings::_disable_log_page = false;
bool ShadowSocksSettings::_disable_import_page = false;
bool ShadowSocksSettings::_disable_exit_page = false;
bool ShadowSocksSettings::_disable_exit_page_hard = false;
bool ShadowSocksSettings::_disable_export_page = false;
bool ShadowSocksSettings::_disable_utils_page = false;
void ShadowSocksMain::readHostData() {
	__DP_LIB_NAMESPACE__::Path file = __DP_LIB_NAMESPACE__::Path(fileHostData());
	if (file.IsFile()) {
		Ifstream in;
		in.open(file.Get());
		SmartParser webhost ("web_host${1:null<string>}=${host:trim<string>}:${port:int}");
		SmartParser cchost ("service_host${1:null<string>}=${host:trim<string>}:${port:int}");
		SmartParser enable_scripting ("tun_scripts${1:null<string>}=${value:trim<int>}");
		SmartParser chache_dir_read ("cache_dir${1:null<string>}=${value:trim<string>}");

		SmartParser disable_import_page("disable_import_page${1:null<string>}=${value:trim<int>}");
		SmartParser disable_export_page("disable_export_page${1:null<string>}=${value:trim<int>}");
		SmartParser disable_utils_page("disable_utils_page${1:null<string>}=${value:trim<int>}");
		SmartParser disable_log_page("disable_log_page${1:null<string>}=${value:trim<int>}");
		SmartParser disable_exit_page("disable_exit_page${1:null<string>}=${value:trim<int>}");
		SmartParser disable_exit_page_hard_mode("disable_exit_page_hard_mode${1:null<string>}=${value:trim<int>}");

		while (!in.eof()) {
			String line;
			getline(in, line);
			if (webhost.Check(line)) {
				host_web.host = webhost.Get("host");
				host_web.port = parse<unsigned short>(webhost.Get("port"));
			}
			if (chache_dir_read.Check(line)) {
				this->cache_directory = chache_dir_read.Get("value");
			}
			if (cchost.Check(line)) {
				host_service.host = cchost.Get("host");
				host_service.port = parse<unsigned short>(cchost.Get("port"));
			}
			if (enable_scripting.Check(line))
				ShadowSocksSettings::enablePreStartStopScripts = parse<bool>(enable_scripting.Get("value"));
			if (disable_import_page.Check(line))
				ShadowSocksSettings::_disable_import_page = parse<bool>(disable_import_page.Get("value"));
			if (disable_log_page.Check(line))
				ShadowSocksSettings::_disable_log_page = parse<bool>(disable_log_page.Get("value"));
			//Warn: before disable_exit_page: SmartParser disable_exit_page_hard_mode.OK && disable_exit_page.OK
			if (disable_exit_page_hard_mode.Check(line)) {
				ShadowSocksSettings::_disable_exit_page_hard = parse<bool>(disable_exit_page_hard_mode.Get("value"));
				continue;
			}
			if (disable_exit_page.Check(line))
				ShadowSocksSettings::_disable_exit_page = parse<bool>(disable_exit_page.Get("value"));
			if (disable_export_page.Check(line))
				ShadowSocksSettings::_disable_export_page = parse<bool>(disable_export_page.Get("value"));
			if (disable_utils_page.Check(line))
				ShadowSocksSettings::_disable_utils_page = parse<bool>(disable_utils_page.Get("value"));

		}
	} else {
		host_web.host = "127.0.0.1";
		host_web.port = 0;
		host_service.host = "127.0.0.1";
		host_service.port = 8898;
		ShadowSocksSettings::_disable_import_page = false;
		ShadowSocksSettings::_disable_log_page = false;
		ShadowSocksSettings::_disable_exit_page = false;
		ShadowSocksSettings::_disable_exit_page_hard = false;
		ShadowSocksSettings::_disable_export_page = false;
		ShadowSocksSettings::_disable_utils_page = false;
		#ifdef DP_ANDROID
			ShadowSocksSettings::_disable_exit_page = true;
			ShadowSocksSettings::_disable_exit_page_hard = true;
			host_web.port = 35080;
			host_service.port = 0;
		#endif
		ShadowSocksSettings::enablePreStartStopScripts = false;
	}
}

String ShadowSocksMain::fileHostData() const {
	__DP_LIB_NAMESPACE__::Path logF {getWritableDir()};
	logF.Append("PORTS.txt");
	return logF.Get();
}

void ShadowSocksMain::writeHostData() {
	__DP_LIB_NAMESPACE__::Path file = __DP_LIB_NAMESPACE__::Path(fileHostData());
	__DP_LIB_NAMESPACE__::Ofstream out;
	out.open(file.Get());
	out << "web_host=" << host_web.host << ":" << host_web.port << "\n";
	out << "service_host=" << host_service.host << ":" << host_service.port << "\n";
	out << "tun_scripts=" << toString(ShadowSocksSettings::enablePreStartStopScripts) << "\n";
	out << "disable_import_page=" << toString(ShadowSocksSettings::_disable_import_page) << "\n";
	out << "disable_log_page=" << toString(ShadowSocksSettings::_disable_log_page) << "\n";
	out << "disable_exit_page=" << toString(ShadowSocksSettings::_disable_exit_page) << "\n";
	out << "disable_exit_page_hard_mode=" << toString(ShadowSocksSettings::_disable_exit_page_hard) << "\n";
	out << "disable_export_page=" << toString(ShadowSocksSettings::_disable_export_page) << "\n";
	out << "disable_utils_page=" << toString(ShadowSocksSettings::_disable_utils_page) << "\n";
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

String ShadowSocksSettings::replaceVariables(const String & src) const {
	SmartParser parser(src);
	for (const auto & it : variables)
		parser.SetAll(it.first, it.second);
	return parser.ToString();
}

String ShadowSocksSettings::replacePath(const String & path, bool is_dir) const {
	__DP_LIB_NAMESPACE__::SmartParser parser(path);
	Path p = Path{getExecutableDirectory()};
	parser.SetAll("INSTALLED", p.Get());
	String res = parser.ToString();
	#ifdef DP_WIN
		if (!is_dir)
			if (!(__DP_LIB_NAMESPACE__::endWithN(res, ".exe") || __DP_LIB_NAMESPACE__::endWithN(res, ".EXE")))
				res = res + ".exe";
	#endif
	#ifdef DP_ANDROID
		if (!is_dir)
			if (!(__DP_LIB_NAMESPACE__::endWithN(res, ".so") || __DP_LIB_NAMESPACE__::endWithN(res, ".SO"))) {
				res = res + ".so";
				p = Path{res};
				Path folder = Path{p.GetFolder()};
				folder.Append("lib" + p.GetFile());
				res = folder.Get();
			}
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
	{
		bool need_task = this->checkServerMode == ServerCheckingMode::DeepCheck || this->checkServerMode == ServerCheckingMode::DeepFast;
		need_task = need_task && flags.deepCheck;
		if (! run_params.multimode)
			srv_res = checker->Check(need_task ? tk : nullptr, srvs, bootstrapDNS, flags.checkServerMode, [this](const String & txt) { return this->replaceVariables(txt); });
		else
			srv_ress = checker->CheckAll(need_task ? tk : nullptr, srvs, bootstrapDNS, flags.checkServerMode, [this](const String & txt) { return this->replaceVariables(txt); });
	}

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
	#ifndef DP_ANDROID
	if (!conf.isNull && autoDetectTunInterface) {
		conf.defaultRoute = Tun2Socks::DetectDefaultRoute();
		conf.tunName = Tun2Socks::DetectInterfaceName();
		if (conf.defaultRoute.size() == 0)
			throw EXCEPTION("Can't auto detect default route");
		if (conf.tunName.size() == 0)
			throw EXCEPTION("Can't auto detect default TAP interface");
	}
	#endif
	if (! ShadowSocksSettings::enablePreStartStopScripts) {
		conf.preStopCommand = "";
		conf.postStartCommand = "";
	}
	if (! run_params.multimode)
		res = new ShadowSocksClient(srv_res, r, run_params, conf);
	else {
		res = new ShadowSocksClient(nullptr, r, run_params, conf);
		res->SetMultiModeServers(srv_ress);
	}

	_RunParams::ShadowSocksType shadowSockType = this->shadowSocksType;
	if (run_params.shadowsocks_type != _RunParams::ShadowSocksType::None)
		shadowSockType = run_params.shadowsocks_type;
	if (flags.type != _RunParams::ShadowSocksType::None)
		shadowSockType = flags.type;
	#ifdef DP_WIN
	if (!Is64bitWindows() && shadowSockType == _RunParams::ShadowSocksType::Rust) {
		DP_LOG_ERROR << "shadowsocks-rust is not support for x86 platform (only x86_64/amd64). Force set shadowsocks-go mode";
		shadowSockType = _RunParams::ShadowSocksType::GO;
	}
	#endif

	#ifdef DP_ANDROID
		shadowSockType = _RunParams::ShadowSocksType::Android;
	#endif


	// Устанавливаем глобальные настройки запуска
	res->SetShadowSocksPath(this->replacePath(shadowSockType == _RunParams::ShadowSocksType::GO ? this->shadowSocksPath : this->shadowSocksPathRust));
	res->SetShadowSocksType(shadowSockType);
	res->SetV2RayPluginPath(this->replacePath(this->v2rayPluginPath));
	res->SetTempPath(this->replacePath(getCacheDirectory(), true));
	res->SetUDPTimeout(this->udpTimeout);
	Tun2Socks::SetT2SPath(this->replacePath(this->tun2socksPath));

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
			#ifdef DP_ANDROID
				p = Path("/system/xbin/wget");
				if (p.IsFile())
					return p.Get();
			#endif
		}
	#endif
	return this->replacePath(this->wgetPath);
}

bool _ShadowSocksController::CheckInstall() {
	DP_LOG_DPDEBUG << "Checking install\n";
	DP_LOG_DPDEBUG << "Check SS\n";
	__DP_LIB_NAMESPACE__::Path p(settings.replacePath(settings.shadowSocksPath));
	bool shadowSocks = p.IsFile();
	p = __DP_LIB_NAMESPACE__::Path(settings.replacePath(settings.shadowSocksPathRust));
	shadowSocks = shadowSocks || p.IsFile();
	if (!shadowSocks)
		return false;
	DP_LOG_DPDEBUG << "Check V2Ray\n";
	p = __DP_LIB_NAMESPACE__::Path(settings.replacePath(settings.v2rayPluginPath));
	if (!p.IsFile())
		return false;
	DP_LOG_DPDEBUG << "Check T2S\n";
	p = __DP_LIB_NAMESPACE__::Path(settings.replacePath(settings.tun2socksPath));
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
		Set(key + "group", sr->group);
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
		SetType(key + "enableDefaultRouting", conf.enableDefaultRouting);
		SetType(key + "isDNS2Socks", conf.isDNS2Socks);
		Set(key + "preStopCommand", conf.preStopCommand);
		Set(key + "postStartCommand", conf.postStartCommand);
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
		Set(key + "group", tk->group);
		Set(key + "password", tk->password);
		Set(key + "method", tk->method);
		SetType(key + "autostart", tk->autostart);
		Set(key + "RunParams", tk->runParamsName);
		SetType(key + "EnableIPv6", tk->enable_ipv6);
		SetType(key + "EnableUDP", tk->enable_udp);
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
		Set(key + "defaultShadowSocks", SSTtypetoString(tk.shadowsocks_type));
		SetType(key + "httpport", tk.httpProxy);
		SetType(key + "SystemProxy", tk.systemProxy);
		SetType(key + "multimode", tk.multimode);
	}

	{
		String key = "System.Settings.";
		Set(key + "autostart", AutoStartMode_to_str(this->autostart));
		SetType(key + "countRestartAutostarted", this->countRestartAutostarted);
		Set(key + "ShadowSocksPath", this->shadowSocksPath);
		Set(key + "ShadowSocksPathRust", this->shadowSocksPathRust);
		Set(key + "ShadowSocksType", SSTtypetoString(this->shadowSocksType));
		Set(key + "V2RayPath", this->v2rayPluginPath);
		Set(key + "Tun2Socks", this->tun2socksPath);
		Set(key + "WGetPath", this->wgetPath);
		SetType(key + "UDPTimeout", this->udpTimeout);
		SetType(key + "WebSessionTimeout", this->web_session_timeout_m);
		SetType(key + "WebEnableLogPage", this->enable_log_page);
		SetType(key + "WebEnableImportPage", this->enable_import_page);
		SetType(key + "WebEnableUtilsPage", this->enable_utils_page);
		SetType(key + "WebEnableExportPage", this->enable_export_page);
		SetType(key + "WebEnableExitPage", this->enable_exit_page);
		SetType(key + "fixLinuxWgetPath", this->fixLinuxWgetPath);
		SetType(key + "autoDetectTunInterface", this->autoDetectTunInterface);
		SetType(key + "enableLogging", this->enableLogging);
		Set(key + "BootstrapDNS", this->bootstrapDNS);
		String tmp = AutoCheckingMode_to_str(this->auto_check_mode);
		Set(key + "AutoCheckingMode", tmp);
		tmp = ServerCheckingMode_to_str(checkServerMode);
		Set(key + "checkServerMode", tmp);
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

String ShadowSocksSettings::GetDiffConfig(const ShadowSocksSettings & s) {
	__DP_LIB_NAMESPACE__::Setting setting;

#define SetDiff(X, Var1, Var2) \
	if (Var1 != Var2) \
		setting.add(X, Var1);

#define SetDiffType(X, Var1, Var2) \
	if (Var1 != Var2) \
		setting.add(X, __DP_LIB_NAMESPACE__::toString(Var1));

	//ToDo
	for (_Server* sr: servers) {
		_Server * sr2 = nullptr;
		for (_Server * _s : s.servers)
			if (_s->id == sr->id) {
				sr2 = _s;
				break;
			}

		bool server_added = false;
		if (sr2 == nullptr) {
			sr2 = new _V2RayServer(0);
			server_added = true;
		}


		String key = "Servers." + toString(sr->id) + ".";
		SetDiffType(key + "port", sr->port, sr2->port);
		SetDiff(key + "name", sr->name, sr2->name);
		SetDiff(key + "group", sr->group, sr2->group);
		SetDiff(key + "host", sr->host, sr2->host);
		{
			_V2RayServer * srv = dynamic_cast<_V2RayServer *>(sr);
			if (srv != nullptr) {
				_V2RayServer * srv2 = dynamic_cast<_V2RayServer *>(sr2);
				if (srv2 == nullptr) {
					Set(key + "V2Ray", "true");
					SetType(key + "tls", srv->isTLS);
					Set(key + "mode", srv->mode);
					Set(key + "v2host", srv->v2host);
					Set(key + "v2path", srv->path);
				} else {
					SetDiffType(key + "tls", srv->isTLS, srv2->isTLS);
					SetDiff(key + "mode", srv->mode, srv2->mode);
					SetDiff(key + "v2host", srv->v2host, srv2->v2host);
					SetDiff(key + "v2path", srv->path, srv2->path);
				}
			}
		}
		if (server_added)
			delete sr2;
	}

	for (const Tun2SocksConfig & conf : this->tun2socksConf) {
		Tun2SocksConfig conf2;
		for (const Tun2SocksConfig & c2 : s.tun2socksConf)
			if (c2.name == conf.name){
				conf2 = c2;
				break;
			}

		String key = "Tun2Socks." + conf.name + ".";
		SetDiff(key + "name", conf.name, conf2.name);
		SetDiff(key + "tunName", conf.tunName, conf2.tunName);
		SetDiff(key + "defaultRoute", conf.defaultRoute, conf2.defaultRoute);
		SetDiffType(key + "removeDefaultRoute", conf.removeDefaultRoute, conf2.removeDefaultRoute);
		SetDiffType(key + "enableDefaultRouting", conf.enableDefaultRouting, conf2.enableDefaultRouting);
		SetDiffType(key + "isDNS2Socks", conf.isDNS2Socks, conf2.isDNS2Socks);
		SetDiff(key + "preStopCommand", conf.preStopCommand, conf2.preStopCommand);
		SetDiff(key + "postStartCommand", conf.postStartCommand, conf2.postStartCommand);
		String ids = "";
		for (const String & id : conf.dns)
			ids = ids + id + ";";
		String ids2 = "";
		for (const String & id : conf2.dns)
			ids2 = ids2 + id + ";";
		SetDiff(key + "DNS", ids, ids2);
		ids = "";
		for (const String & id : conf.ignoreIP)
			ids = ids + id + ";";
		ids2 = "";
		for (const String & id : conf2.ignoreIP)
			ids2 = ids2 + id + ";";
		SetDiff(key + "IgnoreIP", ids, ids2);
	}

	for (_Task * tk : tasks) {
		_Task * tk2 = nullptr;
		for (_Task * t : s.tasks)
			if (t->id == tk->id) {
				tk2 = t;
				break;
			}

		bool task_added = false;
		if (tk2 == nullptr) {
			tk2 = new _Task(0);
			task_added = true;
		}

		String key = "Tasks." + toString(tk->id) + ".";
		SetDiff(key + "name", tk->name, tk2->name);
		SetDiff(key + "group", tk->group, tk2->group);
		SetDiff(key + "password", tk->password, tk2->password);
		SetDiff(key + "method", tk->method, tk2->method);
		SetDiffType(key + "autostart", tk->autostart, tk2->autostart);
		SetDiff(key + "RunParams", tk->runParamsName, tk2->runParamsName);
		SetDiffType(key + "EnableIPv6", tk->enable_ipv6, tk2->enable_ipv6);
		SetDiffType(key + "EnableUDP", tk->enable_udp, tk2->enable_udp);
		String ids = "";
		for (int id : tk->servers_id)
			ids = ids + toString(id) + ";";
		String ids2 = "";
		for (int id : tk2->servers_id)
			ids2 = ids2 + toString(id) + ";";
		SetDiff(key + "servers", ids, ids2);

		int i = 0;
		for (_Tun t : tk->tuns) {
			_Tun t2;
			for (_Tun _t2 : tk2->tuns)
				if (_t2.localHost == t.localHost && _t2.localPort == t.localPort && _t2.type == t.type) {
					t2 = _t2;
					break;
				}
			bool created = t2.remoteHost.size() == 0;

			String kk = key + "tun." + toString(i++) + ".";
			SetDiff(kk + "remoteHost", t.remoteHost, t2.remoteHost);
			SetDiffType(kk + "remotePort", t.remotePort, t2.remotePort);
			SetDiffType(kk + "localPort", t.localPort, t2.localPort);
			if (t.type != t2.type || created) {
				if (t.type == TunType::TCP)
					Set(kk + "proto", "tcp");
				if (t.type == TunType::UDP)
					Set(kk + "proto", "udp");
			}
		}
		if (task_added)
			delete tk2;

	}

	for (const _RunParams & tk : runParams) {
		_RunParams tk2;
		for (const _RunParams & _tk : s.runParams)
			if (_tk.name == tk.name) {
				tk2 = _tk;
				break;
			}

		String key = "RunParams." + toString(tk.name) + ".";
		SetDiff(key + "name", tk.name, tk2.name);
		SetDiffType(key + "localport", tk.localPort, tk2.localPort);
		SetDiff(key + "localhost", tk.localHost, tk2.localHost);
		SetDiff(key + "VPN", tk.tun2SocksName, tk2.tun2SocksName);
		SetDiff(key + "defaultShadowSocks", SSTtypetoString(tk.shadowsocks_type), SSTtypetoString(tk2.shadowsocks_type));
		SetDiffType(key + "httpport", tk.httpProxy, tk2.httpProxy);
		SetDiffType(key + "SystemProxy", tk.systemProxy, tk2.systemProxy);
		SetDiffType(key + "multimode", tk.multimode, tk2.multimode);
	}

	{
		String key = "System.Settings.";
		SetDiff(key + "autostart", AutoStartMode_to_str(this->autostart), AutoStartMode_to_str(s.autostart));
		SetDiffType(key + "countRestartAutostarted", this->countRestartAutostarted, s.countRestartAutostarted);
		SetDiff(key + "ShadowSocksPath", this->shadowSocksPath, s.shadowSocksPath);
		SetDiff(key + "ShadowSocksPathRust", this->shadowSocksPathRust, s.shadowSocksPathRust);
		SetDiff(key + "ShadowSocksType", SSTtypetoString(this->shadowSocksType), SSTtypetoString(s.shadowSocksType));
		SetDiff(key + "V2RayPath", this->v2rayPluginPath, s.v2rayPluginPath);
		SetDiff(key + "Tun2Socks", this->tun2socksPath, s.tun2socksPath);
		SetDiff(key + "WGetPath", this->wgetPath, s.wgetPath);
		SetDiffType(key + "UDPTimeout", this->udpTimeout, s.udpTimeout);
		SetDiffType(key + "WebSessionTimeout", this->web_session_timeout_m, s.web_session_timeout_m);
		SetDiffType(key + "WebEnableLogPage", this->enable_log_page, s.enable_log_page);
		SetDiffType(key + "WebEnableImportPage", this->enable_import_page, s.enable_import_page);
		SetDiffType(key + "WebEnableUtilsPage", this->enable_utils_page, s.enable_utils_page);
		SetDiffType(key + "WebEnableExportPage", this->enable_export_page, s.enable_export_page);
		SetDiffType(key + "WebEnableExitPage", this->enable_exit_page, s.enable_exit_page);
		SetDiffType(key + "fixLinuxWgetPath", this->fixLinuxWgetPath, s.fixLinuxWgetPath);
		SetDiffType(key + "autoDetectTunInterface", this->autoDetectTunInterface, s.autoDetectTunInterface);
		SetDiffType(key + "enableLogging", this->enableLogging, s.enableLogging);
		SetDiff(key + "BootstrapDNS", this->bootstrapDNS, s.bootstrapDNS);
		String tmp = AutoCheckingMode_to_str(this->auto_check_mode);
		SetDiff(key + "AutoCheckingMode", tmp, AutoCheckingMode_to_str(s.auto_check_mode));
		tmp = ServerCheckingMode_to_str(checkServerMode);
		SetDiff(key + "checkServerMode", tmp, ServerCheckingMode_to_str(s.checkServerMode));
		SetDiffType(key + "AutoCheckingIntervalS", this->auto_check_interval_s, s.auto_check_interval_s);
		SetDiff(key + "AutoCheckingIpUrl", this->auto_check_ip_url, s.auto_check_ip_url);
		SetDiff(key + "AutoCheckingDownloadUrl", this->auto_check_download_url, s.auto_check_download_url);
	}

	{
		String key = "System.Variables.";
		for (const auto & it : this->variables) {
			String v2 = "";
			for (const auto & it2 : s.variables)
				if (it2.first == it.first) {
					v2 = it2.second;
				}

			SetDiff(key + it.first, it.second, v2);
		}
	}

	__DP_LIB_NAMESPACE__::OStrStream _out;
	_out << setting;
	return _out.str();
}

void ShadowSocksSettings::ApplyPatch(const String & text) {
	__DP_LIB_NAMESPACE__::IStrStream in;
	in.str(text);

	__DP_LIB_NAMESPACE__::Setting setting;
	in * setting;

	//ToDO
	auto serversList = setting.getFolders<List<String>>("Servers");
	for (String id : serversList) {
		_Server * sr = findServerById(parse<int>(id));
		bool server_created = false;
		if (sr == nullptr) {
			sr = new _Server(__DP_LIB_NAMESPACE__::parse<int>(id));
			server_created = true;
		}

		String key = "Servers." + id + ".";

		if (setting.Conteins(key + "V2Ray")) {
			_V2RayServer * srv = dynamic_cast<_V2RayServer*>(sr);
			if (srv == nullptr) {
				srv = new _V2RayServer(sr);
				if (server_created)
					delete sr;
				sr = srv;
				server_created = true;
				deleteServerById(__DP_LIB_NAMESPACE__::parse<int>(id));
			}
			ReadNType(key + "tls", srv->isTLS, bool);
			ReadN(key + "mode", srv->mode);
			ReadN(key + "v2path", srv->path);
			ReadN(key + "v2host", srv->v2host);
		}

		if (sr->id >= _Server::glob_id)
			_Server::glob_id = sr->id + 1;
		ReadN(key + "port", sr->port);
		ReadN(key + "host", sr->host);
		ReadN(key + "name", sr->name);
		ReadN(key + "group", sr->group);
		if (server_created)
			servers.push_back(sr);
	}

	auto tun2SocksList = setting.getFolders<List<String>>("Tun2Socks");
	for (String id : tun2SocksList) {
		String key = "Tun2Socks." + id + ".";
		String __name = id;
		ReadN(key + "name", __name);
		Tun2SocksConfig conf = findVPNbyName(__name);
		bool created = !conf.isNull;
		ReadN(key + "name", conf.name);
		ReadN(key + "tunName", conf.tunName);
		ReadN(key + "defaultRoute", conf.defaultRoute);
		ReadNType(key + "removeDefaultRoute", conf.removeDefaultRoute, bool);
		ReadNType(key + "isDNS2Socks", conf.isDNS2Socks, bool);
		ReadNType(key + "enableDefaultRouting", conf.enableDefaultRouting, bool);
		ReadN(key + "preStopCommand", conf.preStopCommand);
		ReadN(key + "postStartCommand", conf.postStartCommand);

		String ids = "";
		ReadN(key + "DNS", ids);
		if (ids.size() > 2) {
			conf.dns.clear();
			auto ll = __DP_LIB_NAMESPACE__::split(ids, ';');
			for (String id: ll) {
				if (id.size() > 0)
					conf.dns.push_back(id);
			}
		}
		ids = "";
		ReadN(key + "IgnoreIP", ids);
		if (ids.size() > 2) {
			conf.ignoreIP.clear();
			auto ll = __DP_LIB_NAMESPACE__::split(ids, ';');
			for (String id: ll) {
				if (id.size() > 0)
					conf.ignoreIP.push_back(id);
			}
		}
		conf.isNull = false;
		if (!findVPNbyName(conf.name).isNull)
			deleteVPNByName(conf.name);

		this->tun2socksConf.push_back(conf);
	}

	auto taskList = setting.getFolders<List<String>>("Tasks");
	for (String id : taskList) {
		String key = "Tasks." + id + ".";

		_Task * tk = findTaskById(__DP_LIB_NAMESPACE__::parse<int>(id));
		bool created = false;
		if (tk == nullptr) {
			tk = new _Task(__DP_LIB_NAMESPACE__::parse<int>(id));
			created = true;
		}

		if (tk->id >= _Task::glob_id)
			_Task::glob_id = tk->id + 1;
		ReadN(key + "name", tk->name);
		ReadN(key + "group", tk->group);
		ReadN(key + "password", tk->password);
		ReadN(key + "method", tk->method);
		ReadNType(key + "autostart", tk->autostart, bool);
		ReadNType(key + "EnableUDP", tk->enable_udp, bool);
		ReadNType(key + "EnableIPv6", tk->enable_ipv6, bool);
		ReadN(key + "RunParams", tk->runParamsName);
		if (tk->runParamsName.size() == 0)
			tk->runParamsName = "DEFAULT";
		String ids = "";
		ReadN(key + "servers", ids);
		if (ids.size() > 3) {
			tk->servers_id.clear();
			auto ll = __DP_LIB_NAMESPACE__::split(ids, ';');
			for (String id: ll) {
				if (id.size() > 0)
					tk->servers_id.push_back(__DP_LIB_NAMESPACE__::parse<int>(id));
			}
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

		if (created)
			tasks.push_back(tk);
	}
	auto runList = setting.getFolders<List<String>>("RunParams");
	for (String id : runList) {
		String key = "RunParams." + id + ".";
		String __name = id;
		ReadN(key + "name", __name);
		_RunParams tk = findRunParamsbyName(__name);

		ReadN(key + "name", tk.name);
		ReadNType(key + "localport", tk.localPort, int);
		ReadN(key + "localhost", tk.localHost);
		ReadN(key + "VPN", tk.tun2SocksName);
		String tm = "";
		ReadN(key + "defaultShadowSocks", tm);
		if (tm.size() > 0)
			tk.shadowsocks_type = parseSSType(tm);
		ReadNType(key + "httpport", tk.httpProxy, int);
		ReadNType(key + "SystemProxy", tk.systemProxy, bool);
		ReadNType(key + "multimode", tk.multimode, bool);
		tk.isNull = false;

		if (!findRunParamsbyName(__name).isNull)
			deleteRunParamsByName(__name);
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

		String tmp;
		ReadN(key + "autostart", tmp);
		if (tmp.size() > 0)
			this->autostart = str_to_AutoStartMode(tmp);
		ReadNType(key + "countRestartAutostarted", this->countRestartAutostarted, UInt);
		ReadN(key + "ShadowSocksPath", this->shadowSocksPath);
		ReadN(key + "ShadowSocksPathRust", this->shadowSocksPathRust);
		{
			String t;
			ReadN(key + "ShadowSocksType", t);
			if (t.size() > 0)
				this->shadowSocksType = parseSSType(t);
		}

		ReadN(key + "V2RayPath", this->v2rayPluginPath);
		ReadN(key + "Tun2Socks", this->tun2socksPath);
		ReadN(key + "WGetPath", this->wgetPath);
		ReadNType(key + "WebEnableLogPage", this->enable_log_page, bool);
		ReadNType(key + "WebEnableImportPage", this->enable_import_page, bool);
		ReadNType(key + "WebEnableUtilsPage", this->enable_utils_page, bool);
		ReadNType(key + "WebEnableExportPage", this->enable_export_page, bool);
		ReadNType(key + "WebEnableExitPage", this->enable_exit_page, bool);
		ReadNType(key + "UDPTimeout", this->udpTimeout, UInt);
		ReadNType(key + "WebSessionTimeout", this->web_session_timeout_m, UInt);
		ReadNType(key + "fixLinuxWgetPath", this->fixLinuxWgetPath, bool);
		ReadNType(key + "autoDetectTunInterface", this->autoDetectTunInterface, bool);

		tmp = "";
		ReadN(key + "checkServerMode", tmp);
		if (tmp.size() > 0)
			this->checkServerMode = str_to_ServerCheckingMode(tmp);

		ReadNType(key + "enableLogging", this->enableLogging, bool);
		ReadN(key + "BootstrapDNS", this->bootstrapDNS);

		tmp = "";
		ReadN(key + "AutoCheckingMode", tmp);
		if (tmp.size() > 0)
			this->auto_check_mode = str_to_AutoCheckingMode(tmp);
		ReadNType(key + "AutoCheckingIntervalS", this->auto_check_interval_s, UInt);
		ReadN(key + "AutoCheckingIpUrl", this->auto_check_ip_url);
		ReadN(key + "AutoCheckingDownloadUrl", this->auto_check_download_url);
	}


	{

		auto variablesList = setting.getKeys<List<String>>("System.Variables");
		for (String id : variablesList) {
			String key = "System.Variables." + id;
			this->variables[id] = setting.get(key);
		}
	}
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
			delete sr;
			sr = srv;
			ReadNType(key + "tls", srv->isTLS, bool);
			ReadN(key + "mode", srv->mode);
			ReadN(key + "v2path", srv->path);
			ReadN(key + "v2host", srv->v2host);
		}

		if (sr->id >= _Server::glob_id)
			_Server::glob_id = sr->id + 1;
		Read(key + "port", sr->port);
		Read(key + "host", sr->host);
		ReadN(key + "name", sr->name);
		ReadN(key + "group", sr->group);
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
		if (setting.Conteins(key + "isDNS2Socks")) {
			ReadNType(key + "isDNS2Socks", conf.isDNS2Socks, bool);
		} else {
			conf.isDNS2Socks = true;
		}
		if (setting.Conteins(key + "enableDefaultRouting")) {
			ReadNType(key + "enableDefaultRouting", conf.enableDefaultRouting, bool);
		} else
			conf.enableDefaultRouting = true;
		ReadN(key + "preStopCommand", conf.preStopCommand);
		ReadN(key + "postStartCommand", conf.postStartCommand);

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
		Read(key + "name", tk->name);
		ReadN(key + "group", tk->group);
		Read(key + "password", tk->password);
		Read(key + "method", tk->method);
		ReadNType(key + "autostart", tk->autostart, bool);
		ReadNType(key + "EnableUDP", tk->enable_udp, bool);
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
		String tm = SSTtypetoString(_RunParams::ShadowSocksType::None);
		ReadN(key + "defaultShadowSocks", tm);
		tk.shadowsocks_type = parseSSType(tm);
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

		String tmp = "off";
		ReadN(key + "autostart", tmp);
		this->autostart = str_to_AutoStartMode(tmp);
		ReadNType(key + "countRestartAutostarted", this->countRestartAutostarted, UInt);
		ReadN(key + "ShadowSocksPath", this->shadowSocksPath);
		ReadN(key + "ShadowSocksPathRust", this->shadowSocksPathRust);
		{
			String t = "go";
			ReadN(key + "ShadowSocksType", t);
			this->shadowSocksType = parseSSType(t);
		}

		ReadN(key + "V2RayPath", this->v2rayPluginPath);
		ReadN(key + "Tun2Socks", this->tun2socksPath);
		ReadN(key + "WGetPath", this->wgetPath);
		ReadNType(key + "WebEnableLogPage", this->enable_log_page, bool);
		ReadNType(key + "WebEnableImportPage", this->enable_import_page, bool);
		ReadNType(key + "WebEnableUtilsPage", this->enable_utils_page, bool);
		ReadNType(key + "WebEnableExportPage", this->enable_export_page, bool);
		ReadNType(key + "WebEnableExitPage", this->enable_exit_page, bool);
		ReadNType(key + "UDPTimeout", this->udpTimeout, UInt);
		ReadNType(key + "WebSessionTimeout", this->web_session_timeout_m, UInt);
		ReadNType(key + "fixLinuxWgetPath", this->fixLinuxWgetPath, bool);
		ReadNType(key + "autoDetectTunInterface", this->autoDetectTunInterface, bool);

		tmp = "tcp";
		ReadN(key + "checkServerMode", tmp);
		this->checkServerMode = str_to_ServerCheckingMode(tmp);

		// ToDo: Delete it
		//*b
		if (setting.Conteins(key + "enableDeepCheckServer")) {
			bool enable = false;
			ReadNType(key + "enableDeepCheckServer", enable, bool);
			if (enable)
				this->checkServerMode = ServerCheckingMode::DeepCheck;
		}

		if (setting.Conteins(key + "ignoreCheck")) {
			bool ignore = false;
			ReadNType(key + "ignoreCheck", ignore, bool);
			if (ignore)
				this->checkServerMode = ServerCheckingMode::Off;
		}
		//*e

		ReadNType(key + "enableLogging", this->enableLogging, bool);
		ReadN(key + "BootstrapDNS", this->bootstrapDNS);

		tmp = "Off";
		ReadN(key + "AutoCheckingMode", tmp);
		this->auto_check_mode = str_to_AutoCheckingMode(tmp);
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
			if (hash != "c950ec0ba1ebbf212b155d39d000d2fbb46eafe4")
				throw EXCEPTION("Bad password");
		String r = checksum.final();
		if (r != hash)
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

#ifdef DP_ANDROID
	/*__DP_LIB_NAMESPACE__::Thread * __And_thread_service = nullptr;
	
	extern "C" JNIEXPORT jbool JNICALL Java_us_myprogram_shadowsocksconsole_MainActivity_IsRunning(JNIEnv * env, jobject thiz) {
		if (__And_thread_service == nullptr)
			return false;
		if (__And_thread_service->isFinished())
			return false;
		
	}
	
	extern "C" JNIEXPORT jint JNICALL Java_us_myprogram_shadowsocksconsole_MainActivity_Start(JNIEnv * env, jobject thiz) {
		__DP_LIB_NAMESPACE__::init_network_in_application();
		ShadowSocksMain * ms = new ShadowSocksMain();
		__DP_LIB_NAMESPACE__::ServiceSinglton::Create(ms);
		__DP_LIB_NAMESPACE__::ServiceControler & c = __DP_LIB_NAMESPACE__::ServiceSinglton::Get();
		int argc = 3;
		char ** argv = new char * [argc];
		argv[0] = new char[] { "libShadowSocksConsole.so" };
		argv[1] = new char[] { "log" };
		argv[2] = new char[] {"sexecute"};
		c.load(argc, argv);
		__And_thread_service = new __DP_LIB_NAMESPACE__::Thread([]() {
			__DP_LIB_NAMESPACE__::ServiceControler & c = __DP_LIB_NAMESPACE__::ServiceSinglton::Get();
			c.Main();
		} );
		t->start();
		return 123;
	}*/
#endif

DP_ADD_MAIN_FUNCTION_WITH_NETWORK(new ShadowSocksMain())
