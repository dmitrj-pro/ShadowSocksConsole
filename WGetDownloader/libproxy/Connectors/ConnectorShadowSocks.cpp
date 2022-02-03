#include "ConnectorShadowSocks.h"
#include "../Log.h"
#include "../Network/TCPServer.h"
#include "../ProtoParsers/SocketParserTunnel.h"
#include "./ConnectorSocks.h"
#include "./ConnectorDirect.h"
#include <mutex>

using std::string;

unsigned short findAllowPort(const String & host) {
	while (true) {
		unsigned short v = rand() % 65025 + 1024;
		if (TCPServer::portIsAllow(host, v))
			return v;
	}
	return 0;
}

TCPClient * ConnectorShadowSocks::makeConnect(MakeConnect make, const std::string & host, unsigned short port) {
	if (config->run == nullptr) {
		config->locker.lock();
		if (config->run == nullptr) {
			config->run = new __DP_LIB_NAMESPACE__::Application(config->applicationPath);
			__DP_LIB_NAMESPACE__::Application & app = *config->run;
			config->out_port = findAllowPort(config->localhost);
			config->in_port = findAllowPort(config->localhost);

			app << "-cipher" << config->hostData.login;
			app << "-password" << config->hostData.password;
			if (config->pluginPath.size() > 0) {
				app << "-plugin" << config->pluginPath;
				if (config->pluginParams.size() > 0) {
					app << "-plugin-opts" << config->pluginParams;
				}
			}
			app << "-socks" << (config->localhost + ":" + toString(config->out_port));

			SocketParserTunnel * parser = new SocketParserTunnel(config->hostData.host, config->hostData.port);

			config->server = new TCPServer();
			config->server_thread = new Thread([this, parser, make](){
				config->server->ThreadListen(config->localhost, config->in_port, [parser, make](TCPServerClient socks) {
					SocketReader reader(&socks);
					parser->tryParse(&reader, make);
				});
			});
			config->server_thread->start();

			app << "-c" << (config->localhost + ":" + toString(config->in_port)); // << //ToDo;
			app.ExecInNewThread();
			app.WaitForStart();

			config->connector = new ConnectorSocks(config->localhost, config->out_port);
			config->directConnector = new ConnectorDirect();
		}
		config->locker.unlock();
	}

	return config->connector->makeConnect([this](const std::string & host, unsigned short port){
		return this->config->directConnector->makeConnect(nullptr, host, port);
	}, host, port);
}
