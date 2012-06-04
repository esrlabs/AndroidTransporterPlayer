#include "Socket.h"
#include <unistd.h>
#include <arpa/inet.h>
#include <cassert>

namespace android {
namespace net {

Socket::Socket() :
	mSocketId(-1),
	mIsConnected(false) {
}

Socket::~Socket() {
	close();
}

bool Socket::connect(const char* ipAddress, const uint16_t port) {
	mSocketId = ::socket(AF_INET, SOCK_STREAM, 0);
	if(mSocketId < 0) {
		return false;
	}
	else {
		memset(&mSocket, 0, sizeof(mSocket));
		mSocket.sin_addr.s_addr = inet_addr(ipAddress);
		mSocket.sin_family = AF_INET;
		mSocket.sin_port = htons(port);
		if(::connect(mSocketId, (struct sockaddr*)&mSocket, sizeof(mSocket)) == 0) {
			mIsConnected = true;
			return true;
		}
		else {
			::close(mSocketId);
			return false;
		}
	}
}

int32_t Socket::read(uint8_t* data, const uint32_t size) {
	return ::recv(mSocketId, reinterpret_cast<char*>(data), size, 0);
}

int32_t Socket::readFully(uint8_t* data, const uint32_t size) {
	uint32_t dataSize = 0;
	int32_t result = 0;
	while (dataSize < size) {
		result = ::recv(mSocketId, reinterpret_cast<char*>(data + dataSize), size - dataSize, 0);
		if (result > 0) {
			dataSize += result;
		} else {
			break;
		}
	}
	return (result > 0) ? dataSize : -1;
}

bool Socket::write(const void* data, const uint32_t size) {
	return ::send(mSocketId, reinterpret_cast<const char*>(data), size, 0) == size;
}

void Socket::close() {
	if (mIsConnected) {
		::close(mSocketId);
		mIsConnected = false;
	}
}

} /* namespace net */
} /* namespace android */
