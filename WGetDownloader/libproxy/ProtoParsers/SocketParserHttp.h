#pragma once
#include "SocketParser.h"

class SocketParserHttp: public SocketParser{
	public:
		SocketParserHttp() {}
		virtual bool tryParse(SocketReader * cl, MakeConnect make) const override;

};
