#include "SocketParser.h"

#define byte char
class SocketParserSocks5: public SocketParser{
	private:

		bool read_headers_and_send_hello(SocketReader * socks) const;
		struct readed_host_port{
			byte cmd = 0;
			String host = "";
			byte host_type = 0;
			int port = 0;
		};
		readed_host_port read_host_port(SocketReader * socks) const;
		enum errors {
			request_granted = 0x00,
			general_failure = 0x01,
			connection_not_allowed_by_ruleset = 0x02,
			network_unreachable = 0x03,
			host_unreachable = 0x04,
			connection_refused_by_destination_host = 0x05,
			TTL_expired = 0x06,
			command_not_supported = 0x07,
			address_type_not_supported = 0x08
		};
		void send_error(SocketReader *, errors error, byte address_type, const String &addr, short port) const;
	public:
		SocketParserSocks5() {}
		virtual bool tryParse(SocketReader * cl, MakeConnect make) const override;
};
#undef byte
