#include "RtspSocket.h"
#include <sys/socket.h>

RtspSocket::RtspSocket() :
	Socket() {
}

RtspSocket::RtspSocket(const char* host, uint16_t port) :
	Socket(host, port) {
}

int32_t RtspSocket::readPacket(uint8_t* data, const uint32_t size) {
	uint32_t dataSize = 0;
	int32_t result = 0;
	while (dataSize < size) {
		result = ::recv(mSocketId, reinterpret_cast<char*>(data + dataSize), size - dataSize, 0);
		if (result > 0) {
			dataSize += result;
			if (data[dataSize - 4] == '\r' && data[dataSize - 3] == '\n' && data[dataSize - 2] == '\r' && data[dataSize - 1] == '\n') {
				break;
			}
		} else {
			break;
		}
	}
	return (result > 0) ? dataSize : -1;
}
