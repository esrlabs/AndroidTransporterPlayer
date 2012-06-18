#ifndef RTSPSOCKET_H_
#define RTSPSOCKET_H_

#include "android/net/Socket.h"

class RtspSocket :
	public android::net::Socket
{
public:
	RtspSocket();
	RtspSocket(const char* host, uint16_t port);
	virtual ~RtspSocket() {}
	int32_t readPacket(uint8_t* data, const uint32_t size);

private:
	NO_COPY_CTOR_AND_ASSIGNMENT_OPERATOR(RtspSocket)
};


#endif /* RTSPSOCKET_H_ */
