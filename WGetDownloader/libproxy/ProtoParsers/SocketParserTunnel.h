#include "SocketParser.h"

class SocketParserTunnel: public SocketParser{
	private:
		String host;
		unsigned short port;
	public:
		SocketParserTunnel(const String & host, unsigned short port):host(host), port(port) {}
		virtual bool tryParse(SocketReader * cl, MakeConnect make) const override;
};
