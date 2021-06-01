#include <Parser/SmartParser.h>
#include "ShadowSocksController.h"
#include <_Driver/Path.h>
#include <Types/Exception.h>
#include <Crypt/Crypt.h>
#include <_Driver/ServiceMain.h>

using __DP_LIB_NAMESPACE__::Path;
using __DP_LIB_NAMESPACE__::IStrStream;
using __DP_LIB_NAMESPACE__::toString;

void _ShadowSocksController::Stop() {
	for (auto pair: clients)
		pair.second->Stop(false);
	clients.clear();
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
		res += line + "\n";
	}
	return res;
}

String _ShadowSocksController::GetConfigPath() {
	Path p = Path(__DP_LIB_NAMESPACE__::ServiceSinglton::Get().GetPathToFile());
	p = Path(p.GetFolder());
	p.Append("config.conf");
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
	this->password = password;
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

void _ShadowSocksController::AutoStart(std::function<void()> onCrash) {
	if (!((mode == 1) || (settings.autostart && (mode != 2))) )
		return;
	for (const _Task * tk : settings.tasks)
		if (tk->autostart)
			try{
				StartById(tk->id, [] () {

				}, onCrash, SSClientFlags());
			}catch (__DP_LIB_NAMESPACE__::LineException e) {
				onCrash();
				__DP_LIB_NAMESPACE__::log << "Can't start task #" << toString(tk->id) << ": " << e.toString() << "\n";
				//throw EXCEPTION("Can't start task #" + toString(tk->id) + ": " + e.toString());
				//return;
			}
}

void _ShadowSocksController::StartById(int id, std::function<void()> onSuccess, std::function<void()> onCrash, SSClientFlags flags) {
	__DP_LIB_NAMESPACE__::log << "Try start task #" << id << " with flags:";
	DP_PRINT_VAL_0(flags.runVPN);
	DP_PRINT_VAL_0(flags.vpnName);
	DP_PRINT_VAL_0(flags.port);
	DP_PRINT_VAL_0(flags.server_name);
	DP_PRINT_VAL_0(flags.http_port);

	MakeServerFlags fl;
	fl.server_name = flags.server_name;
	fl.vpn_name = flags.vpnName;
	ShadowSocksClient * shad = settings.makeServer(id, fl);
	shad->SetOnCrash(onCrash);
	shad -> Start(flags, onSuccess);
	DP_PRINT_TEXT("Task #" + toString(id) + "started");
	clients.push_back(__DP_LIB_NAMESPACE__::Pair<int, ShadowSocksClient * >(id, shad));
}

void _ShadowSocksController::StartByName(const String & name, std::function<void()> onSuccess, std::function<void()> onCrash, SSClientFlags flags) {
	for (const _Task * tk : settings.tasks)
		if (tk->name == name)
			StartById(tk->id, onSuccess, onCrash, flags);
}

void _ShadowSocksController::StopByName(const String & name){
	for (auto it = clients.begin(); it != clients.end(); it++) {
		if (it->second->getTask()->name == name) {
			it->second->Stop();
			//delete it->second;
			clients.erase(it);
			break;
		}
	}
}

void _ShadowSocksController::OpenConfig(const String & password) {
	settings = ShadowSocksSettings();
	settings.Load(GetSourceConfig(password));
	this->password = password;
}

void _ShadowSocksController::OpenConfig() {
	OpenConfig(password);
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
}

DP_SINGLTONE_CLASS_CPP(_ShadowSocksController, ShadowSocksController)
