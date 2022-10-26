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
using __DP_LIB_NAMESPACE__::SmartParser;

Request WebUI::processGetServers(Request req) {
	String filter_group = "";
	bool force_cookie = false;
	if (ConteinsKey(req->get, "group"))
		filter_group = req->get["group"];
	String filter_name = "";
	if (ConteinsKey(req->get, "name"))
		filter_name = req->get["name"];
	if (ConteinsKey(req->cookie, "s_group") && filter_group.size() == 0 && !ConteinsKey(req->get, "group"))
		return makeRedirect(req, "/servers.html?group=" + req->cookie["s_group"] + (filter_name.size() == 0 ? "" : "&name=" + filter_name));
	if (filter_group.size() == 0 && ConteinsKey(req->get, "group"))
		force_cookie = true;

	SmartParser filter_name_parser{"*" + filter_name + "*"};

	OStrStream out;
	const auto & servers = ShadowSocksController::Get().getConfig().servers;
	OStrStream group_gen;
	group_gen << findFillText("servers/index_group_list_item.txt", List<String>( {
																						 ( filter_group == "" ) ? findText("servers/index_group_list_item_selected.txt") : findText("servers/index_group_list_item_unselected.txt"),
																						 ""
																					 }));
	__DP_LIB_NAMESPACE__::Map<String, bool> added;
	for (_Server * s : servers) {
		if (!ConteinsKey(added, s->group) && s->group.size() > 0) {
			group_gen << findFillText("servers/index_group_list_item.txt", List<String>( {
																								 ( s->group.size() > 0 && s->group == filter_group ) ? findText("servers/index_group_list_item_selected.txt") : findText("servers/index_group_list_item_unselected.txt"),
																								 s->group
																							 }));
			added[s->group] = true;
		}

		if (filter_group.size() > 0 && filter_group != s->group)
			continue;
		if (filter_name.size() > 0 && !filter_name_parser.Check(s->name))
			continue;

		String path = "";


		if (dynamic_cast<_V2RayServer * >(s) != nullptr) {
			_V2RayServer * t = dynamic_cast<_V2RayServer *>(s);
			path = t->isTLS ? "https://" : "http://";
			path += t->host + ":";
			path += toString(t->port);
			path += t->path;
			path = ShadowSocksController::Get().getConfig().replaceVariables(path);
		} else {
			path = "ss://";
			path += s->host + ":";
			path += toString(s->port);
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
																path,
																status,
																toString(s->id),
																toString(s->id)
														  }));
	}

	for (_Task * t : ShadowSocksController::Get().getConfig().tasks)
		if (!ConteinsKey(added, t->group) && t->group.size() > 0) {
			group_gen << findFillText("servers/index_group_list_item.txt", List<String>( {
																								 ( t->group.size() > 0 && t->group == filter_group ) ? findText("servers/index_group_list_item_selected.txt") : findText("servers/index_group_list_item_unselected.txt"),
																								 t->group
																							 }));
			added[t->group] = true;
		}

	String html = makePage("Servers", "servers/servers_index.txt", List<String>( { filter_name, group_gen.str(), out.str()}));
	Request resp = makeRequest();
	if (filter_group.size() != 0 || force_cookie)
		resp->cookie["s_group"] = filter_group;
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

	OStrStream group_gen;
	__DP_LIB_NAMESPACE__::Map<String, bool> added;
	for (_Server * t : ShadowSocksController::Get().getConfig().servers)
		if (!ConteinsKey(added, t->group) && t->group.size() > 0) {
			group_gen << findFillText("servers/group_list_item.txt", List<String>( {t->group}));
			added[t->group] = true;
		}
	for (_Task * t : ShadowSocksController::Get().getConfig().tasks)
		if (!ConteinsKey(added, t->group) && t->group.size() > 0) {
			group_gen << findFillText("servers/group_list_item.txt", List<String>( {t->group}));
			added[t->group] = true;
		}

	String html = makePage("New server", "servers/edit.txt", List<String>({
																toString(_t->id),
																_t->name,
																_t->group,
																group_gen.str(),
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
	readParametr_n(sr->group, "group");
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
Request WebUI::processGetNewServerPage(Request) {
	OStrStream group_gen;
	__DP_LIB_NAMESPACE__::Map<String, bool> added;
	for (_Server * t : ShadowSocksController::Get().getConfig().servers)
		if (!ConteinsKey(added, t->group) && t->group.size() > 0) {
			group_gen << findFillText("servers/group_list_item.txt", List<String>( {t->group}));
			added[t->group] = true;
		}
	for (_Task * t : ShadowSocksController::Get().getConfig().tasks)
		if (!ConteinsKey(added, t->group) && t->group.size() > 0) {
			group_gen << findFillText("servers/group_list_item.txt", List<String>( {t->group}));
			added[t->group] = true;
		}

	String html = makePage("New server", "servers/new.txt", List<String>({group_gen.str()}));
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
	readParametr_n(sr->group, "group");
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
