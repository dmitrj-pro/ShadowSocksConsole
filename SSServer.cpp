#include "SSServer.h"
#include <_Driver/ThreadWorker.h>

void ShadowSocksServer::Start(const String & host, unsigned short port) {
	_server = new __DP_LIB_NAMESPACE__::TCPServer();
	_server->ThreadListen(host, port,  std::bind(&ShadowSocksServer::onNewClient, this, std::placeholders::_1));
}

void ShadowSocksServer::Stop() {
	_server->exit();
}

void ShadowSocksServer::onNewClient(__DP_LIB_NAMESPACE__::TCPServerClient client) {
	SSStream * str = new SSStream([&client](char * text, int count) {
		client.Send(text, count);
	}, [&client] (int count) {
		return client.ReadN(count);
	}, [&client] () {
		client.Close();
	});
	ConsoleLooper<SSStream,SSStream> looper(*str, *str);
	looper.Load();
	loppers.push_back(looper);
	looper.Loop();
}

void ShadowSocksServer::StartInNewThread(const String & host, unsigned short port) {
	static __DP_LIB_NAMESPACE__::Thread * th = new __DP_LIB_NAMESPACE__::Thread(std::bind(&ShadowSocksServer::Start, this, host, port));
	th->SetName("ShadowSocksServer::Start");
	th->start();
}

bool ShadowSocksServer::IsCanConnect(const String & host, unsigned short port) {
	return __DP_LIB_NAMESPACE__::TCPClient::IsCanConnect(host, port);
}

void getline(SSStream & str, String & res) {
	while (!str.eof() && str.size() == 0)
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	if (str.eof())
		return;
	res = str.getline();
}
