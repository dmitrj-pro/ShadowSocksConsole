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

Request WebUI::processGetVPN(Request req) {
	OStrStream out;
	const auto & tuns = ShadowSocksController::Get().getConfig().tun2socksConf;
	for (auto t : tuns) {
		OStrStream ignoreIp;
		OStrStream dns;

		for (String d : t.dns)
			dns << d << ", ";
		for (String i : t.ignoreIP)
			ignoreIp << i << ", ";

		out << findFillText("vpn/vpn_item.txt", List<String>({
																t.name,
																t.tunName,
																t.defaultRoute,
																toString(t.removeDefaultRoute),
																dns.str(),
																ignoreIp.str(),
																t.name,
																t.name,
																t.name
														  }));
	}

	String html = makePage("VPN", "vpn/vpn_index.txt", List<String>( { out.str()}));
	Request resp = makeRequest();
	resp->body = new char[html.size() + 1];
	strncpy(resp->body, html.c_str(), html.size());
	resp->body_length = html.size();
	return resp;
}

Request WebUI::processGetVPNDelete(Request req) {
	if (!ConteinsKey(req->get, "delete"))
		return HttpServer::generate404(req->method, req->host, req->path);
	auto & ctrl = ShadowSocksController::Get();

	Tun2SocksConfig conf = ctrl.getConfig().findVPNbyName(req->get["delete"]);
	if (conf.isNull)
		return HttpServer::generate404(req->method, req->host, req->path);

	String html = makePage("Delete vpn " + conf.name, "vpn/delete.txt", List<String>({
															toString(conf.name)
														}));

	Request resp = makeRequest();
	resp->body = new char[html.size() + 1];
	strncpy(resp->body, html.c_str(), html.size());
	resp->body_length = html.size();
	return resp;
}


Request WebUI::processPostVPNDelete(Request req) {
	auto user_id = req->cookie["auth"];

	if (!ConteinsKey(req->get, "delete"))
		return HttpServer::generate404(req->method, req->host, req->path);
	auto & ctrl = ShadowSocksController::Get();

	Tun2SocksConfig conf = ctrl.getConfig().findVPNbyName(req->get["delete"]);

	if (conf.isNull)
		return HttpServer::generate404(req->method, req->host, req->path);

	DP_LOG_DEBUG << "WebUI (" << user_id << "): Delete vpn " << conf.name;

	for (const _RunParams & t : ctrl.getConfig().runParams)
		if (t.tun2SocksName == conf.name) {
			notifyUser(user_id, "Can't remove this vpn. It use in run parametr " + t.name);
			return makeRedirect(req, "/vpn.html");
		}

	ctrl.getConfig().deleteVPNByName(conf.name);
	ctrl.SaveConfig();

	return makeRedirect(req, "/vpn.html");
}

Request WebUI::processGetVPNEditPage(Request req){
	if (!ConteinsKey(req->get, "edit"))
		return HttpServer::generate404(req->method, req->host, req->path);

	auto & ctrl = ShadowSocksController::Get();
	Tun2SocksConfig cnf = ctrl.getConfig().findVPNbyName(req->get["edit"]);
	if (cnf.isNull)
		return HttpServer::generate404(req->method, req->host, req->path);

	OStrStream dns;
	OStrStream ignore;
	for (const String & d : cnf.dns)
		dns << d << "\n";
	for (const String & i : cnf.ignoreIP)
		ignore << i << "\n";

	String html = makePage("Edit VPN " + cnf.name, "vpn/edit.txt", List<String>({
															cnf.tunName,
															cnf.defaultRoute,
															cnf.removeDefaultRoute ? findText("vpn/checket_true.txt") : findText("vpn/checket_false.txt"),
															!cnf.removeDefaultRoute ? findText("vpn/checket_true.txt") : findText("vpn/checket_false.txt"),
															dns.str(),
															ignore.str()
														}));
	Request resp = makeRequest();
	resp->body = new char[html.size() + 1];
	strncpy(resp->body, html.c_str(), html.size());
	resp->body_length = html.size();
	return resp;
}

Request WebUI::processPostVPNEditPage(Request req){
	auto user_id = req->cookie["auth"];

	if (!ConteinsKey(req->get, "edit"))
		return HttpServer::generate404(req->method, req->host, req->path);

	auto & ctrl = ShadowSocksController::Get();
	Tun2SocksConfig cnf = ctrl.getConfig().findVPNbyName(req->get["edit"]);
	if (cnf.isNull)
		return HttpServer::generate404(req->method, req->host, req->path);

	DP_LOG_DEBUG << "WebUI (" << user_id << "): Edit vpn " << cnf.name;

	readParametr_nd(cnf.tunName, "tunname");
	readParametr_nd(cnf.defaultRoute, "ethernet");
	String rem = "";
	readParametr_nd(rem, "removeroute");
	cnf.removeDefaultRoute = rem == "yes";
	readParametr_nd(rem, "dns");
	{
		IStrStream in;
		in.str(rem);
		cnf.dns.clear();
		while (!in.eof()) {
			String l;
			getline(in, l);
			l = trim(l);
			if (l.size() > 1)
				cnf.dns.push_back(l);
		}
	}
	rem = "";
	{
		readParametr_n(rem, "ignore");
		IStrStream in;
		in.str(rem);
		cnf.ignoreIP.clear();
		while (!in.eof()) {
			String l;
			getline(in, l);
			l = trim(l);
			if (l.size() > 1)
				cnf.ignoreIP.push_back(l);
		}
	}

	auto res = ctrl.getConfig().findVPNbyName(cnf.name);
	ctrl.getConfig().deleteVPNByName(cnf.name);
	if (!ctrl.getConfig().CheckT2S(cnf)) {
		ctrl.getConfig().tun2socksConf.push_back(res);
		notifyUser(user_id, "Fail to add VPN mode\n");
		return makeRedirect(req, "/vpn.html");
	}
	ctrl.getConfig().tun2socksConf.push_back(cnf);
	ctrl.SaveConfig();

	return makeRedirect(req, "/vpn.html");
}

Request WebUI::processGetNewVPNPage(Request req){
	String html = makePage("New VPN", "vpn/new.txt", List<String>({
															Tun2Socks::DetectInterfaceName(),
															Tun2Socks::DetectDefaultRoute()
														}));
	Request resp = makeRequest();
	resp->body = new char[html.size() + 1];
	strncpy(resp->body, html.c_str(), html.size());
	resp->body_length = html.size();
	return resp;
}

Request WebUI::processPostNewVPNPage(Request req){
	auto user_id = req->cookie["auth"];

	auto & ctrl = ShadowSocksController::Get();
	Tun2SocksConfig cnf;

	readParametr_nd(cnf.name, "name");
	DP_LOG_DEBUG << "WebUI (" << user_id << "): Add vpn " << cnf.name;
	readParametr_nd(cnf.tunName, "tunname");
	readParametr_nd(cnf.defaultRoute, "ethernet");
	String rem = "";
	readParametr_nd(rem, "removeroute");
	cnf.removeDefaultRoute = rem == "yes";
	readParametr_nd(rem, "dns");
	{
		IStrStream in;
		in.str(rem);
		while (!in.eof()) {
			String l;
			getline(in, l);
			l = trim(l);
			if (l.size() > 1)
				cnf.dns.push_back(l);
		}
	}
	rem = "";
	{
		readParametr_n(rem, "ignore");
		IStrStream in;
		in.str(rem);
		while (!in.eof()) {
			String l;
			getline(in, l);
			l = trim(l);
			if (l.size() > 1)
				cnf.ignoreIP.push_back(l);
		}
	}
	if (!ctrl.getConfig().CheckT2S(cnf)) {
		notifyUser(user_id, "Fail to add VPN mode\n");
		return makeRedirect(req, "/vpn.html");
	}
	cnf.isNull = false;
	ctrl.getConfig().tun2socksConf.push_back(cnf);
	ctrl.SaveConfig();

	return makeRedirect(req, "/vpn.html");
}

Request WebUI::processGetVPNRefresh(Request req) {
	if (!ConteinsKey(req->get, "refresh"))
		return HttpServer::generate404(req->method, req->host, req->path);

	auto & ctrl = ShadowSocksController::Get();
	Tun2SocksConfig conf = ctrl.getConfig().findVPNbyName(req->get["refresh"]);

	if (conf.isNull)
		return HttpServer::generate404(req->method, req->host, req->path);

	String route = Tun2Socks::DetectDefaultRoute();
	String tapInterface = Tun2Socks::DetectInterfaceName();
	conf.tunName = tapInterface;
	conf.defaultRoute = route;
	ctrl.getConfig().deleteVPNByName(conf.name);
	ctrl.getConfig().tun2socksConf.push_back(conf);
	ctrl.SaveConfig();

	return makeRedirect(req, "/vpn.html");
}
