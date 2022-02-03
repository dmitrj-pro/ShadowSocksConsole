#pragma once
#include <Network/TCPClient.h>
#include <functional>

using __DP_LIB_NAMESPACE__::TCPClient;
using __DP_LIB_NAMESPACE__::String;
using __DP_LIB_NAMESPACE__::List;
typedef unsigned char byte;

class Connector {
	public:
		typedef std::function<TCPClient *(const String & host, unsigned short port)> MakeConnect;
		virtual ~Connector() {}
		virtual TCPClient * makeConnect(MakeConnect make, const String & host, unsigned short port) = 0;
};
