#pragma once
#include "Connector.h"

class ConnectorDirect : public Connector{
	public:
		ConnectorDirect() {}
		virtual TCPClient * makeConnect(MakeConnect make, const String & host, unsigned short port) override;
};
