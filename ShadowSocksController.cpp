#include <Parser/SmartParser.h>
#include "ShadowSocksController.h"
#include <_Driver/Path.h>
#include <Types/Exception.h>
#include <Crypt/Crypt.h>
#include <_Driver/ServiceMain.h>
#include "WGetDownloader/Downloader.h"
#include <_Driver/Files.h>
#include <_Driver/Service.h>
#include "VERSION.h"

using __DP_LIB_NAMESPACE__::Path;
using __DP_LIB_NAMESPACE__::IStrStream;
using __DP_LIB_NAMESPACE__::toString;

void _ShadowSocksController::Stop() {
	need_to_exit = true;
	clients_lock.lock();
	for (auto pair: clients)
		try{
			pair.second->Stop(false);
		} catch(...) { }
	clients.clear();
	clients_lock.unlock();

	if (checkerThread != nullptr) {
		checkerThread->join();
		delete checkerThread;
		checkerThread = nullptr;
	}
}

String ReadAllFile(const String & file) {
	String res;
	std::ifstream in;
	in.open(file);
	if (in.fail())
		throw EXCEPTION(String("Fail to open file ") + file);
	while (!in.eof()) {
		String line;
		getline(in, line);
		res += line;
		if (!in.eof())
			res += "\n";
	}
	return res;
}

String _ShadowSocksController::GetConfigPath() {
	Path p = Path(__DP_LIB_NAMESPACE__::ServiceSinglton::Get().GetPathToFile());
	p = Path(p.GetFolder());
	p.Append("config.conf");
	return p.Get();
}

String _ShadowSocksController::GetCashePath() {
	Path p = Path(__DP_LIB_NAMESPACE__::ServiceSinglton::Get().GetPathToFile());
	p = Path(p.GetFolder());
	p.Append("cashe.conf");
	return p.Get();
}

String _ShadowSocksController::GetSourceConfig(const String & filename, const String & password) {
	Path file(filename);
	if (!file.IsFile())
		throw EXCEPTION("Fail to open file " + filename);

	String _all_file = ReadAllFile(filename);
	if (password.size() > 1) {
		__DP_LIB_NAMESPACE__::Crypt crypt = __DP_LIB_NAMESPACE__::Crypt(password, "SCH5");
		_all_file = crypt.Dec(_all_file);
		if (_all_file.size() == 0)
			throw EXCEPTION("Password is invalid");
	}
	return _all_file;
}

String _ShadowSocksController::GetSourceConfig(const String & password) {
	return GetSourceConfig(GetConfigPath(), password);
}

String _ShadowSocksController::GetSourceConfig() {
	return GetSourceConfig(this->password);
}

bool _ShadowSocksController::isEncrypted() {
	try {
		String text = ReadAllFile(GetConfigPath());
		return __DP_LIB_NAMESPACE__::startWithN(text, "SCH");
	} catch (__DP_LIB_NAMESPACE__::LineException e) {
		return true;
	}
}

void _ShadowSocksController::SetPassword(const String & password) {
	if (isCreated())
		throw EXCEPTION("Can't set password for inited settings");
	this->password = password;
}

bool _ShadowSocksController::isCreated(){
	Path p (GetConfigPath());
	return p.IsFile();
}

void _ShadowSocksController::SaveBootConfig() {
	Path p = Path(__DP_LIB_NAMESPACE__::ServiceSinglton::Get().GetPathToFile());
	p = Path(p.GetFolder());
	p.Append("boot.conf");

	if (settings.autostart != AutoStartMode::OnCoreStart)  {
		if (p.IsFile())
			__DP_LIB_NAMESPACE__::RemoveFile(p.Get());
		return;
	}
	String source = "";
	// Готовим конфиг
	{
		ShadowSocksSettings sett = settings;
		sett.servers.clear();
		sett.tasks.clear();
		sett.variables.clear();
		sett.tun2socksConf.clear();
		sett.runParams.clear();

		sett.auto_check_mode = ShadowSocksSettings::AutoCheckingMode::Off;
		sett.auto_check_interval_s = 0;
		sett.web_session_timeout_m = 0;

		for (const _Task * tk : settings.tasks)
			if (tk->autostart) {
				_Task * tm = tk->Copy([this](const String & txt) { return settings.replaceVariables(txt); });
				tm->group = "";
				_RunParams p = settings.findRunParamsbyName(tm->runParamsName);
				if (p.isNull) {
					DP_LOG_FATAL << "Finded task with incorrect run parametr";
					continue;
				}
				sett.tasks.push_back(tm);
				p.Copy(&p, [this](const String & txt) { return settings.replaceVariables(txt); });
				sett.runParams.push_back(p);
				if (p.tun2SocksName.size() > 0) {
					sett.tun2socksConf.push_back(settings.findVPNbyName(p.tun2SocksName));
				}
				for (int srv : tm->servers_id ) {
					_Server * sr = settings.findServerById(srv);
					if (sr == nullptr) {
						DP_LOG_FATAL << "Finded task with nullpointer server";
						continue;
					}
					_Server * sr2 = sr->Copy([this](const String & txt) { return settings.replaceVariables(txt); });
					sr2->group = "";
					sett.servers.push_back(sr2);
				}

			}
		source = sett.GetSource();
	}
	{
		String password = p.Get() + SS_VERSION_HASHE;
		__DP_LIB_NAMESPACE__::Crypt crypt = __DP_LIB_NAMESPACE__::Crypt(password, "SCH5");
		source = crypt.Enc(source);

		__DP_LIB_NAMESPACE__::Ofstream out;
		out.open(p.Get());
		if (out.fail()) {
			DP_LOG_FATAL << "Fail to open boot config file to save";
			return;
		}
		out << source;
		out.flush();
		out.close();
		if (out.fail()){
			DP_LOG_FATAL << "Fail to save boot config file";
			return;
		}
	}
}

void _ShadowSocksController::StartOnBoot(OnShadowSocksError onCrash) {
	{
		Path p = Path(__DP_LIB_NAMESPACE__::ServiceSinglton::Get().GetPathToFile());
		p = Path(p.GetFolder());
		p.Append("boot.conf");
		if (!p.IsFile())
			return;

		String password = p.Get() + SS_VERSION_HASHE;
		String source = "";
		try {
			source = GetSourceConfig(p.Get(), password);
		} catch(__DP_LIB_NAMESPACE__::LineException & e) {
			DP_LOG_FATAL << "Fail load boot config: " << e.toString();
			return;
		} catch (std::exception & e) {
			DP_LOG_FATAL << "Fail load boot config: " << e.what();
			return;
		}
		ShadowSocksSettings sett;
		sett.Load(source);
		if (!(((sett.autostart == AutoStartMode::On || sett.autostart == AutoStartMode::OnCoreStart) && (mode != 2))) )
			return;
		ShadowSocksSettings defaul_sett = this->settings;
		this->settings = sett;
		for (const _Task * tk : sett.tasks)
			if (tk->autostart)
				try{
					ShadowSocksClient * shad = sett.makeServer(tk->id, SSClientFlags{});
					clients_lock.lock();
					try {
						shad->SetOnCrash(onCrash);
						shad -> Start(SSClientFlags{}, [this] (const String & name) {
							makeNotifyTask(name, "Started");
						});
						DP_PRINT_TEXT("Task #" + toString(tk->id) + "started");

						clients.push_back(__DP_LIB_NAMESPACE__::Pair<int, ShadowSocksClient * >(tk->id, shad));
					} catch (__DP_LIB_NAMESPACE__::LineException e) {
						ExitStatus status;
						status.code = ExitStatusCode::UnknowError;
						status.str = e.toString();
						onCrash(tk->name, status);
						DP_LOG_FATAL << "Can't start task #" << toString(tk->name) << ": " << e.toString() << "\n";
					} catch (...) { }

					clients_lock.unlock();
				}catch (__DP_LIB_NAMESPACE__::LineException e) {
					ExitStatus status;
					status.code = ExitStatusCode::UnknowError;
					status.str = e.toString();
					onCrash(tk->name, status);
					DP_LOG_FATAL << "Can't start task #" << toString(tk->name) << ": " << e.toString() << "\n";
				}
		this->settings = defaul_sett;
	}

}

void _ShadowSocksController::AutoStart(OnShadowSocksError onCrash) {
	if (!((mode == 1) || ((settings.autostart == AutoStartMode::On) && (mode != 2))) )
		return;
	for (const _Task * tk : settings.tasks)
		if (tk->autostart)
			try{
				StartById(tk->id, [this] (const String & name) {
					makeNotifyTask(name, "Started");
				}, onCrash, SSClientFlags());
			}catch (__DP_LIB_NAMESPACE__::LineException e) {
				ExitStatus status;
				status.code = ExitStatusCode::UnknowError;
				status.str = e.toString();
				onCrash(tk->name, status);
				DP_LOG_FATAL << "Can't start task #" << toString(tk->name) << ": " << e.toString() << "\n";
			}
}

ShadowSocksClient * _ShadowSocksController::StartById(int id, OnShadowSocksRunned onSuccess, OnShadowSocksError onCrash, SSClientFlags flags) {
	DP_LOG_INFO << "Try start task #" << id << " with flags:";
	DP_PRINT_VAL_0(flags.runVPN);
	DP_PRINT_VAL_0(flags.vpnName);
	DP_PRINT_VAL_0(flags.port);
	DP_PRINT_VAL_0(flags.server_name);
	DP_PRINT_VAL_0(flags.http_port);
	DP_PRINT_VAL_0(flags.sysproxy_s);
	DP_PRINT_VAL_0(flags.deepCheck);
	DP_PRINT_VAL_0(flags.multimode);
	DP_PRINT_VAL_0(SSTtypetoString(flags.type));

	ShadowSocksClient * shad = settings.makeServer(id, flags);
	clients_lock.lock();
	try {
		shad->SetOnCrash(onCrash);
		shad -> Start(flags, onSuccess);
		DP_PRINT_TEXT("Task #" + toString(id) + "started");

		clients.push_back(__DP_LIB_NAMESPACE__::Pair<int, ShadowSocksClient * >(id, shad));
	} catch (...) {
		clients_lock.unlock();
		throw;
	}

	clients_lock.unlock();
	return shad;
}

ShadowSocksClient * _ShadowSocksController::StartByName(const String & name, OnShadowSocksRunned onSuccess, OnShadowSocksError onCrash, SSClientFlags flags) {
	for (const _Task * tk : settings.tasks)
		if (tk->name == name)
			return StartById(tk->id, onSuccess, onCrash, flags);
	return nullptr;
}

bool _ShadowSocksController::isRunning(int id) {
	for (auto it = clients.begin(); it != clients.end(); it++)
		if (it->first == id)
			return true;
	return false;
}

void _ShadowSocksController::StopByName(const String & name){
	DP_LOG_DEBUG << "Try stop task" << name;
	clients_lock.lock();
	for (auto it = clients.begin(); it != clients.end(); it++) {
        if (it->second->getTask() == nullptr) {
            DP_LOG_DEBUG << "Found crashed task. Close it";
            clients.erase(it);
            clients_lock.unlock();
            StopByName(name);
            return;
        }
		if (it->second->getTask()->name == name) {
			if (it->second->GetStatus() == ShadowSocksClientStatus::DoStop || it->second->GetStatus() == ShadowSocksClientStatus::Stoped) {
                DP_LOG_DEBUG << "Task " << name << " will be stop automatical";
                bool need_delete = it->second->GetStatus() == ShadowSocksClientStatus::Stoped;
                ShadowSocksClient * client = it->second;
                clients.erase(it);
                clients_lock.unlock();
                if (need_delete)
                    delete client;
                return;
			}
			DP_LOG_DEBUG << "Stop process for " << name;
			ShadowSocksClient * client = it->second;
			clients.erase(it);
			clients_lock.unlock();

			client->Stop(false);
			delete client;
			return;
		}
	}
	clients_lock.unlock();
}

void _ShadowSocksController::StopByClient(ShadowSocksClient * client) {
	if (client == nullptr)
		return;
	DP_LOG_DEBUG << "Try stop task" << client->getTask()->name;
	clients_lock.lock();
	for (auto it = clients.begin(); it != clients.end(); it++) {
        if (it->second->getTask() == nullptr) {
            DP_LOG_DEBUG << "Found crashed task. Close it";
            clients.erase(it);
            clients_lock.unlock();
            StopByClient(client);
            return;
        }
		if (it->second == client) {
			if (it->second->GetStatus() == ShadowSocksClientStatus::DoStop || it->second->GetStatus() == ShadowSocksClientStatus::Stoped) {
                DP_LOG_DEBUG << "Task " << client->getTask()->name << " will be stop automatical";
                bool need_delete = it->second->GetStatus() == ShadowSocksClientStatus::Stoped;
                ShadowSocksClient * cl = it->second;
                clients.erase(it);
                clients_lock.unlock();
                if (need_delete)
                    delete cl;
                return;
			}
			DP_LOG_DEBUG << "Stop process for " << client->getTask()->name;
			clients.erase(it);
			clients_lock.unlock();
			client->Stop(false);
			//delete it->second;
			return;
		}
	}
	clients_lock.unlock();
}

void _ShadowSocksController::OpenConfig(const String & password) {
	if (settings.tasks.size() > 0) {
		ShadowSocksSettings tmp;
		tmp.Load(GetSourceConfig(password));
	} else {
		settings = ShadowSocksSettings();
		settings.Load(GetSourceConfig(password));
		this->password = password;

		if (settings.enableLogging) {
			__DP_LIB_NAMESPACE__::Path logF(__DP_LIB_NAMESPACE__::ServiceSinglton::Get().GetPathToFile());
			logF=__DP_LIB_NAMESPACE__::Path(logF.GetFolder());
			logF.Append("LOGGING.txt");
			if (!__DP_LIB_NAMESPACE__::log.FileIsOpen())
				__DP_LIB_NAMESPACE__::log.OpenFile(logF.Get());
			__DP_LIB_NAMESPACE__::log.SetUserLogLevel(__DP_LIB_NAMESPACE__::LogLevel::Trace);
			__DP_LIB_NAMESPACE__::log.SetLibLogLevel(__DP_LIB_NAMESPACE__::LogLevel::DPDebug);
		}
	}
}

void _ShadowSocksController::OpenConfig() {
	OpenConfig(password);
}

void _ShadowSocksController::OpenCashe() {
	try {
		settings.LoadCashe(GetSourceConfig(GetCashePath(), password));
	} catch (__DP_LIB_NAMESPACE__::LineException e) {
		DP_LOG_DEBUG << "Fail to open cashe file " << e.toString();
	}
}

void _ShadowSocksController::SaveCashe() {
	if (getConfig().auto_check_mode != ShadowSocksSettings::AutoCheckingMode::Off) {
		String res = settings.GetSourceCashe();
		if (password.size() > 1) {
			__DP_LIB_NAMESPACE__::Crypt crypt = __DP_LIB_NAMESPACE__::Crypt(password, "SCH5");
			res = crypt.Enc(res);
		}
		__DP_LIB_NAMESPACE__::Ofstream out;
		out.open(GetCashePath());
		if (out.fail())
			throw EXCEPTION("Fail to open config file to save");
		out << res;
		out.flush();
		out.close();
		if (out.fail())
			throw EXCEPTION("Fail to save config file");
	}
}

void _ShadowSocksController::SaveConfig(const String & _res) {
	String res = _res;
	if (password.size() > 1) {
		__DP_LIB_NAMESPACE__::Crypt crypt = __DP_LIB_NAMESPACE__::Crypt(password, "SCH5");
		res = crypt.Enc(res);
	}
	__DP_LIB_NAMESPACE__::Ofstream out;
	out.open(GetConfigPath());
	if (out.fail())
		throw EXCEPTION("Fail to open config file to save");
	out << res;
	out.flush();
	out.close();
	if (out.fail())
		throw EXCEPTION("Fail to save config file");
}

void _ShadowSocksController::SaveConfig() {
	SaveConfig(settings.GetSource());
	SaveBootConfig();
}

void _ShadowSocksController::ExportConfig(__DP_LIB_NAMESPACE__::Ostream & out, bool is_mobile, bool resolve_dns, const String & default_dns, const String & v2ray_path) {
	if (!is_mobile) {
		out << "{\n";
		out << "\t\"version\": \"4.1.10.0\",\n";
		out << "\t\"configs\":\n";
	}
	out << "\t[\n";
	int num = 0;
	ShadowSocksSettings & config = getConfig();
	Map<String, List<_Task * >> sortedByGroups;
	for (_Task * t: config.tasks) {
		sortedByGroups[t->group].push_back(t);
	}
	for (auto & pair_list : sortedByGroups) {
		for (_Task * t: pair_list.second) {
			num += 1;
			int srv_num = 0;
			for (int id : t->servers_id) {
				srv_num ++;
				for (_Server * sr : config.servers)
					if (sr->id == id) {
						if (config.bootstrapDNS.size() > 4)
							__DP_LIB_NAMESPACE__::setGlobalDNS(config.bootstrapDNS);

						String _host = config.replaceVariables(sr->host);
						__DP_LIB_NAMESPACE__::List<String> ip_addrs;
						if (resolve_dns)
							ip_addrs = __DP_LIB_NAMESPACE__::resolveDomainList(_host);
						else
							ip_addrs.push_back(_host);

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
								out << v2ray_path;
							}
							out << "\",\n";
							out << "\t\t\"plugin_opts\": \"";
							if (srv != nullptr) {
								if (srv->isTLS)
									out << "tls;";
								out << "mode=" << config.replaceVariables(srv->mode) << ";host=" << config.replaceVariables(srv->v2host) << ";path=" << config.replaceVariables(srv->path);
							}
							out << "\",\n";
							if (!is_mobile) {
								out << "\t\t\"plugin_args\": \"\",\n";
								out << "\t\t\"timeout\": " << 10 << ",\n";
							}
							out << "\t\t\"remarks\": \"" << sr->name << (ip_addrs.size() > 1 ? "_" + toString(i) : "") << "\"" << (is_mobile ? "," : "") << "\n";
							if (is_mobile) {
								String _dns = default_dns;
								_RunParams p = config.findRunParamsbyName(t->runParamsName);
								if (p.tun2SocksName.size() > 1) {
									const Tun2SocksConfig tun = config.findVPNbyName(p.tun2SocksName);
									if (tun.dns.size() > 0)
										_dns = *(tun.dns.begin());

								}

								out << "\t\t\"route\": \"all\",\n";
								out << "\t\t\"remote_dns\": \"" << _dns << "\",\n";
								out << "\t\t\"ipv6\": " << (t->enable_ipv6 ? "true" : "false") << ",\n";
								out << "\t\t\"metered\": false,\n";
								out << "\t\t\"proxy_apps\": {\n";
								out << "\t\t\t\"enabled\": false\n";
								out << "\t\t},\n";
								out << "\t\t\"udpdns\": false\n";
							}
							out << "\t}";
							if (!((config.tasks.size() == num) && (srv_num == t->servers_id.size()) && (i == (ip_addrs.size() - 1)) ) ) {
								out <<",\n";
							} else {
								out << "\n";
							}
						}


					}
			}
		}
	}
	out << "\t]";
	if (!is_mobile) {
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
}

void calcSizeAndText(double origin, double & res, String & res_s);

_ShadowSocksController::CheckLoopStruct _ShadowSocksController::makeCheckStruct() const {
	CheckLoopStruct args;
	args.auto_check_interval_s = settings.auto_check_interval_s;
	args._server_name = this->settings.__checker_server_name;
	args._task_name = this->settings.__checker_task_name;
	args.save_last_check = true;
	args.auto_check_mode = settings.auto_check_mode;
	args.defaultHost = this->settings.findRunParamsbyName("DEFAULT").localHost;
	args.tempPath = settings.replacePath(settings.tempPath, true);
	args.wgetPath = settings.getWGetPath();
	args.downloadUrl = settings.auto_check_download_url;
	args.checkIpUrl = settings.auto_check_ip_url;
	return  args;


}

void _ShadowSocksController::check_server(_Server * srv, const _Task * task,
				  const CheckLoopStruct & args) {
	if (srv == nullptr || task == nullptr)
		return;
	for (const auto & cl : clients) {
		if (cl.second->getTask()->name == task->name) {
			DP_LOG_DEBUG << "Task " << task->name << " already running. Ignore";
			return;
		}
		if (cl.second->vpnRun()) {
			DP_LOG_DEBUG << "Can't check server while running task with VPN mode";
			return;
		}
	}

	DP_LOG_DEBUG << "Checking task " << task->name << " (" << srv->name << ") mode " << ShadowSocksSettings::auto_to_str(args.auto_check_mode);
	srv->check_result = _Server::ResultCheck();

	static unsigned short _socks5_port = 32000;
	static unsigned short _http_port = 36000;
	static std::mutex port_lock;

	port_lock.lock();
		unsigned short socks5_port = _socks5_port + 1;
		unsigned short http_port = _http_port + 1;
		while (!ShadowSocksClient::portIsAllow(args.defaultHost, socks5_port))
			socks5_port += socks5_port + 1;
		while (!ShadowSocksClient::portIsAllow(args.defaultHost, http_port))
			http_port += http_port + 1;
		_socks5_port = socks5_port;
		_http_port = http_port;
		if (_socks5_port > 65000)
			_socks5_port = 32000;
		if (_http_port > 65000)
			_http_port = 36000;
	port_lock.unlock();


	// Ждем пока освободятся порты.
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	//ShadowSocksSettings & conf = getConfig();
	DP_LOG_DEBUG << "Start Http port on " << http_port << " Socks5 port " << socks5_port;

	auto downloadFile = [args] (_Server * srv, Downloader & dwn) {
		Path p (args.tempPath);
		p.Append("speed.zip");
		auto t1 = std::chrono::high_resolution_clock::now();
		try{
			dwn.Download(args.downloadUrl, p.Get());
		} catch (__DP_LIB_NAMESPACE__::LineException e) {
			srv->check_result.isRun = false;
			srv->check_result.msg = e.toString();
			return;
		}
		auto t2 = std::chrono::high_resolution_clock::now();
		unsigned long filesize = __DP_LIB_NAMESPACE__::fileSize(p.Get());
		__DP_LIB_NAMESPACE__::RemoveFile(p.Get());

		auto time_s = std::chrono::duration_cast<std::chrono::seconds> (t2 - t1).count();
		auto time_ms = std::chrono::duration_cast<std::chrono::milliseconds> (t2 - t1).count();
		if (time_s == 0) {
			if (time_ms == 0) {
				srv->check_result.speed = 0;
				srv->check_result.speed_s = "b/s";
			} else {
				calcSizeAndText(filesize / time_ms, srv->check_result.speed, srv->check_result.speed_s);
				srv->check_result.speed_s = srv->check_result.speed_s + "/ms";
			}
		} else {
			calcSizeAndText(filesize / time_s, srv->check_result.speed, srv->check_result.speed_s);
			srv->check_result.speed_s = srv->check_result.speed_s + "/s";
		}
	};

	SSClientFlags flags;
	flags.port = socks5_port;
	flags.http_port = http_port;
	flags.runVPN = false;
	flags.vpnName = "";
	flags.sysproxy_s = SSClientFlagsBoolStatus::False;
	flags.multimode = SSClientFlagsBoolStatus::False;
	flags.deepCheck = false;
	flags.server_name = srv->name;
	flags.deepCheck = false;
	flags.listen_host = args.defaultHost;
	auto onCrash = [srv](const String &, const ExitStatus & status) {
		srv->check_result.isRun = false;
		srv->check_result.msg = status.str;
	};
	ShadowSocksClient * runned = nullptr;
	try{
		runned = StartByName(task->name, [srv](const String &) {
			srv->check_result.isRun = true;
		}, onCrash, flags);
		// Wait for start
		__DP_LIB_NAMESPACE__::ServiceSinglton::Get().LoopWait(1000);
	} catch (__DP_LIB_NAMESPACE__::LineException e) {
		DP_LOG_DEBUG << "Server " << srv->name << " is not work";
		// Если не удалось, то помечаем сервер нерабочим и идем дальше
		srv->check_result.isRun = false;
		srv->check_result.msg = e.message();
		return;
	}

	__DP_LIB_NAMESPACE__::URLElement proxy;
	proxy.host = args.defaultHost;
	proxy.port = http_port;
	proxy.type = __DP_LIB_NAMESPACE__::URLElement::Type::Http;
	Downloader dwn(proxy);
	dwn.SetWget(args.wgetPath);
	dwn.SetCountTry(1);
	dwn.SetTimeoutS(20);
	dwn.SetIgnoreCheckCert(true);
	try{
		srv->check_result.ipAddr = __DP_LIB_NAMESPACE__::trim(dwn.Download(args.checkIpUrl));
		if (!__DP_LIB_NAMESPACE__::isIPv4(srv->check_result.ipAddr)) {
			srv->check_result.ipAddr = "[FAIL]";
			DP_LOG_DEBUG << "Server " << srv->name << " is work ?";
		}
		DP_LOG_DEBUG << "Server " << srv->name << " ip " << srv->check_result.ipAddr;

	} catch (__DP_LIB_NAMESPACE__::LineException e) {
		DP_LOG_DEBUG << "Server " << srv->name << " is not work (Fail to check ip)";
		srv->check_result.isRun = false;
		srv->check_result.msg = e.message();
		StopByClient(runned);
		return;
	}
	srv->check_result.isRun = srv->check_result.ipAddr.size() > 3;
	if (args.auto_check_mode == ShadowSocksSettings::AutoCheckingMode::Speed && srv->check_result.isRun) {
		__DP_LIB_NAMESPACE__::URLElement proxy;
		proxy.host = args.defaultHost;
		proxy.port = http_port;
		proxy.type = __DP_LIB_NAMESPACE__::URLElement::Type::Http;
		Downloader dwn(proxy);
		dwn.SetWget(args.wgetPath);
		dwn.SetIgnoreCheckCert(true);
		downloadFile(srv, dwn);
		DP_LOG_DEBUG << "Server " << srv->name << " speed " << srv->check_result.speed_s;
	}
	StopByClient(runned);
}

void _ShadowSocksController::check_loop(const CheckLoopStruct & args) {
	String server_name = args._server_name;
	String task_name = args._task_name;
	int checked_count = 0;
	List<String> update_servers;
	update_servers.clear();

	while (!need_to_exit) {
		bool is_next = task_name.size() == 0;
		bool checked = false;

		for (_Task * t : settings.tasks) {
			if (t->name == task_name ) {
				for (int id : t->servers_id) {
					_Server * s = getConfig().findServerById(id);
					if ( is_next ) {
						server_name = s->name;
						check_server(s, t, args);
						update_servers.push_back(server_name);
						checked = true;
						is_next = false;
						break;
					}
					if (s->name == server_name)
						is_next = true;
				}
				if (!checked)
					continue;
			}

			if ( is_next ) {
				task_name = t->name;
				for (int id : t->servers_id) {
					_Server * s = getConfig().findServerById(id);
					server_name = s->name;
					update_servers.push_back(server_name);
					check_server(s, t, args);
					checked = true;
					is_next = false;
					break;
				}
			}
			if (checked)
				break;
		}
		if (!checked) {
			if (args.one_loop) {
				SaveCashe();
				break;
			}
			server_name = "";
			task_name = "";
		} else {
			checked_count ++;
			if (checked_count == 10) {
				if (args.save_last_check) {
					settings.__checker_task_name = task_name;
					settings.__checker_server_name = server_name;
					SaveCashe();
				}
				checked_count = 0;

				String servers = "";
				for (const String & name : update_servers)
					servers += name + ", ";
				makeNotifyServer(servers, "Updated");
				update_servers.clear();
			}
		}
		__DP_LIB_NAMESPACE__::ServiceSinglton::Get().LoopWait(args.auto_check_interval_s * 1000);
	}
}

DP_SINGLTONE_CLASS_CPP(_ShadowSocksController, ShadowSocksController)
