#include "ConnectorHttpProxy.h"
#include <Log/Log.h>
#include <Converter/Converter.h>

using __DP_LIB_NAMESPACE__::toString;

TCPClient * ConnectorHttpProxy::makeConnect(MakeConnect make, const String & host, unsigned short port) {
	TCPClient * connect = make(this->host, this->port);
	if (connect == nullptr)
		return nullptr;

	char delimer[3];
	delimer[0] = 0x0d;
	delimer[1] = 0x0a;
	delimer[2] = 0;


	String connectStr = "CONNECT " + host + ":" + toString(port) + " HTTP/1.1" + delimer;

	connectStr += "User-Agent: Mozilla/5.0 (Windows NT 6.1; Win64; x64; rv:89.0) Gecko/20100101 Firefox/89.0";
	connectStr += delimer;

	connectStr += "Proxy-Connection: keep-alive";
	connectStr += delimer;

	connectStr += "Connection: keep-alive";
	connectStr += delimer;

	connectStr += "Host: " + host + ":" + toString(port);
	connectStr += delimer;
	connectStr += delimer;

	connect->Send(connectStr);

	if (connect->ReadN(delimer, 2) != 2) {
		connect->Close();
		DP_LOG_ERROR << "Proxy server closed connection\n";
		delete connect;
		return nullptr;
	}
	if (!(delimer[0] == 'H' && delimer[1] == 'T')) {
		connect->Close();
		DP_LOG_ERROR << "Proxy server is not a proxy server\n";
		delete connect;
		return nullptr;
	}
	//Read HTTP/1.1
	String res = connect->Read(0x20);
	//Read status
	res = connect->Read(0x20);
	if (res != "200"){
		connect->Close();
		DP_LOG_ERROR << "Proxy server return code " + res + "\n";
		delete connect;
		return nullptr;
	}
	connect->Read(0x0a);
	connect->Read(0x0a);
	return connect;
}
