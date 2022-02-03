#include "SocketParserSocks5.h"
#include <Log/Log.h>
#include <Converter/Converter.h>
#include <Parser/SmartParser.h>

#define __read(val, ret) \
	{\
		if (!socks->ReadN(val)) { \
			return ret; \
		} \
	}

#define byte char

bool SocketParserSocks5::read_headers_and_send_hello(SocketReader * socks) const {
	// ToDo
	// На уровне выше вычитали версию.
	byte buf [20];
	byte version = 5;
	if (! socks->ReadN(&version, sizeof(byte))) {
		return false;
	}
	if (version != 5) {
		socks->setBuffer(&version, sizeof(byte));
		return false;
	}
	buf[0] = version;
	byte methods_size;
	if (! socks->ReadN(&methods_size, sizeof(byte))) {
		socks->setBuffer(&version, sizeof(byte));
		return false;
	}
	buf[1] = methods_size;

	// Методы авторизации. Пускаем без авторизации, но заголовок нужно вычитать
	for (unsigned char i = 0 ; i < methods_size; i++) {
		byte method;
		if (! socks->ReadN(&method, sizeof(byte))) {
			socks->setBuffer(buf, sizeof(byte) * i + 2);
			return false;
		}
		buf[2 + i] = method;
		/*
		0x00 	Аутентификация не требуется
		0x01 	GSSAPI
		0x02 	Имя пользователя / пароль RFC 1929
		0x03-0x7F 	Зарезервировано IANA
		0x03 	CHAP
		0x04 	Не занято
		0x05 	Вызов-ответ (аутентификация)
		0x06 	SSL
		0x07 	NDS аутентификация
		0x08 	Фреймворк многофакторной аутентификации
		0x09 	Блок JSON параметров
		0x0A–0x7F 	Не занято
		*/
	}

	char response[2];
	response[0] = 5;
	response[1] = 0;
	/*
	1 байт 	Номер версии SOCKS (должен быть 0x05 для этой версии)
	1 байт 	Выбранный метод аутентификации или 0xFF, если не было предложено приемлемого метода
	*/
	socks->Send(response, 2);

	return true;
}

SocketParserSocks5::readed_host_port SocketParserSocks5::read_host_port(SocketReader * socks) const {
	/*
	1 байт 	Номер версии SOCKS (должен быть 0x05 для этой версии)
	1 байт 	Код команды:
		0x01 = установка TCP/IP соединения
		0x02 = назначение TCP/IP порта (binding)
		0x03 = ассоциирование UDP-порта
	1 байт 	Зарезервированный байт, должен быть 0x00
	1 байт 	Тип адреса:
		0x01 = адрес IPv4
		0x03 = имя домена
		0x04 = адрес IPv6
	Зависит от типа адреса 	Назначение адреса:
		4 байта для адреса IPv4
		Первый байт — длина имени, затем следует имя домена без завершающего нуля на конце
		16 байт для адреса IPv6
	2 байта 	Номер порта, в порядке от старшего к младшему (big-endian)
	 */
	byte version;
	if (! socks->ReadN(&version, sizeof(byte))) {
		return readed_host_port();
	}
	if (version != 5) {
		socks->Close();
		return readed_host_port();
	}
	readed_host_port res;

	if (! socks->ReadN(&res.cmd, sizeof(byte))) {
		return readed_host_port();
	}
	if (! socks->ReadN(&version, sizeof(byte))) {
		return readed_host_port();
	}
	if (! socks->ReadN(&res.host_type, sizeof(byte))) {
		return readed_host_port();
	}

	if (res.host_type == 1) { // IpV4
		byte addr[4];
		if (!socks->ReadN(addr, 4 * sizeof(byte))) {
			return readed_host_port();
		}
		for (unsigned char i = 0; i < 4; i++)
			res.host = res.host + (res.host.size() > 0 ? "." : "") + __DP_LIB_NAMESPACE__::toString((short)(addr[i]));
	}
	if (res.host_type == 3) { // Domain
		byte length;
		if (! socks->ReadN(&length, sizeof(byte))) {
			return readed_host_port();
		}
		char * d = socks->ReadN(length);
		d[length] = 0;
		res.host = d;
		delete[] d;
	}
	if (res.host_type == 4) {
		send_error(socks, address_type_not_supported, res.host_type, "127.0.0.1", 0);
		socks->Close();
		return readed_host_port();
	}

	byte f, s;
	if (! socks->ReadN(&f, sizeof(byte))) {
		return readed_host_port();
	}
	if (! socks->ReadN(&s, sizeof(byte))) {
		return readed_host_port();
	}
	unsigned char f2 = f;
	unsigned char s2 = s;
	res.port = f2 * 256 + s2;
	return res;
}

void SocketParserSocks5::send_error(SocketReader * socks, errors error, byte address_type, const String & addr, short port) const {
	/*
	1 байт 	Номер версии SOCKS (0x05 для этой версии)
	1 байт 	Код ответа:
		0x00 = запрос предоставлен
		0x01 = ошибка SOCKS-сервера
		0x02 = соединение запрещено набором правил
		0x03 = сеть недоступна
		0x04 = хост недоступен
		0x05 = отказ в соединении
		0x06 = истечение TTL
		0x07 = команда не поддерживается / ошибка протокола
		0x08 = тип адреса не поддерживается
	1 байт 	Байт зарезервирован, должен быть 0x00
	1 байт 	Тип последующего адреса:
		0x01 = адрес IPv4
		0x03 = имя домена
		0x04 = адрес IPv6
	Зависит от типа адреса 	Назначение адреса:
		4 байта для адреса IPv4
		Первый байт — длина имени, затем следует имя домена без завершающего нуля на конце
		16 байт для адреса IPv6

	2 байта 	Номер порта, в порядке от старшего к младшему (big-endian)
	*/
	unsigned short length = 4 + (address_type == 1 ? 4 : 1 + addr.size()) + 2;
	char * response = new char[length];
	response[0] = (byte) 5;
	response[1] = (byte) error;
	response[2] = (byte) 0;
	response[3] = address_type;
	int i = 4;
	if (address_type == 1) {
		auto d = __DP_LIB_NAMESPACE__::split(addr, '.');
		for (const String & p : d)
			response[i++] = __DP_LIB_NAMESPACE__::parse<unsigned short>(p);
	}
	if (address_type == 3) {
		response[i++] = addr.size();
		for (char c : addr)
			response[i++] = c;
	}
	// ToDo
	if (address_type == 4) {
		auto d = __DP_LIB_NAMESPACE__::split(addr, '.');
		for (const String & p : d)
			response[i++] = __DP_LIB_NAMESPACE__::parse<unsigned short>(p);
	}
	response[i++] = port / 256;
	response[i++] = port % 256;

	socks->Send(response, length);
	delete [] response;
}

bool SocketParserSocks5::tryParse(SocketReader * cl, MakeConnect make) const {
	if (!read_headers_and_send_hello(cl)) {
		return false;
	}
	readed_host_port connectData = read_host_port(cl);
	if (connectData.cmd == 1) {
		TCPClient * target = make(connectData.host, connectData.port);
		if (target == nullptr) {
			DP_LOG_DEBUG << cl->ip() << ":" << cl->port() << " => " << connectData.host << connectData.port << ": fail";
			send_error(cl, network_unreachable, connectData.host_type, connectData.host, connectData.port);
			cl->Close();
			return true;
		}
		DP_LOG_DEBUG << cl->ip() << ":" << cl->port() << " => " << connectData.host << connectData.port << ": success";
		send_error(cl, request_granted, 1, "127.0.0.1", connectData.port);
		tcpLoop(*cl, target);
		delete target;
		DP_LOG_DEBUG << "Socks5 disconnected " << cl->ip() << ":" << cl->port() << " => " << connectData.host << ":" << connectData.port << "\n";
	} else {
		send_error(cl, command_not_supported, connectData.host_type, connectData.host, connectData.port);
		cl->Close();
	}
	return true;
}
