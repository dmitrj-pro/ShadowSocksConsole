#include "TCPClient.h"
#include <Log/Log.h>
#include <Types/Exception.h>
#include <Converter/Converter.h>
#include <_Driver/Application.h>
#include "libSimpleDNS/dns.h"

#ifdef DP_LINUX
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

#ifdef DP_WINDOWS
#include <winsock2.h>
#include <winsock.h>
#include <unistd.h>
#endif

using __DP_LIB_NAMESPACE__::log;
using __DP_LIB_NAMESPACE__::toString;
using __DP_LIB_NAMESPACE__::parse;
using __DP_LIB_NAMESPACE__::Application;

#ifdef DP_WINDOWS
bool __sock_inited = false;
WSADATA __WSADATA__;

void __init_wsock() {
	if (__sock_inited) return;
	int res = WSAStartup(MAKEWORD(2, 2), &__WSADATA__);
	if (res != 0) {
		(log << "Fail init WinSock. Error code: ") * res;
		throw EXCEPTION("WinSock Error #" + toString(res));
	}
}
#endif

bool TCPClient::Connect(const String & ip, UInt port) {
	#ifdef DP_WINDOWS
		__init_wsock();
	#endif
	(log << "Try connect to '" << ip << ":" << port ) *"'";
	log.flush();
	_socket = 0;
	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	int err = errno;
	if (sock < 0) {
		#ifdef DP_WINDOWS
			(log * "ERROR: can't create socket ret " << sock << ": ") * WSAGetLastError();
		#else
			(log * "ERROR: can't create socket ret " ) * sock;
		#endif
		
		log.flush();
		return false;
	}
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(ip.c_str());
	_socket = sock;

	if (connect(_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		(log * "ERROR: can't connect to server.");
		close(_socket);
		_socket = -1;
		return false;
	}
	return true;
}

bool TCPClient::Send(const String & str) {
	if (_socket < 0)
		return false;
	(log << "SEND:") * str;
	while (send(_socket, str.c_str(), str.size(), 0) != str.size());
	return true;
}

bool TCPClient::Send(const char * data, unsigned int length) {
	if (_socket <= 0)
		return false;
	while(send(_socket, data, length, 0) != length);
	return true;
}

String TCPClient::Read(char del){
	if (_socket == -1) {
		throw EXCEPTION("Con't read from closed socket.");
	}
	int err=10;
	char Buf=0;
	String query;
	while (err > 0 && Buf != del){
		if (Buf!=0)
			query+=Buf;
		err = recv(_socket, &Buf, 1, 0);

	}
	if (err < 0) {
		log *  "ERROR: can't read from socket";
		return "";
	}

	if (err == 0) { _socket = -1; return ""; } //клиент разорвал соединение

	( log  << "Readed: " ) * query;
	return query;
}

char * TCPClient::ReadN(unsigned int size) {
	if (_socket == -1) {
		return nullptr;
	}
	char * res = new char[size + 1];

	int err = 10;
	char Buf=0;
	for (int i = 0; i < size; i++) {
		err = recv(_socket, &Buf, 1, 0);
		if (err < 0) {
			delete [] res;
			return nullptr;
		}
		if (err == 0) {
			_socket = -1;
			return res;
		}
		if (err > 0)
			res[i] = Buf;
	}
	res[size] = 0;

	return res;
}

void TCPClient::Close() {
	close(_socket);
	_socket = -1;
}

bool TCPClient::IsCanConnect(const String & ip, UInt port, const String & dns) {
	TCPClient cl;
	String _ip = ip;
	if (!isIPv4(ip)) {
		auto list = resolveDNS(ip, dns);
		if (list.size() == 0)
			return false;
		_ip = *(list.begin());
	}
	if (cl.Connect(_ip, port)) {
		cl.Close();
		return true;
	} else
		return false;
}

bool isIPv4(const String & ip) {
	unsigned char count_dot = 0;
	String buf = "";
	for (int i = 0 ; i < ip.size(); i++) {
		if (ip[i] == '.') {
			if (buf.size() == 0)
				return false;
			int value = parse<int>(buf);
			if (!(value >=0 && value <= 255))
				return false;
			count_dot++;
			buf = "";
			continue;
		}
		if (!(ip[i] >='0' && ip[i] <= '9'))
			return false;
		buf = buf + ip[i];
	}
	{
		if (buf.size() == 0)
			return false;
		int value = parse<int>(buf);
		if (!(value >=0 && value <= 255))
			return false;
	}
	if (count_dot == 3)
		return true;
	return false;
}
