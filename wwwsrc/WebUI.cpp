#include "WebUI.h"
#include "../www/textfill.h"
#include <Converter/Converter.h>
#include "../ShadowSocksController.h"
#include <string.h>
#include <Parser/SmartParser.h>
#include <_Driver/ServiceMain.h>
#include <Network/TCPClient.h>
#include <Network/Utils.h>

using __DP_LIB_NAMESPACE__::OStrStream;
using __DP_LIB_NAMESPACE__::IStrStream;
using __DP_LIB_NAMESPACE__::toString;
using __DP_LIB_NAMESPACE__::ConteinsKey;
using __DP_LIB_NAMESPACE__::parse;
using __DP_LIB_NAMESPACE__::trim;

void WebUI::stop() {
	if (!srv.isNull()) {
		SmartPtr<Thread> th = thread;
		SmartPtr<HttpHostPathRouterServer> s = srv;
		thread = SmartPtr<Thread>(nullptr);
		srv = SmartPtr<HttpHostPathRouterServer>(nullptr);

		s->exit();
		th->join();
	}
	ShadowSocksController::Get().disconnectNotify(this);
}

void WebUI::start() {
	thread = SmartPtr<Thread>(new Thread(&WebUI::_start, this));
	thread->start();
}

UserSession & WebUI::getUserSession(const String & id) {
	cookies_lock.lock();
	for (UserSession & s : this->cookies)
		if (s.cookie == id) {
			cookies_lock.unlock();
			return s;
		}
	cookies_lock.unlock();
	throw EXCEPTION("User not auth");
}
bool WebUI::existsUserSession(const String & id) const {
	for (const UserSession & s : this->cookies)
		if (s.cookie == id)
			return true;
	return false;
}

void WebUI::notifyUser(const String & user_id, const String & msg) {
	DP_LOG_DEBUG << "Message for " << user_id << ": " << msg;
	getUserSession(user_id).notify.push_back(msg);
}

#define CHECK_AUTH 	if (!__DP_LIB_NAMESPACE__::ConteinsKey(r->cookie, "auth") || !existsUserSession(r->cookie["auth"])) { \
		return makeRedirect(r, "/login.html"); \
	}

void WebUI::_start() {
	ShadowSocksController::Get().connectNotify(this);
	srv=SmartPtr<HttpHostPathRouterServer>(new HttpHostPathRouterServer());
	srv->add_route("*", "/", "GET", [this](Request r) { CHECK_AUTH; return this->processGetMain(r); });
	srv->add_route("*", "/index.html", "GET", [this](Request r) { CHECK_AUTH; return this->processGetMain(r); });

	srv->add_route("*", "/login.html", "GET", [this](Request r) { return this->processGetStaticFile(r); });
	srv->add_route("*", "/login.html", "POST", [this](Request r) { return this->processPostLogin(r); });

	srv->add_route("*", "/tasks.html", "GET", [this](Request r) { CHECK_AUTH; return this->processGetTasks(r); });
	srv->add_route("*", "/task/start_params", "GET", [this](Request r) { CHECK_AUTH; return this->processGetTaskStartPage(r); });
	srv->add_route("*", "/task/start_params", "POST", [this](Request r) { CHECK_AUTH; return this->processPostTaskStartPage(r); });
	srv->add_route("*", "/task/start", "POST", [this](Request r) { CHECK_AUTH; return this->processPostTaskStart(r); });
	srv->add_route("*", "/task/stop", "POST", [this](Request r) { CHECK_AUTH; return this->processPostTaskStop(r); });
	srv->add_route("*", "/task/delete", "POST", [this](Request r) { CHECK_AUTH; return this->processPostTaskDelete(r); });
	srv->add_route("*", "/task/delete", "GET", [this](Request r) { CHECK_AUTH; return this->processGetTaskDelete(r); });
	srv->add_route("*", "/task/tuns", "POST", [this](Request r) { CHECK_AUTH; return this->processPostTaskTuns(r); });
	srv->add_route("*", "/task/tuns", "GET", [this](Request r) { CHECK_AUTH; return this->processGetTaskTuns(r); });
	srv->add_route("*", "/task/edit", "GET", [this](Request r) { CHECK_AUTH; return this->processGetTaskEditPage(r); });
	srv->add_route("*", "/task/edit", "POST", [this](Request r) { CHECK_AUTH; return this->processPostTaskEdit(r); });
	srv->add_route("*", "/task/new", "GET", [this](Request r) { CHECK_AUTH; return this->processGetNewTaskPage(r); });
	srv->add_route("*", "/task/new", "POST", [this](Request r) { CHECK_AUTH; return this->processPostNewTaskPage(r); });
	srv->add_route("*", "/tasks/check", "GET", [this](Request r) { CHECK_AUTH; return this->processGetCheckServers(r); });
	srv->add_route("*", "/tasks/check", "POST", [this](Request r) { CHECK_AUTH; return this->processPostCheckServers(r); });
	srv->add_route("*", "/task/check", "GET", [this](Request r) { CHECK_AUTH; return this->processGetCheckTask(r); });
	srv->add_route("*", "/task/check", "POST", [this](Request r) { CHECK_AUTH; return this->processPostCheckTask(r); });

	srv->add_route("*", "/runs.html", "GET", [this](Request r) { CHECK_AUTH; return this->processGetRuns(r); });
	srv->add_route("*", "/runs/new", "GET", [this](Request r) { CHECK_AUTH; return this->processGetAddRuns(r); });
	srv->add_route("*", "/runs/new", "POST", [this](Request r) { CHECK_AUTH; return this->processPostAddRuns(r); });
	srv->add_route("*", "/runs/edit", "GET", [this](Request r) { CHECK_AUTH; return this->processGetEditRuns(r); });
	srv->add_route("*", "/runs/edit", "POST", [this](Request r) { CHECK_AUTH; return this->processPostEditRuns(r); });
	srv->add_route("*", "/runs/delete", "GET", [this](Request r) { CHECK_AUTH; return this->processGetDeleteRuns(r); });
	srv->add_route("*", "/runs/delete", "POST", [this](Request r) { CHECK_AUTH; return this->processPostDeleteRuns(r); });

	srv->add_route("*", "/servers.html", "GET", [this](Request r) { CHECK_AUTH; return this->processGetServers(r); });
	srv->add_route("*", "/server/edit", "GET", [this](Request r) { CHECK_AUTH; return this->processGetServerEditPage(r); });
	srv->add_route("*", "/server/edit", "POST", [this](Request r) { CHECK_AUTH; return this->processPostServerEditPage(r); });
	srv->add_route("*", "/server/new", "GET", [this](Request r) { CHECK_AUTH; return this->processGetNewServerPage(r); });
	srv->add_route("*", "/server/new", "POST", [this](Request r) { CHECK_AUTH; return this->processPostNewServerPage(r); });
	srv->add_route("*", "/server/delete", "POST", [this](Request r) { CHECK_AUTH; return this->processPostServerDelete(r); });
	srv->add_route("*", "/server/delete", "GET", [this](Request r) { CHECK_AUTH; return this->processGetServerDelete(r); });

	srv->add_route("*", "/vpn.html", "GET", [this](Request r) { CHECK_AUTH; return this->processGetVPN(r); });
	srv->add_route("*", "/vpn/refresh", "GET", [this](Request r) { CHECK_AUTH; return this->processGetVPNRefresh(r); });
	srv->add_route("*", "/vpn/delete", "POST", [this](Request r) { CHECK_AUTH; return this->processPostVPNDelete(r); });
	srv->add_route("*", "/vpn/delete", "GET", [this](Request r) { CHECK_AUTH; return this->processGetVPNDelete(r); });
	srv->add_route("*", "/vpn/edit", "GET", [this](Request r) { CHECK_AUTH; return this->processGetVPNEditPage(r); });
	srv->add_route("*", "/vpn/edit", "POST", [this](Request r) { CHECK_AUTH; return this->processPostVPNEditPage(r); });
	srv->add_route("*", "/vpn/new", "GET", [this](Request r) { CHECK_AUTH; return this->processGetNewVPNPage(r); });
	srv->add_route("*", "/vpn/new", "POST", [this](Request r) { CHECK_AUTH; return this->processPostNewVPNPage(r); });

	srv->add_route("*", "/variables.html", "GET", [this](Request r) { CHECK_AUTH; return this->processGetVariables(r); });
	srv->add_route("*", "/variable/new", "GET", [this](Request r) { CHECK_AUTH; return this->processGetAddVariables(r); });
	srv->add_route("*", "/variable/new", "POST", [this](Request r) { CHECK_AUTH; return this->processPostAddVariables(r); });
	srv->add_route("*", "/variable/edit", "GET", [this](Request r) { CHECK_AUTH; return this->processGetEditVariables(r); });
	srv->add_route("*", "/variable/edit", "POST", [this](Request r) { CHECK_AUTH; return this->processPostEditVariables(r); });
	srv->add_route("*", "/variable/delete", "GET", [this](Request r) { CHECK_AUTH; return this->processGetDeleteVariables(r); });
	srv->add_route("*", "/variable/delete", "POST", [this](Request r) { CHECK_AUTH; return this->processPostDeleteVariables(r); });




	srv->add_route("*", "/settings.html", "GET", [this](Request r) { CHECK_AUTH; return this->processGetSettings(r); });
	srv->add_route("*", "/settings/edit", "GET", [this](Request r) { CHECK_AUTH; return this->processGetEditSettings(r); });
	srv->add_route("*", "/settings/edit", "POST", [this](Request r) { CHECK_AUTH; return this->processPostEditSettings(r); });

	srv->add_route("*", "/export.html", "GET", [this](Request r) { CHECK_AUTH; return this->processGetExport(r); });
	srv->add_route("*", "/export.html", "POST", [this](Request r) { CHECK_AUTH; return this->processPostExport(r); });
	srv->add_route("*", "/logout.html", "GET", [this](Request r) { CHECK_AUTH; return this->processGetLogout(r); });
	srv->add_route("*", "/exit.html", "GET", [this](Request r) { CHECK_AUTH; return this->processGetExit(r); });
	srv->add_route("*", "/exit.html", "POST", [this](Request r) { CHECK_AUTH; return this->processPostExit(r); });
	srv->add_route("*", "/utils.html", "GET", [this](Request r) {
		CHECK_AUTH;
		return this->processGetUtils(r, makeUtilsStruct());
		}
	);
	srv->add_route("*", "/utils.html", "POST", [this](Request r) { CHECK_AUTH; return this->processPostUtils(r); });

	srv->add_route("*", "/notify", "GET", [this](Request r) { CHECK_AUTH; return this->processGetNews(r); });
	srv->add_route("*", "/notify_exists", "GET", [this](Request r) { CHECK_AUTH; return this->processCheckNews(r); });

	srv->add_route("*", "*", "GET", [this](Request r) { return this->processGetStaticFile(r); });
	DP_LOG_INFO << "Start web server " << web_host << ":" << web_port;
	srv->listen(web_host, web_port);
}



Request WebUI::processGetMain(Request req) {
	return makeRedirect(req, "/tasks.html");
}

Request WebUI::processGetNews(Request req) {
	auto user_id = req->cookie["auth"];
	Request resp = makeRequest();
	UserSession & s = getUserSession(user_id);

	if (s.notify.size() > 0) {
		String fr = *s.notify.begin();
		s.notify.pop_front();
		resp->body_length = fr.size();
		resp->body = new char[resp->body_length + 1];
		strncpy(resp->body, fr.c_str(), resp->body_length);
	}
	return resp;
}

Request WebUI::processCheckNews(Request req) {
	auto user_id = req->cookie["auth"];
	Request resp = makeRequest();

	UserSession & s = getUserSession(user_id);
	String r = "no";
	if (s.notify.size() > 0)
		r = "yes";
	resp->body_length = r.size();
	resp->body = new char[resp->body_length + 1];
	strncpy(resp->body, r.c_str(), resp->body_length);
	return resp;
}

Request WebUI::processGetStaticFile(Request req) {
	if (req->path.size() > 0) {
		Request resp = makeRequest();
		resp->body_length = 0;
		String filename = req->path.substr(1);

		resp->headers["Content-Type"] = getMimeType(filename);
		resp->body = findResourc(filename, resp->body_length);
		if (resp->body_length > 0 && resp->body != nullptr) {
			return resp;
		} else {
			String txt = findText(filename);
			if (txt.size() < 2)
				return HttpServer::generate404(req->method, req->host, req->path);
			resp->body = new char[txt.size() + 1];
			strncpy(resp->body, txt.c_str(), txt.size());
			resp->body_length = txt.size();
			return resp;
		}
	} else
		return HttpServer::generate404(req->method, req->host, req->path);
}

bool web_SetPassword(const String & password) {
	try {
		auto & ctrl = ShadowSocksController::Get();
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

Request WebUI::processPostLogin(Request req) {
	if (!__DP_LIB_NAMESPACE__::ConteinsKey(req->post, "password"))
		return processGetStaticFile(req);
	unsigned int count = ShadowSocksController::Get().getConfig().servers.size();
	if (!web_SetPassword(req->post["password"].value))
		return processGetStaticFile(req);

	String cookie = toString(rand());
	UserSession s;
	s.cookie = cookie;
	s.notify = List<String>();
	s.started = time(nullptr);
	cookies_lock.lock();
	cookies.push_back(s);
	cookies_lock.unlock();
	DP_LOG_INFO << "Authenticated new user " << cookie;
	Request resp = makeRedirect(req, "/");
	resp->cookie["auth"] = cookie;

	if (ShadowSocksController::Get().CheckInstall()) {
		auto funcCrash = [this, cookie] (const String & name, const ExitStatus & status) {
			OStrStream error;
			error << "Fail to auto start task " << name << ": " << status.str;
			DP_LOG_FATAL << error.str();
			notifyUser(cookie, error.str());
		};
		if ( count != ShadowSocksController::Get().getConfig().servers.size()) {
			ShadowSocksController::Get().AutoStart(funcCrash);
			ShadowSocksController::Get().startCheckerThread();
		}
	} else {
		DP_LOG_FATAL << "Fail install. Try exit and enter new password. Or reinstall application";
		notifyUser(cookie, "Fail install. Try exit and enter new password. Or reinstall application");
	}

	return resp;
}

void WebUI::UpdateServerStatus(const String & server, const String & msg) {
	cookies_lock.lock();
	bool is_deleted = true;
	while (is_deleted) {
		is_deleted = false;
		for (auto it = cookies.begin(); it != cookies.end(); it++) {
			if ((*it).notify.size() > 20) {
				DP_LOG_INFO << "User " << it->cookie << " is not active. Delete it";
				cookies.erase(it);
				is_deleted = true;
				break;
			}
		}
	}
	cookies_lock.unlock();
	for (auto it = cookies.begin(); it != cookies.end(); it++)
		notifyUser(it->cookie, server + " " + msg);
}
void WebUI::UpdateTaskStatus(const String & server, const String & msg) {
	cookies_lock.lock();
	bool is_deleted = true;
	while (is_deleted) {
		is_deleted = false;
		for (auto it = cookies.begin(); it != cookies.end(); it++) {
			if ((*it).notify.size() > 20) {
				DP_LOG_INFO << "User " << it->cookie << " is not active. Delete it";
				cookies.erase(it);
				is_deleted = true;
				break;
			}
		}
	}
	cookies_lock.unlock();
	for (auto it = cookies.begin(); it != cookies.end(); it++)
		notifyUser(it->cookie, server + " " + msg);
}

Request WebUI::processGetExport(Request) {
	String html = makePage("Export", "export/page.txt", List<String>());
	Request resp = makeRequest();
	resp->body = new char[html.size() + 1];
	strncpy(resp->body, html.c_str(), html.size());
	resp->body_length = html.size();
	return resp;
}

Request WebUI::processPostExport(Request req) {
	auto user_id = req->cookie["auth"];

	if (!ConteinsKey(req->post, "mode"))
		return HttpServer::generate404(req->method, req->host, req->path);

	Request resp = makeRequest();

	String mode = req->post["mode"].value;
	if (mode == "source") {
		String html = ShadowSocksController::Get().GetSourceConfig();
		resp->headers["Content-Type"] = getMimeType("config.txt");
		resp->headers["Content-Disposition"] = "attachment; filename=\"config.txt\"";
		resp->body = new char[html.size() + 1];
		strncpy(resp->body, html.c_str(), html.size());
		resp->body_length = html.size();
	}
	if (mode == "enc") {
		String html = ReadAllFile(ShadowSocksController::Get().GetConfigPath());
		resp->headers["Content-Type"] = getMimeType("config.bin");
		resp->headers["Content-Disposition"] = "attachment; filename=\"config.conf\"";
		resp->body = new char[html.size() + 1];
		strncpy(resp->body, html.c_str(), html.size());
		resp->body_length = html.size();
	}
	if (mode == "json") {
		bool is_mobile = false;
		bool resolve_dns = true;
		String default_dns = "94.140.14.14";
		String v2ray_path = "v2ray-plugin";

		String t = "desktop";
		readParametr_boolv(resolve_dns, "resolve_dns", true);
		readParametr_n(t,  "j_mode");
		if (t == "mobile")
			is_mobile = true;
		readParametr_n(default_dns,  "default_dns");
		readParametr_n(v2ray_path,  "v2ray_path");

		__DP_LIB_NAMESPACE__::OStrStream out;
		ShadowSocksController::Get().ExportConfig(out, is_mobile, resolve_dns, default_dns, v2ray_path);

		String html = out.str();
		resp->headers["Content-Type"] = getMimeType("config.json");
		resp->headers["Content-Disposition"] = "attachment; filename=\"config.json\"";
		resp->body = new char[html.size() + 1];
		strncpy(resp->body, html.c_str(), html.size());
		resp->body_length = html.size();
	}
	if (resp->body_length == 0) {
		return HttpServer::generate404(req->method, req->host, req->path);
	}
	return resp;
}

Request WebUI::processGetLogout(Request req) {
	auto user_id = req->cookie["auth"];
	cookies_lock.lock();
	for (auto it = cookies.begin(); it != cookies.end(); it++) {
		if (it->cookie == user_id) {
			cookies.erase(it);
			break;
		}
	}
	cookies_lock.unlock();
	Request resp = makeRequest();
	String html = makePage("Logout", "Logout", "Buy!");
	resp->body = new char[html.size() + 1];
	strncpy(resp->body, html.c_str(), html.size());
	resp->body_length = html.size();
	return resp;
}

Request WebUI::processGetExit(Request) {
	String html = makePage("Exit", "Close ShadowSocks", "exit/exit.txt", List<String>());
	Request resp = makeRequest();
	resp->body = new char[html.size() + 1];
	strncpy(resp->body, html.c_str(), html.size());
	resp->body_length = html.size();
	return resp;
}
Request WebUI::processPostExit(Request) {
	Thread * th = new Thread([]() {
		__DP_LIB_NAMESPACE__::ServiceSinglton::Get().ExecuteClose();
	});
	th->SetName("WebUI::processPostExit");
	th->start();

	Request resp = makeRequest();
	String html = makePage("Exit", "Close ShadowSocks", "Buy!");
	resp->body = new char[html.size() + 1];
	strncpy(resp->body, html.c_str(), html.size());
	resp->body_length = html.size();
	return resp;
}

Request WebUI::processGetUtils(Request, const UtilsStruct & res) {
	OStrStream out;
	for (const String & ip : res.resolve_result)
		out << "<p>" << ip << "</p>";

	OStrStream out2;
	for (unsigned short ip : res.find_free_result)
		out2 << "<p>" << res.find_free_host  + ":" + toString(ip) << "</p>\n";

	String html = makePage("Utils", "utils/index.txt", List<String>(
														{
															 res.check_port_host,
															 toString(res.check_port_port),
															 res.check_port_result,
															 res.check_connect_host,
															 toString(res.check_connect_port),
															 res.check_connect_result,
															 res.resolve_domain,
															 out.str(),
															 res.find_free_host,
															 toString(res.find_free_count),
															 out2.str()
														 }));
	Request resp = makeRequest();
	resp->body = new char[html.size() + 1];
	strncpy(resp->body, html.c_str(), html.size());
	resp->body_length = html.size();
	return resp;
}

WebUI::UtilsStruct WebUI::makeUtilsStruct() const {
	UtilsStruct res;
	_RunParams p = ShadowSocksController::Get().getConfig().findDefaultRunParams();
	res.check_port_host = p.localHost;
	res.find_free_host = p.localHost;
	res.find_free_count = 5;
	res.check_port_port = p.localPort;
	return res;
}

Request WebUI::processPostUtils(Request req) {
	if (!ConteinsKey(req->post, "mode"))
		return HttpServer::generate404(req->method, req->host, req->path);

	String mode = req->post["mode"].value;

	if (ShadowSocksController::Get().getConfig().bootstrapDNS.size() > 4)
		__DP_LIB_NAMESPACE__::setGlobalDNS(ShadowSocksController::Get().getConfig().bootstrapDNS);


	if (mode == "check_port") {
		if (!ConteinsKey(req->post, "host") || !ConteinsKey(req->post, "port"))
			return HttpServer::generate404(req->method, req->host, req->path);
		UtilsStruct res = makeUtilsStruct();
		res.check_port_host = req->post["host"].value;
		res.check_port_port = parse<unsigned short>(req->post["port"].value);
		res.check_port_result = ShadowSocksClient::portIsAllow(res.check_port_host, res.check_port_port) ? findText("utils/check_port_allow.txt") : findText("utils/check_port_dinay.txt");
		return processGetUtils(req, res);
	}
	if (mode == "check_connect") {
		if (!ConteinsKey(req->post, "host") || !ConteinsKey(req->post, "port"))
			return HttpServer::generate404(req->method, req->host, req->path);
		UtilsStruct res = makeUtilsStruct();
		res.check_connect_host = req->post["host"].value;
		res.check_connect_port = parse<unsigned short>(req->post["port"].value);
		res.check_connect_result = __DP_LIB_NAMESPACE__::TCPClient::IsCanConnect(res.check_connect_host, res.check_connect_port) ? findText("utils/check_port_allow.txt") : findText("utils/check_port_dinay.txt");
		return processGetUtils(req, res);
	}
	if (mode == "resolve_domain") {
		if (!ConteinsKey(req->post, "host"))
			return HttpServer::generate404(req->method, req->host, req->path);
		UtilsStruct res = makeUtilsStruct();
		res.resolve_domain = req->post["host"].value;
		res.resolve_result = __DP_LIB_NAMESPACE__::resolveDomainList(res.resolve_domain);
		return processGetUtils(req, res);
	}
	if (mode == "free_ports") {
		if (!ConteinsKey(req->post, "host") || !ConteinsKey(req->post, "count"))
			return HttpServer::generate404(req->method, req->host, req->path);
		UtilsStruct res = makeUtilsStruct();
		res.find_free_host = req->post["host"].value;
		res.find_free_count = parse<unsigned short>(req->post["count"].value);
		for (unsigned short i = 0; i < res.find_free_count; i++)
			res.find_free_result.push_back(ShadowSocksClient::findAllowPort(res.find_free_host));
		return processGetUtils(req, res);
	}
	return HttpServer::generate404(req->method, req->host, req->path);
}
