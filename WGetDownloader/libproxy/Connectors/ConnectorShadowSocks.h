#pragma once
#include "Connector.h"
#include "../Network/HostParser.h"
#include "../Network/SmartPtr.h"
#include "../Network/Application.h"
#include "../Network/TCPServer.h"
#include "./ConnectorSocks.h"
#include "./ConnectorDirect.h"
#include "../Network/ThreadWorker.h"
#include <mutex>

struct ShadowSocksConfig{
	std::string applicationPath = "./ss.exe";
	ProxyElement hostData;
	std::string pluginPath = "";
	std::string pluginParams = "";

	__DP_LIB_NAMESPACE__::Application * run = nullptr;
	TCPServer * server = nullptr;
	std::string localhost = "127.0.0.1";
	ConnectorSocks * connector;
	Thread * server_thread = nullptr;
	ConnectorDirect * directConnector;
	unsigned short out_port = 0; // Ведет на ShadowSocks
	unsigned short in_port = 0; // Ведет на удаленный сервер
	std::mutex locker;
};

class ConnectorShadowSocks : public Connector{
	private:
		SmartPtr<ShadowSocksConfig> config = SmartPtr<ShadowSocksConfig>();
	public:
		ConnectorShadowSocks(const std::string & applicationPath, const ProxyElement & hostData, const std::string & pluginPath = "", const std::string & pluginParams = "") {
			config = SmartPtr<ShadowSocksConfig>( new ShadowSocksConfig());
			config->hostData = hostData;
			config->applicationPath = applicationPath;
			config->pluginPath = pluginPath;
			config->pluginParams = pluginParams;
		}

		virtual TCPClient * makeConnect(MakeConnect make, const std::string & host, unsigned short port) override;
};

unsigned short findAllowPort(const String & host);
