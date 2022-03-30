#include "SocketParserHttp.h"
#include <string.h>
#include <Log/Log.h>
#include <Network/Utils.h>
#include <Converter/Converter.h>



bool SocketParserHttp::tryParse(SocketReader * cl, MakeConnect make) const {
	#define buffer_size 256
	char buffer [buffer_size];
	int readed = cl->ReadN(buffer, 7);
	if (readed != 7) {
		cl->setBuffer(buffer, readed);
		return false;
	}

	buffer[7] = 0;

	String res_400 = "HTTP/1.1 400 Bad Request\r\n\
Server: nginx 5.0.1\r\n\
Connection: close\r\n\
Content-Length: 193\r\n\
Content-Type: text/html\r\n\
\r\n\
\r\n\
<html>\r\n\
	<head>\r\n\
		<title>400 Bad Request</title>\r\n\
	</head>\r\n\
	<body>\r\n\
		<h1>400 Bad Request</h1>\r\n\
		<p>Unexpected CONNECT request.</p>\r\n\
	</body>\r\n\
</html>";

	#ifdef BUFFER_SIZE
		#undef BUFFER_SIZE
	#endif
	#define BUFFER_SIZE 1024
	if (strncmp(buffer, "CONNECT", 7) == 0) {
		char buffer[BUFFER_SIZE];
		String host = "";
		String port = "";
		short mode = 0;

		bool isBreak = false;
		while (!isBreak) {
			unsigned int readed = cl->ReadN(buffer, BUFFER_SIZE - 1);
			unsigned int i = mode == 0 ? 1 : 0;
			for (; i < readed; i++) {
				if (buffer[i] == ' ' && mode < 2) {
					// Мы прочитали то, что нам нужно, но нужно дочитать до \r\n\r\n
					mode = 2;
				}
				if (buffer[i] == ':') {
					mode++;
					continue;
				}
				if (buffer[i] == '\r' && mode == 2)
					mode = 3;
				if (buffer[i] == '\n' && mode == 3)
					mode = 4;
				if (buffer[i] == '\r' && mode == 4)
					mode = 5;
				if (buffer[i] == '\n' && mode == 5) {
					i++;
					isBreak = true;
					break;
				}
				if ( (mode >= 3 && mode <= 5) && (buffer[i] != '\n' && buffer[i] != '\r'))
					mode = 2;
				if (mode == 0)
					host += buffer[i];

				if (mode == 1)
					port += buffer[i];
			}
			cl->setBuffer(buffer + i, readed - i);

		}

		DP_LOG_DEBUG << "Try " << cl->ip() << ":" << cl->port() << " => " << host << ":" << port;
		if (host.size() == 0 || port.size() == 0) {
			cl->Send(res_400);
			cl->Close();
			DP_LOG_DEBUG << "Fail " << cl->ip() << ":" << cl->port() << " => " << host << ":" << port;
			return true;
		}
		DP_LOG_DEBUG << "Success " << cl->ip() << ":" << cl->port() << " => " << host << ":" << port;
		String res_200 = "HTTP/1.1 200 Connection established\r\n\
\r\n";
		TCPClient * target = make(host, __DP_LIB_NAMESPACE__::parse<unsigned short>(port));
		if (target == nullptr) {
			DP_LOG_DEBUG << cl->ip() << ":" << cl->port() << " => " << host << port << ": fail";
			cl->Send(res_400);
			cl->Close();
			return true;
		}
		DP_LOG_DEBUG << cl->ip() << ":" << cl->port() << " => " << host << port << ": success";
		cl->Send(res_200);
		tcpLoop(*cl, target);
		delete target;
		DP_LOG_TRACE << "Http disconnected " << cl->ip() << ":" << cl->port() << " => " << host << ":" << port << "\n";
	} else {
		if ((buffer[0] >= 'a' && buffer[0] <= 'z') || (buffer[0] >= 'A' && buffer[0] <= 'Z')) {
			if ((buffer[1] >= 'a' && buffer[1] <= 'z') || (buffer[1] >= 'A' && buffer[1] <= 'Z')) {
				char buffer_local[SocketReaderBuffer];
				String url = "";
				url.reserve(256);

				__DP_LIB_NAMESPACE__::URLElement parsed_host;

				short mode = 0;
				strncpy(buffer_local, buffer, 7);
				unsigned int readed = cl->ReadN(buffer_local + 7, SocketReaderBuffer - 7);

				char buffer_for_buffer[SocketReaderBuffer];
				unsigned short buffer_for_buffer_pos = 0;

				for (unsigned int i = 0; i < readed+7; i++) {
					if (mode == 1) {
						if (buffer_local[i] == ' ') {
							mode ++;
							parsed_host = __DP_LIB_NAMESPACE__::URLElement::parse(url);
							strncpy(buffer_for_buffer + buffer_for_buffer_pos, parsed_host.path.c_str(), parsed_host.path.size());
							buffer_for_buffer_pos += parsed_host.path.size();
							strncpy(buffer_for_buffer + buffer_for_buffer_pos, " ", 1);
							buffer_for_buffer_pos++;
							continue;
						}
						url += buffer_local[i];
						continue;
					}
					if (buffer_local[i] == ' ')
						mode ++;

					buffer_for_buffer[buffer_for_buffer_pos++] = buffer_local[i];
				}
				cl->setBuffer(buffer_for_buffer, buffer_for_buffer_pos);

				if (parsed_host.port == 0) {
					//if (parsed_host.type == __DP_LIB_NAMESPACE__::URLElement::Type::Http)
						parsed_host.port = 80;
					if (parsed_host.type == __DP_LIB_NAMESPACE__::URLElement::Type::Https)
						parsed_host.port = 443;
				}

				TCPClient * target = make(parsed_host.host, parsed_host.port);
				if (target == nullptr) {
					DP_LOG_DEBUG << cl->ip() << ":" << cl->port() << " => " << parsed_host.host << parsed_host.port << ": fail";
					cl->Send(res_400);
					cl->Close();
					return true;
				}
				DP_LOG_DEBUG << cl->ip() << ":" << cl->port() << " => " << parsed_host.host << parsed_host.port << ": success";
				tcpLoop(*cl, target);
				delete target;
				DP_LOG_TRACE << "Http disconnected " << cl->ip() << ":" << cl->port() << " => " << parsed_host.host << ":" << parsed_host.port << "\n";
			} else {
				cl->setBuffer(buffer, 7);
				return false;
			}
		} else {
			cl->setBuffer(buffer, 7);
			return false;
		}
	}
	return true;

}
