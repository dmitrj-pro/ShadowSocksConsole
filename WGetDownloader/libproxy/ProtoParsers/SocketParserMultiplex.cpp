#include "SocketParserMultiplex.h"
#include "SocketParserSocks5.h"
#include "SocketParserTunnel.h"
#include "SocketParserHttp.h"

SocketParserMultiplex::SocketParserMultiplex() {
	parsers.push_back(new SocketParserSocks5());
	parsers.push_back(new SocketParserHttp());
}

bool SocketParserMultiplex::tryParse(SocketReader * cl, MakeConnect make) const {
	for (auto it = parsers.cbegin(); it != parsers.cend(); it ++) {
		if ((*it)->tryParse(cl, make))
			return true;
	}
	return false;
}
