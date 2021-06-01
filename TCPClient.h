#pragma once
#include <DPLib.conf.h>
using __DP_LIB_NAMESPACE__::String;
using __DP_LIB_NAMESPACE__::List;
using __DP_LIB_NAMESPACE__::UInt;

class TCPClient {
	private:
		int _socket;
	public:
		TCPClient():_socket(-1) {}
		bool Connect(const String & ip, UInt port);
		bool Send(const String & str);
		bool Send(const char * data, unsigned int length);
		String Read(char del);
		char * ReadN(unsigned int size);
		void Close();
		static bool IsCanConnect(const String & ip, UInt port, const String & dns_server);
};


bool isIPv4(const String & ip);
