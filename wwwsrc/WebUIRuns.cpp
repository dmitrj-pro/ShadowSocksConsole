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

/*


Request WebUI::processPostTaskEdit(Request req) {
	auto user_id = req->cookie["auth"];
	if (!ConteinsKey(req->get, "edit"))
		return HttpServer::generate404(req->method, req->host, req->path);
	auto & ctrl = ShadowSocksController::Get();
	_Task * sr = ctrl.getConfig().findTaskById(parse<int>(req->get["edit"]));
	if (sr == nullptr)
		return HttpServer::generate404(req->method, req->host, req->path);

	DP_LOG_DEBUG << "WebUI (" << user_id << "): Edit task " << sr->name;

	_Task * tk = sr->Copy([](const String & txt) { return txt; });


	readParametr(tk->name, "name", tk);
	readParametr(tk->method, "method", tk);
	readParametr(tk->password, "password", tk);
	readParametr_n(tk->localHost, "local_host");
	readParametr_ntv(tk->localPort, "local_port", int, -1);
	readParametr_ntv(tk->httpProxy, "local_http_port", int, -1);
	readParametr_boolv(tk->systemProxy, "sysproxy", false);
	readParametr_boolv(tk->enable_ipv6, "ipv6", false);
	readParametr_boolv(tk->autostart, "autostart", false);
	readParametr_n(tk->tun2SocksName, "vpn");
	if (tk->tun2SocksName == "none")
		tk->tun2SocksName = "";
*/

Request WebUI::processGetRuns(Request) {
	OStrStream out;
	const auto & tuns = ShadowSocksController::Get().getConfig().runParams;
	for (auto t : tuns) {
		OStrStream ignoreIp;
		OStrStream dns;

		out << findFillText("runs/index_item.txt", List<String>({
																t.name,
																(t.localHost + ":" + toString(t.localPort)),
																(t.httpProxy > 1 ? (t.localHost + ":" + toString(t.httpProxy)) : "Off"),
																(t.systemProxy ? "Enable" : "Disable"),
																(t.multimode ? "Enable" : "Disable"),
																t.tun2SocksName,
																t.name,
																t.name
														  }));
	}

	String html = makePage("Run Parametrs", "runs/index.txt", List<String>( { out.str()}));
	Request resp = makeRequest();
	resp->body = new char[html.size() + 1];
	strncpy(resp->body, html.c_str(), html.size());
	resp->body_length = html.size();
	return resp;
}

Request WebUI::processGetAddRuns(Request) {
	_RunParams params = ShadowSocksController::Get().getConfig().findDefaultRunParams();

	OStrStream vpn_gen;
	const auto & tuns = ShadowSocksController::Get().getConfig().tun2socksConf;
	for (auto t : tuns) {
		vpn_gen << findFillText("runs/new_vpn.txt", List<String>({
																			 t.name,
																			 t.name}));
	}

	String html = makePage("New run parametr", "runs/new.txt", List<String>({
																				params.localHost,
																				toString(params.localPort),
																				toString(params.httpProxy),
																				params.systemProxy ?
																					findText("runs/new_checked_true.txt"):
																					findText("runs/new_checked_false.txt"),
																				!params.systemProxy ?
																					findText("runs/new_checked_true.txt"):
																					findText("runs/new_checked_false.txt"),
																				params.multimode ?
																					findText("runs/new_checked_true.txt"):
																					findText("runs/new_checked_false.txt"),
																				!params.multimode ?
																					findText("runs/new_checked_true.txt"):
																					findText("runs/new_checked_false.txt"),
																				vpn_gen.str()
																			}));
	Request resp = makeRequest();
	resp->body = new char[html.size() + 1];
	strncpy(resp->body, html.c_str(), html.size());
	resp->body_length = html.size();
	return resp;
}

Request WebUI::processPostAddRuns(Request req) {
	auto user_id = req->cookie["auth"];

	_RunParams cnf;

	readParametr_nd(cnf.name, "runname");
	DP_LOG_DEBUG << "WebUI (" << user_id << "): Add run parametr " << cnf.name;
	readParametr_nd(cnf.localHost, "local_host");
	readParametr_nt(cnf.localPort, "local_port", int);
	readParametr_nt(cnf.httpProxy, "local_http_port", int);
	String rem = SSTtypetoString(_RunParams::ShadowSocksType::None);
	readParametr_nd(rem, "shadowSocksType");
	if (rem == "default") rem = SSTtypetoString(_RunParams::ShadowSocksType::None);
	cnf.shadowsocks_type = parseSSType(rem);
	readParametr_nd(rem, "sysproxy");
	cnf.systemProxy = rem == "yes";
	readParametr_nd(rem, "multimode");
	cnf.multimode = rem == "yes";
	readParametr_nd(rem, "vpn");
	if (rem == "none")
		cnf.tun2SocksName = "";
	else {
		cnf.tun2SocksName = rem;
	}
	cnf.isNull = false;
	if (!ShadowSocksController::Get().getConfig().IsCorrect(cnf)) {
		Request resp = makeRequest();
		resp->status = 400;
		String html = makePage("Run parametr is incorrect", "Run parametr is incorrect", "Run parametr is incorrect");

		resp->body = new char[html.size() + 1];
		strncpy(resp->body, html.c_str(), html.size());
		resp->body_length = html.size();
		return resp;
	}
	ShadowSocksController::Get().getConfig().runParams.push_back(cnf);

	ShadowSocksController::Get().SaveConfig();

	return makeRedirect(req, "/runs.html");
}

Request WebUI::processGetEditRuns(Request req) {
	if (!ConteinsKey(req->get, "edit"))
		return HttpServer::generate404(req->method, req->host, req->path);

	auto & ctrl = ShadowSocksController::Get();
	_RunParams cnf = ctrl.getConfig().findRunParamsbyName(req->get["edit"]);
	if (cnf.isNull)
		return HttpServer::generate404(req->method, req->host, req->path);

	OStrStream vpn_gen;
	const auto & tuns = ShadowSocksController::Get().getConfig().tun2socksConf;
	for (auto t : tuns) {
		vpn_gen << findFillText("runs/edit_vpn.txt", List<String>({
																	t.name,
																	cnf.tun2SocksName == t.name ?
																		findText("runs/new_checked_true.txt"):
																		findText("runs/new_checked_false.txt"),
																	t.name}));
	}

	String html = makePage("Edit run parametr " + cnf.name, "runs/edit.txt", List<String>({
																							  cnf.localHost,
																							  toString(cnf.localPort),
																							  toString(cnf.httpProxy),
																							  cnf.systemProxy ?
																								findText("runs/new_checked_true.txt"):
																								findText("runs/new_checked_false.txt"),
																							  !cnf.systemProxy ?
																								findText("runs/new_checked_true.txt"):
																								findText("runs/new_checked_false.txt"),
																							  cnf.multimode ?
																								findText("runs/new_checked_true.txt"):
																								findText("runs/new_checked_false.txt"),
																							  !cnf.multimode ?
																								findText("runs/new_checked_true.txt"):
																								findText("runs/new_checked_false.txt"),
																							  cnf.shadowsocks_type == _RunParams::ShadowSocksType::None ?
																								findText("runs/new_checked_true.txt"):
																								findText("runs/new_checked_false.txt"),
																							  cnf.shadowsocks_type == _RunParams::ShadowSocksType::GO ?
																								findText("runs/new_checked_true.txt"):
																								findText("runs/new_checked_false.txt"),
																							  cnf.shadowsocks_type == _RunParams::ShadowSocksType::Rust ?
																								findText("runs/new_checked_true.txt"):
																								findText("runs/new_checked_false.txt"),
																							  cnf.tun2SocksName.size() == 0 ?
																								findText("runs/new_checked_true.txt"):
																								findText("runs/new_checked_false.txt"),
																							  vpn_gen.str()
																						}));
	Request resp = makeRequest();
	resp->body = new char[html.size() + 1];
	strncpy(resp->body, html.c_str(), html.size());
	resp->body_length = html.size();
	return resp;
}

Request WebUI::processPostEditRuns(Request req) {
	auto user_id = req->cookie["auth"];

	if (!ConteinsKey(req->get, "edit"))
		return HttpServer::generate404(req->method, req->host, req->path);

	auto & ctrl = ShadowSocksController::Get();
	_RunParams cnf = ctrl.getConfig().findRunParamsbyName(req->get["edit"]);
	if (cnf.isNull)
		return HttpServer::generate404(req->method, req->host, req->path);

	DP_LOG_DEBUG << "WebUI (" << user_id << "): Edit run parametr " << cnf.name;
	readParametr_nd(cnf.localHost, "local_host");
	readParametr_nt(cnf.localPort, "local_port", int);
	readParametr_nt(cnf.httpProxy, "local_http_port", int);
	String rem = SSTtypetoString(_RunParams::ShadowSocksType::None);
	readParametr_nd(rem, "shadowSocksType");
	if (rem == "default") rem = SSTtypetoString(_RunParams::ShadowSocksType::None);
	cnf.shadowsocks_type = parseSSType(rem);
	readParametr_nd(rem, "sysproxy");
	cnf.systemProxy = rem == "yes";
	readParametr_nd(rem, "multimode");
	cnf.multimode = rem == "yes";
	readParametr_nd(rem, "vpn");
	if (rem == "none")
		cnf.tun2SocksName = "";
	else {
		cnf.tun2SocksName = rem;
	}

	_RunParams cnf2 = ctrl.getConfig().findRunParamsbyName(req->get["edit"]);
	ctrl.getConfig().deleteRunParamsByName(cnf2.name);

	if (!ShadowSocksController::Get().getConfig().IsCorrect(cnf)) {
		ctrl.getConfig().runParams.push_back(cnf2);
		Request resp = makeRequest();
		resp->status = 400;
		String html = makePage("Run parametr is incorrect", "Run parametr is incorrect", "Run parametr is incorrect");

		resp->body = new char[html.size() + 1];
		strncpy(resp->body, html.c_str(), html.size());
		resp->body_length = html.size();
		return resp;
	}
	ShadowSocksController::Get().getConfig().runParams.push_back(cnf);

	ShadowSocksController::Get().SaveConfig();

	return makeRedirect(req, "/runs.html");
}

Request WebUI::processGetDeleteRuns(Request req) {
	if (!ConteinsKey(req->get, "delete"))
		return HttpServer::generate404(req->method, req->host, req->path);
	auto & ctrl = ShadowSocksController::Get();

	_RunParams conf = ctrl.getConfig().findRunParamsbyName(req->get["delete"]);
	if (conf.isNull)
		return HttpServer::generate404(req->method, req->host, req->path);

	String html = makePage("Delete run parametr " + conf.name, "runs/delete.txt", List<String>({
															toString(conf.name)
														}));

	Request resp = makeRequest();
	resp->body = new char[html.size() + 1];
	strncpy(resp->body, html.c_str(), html.size());
	resp->body_length = html.size();
	return resp;
}

Request WebUI::processPostDeleteRuns(Request req) {
	if (!ConteinsKey(req->get, "delete"))
		return HttpServer::generate404(req->method, req->host, req->path);
	auto & ctrl = ShadowSocksController::Get();

	_RunParams conf = ctrl.getConfig().findRunParamsbyName(req->get["delete"]);

	if (conf.isNull)
		return HttpServer::generate404(req->method, req->host, req->path);

	auto user_id = req->cookie["auth"];
	DP_LOG_DEBUG << "WebUI (" << user_id << "): Delete run parametr " << conf.name;

	for (const _Task * t : ctrl.getConfig().tasks)
		if (t->runParamsName == conf.name) {
			notifyUser(user_id, "Can't remove this run parametr. It use in task " + t->name);
			return makeRedirect(req, "/runs.html");
		}
	if (conf.name == "DEFAULT") {
		notifyUser(user_id, "Can't remove default running parametr");
		return makeRedirect(req, "/runs.html");
	}

	ctrl.getConfig().deleteRunParamsByName(conf.name);
	ctrl.SaveConfig();

	return makeRedirect(req, "/runs.html");
}
