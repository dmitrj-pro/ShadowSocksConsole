#include "ConnectorDirect.h"


TCPClient * ConnectorDirect::makeConnect(MakeConnect make, const String & host, unsigned short port) {
	TCPClient * res = new TCPClient();
	if (res->Connect(host, port)) {
		return res;
	}
	delete res;
	return nullptr;

}
