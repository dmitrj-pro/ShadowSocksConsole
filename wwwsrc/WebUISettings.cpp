#include "WebUI.h"
#include "../www/textfill.h"
#include <Converter/Converter.h>
#include "../ShadowSocksController.h"
#include <string.h>

using __DP_LIB_NAMESPACE__::OStrStream;
using __DP_LIB_NAMESPACE__::toString;
using __DP_LIB_NAMESPACE__::ConteinsKey;
using __DP_LIB_NAMESPACE__::parse;
using __DP_LIB_NAMESPACE__::trim;

Request WebUI::processGetEditSettings(Request req) {
	if (!ConteinsKey(req->get, "edit"))
		return HttpServer::generate404(req->method, req->host, req->path);

	List<String> stringParams = List<String> ({"shadowSocksPath", "shadowSocksPathRust", "v2rayPluginPath", "tun2socksPath", "dns2socksPath", "wgetPath", "tempPath", "bootstrapDNS", "auto_check_ip_url", "auto_check_download_url"});
	List<String> uintParams = List<String>({"udpTimeout", "auto_check_interval_s"});

	String params = req->get["edit"];
	Request resp = makeRequest();
	if (__DP_LIB_NAMESPACE__::ConteinsElement(stringParams, params)) {
		String var;
		String val;

		auto & set = ShadowSocksController::Get().getConfig();

		if (params == "shadowSocksPath") { val = set.shadowSocksPath; var = "ShadowSocks Path"; }
		if (params == "shadowSocksPathRust") { val = set.shadowSocksPathRust; var = "ShadowSocksRust Path"; }
		if (params == "v2rayPluginPath") { val = set.v2rayPluginPath; var = "V2Ray Path"; }
		if (params == "tun2socksPath") { val = set.tun2socksPath; var = "Tun2Socks path"; }
		if (params == "dns2socksPath") { val = set.dns2socksPath; var = "Dns2Socks path"; }
		if (params == "wgetPath") { val = set.wgetPath; var = "WGet path"; }
		if (params == "tempPath") { val = set.tempPath; var = "TempPath"; }
		if (params == "bootstrapDNS") { val = set.bootstrapDNS; var = "Bootstrap DNS"; }
		if (params == "auto_check_ip_url") { val = set.auto_check_ip_url; var = "Auto check server ip url"; }
		if (params == "auto_check_download_url") { val = set.auto_check_download_url; var = "Auto check server download url"; }

		String html = makePage("Edit setting", "settings/edit_string.txt", List<String>({ params, var, val }));
		resp->body = new char[html.size() + 1];
		strncpy(resp->body, html.c_str(), html.size());
		resp->body_length = html.size();
	}

	if (__DP_LIB_NAMESPACE__::ConteinsElement(uintParams, params)) {
		String var;
		String val;
		int max = 2147483647;
		int min = -2147483648;

		auto & set = ShadowSocksController::Get().getConfig();

		if (params == "udpTimeout") { val = toString(set.udpTimeout); var = "Udp Timeout (s)"; min = 0;}
		if (params == "auto_check_interval_s") { val = toString(set.auto_check_interval_s); var = "Auto check server interval (s)"; min = 0; }

		String html = makePage("Edit setting", "settings/edit_uint.txt", List<String>({ params, var, toString(min), toString(max), val }));
		resp->body = new char[html.size() + 1];
		strncpy(resp->body, html.c_str(), html.size());
		resp->body_length = html.size();
	}

	if (resp->body_length == 0) {
		String var;

		OStrStream gen;
		auto & set = ShadowSocksController::Get().getConfig();

		if (params == "autostart") {
			var = "Enable autostart";
			gen <<  findFillText("settings/edit_enum_value.txt", List<String>({
																				"true",
																				set.autostart ? findText("settings/edit_enum_value_checked.txt") : findText("settings/edit_enum_value_unchecked.txt"),
																				"Enable"
																		  }));
			gen <<  findFillText("settings/edit_enum_value.txt", List<String>({
																				"false",
																				(!set.autostart) ? findText("settings/edit_enum_value_checked.txt") : findText("settings/edit_enum_value_unchecked.txt"),
																				"Disable"
																		  }));
		}
		if (params == "enableLogging") {
			var = "Enable logging file";
			gen <<  findFillText("settings/edit_enum_value.txt", List<String>({
																				"true",
																				set.enableLogging ? findText("settings/edit_enum_value_checked.txt") : findText("settings/edit_enum_value_unchecked.txt"),
																				"Enable"
																		  }));
			gen <<  findFillText("settings/edit_enum_value.txt", List<String>({
																				"false",
																				(!set.enableLogging) ? findText("settings/edit_enum_value_checked.txt") : findText("settings/edit_enum_value_unchecked.txt"),
																				"Disable"
																		  }));
		}
		if (params == "defaultShadowSocks") {
			var = "Default ShadowSocks";
			gen <<  findFillText("settings/edit_enum_value.txt", List<String>({
																				SSTtypetoString(_RunParams::ShadowSocksType::GO),
																				set.shadowSocksType == _RunParams::ShadowSocksType::GO ? findText("settings/edit_enum_value_checked.txt") : findText("settings/edit_enum_value_unchecked.txt"),
																				SSTtypetoString(_RunParams::ShadowSocksType::GO)
																		  }));
			gen <<  findFillText("settings/edit_enum_value.txt", List<String>({
																				SSTtypetoString(_RunParams::ShadowSocksType::Rust),
																				set.shadowSocksType == _RunParams::ShadowSocksType::Rust ? findText("settings/edit_enum_value_checked.txt") : findText("settings/edit_enum_value_unchecked.txt"),
																				SSTtypetoString(_RunParams::ShadowSocksType::Rust)
																		  }));
		}

		if (params == "hideDNS2Socks") {
			var = "Hide DNS2Socks (Windows)";
			gen <<  findFillText("settings/edit_enum_value.txt", List<String>({
																				"true",
																				set.hideDNS2Socks ? findText("settings/edit_enum_value_checked.txt") : findText("settings/edit_enum_value_unchecked.txt"),
																				"Hide"
																		  }));
			gen <<  findFillText("settings/edit_enum_value.txt", List<String>({
																				"false",
																				(!set.hideDNS2Socks) ? findText("settings/edit_enum_value_checked.txt") : findText("settings/edit_enum_value_unchecked.txt"),
																				"Show"
																		  }));
		}
		if (params == "fixLinuxWgetPath") {
			var = "Use system wget (Linux)";
			gen <<  findFillText("settings/edit_enum_value.txt", List<String>({
																				"true",
																				set.fixLinuxWgetPath ? findText("settings/edit_enum_value_checked.txt") : findText("settings/edit_enum_value_unchecked.txt"),
																				"Use system wget"
																		  }));
			gen <<  findFillText("settings/edit_enum_value.txt", List<String>({
																				"false",
																				(!set.fixLinuxWgetPath) ? findText("settings/edit_enum_value_checked.txt") : findText("settings/edit_enum_value_unchecked.txt"),
																				"Use custom wget"
																		  }));
		}
		if (params == "enableDeepCheckServer") {
			var = "Deep check servers before start task";
			gen <<  findFillText("settings/edit_enum_value.txt", List<String>({
																				"true",
																				set.enableDeepCheckServer ? findText("settings/edit_enum_value_checked.txt") : findText("settings/edit_enum_value_unchecked.txt"),
																				"Deep check servers"
																		  }));
			gen <<  findFillText("settings/edit_enum_value.txt", List<String>({
																				"false",
																				(!set.enableDeepCheckServer) ? findText("settings/edit_enum_value_checked.txt") : findText("settings/edit_enum_value_unchecked.txt"),
																				"Lite check servers"
																		  }));
		}
		if (params == "autoDetectTunInterface") {
			var = "Auto detect Tap interface";
			gen <<  findFillText("settings/edit_enum_value.txt", List<String>({
																				"true",
																				set.autoDetectTunInterface ? findText("settings/edit_enum_value_checked.txt") : findText("settings/edit_enum_value_unchecked.txt"),
																				"Enable"
																		  }));
			gen <<  findFillText("settings/edit_enum_value.txt", List<String>({
																				"false",
																				(!set.autoDetectTunInterface) ? findText("settings/edit_enum_value_checked.txt") : findText("settings/edit_enum_value_unchecked.txt"),
																				"Disable"
																		  }));
		}
		if (params == "IGNORECHECKSERVER") {
			var = "Ignore check result";
			gen <<  findFillText("settings/edit_enum_value.txt", List<String>({
																				"true",
																				set.IGNORECHECKSERVER ? findText("settings/edit_enum_value_checked.txt") : findText("settings/edit_enum_value_unchecked.txt"),
																				"Ignore"
																		  }));
			gen <<  findFillText("settings/edit_enum_value.txt", List<String>({
																				"false",
																				(!set.IGNORECHECKSERVER) ? findText("settings/edit_enum_value_checked.txt") : findText("settings/edit_enum_value_unchecked.txt"),
																				"Check"
																		  }));
		}

		if (params == "auto_check_mode") {
			var = "Auto check servers";
			gen <<  findFillText("settings/edit_enum_value.txt", List<String>({
																				"Off",
																				set.auto_check_mode == ShadowSocksSettings::AutoCheckingMode::Off ? findText("settings/edit_enum_value_checked.txt") : findText("settings/edit_enum_value_unchecked.txt"),
																				"Off"
																		  }));
			gen <<  findFillText("settings/edit_enum_value.txt", List<String>({
																				"Passiv",
																				set.auto_check_mode == ShadowSocksSettings::AutoCheckingMode::Passiv ? findText("settings/edit_enum_value_checked.txt") : findText("settings/edit_enum_value_unchecked.txt"),
																				"Passiv Mode"
																		  }));
			gen <<  findFillText("settings/edit_enum_value.txt", List<String>({
																				"Ip",
																				set.auto_check_mode == ShadowSocksSettings::AutoCheckingMode::Ip ? findText("settings/edit_enum_value_checked.txt") : findText("settings/edit_enum_value_unchecked.txt"),
																				"Ip"
																		  }));
			gen <<  findFillText("settings/edit_enum_value.txt", List<String>({
																				"Speed",
																				set.auto_check_mode == ShadowSocksSettings::AutoCheckingMode::Speed ? findText("settings/edit_enum_value_checked.txt") : findText("settings/edit_enum_value_unchecked.txt"),
																				"Speed"
																		  }));
		}
		if (params == "enablePreStartStopScripts") {
			var = "Enable pre start/stop tun scripting (WARN: Parametr can't edit by WebUI)";
			gen <<  findFillText("settings/edit_enum_value.txt", List<String>({
																				"true",
																				ShadowSocksSettings::enablePreStartStopScripts ? findText("settings/edit_enum_value_checked.txt") : findText("settings/edit_enum_value_unchecked.txt"),
																				"Enable"
																		  }));
			gen <<  findFillText("settings/edit_enum_value.txt", List<String>({
																				"false",
																				(!ShadowSocksSettings::enablePreStartStopScripts) ? findText("settings/edit_enum_value_checked.txt") : findText("settings/edit_enum_value_unchecked.txt"),
																				"Disable"
																		  }));
		}

		if (gen.str().size() == 0)
			return HttpServer::generate404(req->method, req->host, req->path);

		String html = makePage("Edit setting", "settings/edit_enum.txt", List<String>({ params, var, gen.str() }));
		resp->body = new char[html.size() + 1];
		strncpy(resp->body, html.c_str(), html.size());
		resp->body_length = html.size();
	}
	if (resp->body_length == 0)
		return HttpServer::generate404(req->method, req->host, req->path);



	return resp;
}

Request WebUI::processPostEditSettings(Request req) {
	if (!ConteinsKey(req->get, "edit"))
		return HttpServer::generate404(req->method, req->host, req->path);

	if (!ConteinsKey(req->post, "value"))
		return HttpServer::generate404(req->method, req->host, req->path);

	List<String> stringParams = List<String> ({"shadowSocksPath","shadowSocksPathRust" "v2rayPluginPath", "tun2socksPath", "dns2socksPath", "wgetPath", "tempPath", "bootstrapDNS", "auto_check_ip_url", "auto_check_download_url"});
	List<String> uintParams = List<String>({"udpTimeout", "auto_check_interval_s"});

	String params = req->get["edit"];
	String value = req->post["value"].value;

	if (__DP_LIB_NAMESPACE__::ConteinsElement(stringParams, params)) {
		String * val = nullptr;
		auto & set = ShadowSocksController::Get().getConfig();

		if (params == "shadowSocksPath") { val = &set.shadowSocksPath;}
		if (params == "shadowSocksPathRust") { val = &set.shadowSocksPathRust;}
		if (params == "v2rayPluginPath") { val = &set.v2rayPluginPath; }
		if (params == "tun2socksPath") { val = &set.tun2socksPath; }
		if (params == "dns2socksPath") { val = &set.dns2socksPath; }
		if (params == "wgetPath") { val = &set.wgetPath; }
		if (params == "tempPath") { val = &set.tempPath; }
		if (params == "bootstrapDNS") { val = &set.bootstrapDNS; }
		if (params == "auto_check_ip_url") { val = &set.auto_check_ip_url; }
		if (params == "auto_check_download_url") { val = &set.auto_check_download_url; }

		if (val == nullptr) {
			DP_LOG_FATAL << "Unknow parametr " << params;
			return HttpServer::generate404(req->method, req->host, req->path);
		}
		(*val) = req->post["value"].value;
	}
	if (__DP_LIB_NAMESPACE__::ConteinsElement(uintParams, params)) {
		__DP_LIB_NAMESPACE__::UInt * val = nullptr;
		auto & set = ShadowSocksController::Get().getConfig();

		if (params == "udpTimeout") { val = &set.udpTimeout; }
		if (params == "auto_check_interval_s") { val = &set.auto_check_interval_s;}

		if (val == nullptr) {
			DP_LOG_FATAL << "Unknow parametr " << params;
			return HttpServer::generate404(req->method, req->host, req->path);
		}
		(*val) = parse<__DP_LIB_NAMESPACE__::UInt>(req->post["value"].value);
	}

	if (params == "autostart") {
		auto & set = ShadowSocksController::Get().getConfig();
		set.autostart = value == "true" ? true : false;
	}
	if (params == "defaultShadowSocks") {
		auto & set = ShadowSocksController::Get().getConfig();
		set.shadowSocksType = parseSSType(value);
	}
	if (params == "enableLogging") {
		auto & set = ShadowSocksController::Get().getConfig();
		set.enableLogging = value == "true" ? true : false;
	}
	if (params == "autoDetectTunInterface") {
		auto & set = ShadowSocksController::Get().getConfig();
		set.autoDetectTunInterface = value == "true" ? true : false;
	}
	if (params == "hideDNS2Socks") {
		auto & set = ShadowSocksController::Get().getConfig();
		set.hideDNS2Socks = value == "true" ? true : false;
	}
	if (params == "fixLinuxWgetPath") {
		auto & set = ShadowSocksController::Get().getConfig();
		set.fixLinuxWgetPath = value == "true" ? true : false;
	}
	if (params == "enableDeepCheckServer") {
		auto & set = ShadowSocksController::Get().getConfig();
		set.enableDeepCheckServer = value == "true" ? true : false;
	}
	if (params == "IGNORECHECKSERVER") {
		auto & set = ShadowSocksController::Get().getConfig();
		set.IGNORECHECKSERVER = value == "true" ? true : false;
	}

	if (params == "auto_check_mode") {
		auto & set = ShadowSocksController::Get().getConfig();
		set.auto_check_mode = ShadowSocksSettings::str_to_auto(value);
	}

	ShadowSocksController::Get().SaveConfig();

	return makeRedirect(req, "/settings.html");
}

Request WebUI::processGetSettings(Request) {
	OStrStream out;

	const auto & c = ShadowSocksController::Get().getConfig();
	out << findFillText("settings/settings_item.txt", List<String>({
															"Enable autostart",
															toString(c.autostart),
															"autostart"
													  }));
	out << findFillText("settings/settings_item.txt", List<String>({
															"ShadowSocks path",
															toString(c.shadowSocksPath),
															"shadowSocksPath"
													  }));
	out << findFillText("settings/settings_item.txt", List<String>({
															"ShadowSocksRust path",
															toString(c.shadowSocksPathRust),
															"shadowSocksPathRust"
													  }));
	out << findFillText("settings/settings_item.txt", List<String>({
															"Default ShadowSocks",
															SSTtypetoString(c.shadowSocksType),
															"defaultShadowSocks"
													  }));
	out << findFillText("settings/settings_item.txt", List<String>({
															"Use system wget (Linux)",
															toString(c.fixLinuxWgetPath),
															"fixLinuxWgetPath"
													  }));
	out << findFillText("settings/settings_item.txt", List<String>({
															"Deep check servers before start task",
															toString(c.enableDeepCheckServer),
															"enableDeepCheckServer"
													  }));
	out << findFillText("settings/settings_item.txt", List<String>({
															"Auto detect Tap interface",
															toString(c.autoDetectTunInterface),
															"autoDetectTunInterface"
													  }));
	out << findFillText("settings/settings_item.txt", List<String>({
															"V2Ray path",
															c.v2rayPluginPath,
															"v2rayPluginPath"
													  }));
	out << findFillText("settings/settings_item.txt", List<String>({
															"T2Socks path",
															toString(c.tun2socksPath),
															"tun2socksPath"
													  }));
	out << findFillText("settings/settings_item.txt", List<String>({
															"Dns2Socks path",
															toString(c.dns2socksPath),
															"dns2socksPath"
													  }));
	out << findFillText("settings/settings_item.txt", List<String>({
															"WGet path",
															toString(c.wgetPath),
															"wgetPath"
													  }));
	out << findFillText("settings/settings_item.txt", List<String>({
															"Temp path",
															toString(c.tempPath),
															"tempPath"
													  }));
	out << findFillText("settings/settings_item.txt", List<String>({
															"Enable Logging",
															toString(c.enableLogging),
															"enableLogging"
													  }));
	out << findFillText("settings/settings_item.txt", List<String>({
															"Hide Dns2Socks",
															toString(c.hideDNS2Socks),
															"hideDNS2Socks"
													  }));
	out << findFillText("settings/settings_item.txt", List<String>({
															"UDP Timeout",
															toString(c.udpTimeout),
															"udpTimeout"
													  }));
	out << findFillText("settings/settings_item.txt", List<String>({
															"Ignore result check server",
															toString(c.IGNORECHECKSERVER),
															"IGNORECHECKSERVER"
													  }));
	out << findFillText("settings/settings_item.txt", List<String>({
															"Bootstrap DNS",
															toString(c.bootstrapDNS),
															"bootstrapDNS"
													  }));

	out << findFillText("settings/settings_item.txt", List<String>({
															"Auto check servers",
															ShadowSocksSettings::auto_to_str(c.auto_check_mode),
															"auto_check_mode"
													  }));

	out << findFillText("settings/settings_item.txt", List<String>({
															"Auto check server interval (s)",
															toString(c.auto_check_interval_s),
															"auto_check_interval_s"
													  }));

	out << findFillText("settings/settings_item.txt", List<String>({
															"Auto check server ip url",
															c.auto_check_ip_url,
															"auto_check_ip_url"
													  }));

	out << findFillText("settings/settings_item.txt", List<String>({
															"Auto check server download url",
															c.auto_check_download_url,
															"auto_check_download_url"
													  }));
	out << findFillText("settings/settings_item.txt", List<String>({
															"Enable pre start/stop tun scripting",
															toString(ShadowSocksSettings::enablePreStartStopScripts),
															"enablePreStartStopScripts"
													  }));

	String html = makePage("Settings", "settings/settings_index.txt", List<String>( { out.str()}));
	Request resp = makeRequest();
	resp->body = new char[html.size() + 1];
	strncpy(resp->body, html.c_str(), html.size());
	resp->body_length = html.size();
	return resp;
}

Request WebUI::processGetVariables(Request) {
	OStrStream out;
	const auto & vars = ShadowSocksController::Get().getConfig().variables;
	for (const auto & pair : vars) {
		out << findFillText("variables/variable_item.txt", List<String>({
																pair.first,
																pair.second,
																pair.first,
																pair.first,
														  }));
	}

	String html = makePage("Variables", "variables/variable_index.txt", List<String>( { out.str()}));
	Request resp = makeRequest();
	resp->body = new char[html.size() + 1];
	strncpy(resp->body, html.c_str(), html.size());
	resp->body_length = html.size();
	return resp;
}

Request WebUI::processGetAddVariables(Request) {
	String html = makePage("Add variable", "variables/add.txt", List<String>());
	Request resp = makeRequest();
	resp->body = new char[html.size() + 1];
	strncpy(resp->body, html.c_str(), html.size());
	resp->body_length = html.size();
	return resp;
}

Request WebUI::processPostAddVariables(Request req) {
	if (!ConteinsKey(req->post, "name") || !ConteinsKey(req->post, "value"))
		return HttpServer::generate404(req->method, req->host, req->path);

	String var = req->post["name"].value;
	String val = req->post["value"].value;
	ShadowSocksController::Get().getConfig().variables[var] = val;
	ShadowSocksController::Get().SaveConfig();

	return makeRedirect(req, "/variables.html");
}

Request WebUI::processGetEditVariables(Request req) {
	if (!ConteinsKey(req->get, "edit"))
		return HttpServer::generate404(req->method, req->host, req->path);
	String var = req->get["edit"];
	if (!ConteinsKey(ShadowSocksController::Get().getConfig().variables, var))
		return HttpServer::generate404(req->method, req->host, req->path);
	String val = ShadowSocksController::Get().getConfig().variables[var];

	String html = makePage("Edit variable", "variables/edit.txt", List<String>({ var, val }));
	Request resp = makeRequest();
	resp->body = new char[html.size() + 1];
	strncpy(resp->body, html.c_str(), html.size());
	resp->body_length = html.size();
	return resp;
}

Request WebUI::processPostEditVariables(Request req) {
	if (!ConteinsKey(req->post, "name") || !ConteinsKey(req->post, "value"))
		return HttpServer::generate404(req->method, req->host, req->path);

	String var = req->post["name"].value;
	if (!ConteinsKey(ShadowSocksController::Get().getConfig().variables, var))
		return HttpServer::generate404(req->method, req->host, req->path);
	String val = req->post["value"].value;
	ShadowSocksController::Get().getConfig().variables[var] = val;
	ShadowSocksController::Get().SaveConfig();

	return makeRedirect(req, "/variables.html");
}

Request WebUI::processGetDeleteVariables(Request req) {
	if (!ConteinsKey(req->get, "delete"))
		return HttpServer::generate404(req->method, req->host, req->path);
	String var = req->get["delete"];
	if (!ConteinsKey(ShadowSocksController::Get().getConfig().variables, var))
		return HttpServer::generate404(req->method, req->host, req->path);

	String html = makePage("Delete variable " + var, "variables/delete.txt", List<String>({
																	var
																}));

	Request resp = makeRequest();
	resp->body = new char[html.size() + 1];
	strncpy(resp->body, html.c_str(), html.size());
	resp->body_length = html.size();
	return resp;
}

Request WebUI::processPostDeleteVariables(Request req) {
	if (!ConteinsKey(req->get, "delete"))
		return HttpServer::generate404(req->method, req->host, req->path);
	String var = req->get["delete"];
	if (!ConteinsKey(ShadowSocksController::Get().getConfig().variables, var))
		return HttpServer::generate404(req->method, req->host, req->path);

	ShadowSocksController::Get().getConfig().variables.erase(ShadowSocksController::Get().getConfig().variables.find(var));
	ShadowSocksController::Get().SaveConfig();

	return makeRedirect(req, "/variables.html");
}
