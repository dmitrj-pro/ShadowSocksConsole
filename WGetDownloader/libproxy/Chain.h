#pragma once
#include "Connectors/Connector.h"
#include <DPLib.conf.h>

using __DP_LIB_NAMESPACE__::List;

class Node {
	private:
		Connector * connector;
		Node * next;
	public:
		Node(Connector * connector): connector(connector), next(nullptr) {}
		Node(Connector * connector, Node * next): connector(connector), next(next) {}
		TCPClient * makeConnect(const String & host, unsigned short port);
		inline Node * getNext() { return next; }
		inline Connector * getConnector() { return connector; }
};

class Chain {
	private:
		Node * nodes;
	public:
		Chain(List<Connector *> nodes);
		TCPClient * makeConnect(const String & host, unsigned short port);
};
