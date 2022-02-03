#pragma once
#include "SocketParser.h"
#include <DPLib.conf.h>

using __DP_LIB_NAMESPACE__::List;

class SocketParserMultiplex: public SocketParser{
	private:
		List<SocketParser *> parsers;
	public:
		SocketParserMultiplex();
		inline void addSocketParser(SocketParser * parser) { parsers.push_back(parser); }

		virtual bool tryParse(SocketReader * cl, MakeConnect make) const override;

};
