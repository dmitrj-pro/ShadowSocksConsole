#pragma once
#include "Connector.h"

class ConnectorSocks : public Connector{
	private:
		String host;
		unsigned short port;
	public:
		ConnectorSocks(const String & host, unsigned short port) : host(host), port(port) {}
		virtual TCPClient * makeConnect(MakeConnect make, const String & host, unsigned short port) override;
};
