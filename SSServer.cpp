#include "SSServer.h"

void ShadowSocksServer::Start(int port) {
	_server = new DP::NET::Server::Server();
	_server->ThreadListen("0.0.0.0", port,  std::bind(&ShadowSocksServer::onNewClient, this, std::placeholders::_1));
}

void ShadowSocksServer::Stop() {
	_server->exit();
}

void ShadowSocksServer::onNewClient(int id) {
	SSStream * str = new SSStream([this, id](char * text, int count) {
		this->_server->Send(id, text, count);
	}, [this, id] (int count) {
		return this->_server->ReadN(id, count);
	}, [this, id] () {
		this->_server->CloseClient(id);
	});
	ConsoleLooper<SSStream,SSStream> looper(*str, *str);
	looper.Load();
	loppers.push_back(looper);
	looper.Loop();
}

void ShadowSocksServer::StartInNewThread(int port) {
	static std::thread * th = new std::thread(std::bind(&ShadowSocksServer::Start, this, port));
}

void getline(SSStream & str, String & res) {
	while (str.size() == 0)
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	res = str.getline();
}
