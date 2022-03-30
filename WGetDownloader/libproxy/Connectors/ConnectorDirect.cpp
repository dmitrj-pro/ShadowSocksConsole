#include "ConnectorDirect.h"


TCPClient * ConnectorDirect::makeConnect(MakeConnect, const String & host, unsigned short port) {
	TCPClient * res = new TCPClient();
	if (res->Connect(host, port)) {
		return res;
	}
	delete res;
	return nullptr;

}
