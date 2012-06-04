#ifndef ANDROID_NET_DATAGRAMSOCKET_H_
#define ANDROID_NET_DATAGRAMSOCKET_H_

#include "Utils.h"
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

namespace android {
namespace net {

class DatagramSocket
{
public:
	DatagramSocket();
	DatagramSocket(const uint16_t port);
	virtual ~DatagramSocket();

	bool bind(const uint16_t port);
	bool bind(const char* ipAddress, const uint16_t port);
	int32_t recv(uint8_t* data, const uint32_t size, struct sockaddr_in* clientSocket = NULL);
	bool send(const void* data, const uint32_t size, struct sockaddr_in* clientSocket);
	void close();

private:
	int32_t mSocketId;
	struct sockaddr_in mSocket;

	NO_COPY_CTOR_AND_ASSIGNMENT_OPERATOR(DatagramSocket)
};

} /* namespace net */
} /* namespace android */

#endif /*ANDROID_NET_DATAGRAMSOCKET_H_*/
