#pragma once
#include "Connector.h"

class ConnectorHttpProxy : public Connector{
	private:
		String host;
		unsigned short port;
	public:
		ConnectorHttpProxy(const String & host, unsigned short port) : host(host), port(port) {}
		virtual TCPClient * makeConnect(MakeConnect make, const String & host, unsigned short port) override;
};
