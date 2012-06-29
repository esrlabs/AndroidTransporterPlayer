#ifndef RTSPSOCKET_H_
#define RTSPSOCKET_H_

#include "android/net/Socket.h"
#include "android/lang/String.h"
#include <map>

typedef std::map<android::lang::String, android::lang::String> RtspHeader;

class RtspSocket :
	public android::net::Socket
{
public:

	RtspSocket();
	RtspSocket(const char* host, uint16_t port);
	virtual ~RtspSocket() {}
	android::lang::String readLine();
	RtspHeader* readPacket();

private:
	NO_COPY_CTOR_AND_ASSIGNMENT_OPERATOR(RtspSocket)
};


#endif /* RTSPSOCKET_H_ */
