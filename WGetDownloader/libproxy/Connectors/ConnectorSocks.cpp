#include "ConnectorSocks.h"
#include <Log/Log.h>
#include <Network/Utils.h>
#include <Parser/SmartParser.h>
#include <Converter/Converter.h>

TCPClient * ConnectorSocks::makeConnect(MakeConnect make, const String & host, unsigned short port) {
	TCPClient * connect = make(this->host, this->port);

	if (connect == nullptr)
		return nullptr;
	if (host.size() > 256) {
		delete connect;
		return nullptr;
	}
	/*
	1 байт 	Номер версии SOCKS (должен быть 0x05 для этой версии)
	1 байт 	Количество поддерживаемых методов аутентификации
	n байт 	Номера методов аутентификации, переменная длина, 1 байт для каждого поддерживаемого метода
	*/
	{
		char c[3];
		c[0] = 5;
		c[1] = 1;
		c[2] = 0;
		connect->Send(c, 3);

		/*
		1 байт 	Номер версии SOCKS (должен быть 0x05 для этой версии)
		1 байт 	Выбранный метод аутентификации или 0xFF, если не было предложено приемлемого метода
		*/
		connect->ReadN(c, 2);
		if (c[0] != 5 || c[1] != 0) {
			connect->Close();
			delete connect;
			return nullptr;
		}
	}
	{
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
		byte address_type;
		if (__DP_LIB_NAMESPACE__::isIPv4(host))
			address_type = 1;
		else
			address_type = 3;

		unsigned short length = 4 + (address_type == 1 ? 4 : 1 + host.size()) + 2;
		//char * response = new char[length];
		// 4 + 1 + 256 + 2
		char response [ 264 ];
		response[0] = (byte) 5;
		response[1] = (byte) 1;
		response[2] = (byte) 0;
		response[3] = address_type;
		int i = 4;
		if (address_type == 1) {
			auto d = __DP_LIB_NAMESPACE__::split(host, '.');
			for (const String & p : d)
				response[i++] = __DP_LIB_NAMESPACE__::parse<byte>(p);
		}
		if (address_type == 3) {
			response[i++] = host.size();
			for (char c : host)
				response[i++] = c;
		}
		// ToDo
		if (address_type == 4) {
			auto d = __DP_LIB_NAMESPACE__::split(host, '.');
			for (const String & p : d)
				response[i++] = __DP_LIB_NAMESPACE__::parse<byte>(p);
		}
		response[i++] = port / 256;
		response[i++] = port % 256;

		connect->Send(response, length);

		connect->ReadN(response, 4);
		if (response[0] != 5 || response[1] != 0) {
			/*
			0x00 = запрос предоставлен
			0x01 = ошибка SOCKS-сервера
			0x02 = соединение запрещено набором правил
			0x03 = сеть недоступна
			0x04 = хост недоступен
			0x05 = отказ в соединении
			0x06 = истечение TTL
			0x07 = команда не поддерживается / ошибка протокола
			0x08 = тип адреса не поддерживается
			*/
			connect->Close();
			delete connect;
			return nullptr;
		}
		address_type = response[3];
		String n_host = "";
		unsigned short n_port = 0;
		if (address_type == 1) {
			connect->ReadN(response, 4);
			for (unsigned short i = 0; i < 4; i++)
				n_host = n_host + (i != 0 ? "." : "") + __DP_LIB_NAMESPACE__::toString((int) response[i]);
		}
		if (address_type == 3) {
			char len;
			connect->ReadN(&len, 1);
			char dom[256];
			connect->ReadN(dom, (int) len);
			n_host = dom;
		}
		// ToDo
		if (address_type == 4) {
			connect->ReadN(response, 4);
			for (unsigned short i = 0; i < 4; i++)
				n_host = n_host + (i != 0 ? "." : "") + __DP_LIB_NAMESPACE__::toString((int) response[i]);
		}
		connect->ReadN(response, 2);
		n_port = ((unsigned int) response[0]) * 256;
		unsigned char c = response[1];
		n_port += c;

		DP_LOG_DEBUG << "Socks5 Connect " << n_host << ":" << n_port << " => " << host << ":" << port << "\n";
	}
	return connect;
}
