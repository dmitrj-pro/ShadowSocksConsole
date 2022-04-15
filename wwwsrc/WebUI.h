#pragma once
#include <_Driver/ThreadWorker.h>
#include <Addon/httpsrv/HttpPathRouter.h>
#include "../ShadowSocksController.h"
#include "../VERSION.h"
#include "../www/textfill.h"

using __DP_LIB_NAMESPACE__::List;
using __DP_LIB_NAMESPACE__::String;
using __DP_LIB_NAMESPACE__::Map;
using __DP_LIB_NAMESPACE__::Thread;

struct UserSession {
	List<String> notify;
	int started = 0;
	String cookie ="";
};

class WebUI : public ShadowSocksControllerUpdateStatus {
	private:
		inline Request makeRequest() const {
			Request resp(new HttpRequest());
			resp->headers["Cache-Control"] = "no-cache";
			resp->headers["Server"] = "nginx";
			resp->status = 200;
			resp->headers["Content-Type"] = getMimeType("index.html");
			return resp;
		}
		inline Request makeRedirect(Request, const String & path = "/") {
			Request r = makeRequest();
			r->status = 301;
			//r->headers["Location"] = "http://" + req->host + path;
			r->headers["Location"] = path;
			return r;
		}
		inline String makePage(const String & page_header, const String & page_name, const String & parametrs) const {
			return findFillText("template.txt", List<String>({page_header, SS_FULL_VERSION, findText("menu.txt"), page_name, parametrs}));
		}
		inline String makePage(const String & page_header, const String & page_name, const String & page_path, const List<String> & parametrs) const {
			return findFillText("template.txt", List<String>({page_header, SS_FULL_VERSION, findText("menu.txt"), page_name, findFillText(page_path, parametrs)}));
		}
		inline String makePage(const String & page_name, const String & page_path, const List<String> & parametrs) const {
			return makePage(page_name, page_name, page_path, parametrs);
		}

		String web_host = "localhost";
		unsigned short web_port = 8080;
		SmartPtr<Thread> thread = SmartPtr<Thread>(nullptr);

		SmartPtr<HttpHostPathRouterServer> srv = SmartPtr<HttpHostPathRouterServer>(nullptr);

		List<UserSession> cookies = List<UserSession>();
		std::mutex cookies_lock;

		void notifyUser(const String & user_id, const String & msg);
		UserSession & getUserSession(const String & id);
		bool existsUserSession(const String & id) const;

		Request processGetMain(Request req);
		Request processPostMain(Request req);

		Request processPostLogin(Request req);

		Request processGetNews(Request req);
		Request processCheckNews(Request req);

		Request processGetTasks(Request req);
		Request processPostTaskStart(Request req);
		Request processGetTaskStartPage(Request req);
		Request processPostTaskTuns(Request req);
		Request processGetTaskTuns(Request req);
		Request processPostTaskStartPage(Request req);
		Request processPostTaskStop(Request req);
		Request processGetTaskEditPage(Request req);
		Request processPostTaskEdit(Request req);
		Request processGetTaskDelete(Request req);
		Request processPostTaskDelete(Request req);
		Request processGetNewTaskPage(Request req);
		Request processPostNewTaskPage(Request req);
		Request processPostCheckServers(Request req);
		Request processGetCheckServers(Request req);
		Request processPostCheckTask(Request req);
		Request processGetCheckTask(Request req);

		Request processGetRuns(Request req);
		Request processGetAddRuns(Request req);
		Request processPostAddRuns(Request req);
		Request processGetEditRuns(Request req);
		Request processPostEditRuns(Request req);
		Request processGetDeleteRuns(Request req);
		Request processPostDeleteRuns(Request req);

		Request processGetServers(Request req);
		Request processPostServerDelete(Request req);
		Request processGetServerDelete(Request req);
		Request processGetServerEditPage(Request req);
		Request processPostServerEditPage(Request req);
		Request processGetNewServerPage(Request req);
		Request processPostNewServerPage(Request req);

		Request processGetVPN(Request req);
		Request processGetVPNDelete(Request req);
		Request processPostVPNDelete(Request req);
		Request processGetVPNEditPage(Request req);
		Request processGetVPNRefresh(Request req);
		Request processPostVPNEditPage(Request req);
		Request processGetNewVPNPage(Request req);
		Request processPostNewVPNPage(Request req);

		Request processGetVariables(Request req);
		Request processGetAddVariables(Request req);
		Request processPostAddVariables(Request req);
		Request processGetEditVariables(Request req);
		Request processPostEditVariables(Request req);
		Request processGetDeleteVariables(Request req);
		Request processPostDeleteVariables(Request req);

		Request processGetSettings(Request req);
		Request processGetEditSettings(Request req);
		Request processPostEditSettings(Request req);

		Request processGetExport(Request req);
		Request processPostExport(Request req);
		Request processGetLogout(Request req);
		Request processGetExit(Request req);
		Request processPostExit(Request req);
		struct UtilsStruct{
			String check_port_host = "";
			unsigned short check_port_port = 0;
			String check_port_result = "";

			String check_connect_host = "";
			unsigned short check_connect_port = 0;
			String check_connect_result = "";

			String resolve_domain;
			List<String> resolve_result;

			String find_free_host ="";
			unsigned short find_free_count = 0;
			List<unsigned short> find_free_result;
		};
		UtilsStruct makeUtilsStruct() const;

		Request processGetUtils(Request req, const UtilsStruct & res);
		Request processPostUtils(Request req);

		Request processGetStaticFile(Request req);
		void _start();

	public:
		WebUI(const String & web_host="localhost", unsigned short port = 35080): web_port(port), web_host(web_host) {}
		void stop();
		void start();

		virtual void UpdateServerStatus(const String & server, const String & msg) override;
		virtual void UpdateTaskStatus(const String & server, const String & msg) override;
};

#define readParametr(TO, X, D) \
	{ \
		if (req->post.find(X) == req->post.end() || req->post[X].value_size < 1) { \
			Request resp = makeRequest(); \
			resp->status = 400; \
			String v = "Parametr ";\
			v += X;\
			v += " is not found"; \
			resp->body = new char[v.size() + 1]; \
			strncpy(resp->body, v.c_str(), v.size()); \
			resp->body_length = v.size(); \
			if (D != nullptr) \
				delete D; \
			return resp; \
		} \
		TO = trim(req->post[X].value); \
	}
#define readParametr_nd(TO, X) \
	{ \
		if (req->post.find(X) == req->post.end() || req->post[X].value_size < 1) { \
			Request resp = makeRequest(); \
			resp->status = 400; \
			String v = "Parametr ";\
			v += X;\
			v += " is not found"; \
			resp->body = new char[v.size() + 1]; \
			strncpy(resp->body, v.c_str(), v.size()); \
			resp->body_length = v.size(); \
			return resp; \
		} \
		TO = trim(req->post[X].value); \
	}
#define readParametr_t(TO, X, T, D) \
	{ \
		if (req->post.find(X) == req->post.end() || req->post[X].value_size < 1) { \
			Request resp = makeRequest(); \
			resp->status = 400; \
			String v = "Parametr ";\
			v += X;\
			v += " is not found"; \
			resp->body = new char[v.size() + 1]; \
			strncpy(resp->body, v.c_str(), v.size()); \
			resp->body_length = v.size(); \
			if (D != nullptr) \
				delete D; \
			return resp; \
		} \
		TO = __DP_LIB_NAMESPACE__::parse<T>(trim(req->post[X].value)); \
	}

#define readParametr_n(TO, X) \
	{ \
		if (req->post.find(X) == req->post.end() || req->post[X].value_size < 1) { \
		} else \
			TO = trim(req->post[X].value); \
	}

#define readParametr_nt(TO, X, T) \
	{ \
		if (req->post.find(X) == req->post.end() || req->post[X].value_size < 1) { \
		} else \
			TO = __DP_LIB_NAMESPACE__::parse<T>(trim(__DP_LIB_NAMESPACE__::String(req->post[X].value))); \
	}
#define readParametr_ntv(TO, X, T, V) \
	{ \
		if (req->post.find(X) == req->post.end() || req->post[X].value_size < 1) { \
			TO = V; \
		} else \
			TO = __DP_LIB_NAMESPACE__::parse<T>(trim(__DP_LIB_NAMESPACE__::String(req->post[X].value))); \
	}

#define readParametr_boolv(TO, X, V) \
	{ \
		if (req->post.find(X) == req->post.end() || req->post[X].value_size < 1) { \
			TO = V; \
		} else \
			TO = __DP_LIB_NAMESPACE__::String(req->post[X].value) == "yes" ? true : false; \
	}
