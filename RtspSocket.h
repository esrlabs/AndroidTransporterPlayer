#ifndef RTSPSOCKET_H_
#define RTSPSOCKET_H_

#include "mindroid/net/Socket.h"
#include "mindroid/lang/String.h"
#include <map>

typedef std::map<mindroid::String, mindroid::String> RtspHeader;

class RtspSocket :
	public mindroid::Socket
{
public:

	RtspSocket();
	RtspSocket(const char* host, uint16_t port);
	virtual ~RtspSocket() {}
	mindroid::String readLine();
	RtspHeader* readPacketHeader();

private:
	NO_COPY_CTOR_AND_ASSIGNMENT_OPERATOR(RtspSocket)
};


#endif /* RTSPSOCKET_H_ */
