#include "WebUI.h"
#include "../www/textfill.h"
#include <Converter/Converter.h>
#include "../ShadowSocksController.h"
#include <string.h>
#include <Parser/SmartParser.h>

using __DP_LIB_NAMESPACE__::OStrStream;
using __DP_LIB_NAMESPACE__::toString;
using __DP_LIB_NAMESPACE__::ConteinsKey;
using __DP_LIB_NAMESPACE__::parse;
using __DP_LIB_NAMESPACE__::trim;

Request WebUI::processGetServers(Request req) {
	OStrStream out;
	const auto & servers = ShadowSocksController::Get().getConfig().servers;
	for (_Server * s : servers) {
		String plugin = "-";
		String mode ="";
		String enable_tls = "";
		String path = "";


		if (dynamic_cast<_V2RayServer * >(s) != nullptr) {
			_V2RayServer * t = dynamic_cast<_V2RayServer *>(s);
			plugin = "v2ray";
			mode = t->mode;
			enable_tls = t->isTLS ? "+" : "-";
			path = t->isTLS ? "https://" : "http://";
			path += t->host + ":";
			path += toString(t->port);
			path += t->path;
			path = ShadowSocksController::Get().getConfig().replaceVariables(path);
		}
		String status = "";
		String status_color = "";
		if (s->check_result.isRun) {
			status_color = findText("servers/server_item_work.txt");
			status = "<p>IP: " + s->check_result.ipAddr + "</p>";
			if (s->check_result.speed_s.size() > 0)
				status += "<p>" + toString(s->check_result.speed) + " " + s->check_result.speed_s + "</p>";
		} else {
			status = s->check_result.msg;
			if (status.size() == 0)
				status_color = findText("servers/server_item_unknown.txt");
			else
				status_color = findText("servers/server_item_fail.txt");
		}

		out << findFillText("servers/server_item.txt", List<String>({
																status_color,
																toString(s->id),
																s->name,
																String(s->host + ":" + toString(s->port)),
																plugin,
																mode,
																enable_tls,
																path,
																status,
																toString(s->id),
																toString(s->id)
														  }));
	}

	String html = makePage("Servers", "servers/servers_index.txt", List<String>( { out.str()}));
	Request resp = makeRequest();
	resp->body = new char[html.size() + 1];
	strncpy(resp->body, html.c_str(), html.size());
	resp->body_length = html.size();
	return resp;
}

#define SaveSRV()\
	{	\
		if (ctrl.getConfig().CheckServer(sr)) { \
			ctrl.getConfig().servers.push_back(sr); \
			ctrl.SaveConfig(); \
		} else {\
			delete sr; \
			String html =  "Server is incorrect"; \
			Request resp = makeRequest(); \
			resp->status = 400; \
			resp->body = new char[html.size() + 1]; \
			strncpy(resp->body, html.c_str(), html.size()); \
			resp->body_length = html.size(); \
			return resp; \
		} \
	}

Request WebUI::processGetServerEditPage(Request req) {
	if (!ConteinsKey(req->get, "edit"))
		return HttpServer::generate404(req->method, req->host, req->path);
	auto & ctrl = ShadowSocksController::Get();
	_Server * _t = ctrl.getConfig().findServerById(parse<int>(req->get["edit"]));
	if (_t == nullptr)
		return HttpServer::generate404(req->method, req->host, req->path);

	bool is_v2ray = false;
	bool v_enable_tls = false;
	bool v_websocket = true;
	String v_host = "";
	String v_path = "";

	_V2RayServer * v = dynamic_cast<_V2RayServer * >(_t);
	if (v != nullptr) {
		is_v2ray = true;
		v_enable_tls = v->isTLS;
		v_websocket = v->mode == "websocket";
		v_host = v->v2host;
		v_path = v->path;
	}

	String html = makePage("New server", "servers/edit.txt", List<String>({
																toString(_t->id),
																_t->name,
																_t->host,
																_t->port,
																!is_v2ray ? findText("servers/checket_true.txt") : findText("servers/checket_false.txt"),
																is_v2ray ? findText("servers/checket_true.txt") : findText("servers/checket_false.txt"),
																v_enable_tls ? findText("servers/checket_true.txt") : findText("servers/checket_false.txt"),
																!v_enable_tls ? findText("servers/checket_true.txt") : findText("servers/checket_false.txt"),
																v_websocket ? findText("servers/checket_true.txt") : findText("servers/checket_false.txt"),
																!v_websocket ? findText("servers/checket_true.txt") : findText("servers/checket_false.txt"),
																v_host,
																v_path
															}));
	Request resp = makeRequest();
	resp->body = new char[html.size() + 1];
	strncpy(resp->body, html.c_str(), html.size());
	resp->body_length = html.size();
	return resp;
	return resp;
}

#define SaveSRV2()\
	{	\
		ctrl.getConfig().deleteServerById(_sr->id); \
		if (ctrl.getConfig().CheckServer(sr)) { \
			delete _sr; \
			_sr = nullptr; \
			ctrl.getConfig().servers.push_back(sr); \
			ctrl.SaveConfig(); \
		} else {\
			delete sr; \
			String html =  "Server is incorrect"; \
			Request resp = makeRequest(); \
			resp->status = 400; \
			resp->body = new char[html.size() + 1]; \
			strncpy(resp->body, html.c_str(), html.size()); \
			resp->body_length = html.size(); \
			return resp; \
		} \
	}
Request WebUI::processPostServerEditPage(Request req) {
	auto user_id = req->cookie["auth"];

	if (!ConteinsKey(req->get, "edit"))
		return HttpServer::generate404(req->method, req->host, req->path);
	auto & ctrl = ShadowSocksController::Get();
	_Server * _sr = ctrl.getConfig().findServerById(parse<int>(req->get["edit"]));
	if (_sr == nullptr)
		return HttpServer::generate404(req->method, req->host, req->path);

	DP_LOG_DEBUG << "WebUI (" << user_id << "): Edit server " << _sr->name;

	_Server * sr = _sr->Copy([](const String & txt) { return txt; });
	readParametr(sr->name, "name", sr);
	readParametr(sr->host, "host", sr);
	readParametr(sr->port, "port", sr);

	if (sr->port.size() > 1) {
		if (__DP_LIB_NAMESPACE__::startWithN(sr->port, "${") && __DP_LIB_NAMESPACE__::endWithN(sr->port, "}")) {

		} else
			if (!isNumber(sr->port)) {
				delete sr;
				String html =  "Port is invalid";
				Request resp = makeRequest();
				resp->status = 400;
				resp->body = new char[html.size() + 1];
				strncpy(resp->body, html.c_str(), html.size());
				resp->body_length = html.size();
				return resp;
			}
	}

	String plugin = "";
	readParametr(plugin, "plugin", sr);
	if (plugin == "v2ray") {
		_V2RayServer * t = dynamic_cast<_V2RayServer *>(sr);
		if (t == nullptr) {
			t = new _V2RayServer(sr);
			delete sr;
			sr = t;
		}
		readParametr(plugin, "v_tls", sr);
		t->isTLS = plugin == "yes";
		readParametr(t->mode, "v_mode", sr);
		readParametr(t->v2host, "v_host", sr);
		readParametr(t->path, "v_path", sr);
	}
	if (plugin == "none") {
		_Server * t = new _Server(*sr);
		delete sr;
		sr = t;
	}
	SaveSRV2();

	return makeRedirect(req, "/servers.html");
}
Request WebUI::processGetNewServerPage(Request req) {
	String html = makePage("New server", "servers/new.txt", List<String>({}));
	Request resp = makeRequest();
	resp->body = new char[html.size() + 1];
	strncpy(resp->body, html.c_str(), html.size());
	resp->body_length = html.size();
	return resp;
}
Request WebUI::processPostNewServerPage(Request req) {
	auto user_id = req->cookie["auth"];

	auto & ctrl = ShadowSocksController::Get();
	_Server * sr = new _Server();

	readParametr(sr->name, "name", sr);
	readParametr(sr->host, "host", sr);
	readParametr(sr->port, "port", sr);

	DP_LOG_DEBUG << "WebUI (" << user_id << "): Add server " << sr->name;

	if (sr->port.size() > 1) {
		if (__DP_LIB_NAMESPACE__::startWithN(sr->port, "${") && __DP_LIB_NAMESPACE__::endWithN(sr->port, "}")) {

		} else
			if (!isNumber(sr->port)) {
				delete sr;
				String html =  "Port is invalid";
				Request resp = makeRequest();
				resp->status = 400;
				resp->body = new char[html.size() + 1];
				strncpy(resp->body, html.c_str(), html.size());
				resp->body_length = html.size();
				return resp;
			}
	}

	String plugin = "";
	readParametr(plugin, "plugin", sr);
	if (plugin == "v2ray") {
		_V2RayServer * t = new _V2RayServer(sr);
		delete sr;
		sr = t;
		readParametr(plugin, "v_tls", sr);
		t->isTLS = plugin == "yes";
		readParametr(t->mode, "v_mode", sr);
		readParametr(t->v2host, "v_host", sr);
		readParametr(t->path, "v_path", sr);
	}
	SaveSRV();

	return makeRedirect(req, "/servers.html");
}

Request WebUI::processGetServerDelete(Request req) {
	auto user_id = req->cookie["auth"];

	if (!ConteinsKey(req->get, "delete"))
		return HttpServer::generate404(req->method, req->host, req->path);
	auto & ctrl = ShadowSocksController::Get();

	_Server * _sr = ctrl.getConfig().findServerById(parse<int>(req->get["delete"]));
	if (_sr == nullptr)
		return HttpServer::generate404(req->method, req->host, req->path);

	String html = makePage("Delete server " + _sr->name, "servers/delete.txt", List<String>({
															toString(_sr->id)
														}));

	Request resp = makeRequest();
	resp->body = new char[html.size() + 1];
	strncpy(resp->body, html.c_str(), html.size());
	resp->body_length = html.size();
	return resp;
}

Request WebUI::processPostServerDelete(Request req) {
	auto user_id = req->cookie["auth"];

	if (!ConteinsKey(req->get, "delete"))
		return HttpServer::generate404(req->method, req->host, req->path);
	auto & ctrl = ShadowSocksController::Get();

	_Server * _sr = ctrl.getConfig().findServerById(parse<int>(req->get["delete"]));
	if (_sr == nullptr)
		return HttpServer::generate404(req->method, req->host, req->path);

	DP_LOG_DEBUG << "WebUI (" << user_id << "): Delete server " << _sr->name;

	for (_Task * t : ctrl.getConfig().tasks)
		for (int srId : t->servers_id)
			if (srId == _sr->id) {
				notifyUser(user_id, "Can't remove this server. It use in task " + t->name);
				return makeRedirect(req, "/servers.html");
			}

	ctrl.getConfig().deleteServerById(_sr->id);
	ctrl.SaveConfig();

	return makeRedirect(req, "/servers.html");
}
