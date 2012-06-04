#ifndef ANDROID_NET_SOCKET_H_
#define ANDROID_NET_SOCKET_H_

#include "Utils.h"
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

namespace android {
namespace net {

class ServerSocket;

class Socket
{
public:
	Socket();
	virtual ~Socket();

	bool connect(const char* ipAddress, const uint16_t port);
	int32_t read(uint8_t* data, const uint32_t size);
	int32_t readFully(uint8_t* data, const uint32_t size);
	bool write(const void* data, const uint32_t size);
	void close();
	bool isConnected() { return mIsConnected; }

private:
public:
	int32_t mSocketId;
private:
	struct sockaddr_in mSocket;
	bool mIsConnected;

	friend class ServerSocket;

	NO_COPY_CTOR_AND_ASSIGNMENT_OPERATOR(Socket)
};

} /* namespace net */
} /* namespace android */

#endif /*ANDROID_NET_SOCKET_H_*/
