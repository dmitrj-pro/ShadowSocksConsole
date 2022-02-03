#include "SocketParserTunnel.h"

bool SocketParserTunnel::tryParse(SocketReader * cl, MakeConnect make) const {
	TCPClient * socket = make(this->host, this->port);
	if (socket == nullptr)
		return true;
	tcpLoop(*cl, socket);
	return true;
}
