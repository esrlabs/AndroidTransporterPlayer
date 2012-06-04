#include "DatagramSocket.h"
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <cassert>

namespace android {
namespace net {

DatagramSocket::DatagramSocket() :
	mSocketId(-1) {
}

DatagramSocket::DatagramSocket(const uint16_t port) {
	bind(port);
}

DatagramSocket::~DatagramSocket() {
	close();
}

bool DatagramSocket::bind(const uint16_t port) {
	mSocketId = socket(AF_INET, SOCK_DGRAM, 0);
	memset(&mSocket, 0, sizeof(mSocket));
	mSocket.sin_family = AF_INET;
	mSocket.sin_addr.s_addr = htonl(INADDR_ANY);
	mSocket.sin_port = htons(port);
	return ::bind(mSocketId, (struct sockaddr*)&mSocket, sizeof(mSocket)) == 0;
}

bool DatagramSocket::bind(const char* ipAddress, const uint16_t port) {
	mSocketId = socket(AF_INET, SOCK_DGRAM, 0);
	memset(&mSocket, 0, sizeof(mSocket));
	mSocket.sin_family = AF_INET;
	mSocket.sin_addr.s_addr = inet_addr(ipAddress);
	mSocket.sin_port = htons(port);
	return ::bind(mSocketId, (struct sockaddr*)&mSocket, sizeof(mSocket)) == 0;
}

int32_t DatagramSocket::recv(uint8_t* data, const uint32_t size, struct sockaddr_in* clientSocket) {
	socklen_t socketSize = sizeof(clientSocket);
	return ::recvfrom(mSocketId, reinterpret_cast<char*>(data), size, 0, (struct sockaddr*)clientSocket, &socketSize);
}

bool DatagramSocket::send(const void* data, const uint32_t size, struct sockaddr_in* clientSocket) {
	socklen_t socketSize = sizeof(clientSocket);
	return ::sendto(mSocketId, reinterpret_cast<const char*>(data), size, 0, (struct sockaddr*)clientSocket, socketSize) == size;
}

void DatagramSocket::close() {
	if (mSocketId >= 0) {
		::close(mSocketId);
		mSocketId = -1;
	}
}

} /* namespace net */
} /* namespace android */
