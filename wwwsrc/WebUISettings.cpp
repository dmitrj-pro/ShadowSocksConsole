#include "WebUI.h"
#include "../www/textfill.h"
#include <Converter/Converter.h>
#include "../ShadowSocksController.h"
#include <string.h>
#include <_Driver/ServiceMain.h>
#include <_Driver/Files.h>

using __DP_LIB_NAMESPACE__::OStrStream;
using __DP_LIB_NAMESPACE__::toString;
using __DP_LIB_NAMESPACE__::ConteinsKey;
using __DP_LIB_NAMESPACE__::parse;
using __DP_LIB_NAMESPACE__::trim;

Request WebUI::processGetEditSettings(Request req) {
	if (!ConteinsKey(req->get, "edit"))
		return HttpServer::generate404(req->method, req->host, req->path);

	List<String> stringParams = List<String> ({"shadowSocksPath", "shadowSocksPathRust", "v2rayPluginPath", "tun2socksPath", "wgetPath", "tempPath", "bootstrapDNS", "auto_check_ip_url", "auto_check_download_url"});
	List<String> uintParams = List<String>({"udpTimeout", "auto_check_interval_s", "web_session_timeout_m", "countRestartAutostarted"});

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
		if (params == "wgetPath") { val = set.wgetPath; var = "WGet path"; }
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
		if (params == "web_session_timeout_m") { val = toString(set.web_session_timeout_m); var = "Web session timeout (m)"; min = 0; }
		if (params == "auto_check_interval_s") { val = toString(set.auto_check_interval_s); var = "Auto check server interval (s)"; min = 0; }
		if (params == "countRestartAutostarted") { val = toString(set.countRestartAutostarted); var = "Count of restart task on start on boot"; min = 0; }

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
																				"off",
																				(set.autostart == AutoStartMode::Off) ? findText("settings/edit_enum_value_checked.txt") : findText("settings/edit_enum_value_unchecked.txt"),
																				"Disable"
																		  }));
			gen <<  findFillText("settings/edit_enum_value.txt", List<String>({
																				"on",
																				set.autostart == AutoStartMode::On ? findText("settings/edit_enum_value_checked.txt") : findText("settings/edit_enum_value_unchecked.txt"),
																				"Enable"
																		  }));
			gen <<  findFillText("settings/edit_enum_value.txt", List<String>({
																				"core",
																				set.autostart == AutoStartMode::OnCoreStart ? findText("settings/edit_enum_value_checked.txt") : findText("settings/edit_enum_value_unchecked.txt"),
																				"Start on run core (low encryption mode for running parametr)"
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
		if (params == "enable_log_page") {
			var = "Web enable log page";
			gen <<  findFillText("settings/edit_enum_value.txt", List<String>({
																				"true",
																				set.enable_log_page ? findText("settings/edit_enum_value_checked.txt") : findText("settings/edit_enum_value_unchecked.txt"),
																				"Enable"
																		  }));
			gen <<  findFillText("settings/edit_enum_value.txt", List<String>({
																				"false",
																				(!set.enable_log_page) ? findText("settings/edit_enum_value_checked.txt") : findText("settings/edit_enum_value_unchecked.txt"),
																				"Disable"
																		  }));
		}
		if (params == "enable_import_page") {
			var = "Web enable import page";
			gen <<  findFillText("settings/edit_enum_value.txt", List<String>({
																				"true",
																				set.enable_import_page ? findText("settings/edit_enum_value_checked.txt") : findText("settings/edit_enum_value_unchecked.txt"),
																				"Enable"
																		  }));
			gen <<  findFillText("settings/edit_enum_value.txt", List<String>({
																				"false",
																				(!set.enable_import_page) ? findText("settings/edit_enum_value_checked.txt") : findText("settings/edit_enum_value_unchecked.txt"),
																				"Disable"
																		  }));
		}
		if (params == "enable_export_page") {
			var = "Web enable export page";
			gen <<  findFillText("settings/edit_enum_value.txt", List<String>({
																				"true",
																				set.enable_export_page ? findText("settings/edit_enum_value_checked.txt") : findText("settings/edit_enum_value_unchecked.txt"),
																				"Enable"
																		  }));
			gen <<  findFillText("settings/edit_enum_value.txt", List<String>({
																				"false",
																				(!set.enable_export_page) ? findText("settings/edit_enum_value_checked.txt") : findText("settings/edit_enum_value_unchecked.txt"),
																				"Disable"
																		  }));
		}
		if (params == "enable_utils_page") {
			var = "Web enable utils page";
			gen <<  findFillText("settings/edit_enum_value.txt", List<String>({
																				"true",
																				set.enable_utils_page ? findText("settings/edit_enum_value_checked.txt") : findText("settings/edit_enum_value_unchecked.txt"),
																				"Enable"
																		  }));
			gen <<  findFillText("settings/edit_enum_value.txt", List<String>({
																				"false",
																				(!set.enable_utils_page) ? findText("settings/edit_enum_value_checked.txt") : findText("settings/edit_enum_value_unchecked.txt"),
																				"Disable"
																		  }));
		}
		if (params == "enable_exit_page") {
			var = "Web enable exit page";
			gen <<  findFillText("settings/edit_enum_value.txt", List<String>({
																				"true",
																				set.enable_exit_page ? findText("settings/edit_enum_value_checked.txt") : findText("settings/edit_enum_value_unchecked.txt"),
																				"Enable"
																		  }));
			gen <<  findFillText("settings/edit_enum_value.txt", List<String>({
																				"false",
																				(!set.enable_exit_page) ? findText("settings/edit_enum_value_checked.txt") : findText("settings/edit_enum_value_unchecked.txt"),
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


		if (params == "checkServerMode") {
			var = "Servers checking mode";
			gen <<  findFillText("settings/edit_enum_value.txt", List<String>({
																				"off",
																				set.checkServerMode == ServerCheckingMode::Off ? findText("settings/edit_enum_value_checked.txt") : findText("settings/edit_enum_value_unchecked.txt"),
																				"Ignore checking"
																		  }));
			gen <<  findFillText("settings/edit_enum_value.txt", List<String>({
																				"tcp",
																				set.checkServerMode == ServerCheckingMode::TCPCheck ? findText("settings/edit_enum_value_checked.txt") : findText("settings/edit_enum_value_unchecked.txt"),
																				"Checking by tcp"
																		  }));
			gen <<  findFillText("settings/edit_enum_value.txt", List<String>({
																				"deepfast",
																				set.checkServerMode == ServerCheckingMode::DeepFast ? findText("settings/edit_enum_value_checked.txt") : findText("settings/edit_enum_value_unchecked.txt"),
																				"Deep fast (No IP)"
																		  }));
			gen <<  findFillText("settings/edit_enum_value.txt", List<String>({
																				"deep",
																				set.checkServerMode == ServerCheckingMode::DeepCheck ? findText("settings/edit_enum_value_checked.txt") : findText("settings/edit_enum_value_unchecked.txt"),
																				"Checking by get IP (Deep checking)"
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

		if (params == "auto_check_mode") {
			var = "Auto check servers";
			gen <<  findFillText("settings/edit_enum_value.txt", List<String>({
																				"Off",
																				set.auto_check_mode == AutoCheckingMode::Off ? findText("settings/edit_enum_value_checked.txt") : findText("settings/edit_enum_value_unchecked.txt"),
																				"Off"
																		  }));
			gen <<  findFillText("settings/edit_enum_value.txt", List<String>({
																				"Passiv",
																				set.auto_check_mode == AutoCheckingMode::Passiv ? findText("settings/edit_enum_value_checked.txt") : findText("settings/edit_enum_value_unchecked.txt"),
																				"Passiv Mode"
																		  }));
			gen <<  findFillText("settings/edit_enum_value.txt", List<String>({
																				"Work",
																				set.auto_check_mode == AutoCheckingMode::Work ? findText("settings/edit_enum_value_checked.txt") : findText("settings/edit_enum_value_unchecked.txt"),
																				"Check is Work"
																		  }));
			gen <<  findFillText("settings/edit_enum_value.txt", List<String>({
																				"Ip",
																				set.auto_check_mode == AutoCheckingMode::Ip ? findText("settings/edit_enum_value_checked.txt") : findText("settings/edit_enum_value_unchecked.txt"),
																				"Ip"
																		  }));
			gen <<  findFillText("settings/edit_enum_value.txt", List<String>({
																				"Speed",
																				set.auto_check_mode == AutoCheckingMode::Speed ? findText("settings/edit_enum_value_checked.txt") : findText("settings/edit_enum_value_unchecked.txt"),
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

	List<String> stringParams = List<String> ({"shadowSocksPath","shadowSocksPathRust" "v2rayPluginPath", "tun2socksPath", "wgetPath", "bootstrapDNS", "auto_check_ip_url", "auto_check_download_url"});
	List<String> uintParams = List<String>({"udpTimeout", "auto_check_interval_s", "web_session_timeout_m", "countRestartAutostarted"});

	String params = req->get["edit"];
	String value = req->post["value"].value;

	if (__DP_LIB_NAMESPACE__::ConteinsElement(stringParams, params)) {
		String * val = nullptr;
		auto & set = ShadowSocksController::Get().getConfig();

		if (params == "shadowSocksPath") { val = &set.shadowSocksPath;}
		if (params == "shadowSocksPathRust") { val = &set.shadowSocksPathRust;}
		if (params == "v2rayPluginPath") { val = &set.v2rayPluginPath; }
		if (params == "tun2socksPath") { val = &set.tun2socksPath; }
		if (params == "wgetPath") { val = &set.wgetPath; }
		if (params == "bootstrapDNS") { val = &set.bootstrapDNS; }
		if (params == "auto_check_ip_url") { val = &set.auto_check_ip_url; }
		if (params == "auto_check_download_url") { val = &set.auto_check_download_url; }

		if (val == nullptr) {
			DP_LOG_FATAL << "Unknow parametr " << params;
			return HttpServer::generate404(req->method, req->host, req->path);
		}
		(*val) = req->post["value"].value;
		if (params == "bootstrapDNS") {
			if (set.bootstrapDNS.size() > 0)
				__DP_LIB_NAMESPACE__::global_config.setDNS(set.bootstrapDNS);
			else
				__DP_LIB_NAMESPACE__::global_config.resetDNS();
		}
	}
	if (__DP_LIB_NAMESPACE__::ConteinsElement(uintParams, params)) {
		__DP_LIB_NAMESPACE__::UInt * val = nullptr;
		auto & set = ShadowSocksController::Get().getConfig();

		if (params == "udpTimeout") { val = &set.udpTimeout; }
		if (params == "web_session_timeout_m") { val = &set.web_session_timeout_m; }
		if (params == "auto_check_interval_s") { val = &set.auto_check_interval_s;}
		if (params == "countRestartAutostarted") { val = &set.countRestartAutostarted;}

		if (val == nullptr) {
			DP_LOG_FATAL << "Unknow parametr " << params;
			return HttpServer::generate404(req->method, req->host, req->path);
		}
		(*val) = parse<__DP_LIB_NAMESPACE__::UInt>(req->post["value"].value);
	}

	if (params == "autostart") {
		auto & set = ShadowSocksController::Get().getConfig();
		set.autostart = str_to_AutoStartMode(value);
		ShadowSocksController::Get().SaveBootConfig();
	}
	if (params == "defaultShadowSocks") {
		auto & set = ShadowSocksController::Get().getConfig();
		set.shadowSocksType = parseSSType(value);
	}
	if (params == "enableLogging") {
		auto & set = ShadowSocksController::Get().getConfig();
		set.enableLogging = value == "true" ? true : false;
		__DP_LIB_NAMESPACE__::Path logF{getCacheDirectory()};
		logF.Append("LOGGING.txt");
		if (set.enableLogging) {
			ShadowSocksController::Get().OpenLogFile();
		} else {
			ShadowSocksController::Get().CloseLogFile();
		}
	}
	if (params == "autoDetectTunInterface") {
		auto & set = ShadowSocksController::Get().getConfig();
		set.autoDetectTunInterface = value == "true" ? true : false;
	}
	if (params == "enable_utils_page") {
		auto & set = ShadowSocksController::Get().getConfig();
		set.enable_utils_page = value == "true" ? true : false;
	}
	if (params == "enable_export_page") {
		auto & set = ShadowSocksController::Get().getConfig();
		set.enable_export_page = value == "true" ? true : false;
	}
	if (params == "enable_log_page") {
		auto & set = ShadowSocksController::Get().getConfig();
		set.enable_log_page = value == "true" ? true : false;
	}
	if (params == "enable_import_page") {
		auto & set = ShadowSocksController::Get().getConfig();
		set.enable_import_page = value == "true" ? true : false;
	}
	if (params == "enable_exit_page") {
		auto & set = ShadowSocksController::Get().getConfig();
		set.enable_exit_page = value == "true" ? true : false;
	}
	if (params == "fixLinuxWgetPath") {
		auto & set = ShadowSocksController::Get().getConfig();
		set.fixLinuxWgetPath = value == "true" ? true : false;
	}
	if (params == "checkServerMode") {
		auto & set = ShadowSocksController::Get().getConfig();
		set.checkServerMode = str_to_ServerCheckingMode(value);
	}

	if (params == "auto_check_mode") {
		auto & set = ShadowSocksController::Get().getConfig();
		set.auto_check_mode = str_to_AutoCheckingMode(value);
	}

	ShadowSocksController::Get().SaveConfig();

	return makeRedirect(req, "/settings.html");
}

Request WebUI::processGetSettings(Request) {
	OStrStream out;

	const auto & c = ShadowSocksController::Get().getConfig();
	out << findFillText("settings/settings_item.txt", List<String>({
															"Enable autostart",
															AutoStartMode_to_str(c.autostart),
															"autostart"
													  }));
	out << findFillText("settings/settings_item.txt", List<String>({
															"Count of try restart task on autostart",
															toString(c.countRestartAutostarted),
															"countRestartAutostarted"
													  }));
	out << findFillText("settings/settings_item.txt", List<String>({
															"Web session timeout (m)",
															toString(c.web_session_timeout_m),
															"web_session_timeout_m"
													  }));
	out << findFillText("settings/settings_item.txt", List<String>({
															"Web enable log page",
															toString(c.enable_log_page),
															"enable_log_page"
													  }));
	out << findFillText("settings/settings_item.txt", List<String>({
															"Web enable import page",
															toString(c.enable_import_page),
															"enable_import_page"
													  }));
	out << findFillText("settings/settings_item.txt", List<String>({
															"Web enable export page",
															toString(c.enable_export_page),
															"enable_export_page"
													  }));
	out << findFillText("settings/settings_item.txt", List<String>({
															"Web enable utils page",
															toString(c.enable_utils_page),
															"enable_utils_page"
													  }));
	out << findFillText("settings/settings_item.txt", List<String>({
															"Web enable exit page",
															toString(c.enable_exit_page),
															"enable_exit_page"
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
															"WGet path",
															toString(c.wgetPath),
															"wgetPath"
													  }));
	out << findFillText("settings/settings_item.txt", List<String>({
															"Use system wget (Linux)",
															toString(c.fixLinuxWgetPath),
															"fixLinuxWgetPath"
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
															"Auto detect Tap interface",
															toString(c.autoDetectTunInterface),
															"autoDetectTunInterface"
													  }));
	out << findFillText("settings/settings_item.txt", List<String>({
															"Enable Logging",
															toString(c.enableLogging),
															"enableLogging"
													  }));
	out << findFillText("settings/settings_item.txt", List<String>({
															"UDP Timeout",
															toString(c.udpTimeout),
															"udpTimeout"
													  }));
	out << findFillText("settings/settings_item.txt", List<String>({
															"Servers checking mode",
															ServerCheckingMode_to_str(c.checkServerMode),
															"checkServerMode"
													  }));
	out << findFillText("settings/settings_item.txt", List<String>({
															"Bootstrap DNS",
															toString(c.bootstrapDNS),
															"bootstrapDNS"
													  }));

	out << findFillText("settings/settings_item.txt", List<String>({
															"Auto check servers",
															AutoCheckingMode_to_str(c.auto_check_mode),
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

	auto & ctrl = ShadowSocksController::Get();
	String html = makePage("Settings", "settings/settings_index.txt", List<String>( {
																						out.str(),
																						ctrl.RecordStarted() ? findText("settings/settings_patch_stop_record.txt") : "",
																						ctrl.haveRecord() ? findText("settings/settings_patch_apply_record.txt") : ""
																					}));
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

Request WebUI::processPostPatch(Request req) {
	if (ConteinsKey(req->post, "start"))
		ShadowSocksController::Get().startRecord();
	if (ConteinsKey(req->post, "stop"))
		ShadowSocksController::Get().stopRecord();
	if (ConteinsKey(req->post, "apply"))
		ShadowSocksController::Get().applyRecord();
	return makeRedirect(req, "/settings.html");
}
