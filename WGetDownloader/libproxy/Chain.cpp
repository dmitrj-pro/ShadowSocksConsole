#include "Chain.h"


TCPClient * Node::makeConnect(const String & host, unsigned short port) {
	return connector->makeConnect([this](const String & host, unsigned short port){
		return this->next->makeConnect(host, port);
	}, host, port);
}

TCPClient * Chain::makeConnect(const String & host, unsigned short port) {
	return nodes->makeConnect(host, port);
}

