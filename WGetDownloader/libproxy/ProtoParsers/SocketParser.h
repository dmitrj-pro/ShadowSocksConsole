#pragma once
#include <Network/TCPServer.h>
#include "../Chain.h"


using __DP_LIB_NAMESPACE__::TCPServerClient;
using __DP_LIB_NAMESPACE__::UInt;

#define SocketReaderBuffer 300
class SocketReader{
	private:
		char buffer[SocketReaderBuffer];
		UInt bufferPos = 0;
		UInt bufferSize = 0;
		TCPServerClient * client;
	public:
		SocketReader(TCPServerClient * cl):client(cl) {};
		int Send(const String & str);
		int Send(const char * data, UInt length);
		//std::string Read(char del);
		char * ReadN(UInt size);
		inline String ip() const {return client->ip; }
		inline unsigned short port() const { return client->port; }
		UInt ReadN(char * data, UInt size);
		void setBuffer(char * buffer, UInt length);
		void Close();
		inline UInt getBufferSize() const { return bufferSize; }
		inline UInt getAvailableSize() const { return bufferSize - bufferPos; }
		inline TCPServerClient * getClient() { return client; }

};


#define BUFFER_SIZE 4096

void tcpLoop(SocketReader & client,   TCPClient * reader);

class SocketParser{
	private:
		Chain * chain;
	protected:
		//inline virtual SocketReader * makeConnect(const std::string & host, unsigned short port) { return new SocketReader(chain->makeConnect(host, port)); }
	public:
		typedef std::function<TCPClient *(const String & host, unsigned short port)> MakeConnect;

		virtual ~SocketParser() {}
		inline void setChain(Chain * c) { chain = c; }

		virtual bool tryParse(SocketReader * cl, MakeConnect make) const = 0;

};
