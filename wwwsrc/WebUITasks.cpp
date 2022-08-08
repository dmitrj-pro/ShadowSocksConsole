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
using __DP_LIB_NAMESPACE__::SmartParser;

Request WebUI::processPostTaskStartPage(Request req) {
	auto user_id = req->cookie["auth"];

	if (!ConteinsKey(req->get, "start"))
		return HttpServer::generate404(req->method, req->host, req->path);
	auto & ctrl = ShadowSocksController::Get();
	_Task * t = ctrl.getConfig().findTaskById(parse<int>(req->get["start"]));
	if (t == nullptr)
		return HttpServer::generate404(req->method, req->host, req->path);

	DP_LOG_DEBUG << "WebUI (" << user_id << "): Custom start task " << t->name;

	try{
		auto funcCrash = [this, user_id] (const String & name, const ExitStatus & status) {
			auto & ctrl = ShadowSocksController::Get();
			ctrl.StopByName(name);
			notifyUser(user_id, "Fail to start task " + name + ": " + status.str);
		};

		SSClientFlags flags;
		flags.checkServerMode = ctrl.getConfig().checkServerMode;


		String tmp = "";
		readParametr_n(tmp, "CHECK");
		if (tmp == "no")
			flags.checkServerMode = ServerCheckingMode::Off;
		readParametr_n(tmp, "NOVPN");
		flags.runVPN = tmp == "no";

		tmp = SSTtypetoString(_RunParams::ShadowSocksType::None);
		readParametr_n(tmp, "shadowSocksType");
		if (tmp == "default") tmp = SSTtypetoString(_RunParams::ShadowSocksType::None);
		flags.type = parseSSType(tmp);

		readParametr_n(tmp, "sysproxy");
		flags.sysproxy_s = tmp == "yes" ? SSClientFlagsBoolStatus::True : (tmp == "no" ? SSClientFlagsBoolStatus::False : SSClientFlagsBoolStatus::None);

		readParametr_n(tmp, "multimode");
		flags.multimode = tmp == "yes" ? SSClientFlagsBoolStatus::True : (tmp == "no" ? SSClientFlagsBoolStatus::False : SSClientFlagsBoolStatus::None);

		int _port = 0;
		readParametr_nt(_port, "local_port", int);
		if (_port < 0 || _port > 65536)
			flags.port = 0;
		else
			flags.port = _port;

		_port = 0;
		readParametr_nt(_port, "local_http_port", int);
		if (_port < 0 || _port > 65536)
			flags.http_port = 0;
		else
			flags.http_port = _port;


		readParametr_n(flags.vpnName, "vpn");
		tmp = "";
		readParametr_n(tmp, "server");
		if (tmp.size() > 0)
			flags.server_name = tmp;


		try{
			ctrl.StartByName(t->name, [this, user_id] (const String & name) {
				notifyUser(user_id, "Task " + name + " succesfully started\n");
			}, funcCrash, flags);
		} catch (__DP_LIB_NAMESPACE__::LineException e) {
			notifyUser(user_id, "Fail to start task " + t->name + ": " + e.toString());
		}

		return makeRedirect(req, "/tasks.html");
	} catch (__DP_LIB_NAMESPACE__::LineException e) {
		notifyUser(user_id, "Fail to start task " + t->name + ": " + e.toString());
	} catch (...) {
		notifyUser(user_id, "Fail to start task " + t->name);
	}
	Request resp = makeRequest();
	resp->headers["Cache-Control"] = "no-cache";
	resp->status = 500;
	return resp;
}

Request WebUI::processPostTaskStart(Request req) {
	auto user_id = req->cookie["auth"];

	if (!ConteinsKey(req->post, "start"))
		return HttpServer::generate404(req->method, req->host, req->path);
	auto & ctrl = ShadowSocksController::Get();
	_Task * t = ctrl.getConfig().findTaskById(parse<int>(req->post["start"].value));
	if (t == nullptr)
		return HttpServer::generate404(req->method, req->host, req->path);
	DP_LOG_DEBUG << "WebUI (" << user_id << "): Start task " << t->name;
	try{
		auto funcCrash = [this, user_id] (const String & name, const ExitStatus & status) {
			auto & ctrl = ShadowSocksController::Get();
			ctrl.StopByName(name);
			notifyUser(user_id, "Fail to start task " + name + ": " + status.str);
		};

		SSClientFlags flags;

		ctrl.StartByName(t->name, [this, user_id] (const String & name) {
			notifyUser(user_id, "Task " + name + " succesfully started\n");
		}, funcCrash, flags);

		return makeRedirect(req, "/tasks.html");
	} catch (__DP_LIB_NAMESPACE__::LineException e) {
		notifyUser(user_id, "Fail to start task " + t->name + ": " + e.toString());
	} catch (...) {
		notifyUser(user_id, "Fail to start task " + t->name);
	}
	Request resp = makeRequest();
	resp->status = 500;
	return resp;
}

Request WebUI::processGetTaskDelete(Request req) {
	if (!ConteinsKey(req->get, "delete"))
		return HttpServer::generate404(req->method, req->host, req->path);
	auto & ctrl = ShadowSocksController::Get();
	_Task * t = ctrl.getConfig().findTaskById(parse<int>(req->get["delete"]));
	if (t == nullptr)
		return HttpServer::generate404(req->method, req->host, req->path);

	String html = makePage("Delete task " + t->name, "tasks/tasks_delete.txt", List<String>({
																	toString(t->id)
																}));
	Request resp = makeRequest();
	resp->body = new char[html.size() + 1];
	strncpy(resp->body, html.c_str(), html.size());
	resp->body_length = html.size();
	return resp;
}

Request WebUI::processPostTaskTuns(Request req) {
	if (!ConteinsKey(req->get, "edit"))
		return HttpServer::generate404(req->method, req->host, req->path);
	auto & ctrl = ShadowSocksController::Get();
	_Task * t = ctrl.getConfig().findTaskById(parse<int>(req->get["edit"]));
	if (t == nullptr)
		return HttpServer::generate404(req->method, req->host, req->path);

	t->tuns.clear();

	__DP_LIB_NAMESPACE__::SmartParser parser ("proto_${id:int}");
	for (auto it : req->post) {
		if (parser.Check(it.first)) {
			String id = parser.Get("id");
			if (!ConteinsKey(req->post, "local_port_" + id) || !ConteinsKey(req->post, "remote_host_" + id) || !ConteinsKey(req->post, "remote_port_" + id))
				return HttpServer::generate404(req->method, req->host, req->path);
			_Tun tun;
			String tmp = req->post["proto_" + id].value;
			if (tmp == "udp") tun.type = TunType::UDP;
			if (tmp == "tcp") tun.type = TunType::TCP;
			tun.remoteHost = req->post["remote_host_" + id].value;
			tmp = req->post["local_port_" + id].value;
			tun.localPort = parse<UInt>(tmp);
			tmp = req->post["remote_port_" + id].value;
			tun.remotePort = parse<UInt>(tmp);
			t->tuns.push_back(tun);
		}
	}

	ctrl.SaveConfig();
	return makeRedirect(req, "/tasks.html");

}

Request WebUI::processGetTaskTuns(Request req) {
	if (!ConteinsKey(req->get, "edit"))
		return HttpServer::generate404(req->method, req->host, req->path);
	auto & ctrl = ShadowSocksController::Get();
	_Task * t = ctrl.getConfig().findTaskById(parse<int>(req->get["edit"]));
	if (t == nullptr)
		return HttpServer::generate404(req->method, req->host, req->path);

	OStrStream out;
	int i = 0;
	for (const _Tun & tun : t->tuns) {
		i++;
		out << findFillText("tasks/tuns_item.txt", List<String>({
																	toString(i),
																	toString(i),
																	tun.type == TunType::TCP ?
																		findText("tasks/tasks_edit_checket_true.txt") :
																		findText("tasks/tasks_edit_checket_false.txt"),
																	toString(i),
																	tun.type == TunType::UDP ?
																		findText("tasks/tasks_edit_checket_true.txt") :
																		findText("tasks/tasks_edit_checket_false.txt"),
																	toString(i),
																	toString(i),
																	toString(i),
																	toString(tun.localPort),
																	toString(i),
																	toString(i),
																	toString(i),
																	tun.remoteHost,
																	toString(i),
																	toString(i),
																	toString(i),
																	toString(tun.remotePort),
																	toString(i)
																}));
	}

	String html = makePage("Edit tuns", "tasks/tuns.txt", List<String>({ toString(t->id), out.str(), toString (++i)}));
	Request resp = makeRequest();
	resp->body = new char[html.size() + 1];
	strncpy(resp->body, html.c_str(), html.size());
	resp->body_length = html.size();
	return resp;
}

Request WebUI::processPostTaskDelete(Request req) {
	auto user_id = req->cookie["auth"];

	if (!ConteinsKey(req->get, "delete"))
		return HttpServer::generate404(req->method, req->host, req->path);
	auto & ctrl = ShadowSocksController::Get();
	_Task * t = ctrl.getConfig().findTaskById(parse<int>(req->get["delete"]));
	if (t == nullptr)
		return HttpServer::generate404(req->method, req->host, req->path);
	DP_LOG_DEBUG << "WebUI (" << user_id << "): Delete task " << t->name;
	ctrl.getConfig().deleteTaskById(t->id);
	ctrl.SaveConfig();

	return makeRedirect(req, "/tasks.html");
}

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
	readParametr(tk->group, "group", tk);
	readParametr(tk->method, "method", tk);
	readParametr(tk->password, "password", tk);
	readParametr_boolv(tk->enable_ipv6, "ipv6", false);
	readParametr_boolv(tk->enable_udp, "enable_udp", false);
	readParametr_boolv(tk->autostart, "autostart", false);
	readParametr_n(tk->runParamsName, "runParams");
	if (req->post.find("servers") != req->post.end() && req->post["servers"].value_size > 0) {
		tk->servers_id.clear();
		PostData * p = &req->post["servers"];
		do {
			_Server * s = ctrl.getConfig().findServerByName(trim(p->value));
			if (s != nullptr)
				tk->servers_id.push_back(s->id);
			p = p->next;
		} while (p != nullptr);
	}
	ctrl.getConfig().deleteTaskById(tk->id);
	if (!ctrl.getConfig().CheckTask(tk)) {
		ctrl.getConfig().tasks.push_back(sr);
		delete tk;
		String html =  "Fail to check task. See help\n";
		Request resp = makeRequest();
		resp->status = 400;
		resp->body = new char[html.size() + 1];
		strncpy(resp->body, html.c_str(), html.size());
		resp->body_length = html.size();
		delete tk;
		return resp;
	}
	ctrl.getConfig().tasks.push_back(tk);
	delete sr;
	ctrl.SaveConfig();

	return makeRedirect(req, "/tasks.html");
}

Request WebUI::processGetTaskEditPage(Request req) {
	auto user_id = req->cookie["auth"];
	if (!ConteinsKey(req->get, "edit"))
		return HttpServer::generate404(req->method, req->host, req->path);
	auto & ctrl = ShadowSocksController::Get();
	_Task * _t = ctrl.getConfig().findTaskById(parse<int>(req->get["edit"]));
	if (_t == nullptr)
		return HttpServer::generate404(req->method, req->host, req->path);

	OStrStream vpn_gen;
	const auto & tuns = ShadowSocksController::Get().getConfig().runParams;
	for (auto t : tuns) {
		vpn_gen << findFillText("tasks/tasks_edit_runparams.txt", List<String>({
																			 t.name,
																			 _t->runParamsName == t.name ?
																				findText("tasks/tasks_edit_checket_true.txt") :
																				findText("tasks/tasks_edit_checket_false.txt"),
																			 t.name}));
	}

	OStrStream server_gen;
	const auto & servers = ShadowSocksController::Get().getConfig().servers;
	for (const _Server * s : servers)
		server_gen << findFillText("tasks/tasks_edit_server.txt", List<String>({
																					s->group,
																					s->name,
																					__DP_LIB_NAMESPACE__::ConteinsElement(_t->servers_id, s->id) ?
																						findText("tasks/tasks_edit_selected_true.txt") :
																						findText("tasks/tasks_edit_selected_false.txt"),
																					s->name
																			   }));


	OStrStream group_gen;
	__DP_LIB_NAMESPACE__::Map<String, bool> added;
	for (_Task * t : ShadowSocksController::Get().getConfig().tasks)
		if (!ConteinsKey(added, t->group) && t->group.size() > 0) {
			group_gen << findFillText("tasks/task_group_list_item.txt", List<String>( {t->group}));
			added[t->group] = true;
		}
	for (_Server * t : ShadowSocksController::Get().getConfig().servers)
		if (!ConteinsKey(added, t->group) && t->group.size() > 0) {
			group_gen << findFillText("tasks/task_group_list_item.txt", List<String>( {t->group}));
			added[t->group] = true;
		}



	String html = makePage("Edit task " + _t->name, "tasks/tasks_edit.txt", List<String>({
															toString(_t->id),
															_t->name,
															_t->group,
															group_gen.str(),
															_t->method == "AEAD_CHACHA20_POLY1305" ?
																findText("tasks/tasks_edit_checket_true.txt") :
																findText("tasks/tasks_edit_checket_false.txt"),
															 _t->method == "AEAD_AES_128_GCM" ?
																 findText("tasks/tasks_edit_checket_true.txt") :
																 findText("tasks/tasks_edit_checket_false.txt"),
															 _t->method == "AEAD_AES_256_GCM" ?
																 findText("tasks/tasks_edit_checket_true.txt") :
																 findText("tasks/tasks_edit_checket_false.txt"),
															_t->password,
															_t->enable_ipv6 ?
																findText("tasks/tasks_edit_checket_true.txt") :
																findText("tasks/tasks_edit_checket_false.txt"),
															!_t->enable_ipv6 ?
																findText("tasks/tasks_edit_checket_true.txt") :
																findText("tasks/tasks_edit_checket_false.txt"),
															_t->enable_udp ?
																findText("tasks/tasks_edit_checket_true.txt") :
																findText("tasks/tasks_edit_checket_false.txt"),
															!_t->enable_udp ?
																findText("tasks/tasks_edit_checket_true.txt") :
																findText("tasks/tasks_edit_checket_false.txt"),
															_t->autostart ?
																findText("tasks/tasks_edit_checket_true.txt") :
																findText("tasks/tasks_edit_checket_false.txt"),
															!_t->autostart ?
																findText("tasks/tasks_edit_checket_true.txt") :
																findText("tasks/tasks_edit_checket_false.txt"),
															vpn_gen.str(),
															server_gen.str()
														}));
	Request resp = makeRequest();
	resp->body = new char[html.size() + 1];
	strncpy(resp->body, html.c_str(), html.size());
	resp->body_length = html.size();
	return resp;
}

Request WebUI::processPostNewTaskPage(Request req) {
	auto user_id = req->cookie["auth"];

	_Task * tk = new _Task();
	auto & ctrl = ShadowSocksController::Get();

	readParametr(tk->name, "name", tk);
	readParametr(tk->group, "group", tk);
	readParametr(tk->method, "method", tk);
	readParametr(tk->password, "password", tk);
	readParametr_boolv(tk->enable_ipv6, "ipv6", false);
	readParametr_boolv(tk->enable_udp, "enable_udp", false);
	readParametr_boolv(tk->autostart, "autostart", false);
	readParametr_n(tk->runParamsName, "runParams");
	if (req->post.find("servers") != req->post.end() && req->post["servers"].value_size > 0) {
		tk->servers_id.clear();
		PostData * p = &req->post["servers"];
		do {
			_Server * s = ctrl.getConfig().findServerByName(trim(p->value));
			if (s != nullptr)
				tk->servers_id.push_back(s->id);
			p = p->next;
		} while (p != nullptr);
	}
	DP_LOG_DEBUG << "WebUI (" << user_id << "): Add task " << tk->name;

	if (!ctrl.getConfig().CheckTask(tk)) {
		String html =  "Fail to check task. See help\n";
		Request resp;
		resp->status = 400;
		resp->body = new char[html.size() + 1];
		strncpy(resp->body, html.c_str(), html.size());
		resp->body_length = html.size();
		delete tk;
		return resp;

	}
	ctrl.getConfig().tasks.push_back(tk);
	ctrl.SaveConfig();

	return makeRedirect(req, "/tasks.html");
}

Request WebUI::processPostCheckServers(Request req) {
	if (!ConteinsKey(req->post, "mode") || !ConteinsKey(req->post, "checkIpUrl") || !ConteinsKey(req->post, "downloadUrl"))
		return HttpServer::generate404(req->method, req->host, req->path);

	_ShadowSocksController::CheckLoopStruct args = ShadowSocksController::Get().makeCheckStruct();
	args.auto_check_mode = str_to_AutoCheckingMode(req->post["mode"].value);
	args._task_name = "";
	args._server_name = "";
	args.auto_check_interval_s = 0;
	args.checkIpUrl = req->post["checkIpUrl"].value;
	args.downloadUrl = req->post["downloadUrl"].value;
	args.save_last_check = true;
	ShadowSocksController::Get().check_loop(args);

	return makeRedirect(req, "/tasks.html");
}

Request WebUI::processGetCheckServers(Request) {
	_ShadowSocksController::CheckLoopStruct args = ShadowSocksController::Get().makeCheckStruct();

	String html = makePage("Check tasks", "tasks/check.txt", List<String>({ args.checkIpUrl, args.downloadUrl }));
	Request resp = makeRequest();
	resp->body = new char[html.size() + 1];
	strncpy(resp->body, html.c_str(), html.size());
	resp->body_length = html.size();
	return resp;
}

Request WebUI::processPostCheckTask(Request req) {
	if (!ConteinsKey(req->get, "check"))
		return HttpServer::generate404(req->method, req->host, req->path);

	if (!ConteinsKey(req->post, "mode") || !ConteinsKey(req->post, "checkIpUrl") || !ConteinsKey(req->post, "downloadUrl"))
		return HttpServer::generate404(req->method, req->host, req->path);

	auto & ctrl =  ShadowSocksController::Get();
	_Task * t = ctrl.getConfig().findTaskById(parse<int>(req->get["check"]));
	if (t == nullptr)
		return HttpServer::generate404(req->method, req->host, req->path);

	_ShadowSocksController::CheckLoopStruct args = ShadowSocksController::Get().makeCheckStruct();
	args.auto_check_mode = str_to_AutoCheckingMode(req->post["mode"].value);
	args._task_name = "";
	args._server_name = "";
	args.auto_check_interval_s = 0;
	args.checkIpUrl = req->post["checkIpUrl"].value;
	args.downloadUrl = req->post["downloadUrl"].value;
	args.save_last_check = true;

	auto servers = t->servers_id;
	for (int id : servers) {
		_Server * sr = ctrl.getConfig().findServerById(id);
		ctrl.check_server(sr, t, args);
	}
	ctrl.SaveCashe();

	return makeRedirect(req, "/tasks.html");

}
Request WebUI::processGetCheckTask(Request req) {
	if (!ConteinsKey(req->get, "check"))
		return HttpServer::generate404(req->method, req->host, req->path);

	_Task * t = ShadowSocksController::Get().getConfig().findTaskById(parse<int>(req->get["check"]));
	if (t == nullptr)
		return HttpServer::generate404(req->method, req->host, req->path);

	_ShadowSocksController::CheckLoopStruct args = ShadowSocksController::Get().makeCheckStruct();

	String html = makePage("Check task", "tasks/check.txt", List<String>({ args.checkIpUrl, args.downloadUrl }));
	Request resp = makeRequest();
	resp->body = new char[html.size() + 1];
	strncpy(resp->body, html.c_str(), html.size());
	resp->body_length = html.size();
	return resp;
}

Request WebUI::processGetNewTaskPage(Request req) {
	auto user_id = req->cookie["auth"];

	OStrStream vpn_gen;
	const auto & tuns = ShadowSocksController::Get().getConfig().runParams;
	for (auto t : tuns) {
		vpn_gen << findFillText("tasks/tasks_new_runParams.txt", List<String>({t.name, t.name}));
	}

	OStrStream server_gen;
	const auto & servers = ShadowSocksController::Get().getConfig().servers;
	for (const _Server * s : servers)
		server_gen << findFillText("tasks/tasks_new_server.txt", List<String>({s->group, s->name}));

	OStrStream group_gen;
	__DP_LIB_NAMESPACE__::Map<String, bool> added;
	for (_Task * t : ShadowSocksController::Get().getConfig().tasks)
		if (!ConteinsKey(added, t->group) && t->group.size() > 0) {
			group_gen << findFillText("tasks/task_group_list_item.txt", List<String>( {t->group}));
			added[t->group] = true;
		}
	for (_Server * t : ShadowSocksController::Get().getConfig().servers)
		if (!ConteinsKey(added, t->group) && t->group.size() > 0) {
			group_gen << findFillText("tasks/task_group_list_item.txt", List<String>( {t->group}));
			added[t->group] = true;
		}


	String html = makePage("New task", "tasks/tasks_new.txt", List<String>({group_gen.str(), vpn_gen.str(), server_gen.str()}));
	Request resp = makeRequest();
	resp->body = new char[html.size() + 1];
	strncpy(resp->body, html.c_str(), html.size());
	resp->body_length = html.size();
	return resp;
}

Request WebUI::processPostTaskStop(Request req) {
	auto user_id = req->cookie["auth"];

	if (!ConteinsKey(req->post, "stop"))
		return HttpServer::generate404(req->method, req->host, req->path);
	auto & ctrl = ShadowSocksController::Get();
	_Task * t = ctrl.getConfig().findTaskById(parse<int>(req->post["stop"].value));
	if (t == nullptr)
		return HttpServer::generate404(req->method, req->host, req->path);

	DP_LOG_DEBUG << "WebUI (" << user_id << "): Stop task " << t->name;

	try{
		ctrl.StopByName(t->name);
		UserSession & s = getUserSession(user_id);
		s.notify.push_back("Task " + t->name + " stoped");

		return makeRedirect(req, "/tasks.html");
	} catch (__DP_LIB_NAMESPACE__::LineException e) {
		notifyUser(user_id, "Fail to stop task " + t->name + ": " + e.toString());
	} catch (...) {
		notifyUser(user_id, "Fail to stop task " + t->name);
	}
	Request resp = makeRequest();
	resp->status = 500;
	return resp;
}

Request WebUI::processGetTaskStartPage(Request req) {
	if (!ConteinsKey(req->get, "start"))
		return HttpServer::generate404(req->method, req->host, req->path);

	auto & ctrl = ShadowSocksController::Get();
	_Task * _t = ctrl.getConfig().findTaskById(parse<int>(req->get["start"]));
	if (_t == nullptr)
		return HttpServer::generate404(req->method, req->host, req->path);

	OStrStream vpn_gen;
	_RunParams run_params = ShadowSocksController::Get().getConfig().findRunParamsbyName(_t->runParamsName);
	const auto & tuns = ShadowSocksController::Get().getConfig().tun2socksConf;
	for (auto t : tuns) {
		vpn_gen << findFillText("tasks/task_start_vpn.txt", List<String>({
																			 t.name,
																			 run_params.tun2SocksName == t.name ?
																				findText("tasks/tasks_edit_checket_true.txt") :
																				findText("tasks/tasks_edit_checket_false.txt"),
																			 t.name}));
	}

	OStrStream server_gen;
	for (int id : _t->servers_id) {
		_Server * s = ctrl.getConfig().findServerById(id);
		if (s == nullptr)
			continue;

		String status_color = "";
		if (s->check_result.isRun) {
			status_color = findText("servers/server_item_work.txt");
		} else {
			if (s->check_result.msg.size() == 0)
				status_color = findText("servers/server_item_unknown.txt");
			else
				status_color = findText("servers/server_item_fail.txt");
		}
		server_gen << findFillText("tasks/task_start_server.txt", List<String>({
																				  s->name,
																				  status_color,
																				  s->name
																			  }));
	}

	String html = makePage("Start task " + _t->name, "tasks/task_start_params.txt", List<String>({
															toString(_t->id),
															ShadowSocksController::Get().getConfig().checkServerMode != ServerCheckingMode::Off ?
																findText("tasks/tasks_edit_checket_true.txt") :
																findText("tasks/tasks_edit_checket_false.txt"),
															ShadowSocksController::Get().getConfig().checkServerMode == ServerCheckingMode::Off ?
																findText("tasks/tasks_edit_checket_true.txt") :
																findText("tasks/tasks_edit_checket_false.txt"),
															run_params.tun2SocksName.size() == 0?
																findText("tasks/tasks_edit_checket_true.txt") :
																findText("tasks/tasks_edit_checket_false.txt"),
															run_params.tun2SocksName.size() > 0?
																findText("tasks/tasks_edit_checket_true.txt") :
																findText("tasks/tasks_edit_checket_false.txt"),
															run_params.systemProxy ?
																findText("tasks/tasks_edit_checket_true.txt") :
																findText("tasks/tasks_edit_checket_false.txt"),
															!run_params.systemProxy ?
																findText("tasks/tasks_edit_checket_true.txt") :
																findText("tasks/tasks_edit_checket_false.txt"),
															run_params.multimode ?
																findText("tasks/tasks_edit_checket_true.txt") :
																findText("tasks/tasks_edit_checket_false.txt"),
															!run_params.multimode ?
																findText("tasks/tasks_edit_checket_true.txt") :
																findText("tasks/tasks_edit_checket_false.txt"),
															run_params.shadowsocks_type == _RunParams::ShadowSocksType::None ?
																findText("tasks/tasks_edit_checket_true.txt") :
																findText("tasks/tasks_edit_checket_false.txt"),
															 run_params.shadowsocks_type == _RunParams::ShadowSocksType::GO ?
																 findText("tasks/tasks_edit_checket_true.txt") :
																 findText("tasks/tasks_edit_checket_false.txt"),
															 run_params.shadowsocks_type == _RunParams::ShadowSocksType::Rust ?
																 findText("tasks/tasks_edit_checket_true.txt") :
																 findText("tasks/tasks_edit_checket_false.txt"),
															toString(run_params.localPort),
															toString(run_params.httpProxy),
															vpn_gen.str(),
															server_gen.str()
														}));
	Request resp = makeRequest();
	resp->body = new char[html.size() + 1];
	strncpy(resp->body, html.c_str(), html.size());
	resp->body_length = html.size();
	return resp;
}


void calcSizeAndText(double origin, double & res, String & res_s);
void calcToBytes(double origin, String res_s, double & res);

Request WebUI::processGetTasks(Request req) {
	String filter_group = "";
	bool force_cookie = false;
	if (ConteinsKey(req->get, "group"))
		filter_group = req->get["group"];
	String filter_name = "";
	if (ConteinsKey(req->get, "name"))
		filter_name = req->get["name"];
	if (ConteinsKey(req->cookie, "t_group") && filter_group.size() == 0 && !ConteinsKey(req->get, "group"))
        return makeRedirect(req, "/tasks.html?group=" + req->cookie["t_group"] + (filter_name.size() == 0 ? "" : "&name=" + filter_name));
	if (filter_group.size() == 0 && ConteinsKey(req->get, "group"))
		force_cookie = true;
	SmartParser filter_name_parser{"*" + filter_name + "*"};

	OStrStream out;
	const auto & tasks = ShadowSocksController::Get().getConfig().tasks;
	auto running = ShadowSocksController::Get().getRunning();
	auto getShadowSocksClient = [&running] (int id) {
		for (const auto & i : running) {
			if (i.first == id)
				return i.second;
		}
		ShadowSocksClient * t = nullptr;
		return t;
	};
	OStrStream group_gen;
	__DP_LIB_NAMESPACE__::Map<String, bool> added;

	group_gen << findFillText("tasks/tasks_index_group_list_item.txt", List<String>( {
																						 ( filter_group == "" ) ? findText("tasks/tasks_index_group_list_item_selected.txt") : findText("tasks/tasks_index_group_list_item_unselected.txt"),
																						 ""
																					 }));

	for (_Task * t : tasks) {
		if (!ConteinsKey(added, t->group) && t->group.size() > 0) {
			group_gen << findFillText("tasks/tasks_index_group_list_item.txt", List<String>( {
																								 ( t->group.size() > 0 && t->group == filter_group ) ? findText("tasks/tasks_index_group_list_item_selected.txt") : findText("tasks/tasks_index_group_list_item_unselected.txt"),
																								 t->group
																							 }));
			added[t->group] = true;
		}
		if (filter_group.size() > 0 && filter_group != t->group)
			continue;
		if (filter_name.size() > 0 && !filter_name_parser.Check(t->name))
			continue;

		OStrStream sr;
		// 0 - unknown
		// 1 - fail
		// 2 - ok
		unsigned short running = 0;
		String ip = "";
		double speed_sum = 0.0;
		String speed_sum_s = "";
		for (int id : t->servers_id) {
			_Server * srv = ShadowSocksController::Get().getConfig().findServerById(id);
			if (srv->check_result.isRun)
				running = 2;
			else {
				if (srv->check_result.msg.size() == 0) {
					running = running > 0 ? running : 0;
				} else
					running = running > 1 ? running : 1;
			}
			if (srv->check_result.isRun && srv->check_result.ipAddr.size() > 0) {
				ip = srv->check_result.ipAddr;

				if (srv->check_result.speed_s.size() > 0 && srv->check_result.speed > 0) {
					double tmp = 0;
					calcToBytes(srv->check_result.speed, srv->check_result.speed_s, tmp);
					speed_sum += tmp;
				}
				sr << findFillText("tasks/tasks_index_server_work.txt", List<String>{srv->name});
			} else {
				if (ShadowSocksController::Get().getConfig().auto_check_mode == AutoCheckingMode::Off)
					sr << findFillText("tasks/tasks_index_server_work.txt", List<String>{srv->name});
				else
					sr << findFillText("tasks/tasks_index_server_down.txt", List<String>{srv->name});
			}
		}
		if (speed_sum > 1) {
			speed_sum = speed_sum / t->servers_id.size();
			calcSizeAndText(speed_sum,speed_sum, speed_sum_s);
		}

		String status = "";
		ShadowSocksClient * c = getShadowSocksClient(t->id);
		// 0 - stoped
		// 1 - doStop
		// 2 - Started
		unsigned short runningStatus = 0;
		if (c != nullptr) {
			OStrStream out;
			if (c->GetStatus() == ShadowSocksClientStatus::DoStop) {
				runningStatus = 1;
				out << "<p>Do Stop</p>\n";
			}
			if (c->GetStatus() == ShadowSocksClientStatus::Running || c->GetStatus() == ShadowSocksClientStatus::Started) {
				runningStatus = 2;
				out << "<p>Remote " << c->getServer()->host << ":" << c->getServer()->port << " (" << c->getServer()->name << ")</p>\n";
				out << "<p>Socks5 " << c->getRunParams().localHost << ":" << c->getRunParams().localPort << "</p>\n";
				if (c->getRunParams().httpProxy > 0)
					out << "<p>Http " << c->getRunParams().httpProxy << (c->getRunParams().systemProxy ? " (SYS)" : "") << "</p>\n";
				const _Task * t = c->getTask();
				for (const _Tun & tun : t->tuns) {
					out << "<p>" << (tun.type == TunType::TCP ? "tcp://" : "udp://") << tun.localHost << ":" << tun.localPort << " => " << tun.remoteHost << ":" << tun.remotePort << "</p>\n";
				}
			}
			if (c->GetStatus() == ShadowSocksClientStatus::None || c->GetStatus() == ShadowSocksClientStatus::Stoped) {
				runningStatus = 0;
			}

			status = out.str();

		}
		String status_text = "";
		if (running == 0) status_text = findText("tasks/task_item_unknown.txt");
		if (running == 1) status_text = findText("tasks/task_item_fail.txt");
		if (running == 2) status_text = findText("tasks/task_item_work.txt");
		_RunParams p = ShadowSocksController::Get().getConfig().findRunParamsbyName( t->runParamsName);
		String runParamInfo = p.name + "<p>s5://" + p.localHost + ":" + toString(p.localPort) + "</p>";
		if (p.httpProxy > 0) {
			runParamInfo += "<p>";
			runParamInfo += (p.systemProxy ? "sys" : "http");
			runParamInfo += "://" + p.localHost + ":" + toString(p.httpProxy) + "</p>";
		}
		if (p.tun2SocksName.size() > 0)
			runParamInfo += "<p>VPN=" + p.tun2SocksName + "</p>";

		String runControlButton = "";
		if (runningStatus == 0)
			runControlButton = findFillText("tasks/task_start.txt", List<String>({toString(t->id), toString(t->id), toString(t->id), toString(t->id), toString(t->id), toString(t->id)}));
		if (runningStatus == 1)
			runControlButton = "";
		if (runningStatus == 2)
			runControlButton = findFillText("tasks/task_stop.txt", List<String>({toString(t->id), toString(t->id)}));

		String taskName = t->name;
		if (ip.size() > 0) {
			if (speed_sum_s.size() > 0) {
				taskName += " (" + ip + ") [";
				taskName += toString(speed_sum) + " " + speed_sum_s + "/s]";
			} else
				taskName += " (" + ip + ")";

		}

		out << findFillText("tasks/task_item.txt", List<String>({
															  status_text,
															  t->autostart ?  findText("tasks/task_item_enabled.txt") :  findText("tasks/task_item_disabled.txt"),
															  toString(t->id),
															  taskName,
															  runParamInfo,
															  sr.str(),
															  status,
															  runControlButton,
															  toString(t->id),
															  toString(t->id),
															  toString(t->id)
														  }));
	}
	for (_Server * t : ShadowSocksController::Get().getConfig().servers)
		if (!ConteinsKey(added, t->group) && t->group.size() > 0) {
			group_gen << findFillText("tasks/tasks_index_group_list_item.txt", List<String>( {
																								 ( t->group.size() > 0 && t->group == filter_group ) ? findText("tasks/tasks_index_group_list_item_selected.txt") : findText("tasks/tasks_index_group_list_item_unselected.txt"),
																								 t->group
																							 }));
			added[t->group] = true;
		}

	String html = makePage("Tasks", "tasks/tasks_index.txt", List<String>( { filter_name, group_gen.str(), out.str()}));
	Request resp = makeRequest();
	if (filter_group.size() != 0 || force_cookie)
        resp->cookie["t_group"] = filter_group;
	resp->body = new char[html.size() + 1];
	strncpy(resp->body, html.c_str(), html.size());
	resp->body_length = html.size();
	return resp;
}
