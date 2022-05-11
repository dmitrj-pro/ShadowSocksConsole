#include "SocketParser.h"
#include <string.h>
#include <Log/Log.h>

int SocketReader::Send(const String & str) {
	return client->Send(str);
}
int SocketReader::Send(const char * data, unsigned int length) {
	return client->Send(data, length);
}

//String SocketReader::Read(char del) {
//	if (bufferSize > 0) {

//	} else
//		return client->Read(del);
//}
char * SocketReader::ReadN(UInt size) {
	if (bufferSize > 0) {
		char * res = new char[size + 1];
		if ((bufferSize - bufferPos) > size) {
			strncpy(res, buffer + bufferPos, size);
			bufferPos += size;
		} else {
			strncpy(res, buffer + bufferPos, bufferSize - bufferPos);
			bufferSize = 0;
			client->ReadN(res + (bufferSize - bufferPos), size - (bufferSize - bufferPos));
		}
		return res;
	} else {
		return client->ReadN(size);
	}
}
unsigned int SocketReader::ReadN(char * res, unsigned int size) {
	if (bufferSize > 0) {
		if ((bufferSize - bufferPos) >= size) {
			strncpy(res, buffer + bufferPos, size);
			bufferPos += size;
			return size;
		} else {
			UInt readed = bufferSize - bufferPos;
			strncpy(res, buffer + bufferPos, readed);
			readed += client->ReadN(res + (bufferSize - bufferPos), size - (bufferSize - bufferPos));
			bufferSize = 0;
			return readed;
		}
	} else {
		return client->ReadN(res, size);
	}
}
void SocketReader::Close() {
	client->Close();
}

void SocketReader::setBuffer(char * buffer, unsigned int length) {
	if (length > SocketReaderBuffer) {
		throw EXCEPTION("New buffer size > self buffer size");
		return;
	}
	strncpy(this->buffer, buffer, length);
	this->bufferSize = length;
	this->bufferPos = 0;
}

// void tcpLoop(TCPServerClient & socks, TCPClient * cl, unsigned int incommin_speed, unsigned int outgoing_speed)
void tcpLoop(SocketReader & client,  TCPClient * target) {
	UInt buffer_size = BUFFER_SIZE;
	char c[BUFFER_SIZE];
	if (client.getBufferSize() > 0) {
		unsigned int readed = client.ReadN(c, client.getBufferSize());
		c[readed] = 0;
		DP_LOG_TRACE << client.ip() << ":" << client.port() << " => " << "target " << readed << " bytes";
		target->Send(c, readed);
	}
	TCPServerClient * cl = client.getClient();

	while (true) {
		TCPClient r = target->WaitOneOf(*cl);
		unsigned int readed = r.ReadN(c, buffer_size);
		if (readed == 0) {
			client.Close();
			target->Close();
			break;
		}

		if (r == *cl) {
			DP_LOG_TRACE << cl->ip << ":" << cl->port << " => " << "target " << readed << " bytes";
			target->Send(c, readed);
		} else {
			DP_LOG_TRACE << "target " << " => " << cl->ip << ":" << cl->port << readed << " bytes";
			client.Send(c, readed);
		}
	}
}
