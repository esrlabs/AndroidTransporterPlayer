#include "android/net/ServerSocket.h"
#include "android/net/Socket.h"
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

namespace android {
namespace net {

ServerSocket::ServerSocket() :
	mServerSocket(-1),
	mIsBound(false),
	mIsClosed(false) {
}

ServerSocket::ServerSocket(const uint16_t port) :
	mIsBound(false),
	mIsClosed(false) {
	bind(port);
}

ServerSocket::ServerSocket(const uint16_t port, const uint32_t backlog) :
	mIsBound(false),
	mIsClosed(false) {
	bind(port, backlog);
}

ServerSocket::~ServerSocket() {
	if(isBound() && !isClosed()) {
		close();
	}
}

bool ServerSocket::bind(const uint16_t port) {
	return bind(port, DEFAULT_BACKLOG);
}

bool ServerSocket::bind(const uint16_t port, const uint32_t backlog) {
	if(mIsBound) {
		return false;
	}

	mServerSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if(mServerSocket < 0) {
		return false;
	}
	sockaddr_in socketAddress;
	memset(&socketAddress, 0, sizeof(socketAddress));
    socketAddress.sin_family = AF_INET;
	socketAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	socketAddress.sin_port = htons(port);
	if(::bind(mServerSocket, (struct sockaddr*)&socketAddress, sizeof(socketAddress)) == 0) {
		if(::listen(mServerSocket, backlog) == 0) {
			mIsBound = true;
			return true;
		}
		else {
			::close(mServerSocket);
            return false;
		}
	}
    else {
		::close(mServerSocket);
		return false;
	}
}

android::os::sp<Socket> ServerSocket::accept() {
	android::os::sp<Socket> socket = new Socket();
	socket->mSocket = ::accept(mServerSocket, 0, 0);
	if(socket->mSocket < 0) {
		return NULL;
	}
	else {
		socket->mIsConnected = true;
		return socket;
	}
}

void ServerSocket::close() {
	if (mServerSocket >= 0) {
		::close(mServerSocket);
	}
	mIsClosed = true;
}

} /* namespace net */
} /* namespace android */
